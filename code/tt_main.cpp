/*
    TODO:
    * Allow to specify input filenames/paths from input arguments.
    * Config file?
* Clean up error printing code.
    * Convert to use Unicode?
    
    * Add sum of the month in the file. Can look like:
        // ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
        //           Sum of the month 2020-07-**        123:20
        // ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
*/


// NOTE: This program ignores concept of timezones to simplify usage.

#include "tt_main.h"

#include "tt_token.cpp"
#include "tt_string.cpp"
#include "tt_time.cpp"
#include "tt_error.cpp"

internal void load_file(Program_State *state);

inline Day *
get_day(Memory_Arena *day_arena, u32 day_index)
{
    Day *result = NULL;
    if (day_index < day_arena->count)
    {
        result = ((Day *)day_arena->base) + day_index;
    }
    
    return result;
}

internal Day *
get_last_day(Program_State *state)
{
    Day *last_day = get_day(&state->day_arena, state->day_arena.count - 1);
    return last_day;
}


internal char *
create_description(Memory_Arena *arena, Token token)
{
    // TODO: Maybe descriptions should go through hash table to avoid duplication.
    Assert(token.type == Token_String);
    
    char *result = Push_Array(arena, token.text_length + 1, char);
    char *dest = result;
    for (u32 index = 0;
         index < token.text_length; 
         ++index, ++dest)
    {
        *dest = token.text[index];
    }
    
    *dest = 0;
    return result;
}



internal Parse_Number_Result 
parse_number(char *src, s32 count)
{
    Parse_Number_Result result = {};
    
    s32 multiplier = 1;
    for (s32 index = count - 1;
         index >= 0;
         --index, multiplier *= 10)
    {
        if (!(src[index] >= '0' && src[index] <= '9')) return result;
        
        s32 to_add = (src[index] - '0') * multiplier;
        result.number += to_add;
    }
    
    result.success = true;
    return result;
}

internal Parse_Time_Result 
parse_date(Program_State *state, Token token)
{
    // NOTE: Supported format: 2020-12-31
    Parse_Time_Result result = {};
    
    if (token.text_length == (4 + 1 + 2 + 1 + 2))
    {
        char *text = token.text;
        tm date = {};
        
        // year    
        auto year = parse_number(text, 4);
        date.tm_year = year.number - 1900;
        text += 4;
        
        b32 dash1 = is_date_separator(*text++);
        
        // month
        auto month = parse_number(text, 2);
        date.tm_mon = month.number - 1;
        text += 2;
        
        b32 dash2 = is_date_separator(*text++);
        
        // day
        auto day = parse_number(text, 2);
        date.tm_mday = day.number;
        
        
        result.time = platform_tm_to_time(&date);
        result.success = (year.success && month.success && day.success && dash1 && dash2);
    }
    
    if (!result.success)
    {
        Print_Parse_Error(state);
        printf("Bad date format!");
        print_line_with_token(token);
        Print_Clear();
        printf("\n");
    }
    
    return result;
}

internal Parse_Time_Result 
parse_time(Program_State *state, Token token)
{
    // NOTE: Supported format: 10:32, 02:00, 2:0, 120...
    Parse_Time_Result result = {};
    
    if (token.text_length > 0)
    {
        b32 had_first_colon = false;
        u32 multiplier = 1;
        for (s32 index = (s32)token.text_length - 1;
             index >= 0;
             --index)
        {
            char c = token.text[index];
            
            if (c >= '0' && c <= '9')
            {
                u32 digit_value = (c - '0') * multiplier * 60;
                if (had_first_colon)
                {
                    digit_value *= 60;
                }
                
                result.time += digit_value;
                multiplier *= 10;
            }
            else if (is_time_separator(c))
            {
                if (had_first_colon)
                {
                    return result;
                }
                else
                {
                    had_first_colon = true;
                    multiplier = 1;
                }
            }
        }
        
        result.success = true;
    }
    
    if (!result.success)
    {
        Print_Parse_Error(state);
        printf("Bad time format!");
        print_line_with_token(token);
        Print_Clear();
        printf("\n");
    }
    
    return result;
}




