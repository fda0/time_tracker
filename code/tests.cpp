#include <stdio.h>
#include <windows.h>


// TODO: Don't compare results when one file is out of range...


int main()
{
    char *output_dir = "output";
    CreateDirectoryA(output_dir, NULL);
    //~ NOTE: Delete old files from output/
    printf("deleted: ");
    {
        // NOTE: Construct search string to find old files
        WIN32_FIND_DATAA file_data = {};
        char search_str[MAX_PATH];
        snprintf(search_str, sizeof(search_str), "%s\\*", output_dir);
        HANDLE file_handle = FindFirstFileA(search_str, &file_data);
        
        // NOTE: Iterate over these files and delete them
        if (file_handle != INVALID_HANDLE_VALUE) {
            do {
                if (!(file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    char full_path[MAX_PATH];
                    snprintf(full_path, sizeof(full_path), "%s\\%s", output_dir, file_data.cFileName);
                    
                    DeleteFileA(full_path);
                    printf(" %s,", full_path);
                }
            } while (FindNextFile(file_handle, &file_data) != 0);
        } 
    }
    
    
    // NOTE: Ensure reference and input directories exist
    char *ref_dir = "referece";
    CreateDirectoryA(ref_dir, NULL);
    char *input_dir = "input";
    CreateDirectoryA(input_dir, NULL);
    
    
    
    int file_counter = 0;
    int success_counter = 0;
    
    {
        // NOTE: Create search string to find all files inside input/
        WIN32_FIND_DATAA file_data = {};
        char search_str[MAX_PATH];
        snprintf(search_str, sizeof(search_str), "%s\\*", input_dir);
        HANDLE file_handle = FindFirstFileA(search_str, &file_data);
        
        
        
        if (file_handle != INVALID_HANDLE_VALUE) {
            do {
                if (!(file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    
                    // NOTE: Copy new files from input/ to output/
                    char input_path[MAX_PATH];
                    snprintf(input_path, sizeof(input_path), "%s\\%s", input_dir, file_data.cFileName);
                    char output_path[MAX_PATH];
                    snprintf(output_path, sizeof(output_path), "%s\\%s", output_dir, file_data.cFileName);
                    
                    CopyFileA(input_path, output_path, false);
                    //printf("copy: \"%s\" > \"%s\"  ", input_path, output_path);
                    
                    
                    
                    // NOTE: Run TimeTracker on output/
                    char command[1024];
                    snprintf(command, sizeof(command), 
                             "..\\build\\tt_main.exe -r -d \"%s\" > nul", output_path);
                    //printf("command: %s\n", command);
                    ++file_counter;
                    
                    system(command);
                    
                    
                    
                    // NOTE: Compare output/ to reference/
                    FILE *output_handle = fopen(output_path, "rb");
                    if (output_handle)
                    {
                        char ref_path[MAX_PATH];
                        snprintf(ref_path, sizeof(ref_path), "%s\\%s", ref_dir, file_data.cFileName);
                        
                        FILE *ref_handle = fopen(ref_path, "rb");
                        if (ref_handle)
                        {
                            int row = 1;
                            int column = 1;
                            
                            for (;;)
                            {
                                int ref_c = getc(ref_handle);
                                int out_c = getc(output_handle);
                                
                                if (ref_c != out_c)
                                {
                                    printf("[match error] FILE: \"%s\",\t"
                                           "'%c'(%02hhx) != '%c'(%02hhx),\t"
                                           "line: %d, column: %d\n",
                                           file_data.cFileName, 
                                           ref_c, ref_c, out_c, out_c, 
                                           row, column);
                                    
                                    break;
                                }
                                
                                if ((ref_c == EOF) || (out_c == EOF)) 
                                {
                                    printf("[success] %s", file_data.cFileName);
                                    ++success_counter;
                                    break;
                                }
                                
                                ++column;
                                if (ref_c == '\n')
                                {
                                    column = 1;
                                    ++row;
                                }
                            }
                            
                        }
                        else
                        {
                            printf("[IO error] Can't open %s\n", ref_path);
                        }
                    }
                    else
                    {
                        printf("[IO error] Can't open %s\n", output_path);
                    }
                }
                
                printf("\n");
            } while (FindNextFile(file_handle, &file_data) != 0);
        } 
    }
    
    
    
    printf("~ ~ ~    success rate:  %d  /  %d    ~ ~ ~\n", success_counter, file_counter);   
    
    
    
    return 0;
}