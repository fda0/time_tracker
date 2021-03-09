#include "stf0.h"
#include "description.h"
#include "token.h"


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


struct Stubs
{
    Description description;
};
global Stubs global_stubs = {};



typedef time_t date64;
typedef s32 time32;



struct Thread_Memory
{
    char input_buffer[256];
    char cursor[64];
    b32 new_data;
};



//~ NOTE: Platforms and compilers


#if Def_Windows
#include "win32_platform.cpp"
#else
#error "system not defined"
#endif



//~ NOTE: Macros

#define Macro_Wrap(Macro) \
    do                    \
    {                     \
        Macro             \
    } while (0)

#if BUILD_INTERNAL
#define Assert(Expression)          \
    Macro_Wrap(if (!(Expression)) { \
        __debugbreak();             \
        *(int *)0 = 1;              \
    })
#else
#define Assert(Expression)
#endif

#define Invalid_Code_Path Assert(0)



#define Array_Count(Array) (sizeof(Array) / sizeof((Array)[0]))

#define Minimum(A, B) (A > B ? B : A)

#define Kilobytes(Value) ((Value)*1024ULL)
#define Megabytes(Value) (Kilobytes(Value) * 1024ULL)
#define Gigabytes(Value) (Megabytes(Value) * (u64)1024ULL)
#define Terabytes(Value) (Gigabytes(Value) * (u64)1024ULL)

#define Days(Value) (Hours(Value) * 24)
#define Hours(Value) (Minutes(Value) * 60)
#define Minutes(Value) (Value * 60)


//~ NOTE: Data types

struct File_Path2
{
    char file_name[MAX_PATH];
    char directory[MAX_PATH];
};



enum Missing_Ending
{
    MissingEnding_None,
    MissingEnding_Assumed,
    MissingEnding_Critical
};


enum Record_Type : s32
{
    Record_Empty,
    Record_TimeStart,
    Record_TimeStop,
    Record_TimeDelta,
    Record_CountDelta
};

struct Record
{
    date64 date;
    u64 desc_hash;

    Record_Type type;
    s32 value;

};



#include "memory.h"



// TODO: Pull out char[MAX_PATH] to StrMaxPath?

struct Program_State
{
    Arena arena;
    Virtual_Array<Record> records;
    Description_Table desc_table;
    
    char archive_directory[MAX_PATH];
    String title;
    Path exe_path;
    Path input_path;
    Directory archive_dir;
    //char input_file_name[MAX_PATH];
    //char input_file_full_path[MAX_PATH]; // TODO: Collapse to File_Path2

    File_Time input_file_mod_time;
    date64 timezone_offset;

    s32 parse_error_count;
    s32 change_count;

    b32 reading_from_file;
};


enum Condition
{
    Con_NoMatchigTokens,
    Con_HasErrors,
    Con_IsValid,
};

inline b32
is_condition_valid(Condition condition)
{
    b32 result = (condition == Con_IsValid);
    return result;
}

struct Parse_Date_Result
{
    date64 date;
    b32 is_valid;
};

struct Parse_Complex_Date_Result
{
    date64 date;
    Condition condition;
};

struct Parse_Time_Result
{
    s32 time;
    b32 is_valid;
};

struct Parse_Number_Result
{
    s32 number;
    b32 is_valid;
};



struct Date_Range_Result
{
    union {
        date64 date_ranges[2];
        struct
        {
            date64 begin;
            date64 end; // NOTE: Inclusive end
        };
    };
    Condition condition;
};