internal void 
process_day(Program_State *state, Day *day, b32 print, FILE *current_file = NULL)
{
    Time_Entry *start = 0;
    day->missing_ending = MissingEnding_None;
    day->sum = 0;
    s32 offset_sum = 0;
    
    for (Time_Entry *entry = &day->first_time_entry;
         entry;
         entry = entry->next_in_day)
    {
        if (entry->type == Entry_Add ||
            entry->type == Entry_Subtract)
        {
            s32 value = (s32)entry->time;
            if (entry->type == Entry_Subtract) value *= -1;
            
            day->sum += value;
            
            if (print && entry->description)
            {
                using namespace Global_Color;
                print_offset(value);
                printf(" \t%s\"%s\"%s\n", f_desc, entry->description, f_reset);
            }
            else
            {
                offset_sum += value;
            }
        }
        else if (entry->type == Entry_Start)
        {
            if (start)
            {
                fprint_logic_error_message(state, current_file, "two start commands in a row - stop is missing");
                return;
            }
            
            start = entry;
        }
        else if (entry->type == Entry_Stop)
        {
            if (!start)
            {
                fprint_logic_error_message(state, current_file, "stop is missing its start");
                return;
            }
            
            
            if (entry->date == 0) entry->date = start->date;
            
            time_t start_time = start->date + start->time;
            time_t stop_time = entry->date + entry->time;
            
            if (stop_time < start_time)
            {
                if (stop_time + Days(1) > start_time)
                {
                    entry->date += Days(1);
                    stop_time += Days(1);
                }
            }
            
            if (stop_time < start_time)
            {
                fprint_logic_error_message(state, current_file, "stop time is earlier than start time");
                return;
            }
            
            
            day->sum += (s32)(stop_time - start_time);
            
            if (print) print_work_time_row(start, entry, offset_sum);
            
            start = NULL;
            offset_sum = 0;
        }
    }
    
    
    if (start) // NOTE: Day ends with start - missing end entry.
    {
        time_t now = get_current_timestamp(state);
        
        time_t start_time = start->date + start->time;
        
        if (now >= start_time &&
            now - Days(1) <= start_time)
        {
            day->missing_ending = MissingEnding_Assumed;
            day->sum += (s32)(now - start_time);
            
            if (print) 
            {
                print_work_time_row(start, NULL, offset_sum, "now");
                offset_sum = 0;
            }
        }
        else
        {
            day->missing_ending = MissingEnding_Critical;
            
            if (print) 
            {
                print_work_time_row(start, 0, offset_sum, "...");
                offset_sum = 0;
            }
            
            fprint_logic_error_message(state, current_file, "Missing last stop for a day");
        }
    }
    
    if (print && offset_sum != 0)
    {
        print_offset(offset_sum);
        printf("\n");
    }
}


internal void 
print_days_from_range(Program_State *state, time_t date_begin, time_t date_end, 
                      b32 alternative_colors = false)
{
    for (u32 day_index = 0;
         day_index < state->day_arena.count;
         ++day_index)
    {
        Day *day = get_day(&state->day_arena, day_index);
        
        if ((date_end) && (day->date_start > date_end)) break;
        if ((date_begin) && (day->date_start < date_begin)) continue;
        
        
        
        char date_str[64];
        get_date_string(date_str, sizeof(date_str), day->date_start);
        
        char day_of_week[32];
        get_day_of_the_week_string(day_of_week, sizeof(day_of_week), day->date_start);
        
        using namespace Global_Color;
        if (alternative_colors) 
        {
            printf("\n%s%s%s %s%s%s\n", f_black, b_date, date_str, day_of_week, f_reset, b_reset);
        }
        else
        {
            printf("\n%s%s %s%s\n", f_date, date_str, day_of_week, f_reset);
        }
        
        
        process_day(state, day, true);
        
        
        char sum_bar_str[MAX_SUM_AND_PROGRESS_BAR_STRING_SIZE];
        get_sum_and_progress_bar_string(sum_bar_str, sizeof(sum_bar_str), day);
        
        printf("%s%s%s\n", f_sum, sum_bar_str, f_reset);
    }
}



internal void 
archive_current_file(Program_State *state, b32 long_format = false)
{
    time_t now = get_current_timestamp(state);
    
    char timestamp[MAX_PATH];
    get_timestamp_string_for_file(timestamp, sizeof(timestamp), now, long_format);
    
    char archive_file_name[MAX_PATH];
    snprintf(archive_file_name, sizeof(archive_file_name), "%s%s_%s.txt", 
             state->archive_directory, state->input_file_name, timestamp);
    
    platform_copy_file(state->input_file_full_path, archive_file_name);
    
    if (long_format) printf("File archived as: %s\n", archive_file_name);
}


