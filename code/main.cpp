/*
    TODO:
    * Config file?
* Clean up error printing code.
    * Convert to use Unicode?

    * Add sum of the month in the file. Can look like:
        // ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
        //           Sum of the month 2020-07-**        123:20
        // ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
*/

// TODO: Lower screenshot width in README.md so they won't get resized.

// TODO: Input sorting testing!

// TODO: Input sorting should be behind enable command;
//       Like "sort start 6:40 stop ... "


#include "main.h"
#include "description.cpp"

#include "error.cpp"
#include "token.cpp"
#include "string.cpp"
#include "time.cpp"


internal void load_file(Program_State *state);


internal Record *
get_last_record(Program_State *state)
{
    Record *result = nullptr;
    s64 count = state->records.count;
    if (count)
    {
        result = state->records.at(count - 1);
    }
    
    return result;
}




internal Parse_Number_Result
parse_number(char *src, s32 count)
{
    Parse_Number_Result result = {};
    
    s32 multiplier = 1;
    for (s32 index = count - 1; index >= 0; --index, multiplier *= 10)
    {
        if (!(src[index] >= '0' && src[index] <= '9'))
        {
            return result;
        }
        
        s32 to_add = (src[index] - '0') * multiplier;
        result.number += to_add;
    }
    
    result.is_valid = true;
    return result;
}

internal Parse_Date_Result
parse_date(Program_State *state, Token token)
{
    Assert(token.type == Token_Date);
    
    // NOTE: Supported format: 2020-12-31
    Parse_Date_Result result = {};
    
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
        
        
        result.date = platform_tm_to_time(&date);
        result.is_valid = (year.is_valid && month.is_valid && day.is_valid && dash1 && dash2);
    }
    
    if (!result.is_valid)
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
        
        for (s32 index = (s32)token.text_length - 1; index >= 0; --index)
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
        
        result.is_valid = true;
    }
    
    if (!result.is_valid)
    {
        Print_Parse_Error(state);
        printf("Bad time format!");
        print_line_with_token(token);
        Print_Clear();
        printf("\n");
    }
    
    return result;
}



inline b32
are_in_same_day(Record *record_a, Record *record_b)
{
    b32 result = record_a->date == record_b->date;
    return result;
}



struct Day_Sum_Result
{
    time32 sum;
    Missing_Ending missing_ending;
};

internal Day_Sum_Result
calculate_sum_for_day(Program_State *state, s64 starting_index)
{
    date64 range_start = 0;
    Day_Sum_Result result = {};
    date64 day = state->records.at(starting_index)->date;
    
    for (u64 index = starting_index; index < state->records.count; ++index)
    {
        Record *record = state->records.at(index);
        
        if (day != record->date && range_start == 0)
        {
            break;
        }
        
        if (record->type == Record_TimeStop || record->type == Record_TimeStart)
        {
            if (range_start)
            {
                date64 range_end = record->date + record->value;
                time32 delta = (time32)(range_end - range_start);
                result.sum += delta;
            }
            
            if (day != record->date)
            {
                range_start = 0;
                break;
            }
        }
        
        if (record->type == Record_TimeStart)
        {
            range_start = record->date + record->value;
        }
        else if (record->type == Record_TimeStop)
        {
            range_start = 0;
        }
        else if (day == record->date)
        {
            if (record->type == Record_TimeDelta)
            {
                result.sum += record->value;
            }
        }
    }
    
    
    if (range_start == 0)
    {
        result.missing_ending = MissingEnding_None;
    }
    else
    {
        date64 now = get_current_timestamp(state);
        date64 delta = now - range_start;
        
        if (delta > Days(1))
        {
            result.missing_ending = MissingEnding_Critical;
        }
        else
        {
            result.missing_ending = MissingEnding_Assumed;
            result.sum += (time32)delta;
        }
    }
    
    return result;
}


inline b32
compare_record_type_to_mask(Virtual_Array<Record> *records, u64 index, u32 type_mask)
{
    b32 result = false;
    if (records->count > index && 0 < index)
    {
        Record *record = records->at(index);
        result = record->type & type_mask;
    }
    
    return result;
}


inline char
get_sign_char_and_abs_value(s32 *value)
{
    char result = '+';
    
    if (*value < 0)
    {
        result = '-';
        *value = -(*value);
    }
    
    return result;
}

#if 0
internal void
print_record_tail(Program_State *state, s32 *sum)
{
    using namespace Color;
    
    printf("  ");
    
    if (*sum)
    {
        char sign = get_sign_char_and_abs_value(sum);
        Str32 sum_str = get_time_string(*sum);
        printf("  %c%s", sign, sum_str.str);
        *sum = 0;
    }
    
    for (s64 def_index = 0; def_index < state->defered_descs.count; ++def_index)
    {
        Defered_Description *def = state->defered_descs.at(def_index);
        
        if (def->value)
        {
            char sign = get_sign_char_and_abs_value(&def->value);
            Str32 val_str = get_time_string(def->value);
            printf("  %c%s", sign, val_str.str);
        }
        
        if (def->description.length)
        {
            printf(" %s\"%.*s\"%s", (def->value ? f_desc_delta : f_desc), def->description.length,
                   def->description.content, f_reset);
        }
    }
    
    printf("\n");
    state->defered_descs.clear();
}
#endif

