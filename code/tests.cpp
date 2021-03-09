#include "stf0.h"


s32 main()
{
    Arena arena_ = create_virtual_arena();
    Arena *arena = &arena_;
    
    Directory cw_dir = push_current_working_directory(arena);
    Directory out_dir = push_directory_append(arena, cw_dir, l2s("output"));
    directory_delete_all_files(arena, out_dir);
    
    Directory ref_dir = push_directory_append(arena, cw_dir, l2s("reference"));
    Directory input_dir = push_directory_append(arena, cw_dir, l2s("input"));
    
    b32 errors = false;
    Path_List input_list = push_find_path_list_in_directory(arena, input_dir);
    
    for_linked_list(input_node, input_list)
    {
        memory_scope(arena);
        
        Path *input_path = &input_node->item;
        Path out_path = {
            out_dir,
            input_path->file_name
        };
            
        file_copy(arena, input_path, &out_path, true);
        
        char *out_path_cstr = push_cstr_from_path(arena, &out_path);
        char *command = push_cstrf(arena, "..\\build\\main.exe -r -d \"%s\" > nul", out_path_cstr);
        Pipe_Handle pipe = pipe_open_read(command);
        s32 close_code = pipe_close(&pipe);
        
        if (no_errors(&pipe))
        {
            Path ref_path = {
                ref_dir,
                input_path->file_name
            };
            
            String ref_content = push_read_entire_file(arena, &ref_path);
            String out_content = push_read_entire_file(arena, &out_path);
            
            // TODO(f0): compare that counts lines and position
            Compare_Line_Pos compare = string_compare_with_line_column(ref_content, out_content);
            if (compare.is_equal)
            {
                printf("[Ok] %.*s; ", string_expand(out_path.file_name));
            }
            else
            {
                printf("[Not equal!] %.*s at line(%u), column(%u); ",
                       string_expand(out_path.file_name), compare.line, compare.column);
                errors = true;
            }
        }
        else
        {
            printf("[Pipe error] %s; ", out_path_cstr);
            errors = true;
        }
    }
    
    return (errors ? 1 : 0);
}

