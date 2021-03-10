
// TODO(f0): Parse X (16?) elements ahead and put them in ring buffer

inline b32
is_end_of_line(char c)
{
    b32 result = ((c == '\n') || (c == '\r'));

    return result;
}

inline b32
is_whitespace(char c)
{
    b32 result = ((c == ' ') || (c == '\t') || (c == '\v') || (c == '\f'));

    return result;
}

inline b32
is_alpha(char c)
{
    b32 result = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');

    return result;
}

inline b32
is_number(char c)
{
    b32 result = (c >= '0' && c <= '9');
    return result;
}

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

internal void
eat_all_whitespace(Lexer *lx)
{
    for (;;)
    {
        if (is_whitespace(lx->at[0]))
        {
            ++lx->at;
        }
        else if (is_end_of_line(lx->at[0]))
        {
            ++lx->at;
            lx->line_start = lx->at;
            ++lx->line_count;

            if ((lx->at[0] == '\r') && (lx->at[1] == '\n'))
            {
                --lx->line_count;
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
}


internal Lexer
create_lexer(char *input_text)
{
    Lexer lexer = {};
    lexer.at = (u8 *)input_text;
    lexer.line_start = (u8 *)input_text;
    return lexer;
}


internal Token
get_token(Lexer *lx)
{
    eat_all_whitespace(lx);

    Token token = {};
    token.text = string(lx->at, 1);

    char c = lx->at[0];
    ++lx->at;
    switch (c)
    {
    case '\0': {
        token.type = Token_End_Of_Stream;
    }
    break;

    case ';': {
        token.type = Token_Semicolon;
    }
    break;

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
    }
    break;

    case '-':
    case '+': {
        token.type = Token_Offset;
        token.text.str = lx->at;
        do
        {
            ++lx->at;
        } while (lx->at[0] && is_number(lx->at[0]));

        token.text.size = lx->at - token.text.str;
    }
    break;

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
    }
    break;
    }

    return token;
}



internal b32
token_equals(Token token, char *match, b32 case_sensitive = false)
{
    String match_str = string(match);
    b32 result = string_equal(token.text, match_str, case_sensitive);
    return result;
}


internal Forward_Token
create_forward_token(Lexer *lx)
{
    Forward_Token result = {};
    result.lexer_ = lx;
    result.peek_lexer_ = *result.lexer_;

    result.peek = get_token(&result.peek_lexer_);

    return result;
}

inline void
advance(Forward_Token *forward)
{
    *forward->lexer_ = forward->peek_lexer_;
    forward->token = forward->peek;

    forward->peek = get_token(&forward->peek_lexer_);
}