internal void 
save_to_file(Program_State *state)
{
    archive_current_file(state);
    state->logic_error_count = 0;
    state->parse_error_count = 0;
    
    FILE *file = fopen(state->input_file_full_path, "w");
    if (file)
    {
        for (u32 day_index = 0;
             day_index < state->day_arena.count;
             ++day_index)
        {
            Day *day = get_day(&state->day_arena, day_index);
            process_day(state, day, false, file);
            
            char day_of_week_str[16];
            get_day_of_the_week_string(day_of_week_str, sizeof(day_of_week_str), day->date_start);
            fprintf(file, "// %s\n", day_of_week_str);
            
            b32 first_entry = true;
            
            for (Time_Entry *entry = &day->first_time_entry;
                 entry;
                 entry = entry->next_in_day, first_entry = false)
            {
                char *command = NULL;
                if (entry->type == Entry_Start) command = "start";
                else if (entry->type == Entry_Stop) command = "stop";
                else if (entry->type == Entry_Add) command = "add";
                else if (entry->type == Entry_Subtract) command = "sub";
                else
                {
                    Invalid_Code_Path;
                    continue;
                }
                
                fprintf(file, "%s", command);
                
                
                
                if (first_entry || 
                    (entry->date != day->date_start))
                {
                    char date_str[MAX_TIMESTAMP_STRING_SIZE];
                    get_date_string(date_str, sizeof(date_str), entry->date);
                    fprintf(file, " %s", date_str);
                }
                
                
                
                char time_string[MAX_TIME_STRING_SIZE];
                get_time_string(time_string, sizeof(time_string), entry->time);
                fprintf(file, " %s", time_string);
                
                
                
                
                if (entry->description)
                {
                    fprintf(file, " \t\"%s\"", entry->description);
                }
                
                
                
                fprintf(file, ";\n");
            }
            
            
            
            char sum_bar_str[MAX_SUM_AND_PROGRESS_BAR_STRING_SIZE];
            get_sum_and_progress_bar_string(sum_bar_str, sizeof(sum_bar_str), day);
            fprintf(file, "// %s\n\n", sum_bar_str);
        }
        
        using namespace Global_Color;
        
        if (state->parse_error_count > 0)
        {
            fprintf(file, "// Parse Error count: %d\n", state->parse_error_count);
            printf("%sParse Error count: %d%s\n", b_error, state->parse_error_count, b_reset);
        }
        
        if (state->logic_error_count > 0)
        {
            
            fprintf(file, "// Logic Error count: %d\n", state->logic_error_count);
            printf("%sLogic Error count: %d%s\n", b_error, state->logic_error_count, b_reset);
        }
        
        fclose(file);
        
        state->loaded_input_mod_time = platform_get_file_mod_time(state->input_file_full_path);
    }
    else
    {
        printf("Failed to write to file: %s\n", state->input_file_full_path);
    }
    
    state->change_count = 0;
}


internal b32
automatic_save_to_file(Program_State *state)
{
    b32 did_save = false;
    
    if (state->change_count > 0)
    {
        if ((state->logic_error_count == 0))
        {
            if ((state->parse_error_count == 0))
            {
                save_to_file(state);
                did_save = true;
            }
            else
            {
                Print_Error();
                printf("Can't do automatic save due to "
                       "parse errors (%d). "
                       "Use 'save' command to force save.",
                       state->parse_error_count);
                Print_ClearN();
            }
        }
        else
        {
            Print_Error();
            printf("Can't do automatic save due to "
                   "logic errors (%d). "
                   "Use 'save' command to force save.",
                   state->logic_error_count);
            Print_ClearN();
        }
    }
    
    return did_save;
}



internal void 
process_time_entry(Program_State *state, Time_Entry *entry)
{
    // TODO: Implement insert sorting for start/stop/add/sub.                
    
    
    // NOTE: Select correct day slot.
    Day *day = get_day(&state->day_arena, state->day_arena.count - 1);
    
    if (day == NULL ||
        ((day->date_start < entry->date) && (entry->type != Entry_Stop)))
    {
        if (entry->date == 0)
        {
            Print_Error();
            printf("[Warning] First item can't have missing date! - 1970 assumed");
            Print_ClearN();
            
            Invalid_Code_Path;
            
            entry->date += Days(1);
        }
        
        // NOTE: Add new day
        day = Push_Struct(&state->day_arena, Day);
        *day = {};
        day->date_start = entry->date;
    }
    else if (entry->date == 0)
    {
        entry->date = day->date_start;
    }
    else if ((entry->date > day->date_start) && (entry->type != Entry_Stop))
    {
        if (state->reading_from_file)
        {
            Print_Logic_Error(state);
            printf("Out of order item insertion not supported");
            Print_ClearN();
            
            Invalid_Code_Path;
        }
        else
        {
            Print_Error();
            printf("Can't insert entry out of order");
            Print_ClearN();
            return;
        }
    }
    
    
    day->sum = EMPTY_SUM; // NOTE: Invalidate previous sum calculations for a day.
    
    
    // NOTE: Add Time_Entry to previously selected day slot.
    Time_Entry *entry_dest = &day->first_time_entry;
    while (entry_dest->type != Entry_None)
    {
        if (entry_dest->next_in_day == NULL)
        {
            entry_dest->next_in_day = Push_Struct(&state->element_arena, Time_Entry);
            *entry_dest->next_in_day = {};
        }
        
        entry_dest = entry_dest->next_in_day;
    }
    
    ++state->change_count;
    *entry_dest = *entry;
}



