
internal String
get_timestamp_string(Arena *arena, date64 time)
{
    tm *date = gmtime(&time);
    
    String result = stringf(arena, "%04d-%02d-%02d %02d:%02d", date->tm_year + 1900, date->tm_mon + 1,
                            date->tm_mday, date->tm_hour, date->tm_min);
    return result;
}

internal String
get_timestamp_string_for_file(Arena *arena, date64 time, b32 long_format)
{
    tm *date = gmtime(&time);
    
    String result;
    
    if (long_format)
    {
        result = stringf(arena, "%04d-%02d-%02d_%02d-%02d-%02d", date->tm_year + 1900,
                         date->tm_mon + 1, date->tm_mday, date->tm_hour, date->tm_min, date->tm_sec);
    }
    else
    {
        result = stringf(arena, "%04d-%02d-%02d_%02d-%02d", date->tm_year + 1900, date->tm_mon + 1,
                         date->tm_mday, date->tm_hour, date->tm_min);
    }
    
    return result;
}



internal String
get_time_string(Arena *arena, time32 time)
{
    time = abs(time);
    
    s32 hours = (s32)((time / Hours(1)));
    
    s32 minutes_in_hour = 60;
    s32 minutes = (s32)((time / Minutes(1)) % minutes_in_hour);
    
    String result = stringf(arena, "%02d:%02d", hours, minutes);
    return result;
}


internal String
get_date_string(Arena *arena, date64 timestamp)
{
    tm *date = gmtime(&timestamp);
    
    String result = stringf(arena, "%04d-%02d-%02d", date->tm_year + 1900, date->tm_mon + 1, date->tm_mday);
    return result;
}



internal String
get_progress_bar_string(Arena *arena, time32 time, Missing_Ending missing_ending)
{
    // TODO(f0): clean this function up
    String result = allocate_string(arena, 128); // TODO(f0): what is actual max size
    
    if (time > Days(1)) {
        time = Days(1);
    }
    
    s32 hours = (s32)(time / Hours(1) % (Days(1) / Hours(1)));
    s32 minutes = (s32)(time / Minutes(1)) % (Hours(1) / Minutes(1));
    
    b32 print_open_bracket = true;
    b32 print_minutes_bit = (minutes > 0);
    
    s32 signs_to_print = hours + (print_minutes_bit ? 1 : 0);
    s32 signs_printed = 0;
    b32 printed_separator_recently = true;
    s32 print_align_spaces = (4 - (signs_to_print % 4)) % 4;
    
    char *close_bracket_string;
    if (missing_ending == MissingEnding_None) {
        close_bracket_string = "]";
    } else if (missing_ending == MissingEnding_Assumed) {
        close_bracket_string = ")";
    } else {
        close_bracket_string = "> stop is missing!";
    }
    
    
    for_u32(index, result.size)
    {
        if (print_open_bracket)
        {
            result.str[index] = '[';
            print_open_bracket = false;
        }
        else if (signs_printed < signs_to_print)
        {
            if (!printed_separator_recently && (signs_printed % 4 == 0))
            {
                result.str[index] = ' ';
                printed_separator_recently = true;
            }
            else
            {
                char sign;
                if (print_minutes_bit && (signs_printed == (signs_to_print - 1)))
                    sign = '-';
                else
                    sign = '+';
                
                
                result.str[index] = sign;
                ++signs_printed;
                printed_separator_recently = false;
            }
        }
        else if (print_align_spaces > 0)
        {
            result.str[index] = ' ';
            --print_align_spaces;
        }
        else if (*close_bracket_string)
        {
            result.str[index] = *close_bracket_string;
            ++close_bracket_string;
        }
        else
        {
            result.size = index;
            break;
        }
    }
    
    
    return result;
}



internal String
get_sum_and_progress_bar_string(Arena *arena, time32 sum, Missing_Ending missing_ending)
{
    String time = get_time_string(arena, sum);
    String bar = get_progress_bar_string(arena, sum, missing_ending);
    
    char *minus = (sum < 0) ? "-" : "";
    
    String result = stringf(arena, "sum: %s%.*s       %.*s",
                            minus, string_expand(time), string_expand(bar));
    
    return result;
}


internal String
get_day_of_the_week_string(Arena *arena, date64 timestamp)
{
    tm *date = gmtime(&timestamp);
    
    char *day_str;
    switch (date->tm_wday)
    {
        case 0:  { day_str = "Sunday";    } break;
        case 1:  { day_str = "Monday";    } break;
        case 2:  { day_str = "Tuesday";   } break;
        case 3:  { day_str = "Wednesday"; } break;
        case 4:  { day_str = "Thursday";  } break;
        case 5:  { day_str = "Friday";    } break;
        case 6:  { day_str = "Saturday";  } break;
        default: { day_str = "???";       } break;
    }
    
    String result = stringf(arena, "%s", day_str);
    return result;
}

