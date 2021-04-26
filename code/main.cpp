/* TODO:
  [ ] Unify help messages for incorrect inputs & help command
  [?] ??Group commands, its function pointers, help items all in one array? Could be used for "help summary / help all"
  -> [ ] Could be used to identify errors in previous command (bad parameter order?)
  
  [ ] Unexpected indentifer error should print _LINE_ with error (and maybe idenfiter should be highlighted)
*/


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
#include "parse.cpp"
#include "print.cpp"
#include "records.cpp"
#include "process_helpers.cpp"

//
//~ Session
//
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




//
//~
//

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
                date64 date = record->date;
                Process_Days_Result days = process_days_from_range(state, active_day_index,
                                                                   date, date, {}, ProcessDays_Calculate);
                
                s32 non_assumed_time = days.time_total - days.time_assumed;
                b32 closed_range_ending = (days.time_assumed == 0);
                
                String sum_bar = get_sum_and_progress_bar_string(arena, non_assumed_time, closed_range_ending);
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








//
//~
//
#include "commands.cpp"






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
                      b32 reading_from_file, u8 *content)
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
                    process_command_start(session);
                }
                else if (token_equals(token, "stop"))
                {
                    process_command_stop(session);
                }
                else if (token_equals(token, "add"))
                {
                    process_command_add_sub(session, true);
                }
                else if (token_equals(token, "subtract") || token_equals(token, "sub"))
                {
                    process_command_add_sub(session, false);
                }
                else if (token_equals(token, "show"))
                {
                    if (reading_from_file) {
                        Error_Cmd_Exclusive;
                    } else {
                        process_command_show(state, session);
                    }
                }
                else if (token_equals(token, "summary"))
                {
                    if (reading_from_file) {
                        Error_Cmd_Exclusive;
                    } else {
                        process_command_summary(state, session);
                    }
                }
                else if (token_equals(token, "exit"))
                {
                    if (reading_from_file) {
                        Error_Cmd_Exclusive;
                    } else {
                        process_command_exit(state, session);
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
                        open_in_default_program(&state->arena, &state->input_path);
                    }
                }
                else if (token_equals(token, "dir"))
                {
                    if (reading_from_file) {
                        Error_Cmd_Exclusive;
                    } else {
                        open_in_default_program(&state->arena, state->exe_path.directory);
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
        File_Content file_content = read_entire_file_and_zero_terminate(&state->arena, &state->input_path);
        
        if (no_errors(&file_content))
        {
            Record_Session session = create_record_session(&state->arena, &state->records, true, file_content.content.str);
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
    
    state.exe_path = current_executable_path(arena);
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
        process_days_from_range(&state, 0, range.first, range.last, {}, ProcessDays_Print);
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
                    process_days_from_range(&state, 0, last_record->date, last_record->date, {}, ProcessDays_PrintAltColor);
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