internal void
prase_command_start_stop(Program_State *state, Tokenizer *tokenizer, Entry_Type type)
{
    Assert(type == Entry_Start || type == Entry_Stop);
    
    b32 success = true;
    Forward_Token forward = create_forward_token(tokenizer);
    
    Time_Entry time_entry = {};
    time_entry.type = type;
    
    // argument 1 - optional - date
    if (forward.peek.type == Token_Date) 
    {
        advance_forward_token(&forward);
        
        Parse_Time_Result parsed_date = parse_date(state, forward.token);
        if (parsed_date.success)
        {
            time_entry.date = parsed_date.time;
        }
        else success = false;
    }
    else
    {
        if (!state->reading_from_file) time_entry.date = get_today(state);
        else
        {
            Day *last_day = get_last_day(state);
            if (last_day) time_entry.date = last_day->date_start;
            else
            {
                success = false;
                Print_Parse_Error(state);
                printf("First entry needs to specify date. ");
                Print_Clear();
            }
        }
    }
    
    
    // argument 2 - optional(cmd) required(file) - time
    if (success && (forward.peek.type == Token_Time))
    {
        advance_forward_token(&forward);
        
        Parse_Time_Result parsed_time = parse_time(state, forward.token);
        if (parsed_time.success)
        {
            time_entry.time = parsed_time.time;
        }
        else success = false;
    }
    else 
    {
        if (!state->reading_from_file)
        {
            time_entry.time = get_time(state);
        }
        else success = false;
    }
    
    
    // argument 3 - optional - description
    if (success && (forward.peek.type == Token_String))
    {
        advance_forward_token(&forward);
        
        time_entry.description = create_description(&state->element_arena, forward.token);
    }
    
    
    if (success)
    {
        process_time_entry(state, &time_entry);
    }
    else
    {
        Print_Parse_Error(state);
        printf("Incorect command usage. Use:\n"
               "%s [yyyy-MM-dd] (hh:mm) [\"description\"]",
               (type == Entry_Add) ? "add" : "sub");
        print_line_with_token(forward.token);
        Print_ClearN();
    }
}



internal void
parse_command_add_sub(Program_State *state, Tokenizer *tokenizer, Entry_Type type)
{
    Assert(type == Entry_Add || type == Entry_Subtract);
    
    b32 success = true;
    Forward_Token forward = create_forward_token(tokenizer);
    
    Time_Entry time_entry = {};
    time_entry.type = type;
    
    // argument 1 - optional - date
    if (forward.peek.type == Token_Date) 
    {
        advance_forward_token(&forward);
        
        Parse_Time_Result parsed_date = parse_date(state, forward.token);
        if (parsed_date.success)
        {
            time_entry.date = parsed_date.time;
        }
        else success = false;
    }
    else
    {
        if (!state->reading_from_file) time_entry.date = get_today(state);
        else
        {
            Day *last_day = get_last_day(state);
            if (last_day) time_entry.date = last_day->date_start;
            else
            {
                success = false;
                Print_Parse_Error(state);
                printf("First entry needs to specify date. ");
                Print_Clear();
            }
        }
    }
    
    
    // argument 2 - required - time
    if (success && (forward.peek.type == Token_Time))
    {
        advance_forward_token(&forward);
        
        Parse_Time_Result parsed_time = parse_time(state, forward.token);
        if (parsed_time.success)
        {
            time_entry.time = parsed_time.time;
            
            // argument 3 - optional - description
            if (forward.peek.type == Token_String)
            {
                advance_forward_token(&forward);
                
                time_entry.description = create_description(&state->element_arena, forward.token);
            }
        }
        else success = false;
    }
    else success = false;
    
    
    if (success)
    {
        process_time_entry(state, &time_entry);
    }
    else
    {
        Print_Parse_Error(state);
        printf("Incorect command usage. Use:\n"
               "%s [yyyy-MM-dd] (hh:mm) [\"description\"]",
               (type == Entry_Add) ? "add" : "sub");
        print_line_with_token(forward.token);
        Print_ClearN();
    }
}



struct Time_Range_Result
{
    b32 is_valid;
    union
    {
        time_t date_ranges[2];
        struct
        {
            time_t begin;
            time_t end; // NOTE: Inclusive end
        };
    };
};

