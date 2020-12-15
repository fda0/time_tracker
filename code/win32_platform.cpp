#include <windows.h>

typedef FILETIME File_Time;

internal void 
platform_create_directory(char *path)
{
    CreateDirectoryA(path, NULL);
}

internal void 
platform_copy_file(char *source, char *destination)
{
    CopyFileA(source, destination, TRUE);
}

inline date64
platform_tm_to_time(tm *date)
{
    date64 result = _mkgmtime(date);
    return result;
}

internal File_Time 
platform_get_file_mod_time(char *file_name)
{
    FILETIME lastWriteTime = {};
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (GetFileAttributesEx(file_name, GetFileExInfoStandard, &data))
    {
        lastWriteTime = data.ftLastWriteTime;
    }
    return lastWriteTime;
}

internal s32 
platform_compare_file_time(File_Time first, File_Time second)
{
    s32 result = CompareFileTime(&first, &second);
    return result;
}

internal void 
platform_create_thread(void *start_func, Thread_Memory *data)
{
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_func, data, 0, NULL);
}

inline void 
platform_sleep(u32 miliseconds)
{
    Sleep(miliseconds);
}

internal void 
platform_add_ending_slash_to_path(char *path)
{
    u32 length = (u32)strlen(path);
    if (path[length - 1] != '/' ||
        path[length - 1] != '\\')
    {
        path[length] = '\\';
        path[length + 1] = 0;
    }
}


internal void platform_open_in_default_editor(char *file_name)
{
    ShellExecuteA(NULL, "open", file_name, NULL, NULL, SW_SHOWNORMAL);
}


internal void 
platform_get_executable_path(char *output, u32 output_size)
{
    GetModuleFileNameA(0, output, output_size);
}



namespace Global_Color
{
    global_variable char *f_black = "\033[30m";
    global_variable char *f_date = "\033[33m";
    global_variable char *f_sum = "\033[32m";
    global_variable char *f_desc = "\033[36m";
    global_variable char *f_desc_delta = "\033[96m";
    global_variable char *f_reset = "\033[39m";
    
    global_variable char *b_error = "\033[41m";
    global_variable char *b_date = "\033[43m";
    global_variable char *b_reset = "\033[49m";
};

internal void 
initialize_colors(bool turn_off_colors)
{
    if (!turn_off_colors)
    {
        turn_off_colors = true;
        
        // Set output mode to handle virtual terminal sequences
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE)
        {
            DWORD dwMode = 0;
            if (GetConsoleMode(hOut, &dwMode))
            {
                dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                if (SetConsoleMode(hOut, dwMode))
                {
                    turn_off_colors = false;
                }                
            }
        }
    }
    
    if (turn_off_colors)
    {
        using namespace Global_Color;
        f_black = "",
        f_date = "";
        f_sum = "";
        f_desc = "";
        f_desc_delta = "";
        f_reset = "";
        
        b_error = "";
        b_date = "";
        b_reset = "";
    }
}


internal void 
platform_clear_screen()
{
    s32 columns_to_clear = 30;
    
    HANDLE screen = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO buffer;
    if (GetConsoleScreenBufferInfo(screen, &buffer))
    {
        columns_to_clear = buffer.dwMaximumWindowSize.Y;
    }
    
    for (int i = 0; i < columns_to_clear; ++i)
    {
        printf("\n");
    }
}




