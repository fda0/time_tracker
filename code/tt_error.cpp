
#define Print_Load_Error(State) \
do { \
if (State->reading_from_file) \
{ printf("%s[Load Error #%d] ", Global_Color::b_error, state->load_error_count++); } \
else { printf("%s", Global_Color::b_error); } \
} while(0)


#define Print_Clear() printf("%s%s", Global_Color::b_reset, Global_Color::f_reset)
#define Print_ClearN() printf("%s%s\n", Global_Color::b_reset, Global_Color::f_reset)