#if 0
internal void
print_days_from_range(Program_State *state, date64 date_begin, date64 date_end, b32 alternative_colors = false)
{
    enum Range_State
    {
        Range_Open,
        Range_Closed_PrePrint,
        Range_Closed_PostPrint
    };
    
    
    // TODO: Add/Sub descriptions contained in range should push their descriptions to transient_arena
    // and maybe exclude them from sum... example:
    // 16:30 -> 18:30  +00:30 "Working on X" -00:15 "Tea break"
    using namespace Color;
    
    
    b32 is_new_day = true;
    s64 active_day_index = 0;
    time32 sum = 0;
    
    Range_State range_state = Range_Closed_PostPrint;
    b32 overnight_carry = false;
    
    for_u64(record_index, state->records.count)
    {
        Record *record = state->records.at(record_index);
        
        // skip if not in range
        if ((date_end) && (record->date > date_end))
        {
            break;
        }
        
        if ((date_begin) && (record->date < date_begin))
        {
            active_day_index = record_index + 1;
            continue;
        }
        
        
        
        // print day header
        if (is_new_day)
        {
            Str32 date_str = get_date_string(record->date);
            Str32 day_of_week = get_day_of_the_week_string(record->date);
            
            if (alternative_colors)
            {
                printf("\n%s%s%s %s%s%s\n", f_black, b_date, date_str.str, day_of_week.str, f_reset, b_reset);
            }
            else
            {
                printf("\n%s%s %s%s\n", f_date, date_str.str, day_of_week.str, f_reset);
            }
        }
        
        
        //~ print content
        
        Str32 time_str = get_time_string(record->value);
        
        if (record->type == Record_TimeStop)
        {
            if (overnight_carry)
            {
                printf(" ... ");
                overnight_carry = false;
            }
            
            printf(" -> %s", time_str.str);
            range_state = Range_Closed_PrePrint;
        }
        else if (record->type == Record_TimeDelta)
        {
            if (!record->description.length)
            {
                sum += record->value;
            }
            else
            {
                Defered_Description defered = {record->description, record->value};
                state->defered_descs.add(&defered);
            }
            
            if (range_state == Range_Closed_PostPrint)
            {
                range_state = Range_Closed_PrePrint;
            }
        }
        else if (range_state == Range_Open && record->type == Record_TimeStart)
        {
            if (overnight_carry)
            {
                printf(" ... ");
                overnight_carry = false;
            }
            
            printf("%s -> %s%s", f_dimmed, time_str.str, f_reset);
            range_state = Range_Closed_PrePrint;
        }
        
        
        
        if (range_state == Range_Closed_PrePrint)
        {
            print_record_tail(state, &sum);
            range_state = Range_Closed_PostPrint;
        }
        
        
        if (range_state == Range_Closed_PostPrint && record->type == Record_TimeStart)
        {
            printf("%s", time_str.str);
            range_state = Range_Open;
            
            Defered_Description defered = {record->description};
            state->defered_descs.add(&defered);
        }
        
        
        
        // __prepare next iteration + get sum___
        Record *next_record = record + 1;
        is_new_day = ((record_index == state->records.count - 1) || !are_in_same_day(record, next_record));
        
        if (is_new_day)
        {
            Day_Sum_Result sum_result = calculate_sum_for_day(state, active_day_index);
            Str128 sum_bar = get_sum_and_progress_bar_string(sum_result.sum, sum_result.missing_ending);
            
            if (sum_result.missing_ending != MissingEnding_None)
            {
                printf(" ->  ... ");
                print_record_tail(state, &sum);
            }
            else if (range_state == Range_Open)
            {
                printf(" ->  ... ");
                print_record_tail(state, &sum);
                overnight_carry = true;
            }
            
            printf("%s%s%s\n", f_sum, sum_bar.str, f_reset);
            
            active_day_index = record_index + 1;
        }
    }
}
#endif



internal void
archive_current_file(Program_State *state, b32 long_format = false)
{
    // TODO(f0): calculate hash of the whole file for file_name
    
    date64 now = get_current_timestamp(state);
    Str128 timestamp = get_timestamp_string_for_file(now, long_format);
    
    String file_name = push_stringf(&state->arena, "%.*s_%s.txt",
                                    string_expand(state->title), timestamp.str);
    
    Path archive_path = path_from_directory(state->archive_dir, file_name);
    file_copy(&state->arena, &state->input_path, &archive_path, false);
    
    if (long_format) 
    {
        printf("File archived as: %.*s\n", string_expand(file_name));
    }
}



