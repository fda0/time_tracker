//
// NOTE: Original tokenizer code inspired by video series 
//       "Handmade Hero" by Casey Muratori - episode 206
//

#include "tt_token.h"

internal char *read_entire_file_and_null_terminate(Memory_Arena *arena, char *filename)
{
    char *result = 0;
    
    FILE *file = fopen(filename, "rb");
    if(file)
    {
        fseek(file, 0, SEEK_END);
        size_t file_size = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        result = Push_Array(arena, file_size + 1, char); 
        fread(result, file_size, 1, file);
        result[file_size] = 0;
        
        fclose(file);
    }
    
    return result;
}



inline b32 is_end_of_line(char c)
{
    b32 result = (c == '\n') ||
        (c == '\r');
    
    return result;
}

inline b32 is_whitespace(char c)
{
    b32 result = (c == ' ') ||
        (c == '\t') ||
        (c == '\v') ||
        (c == '\f') ||
        is_end_of_line(c);
    
    return result;
}

inline b32 is_alpha(char c)
{
    b32 result = (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z');
    
    return result;
}

inline b32 is_number(char c)
{
    b32 result = (c >= '0' && c <= '9');
    return result;
}

internal void eat_all_whitespace(Tokenizer *tokenizer)
{
    for (;;)
    {
        if (is_whitespace(tokenizer->at[0]))
        {
            ++tokenizer->at;
        }
        else if (tokenizer->at[0] == '/' &&
                 tokenizer->at[1] == '/')
        {
            tokenizer->at += 2;
            while (tokenizer->at[0] && !is_end_of_line(tokenizer->at[0]))
            {
                ++tokenizer->at;
            }
        }
        else
        {
            break;
        }
    }
}


internal Token get_token(Tokenizer *tokenizer)
{
    eat_all_whitespace(tokenizer);
    
    Token token = {};
    token.text_length = 1;
    token.text = tokenizer->at;
    
    char c = tokenizer->at[0];
    ++tokenizer->at;
    switch (c)
    {
        case '\0': {token.type = Token_End_Of_Stream;} break;
        
        case ';': {token.type = Token_Semicolon;} break;
        
        case '"':
        {
            token.type = Token_String;
            token.text = tokenizer->at;
            while (tokenizer->at[0] &&
                   tokenizer->at[0] != '"')
            {
                ++tokenizer->at;
            }
            
            token.text_length = tokenizer->at - token.text;
            if (tokenizer->at[0] == '"') ++tokenizer->at;
            
        } break;
        
        default:
        {
            if (is_alpha(c))
            {
                token.type = Token_Identifier;
                
                while (is_alpha(tokenizer->at[0]) ||
                       is_number(tokenizer->at[0]) ||
                       tokenizer->at[0] == '_' ||
                       tokenizer->at[0] == '-')
                {
                    ++tokenizer->at;
                }
                
                token.text_length = tokenizer->at - token.text;
            }
            else if (is_number(c))
            {
                token.type = Token_Time;
                
                while (is_number(tokenizer->at[0]) ||
                       tokenizer->at[0] == '-' ||
                       tokenizer->at[0] == ':')
                {
                    ++tokenizer->at;
                    if (tokenizer->at[0] == '-') token.type = Token_Date;
                }
                
                token.text_length = tokenizer->at - token.text;
            }
            else
            {
                token.type = Token_Unknown;
            }
        } break;
    }
    
    return token;
}

internal b32 token_equals(Token token, char *match)
{
    for (u32 index = 0;
         index < token.text_length;
         ++index, ++match)
    {
        if (*match == 0 ||
            token.text[index] != *match)
        {
            return false;
        }
    }
    
    b32 result = (*match == 0);
    return result;
}