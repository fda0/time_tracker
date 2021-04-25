
internal void
process_command_start(Record_Session *session)
{
    Record record = {};
    record.type = Record_TimeStart;
    
    b32 success = fill_complex_date_time_optional(session, &record);
    
    if (success) {
        fill_description_optional(session, &record);
    }
    
    if (success) {
        add_record(session, &record);
    } else {
        session_set_error(session, "Incorect command usage. Use:\n"
                          "start [yyyy-MM-dd] (hh:mm) [\"description\"]");
    }
}



internal void
process_command_stop(Record_Session *session)
{
    Record record = {};
    record.type = Record_TimeStop;
    
    b32 success = fill_complex_date_time_optional(session, &record);
    
    if (success) {
        add_record(session, &record);
    } else {
        session_set_error(session, "Incorect command usage. Use:\n"
                          "start [yyyy-MM-dd] (hh:mm)");
    }
}



internal void
process_command_add_sub(Record_Session *session, b32 is_add)
{
    Record record = {};
    record.type = Record_TimeDelta;
    
    b32 success = fill_complex_date_optional(session, &record);
    
    if (success)
    {
        success = fill_time_required(session, &record);
        if (!is_add) {
            record.value *= -1;
        }
        
        if (success && record.value == 0) {
            session_set_error(session, "Time equal to zero!");
            return;
        }
    }
    
    if (success) {
        fill_description_optional(session, &record);
    }
    
    if (success)
    {
        add_record(session, &record);
    }
    else
    {
        char error_message[512];
        snprintf(error_message, sizeof(error_message),
                 "Incorect command usage. Use:\n"
                 "%s [yyyy-MM-dd] (hh:mm) [\"description\"]",
                 (is_add) ? "add" : "sub");
        
        session_set_error(session, error_message);
    }
}




internal void
process_command_show(Program_State *state, Record_Session *session)
{
    Date_Range_Result range = get_date_range(session);
    
    if (range.status != Status_HasErrors)
    {
        String filter = {};
        
        Token token = peek_token(&session->lexer, 0);
        if (token.type == Token_String) {
            advance(&session->lexer);
            filter = token.text;
        }
        
        char *message = nullptr;
        
        if (range.status == Status_NoMatchigTokens)
        {
            if (filter.size)
            {
                range = get_max_date_range(true);
            }
            else
            {
                range = get_recent_days_range(session->records);
                message = "Range assumed from xxxx-xx-xx to xxxx-xx-xx; "
                    "To use all records specify filter or use \"show all\"\n";
            }
        }
        
        process_days_from_range(state, range.first, range.last, filter, ProcessDays_Print);
        
        if (message) {
            print_color(Color_Dimmed);
            printf("%s", message);
            print_color(Color_Reset);
        }
    }
    else
    {
        session_set_error(session, "Incorect command usage. Use:\n"
                          "show [yyyy-MM-dd]\n"
                          "show [from yyyy-MM-dd] [to yyyy-MM-dd]\n");
    }
}




internal void
process_command_summary(Program_State *state, Record_Session *session)
{
    Granularity granularity = Granularity_Months;
    // TODO: Pull out granularity check.
    
    {
        Token token = peek_token(&session->lexer, 0);
        
        if (token.type == Token_Identifier)
        {
            if (token_equals(token, "days") ||
                token_equals(token, "d"))
            {
                advance(&session->lexer);
                granularity = Granularity_Days;
            }
            else if (token_equals(token, "months") ||
                     token_equals(token, "m"))
            {
                advance(&session->lexer);
                granularity = Granularity_Months;
            }
            else if (token_equals(token, "quarters") ||
                     token_equals(token, "q"))
            {
                advance(&session->lexer);
                granularity = Granularity_Quarters;
            }
            else if (token_equals(token, "years") ||
                     token_equals(token, "y"))
            {
                advance(&session->lexer);
                granularity = Granularity_Years;
            }
        }
    }
    
    
    Date_Range_Result range = get_date_range(session);
    
    if (range.status == Status_NoMatchigTokens) {
        range = get_max_date_range(true);
    }
    
    
    String filter = {};
    {
        Token token = peek_token(&session->lexer, 0);
        if (token.type == Token_String) {
            advance(&session->lexer);
            filter = token.text;
        }
    }
    
    
    print_summary(state, granularity, range.first, range.last, filter);
    
    
    if (range.status == Status_HasErrors)
    {
        session_set_error(session, "Incorect command usage. Use:\n"
                          "summary [granularity]\n"
                          "summary [granularity] [yyyy-MM-dd]\n"
                          "summary [granularity] [from yyyy-MM-dd] [to yyyy-MM-dd]\n"
                          "\n\tgranularity - days/months (or d/m)\n");
    }
}


internal void
process_command_exit(Program_State *state, Record_Session *session)
{
    Token token = peek_token(&session->lexer, 0);
    
    if ((token.type == Token_Identifier) && (token_equals(token, "no-save")))
    {
        advance(&session->lexer);
        exit(0);
    }
    else
    {
        if (no_errors(session))
        {
            if (session->change_count > 0)
            {
                b32 save_result = save_to_file(state);
                
                if (save_result) {
                    exit(0);
                } else {
                    session_set_error(session, "Failed to save to file!");
                }
            }
            else
            {
                exit(0);
            }
        }
        else
        {
            session_set_error(session, "Errors detected, exit aborted");
        }
    }
}