internal void
save_to_file(Program_State *state)
{
    memory_scope(&state->arena);
    
    archive_current_file(state);
    state->parse_error_count = 0;
    
    
    File_Handle file = file_open_write(&state->arena, &state->input_path);
    if (no_errors(&file))
    {
        String_Builder builder = {};
        auto add = [&](String string) {
            builder_add(&state->arena, &builder, string);
        };
        
        b32 is_new_day = true;
        s64 active_day_index = 0;
        
        for_u64(record_index, state->records.count)
        {
            Record *record = state->records.at(record_index);
            
            // print day header comment
            if (is_new_day)
            {
                Str32 day_of_week = get_day_of_the_week_string(record->date);
                add(l2s("// "));
                add(string(day_of_week.str));
                add(l2s("\n"));
            }
            
            
            // print command
            char *command = NULL;
            if (record->type == Record_TimeStart)
            {
                command = "start";
            }
            else if (record->type == Record_TimeStop)
            {
                command = "stop";
            }
            else if (record->type == Record_TimeDelta)
            {
                if (record->value < 0)
                {
                    command = "sub";
                }
                else
                {
                    command = "add";
                }
            }
            else
            {
                Invalid_Code_Path;
                continue;
            }
            
            add(string(command));
            
            
            // print date
            if (is_new_day)
            {
                Str32 date_str = get_date_string(record->date);
                add(string(date_str.str));
            }
            
            
            // print time
            Str32 time_str = get_time_string(record->value);
            add(string(time_str.str));
            
            
            // print description
            Description *desc = get_description(&state->desc_table, record->desc_hash);
            if (desc->size)
            {
                add(l2s(" \""));
                add(string_from_desc(desc));
                add(l2s("\""));
            }
            
            // new line
            add(l2s("\n"));
            
            
            // __prepare next iteration + get sum___
            if (record_index == state->records.count - 1) {
                is_new_day = true;
            } else {
                Record *next_record = state->records.at(record_index + 1);
                is_new_day = !are_in_same_day(record, next_record);
            }
            
            if (is_new_day)
            {
                Day_Sum_Result sum_result = calculate_sum_for_day(state, active_day_index);
                Str128 sum_bar = get_sum_and_progress_bar_string(sum_result.sum, sum_result.missing_ending);
                
                add(l2s("// "));
                add(string(sum_bar.str));
                add(l2s("\n\n"));
                active_day_index = record_index + 1;
            }
        }
        
        
        String output_string = build_string(&state->arena, &builder);
        file_write_string(&file, output_string);
        file_close(&file);
        
        {
            // TODO(f0): pull get file mod time to stf0
            char *input_path_cstr = push_cstr_from_path(&state->arena, &state->input_path);
            state->input_file_mod_time = platform_get_file_mod_time(input_path_cstr);
        }
    }
    else
    {
        char *input_path_cstr = push_cstr_from_path(&state->arena, &state->input_path);
        printf("Failed to write to file: %s\n", input_path_cstr);
    }
    
    state->change_count = 0;
}



internal b32
automatic_save_to_file(Program_State *state)
{
    b32 did_save = false;
    
    if (state->change_count > 0)
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
    
    return did_save;
}



internal void
add_record(Program_State *state, Record *data, b32 allow_sorting)
{
    s64 candidate_index = state->records.count;
    b32 replace = false;
    
    for (s64 index = candidate_index - 1; index >= 0; --index)
    {
        Record *old_space = state->records.at(index);
        if (old_space->date < data->date)
        {
            break;
        }
        else if (old_space->date > data->date)
        {
            candidate_index = index;
        }
        else if ((data->type == Record_TimeStart || data->type == Record_TimeStop) &&
                 (old_space->type == Record_TimeStart || old_space->type == Record_TimeStop))
        {
            
            if (old_space->value > data->value)
            {
                candidate_index = index;
            }
            else if (old_space->type == Record_TimeStop && old_space->value == data->value)
            {
                
                candidate_index = index;
                replace = true;
                break;
            }
            else
            {
                break;
            }
        }
    }
    
    
    // NOTE: Reject sorting
    if (!allow_sorting &&
        !(candidate_index == state->records.count || (candidate_index == state->records.count - 1 && replace)))
    {
        
        Print_Parse_Error(state);
        if (state->reading_from_file)
        {
            printf("Can't insert out of order records from file!");
        }
        else
        {
            printf("Preface your commands with \"sort\" command if you want to insert out of order records.");
        }
        Print_ClearN();
        
        return;
    }
    
    
    
    if (!replace)
    {
        b32 can_add = false;
        if (data->type == Record_TimeStop)
        {
            
            for (s64 index = candidate_index - 1; index >= 0; --index)
            {
                Record *old_space = state->records.at(index);
                if (old_space->type == Record_TimeStart)
                {
                    can_add = true;
                    break;
                }
                else if (old_space->type == Record_TimeStop)
                {
                    break;
                }
            }
        }
        else
        {
            can_add = true;
        }
        
        if (can_add)
        {
            state->records.insert_at(data, candidate_index);
            ++state->change_count;
        }
        else
        {
            Print_Error();
            printf("[Warning] stop is being skipped - it can be added only when start is active");
            Print_ClearN();
        }
    }
    else
    {
        *state->records.at(candidate_index) = *data;
        ++state->change_count;
    }
}