internal Time_Range_Result
parse_time_range(Program_State *state, Tokenizer *tokenizer)
{
    Time_Range_Result result = {};
    result.is_valid = true;
    
    Forward_Token forward = create_forward_token(tokenizer);
    
    char *date_range_identifiers[2] = {"from", "to"};
    
    // arg 1 (from), 2 (date), 3 (to), 4 (date)
    for (s32 date_index = 0;
         date_index < Array_Count(result.date_ranges);
         ++date_index)
    {
        time_t *date = result.date_ranges + date_index;
        char *identifier = date_range_identifiers[date_index];
        
        // arg - optional - "from/to"
        if (result.is_valid &&
            (forward.peek.type == Token_Identifier) &&
            (token_equals(forward.peek, identifier)))
        {
            advance_forward_token(&forward);
            
            // arg - required - date/today
            if (forward.peek.type == Token_Date)
            {
                advance_forward_token(&forward);
                
                Parse_Time_Result parsed_date = parse_date(state, forward.token);
                if (parsed_date.success)
                {
                    *date = parsed_date.time;
                }
                else result.is_valid = false;
            }
            else if ((forward.peek.type == Token_Identifier) && 
                     token_equals(forward.peek, "today"))
            {
                advance_forward_token(&forward);
                
                *date = get_today(state);
            }
            else result.is_valid = false;
        }
    }
    
    
    // arg - optional date
    if (result.is_valid &&
        (!result.begin && !result.end) &&
        (forward.peek.type == Token_Date))
    {
        advance_forward_token(&forward);
        
        Parse_Time_Result parsed_date = parse_date(state, forward.token);
        if (parsed_date.success)
        {
            result.begin = parsed_date.time;
            result.end = parsed_date.time;
        }
        else result.is_valid = false;
    }
    
    return result;
}



internal void
parse_command_show(Program_State *state, Tokenizer *tokenizer)
{
    Forward_Token forward = create_forward_token(tokenizer);
    
    Time_Range_Result range = parse_time_range(state, tokenizer);
    
    if (range.is_valid)
    {
        print_days_from_range(state, range.begin, range.end);
    }
    else
    {
        Print_Parse_Error(state);
        printf("Incorect command usage. Use:\n"
               "show [yyyy-MM-dd]\n"
               "show [from yyyy-MM-dd] [to yyyy-MM-dd]\n"
               );
        print_line_with_token(forward.token);
        Print_ClearN();
    }
}



enum Granularity // TODO: Support more granularities
{
    Granularity_Days,
    //Granularity_Weeks,
    Granularity_Months,
    //Granularity_Quarters,
    //Granularity_Years
};


internal void
process_and_print_summary(Program_State *state, Granularity granularity, 
                          time_t date_begin, time_t date_end)
{
    // TODO: Support more granularities. Now this only supports Days.
    
    Boundries_Result boundries = {};
    time_t time_sum = 0;
    
    for (u32 day_index = 0;
         day_index <= state->day_arena.count;
         ++day_index)
    {
        b32 last_loop = day_index == state->day_arena.count;
        Day *day = get_day(&state->day_arena, day_index);
        Assert(last_loop || day);
        
        if (!last_loop)
        {
            if ((date_end) && (day->date_start > date_end))
            {
                day_index = state->day_arena.count;
                last_loop = true;
            }
            if ((date_begin) && (day->date_start < date_begin))
            {
                continue;
            }
        }
        
        
        if (last_loop ||
            (day->date_start >= boundries.one_day_past_end))
        {
            if (boundries.day_count > 0)
            {
                char date_str[64];
                get_date_string(date_str, sizeof(date_str), boundries.begin);
                
                char sum_bar_str[MAX_SUM_AND_PROGRESS_BAR_STRING_SIZE + MAX_TIME_STRING_SIZE + 5];
                {
                    time_t day_count = boundries.day_count;
                    if (last_loop)
                    {
                        time_t today = get_today(state);
                        day_count = (today / (Days(1))) - (boundries.begin / (Days(1)));
                        if (day_count <= 0) day_count = 1;
                    }
                    
                    time_t time_avg = time_sum / day_count;
                    
                    char sum_str[MAX_TIME_STRING_SIZE];
                    get_time_string(sum_str, sizeof(sum_str), time_sum);
                    
                    char bar_str[MAX_PROGRESS_BAR_SIZE];
                    get_progress_bar_string(bar_str, sizeof(bar_str), time_avg, MissingEnding_None);
                    
                    if (day_count > 1)
                    {
                        char avg_str[MAX_TIME_STRING_SIZE];
                        get_time_string(avg_str, sizeof(avg_str), time_avg);
                        
                        snprintf(sum_bar_str, sizeof(sum_bar_str), 
                                 "sum: %s\tavg(/%llud): %s\t%s", 
                                 sum_str, day_count, avg_str, bar_str);
                    }
                    else
                    {
                        snprintf(sum_bar_str, sizeof(sum_bar_str), "sum: %s\t%s", 
                                 sum_str, bar_str);
                    }
                }
                
                using namespace Global_Color;
                printf("%s%s\t%s%s%s\n", f_date, date_str, f_sum, sum_bar_str, f_reset);
            }
            
            time_sum = 0;
            
            if (!last_loop)
            {
                switch (granularity)
                {
                    case Granularity_Months:
                    {
                        boundries = get_month_boundries(day->date_start);
                    } break;
                    
                    
                    default: Invalid_Code_Path;
                    case Granularity_Days:
                    {
                        boundries.day_count = 1;
                        boundries.begin = day->date_start;
                        boundries.one_day_past_end = day->date_start + Days(1);
                    } break;
                }
            }
        }
        
        
        
        if (!last_loop)
        {
            if (day->sum == EMPTY_SUM)
            {
                process_day(state, day, false);
            }
            
            time_sum += day->sum;
        }
    }
}



