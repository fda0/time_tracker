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
        result.time += to_add;
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
        date.tm_year = year.time - 1900;
        
        text += 4;
        b32 dash1 = (*text++ == '-');
        
        // month
        auto month = parse_number(text, 2);
        date.tm_mon = month.time - 1;
        
        text += 2;
        b32 dash2 = (*text++ == '-');
        
        // day
        auto day = parse_number(text, 2);
        date.tm_mday = day.time;
        
        
        result.time = platform_tm_to_time(&date);
        result.success = (year.success && month.success && day.success && dash1 && dash2);
    }
    
    if (!result.success)
    {
        Print_Load_Error(state);
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
            else if (c == ':')
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
        Print_Load_Error(state);
        printf("Bad time format!");
        print_line_with_token(token);
        Print_Clear();
        printf("\n");
    }
    
    return result;
}




internal void 
calculate_day_sum_and_validate(Program_State *state, Day *day, b32 print, FILE *errors_to_file = NULL)
{
    
#define PRINT_ERROR(Message) \
if (errors_to_file) \
{ \
fprintf(errors_to_file, "// " Message, \
state->save_error_count++); \
} \
else \
{ \
printf("%s" Message "%s", \
b_error, state->save_error_count++, \
b_reset); \
}
    
    
    using namespace Global_Color;
    
    Time_Entry *start = 0;
    day->missing = Missing_None;
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
                PRINT_ERROR("[Error #%d] two start commands in a row - stop is missing\n");
                return;
            }
            
            start = entry;
        }
        else if (entry->type == Entry_Stop)
        {
            if (!start)
            {
                PRINT_ERROR("[Error #%d] stop is missing its start\n");
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
                PRINT_ERROR("[Error #%d] stop time is earlier than start time\n");
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
        time_t now = get_current_time(state);
        
        time_t start_time = start->date + start->time;
        
        if (now >= start_time &&
            now - Days(1) <= start_time)
        {
            day->missing = Missing_Assumed;
            day->sum += (s32)(now - start_time);
            
            if (print) 
            {
                print_work_time_row(start, NULL, offset_sum, "now");
                offset_sum = 0;
            }
        }
        else
        {
            day->missing = Missing_Critical;
            
            if (print) 
            {
                print_work_time_row(start, 0, offset_sum, "...");
                offset_sum = 0;
            }
            
            PRINT_ERROR("[Error #%d] Missing last stop for a day\n");
        }
    }
    
    if (print && offset_sum != 0)
    {
        print_offset(offset_sum);
        printf("\n");
    }
}


internal void 
print_days_from_range(Program_State *state, time_t *date_start, time_t *date_stop, b32 alternative_colors = false)
{
    for (u32 day_index = 0;
         day_index < state->day_arena.count;
         ++day_index)
    {
        Day *day = get_day(&state->day_arena, day_index);
        
        if ((date_stop) && (day->date_start > *date_stop)) break;
        if ((date_start) && (day->date_start < *date_start)) continue;
        
        
        
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
        
        
        calculate_day_sum_and_validate(state, day, true);
        
        char sum_bar_str[MAX_SUM_AND_PROGRESS_BAR_STRING_SIZE];
        get_sum_and_progress_bar_string(sum_bar_str, sizeof(sum_bar_str), day);
        
        printf("%s%s%s\n", f_sum, sum_bar_str, f_reset);
    }
}