internal b32
fill_date_optional(Program_State *state, Forward_Token *forward, Record *record)
{
    b32 success = false;
    
    if (forward->peek.type == Token_Date)
    {
        advance(forward);
        
        Parse_Date_Result parsed_date = parse_date(state, forward->token);
        if (parsed_date.is_valid)
        {
            record->date = parsed_date.date;
            success = true;
        }
    }
    else
    {
        if (!state->reading_from_file)
        {
            record->date = get_today(state);
            success = true;
        }
        else
        {
            Record *last_record = get_last_record(state);
            if (last_record)
            {
                record->date = last_record->date;
                success = true;
            }
            else
            {
                Print_Parse_Error(state);
                printf("First record needs to specify date. ");
                Print_Clear();
            }
        }
    }
    
    return success;
}

internal b32
fill_time_optional(Program_State *state, Forward_Token *forward, Record *record)
{
    b32 success = false;
    
    if ((forward->peek.type == Token_Time))
    {
        advance(forward);
        
        Parse_Time_Result parsed_time = parse_time(state, forward->token);
        if (parsed_time.is_valid)
        {
            record->value = parsed_time.time;
            success = true;
        }
    }
    else
    {
        if (!state->reading_from_file)
        {
            record->value = get_time(state);
            success = true;
        }
    }
    
    return success;
}


internal b32
fill_time_required(Program_State *state, Forward_Token *forward, Record *record)
{
    b32 success = false;
    
    if ((forward->peek.type == Token_Time))
    {
        advance(forward);
        
        Parse_Time_Result parsed_time = parse_time(state, forward->token);
        if (parsed_time.is_valid)
        {
            record->value = parsed_time.time;
            success = true;
        }
    }
    
    return success;
}

internal void
fill_description_optional(Program_State *state, Forward_Token *forward, Record *record)
{
    if ((forward->peek.type == Token_String))
    {
        advance(forward);
        record->description = create_description(state, forward->token);
    }
}


internal void
prase_command_start(Program_State *state, Tokenizer *tokenizer, b32 allow_sorting)
{
    Forward_Token forward = create_forward_token(tokenizer);
    Record record = {};
    record.type = Record_TimeStart;
    
    b32 success = fill_date_optional(state, &forward, &record);
    
    if (success)
    {
        success = fill_time_optional(state, &forward, &record);
    }
    
    if (success)
    {
        fill_description_optional(state, &forward, &record);
    }
    
    if (success)
    {
        add_record(state, &record, allow_sorting);
    }
    else
    {
        Print_Parse_Error(state);
        printf("Incorect command usage. Use:\n"
               "start [yyyy-MM-dd] (hh:mm) [\"description\"]");
        print_line_with_token(forward.token);
        Print_ClearN();
    }
}


internal void
prase_command_stop(Program_State *state, Tokenizer *tokenizer, b32 allow_sorting)
{
    Forward_Token forward = create_forward_token(tokenizer);
    Record record = {};
    record.type = Record_TimeStop;
    
    b32 success = fill_date_optional(state, &forward, &record);
    
    if (success)
    {
        success = fill_time_optional(state, &forward, &record);
    }
    
    if (success)
    {
        add_record(state, &record, allow_sorting);
    }
    else
    {
        Print_Parse_Error(state);
        printf("Incorect command usage. Use:\n"
               "start [yyyy-MM-dd] (hh:mm)");
        print_line_with_token(forward.token);
        Print_ClearN();
    }
}



internal void
parse_command_add_sub(Program_State *state, Tokenizer *tokenizer, b32 is_add, b32 allow_sorting)
{
    Forward_Token forward = create_forward_token(tokenizer);
    
    Record record = {};
    record.type = Record_TimeDelta;
    
    b32 success = fill_date_optional(state, &forward, &record);
    
    if (success)
    {
        success = fill_time_required(state, &forward, &record);
        if (!is_add)
            record.value *= -1;
        
        if (success && record.value == 0)
        {
            Print_Parse_Error(state);
            printf("[Warning] Time equal to zero! Record not added.");
            print_line_with_token(forward.token);
            Print_ClearN();
            return;
        }
    }
    
    if (success)
    {
        fill_description_optional(state, &forward, &record);
    }
    
    if (success)
    {
        add_record(state, &record, allow_sorting);
    }
    else
    {
        Print_Parse_Error(state);
        printf("Incorect command usage. Use:\n"
               "%s [yyyy-MM-dd] (hh:mm) [\"description\"]",
               (is_add) ? "add" : "sub");
        print_line_with_token(forward.token);
        Print_ClearN();
    }
}



internal Parse_Complex_Date_Result
parse_complex_date(Program_State *state, Token token)
{
    // NOTE: Allows to use keywords like today, yesterday + normal date formats
    // TODO: ^ today+1 yesterday-4
    
    Parse_Complex_Date_Result result = {};
    
    if (token.type == Token_Date)
    {
        Parse_Date_Result parse = parse_date(state, token);
        result.date = parse.date;
        result.condition = (parse.is_valid) ? Con_IsValid : Con_HasErrors;
    }
    
    return result;
}


