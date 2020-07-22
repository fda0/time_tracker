/*
    TODO(mateusz):
    * Add nice error messages and don't crash the program if not needed

    * Allow for start without date (in the same day).
    * Delete asserts where possible - add user facing message errors instead.

*/

// NOTE(mateusz): This program ignores concept of timezones to simplify usage.

#include "tt_main.h"

#include "tt_token.cpp"
#include "tt_string.cpp"



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

    time_t timestamp = tm_to_time(&date);
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

inline time_t truncate_to_date_timestamp(time_t timestamp)
{
    time_t result = (timestamp / Days(1)) * Days(1);
    return result;
}


internal void calculate_work_time(Day *day, s32 *error_count)
{
    // NOTE(mateusz): If error_count == NULL function will print to console.
    b32 print = error_count == NULL;

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
            Assert(!start);
            start = entry;
        }
        else if (entry->type == Entry_Stop)
        {
            Assert(start);
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

            Assert(stop_time > start_time);

            day->sum += (s32)(stop_time - start_time);

            if (print) print_work_time_row(start, entry, offset_sum);

            start = NULL;
            offset_sum = 0;
        }
    }


    if (start) // NOTE(mateusz): Day ends with start - missing end entry.
    {
        time_t now;
        time(&now);
        
        time_t start_time = start->date + start->time;

        if (now > start_time &&
            now - Days(1) < start_time)
        {
            day->missing = Missing_Assumed;
            day->sum += (s32)(now - start_time);

            if (print) print_work_time_row(start, NULL, offset_sum, "now");
        }
        else
        {
            day->missing = Missing_Critical;

            if (print) 
            {
                print_work_time_row(start, 0, offset_sum, "...");
            }
            else
            {
                char timestamp_str[MAX_TIMESTAMP_STRING_SIZE];
                get_timestamp_string(timestamp_str, sizeof(timestamp_str), start_time);
                printf("[Error #%d] MISSING stop time: %s\n", *error_count++, timestamp_str);
            }
        }
    }
}

internal void print_all_days(Day *days, u32 day_count)
{
    for (u32 day_index = 0;
         day_index < day_count;
         ++day_index)
    {
        Day *day = &days[day_index];

        tm *date = gmtime(&day->date_stamp);
        char date_str[64];
        get_date_string(date_str, sizeof(date_str), date);

        char sum_str[64];
        get_time_string(sum_str, sizeof(sum_str), day->sum);

        char bar_str[MAX_PROGRESS_BAR_SIZE];
        get_progress_bar_string(bar_str, sizeof(bar_str), day->sum, day->missing);
        printf("\n%s\n", date_str);

        calculate_work_time(day, NULL);

        printf("sum: %s\t%s\n", sum_str, bar_str);
    }
}

internal void archive_current_file(Program_State *state)
{
    time_t now;
    time(&now);
    tm *date = gmtime(&now);

    char timestamp[MAX_PATH];
    get_timestamp_string_for_file(timestamp, sizeof(timestamp), date);

    char archive_filename[MAX_PATH];
    snprintf(archive_filename, sizeof(archive_filename), "%s%s_%s.txt", 
             state->archive_directory, state->input_filename, timestamp);

    copy_file(state->input_filename, archive_filename);
    printf("File archived to: %s\n", archive_filename);
}

