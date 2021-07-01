#include "stf0.h"

// TODO(f0): save outputs to array, show errors first for readability

struct Message_Pair
{
    String message;
    String file_name;
};

struct Tests_Summary
{
    Arena arena;
    Linked_List<Message_Pair> messages;
    s32 success_count;
    s32 test_count;
};

inline void
add_summary(Tests_Summary *summary, b32 success, String message, String file_name)
{
    summary->test_count += 1;
    Message_Pair *pair;
    
    if (success)
    {
        summary->success_count += 1;
        pair = summary->messages.push(&summary->arena);
    }
    else
    {
        pair = summary->messages.push(&summary->arena);
    }
    
    pair->message = copy(&summary->arena, message);
    pair->file_name = copy(&summary->arena, file_name);
}

internal void
print_summary(Tests_Summary *summary)
{
    printf("Success %d / %d;", summary->success_count, summary->test_count);
    s32 error_count = summary->test_count - summary->success_count;
    if (error_count) {
        printf("__________ Errors: %d __________; ", error_count);
    }
    
    printf("\n");
    
    for_linked_list(node, summary->messages)
    {
        Message_Pair *item = &node->item;
        
        printf("[%.*s] %.*s; ", string_expand(item->file_name), string_expand(item->message));
        
        if (--error_count == 0) {
            printf("\n");
        }
    }
}








s32 main()
{
    stf0_initialize();
    Time_Perfomance start = platform_get_perfomance_time();
    
    Tests_Summary summary = {};
    summary.arena = create_virtual_arena();
    
    Arena arena_ = create_virtual_arena();
    Arena *arena = &arena_;
    
    Directory cw_dir = platform_get_current_working_directory(arena);
    Directory out_dir = directory_append(arena, cw_dir, l2s("output"));
    platform_directory_delete_all_files(arena, out_dir);
    
    Directory ref_dir = directory_append(arena, cw_dir, l2s("reference"));
    Directory input_dir = directory_append(arena, cw_dir, l2s("input"));
    
    Path_List input_list = platform_list_files_in_directory(arena, input_dir);
    
    for_linked_list(input_node, input_list)
    {
        arena_scope(arena);
        
        Path input_path = input_node->item;
        Path out_path = {
            out_dir,
            input_path.file_name
        };
            
        platform_file_copy(arena, input_path, out_path, true);
        
        char *out_path_cstr = to_cstr(arena, out_path);
        char *command = cstrf(arena, "..\\build\\tt.exe -r -d \"%s\" > nul", out_path_cstr);
        Pipe_Handle pipe = platform_pipe_open_read(command);
        //String_List pipe_output = save_pipe_output(arena, &pipe); // TODO(f0): figure out why this didn't work
        s32 close_code = platform_pipe_close(&pipe);
        
        if (no_errors(&pipe))
        {
            Path ref_path = {
                ref_dir,
                input_path.file_name
            };
            
            File_Content ref_content = platform_read_entire_file(arena, ref_path);
            File_Content out_content = platform_read_entire_file(arena, out_path);
            
            if (no_errors(&ref_content))
            {
                if (no_errors(&out_content))
                {
                    Compare_Line_Pos compare = compare_with_line_column(ref_content.content, out_content.content);
                    if (compare.is_equal)
                    {
                        add_summary(&summary, true, l2s("Ok"), out_path.file_name);
                    }
                    else
                    {
                        u64 half_range = 6;
                        u64 diff_index = pick_bigger(0, compare.column - half_range);
                        
                        String ref_diff = string_advance(ref_content.content, diff_index);
                        ref_diff.size = pick_smaller(half_range*2, ref_diff.size);
                        
                        String out_diff = string_advance(out_content.content, diff_index);
                        out_diff.size = pick_smaller(half_range*2, out_diff.size);
                        
                        
                        String message = stringf(arena, "Files not equal at line(%u), col(%u)\n"
                                                 "ref::>%.*s\n"
                                                 "out::>%.*s\n",
                                                 compare.line, compare.column,
                                                 string_expand(ref_diff),
                                                 string_expand(out_diff));
                        
                        add_summary(&summary, false, message, out_path.file_name);
                    }
                }
                else
                {
                    add_summary(&summary, false, l2s("Read fail"), out_path.file_name);
                }
            }
            else
            {
                add_summary(&summary, false, l2s("Read fail"), ref_path.file_name);
            }
            
            
        }
        else
        {
            add_summary(&summary, false, l2s("Pipe open fail"), out_path.file_name);
        }
    }
    
    
    Time_Perfomance end = platform_get_perfomance_time();
    f32 seconds = get_seconds_elapsed(end, start);
    
    printf("[Perfomance time] %fs, %.3fms; ", seconds, 1000.f*seconds);
    
    print_summary(&summary);
    
    
    return summary.test_count - summary.success_count;
}