internal Date_Range_Result
get_max_date_range()
{
    Date_Range_Result result;
    result.condition = Con_IsValid;
    result.begin = 1;
    result.end = S64_MAX;
    
    return result;
}

internal Date_Range_Result
get_date_range(Program_State *state, Tokenizer *tokenizer)
{
    Date_Range_Result result = {};
    b32 success = true;
    b32 has_matching_token = false;
    
    Forward_Token forward = create_forward_token(tokenizer);
    
    
    if (forward.peek.type == Token_Identifier && token_equals(forward.peek, "from"))
    {
        
        advance(&forward);
        has_matching_token = true;
        
        Parse_Complex_Date_Result date = parse_complex_date(state, forward.peek);
        success = is_condition_valid(date.condition);
        if (success)
        {
            result.begin = date.date;
            advance(&forward);
        }
    }
    
    
    if (success)
    {
        if (forward.peek.type == Token_Identifier && token_equals(forward.peek, "to"))
        {
            
            advance(&forward);
            has_matching_token = true;
            
            Parse_Complex_Date_Result date = parse_complex_date(state, forward.peek);
            success = is_condition_valid(date.condition);
            if (success)
            {
                result.end = date.date;
                advance(&forward);
            }
        }
    }
    
    
    if (success)
    {
        if (!has_matching_token)
        {
            
            if (token_equals(forward.peek, "all"))
            {
                has_matching_token = true;
                result = get_max_date_range();
                advance(&forward);
            }
            else
            {
                Parse_Complex_Date_Result date = parse_complex_date(state, forward.peek);
                result.condition = date.condition;
                if (is_condition_valid(result.condition))
                {
                    result.begin = date.date;
                    result.end = date.date;
                    advance(&forward);
                }
            }
        }
        else
        {
            result.condition = Con_IsValid;
        }
    }
    else
    {
        result.condition = Con_HasErrors;
    }
    
    
    return result;
}


internal Date_Range_Result
get_recent_days_range(Program_State *state)
{
    date64 today = get_today(state);
    date64 start = today - Days(31);
    
    s64 record_count = state->records.count;
    if (record_count)
    {
        date64 last_date = state->records.at(record_count - 1)->date;
        if (last_date > today)
        {
            today = last_date;
        }
        
        s64 past_index = record_count - 120;
        if (past_index < 0)
        {
            past_index = 0;
        }
        
        date64 start_date = state->records.at(past_index)->date;
        if (start_date < start)
        {
            start = start_date;
        }
    }
    
    Date_Range_Result result = {start, today, Con_IsValid};
    return result;
}


internal void
parse_command_show(Program_State *state, Tokenizer *tokenizer)
{
    Forward_Token forward = create_forward_token(tokenizer);
    
    Date_Range_Result range = get_date_range(state, tokenizer);
    
    if (range.condition == Con_IsValid)
    {
        print_days_from_range(state, range.begin, range.end);
    }
    else if (range.condition == Con_NoMatchigTokens)
    {
        Date_Range_Result recent = get_recent_days_range(state);
        print_days_from_range(state, recent.begin, recent.end);
    }
    else
    {
        Print_Parse_Error(state);
        printf("Incorect command usage. Use:\n"
               "show [yyyy-MM-dd]\n"
               "show [from yyyy-MM-dd] [to yyyy-MM-dd]\n");
        print_line_with_token(forward.token);
        Print_ClearN();
    }
}


inline b32
increase_index_to_next_day(Dynamic_Array<Record> *records, s64 *index)
{
    date64 start_date = records->at(*index)->date;
    
    for (;
         *index < records->count;
         ++(*index))
    {
        Record *record = records->at(*index);
        if (record->date > start_date)
        {
            return true;
        }
    }
    
    return false;
}


enum Granularity // TODO: Support more granularities
{
    Granularity_Days,
    // Granularity_Weeks,
    Granularity_Months,
    // Granularity_Quarters,
    // Granularity_Years
};


