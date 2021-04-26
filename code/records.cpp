struct Record_Index_Pair
{
    Record *record;
    u64 index;
};


internal Record_Range
get_day_range_for_record_index(Program_State *state, u64 start_index)
{
    Record_Range result = {};
    b32 start_is_active = false;
    u64 index = start_index;
    
    if (index < state->records.count)
    {
        date64 date_begin = state->records.at(index)->date;
        
        for (;
             index < state->records.count;
             ++index)
        {
            Record *record = state->records.at(index);
            if (date_begin <= record->date &&
                record->type != Record_TimeStop)
            {
                result.date = record->date;
                result.first = index;
                
                if (record->type == Record_TimeStart)
                {
                    start_is_active = true;
                }
                break;
            }
        }
        
        
        for (;
             index < state->records.count;
             ++index)
        {
            Record *record = state->records.at(index);
            
            if (result.date != record->date)
            {
                result.one_past_last = result.next_day_record_index = index;
                
                if (start_is_active) {
                    result.one_past_last += 1;
                }
                break;
            }
            
            if (record->type == Record_TimeStart)
            {
                start_is_active = true;
            }
            else if (record->type == Record_TimeStop)
            {
                start_is_active = false;
            }
        }
        
        if (!result.one_past_last) {
            result.one_past_last = result.next_day_record_index = state->records.count;
        }
    }
    
    return result;
}


internal Range_u64
get_index_range_for_date_range(Program_State *state, u64 starting_index,
                               date64 date_begin, date64 date_end)
{
    Range_u64 result = {};
    
    u64 index = starting_index;
    for (;
         index < state->records.count;
         ++index)
    {
        Record *record = state->records.at(index);
        if (date_begin <= record->date) {
            result.first = index;
            break;
        }
    }
    
    
    for (;
         index < state->records.count;
         ++index)
    {
        Record *record = state->records.at(index);
        if (date_end < record->date) {
            result.one_past_last = index;
            break;
        }
    }
    
    if (!result.one_past_last) {
        result.one_past_last = state->records.count;
    }
    
    return result;
}



inline Record_Index_Pair
get_record_and_index_for_starting_date(Program_State *state, date64 date_begin, u64 starting_index)
{
    Record_Index_Pair result = {};
    
    for (u64 record_index = starting_index;
         record_index < state->records.count;
         ++record_index)
    {
        Record *record_test = state->records.at(record_index);
        if (record_test->date >= date_begin)
        {
            result.record = record_test;
            result.index = record_index;
            break;
        }
    }
    
    return result;
}






