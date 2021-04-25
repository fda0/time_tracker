
inline char *
get_color(Color_Code code)
{
#if Def_Slow
    s64 count = Color_Count;
    assert(count == array_count(color_pairs));
    assert(array_count(color_pairs) > code);
    assert(color_pairs[code].code_check == code);
#endif
    char *result = "";
    
    if (!global_state.colors_disabled) {
        result = color_pairs[code].value;
    }
    
    return result;
}

inline void
print_color(Color_Code code)
{
    if (!global_state.colors_disabled)
    {
        char *color = get_color(code);
        printf("%s", color);
    }
}



inline void
print_help_item(char *command, char *args, char *help)
{
    print_color(Color_Positive);
    printf("%s\t", command);
    
    if (args) {
        print_color(Color_Description);
        printf("%s\t", args);
    }
    
    print_color(Color_Reset);
    printf("%s\n", help);
}

inline void
print_help_header(char *text)
{
    print_color(Color_HelpHeader);
    printf("%s", text);
    print_color(Color_Reset);
    printf("\n");
}

inline void
print_help_desc(char *text)
{
    print_color(Color_Dimmed);
    printf("%s\n", text);
    print_color(Color_Reset);
}

inline void
print_description(Record *record, Color_Code color = Color_Description)
{
    String desc = record->desc;
    if (desc.size)
    {
        print_color(color);
        printf(" \"%.*s\"", string_expand(desc));
        print_color(Color_Reset);
    }
}

inline void
print_time_delta(Arena *temp_arena, Record *time_delta_record)
{
    arena_scope(temp_arena);
    
    s32 time = time_delta_record->value;
    char sign = (time < 0) ? '-' : '+';
    
    if (sign == '-') {
        print_color(Color_AltNegative);
    } else {
        print_color(Color_AltPositive);
    }
    
    String time_str = get_time_string(temp_arena, time);
    printf("  %c%.*s", sign, string_expand(time_str));
    print_color(Color_Reset);
    
    print_description(time_delta_record, Color_AltDescription);
}

inline void
print_defered_time_deltas(Arena *temp_arena, Linked_List<Record> *defered)
{
    for_linked_list_ptr(node, defered)
    {
        Record record = node->item;
        print_time_delta(temp_arena, &record);
    }
}