internal void
process_and_print_summary(Program_State *state, Granularity granularity, date64 date_begin, date64 date_end)
{
    s64 record_index = 0;
    
    // NOTE: Skip record before date_begin
    for (;
         record_index < state->records.count;
         ++record_index)
    {
        Record *record = state->records.at(record_index);
        if (date_begin <= record->date)
        {
            break;
        }
    }
    
    
    time32 sum = 0;
    Boundries_Result boundries = {};
    b32 loop = true;
    Record *record = state->records.at(record_index);
    
    while (loop)
    {
        loop = increase_index_to_next_day(&state->records, &record_index);
        
        if (loop)
        {
            record = state->records.at(record_index);
            if (date_end < record->date)
            {
                loop = false;
            }
        }
        
        
        
        b32 past_boundary = loop && (record->date >= boundries.one_day_past_end);
        
        
        // NOTE: Print line
        if (!loop || past_boundary)
        {
            s32 day_count = (s32)boundries.day_count;
            if (day_count > 0)
            {
                Str32 date_str = get_date_string(boundries.begin);
                
                date64 today = get_today(state);
                if (today < boundries.one_day_past_end && today >= boundries.begin)
                {
                    day_count = (s32)(today / (Days(1))) - (s32)(boundries.begin / (Days(1)));
                    
                    if (day_count <= 0)
                    {
                        day_count = 1;
                    }
                }
                
                time32 time_avg = sum / day_count;
                
                Str32 sum_str = get_time_string(sum);
                Str128 avg_bar = get_progress_bar_string(time_avg, MissingEnding_None);
                
                Str128 sum_bar;
                if (day_count >= 2)
                {
                    Str32 avg_str = get_time_string(time_avg);
                    
                    snprintf(sum_bar.str, sizeof(sum_bar.str), "sum: %s\tavg(/%d): %s\t%s", sum_str.str, day_count,
                             avg_str.str, avg_bar.str);
                }
                else
                {
                    snprintf(sum_bar.str, sizeof(sum_bar.str), "sum: %s\t%s", sum_str.str, avg_bar.str);
                }
                
                using namespace Color;
                printf("%s%s\t%s%s%s\n", f_date, date_str.str, f_sum, sum_bar.str, f_reset);
            }
            
            // NOTE: Clear sum
            sum = 0;
        }
        
        
        // NOTE: Summing
        if (loop)
        {
            Day_Sum_Result day_sum = calculate_sum_for_day(state, record_index);
            sum += day_sum.sum;
        }
        
        
        // NOTE: Update boundary
        if (loop && past_boundary)
        {
            switch (granularity)
            {
                case Granularity_Months: {
                    boundries = get_month_boundries(record->date);
                }
                break;
                
                default:
                Invalid_Code_Path;
                case Granularity_Days: {
                    boundries.day_count = 1;
                    boundries.begin = record->date;
                    boundries.one_day_past_end = record->date + Days(1);
                }
                break;
            }
        }
    }
}



internal void
parse_command_summary(Program_State *state, Tokenizer *tokenizer)
{
    Forward_Token forward = create_forward_token(tokenizer);
    
    Granularity granularity = Granularity_Days;
    // TODO: Pull out granularity check.
    if (forward.peek.type == Token_Identifier)
    {
        if (token_equals(forward.peek, "days") ||
            token_equals(forward.peek, "d"))
        {
            advance(&forward);
            granularity = Granularity_Days;
        }
        else if (token_equals(forward.peek, "months") ||
                 token_equals(forward.peek, "m"))
        {
            advance(&forward);
            granularity = Granularity_Months;
        }
#if 0
        else if (token_equals(forward.peek, "years") ||
                 token_equals(forward.peek, "y"))
        {
            advance(&forward);
            granularity = Granularity_Years;
        }
#endif
    }
    
    // arg - optional - time range
    Date_Range_Result range = get_date_range(state, tokenizer);
    
    if (range.condition == Con_NoMatchigTokens)
    {
        range = get_max_date_range();
    }
    
    process_and_print_summary(state, granularity, range.begin, range.end);
    
    
    if (range.condition == Con_HasErrors)
    {
        Print_Parse_Error(state);
        printf("Incorect command usage. Use:\n"
               "summary [granularity]\n"
               "summary [granularity] [yyyy-MM-dd]\n"
               "summary [granularity] [from yyyy-MM-dd] [to yyyy-MM-dd]\n"
               "\n\tgranularity - days/months (or d/m)\n");
        print_line_with_token(forward.token);
        Print_ClearN();
    }
}


internal void
parse_command_exit(Program_State *state, Tokenizer *tokenizer, b32 *main_loop_is_running)
{
    Assert(main_loop_is_running != NULL);
    
    Forward_Token forward = create_forward_token(tokenizer);
    if ((forward.peek.type == Token_Identifier) && (token_equals(forward.peek, "no-save")))
    {
        advance(&forward);
        *main_loop_is_running = false;
    }
    else
    {
        
        b32 did_save = automatic_save_to_file(state);
        
        if (did_save || (state->change_count == 0))
        {
            *main_loop_is_running = false;
        }
        else
        {
            Print_Error();
            printf("Automatic save failed. To exit without saving use: exit no-save");
            Print_ClearN();
        }
    }
}