internal Process_Days_Result
process_days_from_range(Program_State *state, u64 starting_index,
                        date64 date_begin, date64 date_end,
                        String filter,
                        u32 flags)
{
#if Def_Slow
    u32 open_start_ending_should_happen_only_once_test = 0;
#endif
    
    enum Open_State
    {
        Open,
        Closed_PrePrint,
        Closed_PostPrint,
    };
    
    Arena *arena = &state->arena;
    arena_scope(arena);
    
    b32 should_print = (flags & ProcessDays_Print) != 0;
    b32 alt_color = (flags & ProcessDays_AltColor) != 0;
    b32 has_filter = (filter.size != 0);
    
    Process_Days_Result result = {};
    
    
    
    Record_Index_Pair start_pair = get_record_and_index_for_starting_date(state, date_begin, starting_index);
    if (start_pair.record && start_pair.record->date <= date_end)
    {
        Record_Range range = get_day_range_for_record_index(state, start_pair.index);
        
        for (;
             range.date <= date_end && range.date != 0;
             range = get_day_range_for_record_index(state, range.next_day_record_index))
        {
            arena_scope(arena);
            
            Linked_List<Record> defered_time_deltas = {}; // NOTE: for prints only
            Record *active_start = nullptr;
            Open_State open_state = Closed_PostPrint;
            s32 day_time_sum = 0;
            b32 day_header_printed = false;
            
            
            
            for (u64 index = range.first;
                 index < range.one_past_last;
                 ++index)
            {
                Record record = *state->records.at(index);
                
                if (!active_start && record.date > date_end) {
                    goto escape_all_loops_label;
                }
                
                
                if (has_filter)
                {
                    if (!active_start && index == range.next_day_record_index) {
                        continue;
                    }
                    
                    b32 matches_filter = equals(filter, record.desc);
                    
                    if (!active_start && !matches_filter) {
                        continue;
                    }
                    
                    if (active_start && record.type == Record_TimeStart) {
                        record.type = Record_TimeStop;
                    }
                }
                
                
                if (should_print && !day_header_printed)
                {
                    day_header_printed = true;
                    
                    String date_str = get_date_string(arena, range.date);
                    String day_of_week = get_day_of_the_week_string(arena, range.date);
                    
                    printf("\n");
                    
                    if (alt_color) {
                        print_color(Color_AltDate);
                    } else {
                        print_color(Color_Date);
                    }
                    
                    printf("%.*s %.*s", string_expand(date_str), string_expand(day_of_week));
                    
                    print_color(Color_Reset);
                    printf("\n");
                }
                
                
                
                if (record.date != range.date &&
                    active_start)
                {
                    // NOTE: case: "start" ends on next/another day   
                    assert(record.type == Record_TimeStop || record.type == Record_TimeStart);
                    assert(active_start);
                    
                    day_time_sum += record.value - active_start->value;
                    day_time_sum += safe_truncate_to_s32(record.date - active_start->date);
                    
                    if (should_print)
                    {
                        String time_str = get_time_string(arena, record.value);
                        printf("%.*s", string_expand(time_str));
                        
                        print_color(Color_Dimmed);
                        if (record.date != range.date + Days(1))
                        {
                            String date_str = get_date_string(arena, record.date);
                            printf(" (%.*s)", string_expand(date_str));
                        }
                        else
                        {
                            printf(" (next day)");
                        }
                        print_color(Color_Reset);
                        
                        
                        print_description(active_start);
                        print_defered_time_deltas(arena, &defered_time_deltas);
                        // print all defers (TimeDelta & CountDelta) here
                        printf("\n");
                    }
                    
                    active_start = nullptr;
                }
                else
                {
                    if (open_state == Open)
                    {
                        if (record.type == Record_TimeDelta)
                        {
                            day_time_sum += record.value;
                            
                            if (should_print) {
                                // NOTE: case: this needs to be printed _after_ we print "stop"
                                *defered_time_deltas.push_get_item(arena) = record;
                            }
                        }
                        else if (record.type == Record_TimeStart ||
                                 record.type == Record_TimeStop)
                        {
                            assert(active_start);
                            day_time_sum += record.value - active_start->value;
                            
                            
                            if (should_print)
                            {
                                if (record.type == Record_TimeStart) {
                                    print_color(Color_Dimmed);
                                }
                                
                                String time_str = get_time_string(arena, record.value);
                                printf("%.*s", string_expand(time_str));
                                
                                print_color(Color_Reset);
                                
                                
                                print_description(active_start);
                                
                                
                                print_defered_time_deltas(arena, &defered_time_deltas);
                                // print CountDelta defers on new lines
                                
                                printf("\n");
                                
                                
                                // NOTE: In case of "start 1; start 2; assumed end time"
                                // this defered_time_deltas would get printed twice
                                // (additional print for assumed time range)
                                defered_time_deltas = {};
                            }
                            
                            
                            
                            active_start = nullptr;
                        }
                        else
                        {
                            // defer2 counts?
                            assert(0);
                        }
                        
                        
                        if (record.type == Record_TimeStart) {
                            open_state = Closed_PostPrint;
                        } else if (record.type == Record_TimeStop) {
                            open_state = Closed_PrePrint;
                        }
                    }
                    
                    
                    if (open_state == Closed_PostPrint)
                    {
                        if (record.type == Record_TimeDelta)
                        {
                            day_time_sum += record.value;
                            
                            if (should_print)
                            {
                                printf("      ");
                                print_time_delta(arena, &record);
                                printf("\n");
                            }
                        }
                        else if (record.type == Record_TimeStart)
                        {
                            active_start = state->records.at(index);
                            open_state = Open;
                            
                            if (should_print)
                            {
                                String time_str = get_time_string(arena, record.value);
                                printf("%.*s -> ", string_expand(time_str));
                            }
                        }
                        else
                        {
                            // print count\n
                            assert(record.type != Record_TimeStop);
                            assert(0);
                        }
                    }
                    
                    
                    if (open_state == Closed_PrePrint) {
                        open_state = Closed_PostPrint;
                    }
                }
            }
            
            
            
            
            if (active_start)
            {
#if Def_Slow
                open_start_ending_should_happen_only_once_test += 1;
                assert(open_start_ending_should_happen_only_once_test < 2);
#endif
                date64 today = get_today();
                s32 now_time = get_time();
                
                s32 local_sum = now_time - active_start->value;
                local_sum += safe_truncate_to_s32(today - active_start->date);
                
                
                day_time_sum += local_sum;
                result.time_assumed += local_sum;
                
                
                if (should_print)
                {
                    String time_str = get_time_string(arena, now_time);
                    print_color(Color_HelpHeader);
                    printf("%.*s ", string_expand(time_str));
                    
                    if (local_sum < Days(1)) {
                        printf("(now)");
                    } else {
                        print_color(Color_Error);
                        printf("(missing stop)");
                    }
                    
                    print_color(Color_Reset);
                    print_description(active_start);
                    print_defered_time_deltas(arena, &defered_time_deltas);
                    // print CountDelta defers on new lines
                    printf("\n");
                }
                
                
                active_start = nullptr;
            }
            
            
            
            if (should_print && day_header_printed)
            {
                // TODO(f0): Figure out missing ending stuff
                String time = get_time_string(arena, day_time_sum);
                String bar = get_progress_bar_string(arena, day_time_sum, (result.time_assumed == 0));
                
                print_color(Color_Dimmed);
                print_color(Color_Positive);
                printf("Time total: %.*s  ", string_expand(time));
                
                printf("%.*s", string_expand(bar));
                
                print_color(Color_Reset);
                printf("\n");
            }
            
            
            result.time_total += day_time_sum;
            
            if (range.next_day_record_index >= state->records.count) {
                goto escape_all_loops_label;
            }
        }
        
        
        escape_all_loops_label:
        
        result.next_day_record_index = range.next_day_record_index;
    }
    
    
    return result;
}



