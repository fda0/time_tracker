

inline time_t 
truncate_to_date(time_t timestamp)
{
    time_t result = (timestamp / Days(1)) * Days(1);
    return result;
}

inline time_t 
truncate_to_time(time_t timestamp)
{
    time_t result = timestamp % Days(1);
    return result;
}


internal time_t 
get_current_timestamp(Program_State *state)
{
    time_t now;
    time(&now);
    
    time_t result = now + state->timezone_offset;
    return result;
}

internal time_t
get_time(Program_State *state)
{
    time_t now = get_current_timestamp(state);
    time_t time = truncate_to_time(now);
    return time;
}

internal time_t 
get_today(Program_State *state)
{
    time_t now = get_current_timestamp(state);
    time_t today = truncate_to_date(now);
    return today;
}


internal void 
initialize_timezone_offset(Program_State *state)
{
    time_t test_time;
    time(&test_time);
    
    tm *local_date = localtime(&test_time);
    time_t utc_test_time = platform_tm_to_time(local_date);
    
    time_t offset = utc_test_time - test_time;
    state->timezone_offset = offset;
}


struct Boundries_Result
{
    time_t day_count;
    
    time_t begin;
    time_t one_day_past_end;
};

internal Boundries_Result
get_month_boundries(time_t timestamp)
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
    
    result.day_count = ((result.one_day_past_end / Days(1)) - 
                        (result.begin / Days(1)));
    
    return result;
}
