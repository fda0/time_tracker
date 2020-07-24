/*
    TODO(mateusz):
    * Colors in output.
    
    * Add edit command - opens editor with file.
    * Add help command.
    * Don't use relative path by default - use path based on executable root path.

*/


// NOTE(mateusz): This program ignores concept of timezones to simplify usage.

#include "tt_main.h"

#include "tt_token.cpp"
#include "tt_string.cpp"
#include "tt_time.cpp"



internal char *create_description(Memory_Arena *arena, Token token)
{
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

// TODO(mateusz): Smarter parsing of incorrect dates/incomplete formats.
internal s32 parse_number(char *src, s32 count)
{
    s32 result = 0;

    s32 multiplier = 1;
    for (s32 index = count - 1;
         index >= 0;
         --index, multiplier *= 10)
    {
        Assert(src[index] >= '0' && src[index] <= '9');
        s32 to_add = (src[index] - '0') * multiplier;
        result += to_add;
    }

    return result;
}

internal time_t parse_date(Token token)
{
    // NOTE(mateusz): Supported format: 2020-12-31
    Assert(token.text_length >= 4 + 1 + 2 + 1 + 2); 
    char *text = token.text;

    tm date = {};
    date.tm_year = parse_number(text, 4) - 1900;
    text += 5;

    date.tm_mon = parse_number(text, 2) - 1;
    text += 3;

    date.tm_mday = parse_number(text, 2);
    text += 2;

    time_t timestamp = platform_tm_to_time(&date);
    return timestamp;
}

internal time_t parse_time(Token token)
{
    // NOTE(mateusz): Supported format: 23:59
    Assert(token.text_length >= 2 + 1 + 2); 
    char *text = token.text;

    s32 hour = parse_number(text, 2);
    text += 3;

    s32 minute = parse_number(text, 2);
    text += 2;

    time_t timestamp = hour*60*60 + minute*60;
    return timestamp;
}


internal void calculate_day_sum_and_validate(Program_State *state, Day *day, 
                                             b32 print, FILE *errors_to_file = NULL)
{
    FILE *error_output = stdout;
    if (errors_to_file) error_output = errors_to_file;

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
                printf("\t\"%s\"\n", entry->description);
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
                fprintf(error_output, "// [Error #%d] two start commands in a row - stop is missing\n", 
                        state->save_error_count++);
                return;
            }

            start = entry;
        }
        else if (entry->type == Entry_Stop)
        {
            if (!start)
            {
                fprintf(error_output, "// [Error #%d] stop is missing its start\n", 
                        state->save_error_count++);
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
                fprintf(error_output, "// [Error #%d] stop time is earlier than start time\n", 
                        state->save_error_count++);
                return;
            }


            day->sum += (s32)(stop_time - start_time);

            if (print) print_work_time_row(start, entry, offset_sum);

            start = NULL;
            offset_sum = 0;
        }
    }


    if (start) // NOTE(mateusz): Day ends with start - missing end entry.
    {
        time_t now = get_current_time(state);
        
        time_t start_time = start->date + start->time;

        if (now > start_time &&
            now - Days(1) < start_time)
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

            fprintf(error_output, "// [Error #%d] Missing last stop for a day\n", state->save_error_count++);
        }
    }

    if (offset_sum != 0)
    {
        print_offset(offset_sum);
        printf("\n");
    }
}




internal void print_all_days(Program_State *state)
{
    for (u32 day_index = 0;
         day_index < state->day_count;
         ++day_index)
    {
        Day *day = &state->days[day_index];

        char date_str[64];
        get_date_string(date_str, sizeof(date_str), day->date_start);
        printf("\n%s\n", date_str);

        calculate_day_sum_and_validate(state, day, true);

        char sum_bar_str[MAX_SUM_AND_PROGRESS_BAR_STRING_SIZE];
        get_sum_and_progress_bar_string(sum_bar_str, sizeof(sum_bar_str), day);

        printf("%s\n", sum_bar_str);
    }
}

internal void archive_current_file(Program_State *state, b32 long_format = false)
{
    time_t now = get_current_time(state);

    char timestamp[MAX_PATH];
    get_timestamp_string_for_file(timestamp, sizeof(timestamp), now, long_format);

    char archive_filename[MAX_PATH];
    snprintf(archive_filename, sizeof(archive_filename), "%s%s_%s.txt", 
             state->archive_directory, state->input_filename, timestamp);

    platform_copy_file(state->input_filename, archive_filename);

    if (long_format) printf("File archived as: %s\n", archive_filename);
}

