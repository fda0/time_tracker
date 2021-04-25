#include "main.h"

global Global_State global_state;
global Stubs global_stubs = {};

global Color_Pair color_pairs[Color_Count] = {
    {Color_Empty, ""},
    {Color_Reset, "\033[39m\033[49m"},
    
    {Color_Base, "\033[97m"},
    {Color_Dimmed, "\033[90m"},
    
    {Color_Date, "\033[33m"},
    {Color_AltDate, "\033[43m\033[30m"},
    
    {Color_Description, "\033[36m"},
    {Color_AltDescription, "\033[96m"},
    
    {Color_Positive, "\033[32m"},
    {Color_AltPositive, "\033[92m"},
    
    {Color_Negative, "\033[31m"},
    {Color_AltNegative, "\033[91m"},
    
    {Color_Error, "\033[41m\033[97m"},
    {Color_Warning, "\033[103m\033[30m"},
    
    {Color_HelpHeader, "\033[100m"},
};


#include "win32_platform.cpp"
#include "description.cpp"
#include "lexer.cpp"
#include "string.cpp"
#include "time.cpp"


internal void load_file(Program_State *state);













inline char *
get_color(Color_Code code)
{
#if Def_Slow
    s64 count = Color_Count;
    assert(count == array_count(color_pairs));
    assert(array_count(color_pairs) > code);
    assert(color_pairs[code].code_check == code);
#endif
    char *result = "";
    
    if (!global_state.colors_disabled) {
        result = color_pairs[code].value;
    }
    
    return result;
}

inline void
print_color(Color_Code code)
{
    if (!global_state.colors_disabled)
    {
        char *color = get_color(code);
        printf("%s", color);
    }
}

internal void
print_help_item(char *command, char *args, char *help)
{
    print_color(Color_Positive);
    printf("%s\t", command);
    
    if (args) {
        print_color(Color_Description);
        printf("%s\t", args);
    }
    
    print_color(Color_Reset);
    printf("%s\n", help);
}

internal void
print_help_header(char *text)
{
    print_color(Color_HelpHeader);
    printf("%s", text);
    print_color(Color_Reset);
    printf("\n");
}


internal void
print_help_desc(char *text)
{
    print_color(Color_Dimmed);
    printf("%s\n", text);
    print_color(Color_Reset);
}


inline void
print_description(Record *record, Color_Code color = Color_Description)
{
    String desc = record->desc;
    if (desc.size)
    {
        print_color(color);
        printf(" \"%.*s\"", string_expand(desc));
        print_color(Color_Reset);
    }
}

inline void
print_time_delta(Arena *temp_arena, Record *time_delta_record)
{
    arena_scope(temp_arena);
    
    s32 time = time_delta_record->value;
    char sign = (time < 0) ? '-' : '+';
    
    if (sign == '-') {
        print_color(Color_AltNegative);
    } else {
        print_color(Color_AltPositive);
    }
    
    String time_str = get_time_string(temp_arena, time);
    printf("  %c%.*s", sign, string_expand(time_str));
    print_color(Color_Reset);
    
    print_description(time_delta_record, Color_AltDescription);
}

inline void
print_defered_time_deltas(Arena *temp_arena, Linked_List<Record> *defered)
{
    for_linked_list_ptr(node, defered)
    {
        Record record = node->item;
        print_time_delta(temp_arena, &record);
    }
}

































inline b32
no_errors(Record_Session *session)
{
    b32 result = session->no_errors;
    return result;
}


inline void
session_set_error(Record_Session *session, char *message)
{
    session->no_errors = false;
                                                               
    Token command_token = session->current_command_token;
    Find_Index find = index_of((char *)command_token.text.str, l2s("\0\n\r"));
    String at_str = string(command_token.text.str, find.index);
    
    
    print_color(Color_Error);
    if (command_token.line.row > 1)
    {
        printf("[Error] %s; at (line %u): %.*s", message, command_token.line.row, string_expand(at_str));
    }
    else
    {
        printf("[Error] %s; at: %.*s", message, string_expand(at_str));
    }
    print_color(Color_Reset);
    printf("\n");
}


inline Program_Scope
create_program_scope(Arena *arena, Virtual_Array<Record> *records)
{
    Program_Scope result = {};
    result.arena_scope = create_arena_scope(arena);
    result.records_scope = create_virtual_array_scope(records);
    return result;
}

inline void
pop_program_scope(Program_Scope *scope)
{
    assert(scope);
    pop_arena_scope(&scope->arena_scope);
    pop_virtual_array_scope(&scope->records_scope);
}













internal Record *
get_last_record(Record_Session *session)
{
    Record *result = nullptr;
    s64 count = session->records->count;
    if (count)
    {
        result = session->records->at(count - 1);
    }
    
    return result;
}


internal Parse_Number_Result
parse_number(u8 *src, u64 count)
{
    Parse_Number_Result result = {};
    
    s32 sign = 1;
    if (count > 0 &&
        (src[0] == '+' || src[0] == '-'))
    {
        if (src[0] == '-') {
            sign = -1;
        }
        
        src += 1;
        count -= 1;
    }
    
    u64 leading_spaces = 0;
    for_u64(index, count)
    {
        u8 character = src[index];
        if (character == ' ') {
            leading_spaces += 1;
        } else {
            break;
        }
    }
    src += leading_spaces;
    count -= leading_spaces;
    
    
    s32 multiplier = 1;
    for_u64(index, count)
    {
        u64 reverse_index = count-index-1;
        u8 character = src[reverse_index];
        
        if (!((character >= '0') && (character <= '9'))) {
            return result; // invalid
        }
        
        s32 to_add = (character - '0') * multiplier;
        result.value += to_add;
        
        multiplier *= 10;
    }
    
    result.value *= sign;
    result.is_valid = true;
    return result;
}

inline Parse_Number_Result
parse_number(Token token)
{
    Parse_Number_Result result = parse_number(token.text.str, token.text.size);
    return result;
}


