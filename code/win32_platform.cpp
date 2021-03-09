#include <windows.h>
#pragma comment(lib, "shell32.lib")


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
platform_get_file_mod_time(Arena *arena, Path *path)
{
    memory_scope(arena);
    char *file_name = push_cstr_from_path(arena, path);
    
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
    if (path[length - 1] != '/' || path[length - 1] != '\\')
    {
        path[length] = '\\';
        path[length + 1] = 0;
    }
}


internal void
platform_open_in_default_editor(char *file_name)
{
    ShellExecuteA(NULL, "open", file_name, NULL, NULL, SW_SHOWNORMAL);
}


internal void
platform_get_executable_path(char *output, u32 output_size)
{
    GetModuleFileNameA(0, output, output_size);
}



namespace Color
{
    global char *f_black = "\033[30m";
    global char *f_white = "\033[97m";
    global char *f_date = "\033[33m";
    global char *f_sum = "\033[32m";
    global char *f_desc = "\033[36m";
    global char *f_dimmed = "\033[90m";
    global char *f_desc_delta = "\033[96m";
    
    global char *b_error = "\033[41m";
    global char *b_date = "\033[43m";
    global char *b_help_header = "\033[100m";
    
    global char *f_reset = "\033[39m";
    global char *b_reset = "\033[49m";
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
        using namespace Color;
        f_black = "", f_white = "";
        f_date = "";
        f_sum = "";
        f_desc = "";
        f_dimmed = "";
        f_desc_delta = "";

        b_error = "";
        b_date = "";
        b_help_header = "";

        f_reset = "";
        b_reset = "";
    }
}


internal void
platform_clear_screen()
{
    // NOTE: Example 2 from https://docs.microsoft.com/en-us/windows/console/clearing-the-screen

    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    SMALL_RECT scrollRect;
    COORD scrollTarget;
    CHAR_INFO fill;

    // Get the number of character cells in the current buffer.
    if (!GetConsoleScreenBufferInfo(console_handle, &csbi))
    {
        return;
    }

    // Scroll the rectangle of the entire buffer.
    scrollRect.Left = 0;
    scrollRect.Top = 0;
    scrollRect.Right = csbi.dwSize.X;
    scrollRect.Bottom = csbi.dwSize.Y;

    // Scroll it upwards off the top of the buffer with a magnitude of the entire height.
    scrollTarget.X = 0;
    scrollTarget.Y = (SHORT)(0 - csbi.dwSize.Y);

    // Fill with empty spaces with the buffer's default text attribute.
    fill.Char.UnicodeChar = TEXT(' ');
    fill.Attributes = csbi.wAttributes;

    // Do the scroll
    ScrollConsoleScreenBuffer(console_handle, &scrollRect, NULL, scrollTarget, &fill);

    // Move the cursor to the top left corner too.
    csbi.dwCursorPosition.X = 0;
    csbi.dwCursorPosition.Y = 0;

    SetConsoleCursorPosition(console_handle, csbi.dwCursorPosition);
}