internal void save_to_file(Program_State *state)
{
    archive_current_file(state);
    state->save_error_count = 0;

    FILE *file = fopen(state->input_filename, "w");
    if (file)
    {
        for (u32 day_index = 0;
             day_index < state->day_count;
             ++day_index)
        {
            Day *day = &state->days[day_index];
            calculate_day_sum_and_validate(state, day, false, file);

            char day_of_week_str[16];
            get_day_of_the_week_string(day_of_week_str, sizeof(day_of_week_str), day->date_start);
            fprintf(file, "// %s\n", day_of_week_str);

            for (Time_Entry *entry = &day->first_time_entry;
                 entry;
                 entry = entry->next_in_day)
            {
                if (entry->type == Entry_Start)
                {
                    time_t time = entry->date + entry->time;
                    char timestamp[MAX_TIMESTAMP_STRING_SIZE];
                    get_timestamp_string(timestamp, sizeof(timestamp), time);
                    fprintf(file, "start %s", timestamp);
                }
                else if (entry->type == Entry_Stop)
                {
                    if (entry->date == day->date_start)
                    {
                        char time_string[MAX_TIME_STRING_SIZE];
                        get_time_string(time_string, sizeof(time_string), entry->time);

                        fprintf(file, "stop %s", time_string);
                    }
                    else
                    {
                        time_t time = entry->date + entry->time;
                        char timestamp[MAX_TIMESTAMP_STRING_SIZE];
                        get_timestamp_string(timestamp, sizeof(timestamp), time);
                        fprintf(file, "stop %s", timestamp);
                    }
                }
                else 
                {
                    char *keyword = NULL;
                    if (entry->type == Entry_Add)           keyword = "add";
                    else if (entry->type == Entry_Subtract) keyword = "sub";
                    else
                    {
                        Invalid_Code_Path;
                        continue;
                    }

                    char time_string[MAX_TIME_STRING_SIZE];
                    get_time_string(time_string, sizeof(time_string), entry->time);
                    fprintf(file, "%s %s", keyword, time_string);
                }

                if (entry->description)
                {
                    fprintf(file, " \"%s\";\n", entry->description);
                }
                else
                {
                    fprintf(file, ";\n");
                }
            }

            char sum_bar_str[MAX_SUM_AND_PROGRESS_BAR_STRING_SIZE];
            get_sum_and_progress_bar_string(sum_bar_str, sizeof(sum_bar_str), day);
            fprintf(file, "// %s\n\n", sum_bar_str);
        }

        if (state->load_error_count > 0)
        {
            fprintf(file, "// Load error count: %d\n", state->load_error_count);
            fprintf(stdout, "// Load error count: %d\n", state->load_error_count);
        }

        if (state->save_error_count > 0)
        {
            fprintf(file, "// Save error count: %d\n", state->save_error_count);
            fprintf(stdout, "// Save error count: %d\n", state->save_error_count);
        }
    
        fclose(file);

        state->loaded_input_mod_time = platform_get_file_mod_time(state->input_filename);
    }
    else
    {
        printf("Failed to write to file: %s\n", state->input_filename);
    }

    state->change_count = 0;
}


internal void process_time_entry(Program_State *state, Time_Entry *entry, b32 reading_from_file)
{
    // TODO(mateusz): Implement insert sorting for start/stop/add/sub.                

    if (!reading_from_file)
    {
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
                    time_t time = turnacate_to_time(now);
                    entry->time = time;
                }
                else
                {
                    printf("You need to specify time (example: 01:10)\n");
                    return;
                }
            }
        }
    }
    

    //
    // NOTE(mateusz): Select correct day slot.
    //

    Day *day = &state->days[state->day_count - 1];

    if ((state->day_count == 0) ||
        ((day->date_start < entry->date) && 
            (entry->type != Entry_Stop)))
    {
        if (entry->date == 0)
        {
            Invalid_Code_Path;
            printf("[Warning] First item can't have missing date! - 1970 assumed :(\n");
            entry->date += Days(1);
        }

        Assert(state->day_count < Array_Count(state->days)); // TODO(mateusz): Dynamic memory for days.
        day = &state->days[state->day_count++];
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
            printf("[Error #%d] Out of order item insertion not supported yet :(\n", 
                   state->save_error_count++);
            Invalid_Code_Path;
        }
        else
        {
            printf("Can't insert entry into the past yet :(\n");
            return;
        }
    }



    //
    // NOTE(mateusz): Add Time_Entry to previously selected day.
    //

    Time_Entry *entry_dest = &day->first_time_entry;
    while (entry_dest->type != Entry_None)
    {
        if (entry_dest->next_in_day == NULL)
        {
            entry_dest->next_in_day = Push_Struct(&state->struct_arena, Time_Entry);
        }

        entry_dest = entry_dest->next_in_day;
    }

    ++state->change_count;
    *entry_dest = *entry;
}



