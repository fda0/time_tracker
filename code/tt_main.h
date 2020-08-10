#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

//
// NOTE(mateusz): Types
//

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef s32 b32;
typedef s32 b32x;

typedef float f32;
typedef double f64;

#define internal static 
#define global_variable static
#define local_persist static




struct Thread_Memory
{
    char input_buffer[256];
    char cursor[64];
    b32 new_data;
};

namespace Global_Color
{
    global_variable char *f_black = "\033[30m";
    global_variable char *f_date = "\033[33m";
    global_variable char *f_sum = "\033[32m";
    global_variable char *f_desc = "\033[36m";
    global_variable char *f_reset = "\033[39m";
    
    global_variable char *b_error = "\033[41m";
    global_variable char *b_date = "\033[43m";
    global_variable char *b_reset = "\033[49m";
};



//
// NOTE(mautesz): Platforms and compilers
//


#define COMPILER_MSVC 1

#if COMPILER_MSVC
#include "win32_platform.cpp"
#endif


#define DEBUG_COMPILATION 1






//
// NOTE(mateusz): Macros
//


#if DEBUG_COMPILATION

#define Assert(Expression) \
if(!(Expression)) {*(int*)0 = 1;}

#else
#define Assert(Expression)
#endif

#define Invalid_Code_Path Assert(0)




#define Array_Count(Array) (sizeof(Array) / sizeof((Array)[0]))

#define Minimum(A, B) (A > B ? B : A)

#define Kilobytes(Value) ((Value) * 1024)
#define Megabytes(Value) (Kilobytes(Value) * 1024)
#define Gigabytes(Value) (Megabytes(Value) * (u64)1024)
#define Terabytes(Value) (Gigabytes(Value) * (u64)1024)

#define Days(Value) (Hours(Value) * 24)
#define Hours(Value) (Minutes(Value) * 60)
#define Minutes(Value) (Value * 60)





//
// NOTE(mateusz): Data types
//

enum Entry_Type
{
    Entry_None,
    
    Entry_Start,
    Entry_Stop,
    Entry_Subtract,
    Entry_Add,
};

struct Time_Entry
{
    Entry_Type type;
    time_t date;
    time_t time;
    char *description;
    Time_Entry *next_in_day;
};

enum Missing_Type
{
    Missing_None,
    Missing_Assumed,
    Missing_Critical
};

struct Day
{
    time_t date_start;
    s32 sum;
    Missing_Type missing;
    Time_Entry first_time_entry;
};



#include "tt_memory.h"

struct Program_State
{
    Memory_Arena element_arena;
    Memory_Arena day_arena;
    
    char input_file_full_path[MAX_PATH];
    char input_filename[MAX_PATH];
    char archive_directory[MAX_PATH];
    
    time_t timezone_offset;
    File_Time loaded_input_mod_time;
    
    s32 save_error_count;
    s32 load_error_count;
    s32 change_count;
};



struct Parse_Time_Result
{
    time_t time;
    b32 success;
};

struct Parse_Number_Result
{
    s32 time;
    b32 success;
};


enum Instruction_Type
{
    Ins_None,
    Ins_Unsupported,
    
    Ins_Time_Entry,
    Ins_Show,
    Ins_Save,
    Ins_Archive,
    Ins_Load,
    Ins_Time,
    Ins_Edit,
    Ins_Clear,
    Ins_Help,
    
    Ins_Exit
};

enum Instruction_Flag : u32
{
    Flag_No_Save = (1 << 0),
};


enum Show_Input
{
    ShowInput_One_Day,
    ShowInput_From,
    ShowInput_To
};

struct Processing_Data
{
    Instruction_Type instruction;
    u32 flags;
    Show_Input show_input;
    
    Entry_Type type;
    time_t date;
    time_t date_from;
    time_t date_to;
    time_t time;
    char *description;
    
};