internal void save_to_file(Program_State *state)
{
    FILE *file = fopen(state->input_filename, "w");
    if (file)
    {
        for (u32 day_index = 0;
             day_index < state->day_count;
             ++day_index)
        {
            Day *day = &state->days[day_index];

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
                    if (entry->date == day->date_stamp)
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
                    char *tag = NULL;
                    if (entry->type == Entry_Add)           tag = "add";
                    else if (entry->type == Entry_Subtract) tag = "sub";
                    else
                    {
                        Invalid_Code_Path;
                        continue;
                    }

                    char time_string[MAX_TIME_STRING_SIZE];
                    get_time_string(time_string, sizeof(time_string), entry->time);
                    fprintf(file, "%s %s", tag, time_string);
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

            fprintf(file, "\n");
        }
    
        printf("File saved\n");
        fclose(file);
    }
    else
    {
        printf("Failed to write to file: %s\n", state->input_filename);
    }
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

            #define Not_Supported_In_File_Continue \
            { \
                printf("[Error #%d] %.*s keyword is not supported in files\n", \
                       state->error_count++, (s32)token.text_length, token.text); continue; \
            }


            case Token_Identifier:
            {
                if (token_equals(token, "start")) 
                {
                    Assert(instruction == Ins_None);
                    Assert(!entry.type);

                    entry.type = Entry_Start;
                    instruction = Ins_Time_Entry;
                }
                else if (token_equals(token, "stop"))
                {
                    Assert(instruction == Ins_None);
                    Assert(!entry.type);
                    
                    entry.type = Entry_Stop;
                    instruction = Ins_Time_Entry;
                }
                else if (token_equals(token, "add"))
                {
                    Assert(instruction == Ins_None);
                    Assert(!entry.type);
                    
                    entry.type = Entry_Add;
                    instruction = Ins_Time_Entry;
                }
                else if (token_equals(token, "subtract") ||
                         token_equals(token, "sub"))
                {
                    Assert(instruction == Ins_None);
                    Assert(!entry.type);
                    
                    entry.type = Entry_Subtract;
                    instruction = Ins_Time_Entry;
                }
                else if (token_equals(token, "show"))
                {
                    Assert(instruction == Ins_None);
                    if (reading_from_file) Not_Supported_In_File_Continue;


                    instruction = Ins_Show;
                }
                else if (token_equals(token, "exit"))
                {
                    Assert(instruction == Ins_None);
                    if (reading_from_file) Not_Supported_In_File_Continue;
                    

                    instruction = Ins_Exit;
                }
                else if (token_equals(token, "save"))
                {
                    Assert(instruction == Ins_None);
                    if (reading_from_file) Not_Supported_In_File_Continue;
                    

                    instruction = Ins_Save;
                }
                else if (token_equals(token, "archive"))
                {
                    Assert(instruction == Ins_None);
                    if (reading_from_file) Not_Supported_In_File_Continue;
                    

                    instruction = Ins_Archive;
                }
                else if (token_equals(token, "no-save"))
                {
                    Assert(instruction == Ins_Exit);
                    if (reading_from_file) Not_Supported_In_File_Continue;
                    

                    flag |= Flag_No_Save;
                }
                else
                {
                    printf("Unknown identifier: %.*s\n", (s32)token.text_length, token.text);
                }
            } break;

            case Token_Date:
            {
                Assert(!entry.date);
                auto date = parse_date(token);
                entry.date = date;
            } break;

            case Token_Time:
            {
                Assert(!entry.time);
                auto time = parse_time(token);
                entry.time = time;
            } break;

            case Token_String:
            {
                Assert(!entry.description);
                entry.description = create_description(&state->description_arena, token);
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
                        printf("Missing semicolon at the end of the file\n");
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
                    Day *day = NULL;

                    // TODO(mateusz): Good error reporting for user with line count.
                    if (entry.type == Entry_Start)
                    {
                        if ((state->day_count == 0) ||
                            (state->days[state->day_count - 1].date_stamp < entry.date))
                        {
                            if (state->day_count != 0)
                            {
                                calculate_work_time(&state->days[state->day_count - 1], 
                                                    &state->error_count);
                            }

                            Assert(state->day_count < Array_Count(state->days));
                            day = &state->days[state->day_count++];
                            day->date_stamp = entry.date;
                        }
                        else
                        {
                            Assert(state->days[state->day_count - 1].date_stamp == entry.date);
                            day = &state->days[state->day_count - 1];
                        }
                    }
                    else
                    {
                        Assert(state->day_count != 0);
                        day = &state->days[state->day_count - 1];
                    }

                    Time_Entry *entry_dest = &day->first_time_entry;
                    while (entry_dest->type != Entry_None)
                    {
                        if (entry_dest->next_in_day == NULL)
                        {
                            entry_dest->next_in_day = Push_Struct(&state->struct_arena, Time_Entry);
                        }

                        entry_dest = entry_dest->next_in_day;
                    }

                    *entry_dest = entry;
                }
                else if (instruction == Ins_Show)
                {
                    print_all_days(state->days, state->day_count);
                }
                else if (instruction == Ins_Exit)
                {
                    if (!(flag & Flag_No_Save)) 
                    {
                        archive_current_file(state);
                        save_to_file(state);
                    }

                    Assert(program_is_running);
                    *program_is_running = false;
                }
                else if (instruction == Ins_Save)
                {
                    archive_current_file(state);
                    save_to_file(state);
                }
                else if (instruction == Ins_Archive)
                {
                    archive_current_file(state);
                }



                instruction = Ins_None;
                flag = 0;
                entry = {};
            } break;
        }
    }

    if (state->day_count > 0)
    {
        auto last_day = &state->days[state->day_count - 1];
        if (last_day->sum == 0)
        {
            calculate_work_time(last_day, &state->error_count);
        }
    }
}

internal void load_file(char *filename, Program_State *state)
{
    char *file_content = read_entire_file_and_null_terminate(filename);
    if (file_content)
    {
        printf("File read: %s\n", filename);

        process_input(file_content, state, true);

        printf("File processed: %s\n", filename);
        free(file_content);
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
    else
    {
        memset(state, 0, sizeof(Program_State));

        initialize_arena(&state->description_arena, sizeof(state->byte_memory_block), 
                         state->byte_memory_block);

        initialize_arena(&state->struct_arena, sizeof(state->aligned_memory_block), 
                         state->aligned_memory_block);
    }

    // TODO(mateusz): Get these filenames/paths from input arguments.
    sprintf(state->input_filename, "database.txt");

    sprintf(state->archive_directory, "archive");
    add_ending_slash_to_path(state->archive_directory);
    create_directory(state->archive_directory);

    load_file(state->input_filename, state);
    if (state->error_count == 0)
    {
        print_all_days(state->days, state->day_count);
    }


    b32 is_running = true;
    while (is_running)
    {
        char input_buffer[256];
        printf(">");
        fgets(input_buffer, sizeof(input_buffer), stdin);
        process_input(input_buffer, state, false, &is_running);
    }

    return 0;
}