
inline b32
is_date_separator(char c)
{
    b32 result = ((c == '-') || (c == '.'));
    return result;
}

inline b32
is_time_separator(char c)
{
    b32 result = (c == ':');
    return result;
}

internal Line
eat_all_whitespace(Lexer *lx)
{
    u32 lines_count = 0;
    
    for (;;)
    {
        if (is_whitespace(lx->at[0]))
        {
            ++lx->at;
        }
        else if (is_end_of_line(lx->at[0]))
        {
            ++lx->at;
            lines_count += 1;

            if ((lx->at[0] == '\r') && (lx->at[1] == '\n'))
            {
                lines_count -= 1;
            }
        }
        else if (lx->at[0] == '/' && lx->at[1] == '/')
        {
            lx->at += 2;
            while (lx->at[0] && !is_end_of_line(lx->at[0]))
            {
                ++lx->at;
            }
        }
        else
        {
            break;
        }
    }
    
    
    lx->line_row += lines_count;
    Line line = {};
    line.row = lx->line_row;
    line.current_line_start = lx->at;
    return line;
}


internal Token
fetch_token(Lexer *lx)
{
    Token token = {};
    token.line = eat_all_whitespace(lx);
    token.text = string(lx->at, 0);

    char c = lx->at[0];
    ++lx->at;
    
    switch (c)
    {
        case '\0': {
            token.type = Token_End_Of_Stream;
            --lx->at;
        } break;
        
        
        case ';': {
            token.type = Token_Semicolon;
        } break;
        
        
        case '"': {
            token.type = Token_String;
            token.text.str = lx->at;
            while (lx->at[0] && lx->at[0] != '"')
            {
                ++lx->at;
            }
            
            token.text.size = lx->at - token.text.str;
            if (lx->at[0] == '"')
                ++lx->at;
        } break;
        
        
        case '-':
        case '+': {
            token.type = Token_Offset;
            token.text.str = lx->at-1;
            
            eat_all_whitespace(lx);
            
            while (lx->at[0] && is_number(lx->at[0])) {
                ++lx->at;
            }
            
            token.text.size = lx->at - token.text.str;
        } break;
        
        
        default: {
            if (is_alpha(c))
            {
                token.type = Token_Identifier;
                
                while (is_alpha(lx->at[0]) || is_number(lx->at[0]) || lx->at[0] == '_')
                {
                    ++lx->at;
                }
                
                token.text.size = lx->at - token.text.str;
            }
            else if (is_number(c))
            {
                token.type = Token_Time;
                
                while (is_number(lx->at[0]) || is_date_separator(lx->at[0]) ||
                       is_time_separator(lx->at[0]))
                {
                    ++lx->at;
                    if (is_date_separator(lx->at[0]))
                    {
                        token.type = Token_Date;
                    }
                }
                
                token.text.size = lx->at - token.text.str;
            }
            else
            {
                token.type = Token_Unknown;
            }
        } break;
    }
    
    
    return token;
}




internal Lexer
create_lexer(char *input_text)
{
    Lexer lexer = {};
    lexer.at = (u8 *)input_text;
    lexer.line_row = 1;
    
    for_u32(i, array_count(lexer.tokens))
    {
        lexer.tokens[i] = fetch_token(&lexer);
    }

    return lexer;
}


inline Token
peek_token(Lexer *lx, u32 index)
{
    assert(index < Max_Token_Peek);
    
    u32 key = (lx->current_token_index + index) % array_count(lx->tokens);
    Token result = lx->tokens[key];
    return result;
}


inline void
advance(Lexer *lx)
{
    lx->current_token_index += 1;
    lx->current_token_index %= array_count(lx->tokens);
    
    assert(lx->current_token_index != lx->next_refresh_start_index);
    
    if (lx->current_token_index < lx->next_refresh_start_index)
    {
        // NOTE: case where: [..., current, ..., next, ...]
        u32 distance = lx->next_refresh_start_index - lx->current_token_index;
        
        
        if (distance < Max_Token_Peek)
        {
            for (u32 token_index = lx->next_refresh_start_index;
                 token_index < array_count(lx->tokens);
                 ++token_index)
            {
                lx->tokens[token_index] = fetch_token(lx);
            }
            
            for (u32 token_index = 0;
                 token_index < lx->current_token_index;
                 ++token_index)
            {
                lx->tokens[token_index] = fetch_token(lx);
            }
            
            lx->next_refresh_start_index = lx->current_token_index;
        }
    }
    else
    {
        // NOTE: case where: [..., next, ..., current, ...]
        u32 distance = array_count(lx->tokens) - lx->current_token_index;
        distance += lx->next_refresh_start_index;
        
        if (distance < Max_Token_Peek)
        {
            for (u32 token_index = lx->next_refresh_start_index;
                 token_index < lx->current_token_index;
                 ++token_index)
            {
                lx->tokens[token_index] = fetch_token(lx);
            }
            
            lx->next_refresh_start_index = lx->current_token_index;
        }
    }
    
}




internal b32
token_equals(Token token, char *match, b32 case_sensitive = false)
{
    String match_str = string(match);
    b32 result = equals(token.text, match_str, case_sensitive);
    return result;
}

