internal void get_timestamp_string(char *output, u32 output_size, tm *s)
{
    snprintf(output, output_size, "%04d-%02d-%02d %02d:%02d", 
             s->tm_year + 1900, s->tm_mon, s->tm_mday,
             s->tm_hour, s->tm_min);
}

#define MAX_TIME_STRING_SIZE sizeof("00:00")
internal void get_time_string(char *output, u32 output_size, time_t time)
{
    Assert(output_size >= MAX_TIME_STRING_SIZE);
    time = abs(time);

    s32 hours_in_day = 24;
    s32 hours = (s32)((time / Hours(1)) % hours_in_day);

    s32 minutes_in_hour = 60;
    s32 minutes = (s32)((time / Minutes(1)) % minutes_in_hour);
    snprintf(output, output_size, "%02d:%02d", hours, minutes);
}

internal void get_date_string(char *output, u32 output_size, tm *s)
{
    snprintf(output, output_size, "%04d-%02d-%02d", 
             s->tm_year + 1900, s->tm_mon, s->tm_mday);
}

internal void print_offset(s32 offset_sum)
{
    const u32 size = MAX_TIME_STRING_SIZE + 2;
    char offset_time_str[size];

    if (offset_sum > Minutes(1))        offset_time_str[0] = '+';
    else if (offset_sum < -Minutes(1))  offset_time_str[0] = '-';
    else                                offset_time_str[0] = 0;

    if (offset_time_str[0])
    {
        get_time_string(offset_time_str + 1, size - 2, offset_sum);
        offset_time_str[size - 2] = '\t';
        offset_time_str[size - 1] = 0;
    }

    printf("%s", offset_time_str);
}

internal void print_work_time_row(time_t start, time_t end, s32 offset_sum, 
                                  char *start_desc, char *end_desc, 
                                  char *replace_end = NULL)
{
    char start_time_str[MAX_TIME_STRING_SIZE];
    get_time_string(start_time_str, sizeof(start_time_str), start);

    char end_time_str[MAX_TIME_STRING_SIZE];
    get_time_string(end_time_str, sizeof(end_time_str), end);

    printf("%s -> %s\t", start_time_str, 
           replace_end ? replace_end : end_time_str);

    print_offset(offset_sum);

    if (start_desc) printf("\"%s\" ", start_desc);
    if (end_desc)   printf("\"%s\" ", end_desc);
    
    printf("\n");
}


#define MAX_PROGRESS_BAR_SIZE sizeof("[++++ ++++ ++++ +-  )!")
internal void get_progress_bar_string(char *output, s32 output_size, time_t time, Missing_Type type)
{
    // NOTE(mateusz): Max output:
    Assert(output_size >= MAX_PROGRESS_BAR_SIZE);
    if (time > Days(1)) time = Days(1);

    s32 hours = (s32)(time / Hours(1) % (Days(1) / Hours(1)));
    s32 minutes = (s32)(time / Minutes(1)) % (Hours(1) / Minutes(1));

    s32 i = 0;
    s32 symbol_counter = 0;
    output[i++] = '[';
    while (hours + 1 > symbol_counter)
    {
        if (symbol_counter > 0 && 
            ((symbol_counter) % 4 == 0))
        {
            output[i++] = ' ';
        }

        if (hours == symbol_counter)
        {
            if (minutes > 0) output[i++] = '-';
            else break;
        }
        else
        {
            output[i++] = '+';
        }

        ++symbol_counter;
    }

    for (int space_index = 0;
         space_index < (4 - (symbol_counter % 4)) % 4;
         ++space_index)
    {
        output[i++] = ' ';
    }

    if (type == Missing_None)          output[i++] = ']';
    if (type == Missing_Assumed)       output[i++] = ')';
    else if (type == Missing_Critical) { output[i++] = ')'; output[i++] = '!'; }

    output[i] = 0;
    Assert(i < output_size);
}
