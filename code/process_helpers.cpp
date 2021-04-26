internal Record *
get_last_record(Record_Session *session)
{
    Record *result = nullptr;
    s64 count = session->records->count;
    if (count)
    {
        result = session->records->at(count - 1);
    }
    
    return result;
}

inline b32
are_in_same_day(Record *record_a, Record *record_b)
{
    b32 result = (record_a->date == record_b->date);
    return result;
}

internal Parse_Complex_Date_Result
get_complex_date(Record_Session *session)
{
    Parse_Complex_Date_Result result = {};
    
    {
        Token token = peek_token(&session->lexer, 0);
        
        if (token.type == Token_Date)
        {
            Parse_Date_Result parse = parse_date(session, token);
            result.date = parse.date;
            result.status = (parse.is_valid) ? Status_Valid : Status_HasErrors;
            advance(&session->lexer);
        }
        else if (token.type == Token_Identifier)
        {
            if (!session->reading_from_file)
            {
                if (token_equals(token, "today") ||
                    (token_equals(token, "now")))
                {
                    result.date = get_today();
                    result.status = Status_Valid;
                    advance(&session->lexer);
                }
                else if (token_equals(token, "yesterday"))
                {
                    result.date = get_today()-Days(1);
                    result.status = Status_Valid;
                    advance(&session->lexer);
                }
                else if (token_equals(token, "tomorrow"))
                {
                    result.date = get_today()+Days(1);
                    result.status = Status_Valid;
                    advance(&session->lexer);
                }
            }
        }
    }
    
    if (result.status == Status_Valid)
    {
        Token token = peek_token(&session->lexer, 0);
        if (token.type == Token_Offset)
        {
            Parse_Number_Result offset = parse_number(token);
            if (!offset.is_valid) {
                result.status = Status_HasErrors;
            } else {
                result.date += offset.value*Days(1);
            }
            
            advance(&session->lexer);
        }
    }
    
    
    return result;
}


internal b32
fill_complex_date_optional(Record_Session *session, Record *record)
{
    b32 success = false;
    
    Parse_Complex_Date_Result parsed_date = get_complex_date(session);
    if (parsed_date.status == Status_Valid)
    {
        record->date = parsed_date.date;
        success = true;
    }
    else if (parsed_date.status == Status_NoMatchigTokens)
    {
        if (!session->reading_from_file)
        {
            record->date = get_today();
            success = true;
        }
        else
        {
            Record *last_record = get_last_record(session);
            if (last_record) {
                record->date = last_record->date;
                success = true;
            } else {
                session_set_error(session, "First record needs to specify date");
            }
        }
    }
    
    
    return success;
}

internal b32
fill_time_optional(Record_Session *session, Record *record)
{
    b32 success = false;
    Token token = peek_token(&session->lexer, 0);
    
    if (token.type == Token_Time)
    {
        advance(&session->lexer);
        
        Parse_Time_Result parsed_time = parse_time(session, token);
        if (parsed_time.is_valid)
        {
            record->value = parsed_time.value;
            success = true;
        }
    }
    else if (!session->reading_from_file)
    {
        record->value = get_time();
        success = true;
    }
    
    return success;
}

internal b32
fill_complex_date_time_optional(Record_Session *session, Record *record)
{
    b32 success = false;
    Token token = peek_token(&session->lexer, 0);
    
    if (token.type == Token_Identifier &&
        token_equals(token, "now"))
    {
        advance(&session->lexer);
        success = true;
        date64 now = get_current_timestamp();
        
        token = peek_token(&session->lexer, 0);
        if (token.type == Token_Offset)
        {
            advance(&session->lexer);
            Parse_Number_Result offset = parse_number(token);
            success = offset.is_valid;
            now += offset.value*Minutes(1);
        }
        
        record->date = truncate_to_date(now);
        record->value = truncate_to_time(now);
    }
    else
    {
        success = (fill_complex_date_optional(session, record) &&
                   fill_time_optional(session, record));
    }
    
    return success;
}

internal b32
fill_time_required(Record_Session *session, Record *record)
{
    b32 success = false;
    Token token = peek_token(&session->lexer, 0);
    
    if (token.type == Token_Time)
    {
        advance(&session->lexer);
        
        Parse_Time_Result parsed_time = parse_time(session, token);
        if (parsed_time.is_valid)
        {
            record->value = parsed_time.value;
            success = true;
        }
    }
    
    return success;
}

internal void
fill_description_optional(Record_Session *session, Record *record)
{
    Token token = peek_token(&session->lexer, 0);
    
    if (token.type == Token_String)
    {
        advance(&session->lexer);
        record->desc = token.text;
    }
}


internal Date_Range_Result
get_max_date_range(b32 valid_status)
{
    Date_Range_Result result;
    result.status = (valid_status ? Status_Valid : Status_NoMatchigTokens);
    result.first = 1;
    result.last = S64_Max;
    
    return result;
}

internal Date_Range_Result
get_date_range(Record_Session *session)
{
    Date_Range_Result result = get_max_date_range(false);
    b32 success = true;
    b32 has_matching_token = false;
    
    {
        Token token = peek_token(&session->lexer, 0);
        
        if (token.type == Token_Identifier &&
            token_equals(token, "from"))
        {
            has_matching_token = true;
            advance(&session->lexer);
            
            Parse_Complex_Date_Result date = get_complex_date(session);
            success = is_valid(date.status);
            result.first = date.date;
        }
    }
    
    
    if (success)
    {
        Token token = peek_token(&session->lexer, 0);
        
        if (token.type == Token_Identifier &&
            token_equals(token, "to"))
        {
            has_matching_token = true;
            advance(&session->lexer);
            
            Parse_Complex_Date_Result date = get_complex_date(session);
            success = is_valid(date.status);
            result.last = date.date;
        }
    }
    
    
    if (success)
    {
        Token token = peek_token(&session->lexer, 0);
        
        if (!has_matching_token)
        {
            if (token_equals(token, "all"))
            {
                has_matching_token = true;
                result = get_max_date_range(true);
                advance(&session->lexer);
            }
            else
            {
                Parse_Complex_Date_Result date = get_complex_date(session);
                result.status = date.status; // may return no matching token
                result.first = date.date;
                result.last = date.date;
            }
        }
        else
        {
            result.status = Status_Valid;
        }
    }
    else
    {
        result.status = Status_HasErrors;
    }
    
    
    return result;
}




internal Date_Range_Result
get_recent_days_range(Virtual_Array<Record> *records)
{
    date64 today = get_today();
    date64 start = today - Days(31);
    
    s64 record_count = records->count;
    if (record_count)
    {
        date64 last_date = records->at(record_count - 1)->date;
        if (last_date > today)
        {
            today = last_date;
        }
        
        s64 past_index = record_count - 120;
        if (past_index < 0)
        {
            past_index = 0;
        }
        
        date64 start_date = records->at(past_index)->date;
        if (start_date < start)
        {
            start = start_date;
        }
    }
    
    Date_Range_Result result = {start, today, Status_Valid};
    return result;
}