internal void process_input(char *content, Program_State *state, 
                            b32 reading_from_file, b32 *program_is_running = NULL)
{
    Tokenizer tokenizer = {};
    tokenizer.at = content;

    Instruction_Type instruction = Ins_None;
    u32 flag = 0;
    Time_Entry entry = {};
    bool parsing = true;
    while (parsing)
    {
        Token token = get_token(&tokenizer);
        switch (token.type)
        {

            //
            // NOTE(mateusz): Processing macros.
            //

            #define Print_Not_Supported_In_File                     \
            {                                                       \
                printf("%.*s keyword is not supported in files\n",  \
                       (s32)token.text_length, token.text);         \
            }


            #define Program_Interface_Only       \
            if (reading_from_file)               \
            {                                    \
                Print_Not_Supported_In_File;     \
                instruction = Ins_Unsupported;   \
                continue;                        \
            }


            #define Continue_If_Instruction_Already_Set           \
            if (instruction != Ins_None)                          \
            {                                                     \
                printf("%.*s is ignored"                          \
                       " - other instruction is already set\n",   \
                       (s32)token.text_length, token.text);       \
                continue;                                         \
            }





            case Token_Identifier:
            {
                if (token_equals(token, "start")) 
                {
                    Continue_If_Instruction_Already_Set;

                    entry.type = Entry_Start;
                    instruction = Ins_Time_Entry;
                }
                else if (token_equals(token, "stop"))
                {
                    Continue_If_Instruction_Already_Set;
                    
                    entry.type = Entry_Stop;
                    instruction = Ins_Time_Entry;
                }
                else if (token_equals(token, "add"))
                {
                    Continue_If_Instruction_Already_Set;
                    
                    entry.type = Entry_Add;
                    instruction = Ins_Time_Entry;
                }
                else if (token_equals(token, "subtract") ||
                         token_equals(token, "sub"))
                {
                    Continue_If_Instruction_Already_Set;
                    
                    entry.type = Entry_Subtract;
                    instruction = Ins_Time_Entry;
                }
                else if (token_equals(token, "show"))
                {
                    Continue_If_Instruction_Already_Set;
                    Program_Interface_Only;

                    instruction = Ins_Show;
                }
                else if (token_equals(token, "exit"))
                {
                    Continue_If_Instruction_Already_Set;
                    Program_Interface_Only;

                    instruction = Ins_Exit;
                }
                else if (token_equals(token, "save"))
                {
                    Continue_If_Instruction_Already_Set;
                    Program_Interface_Only;

                    instruction = Ins_Save;
                }
                else if (token_equals(token, "archive"))
                {
                    Continue_If_Instruction_Already_Set;
                    Program_Interface_Only;

                    instruction = Ins_Archive;
                }
                else if (token_equals(token, "time"))
                {
                    Continue_If_Instruction_Already_Set;
                    Program_Interface_Only;

                    instruction = Ins_Time;
                }
                else if (token_equals(token, "no-save"))
                {
                    Continue_If_Instruction_Already_Set;
                    Program_Interface_Only;

                    flag |= Flag_No_Save;
                }
                else
                {
                    printf("Unknown identifier: %.*s\n", (s32)token.text_length, token.text);
                }
            } break;

            case Token_Date:
            {
                if (instruction == Ins_Time_Entry)
                {
                    if (!entry.date)
                    {
                        auto date = parse_date(token);
                        entry.date = date;
                    }
                    else
                    {
                        printf("Only one date per command is supported\n");
                    }
                }
                else
                {
                    printf("Date supported only for start/stop/add/sub\n");
                }
            } break;

            case Token_Time:
            {
                if (instruction == Ins_Time_Entry)
                {
                    if (!entry.time)
                    {
                        auto time = parse_time(token);
                        entry.time = time;
                    }
                    else
                    {
                        printf("Only one time per command is supported\n");
                    }
                }
                else
                {
                    printf("Time supported only for start/stop/add/sub\n");
                }
            } break;

            case Token_String:
            {
                if (instruction == Ins_Time_Entry)
                {
                    if (!entry.description)
                    {
                        entry.description = create_description(&state->description_arena, token);
                    }
                    else
                    {
                        printf("Only one description per command is supported\n");
                    }
                }
                else
                {
                    printf("Description supported only for start/stop/add/sub\n");
                }
            } break;

            case Token_Unknown:
            default:
            {
            } break;

            case Token_End_Of_Stream: 
            {
                parsing = false;

                if (reading_from_file)
                {
                    if (instruction != Ins_None)
                    {
                        printf("[Load Error #%d] Missing semicolon at the end of the file\n",
                               state->load_error_count++);
                    }

                    break;
                }
                else
                {
                    // NOTE(mateusz): Don't break.
                    //                Assume additional semicolon at the end of the console input.
                }
            } 

            case Token_Semicolon:
            {
                if (instruction == Ins_Time_Entry)
                {
                    process_time_entry(state, &entry, reading_from_file);   
                }
                else if (instruction == Ins_Show)
                {
                    print_all_days(state);
                }
                else if (instruction == Ins_Exit)
                {
                    if (!(flag & Flag_No_Save)) 
                    {
                        if (state->change_count > 0)
                        {
                            save_to_file(state);
                        }
                    }

                    Assert(program_is_running);
                    *program_is_running = false;
                }
                else if (instruction == Ins_Save)
                {                    
                    save_to_file(state);
                    printf("File saved\n");
                }
                else if (instruction == Ins_Archive)
                {
                    archive_current_file(state, true);
                }
                else if (instruction == Ins_Time)
                {
                    time_t now = get_current_time(state);
                    char now_str[MAX_TIMESTAMP_STRING_SIZE];
                    get_timestamp_string(now_str, sizeof(now_str), now);
                    printf("Current time: %s\n", now_str);
                }



                instruction = Ins_None;
                flag = 0;
                entry = {};
            } break;
        }
    }
}

