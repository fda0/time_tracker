
#define Print_Load_Error(State) \
do { \
if (State->reading_from_file) \
{ printf("%s[Load Error #%d] ", Global_Color::b_error, state->load_error_count++); } \
else { printf("%s", Global_Color::b_error); } \
} while(0)


#define Print_Clear() printf("%s%s", Global_Color::b_reset, Global_Color::f_reset)
#define Print_ClearN() printf("%s%s\n", Global_Color::b_reset, Global_Color::f_reset)


#define Command_Line_Only_Message(STATE, TOKEN) \
do { Print_Load_Error(STATE); \
printf("%.*s command supported only in command line", (s32)TOKEN.text_length, TOKEN.text); \
print_line_with_token(token); \
Print_ClearN(); } while(0)




