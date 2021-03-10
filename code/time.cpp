



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


struct Boundries_Result
{
    date64 day_count;
    
    date64 begin;
    date64 one_day_past_end;
};

internal Boundries_Result
get_month_boundries(date64 timestamp)
{
    Boundries_Result result = {};
    
    tm *date = gmtime(&timestamp);
    date->tm_sec = 0;
    date->tm_min = 0;
    date->tm_hour = 0;
    date->tm_mday = 1;
    
    result.begin = platform_tm_to_time(date);
    
    date->tm_mon += 1;
    result.one_day_past_end = platform_tm_to_time(date);
    
    result.day_count = ((result.one_day_past_end / Days(1)) - (result.begin / Days(1)));
    
    return result;
}
