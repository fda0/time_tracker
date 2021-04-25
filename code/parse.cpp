internal Parse_Number_Result
parse_number(u8 *src, u64 count)
{
    Parse_Number_Result result = {};
    
    s32 sign = 1;
    if (count > 0 &&
        (src[0] == '+' || src[0] == '-'))
    {
        if (src[0] == '-') {
            sign = -1;
        }
        
        src += 1;
        count -= 1;
    }
    
    u64 leading_spaces = 0;
    for_u64(index, count)
    {
        u8 character = src[index];
        if (character == ' ') {
            leading_spaces += 1;
        } else {
            break;
        }
    }
    src += leading_spaces;
    count -= leading_spaces;
    
    
    s32 multiplier = 1;
    for_u64(index, count)
    {
        u64 reverse_index = count-index-1;
        u8 character = src[reverse_index];
        
        if (!((character >= '0') && (character <= '9'))) {
            return result; // invalid
        }
        
        s32 to_add = (character - '0') * multiplier;
        result.value += to_add;
        
        multiplier *= 10;
    }
    
    result.value *= sign;
    result.is_valid = true;
    return result;
}

inline Parse_Number_Result
parse_number(Token token)
{
    Parse_Number_Result result = parse_number(token.text.str, token.text.size);
    return result;
}


internal Parse_Date_Result
parse_date(Record_Session *session, Token token)
{
    assert(token.type == Token_Date);
    
    // NOTE: Supported format: 2020-12-31
    Parse_Date_Result result = {};
    
    if (token.text.size == (4 + 1 + 2 + 1 + 2))
    {
        u8 *text = token.text.str;
        tm date = {};
        
        // year
        auto year = parse_number(text, 4);
        date.tm_year = year.value - 1900;
        text += 4;
        
        b32 dash1 = is_date_separator(*text++);
        
        // month
        auto month = parse_number(text, 2);
        date.tm_mon = month.value - 1;
        text += 2;
        
        b32 dash2 = is_date_separator(*text++);
        
        // day
        auto day = parse_number(text, 2);
        date.tm_mday = day.value;
        
        
        result.date = platform_tm_to_time(&date);
        result.is_valid = (year.is_valid && month.is_valid && day.is_valid && dash1 && dash2);
    }
    
    if (!result.is_valid) {
        session_set_error(session, "Bad date format!");
    }
    
    return result;
}

internal Parse_Time_Result
parse_time(Record_Session *session, Token token)
{
    // NOTE: Supported format: 10:32, 02:00, 2:0, 120...
    Parse_Time_Result result = {};
    
    if (token.text.size > 0)
    {
        b32 had_first_colon = false;
        u32 multiplier = 1;
        
        for (s32 index = (s32)token.text.size - 1; index >= 0; --index)
        {
            char c = token.text.str[index];
            
            if (c >= '0' && c <= '9')
            {
                u32 digit_value = (c - '0') * multiplier * 60;
                
                if (had_first_colon)
                {
                    digit_value *= 60;
                }
                
                result.value += digit_value;
                multiplier *= 10;
            }
            else if (is_time_separator(c))
            {
                
                if (had_first_colon)
                {
                    return result;
                }
                else
                {
                    had_first_colon = true;
                    multiplier = 1;
                }
            }
        }
        
        result.is_valid = true;
    }
    
    if (!result.is_valid)
    {
        session_set_error(session, "Bad time format");
    }
    
    return result;
}
