struct Str32
{
    char str[32];
};


struct Str128
{
    char str[128];
};

struct StrMaxPath
{
    char str[MAX_PATH];
};


internal Str128
get_timestamp_string(date64 time)
{
    tm *date = gmtime(&time);
    
    Str128 result;
    snprintf(result.str, sizeof(result.str), "%04d-%02d-%02d %02d:%02d", date->tm_year + 1900, date->tm_mon + 1,
             date->tm_mday, date->tm_hour, date->tm_min);
    
    return result;
}

internal Str128
get_timestamp_string_for_file(date64 time, b32 long_format)
{
    tm *date = gmtime(&time);
    
    Str128 result;
    
    if (long_format)
    {
        snprintf(result.str, sizeof(result.str), "%04d-%02d-%02d_%02d-%02d-%02d", date->tm_year + 1900,
                 date->tm_mon + 1, date->tm_mday, date->tm_hour, date->tm_min, date->tm_sec);
    }
    else
    {
        snprintf(result.str, sizeof(result.str), "%04d-%02d-%02d_%02d-%02d", date->tm_year + 1900, date->tm_mon + 1,
                 date->tm_mday, date->tm_hour, date->tm_min);
    }
    
    return result;
}



internal Str32
get_time_string(time32 time)
{
    time = abs(time);
    
    s32 hours = (s32)((time / Hours(1)));
    
    s32 minutes_in_hour = 60;
    s32 minutes = (s32)((time / Minutes(1)) % minutes_in_hour);
    
    Str32 result;
    snprintf(result.str, sizeof(result.str), "%02d:%02d", hours, minutes);
    return result;
}


internal Str32
get_date_string(date64 timestamp)
{
    tm *date = gmtime(&timestamp);
    
    Str32 result;
    snprintf(result.str, sizeof(result.str), "%04d-%02d-%02d", date->tm_year + 1900, date->tm_mon + 1, date->tm_mday);
    return result;
}



internal Str128
get_progress_bar_string(time32 time, Missing_Ending missing_ending)
{
    Str128 result;
    char *output = result.str;
    s32 output_size = sizeof(result.str);
    
    if (time > Days(1))
        time = Days(1);
    
    s32 hours = (s32)(time / Hours(1) % (Days(1) / Hours(1)));
    s32 minutes = (s32)(time / Minutes(1)) % (Hours(1) / Minutes(1));
    
    b32 print_open_bracket = true;
    b32 print_minutes_bit = (minutes > 0);
    
    s32 signs_to_print = hours + (print_minutes_bit ? 1 : 0);
    s32 signs_printed = 0;
    b32 printed_separator_recently = true;
    s32 print_align_spaces = (4 - (signs_to_print % 4)) % 4;
    
    char *close_bracket_string;
    if (missing_ending == MissingEnding_None)
        close_bracket_string = "]";
    else if (missing_ending == MissingEnding_Assumed)
        close_bracket_string = ")";
    else
        close_bracket_string = "> stop is missing!";
    
    
    for (s32 index = 0; index < output_size; ++index)
    {
        if (print_open_bracket)
        {
            output[index] = '[';
            print_open_bracket = false;
        }
        else if (signs_printed < signs_to_print)
        {
            if (!printed_separator_recently && (signs_printed % 4 == 0))
            {
                output[index] = ' ';
                printed_separator_recently = true;
            }
            else
            {
                char sign;
                if (print_minutes_bit && (signs_printed == (signs_to_print - 1)))
                    sign = '-';
                else
                    sign = '+';
                
                
                output[index] = sign;
                ++signs_printed;
                printed_separator_recently = false;
            }
        }
        else if (print_align_spaces > 0)
        {
            output[index] = ' ';
            --print_align_spaces;
        }
        else if (*close_bracket_string)
        {
            output[index] = *close_bracket_string;
            ++close_bracket_string;
        }
        else
        {
            output[index] = 0;
            break;
        }
    }
    
    output[output_size - 1] = 0;
    
    return result;
}



internal Str128
get_sum_and_progress_bar_string(time32 sum, Missing_Ending missing_ending)
{
    Str32 time = get_time_string(sum);
    
    Str128 bar = get_progress_bar_string(sum, missing_ending);
    
    char *minus = (sum < 0) ? "-" : "";
    Str128 result;
    snprintf(result.str, sizeof(result.str), "sum: %s%s       %s", minus, time.str, bar.str);
    return result;
}


internal Str32
get_day_of_the_week_string(date64 timestamp)
{
    tm *date = gmtime(&timestamp);
    
    char *day_str = NULL;
    switch (date->tm_wday)
    {
        case 0: {
            day_str = "Sunday";
            break;
        }
        case 1: {
            day_str = "Monday";
            break;
        }
        case 2: {
            day_str = "Tuesday";
            break;
        }
        case 3: {
            day_str = "Wednesday";
            break;
        }
        case 4: {
            day_str = "Thursday";
            break;
        }
        case 5: {
            day_str = "Friday";
            break;
        }
        case 6: {
            day_str = "Saturday";
            break;
        }
        default: {
            day_str = "???";
            break;
        }
    }
    
    Str32 result;
    snprintf(result.str, sizeof(result.str), day_str);
    
    return result;
}



//~ NOTE: Print functions

internal void
print_help_item(char *command, char *args, char *help)
{
    using namespace Color;
    printf("%s%s\t", f_sum, command);
    if (args)
    {
        printf("%s%s\t", f_desc, args);
    }
    printf("%s%s\n", f_reset, help);
}

internal void
print_help_header(char *text)
{
    using namespace Color;
    printf("%s%s%s%s%s\n", b_help_header, f_white, text, b_reset, f_reset);
}


internal void
print_help_desc(char *text)
{
    using namespace Color;
    printf("%s%s%s\n", f_dimmed, text, f_reset);
}
