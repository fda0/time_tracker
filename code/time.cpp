



inline date64
truncate_to_date(date64 timestamp)
{
    date64 result = (timestamp / Days(1)) * Days(1);
    return result;
}

inline s32
truncate_to_time(date64 timestamp)
{
    s32 result = timestamp % Days(1);
    return result;
}


internal date64
get_current_timestamp()
{
    date64 now;
    time(&now);
    
    date64 result = now + global_state.timezone_offset;
    return result;
}

internal s32
get_time()
{
    date64 now = get_current_timestamp();
    s32 time = truncate_to_time(now);
    return time;
}

internal date64
get_today()
{
    date64 now = get_current_timestamp();
    date64 today = truncate_to_date(now);
    return today;
}


internal void
initialize_timezone_offset()
{
    date64 test_time;
    time(&test_time);
    
    tm *local_date = localtime(&test_time);
    date64 utc_test_time = platform_tm_to_time(local_date);
    
    date64 offset = utc_test_time - test_time;
    global_state.timezone_offset = offset;
}


struct Boundaries_Result
{
    date64 first;
    date64 last;

    char *description;
};



internal Boundaries_Result
get_month_boundaries(date64 timestamp)
{
    Boundaries_Result result = {};
    
    tm *date = gmtime(&timestamp);
    date->tm_sec = 0;
    date->tm_min = 0;
    date->tm_hour = 0;
    date->tm_mday = 1;
    
    result.first = platform_tm_to_time(date);
    
    date->tm_mon += 1;
    result.last = platform_tm_to_time(date) - Days(1);
    
    return result;
}


internal Boundaries_Result
get_quarter_boundaries(date64 timestamp)
{
    Boundaries_Result result = {};
    
    s32 months_in_quarter = 3;
    
    tm *date = gmtime(&timestamp);
    date->tm_sec = 0;
    date->tm_min = 0;
    date->tm_hour = 0;
    date->tm_mday = 1;
    
    s32 quarter = (date->tm_mon/months_in_quarter);
    date->tm_mon =  quarter*months_in_quarter;
    
    if (quarter == 0) {
        result.description = "Q1";
    } else if (quarter == 1) {
        result.description = "Q2";
    } else if (quarter == 2) {
        result.description = "Q3";
    } else if (quarter == 3) {
        result.description = "Q4";
    }
    
    result.first = platform_tm_to_time(date);
    
    date->tm_mon += months_in_quarter;
    result.last = platform_tm_to_time(date) - Days(1);
    
    return result;
}


internal Boundaries_Result
get_year_boundaries(date64 timestamp)
{
    Boundaries_Result result = {};
    
    tm *date = gmtime(&timestamp);
    date->tm_sec = 0;
    date->tm_min = 0;
    date->tm_hour = 0;
    date->tm_mday = 1;
    date->tm_mon = 0;
    
    result.first = platform_tm_to_time(date);
    
    date->tm_year += 1;
    result.last = platform_tm_to_time(date) - Days(1);
    
    return result;
}

