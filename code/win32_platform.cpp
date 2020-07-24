#include <windows.h>

typedef FILETIME File_Time;

internal void platform_create_directory(char *path)
{
    CreateDirectoryA(path, NULL);
}

internal void platform_copy_file(char *source, char *destination)
{
    CopyFileA(source, destination, TRUE);
}

inline time_t platform_tm_to_time(tm *date)
{
    time_t result = _mkgmtime(date);
    return result;
}

internal File_Time platform_get_file_mod_time(char *filename)
{
    FILETIME lastWriteTime = {};
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (GetFileAttributesEx(filename, GetFileExInfoStandard, &data))
    {
        lastWriteTime = data.ftLastWriteTime;
    }
    return lastWriteTime;
}

internal s32 platform_compare_file_time(File_Time first, File_Time second)
{
    s32 result = CompareFileTime(&first, &second);
    return result;
}

internal void platform_create_thread(LPTHREAD_START_ROUTINE start_func, Thread_Memory *data)
{
    CreateThread(NULL, 0, start_func, data, 0, NULL);
}

inline void platform_sleep(u32 miliseconds)
{
    Sleep(miliseconds);
}