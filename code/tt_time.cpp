

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
get_current_time(Program_State *state)
{
    time_t now;
    time(&now);
    
    time_t result = now + state->timezone_offset;
    return result;
}

internal time_t 
get_today(Program_State *state)
{
    time_t now = get_current_time(state);
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
