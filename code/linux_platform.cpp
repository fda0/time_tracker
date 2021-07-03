
#include <pthread.h>

function date64
platform_tm_to_time(tm *date)
{
    date64 result = timegm(date);
    return result;
}

function File_Time
platform_get_file_mod_time(Arena *arena, Path path)
{
    arena_scope(arena);
    time_t result = 0;
    char *path_cstr = to_cstr(arena, path);
    
    struct stat stat_data;
    if (stat(path_cstr, &stat_data) != -1)
    {
        result = stat_data.st_mtime;
    }
    
    return result;
}

function s32
platform_compare_file_time(File_Time first, File_Time second)
{
    s32 result = first - second;
    return result;
}


function void
platform_create_thread(New_Thread_Function *start_func, Thread_Memory *data)
{
    pthread_t thread_handle = {};
    pthread_create(&thread_handle, nullptr, start_func, data);
}

function void
platform_sleep(u32 miliseconds)
{
    usleep(miliseconds*1000);
}



internal void
initialize_colors()
{
}


function void
platform_clear_screen()
{
    printf("\033[H\033[J");
}