internal void 
archive_current_file(Program_State *state, b32 long_format = false)
{
    time_t now = get_current_time(state);
    
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
    state->save_error_count = 0;
    
    
    
    FILE *file = fopen(state->input_file_full_path, "w");
    if (file)
    {
        for (u32 day_index = 0;
             day_index < state->day_arena.count;
             ++day_index)
        {
            Day *day = get_day(&state->day_arena, day_index);
            calculate_day_sum_and_validate(state, day, false, file);
            
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
        
        if (state->load_error_count > 0)
        {
            fprintf(file, "// Load error count: %d\n", state->load_error_count);
            printf("%sLoad error count: %d%s\n", b_error, state->load_error_count, b_reset);
        }
        
        if (state->save_error_count > 0)
        {
            
            fprintf(file, "// Save error count: %d\n", state->save_error_count);
            printf("%sSave error count: %d%s\n", b_error, state->save_error_count, b_reset);
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


internal void 
process_time_entry(Program_State *state, Time_Entry *entry, b32 reading_from_file)
{
    // TODO: This code shouldn't know about file/command mode
    
    
    // TODO: Implement insert sorting for start/stop/add/sub.                
    using namespace Global_Color;
    
    if (!reading_from_file) // TODO: Pull out from here.
    {
        // NOTE: In terminal mode you don't need to specify date/hour.
        // Missing data is filled with current time.
        
        b32 missing_date = (entry->date == 0);
        b32 missing_time = (entry->time == 0);
        
        if (missing_date || missing_time)
        {
            time_t now = get_current_time(state);
            
            if (missing_date)
            {
                time_t today = truncate_to_date(now);
                entry->date = today;
            }
            
            if (missing_time)
            {
                if (entry->type == Entry_Start || 
                    entry->type == Entry_Stop)
                {
                    time_t time = truncate_to_time(now);
                    entry->time = time;
                }
                else
                {
                    printf("%sYou need to specify time (example: 01:10)%s\n", b_error, b_reset);
                    return;
                }
            }
        }
    }
    
    
    // NOTE: Select correct day slot.
    Day *day = get_day(&state->day_arena, state->day_arena.count - 1);
    
    if (day == NULL ||
        ((day->date_start < entry->date) && (entry->type != Entry_Stop)))
    {
        if (entry->date == 0)
        {
            Invalid_Code_Path;
            printf("%s[Warning] First item can't have missing date! - 1970 assumed :(%s\n",  
                   b_error, b_reset);
            
            entry->date += Days(1);
        }
        
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
        if (reading_from_file)
        {
            printf("%s[Error #%d] Out of order item insertion not supported yet :(%s\n", 
                   b_error, state->save_error_count++, b_reset);
            Invalid_Code_Path;
        }
        else
        {
            printf("%sCan't insert entry into the past yet :(%s\n", b_error, b_reset);
            return;
        }
    }
    
    
    
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



inline void
set_flag(Processing_Data *buffer, Instruction_Flag flag)
{
    buffer->flags |= flag;
}


inline b32
is_flag_set(Processing_Data *buffer, Instruction_Flag flag)
{
    b32 result = buffer->flags & flag;
    return result;
}


inline b32 fill_time_slot(time_t *target, time_t *source)
{
    b32 result = false;
    if (*target == 0)
    {
        result = true;
        *target = *source;
    }
    
    return result;
}


internal void
process_offset_time_entry(Program_State *state, Tokenizer *tokenizer, Entry_Type type, b32 in_command_line)
{
    Assert(type == Entry_Add || type == Entry_Subtract);
    
    b32 success = true;
    Token token = get_token(tokenizer);
    Time_Entry time_entry = {};
    time_entry.type = type;
    
    // argument 1 - optional - date
    if (token.type == Token_Date) 
    {
        Parse_Time_Result parsed_date = parse_date(state, token);
        if (parsed_date.success)
        {
            time_entry.date = parsed_date.time;
            token = get_token(tokenizer);
        }
        else success = false;
    }
    else
    {
        if (in_command_line) time_entry.date = get_today(state);
        else
        {
            Day *last_day = get_last_day(state);
            if (last_day) time_entry.date = last_day->date_start;
            else
            {
                success = false;
                Print_Load_Error(state);
                printf("First entry needs to specify date. ");
                Print_Clear();
            }
        }
    }
    
    
    // argument 2 - required - time
    if (success && (token.type == Token_Time))
    {
        Parse_Time_Result parsed_time = parse_time(state, token);
        if (parsed_time.success)
        {
            time_entry.time = parsed_time.time;
            
            // argument 3 - optional - description
            if (peek_token(tokenizer).type == Token_String)
            {
                token = get_token(tokenizer);
                time_entry.description = create_description(&state->element_arena, token);
            }
        }
        else success = false;
    }
    else success = false;
    
    
    if (!success)
    {
        Print_Load_Error(state);
        printf("Incorect command usage. Use: "
               "(add/sub) [date] (time) [\"description\"]\n");
        print_line_with_token(token);
        Print_ClearN();
        
        if (token.type == Token_Identifier) rewind_tokenizer(tokenizer);
    }
    else
    {
        process_time_entry(state, &time_entry, !in_command_line);
    }
}


internal void 
process_input(char *content, Program_State *state, b32 reading_from_file, b32 *main_loop_is_running = NULL)
{
    state->reading_from_file = reading_from_file;
    
    using namespace Global_Color;
    Tokenizer tokenizer = create_tokenizer(content);
    Processing_Data buffer = {};
    bool parsing = true;
    
    
    while (parsing)
    {
        Token token = get_token(&tokenizer);
        switch (token.type)
        {
            //~ NOTE: Token macro helpers. To add/prohibit usage of certian features.
            
#define Print_Not_Supported_In_File \
{ \
printf("%s%.*s keyword is not supported in files%s\n", \
b_error, (s32)token.text_length, \
token.text, b_reset); \
}
            
            
#define Program_Interface_Only  \
if (reading_from_file) \
{  \
Print_Not_Supported_In_File; \
buffer.instruction = Ins_Unsupported; \
continue; \
}
            
            
#define Continue_If_Instruction_Already_Set \
if (buffer.instruction != Ins_None) \
{ \
printf("%s%.*s is ignored" \
" - other instruction is already set%s\n", \
b_error, (s32)token.text_length, \
token.text, b_reset); \
continue; \
}
            
            
            // end of macros
            
            
            //~ NOTE: Identifiers.
            case Token_Identifier:
            {
                if (token_equals(token, "start")) 
                {
                    Continue_If_Instruction_Already_Set;
                    
                    buffer.type = Entry_Start;
                    buffer.instruction = Ins_Time_Entry;
                }
                else if (token_equals(token, "stop"))
                {
                    Continue_If_Instruction_Already_Set;
                    
                    buffer.type = Entry_Stop;
                    buffer.instruction = Ins_Time_Entry;
                }
                else if (token_equals(token, "add"))
                {
                    process_offset_time_entry(state, &tokenizer, Entry_Add, !reading_from_file);
                }
                else if (token_equals(token, "subtract") ||
                         token_equals(token, "sub"))
                {
                    process_offset_time_entry(state, &tokenizer, Entry_Subtract, !reading_from_file);
                }
                else if (token_equals(token, "show"))
                {
                    Continue_If_Instruction_Already_Set;
                    Program_Interface_Only;
                    
                    buffer.instruction = Ins_Show;
                }
                else if (token_equals(token, "exit"))
                {
                    Continue_If_Instruction_Already_Set;
                    Program_Interface_Only;
                    
                    buffer.instruction = Ins_Exit;
                }
                else if (token_equals(token, "save"))
                {
                    Continue_If_Instruction_Already_Set;
                    Program_Interface_Only;
                    
                    buffer.instruction = Ins_Save;
                }
                else if (token_equals(token, "archive"))
                {
                    Continue_If_Instruction_Already_Set;
                    Program_Interface_Only;
                    
                    buffer.instruction = Ins_Archive;
                }
                else if (token_equals(token, "load"))
                {
                    Continue_If_Instruction_Already_Set;
                    Program_Interface_Only;
                    
                    buffer.instruction = Ins_Load;
                }
                else if (token_equals(token, "time"))
                {
                    Continue_If_Instruction_Already_Set;
                    Program_Interface_Only;
                    
                    buffer.instruction = Ins_Time;
                }
                else if (token_equals(token, "edit"))
                {
                    Continue_If_Instruction_Already_Set;
                    Program_Interface_Only;
                    
                    buffer.instruction = Ins_Edit;
                }
                else if (token_equals(token, "clear"))
                {
                    Continue_If_Instruction_Already_Set;
                    Program_Interface_Only;
                    
                    buffer.instruction = Ins_Clear;
                }
                else if (token_equals(token, "help"))
                {
                    Continue_If_Instruction_Already_Set;
                    Program_Interface_Only;
                    
                    buffer.instruction = Ins_Help;
                }
                else if (token_equals(token, "no-save"))
                {
                    Program_Interface_Only;
                    
                    set_flag(&buffer, Flag_No_Save);
                }
                else if (token_equals(token, "from"))
                {
                    Program_Interface_Only;
                    
                    if (buffer.date)
                    {
                        printf("%sbad date input before \"from\" identifier%s\n", b_error, b_reset);
                    }
                    
                    if (buffer.instruction != Ins_Show)
                    {
                        printf("%s\"from\" supported only with \"show\"%s\n", b_error, b_reset);
                        continue;
                    }
                    
                    buffer.show_input = ShowInput_From;
                }
                else if (token_equals(token, "to"))
                {
                    Program_Interface_Only;
                    
                    if (buffer.date)
                    {
                        printf("%sbad date input before \"to\" identifier%s\n", b_error, b_reset);
                    }
                    
                    if (buffer.instruction != Ins_Show)
                    {
                        printf("%s\"to\" supported only with \"show\"%s\n", b_error, b_reset);
                        continue;
                    }
                    
                    buffer.show_input = ShowInput_To;
                }
                else if (token_equals(token, "today"))
                {
                    Program_Interface_Only;
                    
                    if (buffer.instruction == Ins_Show)
                    {
                        time_t today = get_today(state);
                        if (buffer.show_input == ShowInput_One_Day)
                        {
                            if (!fill_time_slot(&buffer.date, &today))
                            {
                                printf("%s\"time\" can't overwrite previous value%s\n", b_error, b_reset);
                            }
                        }
                        else if (buffer.show_input == ShowInput_From)
                        {
                            if (!fill_time_slot(&buffer.date_from, &today))
                            {
                                buffer.date_from = today;
                            }
                        }
                        else if (buffer.show_input == ShowInput_To)
                        {
                            if (!fill_time_slot(&buffer.date_to, &today))
                            {
                                printf("%s\"time\" can't overwrite previous value%s\n", b_error, b_reset);
                            }
                        }
                        else
                        {
                            printf("%s\"time\" supported only with \"show\"%s\n", b_error, b_reset);
                        }
                    }
                    else
                    {
                        Print_Load_Error(state);
                        printf("Unknown identifier: %.*s. (Try help)", (s32)token.text_length, token.text);
                        Print_ClearN();
                    }
                } break;
                
                //~
                
                case Token_Date:
                {
                    if (buffer.instruction == Ins_Time_Entry)
                    {
                        if (!buffer.date)
                        {
                            auto parsed_date = parse_date(state, token);
                            if (parsed_date.success)
                            {
                                buffer.date = parsed_date.time;
                            }
                            else buffer.instruction = Ins_Unsupported;
                        }
                        else
                        {
                            Print_Load_Error(state);
                            printf("Only one date per command is supported.");
                            Print_ClearN();
                        }
                    }
                    else if (buffer.instruction == Ins_Show)
                    {
                        auto parsed_date = parse_date(state, token);
                        if (parsed_date.success)
                        {
                            if (buffer.show_input == ShowInput_One_Day)
                            {
                                if (!fill_time_slot(&buffer.date, &parsed_date.time))
                                {
                                    Print_Load_Error(state);
                                    printf("Can't input two dates. Use \"from\" & \"to\".");
                                    Print_ClearN();
                                }
                            }
                            else if (buffer.show_input == ShowInput_From)
                            {
                                if (!fill_time_slot(&buffer.date_from, &parsed_date.time))
                                {
                                    Print_Load_Error(state);
                                    printf("Can't input two \"from\" dates.");
                                    Print_ClearN();
                                }
                            }
                            else if (buffer.show_input == ShowInput_To)
                            {
                                if (!fill_time_slot(&buffer.date_to, &parsed_date.time))
                                {
                                    Print_Load_Error(state);
                                    printf("Can't input two \"to\" dates.");
                                    Print_ClearN();
                                }
                            }
                        }
                    }
                    else
                    {
                        Print_Load_Error(state);
                        printf("Date supported only for show & start/stop/add/sub.");
                        Print_ClearN();
                    }
                } break;
                
                
                //~
                case Token_Time:
                {
                    if (buffer.instruction == Ins_Time_Entry)
                    {
                        if (!buffer.time)
                        {
                            auto parsed_time = parse_time(state, token);
                            if (parsed_time.success)
                            {
                                buffer.time = parsed_time.time;
                            }
                            else buffer.instruction = Ins_Unsupported;
                        }
                        else
                        {
                            Print_Load_Error(state);
                            printf("Only one time per command is supported.");
                            Print_ClearN();
                        }
                    }
                    else
                    {
                        Print_Load_Error(state);
                        printf("Time \"%.*s\" supported only for start/stop/add/sub.", 
                               (s32)token.text_length, token.text);
                        Print_ClearN();
                    }
                } break;
                
                
                //~
                case Token_String:
                {
                    if (buffer.instruction == Ins_Time_Entry)
                    {
                        if (!buffer.description)
                        {
                            buffer.description = create_description(&state->element_arena, token);
                        }
                        else
                        {
                            Print_Load_Error(state);
                            printf("Only one description per command is supported");
                            Print_ClearN();
                        }
                    }
                    else
                    {
                        Print_Load_Error(state);
                        printf("Description supported only for start/stop/add/sub");
                        Print_ClearN();
                    }
                } break;
                
                //~
                case Token_Unknown:
                default:
                {
                } break;
                
                //~
                case Token_End_Of_Stream: 
                {
                    parsing = false;
                    
                    if (reading_from_file)
                    {
                        if (buffer.instruction != Ins_None)
                        {
                            Print_Load_Error(state);
                            printf("Missing semicolon at the end of the file");
                            Print_ClearN();
                        }
                        
                        break;
                    }
                    else
                    {
                        // NOTE: Don't break.
                        // Assume additional semicolon at the end of the console input.
                    }
                } 
                
                //~
                case Token_Semicolon:
                {
                    if (buffer.instruction == Ins_Time_Entry)
                    {
                        Time_Entry entry = {};
                        entry.type = buffer.type;
                        entry.date = buffer.date;
                        entry.time = buffer.time;
                        entry.description = buffer.description;
                        process_time_entry(state, &entry, reading_from_file);   
                    }
                    else if (buffer.instruction == Ins_Show)
                    {
                        if (buffer.date)
                        {
                            print_days_from_range(state, &buffer.date, &buffer.date);
                        }
                        else if (buffer.date_from || buffer.date_to)
                        {
                            time_t *from = (buffer.date_from) ? &buffer.date_from : NULL;
                            time_t *to = (buffer.date_to) ? &buffer.date_to : NULL;
                            
                            print_days_from_range(state, from, to);
                        }
                        else
                        {
                            print_days_from_range(state, NULL, NULL);
                        }
                    }
                    else if (buffer.instruction == Ins_Exit)
                    {
                        if (!is_flag_set(&buffer, Flag_No_Save)) 
                        {
                            if (state->change_count > 0)
                            {
                                save_to_file(state);
                            }
                        }
                        
                        Assert(main_loop_is_running);
                        *main_loop_is_running = false;
                    }
                    else if (buffer.instruction == Ins_Save)
                    {                    
                        save_to_file(state);
                        printf("File saved\n");
                    }
                    else if (buffer.instruction == Ins_Archive)
                    {
                        archive_current_file(state, true);
                    }
                    else if (buffer.instruction == Ins_Load)
                    {
                        load_file(state);
                    }
                    else if (buffer.instruction == Ins_Time)
                    {
                        time_t now = get_current_time(state);
                        char now_str[MAX_TIMESTAMP_STRING_SIZE];
                        get_timestamp_string(now_str, sizeof(now_str), now);
                        printf("Current time: %s\n", now_str);
                    }
                    else if (buffer.instruction == Ins_Edit)
                    {
                        platform_open_in_default_editor(state->input_file_full_path);
                    }
                    else if (buffer.instruction == Ins_Clear)
                    {
                        platform_clear_screen();
                    }
                    else if (buffer.instruction == Ins_Help)
                    {
                        printf("Commands available everywhere:\n"
                               "start 2025-12-31 11:20;\tstarts new timespan\n"
                               "stop 2025-12-31 14:12;\tstops current timespan\n"
                               "add 01:00;\t\tadds time to current day\n"
                               "\t\t\tcan also work like: add 2026-01-01 03:00;\n"
                               "sub 01:00;\t\tsubtracts time from current day\n"
                               
                               "\nCommands available only in console:\n"
                               "start & stop assumes current time when unspecified in console\n"
                               "show;\t\t\tshows current history\n"
                               "time;\t\t\tshows current time...\n"
                               "clear;\t\t\tclears the screen\n"
                               "edit;\t\t\topens database file in your default editor\n"
                               "\t\t\tworks best if your editor supports hot-loading\n"
                               
                               "\nThese actions happen automatically:\n"
                               "save;\t\t\tforces save\n"
                               "archive;\t\tforces backup\n"
                               "load;\t\t\tforces load from file\n"
                               );
                    }
                    
                    
                    
                    buffer = {};
                    
                } break;
            }
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
    state->load_error_count = 0;
    
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
        printf("[Load Error #%d] Failed to open the file: %s\n",
               state->load_error_count++, file_name);
    }
}


internal void read_from_keyboard(Thread_Memory *thread_memory)
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
    if (state.load_error_count == 0)
    {
        print_days_from_range(&state, NULL, NULL);
        save_to_file(&state);
    }
    else
    {
        state.change_count = 0;
    }
    
    if (reformat_mode)
    {
        if (state.load_error_count == 0) printf("File reformated. Exiting.\n");
        else printf("Reformatting skipped due to errors. Exiting.\n");
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
                save_to_file(&state);
                if (state.day_arena.count > 0)
                {
                    time_t today = get_today(&state);
                    print_days_from_range(&state, &today, &today, true);
                }
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