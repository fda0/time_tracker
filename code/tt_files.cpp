
//~ File string functions

internal char *
get_file_name_pointer_from_file_path(char *path)
{
    char *result = path;
    s32 len = (s32)strlen(path);
    
    for (s32 index = len - 1; 
         index >= 0; 
         --index)
    {
        if (path[index] == '\\' || 
            path[index] == '/')
        {
            result = path + index + 1;
            break;
        }
    }
    
    return result;
}

internal File_Path2 
get_file_path_divided(char *path)
{
    char *file_name_start = get_file_name_pointer_from_file_path(path);
    
    File_Path2 result = {};
    
    strncpy(result.file_name, file_name_start, sizeof(result.file_name));
    
    strncpy(result.directory, path, file_name_start - path);
    
    return result;
}


internal void
copy_path_with_different_extension(char *output, size_t output_size, char *source, char *new_extension)
{
    if (new_extension[0] == '.') ++new_extension;
    
    s32 source_one_past_dot = (s32)strlen(source);
    
    for (s32 source_index = source_one_past_dot;
         source_index >= 0;
         --source_index)
    {
        if (source[source_index] == '.')
        {
            source_one_past_dot = source_index + 1;
            break;
        }
        else if (source[source_index] == '\\' ||
                 source[source_index] == '/')
        {
            break;
        }
    }
    
    s32 output_index = 0;
    for (;
         output_index < Minimum(source_one_past_dot, output_size - 1);
         ++output_index)
    {
        output[output_index] = source[output_index];
    }
    
    
    if ((source[source_one_past_dot - 1] != '.') &&
        output_index < (output_size - 1))
    {
        output[output_index++] = '.';
    }
    
    
    for (s32 ext_index = 0;
         (output_index < output_size - 1) && new_extension[ext_index];
         ++ext_index, ++output_index)
    {
        output[output_index] = new_extension[ext_index];
    }
    
    output[output_index] = 0;
}




//~ File IO

internal char *
read_entire_file_and_null_terminate(Memory_Arena *arena, char *file_name)
{
    char *result = 0;
    
    FILE *file = fopen(file_name, "rb");
    if(file)
    {
        fseek(file, 0, SEEK_END);
        size_t file_size = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        result = Push_Array(arena, file_size + 1, char); 
        fread(result, file_size, 1, file);
        result[file_size] = 0;
        
        fclose(file);
    }
    else
    {
        printf("Failed to read from file: %s\n", file_name);
        exit(1);
    }
    
    return result;
}

