/*
    TODO(mateusz):
    * Add in program interface for adding/undo/showing data
    * Add save feature

    * Add timezones support + (maybe) ensure timezone safeness? Or get rid of timezone concept.
    * Add nice error messages and don't crash the program if not needed
*/


#include "tt_main.h"

#include "tt_token.cpp"
#include "tt_memory.h"

#define Days(Value) (Hours(Value) * 24)
#define Hours(Value) (Minutes(Value) * 60)
#define Minutes(Value) (Value * 60)

enum Entry_Type
{
    Entry_None,

    Entry_Start,
    Entry_End,
    Entry_Subtract,
    Entry_Add,
};

struct Time_Entry
{
    Entry_Type type;
    time_t date_stamp;
    time_t time;
    char *description;
    Time_Entry *next_in_day;
};

struct Day
{
    time_t date_stamp;
    s32 sum;
    Time_Entry first_time_entry;
};


// TODO(mateusz): (Maybe) More dynamic memory - eg. for days?
struct Data_State
{
    Day days[365];
    u32 day_count;

    Memory_Arena description_arena;
    Memory_Arena struct_arena;

    // memory
    u8 byte_memory_block[Megabytes(8)];
    u8 aligned_memory_block[Megabytes(8)];
};

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

    time_t timestamp = mktime(&date);
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

internal void get_timestamp_string(tm *s, char *output, u32 output_size)
{
    snprintf(output, output_size, "%04d-%02d-%02d %02d:%02d", 
             s->tm_year + 1900, s->tm_mon, s->tm_mday,
             s->tm_hour, s->tm_min);
}

internal void get_time_string(time_t time, char *output, u32 output_size)
{
    snprintf(output, output_size, "%02d:%02d", 
             (s32)(time / Hours(1) % (Days(1) / Hours(1))), 
             (s32)(time / Minutes(1)) % (Hours(1) / Minutes(1)));
}

internal void get_date_string(tm *s, char *output, u32 output_size)
{
    snprintf(output, output_size, "%04d-%02d-%02d", 
             s->tm_year + 1900, s->tm_mon, s->tm_mday);
}

internal void calculate_work_time(Day *day)
{
    time_t start_timestamp = 0;
    time_t end_timestamp = 0;
    s32 sum = 0;
    for (Time_Entry *entry = &day->first_time_entry;
         entry;
         entry = entry->next_in_day)
    {
        if (entry->type == Entry_Add)
        {
            sum += (s32)entry->time;
        }
        else if (entry->type == Entry_Subtract)
        {
            sum -= (s32)entry->time;
        }
        else if (entry->type == Entry_Start)
        {
            Assert(start_timestamp == 0 && end_timestamp == 0);
            start_timestamp = entry->date_stamp + entry->time;
        }
        else if (entry->type == Entry_End)
        {
            Assert(start_timestamp);
            end_timestamp = entry->date_stamp + entry->time;

            Assert(end_timestamp > start_timestamp);
            sum += (s32)(end_timestamp - start_timestamp);
            start_timestamp = 0;
            end_timestamp = 0;
        }
    }

    if (start_timestamp && !end_timestamp)
    {
        time_t now;
        time(&now);
        
        if (now > start_timestamp &&
            now - Days(1) < start_timestamp)
        {
            sum += (s32)(now - start_timestamp);
            printf("End time assumed as now\n");
        }
        else
        {
            tm *date = localtime(&start_timestamp);
            char timestamp_str[64];
            get_timestamp_string(date, timestamp_str, sizeof(timestamp_str));
            printf("MISSING end time for start: %s\n", timestamp_str);
        }


    }

    day->sum = sum;
}