internal void load_file(Program_State *state)
{
    char *filename = state->input_filename;
    state->load_error_count = 0;

    char *file_content = read_entire_file_and_null_terminate(filename);
    if (file_content)
    {
        printf("File loaded: %s\n", filename);

        process_input(file_content, state, true);

        free(file_content);
        state->loaded_input_mod_time = platform_get_file_mod_time(state->input_filename);
    }
    else
    {
        printf("[Load Error #%d] Failed to open the file: %s", 
               state->load_error_count++, filename);
    }
}


internal void read_from_keyboard(Thread_Memory *thread_memory)
{
    for (;;)
    {
        if (thread_memory->new_data)
        {
            // spinlock while new_data == true
        }
        else
        {
            printf(thread_memory->cursor);
            fgets(thread_memory->input_buffer, sizeof(thread_memory->input_buffer), stdin);
            thread_memory->new_data = true;
        }
    }
}

int main(int arg_count, char **args)
{
    Program_State *state = (Program_State *)malloc(sizeof(Program_State));
    if (state == NULL)
    {
        printf("Failed to allocate required memory!\n");
        return 1;
    }

    //
    // NOTE(mateusz): Initialization 
    //

    memset(state, 0, sizeof(Program_State));

    initialize_arena(&state->description_arena, sizeof(state->byte_memory_block), 
                     state->byte_memory_block);

    initialize_arena(&state->struct_arena, sizeof(state->aligned_memory_block), 
                     state->aligned_memory_block);

    initialize_timezone_offset(state);

    // TODO(mateusz): Get these filenames/paths from input arguments.
    sprintf(state->input_filename, "database.txt");

    sprintf(state->archive_directory, "archive");
    add_ending_slash_to_path(state->archive_directory);
    platform_create_directory(state->archive_directory);


    load_file(state);

    if (state->load_error_count == 0)
    {
        print_all_days(state);
        save_to_file(state);
    }
    else
    {
        state->change_count = 0;
    }

    // TODO(mateusz): Factor out windows code away.
    Thread_Memory thread_memory = {};
    sprintf(thread_memory.cursor, "::>");
    platform_create_thread((LPTHREAD_START_ROUTINE)read_from_keyboard, &thread_memory);


    //
    // NOTE(mateusz): Main loop
    //

    b32 is_running = true;
    while (is_running)
    {
        if (thread_memory.new_data)
        {
            process_input(thread_memory.input_buffer, state, false, &is_running);
            if (state->change_count > 0)
            {
                save_to_file(state);
            }

            thread_memory.new_data = false;
        }

        auto current_input_mod_time = platform_get_file_mod_time(state->input_filename);
        b32 source_file_changed =  (platform_compare_file_time(
            state->loaded_input_mod_time, current_input_mod_time) != 0);

        if (source_file_changed)
        {
            load_file(state);
        }
        Sleep(33);
    }

    return 0;
}