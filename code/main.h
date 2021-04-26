#include "stf0.h"
#include "description.h"
#include "lexer.h"
#include "win32_platform.h"
#include "print.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


typedef time_t date64;
typedef s32 time32;



//~ NOTE: Platforms and compilers
struct Thread_Memory
{
    char input_buffer[256];
    char cursor[64];
    b32 new_data;
};


//~ NOTE: Macros
#define Days(Value) (Hours(Value) * 24)
#define Hours(Value) (Minutes(Value) * 60)
#define Minutes(Value) (Value * 60)




//~ NOTE: Structs
enum Granularity
{
    Granularity_Days,
    // Granularity_Weeks,
    Granularity_Months,
    Granularity_Quarters,
    Granularity_Years
};


struct Stubs
{
    Description description;
};

struct Global_State
{
    date64 timezone_offset;
    b32 colors_disabled;
};

struct Color_Pair
{
    Color_Code code_check; // because C++ sucks!
    char *value;
};

enum Process_Days_Flags
{
    ProcessDays_Print = (1 << 0),
    ProcessDays_AltColor = (1 << 1),
};

struct Process_Days_Result
{
    s32 time_total;
    s32 time_assumed;
    
    u64 next_day_record_index;
};

struct Record_Range
{
    date64 date;
    u64 first;
    u64 one_past_last;
    u64 next_day_record_index;
};

struct Range_u64
{
    u64 first;
    u64 one_past_last;
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
    Token current_command_token;
    u32 change_count;
    u32 add_records_call_count;
    b32 no_errors;
    b32 reading_from_file;
    b32 load_file_unresolved_errors;
    
    // unwind backup
    Program_Scope scope;
};



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




enum Status
{
    Status_NoMatchigTokens,
    Status_HasErrors,
    Status_Valid,
};

inline b32
is_valid(Status status)
{
    b32 result = (status == Status_Valid);
    return result;
}



struct Date_Range_Result
{
    union {
        date64 date_ranges[2];
        struct
        {
            date64 first;
            date64 last; // NOTE: Inclusive end
        };
    };
    
    Status status;
};



inline void session_set_error(Record_Session *session, char *message);
internal void load_file(Program_State *state);

//~
#include "parse.h"

