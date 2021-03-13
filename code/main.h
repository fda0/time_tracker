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
    b32 colors_disabled;
};
global Global_State global_state;


enum Color_Code
{
    Color_Empty,
    Color_Reset,
    
    Color_Base,
    Color_Dimmed,
    
    Color_Date,
    Color_AltDate,
    
    Color_Description,
    Color_AltDescription,
    
    Color_Positive,
    Color_AltPositive,
    
    Color_Negative,
    Color_AltNegative,
    
    Color_Error,
    Color_Warning,
    
    Color_HelpHeader,
    Color_Bar,
    
    Color_Count
};

struct Color_Pair
{
    Color_Code code_check; // because C++ sucks!
    char *value;
};

global Color_Pair color_pairs[Color_Count] = {
    {Color_Empty, ""},
    {Color_Reset, "\033[39m\033[49m"},
    
    {Color_Base, "\033[97m"},
    {Color_Dimmed, "\033[90m"},
    
    {Color_Date, "\033[33m"},
    {Color_AltDate, "\033[43m\033[30m"},
    
    {Color_Description, "\033[36m"},
    {Color_AltDescription, "\033[96m"},
    
    {Color_Positive, "\033[32m"},
    {Color_AltPositive, "\033[92m"},
    
    {Color_Negative, "\033[31m"},
    {Color_AltNegative, "\033[91m"},
    
    {Color_Error, "\033[41m\033[97m"},
    {Color_Warning, "\033[103m\033[30m"},
    
    {Color_HelpHeader, "\033[100m"},
    {Color_Bar, "\033[34m"},
};





//~ NOTE: Platforms and compilers
struct Thread_Memory
{
    char input_buffer[256];
    char cursor[64];
    b32 new_data;
};


#if Def_Windows
#include "win32_platform.cpp"
#else
#error "system not defined"
#endif





//~ NOTE: Macros
#define Days(Value) (Hours(Value) * 24)
#define Hours(Value) (Minutes(Value) * 60)
#define Minutes(Value) (Value * 60)




//~ NOTE: Structs
struct Record_Range
{
    date64 date;
    u64 first;
    u64 one_past_last;
    u64 next_day_start_index;
};

struct Range_u64
{
    u64 first;
    u64 one_past_last;
};


enum Missing_Ending
{
    MissingEnding_None,
    MissingEnding_Assumed,
    MissingEnding_Critical
};


enum Record_Type
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
    s32 value; // 8
    date64 date; // 16
    //u64 desc_hash;
    String desc; // 32
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
    Token current_command_token;
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
    String title;
    Path exe_path;
    Path input_path;
    Directory archive_dir;

    File_Time input_file_mod_time;
    
    b32 load_file_error;
    Program_Scope initial_scope;
    
    Time32ms last_input_time;
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