internal void
parse_command_summary(Program_State *state, Tokenizer *tokenizer)
{
    Forward_Token forward = create_forward_token(tokenizer);
    
    Granularity granularity = Granularity_Days;
    // arg - optional - granularity (days/months/years)
    if (forward.peek.type == Token_Identifier)
    {
        if (token_equals(forward.peek, "days") ||
            token_equals(forward.peek, "d"))
        {
            advance_forward_token(&forward);
            granularity = Granularity_Days;
        }
        else if (token_equals(forward.peek, "months") ||
                 token_equals(forward.peek, "m"))
        {
            advance_forward_token(&forward);
            granularity = Granularity_Months;
        }
#if 0
        else if (token_equals(forward.peek, "years") ||
                 token_equals(forward.peek, "y"))
        {
            advance_forward_token(&forward);
            granularity = Granularity_Years;
        }
#endif
    }
    
    // arg - optional - time range
    Time_Range_Result range = parse_time_range(state, tokenizer);
    
    
    if (range.is_valid)
    {
        process_and_print_summary(state, granularity, range.begin, range.end);
    }
    else
    {
        Print_Parse_Error(state);
        printf("Incorect command usage. Use:\n"
               "summary [granularity]\n"
               "summary [granularity] [yyyy-MM-dd]\n"
               "summary [granularity] [from yyyy-MM-dd] [to yyyy-MM-dd]\n"
               "\n\tgranularity - days/months (or d/m)\n"
               );
        print_line_with_token(forward.token);
        Print_ClearN();
    }
    
}


internal void
parse_command_exit(Program_State *state, Tokenizer *tokenizer, b32 *main_loop_is_running)
{
    Assert(main_loop_is_running != NULL);
    
    Forward_Token forward = create_forward_token(tokenizer);
    if ((forward.peek.type == Token_Identifier) &&
        (token_equals(forward.peek, "no-save")))
    {
        advance_forward_token(&forward);
        *main_loop_is_running = false;
    }
    else
    {
        b32 did_save = automatic_save_to_file(state);
        
        if (did_save || 
            (state->change_count == 0))
        {
            *main_loop_is_running = false;
        }
        else
        {
            Print_Error();
            printf("To exit without saving use: exit no-save");
            Print_ClearN();
        }
    }
}


