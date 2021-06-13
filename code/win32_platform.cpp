



inline date64
platform_tm_to_time(tm *date)
{
    date64 result = _mkgmtime(date);
    return result;
}

internal File_Time
platform_get_file_mod_time(Arena *arena, Path path)
{
    arena_scope(arena);
    char *file_name = to_cstr(arena, path);
    
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
initialize_colors()
{
    b32 success = false;
    
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
                success = true;
            }
        }
    }
    
    
    if (!success) {
        global_state.colors_disabled = true;
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