internal Parse_Date_Result
parse_date(Record_Session *session, Token token)
{
    assert(token.type == Token_Date);
    
    // NOTE: Supported format: 2020-12-31
    Parse_Date_Result result = {};
    
    if (token.text.size == (4 + 1 + 2 + 1 + 2))
    {
        u8 *text = token.text.str;
        tm date = {};
        
        // year
        auto year = parse_number(text, 4);
        date.tm_year = year.value - 1900;
        text += 4;
        
        b32 dash1 = is_date_separator(*text++);
        
        // month
        auto month = parse_number(text, 2);
        date.tm_mon = month.value - 1;
        text += 2;
        
        b32 dash2 = is_date_separator(*text++);
        
        // day
        auto day = parse_number(text, 2);
        date.tm_mday = day.value;
        
        
        result.date = platform_tm_to_time(&date);
        result.is_valid = (year.is_valid && month.is_valid && day.is_valid && dash1 && dash2);
    }
    
    if (!result.is_valid) {
        session_set_error(session, "Bad date format!");
    }
    
    return result;
}

internal Parse_Time_Result
parse_time(Record_Session *session, Token token)
{
    // NOTE: Supported format: 10:32, 02:00, 2:0, 120...
    Parse_Time_Result result = {};
    
    if (token.text.size > 0)
    {
        b32 had_first_colon = false;
        u32 multiplier = 1;
        
        for (s32 index = (s32)token.text.size - 1; index >= 0; --index)
        {
            char c = token.text.str[index];
            
            if (c >= '0' && c <= '9')
            {
                u32 digit_value = (c - '0') * multiplier * 60;
                
                if (had_first_colon)
                {
                    digit_value *= 60;
                }
                
                result.value += digit_value;
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
        session_set_error(session, "Bad time format");
    }
    
    return result;
}



inline b32
are_in_same_day(Record *record_a, Record *record_b)
{
    b32 result = record_a->date == record_b->date;
    return result;
}



















internal Record_Range
get_records_range_for_starting_date(Program_State *state, u64 start_index)
{
    Record_Range result = {};
    b32 start_is_active = false;
    u64 index = start_index;
    
    if (index < state->records.count)
    {
        date64 date_begin = state->records.at(index)->date;
        
        for (;
             index < state->records.count;
             ++index)
        {
            Record *record = state->records.at(index);
            if (date_begin <= record->date &&
                record->type != Record_TimeStop)
            {
                result.date = record->date;
                result.first = index;
                
                if (record->type == Record_TimeStart)
                {
                    start_is_active = true;
                }
                break;
            }
        }
        
        
        for (;
             index < state->records.count;
             ++index)
        {
            Record *record = state->records.at(index);
            
            if (result.date != record->date)
            {
                result.one_past_last = result.next_day_first_record_index = index;
                
                if (start_is_active) {
                    result.one_past_last += 1;
                }
                break;
            }
            
            if (record->type == Record_TimeStart)
            {
                start_is_active = true;
            }
            else if (record->type == Record_TimeStop)
            {
                start_is_active = false;
            }
        }
        
        if (!result.one_past_last) {
            result.one_past_last = result.next_day_first_record_index = state->records.count;
        }
    }
    
    return result;
}


internal Range_u64
get_index_range_for_date_range(Program_State *state, date64 date_begin, date64 date_end)
{
    Range_u64 result = {};
    
    u64 index = 0;
    for (;
         index < state->records.count;
         ++index)
    {
        Record *record = state->records.at(index);
        if (date_begin <= record->date) {
            result.first = index;
            break;
        }
    }
    
    
    for (;
         index < state->records.count;
         ++index)
    {
        Record *record = state->records.at(index);
        if (date_end < record->date) {
            result.one_past_last = index;
            break;
        }
    }
    
    if (!result.one_past_last) {
        result.one_past_last = state->records.count;
    }
    
    return result;
}





// TODO(f0): Add start index to avoid traversing whole array all the time
internal Process_Days_Result
process_days_from_range(Program_State *state,
                        date64 date_begin, date64 date_end,
                        String filter,
                        Process_Days_Options options)
{
#if Def_Slow
    u32 open_start_ending_should_happen_only_once_test = 0;
#endif
    
    enum Open_State
    {
        Open,
        Closed_PrePrint,
        Closed_PostPrint,
    };
    
    Arena *arena = &state->arena;
    arena_scope(arena);
    
    
    b32 should_print = (options >= ProcessDays_Print);
    
    
    Process_Days_Result result = {};
    
    b32 has_filter = (filter.size != 0);
    Range_u64 whole_range = get_index_range_for_date_range(state, date_begin, date_end);
    
    
    Record_Range range = get_records_range_for_starting_date(state, whole_range.first);
    
    for (;
         range.date <= date_end && range.date != 0;
         range = get_records_range_for_starting_date(state, range.next_day_first_record_index))
    {
        arena_scope(arena);
        
        Linked_List<Record> defered_time_deltas = {}; // NOTE: for prints only
        Record *active_start = nullptr;
        Open_State open_state = Closed_PostPrint;
        s32 day_time_sum = 0;
        b32 day_header_printed = false;
        
        
        
        for (u64 index = range.first;
             index < range.one_past_last;
             ++index)
        {
            Record record = *state->records.at(index);
            
            if (has_filter)
            {
                if (!active_start && index == range.next_day_first_record_index) {
                    continue;
                }
                
                b32 matches_filter = equals(filter, record.desc);
                
                if (!active_start && !matches_filter) {
                    continue;
                }
                
                if (active_start && record.type == Record_TimeStart) {
                    record.type = Record_TimeStop;
                }
            }
            
            
            if (should_print && !day_header_printed)
            {
                day_header_printed = true;
                
                String date_str = get_date_string(arena, range.date);
                String day_of_week = get_day_of_the_week_string(arena, range.date);
                
                printf("\n");
                
                if (options == ProcessDays_PrintAltColor) {
                    print_color(Color_AltDate);
                } else {
                    print_color(Color_Date);
                }
                
                printf("%.*s %.*s", string_expand(date_str), string_expand(day_of_week));
                
                print_color(Color_Reset);
                printf("\n");
            }
            
            
            
            if (record.date != range.date &&
                active_start)
            {
                // NOTE: case: "start" ends on next/another day   
                assert(record.type == Record_TimeStop || record.type == Record_TimeStart);
                assert(active_start);
                
                day_time_sum += record.value - active_start->value;
                day_time_sum += safe_truncate_to_s32(record.date - active_start->date);
                
                if (should_print)
                {
                    String time_str = get_time_string(arena, record.value);
                    printf("%.*s", string_expand(time_str));
                    
                    print_color(Color_Dimmed);
                    if (record.date != range.date + Days(1))
                    {
                        String date_str = get_date_string(arena, record.date);
                        printf(" (%.*s)", string_expand(date_str));
                    }
                    else
                    {
                        printf(" (next day)");
                    }
                    print_color(Color_Reset);
                    
                    
                    print_description(active_start);
                    
                    print_defered_time_deltas(arena, &defered_time_deltas);
                    // print all defers (TimeDelta & CountDelta) here
                    printf("\n");
                }
                
                active_start = nullptr;
            }
            else
            {
                if (open_state == Open)
                {
                    if (record.type == Record_TimeDelta)
                    {
                        day_time_sum += record.value;
                        
                        if (should_print)
                        {
                            // NOTE: case: this needs to be printed _after_ we print "stop"
                            *defered_time_deltas.append(arena) = record;
                        }
                    }
                    else if (record.type == Record_TimeStart ||
                             record.type == Record_TimeStop)
                    {
                        assert(active_start);
                        day_time_sum += record.value - active_start->value;
                        
                        
                        if (should_print)
                        {
                            if (record.type == Record_TimeStart) {
                                print_color(Color_Dimmed);
                            }
                            
                            String time_str = get_time_string(arena, record.value);
                            printf("%.*s", string_expand(time_str));
                            
                            print_color(Color_Reset);
                            
                            
                            print_description(active_start);
                            
                            
                            print_defered_time_deltas(arena, &defered_time_deltas);
                            // print CountDelta defers on new lines
                            
                            printf("\n");
                            
                            
                            // NOTE: In case of "start 1; start 2; assumed end time"
                            // this defered_time_deltas would get printed twice
                            // (additional print for assumed time range)
                            defered_time_deltas = {};
                        }
                        
                        
                        
                        active_start = nullptr;
                    }
                    else
                    {
                        // defer2 counts?
                        assert(0);
                    }
                    
                    
                    if (record.type == Record_TimeStart) {
                        open_state = Closed_PostPrint;
                    } else if (record.type == Record_TimeStop) {
                        open_state = Closed_PrePrint;
                    }
                }
                
                
                if (open_state == Closed_PostPrint)
                {
                    if (record.type == Record_TimeDelta)
                    {
                        day_time_sum += record.value;
                        
                        if (should_print)
                        {
                            printf("      ");
                            print_time_delta(arena, &record);
                            printf("\n");
                        }
                    }
                    else if (record.type == Record_TimeStart)
                    {
                        active_start = state->records.at(index);
                        open_state = Open;
                        
                        if (should_print)
                        {
                            String time_str = get_time_string(arena, record.value);
                            printf("%.*s -> ", string_expand(time_str));
                        }
                    }
                    else
                    {
                        // print count\n
                        assert(record.type != Record_TimeStop);
                        assert(0);
                    }
                }
                
                
                if (open_state == Closed_PrePrint) {
                    open_state = Closed_PostPrint;
                }
            }
        }
        
        
        if (active_start)
        {
#if Def_Slow
            open_start_ending_should_happen_only_once_test += 1;
            assert(open_start_ending_should_happen_only_once_test < 2);
#endif
            date64 today = get_today();
            s32 now_time = get_time();
            
            s32 local_sum = now_time - active_start->value;
            local_sum += safe_truncate_to_s32(today - active_start->date);
            
            
            day_time_sum += local_sum;
            result.time_assumed += local_sum;
            
            
            if (should_print)
            {
                String time_str = get_time_string(arena, now_time);
                print_color(Color_HelpHeader);
                printf("%.*s ", string_expand(time_str));
                
                if (local_sum < Days(1)) {
                    printf("(now)");
                } else {
                    print_color(Color_Error);
                    printf("(missing stop)");
                }
                
                print_color(Color_Reset);
                print_description(active_start);
                print_defered_time_deltas(arena, &defered_time_deltas);
                // print CountDelta defers on new lines
                printf("\n");
            }
            
            
            active_start = nullptr;
        }
        
        
        
        if (should_print && day_header_printed)
        {
            // TODO(f0): Figure out missing ending stuff
            String time = get_time_string(arena, day_time_sum);
            String bar = get_progress_bar_string(arena, day_time_sum, (result.time_assumed == 0));
            
            print_color(Color_Dimmed);
            print_color(Color_Positive);
            printf("Time total: %.*s  ", string_expand(time));
            
            printf("%.*s", string_expand(bar));
            
            print_color(Color_Reset);
            printf("\n");
        }
        
        
        result.time_total += day_time_sum;
        
        if (range.next_day_first_record_index >= whole_range.one_past_last) {
            break;
        }
    }
    
    
    result.next_day_record_index = range.next_day_first_record_index;
    return result;
}









struct Day_Sum_Result
{
    time32 sum;
    b32 closed_range_ending;
};

inline Day_Sum_Result
get_day_sum(Program_State *state, date64 date)
{
    Process_Days_Result days = process_days_from_range(state, date, date, {}, ProcessDays_Calculate);
    
    Day_Sum_Result result = {};
    result.sum = days.time_total - days.time_assumed;
    result.closed_range_ending = (days.time_assumed == 0);
    
    return result;
}




















internal Parse_Complex_Date_Result
get_complex_date(Record_Session *session)
{
    Parse_Complex_Date_Result result = {};
    
    {
        Token token = peek_token(&session->lexer, 0);
        
        if (token.type == Token_Date)
        {
            Parse_Date_Result parse = parse_date(session, token);
            result.date = parse.date;
            result.status = (parse.is_valid) ? Status_Valid : Status_HasErrors;
            advance(&session->lexer);
        }
        else if (token.type == Token_Identifier)
        {
            if (!session->reading_from_file)
            {
                if (token_equals(token, "today") ||
                    (token_equals(token, "now")))
                {
                    result.date = get_today();
                    result.status = Status_Valid;
                    advance(&session->lexer);
                }
                else if (token_equals(token, "yesterday"))
                {
                    result.date = get_today()-Days(1);
                    result.status = Status_Valid;
                    advance(&session->lexer);
                }
                else if (token_equals(token, "tomorrow"))
                {
                    result.date = get_today()+Days(1);
                    result.status = Status_Valid;
                    advance(&session->lexer);
                }
            }
        }
    }
    
    if (result.status == Status_Valid)
    {
        Token token = peek_token(&session->lexer, 0);
        if (token.type == Token_Offset)
        {
            Parse_Number_Result offset = parse_number(token);
            if (!offset.is_valid) {
                result.status = Status_HasErrors;
            } else {
                result.date += offset.value*Days(1);
            }
            
            advance(&session->lexer);
        }
    }
    
    
    return result;
}




























internal void
archive_current_file(Program_State *state, b32 long_format = false)
{
    Arena *arena = &state->arena;
    arena_scope(arena);
    
    // TODO(f0): calculate hash of the whole file for file_name
    
    date64 now = get_current_timestamp();
    String timestamp = get_timestamp_string_for_file(arena, now, long_format);
    
    String file_name = stringf(arena, "%.*s_%s.txt",
                               string_expand(state->title), timestamp.str);
    
    Path archive_path = get_path(state->archive_dir, file_name);
    file_copy(arena, &state->input_path, &archive_path, false);
    
    if (long_format) {
        printf("File archived as: %.*s\n", string_expand(file_name));
    }
}






internal b32
save_to_file(Program_State *state)
{
    Arena *arena = &state->arena;
    arena_scope(arena);
    archive_current_file(state);
    
    b32 success = false;
    
    File_Handle file = file_open_write(arena, &state->input_path);
    if (no_errors(&file))
    {
        Simple_String_Builder builder = {};
        auto add = [&](String string) {
            builder_add(arena, &builder, string);
        };
        
        b32 is_new_day = true;
        s64 active_day_index = 0;
        
        for_u64(record_index, state->records.count)
        {
            Record *record = state->records.at(record_index);
            
            // print day header comment
            if (is_new_day)
            {
                String day_of_week = get_day_of_the_week_string(arena, record->date);
                add(l2s("// "));
                add(day_of_week);
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
                if (record->value < 0) {
                    command = "sub";
                } else {
                    command = "add";
                }
            }
            else
            {
                assert(0);
                continue;
            }
            
            add(string(command));
            add(l2s(" "));
            
            // print date
            if (is_new_day)
            {
                String date_str = get_date_string(arena, record->date);
                add(date_str);
                add(l2s(" "));
            }
            
            
            // print time
            String time_str = get_time_string(arena, record->value);
            add(time_str);
            
            
            // print description
            //Description *desc = get_description(&state->desc_table, record->desc_hash); @desc
            if (record->desc.size)
            {
                add(l2s(" \""));
                add(record->desc);
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
                Day_Sum_Result sum_result = get_day_sum(state, record->date);
                String sum_bar = get_sum_and_progress_bar_string(arena, sum_result.sum, sum_result.closed_range_ending);
                
                add(l2s("// "));
                add(sum_bar);
                add(l2s("\n\n"));
                active_day_index = record_index + 1;
            }
        }
        
        
        String output_string = build_string(&state->arena, &builder);
        file_write_string(&file, output_string);
        file_close(&file);
        
        if (no_errors(&file)) {
            success = true;
        }
        
        state->input_file_mod_time = platform_get_file_mod_time(&state->arena, &state->input_path);
    }
    else
    {
        char *input_path_cstr = cstr_from_path(&state->arena, &state->input_path);
        printf("Failed to write to file: %s\n", input_path_cstr);
    }
    
    return success;
}





internal void
add_record(Record_Session *session, Record *record)
{
    session->add_records_call_count += 1;
    
    if (no_errors(session) && session->load_file_unresolved_errors) {
        session_set_error(session, "New records can't be added because file has unresolved errors");
    }
    
    if (!no_errors(session)) {
        return;
    }
    
    
    
    b32 allowed = false;
    Record *replace_at = nullptr;
    
    
    if (record->type == Record_TimeStop)
    {
        // Stop needs to follow Start
        if (session->active && session->active->type == Record_TimeStart)
        {
            if (session->active->date < record->date ||
                (session->active->date == record->date &&
                 session->active->value < record->value))
            {
                allowed = true;
            }
            else
            {
                session_set_error(session, "Stop needs to be after the most recent Start");
            }
        }
        else
        {
            session_set_error(session, "Stop can be used only when Start is active");
        }
    }
    else
    {
        if (!session->last)
        {
            allowed = true;
        }
        else
        {
            if (session->active && 
                session->active->type == Record_TimeStart &&
                record->type == Record_TimeDelta && 
                session->active->date < record->date)
            {
                session_set_error(session, "Add/Sub needs to have the same date as active Start");
            }
            else if (session->last->date < record->date)
            {
                allowed = true;
            }
            else if (session->last->date == record->date)
            {
                if (!session->active || record->type == Record_TimeDelta)
                {
                    allowed = true;
                }
                else if (session->active->date < record->date)
                {
                    allowed = true;
                }
                else if (session->active->date == record->date)
                {
                    if (record->type == Record_TimeStart)
                    {
                        if (session->active->value < record->value)
                        {
                            allowed = true;
                        }
                        else if (session->last->type == Record_TimeStop &&
                                 session->last->date == record->date &&
                                 session->last->value == record->value)
                        {
                            // Start can replace recent Stop
                            replace_at = session->last;
                            allowed = true;
                        }
                    }
                }
            }
        }
    }
    
    
    
    if (no_errors(session) && !allowed) {
        session_set_error(session, "Out of order input. Try \"edit\" command");
    }
    
    
    
    if (no_errors(session))
    {
        session->change_count += 1;;
        
        if (replace_at)
        {
            session->last = replace_at;
            session->active = replace_at;
        }
        else
        {
            session->last = session->records->grow();
            
            if (record->type == Record_TimeStart ||
                record->type == Record_TimeStop)
            {
                session->active = session->last;
            }
        }
        
        *session->last = *record;
    }
}






internal b32
fill_complex_date_optional(Record_Session *session, Record *record)
{
    b32 success = false;
    
    Parse_Complex_Date_Result parsed_date = get_complex_date(session);
    if (parsed_date.status == Status_Valid)
    {
        record->date = parsed_date.date;
        success = true;
    }
    else if (parsed_date.status == Status_NoMatchigTokens)
    {
        if (!session->reading_from_file)
        {
            record->date = get_today();
            success = true;
        }
        else
        {
            Record *last_record = get_last_record(session);
            if (last_record) {
                record->date = last_record->date;
                success = true;
            } else {
                session_set_error(session, "First record needs to specify date");
            }
        }
    }
    
    
    return success;
}




internal b32
fill_time_optional(Record_Session *session, Record *record)
{
    b32 success = false;
    Token token = peek_token(&session->lexer, 0);
    
    if (token.type == Token_Time)
    {
        advance(&session->lexer);
        
        Parse_Time_Result parsed_time = parse_time(session, token);
        if (parsed_time.is_valid)
        {
            record->value = parsed_time.value;
            success = true;
        }
    }
    else if (!session->reading_from_file)
    {
        record->value = get_time();
        success = true;
    }
    
    return success;
}

internal b32
fill_complex_date_time_optional(Record_Session *session, Record *record)
{
    b32 success = false;
    Token token = peek_token(&session->lexer, 0);
    
    if (token.type == Token_Identifier &&
        token_equals(token, "now"))
    {
        advance(&session->lexer);
        success = true;
        date64 now = get_current_timestamp();
        
        token = peek_token(&session->lexer, 0);
        if (token.type == Token_Offset)
        {
            advance(&session->lexer);
            Parse_Number_Result offset = parse_number(token);
            success = offset.is_valid;
            now += offset.value*Minutes(1);
        }
        
        record->date = truncate_to_date(now);
        record->value = truncate_to_time(now);
    }
    else
    {
        success = (fill_complex_date_optional(session, record) &&
                   fill_time_optional(session, record));
    }
    
    return success;
}





internal b32
fill_time_required(Record_Session *session, Record *record)
{
    b32 success = false;
    Token token = peek_token(&session->lexer, 0);
    
    if (token.type == Token_Time)
    {
        advance(&session->lexer);
        
        Parse_Time_Result parsed_time = parse_time(session, token);
        if (parsed_time.is_valid)
        {
            record->value = parsed_time.value;
            success = true;
        }
    }
    
    return success;
}





internal void
fill_description_optional(Record_Session *session, Record *record)
{
    Token token = peek_token(&session->lexer, 0);
    
    if (token.type == Token_String)
    {
        advance(&session->lexer);
        //record->desc_hash = add_description(&session->desc_table, forward->token); @desc
        record->desc = token.text;
    }
}





internal void
parse_command_start(Record_Session *session)
{
    Record record = {};
    record.type = Record_TimeStart;
    
    b32 success = fill_complex_date_time_optional(session, &record);
    
    if (success) {
        fill_description_optional(session, &record);
    }
    
    if (success) {
        add_record(session, &record);
    } else {
        session_set_error(session, "Incorect command usage. Use:\n"
                          "start [yyyy-MM-dd] (hh:mm) [\"description\"]");
    }
}



internal void
parse_command_stop(Record_Session *session)
{
    Record record = {};
    record.type = Record_TimeStop;
    
    b32 success = fill_complex_date_time_optional(session, &record);
    
    if (success) {
        add_record(session, &record);
    } else {
        session_set_error(session, "Incorect command usage. Use:\n"
                          "start [yyyy-MM-dd] (hh:mm)");
    }
}



internal void
parse_command_add_sub(Record_Session *session, b32 is_add)
{
    Record record = {};
    record.type = Record_TimeDelta;
    
    b32 success = fill_complex_date_optional(session, &record);
    
    if (success)
    {
        success = fill_time_required(session, &record);
        if (!is_add) {
            record.value *= -1;
        }
        
        if (success && record.value == 0) {
            session_set_error(session, "Time equal to zero!");
            return;
        }
    }
    
    if (success) {
        fill_description_optional(session, &record);
    }
    
    if (success)
    {
        add_record(session, &record);
    }
    else
    {
        char error_message[512];
        snprintf(error_message, sizeof(error_message),
                 "Incorect command usage. Use:\n"
                 "%s [yyyy-MM-dd] (hh:mm) [\"description\"]",
                 (is_add) ? "add" : "sub");
        
        session_set_error(session, error_message);
    }
}




internal Date_Range_Result
get_max_date_range(b32 valid_status)
{
    Date_Range_Result result;
    result.status = (valid_status ? Status_Valid : Status_NoMatchigTokens);
    result.first = 1;
    result.last = S64_Max;
    
    return result;
}

internal Date_Range_Result
get_date_range(Record_Session *session)
{
    Date_Range_Result result = get_max_date_range(false);
    b32 success = true;
    b32 has_matching_token = false;
    
    {
        Token token = peek_token(&session->lexer, 0);
        
        if (token.type == Token_Identifier &&
            token_equals(token, "from"))
        {
            has_matching_token = true;
            advance(&session->lexer);
            
            Parse_Complex_Date_Result date = get_complex_date(session);
            success = is_valid(date.status);
            result.first = date.date;
        }
    }
    
    
    if (success)
    {
        Token token = peek_token(&session->lexer, 0);
        
        if (token.type == Token_Identifier &&
            token_equals(token, "to"))
        {
            has_matching_token = true;
            advance(&session->lexer);
            
            Parse_Complex_Date_Result date = get_complex_date(session);
            success = is_valid(date.status);
            result.last = date.date;
        }
    }
    
    
    
    if (success)
    {
        Token token = peek_token(&session->lexer, 0);
        
        if (!has_matching_token)
        {
            if (token_equals(token, "all"))
            {
                has_matching_token = true;
                result = get_max_date_range(true);
                advance(&session->lexer);
            }
            else
            {
                Parse_Complex_Date_Result date = get_complex_date(session);
                result.status = date.status; // may return no matching token
                result.first = date.date;
                result.last = date.date;
            }
        }
        else
        {
            result.status = Status_Valid;
        }
    }
    else
    {
        result.status = Status_HasErrors;
    }
    
    
    return result;
}







internal Date_Range_Result
get_recent_days_range(Virtual_Array<Record> *records)
{
    date64 today = get_today();
    date64 start = today - Days(31);
    
    s64 record_count = records->count;
    if (record_count)
    {
        date64 last_date = records->at(record_count - 1)->date;
        if (last_date > today)
        {
            today = last_date;
        }
        
        s64 past_index = record_count - 120;
        if (past_index < 0)
        {
            past_index = 0;
        }
        
        date64 start_date = records->at(past_index)->date;
        if (start_date < start)
        {
            start = start_date;
        }
    }
    
    Date_Range_Result result = {start, today, Status_Valid};
    return result;
}



internal void
parse_command_show(Program_State *state, Record_Session *session)
{
    Date_Range_Result range = get_date_range(session);
    
    if (range.status != Status_HasErrors)
    {
        String filter = {};
        
        Token token = peek_token(&session->lexer, 0);
        if (token.type == Token_String) {
            advance(&session->lexer);
            filter = token.text;
        }
        
        char *message = nullptr;
        
        if (range.status == Status_NoMatchigTokens)
        {
            if (filter.size)
            {
                range = get_max_date_range(true);
            }
            else
            {
                range = get_recent_days_range(session->records);
                message = "Range assumed from xxxx-xx-xx to xxxx-xx-xx; "
                    "To use all records specify filter or use \"show all\"\n";
            }
        }
        
        process_days_from_range(state, range.first, range.last, filter, ProcessDays_Print);
        
        if (message) {
            print_color(Color_Dimmed);
            printf("%s", message);
            print_color(Color_Reset);
        }
    }
    else
    {
        session_set_error(session, "Incorect command usage. Use:\n"
                          "show [yyyy-MM-dd]\n"
                          "show [from yyyy-MM-dd] [to yyyy-MM-dd]\n");
    }
}


inline b32
increase_index_to_next_day(Virtual_Array<Record> *records, u64 *index)
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




internal void
print_summary(Program_State *state, Granularity granularity,
              date64 date_begin, date64 date_end,
              String filter)
{
    Arena *arena = &state->arena;
    arena_scope(arena);
    date64 today = get_today();
    
    
    Record *record = nullptr;
    for_u64(record_index, state->records.count)
    {
        Record *record_test = state->records.at(record_index);
        if (record_test->date >= date_begin)
        {
            record = record_test;
            break;
        }
    }
    
    
    
    if (record && (record->date <= date_end))
    {
        for (;;)
        {
            Boundaries_Result boundary = {};
            switch (granularity)
            {
                case Granularity_Days: {
                    boundary.first = record->date;
                    boundary.last = record->date;
                } break;
                
                default: { assert(0); } // fall
                case Granularity_Months: {
                    boundary = get_month_boundaries(record->date);
                } break;
                
                case Granularity_Quarters: {
                    boundary = get_quarter_boundaries(record->date);
                } break;
                
                case Granularity_Years: {
                    boundary = get_year_boundaries(record->date);
                } break;
            }
            
            
            date64 first_date = pick_bigger(date_begin, boundary.first);
            date64 last_date = pick_smaller(date_end, boundary.last);
            Process_Days_Result days = process_days_from_range(state, first_date, last_date,
                                                               filter, ProcessDays_Calculate);
            
            String test_start_date = get_date_string(&state->arena, first_date);
            String test_last_date = get_date_string(&state->arena, last_date);
            
            
            
            if (days.time_total)
            {
                s32 day_count = safe_truncate_to_s32(((last_date - first_date) / Days(1)) + 1);
                
                if (days.next_day_record_index == state->records.count)
                {
                    s32 current_day_count = safe_truncate_to_s32(((today - first_date) / Days(1)) + 1);
                    
                    assert(current_day_count > 0);
                    assert(current_day_count <= day_count);
                    day_count = current_day_count;
                }
                
                
                String date_str = get_date_string(arena, first_date);
                print_color(Color_Date);
                printf("%.*s", string_expand(date_str));
                
                if (boundary.description) {
                    printf(" (%s)", boundary.description);
                }
                
                String sum_str = get_time_string(arena, days.time_total);
                print_color(Color_Positive);
                printf("\tsum: %.*s\t", string_expand(sum_str));
                
                
                b32 is_range_closed = !(today <= last_date);
                String bar = {};
                
                if (day_count > 1)
                {
                    s32 avg = days.time_total / day_count;
                    String avg_str = get_time_string(arena, avg);
                    printf("avg(/%3d): %.*s\t", day_count, string_expand(avg_str));
                    
                    bar = get_progress_bar_string(arena, avg, is_range_closed);
                }
                else
                {
                    bar = get_progress_bar_string(arena, days.time_total, is_range_closed);
                }
                
                printf("%.*s\n", string_expand(bar));
            }
            
            
            if (days.next_day_record_index >= state->records.count || days.next_day_record_index == 0) {
                break;
            }
            
            record = state->records.at(days.next_day_record_index);
            if (record->date > date_end) {
                break;
            }
        }
    }
    
    print_color(Color_Reset);
}






internal void
parse_command_summary(Program_State *state, Record_Session *session)
{
    Granularity granularity = Granularity_Months;
    // TODO: Pull out granularity check.
    
    {
        Token token = peek_token(&session->lexer, 0);
        
        if (token.type == Token_Identifier)
        {
            if (token_equals(token, "days") ||
                token_equals(token, "d"))
            {
                advance(&session->lexer);
                granularity = Granularity_Days;
            }
            else if (token_equals(token, "months") ||
                     token_equals(token, "m"))
            {
                advance(&session->lexer);
                granularity = Granularity_Months;
            }
            else if (token_equals(token, "quarters") ||
                     token_equals(token, "q"))
            {
                advance(&session->lexer);
                granularity = Granularity_Quarters;
            }
            else if (token_equals(token, "years") ||
                     token_equals(token, "y"))
            {
                advance(&session->lexer);
                granularity = Granularity_Years;
            }
        }
    }
    
    
    Date_Range_Result range = get_date_range(session);
    
    if (range.status == Status_NoMatchigTokens) {
        range = get_max_date_range(true);
    }
    
    
    String filter = {};
    {
        Token token = peek_token(&session->lexer, 0);
        if (token.type == Token_String) {
            advance(&session->lexer);
            filter = token.text;
        }
    }
    
    
    print_summary(state, granularity, range.first, range.last, filter);
    
    
    if (range.status == Status_HasErrors)
    {
        session_set_error(session, "Incorect command usage. Use:\n"
                          "summary [granularity]\n"
                          "summary [granularity] [yyyy-MM-dd]\n"
                          "summary [granularity] [from yyyy-MM-dd] [to yyyy-MM-dd]\n"
                          "\n\tgranularity - days/months (or d/m)\n");
    }
}


internal void
parse_command_exit(Program_State *state, Record_Session *session)
{
    Token token = peek_token(&session->lexer, 0);
    
    if ((token.type == Token_Identifier) && (token_equals(token, "no-save")))
    {
        advance(&session->lexer);
        exit(0);
    }
    else
    {
        if (no_errors(session))
        {
            if (session->change_count > 0)
            {
                b32 save_result = save_to_file(state);
                
                if (save_result) {
                    exit(0);
                } else {
                    session_set_error(session, "Failed to save to file!");
                }
            }
            else
            {
                exit(0);
            }
        }
        else
        {
            session_set_error(session, "Errors detected, exit aborted");
        }
    }
}


internal Record_Session
create_record_session_no_lexer(Arena *arena, Virtual_Array<Record> *records,
                                   b32 reading_from_file)
{
    Record_Session result = {};
    result.records = records;
    result.scope = create_program_scope(arena, records);
    
    for_u64(i, records->count)
    {
        u64 index = records->count - i - 1;
        Record *record = records->at(index);
        
        if (!result.last) {
            result.last = record;
        }
        
        if (!result.active) { 
            if (record->type == Record_TimeStart || record->type == Record_TimeStop) {
                result.active = record;
                break;
            }
        }
    }
    
    result.no_errors = true;
    result.reading_from_file = reading_from_file;
    return result;
}


internal Record_Session
create_record_session(Arena *arena, Virtual_Array<Record> *records,
                      b32 reading_from_file, char *content)
{
    Record_Session result = create_record_session_no_lexer(arena, records, reading_from_file);
    result.lexer = create_lexer(content);
    return result;
}


internal void
process_input(Program_State *state, Record_Session *session)
{
#define Error_Cmd_Exclusive session_set_error(session, "This command can be used only from console")
    
    if (state->load_file_error) {
        session->load_file_unresolved_errors = true;
    }
    
    b32 parsing = true;
    b32 reading_from_file = session->reading_from_file;
    
    while (parsing)
    {
        if (!no_errors(session)) {
            break;
        }
        
        Token token = peek_token(&session->lexer, 0);
        advance(&session->lexer);
        session->current_command_token = token;
        
        switch (token.type)
        {
            case Token_Identifier: {
                if (token_equals(token, "start"))
                {
                    parse_command_start(session);
                }
                else if (token_equals(token, "stop"))
                {
                    parse_command_stop(session);
                }
                else if (token_equals(token, "add"))
                {
                    parse_command_add_sub(session, true);
                }
                else if (token_equals(token, "subtract") || token_equals(token, "sub"))
                {
                    parse_command_add_sub(session, false);
                }
                else if (token_equals(token, "show"))
                {
                    if (reading_from_file) {
                        Error_Cmd_Exclusive;
                    } else {
                        parse_command_show(state, session);
                    }
                }
                else if (token_equals(token, "summary"))
                {
                    if (reading_from_file) {
                        Error_Cmd_Exclusive;
                    } else {
                        parse_command_summary(state, session);
                    }
                }
                else if (token_equals(token, "exit"))
                {
                    if (reading_from_file) {
                        Error_Cmd_Exclusive;
                    } else {
                        parse_command_exit(state, session);
                    }
                }
                else if (token_equals(token, "save"))
                {
                    if (reading_from_file) {
                        Error_Cmd_Exclusive;
                    } else {
                        b32 save_result = save_to_file(state);
                        if (save_result) {
                            printf("File saved\n");
                        } else {
                            printf("Failed to save\n");
                        }
                    }
                }
                else if (token_equals(token, "archive"))
                {
                    if (reading_from_file) {
                        Error_Cmd_Exclusive;
                    } else {
                        archive_current_file(state, true);
                    }
                }
                else if (token_equals(token, "reload"))
                {
                    if (reading_from_file) {
                        Error_Cmd_Exclusive;
                    } else {
                        if (no_errors(session)) {
                            load_file(state);
                            return;
                        } else {
                            session_set_error(session, "Load aborted");
                        }
                    }
                }
                else if (token_equals(token, "time"))
                {
                    if (reading_from_file) {
                        Error_Cmd_Exclusive;
                    } else {
                        date64 now = get_current_timestamp();
                        String now_str = get_timestamp_string(&state->arena, now);
                        printf("Current time: %.*s\n", string_expand(now_str));
                    }
                }
                else if (token_equals(token, "edit"))
                {
                    if (reading_from_file) {
                        Error_Cmd_Exclusive;
                    } else {
                        path_open_in_default_program(&state->arena, &state->input_path);
                    }
                }
                else if (token_equals(token, "dir"))
                {
                    if (reading_from_file) {
                        Error_Cmd_Exclusive;
                    } else {
                        directory_open_in_default_program(&state->arena, state->exe_path.directory);
                    }
                }
                else if (token_equals(token, "clear"))
                {
                    if (reading_from_file) {
                        Error_Cmd_Exclusive;
                    } else {
                        platform_clear_screen();
                    }
                }
                else if (token_equals(token, "help"))
                {
                    if (reading_from_file)
                    {
                        Error_Cmd_Exclusive;
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
                    char error_message[512];
                    snprintf(error_message, sizeof(error_message),
                             "%.*s - unexpected identifier", string_expand(token.text));
                    
                    session_set_error(session, error_message);
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
                char error_message[512];
                snprintf(error_message, sizeof(error_message),
                         "%.*s - unexpected element", string_expand(token.text));
                
                session_set_error(session, error_message);
            }
            break;
        }
    }
    
    
    
    
    
    
    if (no_errors(session))
    {
        if (!reading_from_file &&
            session->change_count > 0)
        {
            save_to_file(state);
        }
    }
    else
    {
        if (session->add_records_call_count > 0)
        {
            print_color(Color_Warning);
            printf("[Warning] Records not added due to errors");
            print_color(Color_Reset);
            printf("\n");
        }
        
        pop_program_scope(&session->scope);
    }
}





internal void
load_file(Program_State *state)
{
    pop_program_scope(&state->initial_scope);
    state->load_file_error = false;
    
    b32 load_successful = false;
    
    for (u32 load_tries = 0;
         (load_tries < 5 && !load_successful);
         ++load_tries)
    {
        char *file_content = read_entire_file_and_zero_terminate(&state->arena, &state->input_path);
        
        if (file_content)
        {
            Record_Session session = create_record_session(&state->arena, &state->records, true, file_content);
            process_input(state, &session);
            
            if (no_errors(&session))
            {
                printf("File loaded\n");
            }
            else
            {
                state->load_file_error = true;
                printf("[Warning] File contains errors. It requires manual fixing. "
                       "Use \"edit\" to open it in default editor\n");
            }
            
            load_successful = true;
        }
        
        if (!load_successful) {
            platform_sleep(10);
        }
    }
    
    
    if (!load_successful)
    {
        state->load_file_error = true;
        arena_scope(&state->arena);
        char *file_name = cstr_from_path(&state->arena, &state->input_path);
        printf("[Critial error] Failed to load from file: %s\n", file_name);
    }
    
    
    state->input_file_mod_time = platform_get_file_mod_time(&state->arena, &state->input_path);
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







enum Cmd_Arugment_Type
{
    Cmd_None,
    Cmd_Input_File_Path
};


s32 main(int argument_count, char **arguments)
{
    //~ NOTE: Initialization
    Program_State state = {};
    state.arena = create_virtual_arena();
    state.records = create_virtual_array<Record>();
    state.desc_table = create_description_table(4096);
    //clear_memory(&state);
    Arena *arena = &state.arena;
    
    
    //~ command line inputs
    b32 reformat_mode = false;
    { 
        Cmd_Arugment_Type type = Cmd_None;
        
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
                        global_state.colors_disabled = true;
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
                state.input_path = path_from_string(arena, string(arg));
                type = Cmd_None;
            }
            else
            {
                assert(0);
            }
        }
        
        if (!global_state.colors_disabled) {
            initialize_colors();
        }
    }
    
    
    
    //~ initialize essential state
    initialize_timezone_offset();
    
    state.exe_path = get_this_executable_path(arena);
    state.title = trim_from_index_of_reverse(state.exe_path.file_name, '.');
    
    if (state.input_path.file_name.size == 0)
    {
        state.input_path = state.exe_path;
        state.input_path.file_name = concatenate(arena, state.title, l2s(".txt"));
    }
    
    
    state.archive_dir = directory_append(arena, state.exe_path.directory, l2s("archive"));
    directory_create(arena, state.archive_dir);
    
    // NOTE: Save initial program state
    state.initial_scope = create_program_scope(&state.arena, &state.records);
    
    
    
    
    
    
    
    //~ initial file creation and load
    {
        arena_scope(arena);
        
        char *input_path_cstr = cstr_from_path(arena, &state.input_path);
        if (!file_exists(input_path_cstr))
        {
            if (!reformat_mode)
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
            else
            {
                printf("File doesn't exist: %s. Exiting.\n", input_path_cstr);
                exit(1);
            }
        }
    }
    
    
    load_file(&state);
    if (!state.load_file_error)
    {
        Date_Range_Result range = get_recent_days_range(&state.records);
        process_days_from_range(&state, range.first, range.last, {}, ProcessDays_Print);
        save_to_file(&state);
        
        if (reformat_mode) {
            printf("File reformated. Exiting.\n");
            exit(0);
        }
    }
    else
    {
        if (reformat_mode) {
            printf("Reformatting skipped due to errors. Exiting with force_save.\n");
            // TODO(f0): Change this? -r should be --test_mode? Or --format --force_save
            save_to_file(&state);
            exit(1);
        }
    }
    
    
    
    
    //~
    Thread_Memory thread_memory = {};
    sprintf(thread_memory.cursor, "::>");
    platform_create_thread(read_from_keyboard, &thread_memory);
    
    
    
    //~
    for (;;)
    {
        Time32ms now = get_time32_ms();
        
        if (thread_memory.new_data)
        {
            state.last_input_time = now;
            Record_Session session = create_record_session_no_lexer(&state.arena, &state.records, false);
            char *input_copy = copy_cstr(&state.arena, thread_memory.input_buffer);
            session.lexer = create_lexer(input_copy);
                                                     
            process_input(&state, &session);
            if (no_errors(&session))
            {
                if (session.change_count > 0)
                {
                    Record *last_record = get_last_record(&session);
                    process_days_from_range(&state, last_record->date, last_record->date, {}, ProcessDays_PrintAltColor);
                }
            }
            
            thread_memory.new_data = false;
        }
        
        
        
        File_Time mod_time = platform_get_file_mod_time(&state.arena, &state.input_path);
        b32 source_file_changed = platform_compare_file_time(state.input_file_mod_time, mod_time) != 0;
        
        if (source_file_changed)
        {
            state.last_input_time = now;
            load_file(&state);
            printf(thread_memory.cursor);
        }
        
        
        s32 input_time_delta = (s32)now.t - (s32)state.last_input_time.t;
        if (input_time_delta > 1000*5)
        {
            u32 sleep_duration = input_time_delta / 8192;
            sleep_duration = pick_smaller(sleep_duration, 100);
            platform_sleep(sleep_duration);
        }
    }
}