internal void 
process_input(char *content, Program_State *state, b32 reading_from_file, 
              b32 *main_loop_is_running = NULL)
{
    state->reading_from_file = reading_from_file;
    
    using namespace Global_Color;
    Tokenizer tokenizer = create_tokenizer(content);
    bool parsing = true;
    
    
    while (parsing)
    {
        Token token = get_token(&tokenizer);
        switch (token.type)
        {
            
            case Token_Identifier:
            {
                if (token_equals(token, "start")) 
                {
                    prase_command_start_stop(state, &tokenizer, Entry_Start);
                }
                else if (token_equals(token, "stop"))
                {
                    prase_command_start_stop(state, &tokenizer, Entry_Stop);
                }
                else if (token_equals(token, "add"))
                {
                    parse_command_add_sub(state, &tokenizer, Entry_Add);
                }
                else if (token_equals(token, "subtract") ||
                         token_equals(token, "sub"))
                {
                    parse_command_add_sub(state, &tokenizer, Entry_Subtract);
                }
                else if (token_equals(token, "show"))
                {
                    if (state->reading_from_file) Command_Line_Only_Message(state, token);
                    else                          parse_command_show(state, &tokenizer);     
                }
                else if (token_equals(token, "summary"))
                {
                    if (state->reading_from_file) Command_Line_Only_Message(state, token);
                    else                          parse_command_summary(state, &tokenizer);     
                }
                else if (token_equals(token, "exit"))
                {
                    if (state->reading_from_file) Command_Line_Only_Message(state, token);
                    else
                    {
                        parse_command_exit(state, &tokenizer, main_loop_is_running);
                    }
                }
                else if (token_equals(token, "save"))
                {
                    if (state->reading_from_file) Command_Line_Only_Message(state, token);
                    else
                    {
                        save_to_file(state);
                        printf("File saved\n");
                    }
                }
                else if (token_equals(token, "archive"))
                {
                    if (state->reading_from_file) Command_Line_Only_Message(state, token);
                    else
                    {
                        archive_current_file(state, true);
                    }
                }
                else if (token_equals(token, "load"))
                {
                    if (state->reading_from_file) Command_Line_Only_Message(state, token);
                    else
                    {
                        load_file(state);
                    }
                }
                else if (token_equals(token, "time"))
                {
                    if (state->reading_from_file) Command_Line_Only_Message(state, token);
                    else
                    {
                        time_t now = get_current_timestamp(state);
                        char now_str[MAX_TIMESTAMP_STRING_SIZE];
                        get_timestamp_string(now_str, sizeof(now_str), now);
                        printf("Current time: %s\n", now_str);
                    }
                }
                else if (token_equals(token, "edit"))
                {
                    if (state->reading_from_file) Command_Line_Only_Message(state, token);
                    else
                    {
                        platform_open_in_default_editor(state->input_file_full_path);
                    }
                }
                else if (token_equals(token, "clear"))
                {
                    if (state->reading_from_file) Command_Line_Only_Message(state, token);
                    else
                    {
                        platform_clear_screen();
                    }
                }
                else if (token_equals(token, "help"))
                {
                    if (state->reading_from_file) Command_Line_Only_Message(state, token);
                    else
                    {
                        printf("Commands available everywhere:\n"
                               "start yyyy-MM-dd hh:mm\tstarts new timespan\n"
                               "stop yyyy-MM-dd hh:mm\tstops current timespan\n"
                               "add hh:mm\t\tadds time to current day\n"
                               "add yyyy-MM-dd hh:mm\n"
                               "sub hh:mm\t\tsubtracts time from current day\n"
                               "sub yyyy-MM-dd hh:mm\n"
                               
                               "\nCommands available only in console:\n"
                               "start\t\t\tassumes current time when unspecified\n"
                               "stop\n"
                               "show\t\t\tshows current history\n"
                               "show from yyyy-MM-dd to yyyy-MM-dd\n"
                               "show yyyy-MM-dd\n"
                               "time\t\t\tshows current time\n"
                               "clear\t\t\tclears the screen\n"
                               "edit\t\t\topens database file in your default editor\n"
                               "\t\t\tworks best if your editor supports hot-loading\n"
                               
                               "\nThese actions happen automatically:\n"
                               "save\t\t\tforces save\n"
                               "archive\t\t\tforces backup\n"
                               "load\t\t\tforces load from file\n"
                               );
                    }
                }
                else
                {
                    Print_Parse_Error(state);
                    printf("%.*s - unexpected identifier", (s32)token.text_length, token.text); \
                    print_line_with_token(token); \
                    Print_ClearN();
                }
                
            } break;
            
            
            case Token_End_Of_Stream: 
            {
                parsing = false;
            } break;
            
            case Token_Semicolon:
            {
            } break;
            
            default:
            {
                Print_Parse_Error(state);
                printf("%.*s - unexpected element", (s32)token.text_length, token.text); \
                print_line_with_token(token); \
                Print_ClearN();
            } break;
            
        }
    }
    
    
    state->reading_from_file = false;
}


internal void 
clear_memory(Program_State *state)
{
    clear_arena(&state->day_arena);
    clear_arena(&state->element_arena);
}


internal void 
load_file(Program_State *state)
{
    clear_memory(state);
    state->parse_error_count = 0;
    
    char *file_name = state->input_file_full_path;
    char *file_content = read_entire_file_and_null_terminate(&state->element_arena, file_name);
    if (file_content)
    {
        process_input(file_content, state, true);
        
        state->loaded_input_mod_time = platform_get_file_mod_time(state->input_file_full_path);
        printf("File loaded\n");
    }
    else
    {
        printf("[Parse Error #%d] Failed to open the file: %s\n",
               state->parse_error_count++, file_name);
    }
}


internal void 
read_from_keyboard(Thread_Memory *thread_memory)
{
    for (;;)
    {
        if (thread_memory->new_data)
        {
            platform_sleep(1);
            // NOTE: Spinlock while waiting for main thread to process work.
        }
        else
        {
            printf(thread_memory->cursor);
            fgets(thread_memory->input_buffer, sizeof(thread_memory->input_buffer), stdin);
            thread_memory->new_data = true;
        }
    }
}




enum Cmd_Arugment_Type
{
    Cmd_None,
    Cmd_Input_File_Path
};

