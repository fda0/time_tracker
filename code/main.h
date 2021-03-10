#include "stf0.h"
#include "description.h"
#include "token.h"


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


typedef time_t date64;
typedef s32 time32;


struct Stubs
{
    Description description;
};
global Stubs global_stubs = {};

struct Global_State
{
    date64 timezone_offset;
};
global Global_State global_state;




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



#define Days(Value) (Hours(Value) * 24)
#define Hours(Value) (Minutes(Value) * 60)
#define Minutes(Value) (Value * 60)


//~ NOTE: Data types

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
    Record_Type type;
    s32 value;
    date64 date;
    //u64 desc_hash;
    String desc;
};


struct Program_Scope
{
    Arena_Scope arena_scope;
    Virtual_Array_Scope<Record> records_scope;
};


struct Record_Session
{
    Virtual_Array<Record> *records;
    Record *last;
    Record *active;
    
    Lexer lexer;
    u32 change_count;
    b32 no_errors;
    b32 reading_from_file;
    b32 load_file_unresolved_errors;
    
    // unwind backup
    Program_Scope scope;
};



// TODO: Pull out char[MAX_PATH] to StrMaxPath?

struct Program_State
{
    // NOTE: These require special initialization:
    Arena arena;
    Virtual_Array<Record> records;
    Description_Table desc_table;
    
    
    // NOTE: These can be zero initialized
    char archive_directory[MAX_PATH];
    String title;
    Path exe_path;
    Path input_path;
    Directory archive_dir;

    File_Time input_file_mod_time;
    
    b32 load_file_error;
    Program_Scope initial_scope;
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