//
//~ Summary
//
internal void
print_summary(Program_State *state, Granularity granularity,
              date64 date_begin, date64 date_end,
              String filter)
{
    Arena *arena = &state->arena;
    arena_scope(arena);
    date64 today = get_today();
    
    Record_Index_Pair starting_pair = get_record_and_index_for_starting_date(state, date_begin, 0);
    Record *record = starting_pair.record;
    u64 start_index = starting_pair.index;
    
    
    if (record && (record->date <= date_end))
    {
        for (;;)
        {
            Boundaries_Result boundary = {};
            switch (granularity)
            {
                case Granularity_Days: {
                    boundary.first = record->date;
                    boundary.last = record->date;
                } break;
                
                default: { assert(0); } // fall
                case Granularity_Months: {
                    boundary = get_month_boundaries(record->date);
                } break;
                
                case Granularity_Quarters: {
                    boundary = get_quarter_boundaries(record->date);
                } break;
                
                case Granularity_Years: {
                    boundary = get_year_boundaries(record->date);
                } break;
            }
            
            
            date64 first_date = pick_bigger(date_begin, boundary.first);
            date64 last_date = pick_smaller(date_end, boundary.last);
            Process_Days_Result days = process_days_from_range(state, start_index, first_date, last_date, filter, 0);
            
            start_index = days.next_day_record_index;
            
            
            String test_start_date = get_date_string(&state->arena, first_date);
            String test_last_date = get_date_string(&state->arena, last_date);
            
            
            
            if (days.time_total)
            {
                s32 day_count = safe_truncate_to_s32(((last_date - first_date) / Days(1)) + 1);
                
                if (days.next_day_record_index == state->records.count)
                {
                    s32 current_day_count = safe_truncate_to_s32(((today - first_date) / Days(1)) + 1);
                    
                    assert(current_day_count > 0);
                    assert(current_day_count <= day_count);
                    day_count = current_day_count;
                }
                
                
                String date_str = get_date_string(arena, first_date);
                print_color(Color_Date);
                printf("%.*s", string_expand(date_str));
                
                if (boundary.description) {
                    printf(" (%s)", boundary.description);
                }
                
                String sum_str = get_time_string(arena, days.time_total);
                print_color(Color_Positive);
                printf("\tsum: %.*s\t", string_expand(sum_str));
                
                
                b32 is_range_closed = (days.time_assumed == 0);
                String bar = {};
                
                if (day_count > 1)
                {
                    s32 avg = days.time_total / day_count;
                    String avg_str = get_time_string(arena, avg);
                    printf("avg(/%3d): %.*s\t", day_count, string_expand(avg_str));
                    
                    bar = get_progress_bar_string(arena, avg, is_range_closed);
                }
                else
                {
                    bar = get_progress_bar_string(arena, days.time_total, is_range_closed);
                }
                
                printf("%.*s\n", string_expand(bar));
            }
            
            
            if (days.next_day_record_index >= state->records.count || days.next_day_record_index == 0) {
                break;
            }
            
            record = state->records.at(days.next_day_record_index);
            if (record->date > date_end) {
                break;
            }
        }
    }
    
    print_color(Color_Reset);
}


//
//~ Top
//
internal void
print_top(Program_State *state,
          date64 date_begin, date64 date_end,
          String filter)
{
    Record_Index_Pair starting_pair = get_record_and_index_for_starting_date(state, date_begin, 0);
    Record *record = starting_pair.record;
    u64 start_index = starting_pair.index;
    
    if (record && (record->date <= date_end))
    {
        
    }
}