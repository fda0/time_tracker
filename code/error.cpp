
#define Print_Parse_Error(State)                                                        \
    do                                                                                  \
    {                                                                                   \
        if (State->reading_from_file)                                                   \
        {                                                                               \
            printf("%s[Parse Error #%d] ", Color::b_error, state->parse_error_count++); \
        }                                                                               \
        else                                                                            \
        {                                                                               \
            printf("%s", Color::b_error);                                               \
        }                                                                               \
    } while (0)


#define Print_Error()                 \
    do                                \
    {                                 \
        printf("%s", Color::b_error); \
    } while (0)


#define Print_Clear() printf("%s%s", Color::b_reset, Color::f_reset)
#define Print_ClearN() printf("%s%s\n", Color::b_reset, Color::f_reset)


#define Command_Line_Only_Message(STATE, TOKEN)                                                      \
    do                                                                                               \
    {                                                                                                \
        Print_Parse_Error(STATE);                                                                    \
        printf("%.*s - command supported only in command line", (s32)TOKEN.text_length, TOKEN.text); \
        print_line_with_token(token);                                                                \
        Print_ClearN();                                                                              \
    } while (0)