internal void
process_input(char *content, Program_State *state, b32 reading_from_file, b32 *main_loop_is_running = NULL)
{
    state->reading_from_file = reading_from_file;
    
    using namespace Color;
    Tokenizer tokenizer = create_tokenizer(content);
    b32 parsing = true;
    b32 allow_sorting = false;
    
    
    while (parsing)
    {
        Token token = get_token(&tokenizer);
        switch (token.type)
        {
            case Token_Identifier: {
                if (token_equals(token, "start"))
                {
                    prase_command_start(state, &tokenizer, allow_sorting);
                }
                else if (token_equals(token, "stop"))
                {
                    prase_command_stop(state, &tokenizer, allow_sorting);
                }
                else if (token_equals(token, "add"))
                {
                    parse_command_add_sub(state, &tokenizer, true, allow_sorting);
                }
                else if (token_equals(token, "subtract") || token_equals(token, "sub"))
                {
                    parse_command_add_sub(state, &tokenizer, false, allow_sorting);
                }
                else if (token_equals(token, "sort"))
                {
                    if (state->reading_from_file)
                    {
                        Command_Line_Only_Message(state, token);
                    }
                    else
                    {
                        allow_sorting = true;
                    }
                }
                else if (token_equals(token, "show"))
                {
                    if (state->reading_from_file)
                    {
                        Command_Line_Only_Message(state, token);
                    }
                    else
                    {
                        parse_command_show(state, &tokenizer);
                    }
                }
                else if (token_equals(token, "summary"))
                {
                    if (state->reading_from_file)
                    {
                        Command_Line_Only_Message(state, token);
                    }
                    else
                    {
                        parse_command_summary(state, &tokenizer);
                    }
                }
                else if (token_equals(token, "exit"))
                {
                    if (state->reading_from_file)
                    {
                        Command_Line_Only_Message(state, token);
                    }
                    else
                    {
                        parse_command_exit(state, &tokenizer, main_loop_is_running);
                    }
                }
                else if (token_equals(token, "save"))
                {
                    if (state->reading_from_file)
                    {
                        Command_Line_Only_Message(state, token);
                    }
                    else
                    {
                        save_to_file(state);
                        printf("File saved\n");
                    }
                }
                else if (token_equals(token, "archive"))
                {
                    if (state->reading_from_file)
                    {
                        Command_Line_Only_Message(state, token);
                    }
                    else
                    {
                        archive_current_file(state, true);
                    }
                }
                else if (token_equals(token, "load"))
                {
                    if (state->reading_from_file)
                    {
                        Command_Line_Only_Message(state, token);
                    }
                    else
                    {
                        load_file(state);
                    }
                }
                else if (token_equals(token, "time"))
                {
                    if (state->reading_from_file)
                    {
                        Command_Line_Only_Message(state, token);
                    }
                    else
                    {
                        date64 now = get_current_timestamp(state);
                        Str128 now_str = get_timestamp_string(now);
                        printf("Current time: %s\n", now_str.str);
                    }
                }
                else if (token_equals(token, "edit"))
                {
                    if (state->reading_from_file)
                    {
                        Command_Line_Only_Message(state, token);
                    }
                    else
                    {
                        platform_open_in_default_editor(state->input_file_full_path);
                    }
                }
                else if (token_equals(token, "dir"))
                {
                    if (state->reading_from_file)
                    {
                        Command_Line_Only_Message(state, token);
                    }
                    else
                    {
                        platform_open_in_default_editor(state->executable_path2.directory);
                    }
                }
                else if (token_equals(token, "clear"))
                {
                    if (state->reading_from_file)
                    {
                        Command_Line_Only_Message(state, token);
                    }
                    else
                    {
                        platform_clear_screen();
                    }
                }
                else if (token_equals(token, "help"))
                {
                    if (state->reading_from_file)
                    {
                        Command_Line_Only_Message(state, token);
                    }
                    else
                    {
                        print_help_desc("[...] - optional, (...) - required\n");
                        print_help_header("Commands available everywhere");
                        print_help_item("start", "[yyyy-MM-dd] (hh:mm) [\"description\"]", "starts new timespan");
                        print_help_item("stop", "[yyyy-MM-dd] (hh:mm) [\"description\"]", "stops current timespan");
                        
                        print_help_item("add", "[yyyy-MM-dd] (hh:mm) [\"description\"]", "adds arbitrary amount of time");
                        print_help_item("sub", "[yyyy-MM-dd] (hh:mm) [\"description\"]",
                                        "subtracts arbitrary amount of time");
                        
                        print_help_desc("add/sub commands support inputs formatted as minutes (125) or hours (2:05)");
                        
                        
                        printf("\n");
                        print_help_header("Console only commands");
                        print_help_desc("When using commands from above in console you can skip (hh:mm) requirement");
                        
                        print_help_item("show", "[yyyy-MM-dd]", "\t\t\tshows current history");
                        print_help_item("show", "[from yyyy-MM-dd] [to yyyy-MM-dd]", "");
                        print_help_item("summary", "[d/m] [from yyyy-MM-dd] [to yyyy-MM-dd]", "sums hours for day/month");
                        
                        printf("\n");
                        print_help_item("edit", nullptr,
                                        "opens database file in your default editor"
                                        "\n\tworks best if your editor supports hot-loading");
                        // printf("\n");
                        print_help_item("dir", nullptr, "opens directory with exe");
                        print_help_item("clear", nullptr, "clears the screen");
                        print_help_item("time", nullptr, "shows current time");
                        
                        print_help_desc("\nFollowing commands execute automatically but can be also called manually");
                        print_help_item("save", nullptr, "forces save");
                        print_help_item("archive", nullptr, "forces backup");
                        print_help_item("load", nullptr, "forces load from file");
                    }
                }
                else
                {
                    Print_Parse_Error(state);
                    printf("%.*s - unexpected identifier", (s32)token.text_length, token.text);
                    print_line_with_token(token);
                    Print_ClearN();
                }
            }
            break;
            
            
            case Token_End_Of_Stream: {
                parsing = false;
            }
            break;
            
            case Token_Semicolon: {
            }
            break;
            
            default: {
                Print_Parse_Error(state);
                printf("%.*s - unexpected element", (s32)token.text_length, token.text);
                print_line_with_token(token);
                Print_ClearN();
            }
            break;
        }
    }
    
    
    state->reading_from_file = false;
}