int main(int argument_count, char **arguments)
{
    //~ NOTE: Initialization 
    
    Program_State state = {};
    
    // NOTE: Initialize arenas:
    static u8 raw_memory_block[Gigabytes(1)];
    alocate_arena(&state.element_arena, raw_memory_block, Megabytes(512));
    alocate_arena(&state.day_arena, raw_memory_block + Megabytes(512), Megabytes(512));
    clear_memory(&state);
    
    b32 reformat_mode = false;
    
    { //~ SCOPE: Processing of command line arugments
        Cmd_Arugment_Type type = Cmd_None;
        b32 disable_colors = false;
        
        for (s32 argument_index = 1;
             argument_index < argument_count;
             ++argument_index)
        {
            char *arg = arguments[argument_index];
            
            if (type == Cmd_None)
            {
                if (arg[0] == '-' || arg[0] == '/' || arg[0] == '?')
                {
                    if (arg[0] == '?' || arg[1] == '?' || arg[1] == 'h')
                    {
                        printf("tt.exe [-d database.txt] [-m]"
                               "\n" "Options:"
                               "\n\t" "-d" "\t\t" "Selects file to load and save data from."
                               "\n\t" "-m" "\t\t" "Mono mode. Turns off colors."
                               "\n");
                        exit(0);
                    }
                    else if (arg[1] == 'd')
                    {
                        type = Cmd_Input_File_Path;
                    }
                    else if (arg[1] == 'm')
                    {
                        disable_colors = true;
                    }
                    else if (arg[1] == 'r')
                    {
                        reformat_mode = true;
                    }
                    else
                    {
                        printf("Unknown switch argument: %s\n", arg);
                        exit(0);
                    }
                }
            }
            else if (type == Cmd_Input_File_Path)
            {
                strncpy(state.input_file_full_path, arg, Array_Count(state.input_file_full_path));
                
                type = Cmd_None;
            }
            else
            {
                Invalid_Code_Path;
            }
        }
        
        
        initialize_colors(disable_colors);
    }
    
    
    initialize_timezone_offset(&state);
    
    
    char executable_directory[MAX_PATH];
    platform_get_executable_path(executable_directory, sizeof(executable_directory));
    terminate_string_after_last_slash(executable_directory);
    
    
    if (!state.input_file_full_path[0])
    {
        strncpy(state.input_file_name, "time_tracker.txt", Array_Count(state.input_file_name));
        snprintf(state.input_file_full_path, Array_Count(state.input_file_full_path), 
                 "%s%s", executable_directory, state.input_file_name);
    }
    else
    {
        char *file_name = get_after_last_slash_pointer(state.input_file_full_path);
        if (file_name)
        {
            strncpy(state.input_file_name, file_name, Array_Count(state.input_file_name));
        }
        else
        {
            strncpy(state.input_file_name, state.input_file_full_path, Array_Count(state.input_file_name));
        }
    }
    
    sprintf(state.archive_directory, "%sarchive", executable_directory);
    platform_add_ending_slash_to_path(state.archive_directory);
    platform_create_directory(state.archive_directory);
    
    
    
    //~ NOTE(mautesz): Load file. Save with better formatting if there was no load errors.
    
    load_file(&state);
    b32 did_save_on_first_load = false;
    if (state.parse_error_count == 0)
    {
        print_days_from_range(&state, 0, 0);
        save_to_file(&state);
        did_save_on_first_load = true;
    }
    else
    {
        state.change_count = 0;
    }
    
    if (reformat_mode)
    {
        if (did_save_on_first_load) printf("File reformated. Exiting.\n");
        else                        printf("Reformatting skipped due to errors. Exiting.\n");
        
        exit(0);
    }
    
    
    //~ NOTE: Initialize input thread.
    
    Thread_Memory thread_memory = {};
    sprintf(thread_memory.cursor, "::>");
    platform_create_thread(read_from_keyboard, &thread_memory);
    
    
    
    
    
    //~ NOTE: Main loop
    
    b32 is_running = true;
    while (is_running)
    {
        if (thread_memory.new_data)
        {
            process_input(thread_memory.input_buffer, &state, false, &is_running);
            if (state.change_count > 0)
            {
                if (state.day_arena.count > 0)
                {
                    Day *last_day = get_last_day(&state);
                    print_days_from_range(&state, last_day->date_start, last_day->date_start, true);
                }
                automatic_save_to_file(&state);
            }
            
            thread_memory.new_data = false;
        }
        
        
        
        auto current_input_mod_time = platform_get_file_mod_time(state.input_file_full_path);
        b32 source_file_changed =  (platform_compare_file_time(
                                                               state.loaded_input_mod_time, current_input_mod_time) != 0);
        
        if (source_file_changed)
        {
            load_file(&state);
            state.change_count = 0;
            printf(thread_memory.cursor);
        }
        
        
        platform_sleep(33);
    }
    
    return 0;
}