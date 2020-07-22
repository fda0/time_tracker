#include <windows.h>

internal void create_directory(char *path)
{
    CreateDirectoryA(path, NULL);
}

internal void copy_file(char *source, char *destination)
{
    CopyFileA(source, destination, TRUE);
}