internal void
clear_memory(Program_State *state)
{
    state->defered_descs.clear();
    clear_arena(&state->mixed_arena);
    state->records.clear();
}



internal void
load_file(Program_State *state)
{
    clear_memory(state);
    state->parse_error_count = 0;
    
    char *file_name = state->input_file_full_path;
    char *file_content = read_entire_file(&state->mixed_arena, file_name);
    
    if (file_content)
    {
        process_input(file_content, state, true);
        state->input_file_mod_time = platform_get_file_mod_time(state->input_file_full_path);
        printf("File loaded\n");
    }
    else
    {
        printf("[Parse Error #%d] Failed to open the file: %s\n", state->parse_error_count++, file_name);
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
            
            s64 len = strlen(thread_memory->input_buffer);
            char *last_char = thread_memory->input_buffer + (len - 1);
            if (*last_char == '\n')
            {
                *last_char = 0;
            }
            
            thread_memory->new_data = true;
        }
    }
}



struct Alloc_Resources
{
    size_t size;
    u8 *address;
};

inline Alloc_Resources
plan_alloc(Alloc_Resources *total_res, size_t size_to_alloc)
{
    Alloc_Resources result = {};
    result.size = size_to_alloc;
    result.address = total_res->address;
    
    total_res->size -= size_to_alloc;
    total_res->address += size_to_alloc;
    
    Assert(total_res->size >= 0);
    
    return result;
}


enum Cmd_Arugment_Type
{
    Cmd_None,
    Cmd_Input_File_Path
};

int
main(int argument_count, char **arguments)
{
    //~ NOTE: Initialization
    Program_State state = {};
    state.arena = create_virtual_arena();
    Arena *arena = &state.arena;
    state.desc_table = create_description_table(4096);
    //clear_memory(&state);
    
    
    
    b32 reformat_mode = false;
    
    { //~ SCOPE: Processing of command line arugments
        Cmd_Arugment_Type type = Cmd_None;
        b32 disable_colors = false;
        
        for (s32 argument_index = 1; argument_index < argument_count; ++argument_index)
        {
            char *arg = arguments[argument_index];
            
            if (type == Cmd_None)
            {
                if (arg[0] == '-' || arg[0] == '/' || arg[0] == '?')
                {
                    if (arg[0] == '?' || arg[1] == '?' || arg[1] == 'h')
                    {
                        printf("tt.exe [-d database.txt] [-m]"
                               "\n"
                               "Options:"
                               "\n\t"
                               "-d"
                               "\t\t"
                               "Selects file to load and save data from."
                               "\n\t"
                               "-m"
                               "\t\t"
                               "Mono mode. Turns off colors."
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
                state.input_path = push_path_from_string(arena, string(arg));
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
    
    state.exe_path = push_current_executable_path(arena);
    state.title = string_find_from_right_trim_ending(state.exe_path.file_name, '.');
    
    if (state.input_path.file_name.size == 0)
    {
        state.input_path = state.exe_path;
        state.input_path.file_name = push_string_concatenate(arena, state.title, l2s(".txt"));
    }
    
    
    state.archive_dir = push_directory_append(arena, state.exe_path.directory, l2s("archive"));
    directory_create(arena, state.archive_dir);
    
    
    
    //~ Load file. Save with better formatting if there was no load errors.
    
    {
        memory_scope(arena);
        
        char *input_path_cstr = push_cstr_from_path(arena, &state.input_path);
        if (!file_exists(input_path_cstr))
        {
            File_Handle file = file_open_write(input_path_cstr);
            if (no_errors(&file)) {
                printf("Created new file: %s\n", input_path_cstr);
            }
            else
            {
                printf("Failed to create new file: %s\n", input_path_cstr);
                exit_error();
            }
            
            file_close(&file);
        }
    }
    
    
    load_file(&state);
    b32 did_save_on_first_load = false;
    if (state.parse_error_count == 0)
    {
        Date_Range_Result range = get_recent_days_range(&state);
        print_days_from_range(&state, range.begin, range.end);
        save_to_file(&state);
        did_save_on_first_load = true;
    }
    else
    {
        state.change_count = 0;
    }
    
    if (reformat_mode)
    {
        if (did_save_on_first_load)
            printf("File reformated. Exiting.\n");
        else
            printf("Reformatting skipped due to errors. Exiting.\n");
        
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
                if (state.records.count > 0)
                {
                    Record *last_record = get_last_record(&state);
                    print_days_from_range(&state, last_record->date, last_record->date, true);
                }
                automatic_save_to_file(&state);
            }
            
            thread_memory.new_data = false;
        }
        
        
        
        File_Time mod_time = platform_get_file_mod_time(state.input_file_full_path);
        b32 source_file_changed = platform_compare_file_time(state.input_file_mod_time, mod_time) != 0;
        
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