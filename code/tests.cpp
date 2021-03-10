#include "stf0.h"


s32 main()
{
    stf0_initialize();
    Time_Perfomance start = get_perfomance_time();
    
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
        arena_scope(arena);
        
        Path *input_path = &input_node->item;
        Path out_path = {
            out_dir,
            input_path->file_name
        };
            
        file_copy(arena, input_path, &out_path, true);
        
        char *out_path_cstr = push_cstr_from_path(arena, &out_path);
        char *command = push_cstrf(arena, "..\\build\\tt.exe -r -d \"%s\" > nul", out_path_cstr);
        Pipe_Handle pipe = pipe_open_read(command);
        s32 close_code = pipe_close(&pipe);
        
        if (no_errors(&pipe))
        {
            if (close_code != 0)
            {
                printf("[Error exit code(%d)] %.*s; ", close_code, string_expand(out_path.file_name));
            }
            else
            {
                Path ref_path = {
                    ref_dir,
                    input_path->file_name
                };
                
                String ref_content = push_read_entire_file(arena, &ref_path);
                String out_content = push_read_entire_file(arena, &out_path);
                
                
                Compare_Line_Pos compare = string_compare_with_line_column(ref_content, out_content);
                if (compare.is_equal)
                {
                    printf("[Ok] %.*s; ", string_expand(out_path.file_name));
                }
                else
                {
                    printf("[Not equal!] %.*s at line(%u), col(%u);\n",
                           string_expand(out_path.file_name), compare.line, compare.column);
                    
                    u64 half_range = 6;
                    
                    u64 diff_index = pick_bigger(0, compare.index - half_range);
                    
                    String ref_diff = string_advance_str(ref_content, diff_index);
                    ref_diff.size = pick_smaller(half_range*2, ref_diff.size);
                    printf("%.*s\n", string_expand(ref_diff));
                    
                    String out_diff = string_advance_str(out_content, diff_index);
                    out_diff.size = pick_smaller(half_range*2, out_diff.size);
                    printf("%.*s\n", string_expand(out_diff));
                    
                    
                    //debug_break();
                    errors = true;
                }
            }
        }
        else
        {
            printf("[Pipe error] %s; ", out_path_cstr);
            errors = true;
        }
    }
    
    
    Time_Perfomance end = get_perfomance_time();
    f32 seconds = get_seconds_elapsed(end, start);
    
    printf("[Perfomance time] %fs, %.3fms\n", seconds, 1000.f*seconds);
    
    
    //debug_break();
    return (errors ? 1 : 0);
}