internal void load_file(char *filename, Data_State *state)
{
    char *file_content = read_entire_file_and_null_terminate(filename);
    if (file_content)
    {
        printf("File read: %s\n", filename);

        Tokenizer tokenizer = {};
        tokenizer.at = file_content;

        Time_Entry entry = {};
        bool parsing = true;
        while (parsing)
        {
            // TODO(mateusz): User facing message errors instead of Asserts and crashing.
            Token token = get_token(&tokenizer);
            switch (token.type)
            {
                case Token_Identifier:
                {
                    Assert(!entry.type); // NOTE(mateusz): For exclusive options (start, end, add etc.)

                    if (token_equals(token, "start")) 
                    {
                        entry.type = Entry_Start;
                    }
                    else if (token_equals(token, "end"))
                    {
                        entry.type = Entry_End;
                    }
                    else if (token_equals(token, "add"))
                    {
                        entry.type = Entry_Add;
                    }
                    else if (token_equals(token, "subtract"))
                    {
                        entry.type = Entry_Subtract;
                    }
                } break;

                case Token_Date:
                {
                    Assert(!entry.date_stamp);
                    auto date_stamp = parse_date(token);
                    entry.date_stamp = date_stamp;
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

                case Token_Semicolon:
                {
                    if (entry.type)
                    {
                        Day *day = NULL;

                        // TODO(mateusz): Good error reporting for user with line count.
                        if (entry.type == Entry_Start)
                        {
                            if ((state->day_count == 0) ||
                                (state->days[state->day_count - 1].date_stamp < entry.date_stamp))
                            {
                                if (state->day_count != 0)
                                {
                                    calculate_work_time(&state->days[state->day_count - 1]);
                                }

                                Assert(state->day_count < Array_Count(state->days));
                                day = &state->days[state->day_count++];
                                day->date_stamp = entry.date_stamp;
                            }
                            else
                            {
                                Assert(state->days[state->day_count - 1].date_stamp == entry.date_stamp);
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
                        entry = {};
                    }
                    else
                    {
                        Invalid_Code_Path;
                    }
                } break;

                case Token_End_Of_Stream: 
                {
                    parsing = false;
                } break;
            }
        }

        if (state->day_count > 0)
        {
            auto last_day = &state->days[state->day_count - 1];
            if (last_day->sum == 0)
            {
                calculate_work_time(last_day);
            }
        }

        printf("File processed: %s\n", filename);
        free(file_content);
    }
}

// internal void save_to_file(char *path)
// {
//     FILE *file = fopen(path, "w");
//     if (file)
//     {

//         // fputs();
//     }
//     else
//     {
//         printf("Failed to write to file: %s\n", path);
//     }
// }

internal void print_all_days(Day *days, u32 day_count)
{
    for (u32 day_index = 0;
         day_index < day_count;
         ++day_index)
    {
        Day *day = &days[day_index];

        tm *date = localtime(&day->date_stamp);
        char date_str[64];
        get_date_string(date, date_str, sizeof(date_str));

        char time_str[64];
        get_time_string(day->sum, time_str, sizeof(time_str));

        printf("%s\t time: %s\n", date_str, time_str);
    }
}

int main(int arg_count, char **args)
{
    Data_State *state = (Data_State *)malloc(sizeof(Data_State));
    if (state == NULL)
    {
        printf("Failed to allocate memory for state");
        return 1;
    }
    else
    {
        memset(state, 0, sizeof(Data_State));

        // TODO(mateusz): Research if it is needed...
        // Are there problems with accessing not byte aligned structs?
        initialize_arena(&state->description_arena, sizeof(state->byte_memory_block), 
                         state->byte_memory_block);

        initialize_arena(&state->struct_arena, sizeof(state->aligned_memory_block), 
                         state->aligned_memory_block);
    }

    char database_filename[] = "database.txt";

    load_file(database_filename, state);
    print_all_days(state->days, state->day_count);


    // b32 is_running = true;
    // while (is_running)
    // {
    //     char input_buffer[256];
    //     printf("Input: \n");
    //     fgets(input_buffer, sizeof(input_buffer), stdin);
    //     auto command = parse_command(input_buffer);

    //     switch (command)
    //     {
    //         default: 
    //         {
    //             printf("?\n");
    //         } break;

    //         case Command_Exit:
    //         {
    //             is_running = false;
    //         } break;

    //         case Command_Save: 
    //         {
    //             save_to_file(database_filename);
    //         } break;
    //     }
    // }

    return 0;
}