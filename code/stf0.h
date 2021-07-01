#if !defined(Stf0_H)
#define Stf0_H
/* ======================= #define guide ======================
Switches that can be defined manually:
	> Def_Internal:
		0 - Build for public relrease
		1 - Build for developers only
	> Def_Slow:
		0 - No slow code allowed!
		1 - Slow code is welcomed (example: additional asserts)
    > Stf0_Namespace - wrap this whole code in stf0:: c++ namepsace
    > Stf0_Wide_Char - add functions for interfacing with UTF16 for Windows

Semi automatic switches (but can be specified manually):
    > Def_Windows; Def_Linux - target platforms
    > Def_Msvc; Dev_Llvm     - target compiler
*/

/* TODO:
  [ ] Compile with unicode switch to find all cases where Windows ASCII API is not used
  [ ] Base string builders on fill versions build_to_buffer(buffer, buffer_size)
*/

#define f0_start_template(...)
#define f0_end_template(...)
#define f0_include(...)


//=============================================================
#if !defined(Stf0_Level)
#define Stf0_Level 9999 // Specify this to include less code
#endif

//=============================================================
#if !defined(Stf0_Namespace)
#define Stf0_Namespace 0
#endif
#if !defined(Def_Log)
#define Def_Log 0
#endif
//=====
#if Stf0_Namespace && Stf0_Level >= 20
#define Stf0_Open_Namespace namespace stf0 {
#define Stf0_Close_Namespace };
#else
#define Stf0_Open_Namespace
#define Stf0_Close_Namespace
#endif





#pragma warning(push, 0)
// =========================== Types ==========================
#if Stf0_Level >= 10
#    include <inttypes.h>
#    include <stdint.h>
#    include <limits.h>
#    include <float.h>
#    include <xmmintrin.h>
#    include <emmintrin.h>
#endif

// =========================== Basic ==========================
#if Stf0_Level >= 20
#if !defined(Def_Internal)
#    define Def_Internal 0
#endif
#if !defined(Def_Slow)
#    define Def_Slow 0
#endif
// ======== Detect platform =======
#if !defined(Def_Windows)
#    define Def_Windows 0
#endif
#if !defined(Def_Linux)
#    define Def_Linux 0
#endif
// ====== Platform not found ======
#if !Def_Windows && !Def_Linux
// TODO(f0): Check standard switches to automatically and deduce them and make compiling easier if possible
//           And support different compilers (cl, clang, gcc)
#error "Define Def_Windows or Def_Linux"
#endif
// ======== Detect compiler =======
#if !defined(Def_Compiler_Msvc)
#    define Def_Compiler_Msvc 0
#endif
#if !defined(Def_Compiler_Llvm)
#    define Def_Compiler_Llvm 0
#endif
// ====== Compiler not found ======
#if !Def_Compiler_Msvc && !Def_Compiler_Llvm
#    if _MSC_VER
#        undef Def_Compiler_Msvc
#        define Def_Compiler_Msvc 1
#    else
// TODO(f0): More compilers
#        undef Def_Compiler_Llvm
#        define Def_Compiler_Llvm 1
#        endif
#endif
//=============================
#include <stdio.h>
#include <stdlib.h>
//=============================
#    if Def_Compiler_Msvc
#        include <intrin.h>
#    elif Def_Compiler_Llvm
#        include <x86intrin.h>
#    else
#        error "not impl; SSE/NEON optimizations?"
#    endif
#endif

// ========================== Memory ==========================
#if Stf0_Level >= 30

#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#    include <timeapi.h>
#    include <shellapi.h>
//#    include <debugapi.h>

// Microsoft's wall of shame:
#undef near
#undef interface
#undef RELATIVE
#undef ABSOLUTE

#endif

// =========================== Alloc ==========================
#if Stf0_Level >= 40
#    include <stdarg.h>
#endif

// ========================= Platform =========================
#if Stf0_Level >= 50
#    pragma comment(lib, "winmm.lib")
#    pragma comment(lib, "shell32.lib")
#endif

#pragma warning(pop)






//~ ======================= @Level_Types ======================
#if Stf0_Level >= 10
//~ ======================== @Level_10 ========================
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t b32;
typedef float f32;
typedef double f64;
//
typedef __m128 m128;
typedef __m128i m128i;
//=============
#define S8_Min _I8_MIN
#define S8_Max _I8_MAX
#define S16_Min _I16_MIN
#define S16_Max _I16_MAX
#define S32_Min _I32_MIN
#define S32_Max _I32_MAX
#define S64_Min _I64_MIN
#define S64_Max _I64_MAX
//
#define U8_Max _UI8_MAX
#define U16_Max _UI16_MAX
#define U32_Max _UI32_MAX
#define U64_Max _UI64_MAX
//
#define F32_Max FLT_MAX
#define F64_Max DBL_MAX
//
#define Pi32  3.14159265359f
#define Tau32 6.2831853071795864769f
//=============
#define internal static
#define function static
#define local_global static
#define global static
#define global_const static const
#define force_inline __forceinline
//=============
#define array_count(a) ((sizeof(a))/(sizeof(*a)))
#define pick_smaller(a, b) (((a) > (b)) ? (b) : (a))
#define pick_bigger(a, b) (((a) > (b)) ? (a) : (b))
//
#define offset_of(Type, Member) ((s64)&(((Type *)0)->Member))
//
#define kilobytes(b) (1024*(b))
#define megabytes(b) (1024*kilobytes(b))
#define gigabytes(b) ((s64)1024*megabytes(b))
#define terabytes(b) ((s64)1024*gigabytes(b))
//
#define glue_(a, b) a ## b
#define glue(a, b) glue_(a, b)
#define stringify(a) #a
#define stringify2(a) stringify(a)
#define stringify_macro(a) stringify(a)
//
#define for_range(Type, I, Range) for (Type I = 0; I < (Range); ++I)
#define for_array(I, Array) for_range(u64, I, array_count(Array))
#define for_linked_list(Node, List) for (auto Node = (List).first; Node; Node = Node->next)
#define for_linked_list_ptr(Node, List) for (auto Node = (List)->first; Node; Node = Node->next)
#define for_u64(I, Range) for_range(u64, I, Range)
#define for_s64(I, Range) for_range(s64, I, Range)
#define for_u32(I, Range) for_range(u32, I, Range)
#define for_s32(I, Range) for_range(s32, I, Range)
//
#define u32_from_pointer(Pointer) ((u32)(u64)(Pointer))
#define pointer_from_u32(Type, Value) ((Type *)((u64)Value))
//
// NOTE(f0): Align bits needs to be power of 2
#define align_bin_to(Value, AlignBits) ((Value + (AlignBits-1)) & ~(AlignBits-1))
#define align4(Value) align_bin_to(Value, 4)
#define align8(Value) align_bin_to(Value, 8)
#define align16(Value) align_bin_to(Value, 16)
//=============
#define square_m128(value) _mm_mul_ps((value), (value))
#define f32_from_m128(wide, index) ((f32 *)&(wide))[index]
#define u32_from_m128i(wide, index) ((u32 *)&(wide))[index]
#define s32_from_m128i(wide, index) ((s32 *)&(wide))[index]




//================= Doubly linked list macros =================
#define for_dll_NP(Item, SentinelPtr, NextName) \
for(auto Item = (SentinelPtr)->NextName; Item != (SentinelPtr); Item = Item->NextName)

#define for_dll(Item, SentinelPtr) for_dll_NP(Item, SentinelPtr, next)
#define for_dll_Panel(Item, SentinelPtr) for_dll_NP(Item, SentinelPtr, panel_next)

//-
#define dll_initialize_sentinel_NP(Sentinel, NextName, PrevName) (Sentinel).NextName = (Sentinel).PrevName = &(Sentinel)
#define dll_initialize_sentinel(Sentinel) dll_initialize_sentinel_NP(Sentinel, next, prev)

//-
#define dll_insert_after_NP(Parent, Ptr, NextName, PrevName) do{ \
(Ptr)->NextName = (Parent)->NextName;                            \
(Ptr)->PrevName = (Parent);                                      \
(Ptr)->NextName->PrevName = (Ptr);                               \
(Ptr)->PrevName->NextName = (Ptr);                               \
}while(0)

#define dll_insert_after(Parent, Ptr) dll_insert_after_NP(Parent, Ptr, next, prev)

//-
#define dll_insert_before_NP(Parent, Ptr, NextName, PrevName) do{ \
(Ptr)->NextName = (Parent);                                       \
(Ptr)->PrevName = (Parent)->PrevName;                             \
(Ptr)->NextName->PrevName = (Ptr);                                \
(Ptr)->prev->NextName = (Ptr);                                    \
}while(0)

#define dll_insert_before(Parent, Ptr) dll_insert_before_NP(Parent, Ptr, next, prev)

//-
#define dll_remove_NP(Ptr, NextName, PrevName) do{ \
(Ptr)->NextName->PrevName = (Ptr)->PrevName;       \
(Ptr)->PrevName->NextName = (Ptr)->NextName;       \
}while(0)

#define dll_remove(Ptr) dll_remove_NP(Ptr, next, prev)



//
// ========================= CONSTANTS ========================
//
global_const u32 bitmask_1  = 0x00000001;
global_const u32 bitmask_2  = 0x00000003;
global_const u32 bitmask_3  = 0x00000007;
global_const u32 bitmask_4  = 0x0000000f;
global_const u32 bitmask_5  = 0x0000001f;
global_const u32 bitmask_6  = 0x0000003f;
global_const u32 bitmask_7  = 0x0000007f;
global_const u32 bitmask_8  = 0x000000ff;
global_const u32 bitmask_9  = 0x000001ff;
global_const u32 bitmask_10 = 0x000003ff;
global_const u32 bitmask_11 = 0x000007ff;
global_const u32 bitmask_12 = 0x00000fff;
global_const u32 bitmask_13 = 0x00001fff;
global_const u32 bitmask_14 = 0x00003fff;
global_const u32 bitmask_15 = 0x00007fff;
global_const u32 bitmask_16 = 0x0000ffff;
global_const u32 bitmask_17 = 0x0001ffff;
global_const u32 bitmask_18 = 0x0003ffff;
global_const u32 bitmask_19 = 0x0007ffff;
global_const u32 bitmask_20 = 0x000fffff;
global_const u32 bitmask_21 = 0x001fffff;
global_const u32 bitmask_22 = 0x003fffff;
global_const u32 bitmask_23 = 0x007fffff;
global_const u32 bitmask_24 = 0x00ffffff;
global_const u32 bitmask_25 = 0x01ffffff;
global_const u32 bitmask_26 = 0x03ffffff;
global_const u32 bitmask_27 = 0x07ffffff;
global_const u32 bitmask_28 = 0x0fffffff;
global_const u32 bitmask_29 = 0x1fffffff;
global_const u32 bitmask_30 = 0x3fffffff;
global_const u32 bitmask_31 = 0x7fffffff;

global_const u32 bit_1  = 0x00000001;
global_const u32 bit_2  = 0x00000002;
global_const u32 bit_3  = 0x00000004;
global_const u32 bit_4  = 0x00000008;
global_const u32 bit_5  = 0x00000010;
global_const u32 bit_6  = 0x00000020;
global_const u32 bit_7  = 0x00000040;
global_const u32 bit_8  = 0x00000080;
global_const u32 bit_9  = 0x00000100;
global_const u32 bit_10 = 0x00000200;
global_const u32 bit_11 = 0x00000400;
global_const u32 bit_12 = 0x00000800;
global_const u32 bit_13 = 0x00001000;
global_const u32 bit_14 = 0x00002000;
global_const u32 bit_15 = 0x00004000;
global_const u32 bit_16 = 0x00008000;
global_const u32 bit_17 = 0x00010000;
global_const u32 bit_18 = 0x00020000;
global_const u32 bit_19 = 0x00040000;
global_const u32 bit_20 = 0x00080000;
global_const u32 bit_21 = 0x00100000;
global_const u32 bit_22 = 0x00200000;
global_const u32 bit_23 = 0x00400000;
global_const u32 bit_24 = 0x00800000;
global_const u32 bit_25 = 0x01000000;
global_const u32 bit_26 = 0x02000000;
global_const u32 bit_27 = 0x04000000;
global_const u32 bit_28 = 0x08000000;
global_const u32 bit_29 = 0x10000000;
global_const u32 bit_30 = 0x20000000;
global_const u32 bit_31 = 0x40000000;
global_const u32 bit_32 = 0x80000000;






//~ ======================= @Level_Basic ======================
#if Stf0_Level >= 20
//~ ======================== @Level_20 ========================
Stf0_Open_Namespace

//=============================
#if Def_Windows
#define Native_Slash_Char '\\'
#define Native_Slash_Str "\\"
#elif Def_Linux
#define Native_Slash_Char '/'
#define Native_Slash_Str "/"
#endif



//=============================
#if Def_Compiler_Msvc
#    define This_Function __func__
#    define This_Line_S32 __LINE__
#    define This_File     __FILE__
#    define Counter_Macro __COUNTER__
#else
#    error "not impl"
#endif



//=============================
#define File_Line          This_File "(" stringify2(This_Line_S32) ")"
#define File_Line_Function File_Line ": " This_Function



//=============================
#define force_halt() do{ fflush(stdout); *((s32 *)0) = 1; }while(0)

#define assert_always(Expression) do{ if(!(Expression)) {\
printf("\n" File_Line ": RUNTIME error: assert(%s) in function %s\n",\
stringify(Expression), This_Function);\
fflush(stdout);\
debug_break(); force_halt(); exit(1);\
}}while(0)



//=============================
#if Def_Slow
// TODO(f0): other compilers
#ifdef WIN32_LEAN_AND_MEAN
#    define debug_break() do{if(IsDebuggerPresent()) {fflush(stdout); __debugbreak();}}while(0)
#else
#    define debug_break() do{fflush(stdout); __debugbreak();}while(0)
#endif
#    define assert(Expression) assert_always(Expression)
#    define break_at(Expression) do{if((Expression)){debug_break();}}while(0)

#else
#    define debug_break()
#    define assert(Expression)
#    define break_at(Expression)
#endif

inline b32 static_expression_wrapper_(b32 a) { return a; }
#define runtime_assert(ExpressionMakeNotStatic) assert(static_expression_wrapper_(ExpressionMakeNotStatic))

#define assert_bounds(Index, Array) assert((Index) >= 0 && (Index) < array_count(Array))

#define exit_error() do{ fflush(stdout); debug_break(); exit(1);}while(0)




// ===================== Hacky C++11 defer ====================
template <typename F>
struct Private_Defer {
	F f;
	Private_Defer(F f) : f(f) {}
	~Private_Defer() { f(); }
};

template <typename F>
Private_Defer<F> private_defer_function(F f) {
	return Private_Defer<F>(f);
}

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x)    DEFER_2(x, __COUNTER__)
#define defer(code)   auto DEFER_3(_defer_) = private_defer_function([&](){code;})
//=============



// ======================= @Basic_Flags =======================

function b32
is_set(u32 flag, u32 bits)
{
    b32 result = (flag & bits) == bits;
    return result;
}


function void
set_flag(u32 *flag, u32 bits)
{
    *flag |= bits;
}

function void
clear_flag(u32 *flag, u32 bits)
{
    *flag &= (~bits);
}






// ======================= @Basic_Types =======================

// Time
struct Time32ms
{
    u32 t;
};

struct Time_Perfomance
{
    s64 t_; // NOTE(f0): retrieve with get_*seconds_elapsed(a, b)
};


// Strings
typedef const char* cstr_lit;

struct String
{
    u8 *str;
    u64 size;
};

struct Find_Index
{
    u64 index; // NOTE(f0): Index defaults to 0 when not found
    b32 found;
    u32 _padding;
};





// ====================== @Basic_Helpers ======================

inline u64
safe_truncate_to_u64(s64 value)
{
    assert(value >= 0);
    u64 result = (u64)value;
    return result;
}

////
inline u32 
safe_truncate_to_u32(u64 value)
{
	assert(value <= U32_Max);
    u32 result = (u32)value;
	return result;
}

inline u32 
safe_truncate_to_u32(s64 value)
{
	assert(value <= U32_Max);
    assert(value >= 0);
    u32 result = (u32)value;
	return result;
}

////
inline u16
safe_truncate_to_u16(u32 value)
{
    assert(value <= U16_Max);
    u16 result = (u16)value;
    return result;
}

inline u16
safe_truncate_to_u16(s32 value)
{
    assert(value <= U16_Max);
    assert(value >= 0);
    u16 result = (u16)value;
    return result;
}

////
inline u32 
safe_truncate_to_s32(s64 value)
{
	assert(value <= S32_Max);
    assert(value >= S32_Min);
    u32 result = (u32)value;
	return result;
}
















// ===================== @Basic_Intrinsics ====================

struct Bit_Scan_Result
{
    u32 index;
    b32 found;
};

inline Bit_Scan_Result
find_most_significant_bit(u64 value)
{
    Bit_Scan_Result result = {};
#if Def_Compiler_Msvc
    result.found = _BitScanReverse64((unsigned long *)&result.index, value);
#else
#error "not impl"
#endif
    return result;
}
inline Bit_Scan_Result find_most_significant_bit(s64 value) {
    return find_most_significant_bit((u64)value);
}

inline Bit_Scan_Result
find_most_significant_bit(u32 value)
{
    Bit_Scan_Result result = {};
#if Def_Compiler_Msvc
    result.found = _BitScanReverse((unsigned long *)&result.index, value);
#else
#error "not impl"
#endif
    return result;
}
inline Bit_Scan_Result find_most_significant_bit(s32 value) {
    return find_most_significant_bit((u32)value);
}

///////
inline Bit_Scan_Result
find_least_significant_bit(u32 value)
{
    Bit_Scan_Result result = {};
#if Def_Compiler_Msvc
    result.found = _BitScanForward((unsigned long *)&result.index, value);
    
#else
    for (u32 test = 0; test < 32; ++ test)
    {
        if (value & (1 << test))
        {
            result.index = test;
            result.found = true;
            break;
        }
    }
#endif
    return result;
}
inline Bit_Scan_Result find_least_significant_bit(s32 value) {
    return find_least_significant_bit((u32)value);
};


inline Bit_Scan_Result
find_least_significant_bit(u64 value)
{
    Bit_Scan_Result result = {};
    
#if Def_Compiler_Msvc
    result.found = _BitScanForward64((unsigned long *)&result.index, value);
#else
#error "not impl"
#endif
    return result;
}
inline Bit_Scan_Result find_least_significant_bit(s64 value) {
    return find_least_significant_bit((u64)value);
};








// ================= @Basic_Intrinsic_Helpers =================
inline b32
is_non_zero_power_of_two(u64 value)
{
    Bit_Scan_Result msb = find_most_significant_bit(value);
    Bit_Scan_Result lsb = find_least_significant_bit(value);
    b32 result = (msb.found == lsb.found &&
                  msb.index == lsb.index);
    return result;
}






// ======================= @Basic_Memory ======================

#define copy_array(Destination, Source, Type, Count) copy_bytes(Destination, Source, sizeof(Type)*(Count))

inline void
copy_bytes(void *destination, void *source, u64 number_of_bytes)
{
    u8 *d = (u8 *)destination;
    u8 *s = (u8 *)source;
    
    for (u64 i = 0; i < number_of_bytes; ++i)
    {
        *d++ = *s++;
    }
}


#define clear_array(DestinationPtr, Type, Count) clear_bytes(DestinationPtr, sizeof(Type)*(Count))
#define clear_struct(DestinationValue) clear_bytes(&DestinationValue, sizeof(DestinationValue))
#define clear_struct_ptr(DestinationValue) clear_bytes(DestinationValue, sizeof(*DestinationValue))

inline void
clear_bytes(void *destination, u64 number_of_bytes)
{
    u8 *d = (u8 *)destination;
    for (u64 i = 0; i < number_of_bytes; ++i)
    {
        *d++ = 0;
    }
}





// ================ @Basic_String_Constructors ================
// Macros
#define string_expand(Str) ((s32)((Str).size)), ((char *)((Str).str))
#define string_expand_rev(Str) ((char *)((Str).str)), ((Str).size)
#define lit2str(Literal) string(Literal, array_count(Literal)-1)
#define l2s(Literal) lit2str(Literal)



function u64
length(char *string)
{
    u64 length = 0;
    while (*string++) {
        ++length;
    }
    return length;
}
function u64
length(u8 *string)
{
    u64 length = 0;
    while (*string++) {
        ++length;
    }
    return length;
}



// Constructors
inline String
string(u8 *str, u64 size)
{
    String result = {str, size};
    return result;
}

inline String
string(char *str, u64 size)
{
    return string((u8 *)str, size);
}

inline String
string(char *cstr)
{
    String result = {
        (u8 *)cstr,
        length(cstr)
    };
    return result;
}





// ======================== @Basic_Char =======================
function b32 is_slash(u8 c) { return (c == '\\' || c == '/'); }
function b32 is_slash(char c) { return (c == '\\' || c == '/'); }
function b32 is_slash(u16 c) { return (c == '\\' || c == '/'); }
function b32 is_slash(u32 c) { return (c == '\\' || c == '/'); }

function b32 is_end_of_line(u8 c) { return ((c == '\n') || (c == '\r')); }
function b32 is_end_of_line(char c) { return ((c == '\n') || (c == '\r')); }
function b32 is_end_of_line(u16 c) { return ((c == '\n') || (c == '\r')); }
function b32 is_end_of_line(u32 c) { return ((c == '\n') || (c == '\r')); }

function b32 is_whitespace(u8 c) { return ((c == ' ') || (c == '\t') || (c == '\v') || (c == '\f')); }
function b32 is_whitespace(char c) { return ((c == ' ') || (c == '\t') || (c == '\v') || (c == '\f')); }
function b32 is_whitespace(u16 c) { return ((c == ' ') || (c == '\t') || (c == '\v') || (c == '\f')); }
function b32 is_whitespace(u32 c) { return ((c == ' ') || (c == '\t') || (c == '\v') || (c == '\f')); }

function b32 is_white(u8 c) { return (is_whitespace(c) || is_end_of_line(c)); }
function b32 is_white(char c) { return (is_whitespace(c) || is_end_of_line(c)); }
function b32 is_white(u16 c) { return (is_whitespace(c) || is_end_of_line(c)); }
function b32 is_white(u32 c) { return (is_whitespace(c) || is_end_of_line(c)); }

function b32 is_alpha(u8 c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
function b32 is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
function b32 is_alpha(u16 c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
function b32 is_alpha(u32 c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

function b32 is_number(u8 c) { return (c >= '0' && c <= '9'); }
function b32 is_number(char c) { return (c >= '0' && c <= '9'); }
function b32 is_number(u16 c) { return (c >= '0' && c <= '9'); }
function b32 is_number(u32 c) { return (c >= '0' && c <= '9'); }


function u8 get_lower(u8 c)
{
    if (c >= 'A' && c <= 'Z')
    {
        u8 delta = 'a' - 'A';
        c += delta;
    }
    return c;
}
function char get_lower(char c)
{
    if (c >= 'A' && c <= 'Z')
    {
        char delta = 'a' - 'A';
        c += delta;
    }
    return c;
}
function u16 get_lower(u16 c)
{
    if (c >= 'A' && c <= 'Z')
    {
        u16 delta = 'a' - 'A';
        c += delta;
    }
    return c;
}
function u32 get_lower(u32 c)
{
    if (c >= 'A' && c <= 'Z')
    {
        u32 delta = 'a' - 'A';
        c += delta;
    }
    return c;
}









// ======================== @Basic_Cstr =======================
function b32
equals(u8 *value_a, u8 *value_b, b32 case_ins = false)
{
    b32 result = false;
    for(;;)
    {
        u8 a = (*value_a++);
        u8 b = (*value_b++);
        
        if (case_ins) {
            a = get_lower(a);
            b = get_lower(b);
        }
        
        if (a != b) {
            break;
        } else if (a == 0) {
            result = true;
            break;
        }
    }
    return result;
}
function b32
equals(char *value_a, char *value_b, b32 case_ins = false)
{
    b32 result = false;
    for(;;)
    {
        char a = (*value_a++);
        char b = (*value_b++);
        
        if (case_ins) {
            a = get_lower(a);
            b = get_lower(b);
        }
        
        if (a != b) {
            break;
        } else if (a == 0) {
            result = true;
            break;
        }
    }
    return result;
}




function b32
starts_with(u8 *haystack, u8 *needle, b32 case_ins = false)
{
    b32 result = false;
    for(;;)
    {
        u8 h = (*haystack++);
        u8 n = (*needle++);
        
        if (case_ins) {
            h = get_lower(h);
            n = get_lower(n);
        }
        
        if (n == 0) {
            result = true;
            break;
        } else if (h == 0 || h != n) {
            break;
        }
    }
    return result;
}
function b32
starts_with(char *haystack, char *needle, b32 case_ins = false)
{
    b32 result = false;
    for(;;)
    {
        char h = (*haystack++);
        char n = (*needle++);
        
        if (case_ins) {
            h = get_lower(h);
            n = get_lower(n);
        }
        
        if (n == 0) {
            result = true;
            break;
        } else if (h == 0 || h != n) {
            break;
        }
    }
    return result;
}


function b32
ends_with(u8 *haystack, u8 *needle, b32 case_ins = false)
{
    u64 haystack_len = length(haystack);
    u64 needle_len = length(needle);
    
    for (u64 i = 1; i <= needle_len; ++i)
    {
        u8 h = haystack[haystack_len - i];
        u8 n = needle[needle_len - i];
        
        if (case_ins) {
            h = get_lower(h);
            n = get_lower(n);
        }
        
        if (h != n) {
            return false;
        }
    }
    
    return true;
}
function b32
ends_with(char *haystack, char *needle, b32 case_ins = false)
{
    u64 haystack_len = length(haystack);
    u64 needle_len = length(needle);
    
    for (u64 i = 1; i <= needle_len; ++i)
    {
        char h = haystack[haystack_len - i];
        char n = needle[needle_len - i];
        
        if (case_ins) {
            h = get_lower(h);
            n = get_lower(n);
        }
        
        if (h != n) {
            return false;
        }
    }
    
    return true;
}




function Find_Index
index_of(u8 *value, u8 c)
{
    Find_Index result = {};
    for (u64 index = 0;
         ;
         ++index)
    {
        u8 v = value[index];
        
        if (v == c)
        {
            result.index = index;
            result.found = true;
            break;
        }
        else if (!v)
        {
            break;
        }
    }
    return result;
}
function Find_Index
index_of(char *value, char c)
{
    Find_Index result = {};
    for (u64 index = 0;
         ;
         ++index)
    {
        char v = value[index];
        
        if (v == c)
        {
            result.index = index;
            result.found = true;
            break;
        }
        else if (!v)
        {
            break;
        }
    }
    return result;
}



function Find_Index
index_of_difference(u8 *value_a, u8 *value_b, b32 case_ins = false)
{
    Find_Index result = {};
    
    for (u64 diff_index = 0;
         ; 
         ++diff_index)
    {
        u8 a = *value_a++;
        u8 b = *value_b++;
        
        if (case_ins) {
            a = get_lower(a);
            b = get_lower(b);
        }
        
        if (a != b)
        {
            result.index = diff_index;
            result.found = true;
            break;
        }
        
        if (a == 0) {
            break;
        }
    }
    
    return result;
}
function Find_Index
index_of_difference(char *value_a, char *value_b, b32 case_ins = false)
{
    Find_Index result = {};
    
    for (u64 diff_index = 0;
         ; 
         ++diff_index)
    {
        char a = *value_a++;
        char b = *value_b++;
        
        if (case_ins) {
            a = get_lower(a);
            b = get_lower(b);
        }
        
        if (a != b)
        {
            result.index = diff_index;
            result.found = true;
            break;
        }
        
        if (a == 0) {
            break;
        }
    }
    
    return result;
}





function Find_Index
index_of_from_table(u8 *value, String character_table)
{
    Find_Index result = {};
    
    for (u64 value_index = 0;
         ;
         ++value_index)
    {
        u8 v = value[value_index];
        
        for (u64 search_index = 0;
             search_index < character_table.size;
             ++search_index)
        {
            u8 s = (u8)character_table.str[search_index];
            if (v == s)
            {
                result.index = value_index;
                result.found = true;
                goto exit_break_label;
            }
        }
        
        if (!v) {
            break;
        }
    }
    
    exit_break_label:
    return result;
}
function Find_Index
index_of_from_table(char *value, String character_table)
{
    Find_Index result = {};
    
    for (u64 value_index = 0;
         ;
         ++value_index)
    {
        char v = value[value_index];
        
        for (u64 search_index = 0;
             search_index < character_table.size;
             ++search_index)
        {
            char s = (char)character_table.str[search_index];
            if (v == s)
            {
                result.index = value_index;
                result.found = true;
                goto exit_break_label;
            }
        }
        
        if (!v) {
            break;
        }
    }
    
    exit_break_label:
    return result;
}




function u64
length_trim_white_reverse(u8 *cstr)
{
    u64 len = length(cstr);
    for (; len > 0; --len)
    {
        if (!is_white(cstr[len-1]))
        {
            break;
        }
    }
    return len;
}
function u64
length_trim_white_reverse(char *cstr)
{
    u64 len = length(cstr);
    for (; len > 0; --len)
    {
        if (!is_white(cstr[len-1]))
        {
            break;
        }
    }
    return len;
}











// ======================= @Basic_String ======================

function String
string_prefix(String text, u64 size)
{
    size = pick_smaller(size, text.size);
    text.size = size;
    return text;
}

////////////////////////////////
function String
string_postfix(String text, u64 size)
{
    size = pick_smaller(text.size, size);
    u64 distance = text.size - size;
    
    text.str = text.str + distance;
    text.size = size;
    
    return text;
};

////////////////////////////////
function String
string_skip(String text, u64 distance)
{
    distance = pick_smaller(distance, text.size);
    String result = {};
    result.str = text.str + distance;
    result.size = text.size - distance;
    return result;
}
function String
string_advance(String text, u64 distance)
{
    return string_skip(text, distance);
}

////////////////////////////////
function String
string_chop(String text, u64 distance_from_end)
{
    if (distance_from_end > text.size)
    {
        text.size = 0;
    }
    else
    {
        text.size = text.size - distance_from_end;
    }
    return text;
}

////////////////////////////////
function String
string_substr(String text, u64 distance, u64 length)
{
    String result = string_skip(text, distance);
    result.size = pick_smaller(result.size, length);
    return result;
}








function b32
equals(String str_a, String str_b, b32 case_ins = false)
{
    b32 result = false;
    
    if (str_a.size == str_b.size)
    {
        result = true;
        for(u64 i = 0; i < str_a.size; ++i)
        {
            u8 a = str_a.str[i];
            u8 b = str_b.str[i];
            
            if (case_ins) {
                a = get_lower(a);
                b = get_lower(b);
            }
            
            if (a != b) {
                result = false;
                break;
            }
        }
    }
    
    return result;
}




function b32
starts_with(String haystack, String needle, b32 case_ins = false)
{
    b32 result = false;
    if (haystack.size >= needle.size)
    {
        result = true;
        for(u64 i = 0; i < needle.size; ++i)
        {
            u8 h = haystack.str[i];
            u8 n = needle.str[i];
            
            if (case_ins) {
                h = get_lower(h);
                n = get_lower(n);
            }
            
            if (h != n) {
                result = false;
                break;
            }
        }
    }
    
    return result;
}


function b32
ends_with(String haystack, String needle, b32 case_ins = false)
{
    b32 result = false;
    if (haystack.size >= needle.size)
    {
        result = true;
        for(u64 i = 1; i <= needle.size; ++i)
        {
            u8 h = haystack.str[haystack.size - i];
            u8 n = needle.str[needle.size - i];
            
            if (case_ins) {
                h = get_lower(h);
                n = get_lower(n);
            }
            
            if (h != n) {
                result = false;
                break;
            }
        }
    }
    
    return result;
}











function Find_Index
index_of(String haystack, char needle, b32 case_ins = false)
{
    Find_Index result = {};
    
    if (case_ins) {
        needle = get_lower(needle);
    }
    
    for (u64 i = 0; i < haystack.size; ++i)
    {
        u8 h = haystack.str[i];
        if (case_ins) {
            h = get_lower(h);
        }
        
        if (h == needle) {
            result.index = i;
            result.found = true;
            break;
        }
    }
    return result;
}


function Find_Index
index_of(String haystack, String needle)
{
    // naive implementation
    // TODO(f0): better search impl
    Find_Index result = {};
    
    if (needle.size > 0)
    {
        for_u64(start_haystack_index, haystack.size)
        {
            u64 characters_left = haystack.size - start_haystack_index;
            if (characters_left < needle.size) { break; }
            
            u64 haystack_index = start_haystack_index;
            
            for (u64 needle_index = 0;
                 ;
                 ++needle_index, ++haystack_index)
            {
                if (needle_index == needle.size)
                {
                    result.index = haystack_index;
                    result.found = true;
                    return result;
                }
                
                u8 h = haystack.str[haystack_index];
                u8 n = needle.str[needle_index];
                
                if (h != n) {
                    break;
                }
            }
        }
    }
    
    return result;
}


function String
trim_white(String text)
{
    String result = text;
    for_u64(i, result.size)
    {
        u8 u = result.str[i];
        if (!is_white(u))
        {
            result = string_advance(result, i);
            break;
        }
    }
    
    for_u64(i, result.size)
    {
        u64 inverse_index = result.size - i - 1;
        u8 u = result.str[inverse_index];
        if (!is_white(u))
        {
            result.size = inverse_index + 1;
            break;
        }
    }
    
    return result;
}

function String
trim_from_index_of_reverse(String haystack, char needle)
{
    String result = haystack;
    
    for (u64 negative = 1;
         negative <= result.size;
         ++negative)
    {
        u64 index = result.size - negative;
        u8 h = result.str[index];
        
        if (h == needle) {
            result.size = index;
            break;
        }
    }
    
    return result;
}



function String
file_name_from_path(String source)
{
    String result = source;
    
    u64 offset = 0;
    
    for (u64 negative = 1;
         negative <= result.size;
         ++negative)
    {
        u64 index = result.size - negative;
        u8 c = result.str[index];
        if (is_slash(c))
        {
            offset = index + 1;
            break;
        }
    }
    
    result.str += offset;
    result.size -= offset;
    return result;
}


struct String_Path_Split
{
    String directory;
    String file_name;
};


function String_Path_Split
split_into_directory_and_file_name(String source)
{
    u64 offset = 0;
    
    for (u64 negative = 1;
         negative <= source.size;
         ++negative)
    {
        u64 index = source.size - negative;
        u8 c = source.str[index];
        
        if (is_slash(c))
        {
            offset = index + 1;
            break;
        }
    }
    
    String_Path_Split result = {};
    result.file_name = string_advance(source, offset);
    result.directory = source;
    result.directory.size = offset;
    return result;
}




function String
trim_file_name_from_path(String source)
{
    String result = source;
    
    for (u64 negative = 1;
         negative <= result.size;
         ++negative)
    {
        u64 index = result.size - negative;
        if (is_slash(result.str[index]))
        {
            result.size = index + 1;
            break;
        }
    }
    
    return result;
}



function u64
count_of(String haystack, String needle)
{
    u64 result = 0;
    
    for (u64 haystack_index = 0;
         haystack_index < haystack.size;
         ++haystack_index)
    {
        u8 h = haystack.str[haystack_index];
        
        for (u64 needle_index = 0;
             needle_index < needle.size;
             ++needle_index)
        {
            u8 n = needle.str[needle_index];
            
            if (h == n) {
                result += 1;
            }
        }
    }
    
    return result;
}






struct Compare_Line_Pos
{
    u32 line;
    u32 column;
    b32 is_equal;
};

function Compare_Line_Pos
compare_with_line_column(String value_a, String value_b, b32 case_ins = false)
{
    // TODO(f0): column counter that works with utf8
    
    Compare_Line_Pos result = {};
    result.is_equal = true;
    result.line = 1;
    result.column = 1;
    
    u64 size = pick_smaller(value_a.size, value_b.size);
    u64 i = 0;
    for (; i < size; ++i)
    {
        u8 a = value_a.str[i];
        u8 b = value_b.str[i];
        
        if (case_ins) {
            a = get_lower(a);
            b = get_lower(b);
        }
        
        if (a != b) {
            result.is_equal = false;
            break;
        }
        
        result.column += 1;
        
        if (a == '\n') {
            result.line += 1;
            result.column = 1;
        }
    }
    
    if (result.is_equal &&
        value_a.size != value_b.size)
    {
        result.is_equal = false;
    }
    
    return result;
}



struct Parse_S32_Result
{
    s32 value;
    b32 success;
};

function Parse_S32_Result
parse_string_to_s32(String text)
{
    Parse_S32_Result result = {};
    
    if (text.size > 0)
    {
        b32 has_minus = false;
        
        for (u64 i = 0;
             ;
             ++i)
        {
            if (i >= text.size) {
                result.success = true;
                break;
            }
            
            u8 u = text.str[i];
            
            if (is_number(u))
            {
                result.value *= 10;
                result.value += (u - '0');
            }
            else if (u == '-' && (i == 0))
            {
                has_minus = true;
            }
            else
            {
                break;
            }
        }
        
        if (has_minus) {
            result.value *= -1;
        }
    }
    
    return result;
}






//~ ======================= @Level_Math =======================
#if Stf0_Level >= 21
//~ ======================== @Level_21 ========================

//~ ===================== @Math_Intrinics =====================
// TODO(f0): Remove math.h in some beautiful future?
#include <math.h>

inline s32
sign_of(s32 value)
{
    s32 result = (value >= 0) ? 1 : -1;
    return result;
}

inline f32
sign_of(f32 value)
{
    f32 result = (value >= 0.f) ? 1.f : -1.f;
    return result;
}

inline f32
square_root(f32 real32)
{
    f32 result = sqrtf(real32);
    return result;
}

inline f32
absolute_value(f32 real32)
{
    f32 result = (f32)fabs(real32);
    return result;
}

inline u32
rotate_left(u32 value, s32 amount)
{
#if Def_Compiler_Msvc
    u32 result = _rotl(value, amount);
#else
#error this is slow
    amount &= 31;
    result = (value << amount) | (value >> (32 - amount));
#endif
    
    return result;
}

inline u32
rotate_right(u32 value, s32 amount)
{
#if Def_Compiler_Msvc
    u32 result = _rotr(value, amount);
#else
#error this is slow
    amount &= 31;
    u32 result = (value >> amount)  | (value << (32 - amount));
#endif
    
    return result;
}

inline f32
floor(f32 value)
{
    f32 result = floorf(value);
    return result;
}

inline f32
ceil(f32 value)
{
    f32 result = ceilf(value);
    return result;
}


inline s32
round_f32_to_s32(f32 value)
{
    s32 result = (s32)roundf(value);
    return result;
}

inline u32
round_f32_to_u32(f32 value)
{
    u32 result = (u32)roundf(value);
    return result;
}

inline s32
floor_f32_to_s32(f32 value)
{
    s32 result = (s32)floor(value);
    return result;
}

inline s32
ceil_f32_to_s32(f32 value)
{
    s32 result = (s32)ceil(value);
    return result;
}

inline s32
truncate_f32_to_s32(f32 value)
{
    s32 result = (s32)value;
    return result;
}

inline f32
sin(f32 angle)
{
    f32 result = sinf(angle);
    return result;
}

inline f32
cos(f32 angle)
{
    f32 result = cosf(angle);
    return result;
}

inline f32
atan2(f32 y, f32 x)
{
    f32 result = atan2f(y, x);
    return result;
}











//~ ======================= @Math_Types =======================
// ====================== Vectors (float) =====================
union v2
{
    struct {
        f32 x, y;
    };
    f32 e[2];
};
union v3
{
    struct {
        f32 x, y, z;
    };
    struct {
        v2 xy;
    };
    struct {
        f32 _x_;
        v2 yz;
    };
    f32 e[3];
};
union v4
{
    struct {
        f32 x, y, z, w;
    };
    struct {
        v3 xyz;
    };
    struct {
        v2 xy, zw;
    };
    struct {
        f32 r, g, b, a;
    };
    struct {
        v3 rgb;
    };
    f32 e[4];
};

// ==================== Rectangles (float) ====================
struct Rect2
{
    v2 min, max;
};
struct Rect3
{
    v3 min, max;
};


// ======================= Vectors (int) ======================
union v2s
{
    struct {
        s32 x, y;
    };
    s32 e[2];
};
// ===================== Rectangles (int) =====================
struct Rect2s
{
    v2s min, max;
};



//~ ======================== @s32_math ========================
inline s32
clamp(s32 min, s32 value, s32 max)
{
    s32 result = value;
    
    if (result < min) {
        result = min;
    }
    
    if (result > max) {
        result = max;
    }
    
    return result;
}

inline void
clamp_bot(s32 *value, s32 bound)
{
    if (*value < bound) {
        *value = bound;
    }
}

inline void
clamp_top(s32 *value, s32 bound)
{
    if (*value > bound) {
        *value = bound;
    }
}


//~ ======================== @s64_math ========================
inline void
clamp_bot(s64 *value, s64 bound)
{
    if (*value < bound) {
        *value = bound;
    }
}

inline void
clamp_top(s64 *value, s64 bound)
{
    if (*value > bound) {
        *value = bound;
    }
}





//~ ======================== @f32_math ========================
inline void
clamp_bot(f32 *value, f32 bound)
{
    if (*value < bound) {
        *value = bound;
    }
}

inline void
clamp_top(f32 *value, f32 bound)
{
    if (*value > bound) {
        *value = bound;
    }
}

inline f32
square(f32 a)
{
    f32 result = a * a;
    return result;
}

inline f32
lerp(f32 a, f32 b, f32 t)
{
    f32 result = (1.0f - t)*a + t*b;
    return result;
}

inline f32
safe_ratio_n(f32 numerator, f32 divisor, f32 n)
{
    f32 result = n;
    if (divisor != 0.0f)
    {
        result = numerator / divisor;
    }
    return result;
}

inline f32
safe_ratio_0(f32 numerator, f32 divisor)
{
    f32 result = safe_ratio_n(numerator, divisor, 0.0f);
    return result;
}

inline f32
safe_ratio_1(f32 numerator, f32 divisor)
{
    f32 result = safe_ratio_n(numerator, divisor, 1.0f);
    return result;
}

inline f32
clamp(f32 min, f32 value, f32 max)
{
    f32 result = value;
    if (result < min) {
        result = min;
    }
    // NOTE(f0): No else here produces branchless clamp
    if (result > max) {
        result = max;
    }
    return result;
}

inline f32
clamp01(f32 value)
{
    f32 result = clamp(0.0f, value, 1.0f);
    return result;
}

inline f32
clamp01_map_to_range(f32 min, f32 t, f32 max)
{
    f32 result = 0.f;
    
    f32 range = max - min;
    if (range != 0.0f)
    {
        result = clamp01((t - min) / range);
    }
    
    return result;
}





//~ ========================= @v2_math ========================
inline v2
arm2(f32 angle)
{
    v2 result = {cos(angle), sin(angle)};
    return result;
}

inline v2
perp(v2 a)
{
    v2 result = {-a.y, a.x};
    return result;
}

inline void
clamp_bot(v2 *value, v2 bound)
{
    clamp_bot(&value->x, bound.x);
    clamp_bot(&value->y, bound.y);
}

inline void
clamp_top(v2 *value, v2 bound)
{
    clamp_top(&value->x, bound.x);
    clamp_top(&value->y, bound.y);
}



inline v2
V2(f32 x, f32 y)
{
    v2 result = {x, y};
    return result;
}

inline v2
V2(f32 xy)
{
    v2 result = {xy, xy};
    return result;
}

inline v2
V2i(s32 x, s32 y)
{
    v2 result = {(f32)x, (f32)y};
    return result;
}

inline v2
V2i(u32 x, u32 y)
{
    v2 result = {(f32)x, (f32)y};
    return result;
}

inline v2
V2i(s32 xy)
{
    v2 result = {(f32)xy, (f32)xy};
    return result;
}

inline v2
V2i(u32 xy)
{
    v2 result = {(f32)xy, (f32)xy};
    return result;
}

inline v2
V2i(v2s a)
{
    v2 result = {(f32)a.x, (f32)a.y};
    return result;
}



inline v2
operator*(f32 a, v2 b)
{
    v2 result {
        a * b.x,
        a * b.y
    };
    return result;
}

inline v2
operator*(v2 a, f32 b)
{
    v2 result = b * a;
    return result;
}

inline v2
operator-(v2 a)
{
    v2 result;
    result.x = -a.x;
    result.y = -a.y;
    return result;
}

inline v2
operator+(v2 a, v2 b)
{
    v2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

inline v2
operator-(v2 a, v2 b)
{
    v2 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

inline v2
operator*=(v2 &a, f32 b)
{ 
    a = b * a;
    return a;
}

inline v2
operator+=(v2 &a, v2 b)
{
    a = a + b;
    return a;   
}

inline v2
operator-=(v2 &a, v2 b)
{
    a = a - b;
    return a;   
}


inline void
clamp_bot(v2 *value, f32 bound)
{
    clamp_bot(&value->x, bound);
    clamp_bot(&value->y, bound);
}

inline void
clamp_top(v2 *value, f32 bound)
{
    clamp_top(&value->x, bound);
    clamp_top(&value->y, bound);
}



inline v2
lerp(v2 a, v2 b, f32 t)
{
    v2 result = (1.0f - t)*a + t*b;
    return result;
}

inline v2
hadamard(v2 a, v2 b)
{
    v2 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    
    return result;
}

inline f32
inner(v2 a, v2 b)
{
    f32 result = a.x * b.x + a.y * b.y;
    return result;
}

inline f32
length_sq(v2 a)
{
    f32 result = inner(a, a);
    return result;
}

inline f32
length(v2 a)
{
    f32 result = square_root(length_sq(a));
    return result;
}

inline v2
normalize (v2 a)
{
    v2 result = a * (1.0f / length(a));
    return result;
}

inline v2
clamp01(v2 value)
{
    v2 result;
    result.x = clamp01(value.x);
    result.y = clamp01(value.y);
    
    return result;
}











//~ ========================= @v3_math ========================
inline v3
V3(f32 x, f32 y, f32 z)
{
    v3 result = {x, y, z};
    return result;
}

inline v3
V3(v2 xy, f32 z)
{
    v3 result = {xy.x, xy.y, z};
    return result;
}

inline v3
V3(f32 xyz)
{
    v3 result = {xyz, xyz, xyz};
    return result;
}

inline v3
V3i(s32 x, s32 y, s32 z)
{
    v3 result = {(f32)x, (f32)y, (f32)z};
    return result;
}

inline v3
V3i(u32 x, u32 y, u32 z)
{
    v3 result = {(f32)x, (f32)y, (f32)z};
    return result;
}

inline v3
operator*(f32 a, v3 b)
{
    v3 result;
    result.x = a * b.x;
    result.y = a * b.y;
    result.z = a * b.z;
    
    return result;
}

inline v3
operator*(v3 a, f32 b)
{
    v3 result = b * a;
    return result;
}

inline v3
operator-(v3 a)
{
    v3 result;
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    
    return result;
}

inline v3
operator+(v3 a, v3 b)
{
    v3 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}

inline v3
operator-(v3 a, v3 b)
{
    v3 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

inline v3
operator*=(v3 &a, f32 b)
{ 
    a = b * a;
    return a;
}

inline v3
operator+=(v3 &a, v3 b)
{
    a = a + b;
    return a;   
}

inline v3
operator-=(v3 &a, v3 b)
{
    a = a - b;
    return a;
}

inline v3
lerp(v3 a, v3 b, f32 t)
{
    v3 result = (1.0f - t)*a + t*b;
    return result;
}

inline v3
hadamard(v3 a, v3 b)
{
    v3 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y; 
    result.z = a.z * b.z;
    
    return result;
}

inline f32
inner(v3 a, v3 b)
{
    f32 result = (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
    return result;
}

inline f32
length_sq(v3 a)
{
    f32 result = inner(a, a);
    return result;
}

inline f32
length(v3 a)
{
    f32 result = square_root(length_sq(a));
    return result;
}

inline v3
normalize (v3 a)
{
    v3 result = a * (1.0f /length(a));
    return result;
}

inline v3
clamp01(v3 value)
{
    v3 result;
    result.x = clamp01(value.x);
    result.y = clamp01(value.y);
    result.z = clamp01(value.z);
    
    return result;
}









//~ ========================= vector4 =========================

inline v4
V4(f32 x, f32 y, f32 z, f32 w)
{
    v4 result = {x, y, z, w};
    return result;
}

inline v4
V4(v3 xyz, f32 w)
{
    v4 result;
    result.xyz = xyz;
    result.w = w;
    return result;
}


inline v4
V4(f32 scalar_xyzw)
{
    v4 result = {scalar_xyzw, scalar_xyzw, scalar_xyzw, scalar_xyzw};
    return result;
}


inline v4
V4i(s32 x, s32 y, s32 z, s32 w)
{
    v4 result = { (f32)x, (f32)y, (f32)z, (f32)w };
    return result;
}

inline v4
V4i(u32 x, u32 y, u32 z, u32 w)
{
    v4 result = { (f32)x, (f32)y, (f32)z, (f32)w };
    return result;
}

inline v4
operator*(f32 a, v4 b)
{
    v4 result;
    result.x = a * b.x;
    result.y = a * b.y;
    result.z = a * b.z;
    result.w = a * b.w;
    
    return result;
}

inline v4
operator*(v4 a, f32 b)
{
    v4 result = b * a;
    return result;
}

inline v4
operator-(v4 a)
{
    v4 result;
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    result.w = -a.w;
    
    return result;
}

inline v4
operator+(v4 a, v4 b)
{
    v4 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
    
    return result;
}

inline v4
operator-(v4 a, v4 b)
{
    v4 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    result.w = a.w - b.w;
    
    return result;
}

inline v4
operator*=(v4 &a, f32 b)
{ 
    a = b * a;
    return a;
}

inline v4
operator+=(v4 &a, v4 b)
{
    a = a + b;
    return a;   
}

inline v4
operator-=(v4 &a, v4 b)
{
    a = a - b;
    return a;   
}

inline v4
lerp(v4 a, v4 b, f32 t)
{
    v4 result = (1.0f - t)*a + t*b;
    return result;
}

inline v4
hadamard(v4 a, v4 b)
{
    v4 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y; 
    result.z = a.z * b.z;
    result.w = a.w * b.w;
    
    return result;
}

inline f32
inner(v4 a, v4 b)
{
    f32 result = (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
    return result;
}

inline f32
length_sq(v4 a)
{
    f32 result = inner(a, a);
    return result;
}

inline f32
length(v4 a)
{
    f32 result = square_root(length_sq(a));
    return result;
}

inline v4
normalize (v4 a)
{
    v4 result = a * (1.0f / length(a));
    return result;
}

inline v4
clamp01(v4 value)
{
    v4 result;
    result.x = clamp01(value.x);
    result.y = clamp01(value.y);
    result.z = clamp01(value.z);
    result.w = clamp01(value.w);
    
    return result;
}









//~ ========================== Rect2 ==========================


inline Rect2 // NOTE(mg): Rect3 -> Rect2
to_rect_xy(Rect3 a)
{
    Rect2 result;
    result.min = a.min.xy;
    result.max = a.max.xy;
    return result;
}

// Rect2 constructors

inline Rect2
rect_min_max(v2 min, v2 max)
{
    Rect2 output;
    output.min = min;
    output.max = max;
    return output;
};

inline Rect2
rect_min_dim(v2 min, v2 dim)
{
    Rect2 output;
    output.min = min;
    output.max = min + dim;
    return output;
};

inline Rect2
rect_center_half_dim(v2 center, v2 half_dim)
{
    Rect2 output;
    output.min = center - half_dim;
    output.max = center + half_dim;
    return output;
};

inline Rect2
rect_center_dim(v2 center, v2 dim)
{
    Rect2 output = rect_center_half_dim(center, 0.5f*dim);
    return output;
};

// Rect2 functions

inline Rect2
add_radius_to(Rect2 rectangle, v2 radius)
{
    Rect2 result;
    result.min = rectangle.min - radius;
    result.max = rectangle.max + radius;
    
    return result;
}

inline Rect2
get_offset(Rect2 rectangle, v2 offset)
{
    Rect2 result;
    result.min = rectangle.min + offset;
    result.max = rectangle.max + offset;
    
    return result;
}

inline b32
is_in_rectangle(Rect2 rectangle, v2 test)
{
    b32 result = ((test.x >= rectangle.min.x) && 
                  (test.y >= rectangle.min.y) &&
                  (test.x < rectangle.max.x) &&
                  (test.y < rectangle.max.y));
    
    return result;
} 

inline v2
get_min_corner(Rect2 rect)
{
    v2 result = rect.min;
    return result;
}

inline v2
get_max_corner(Rect2 rect)
{
    v2 result = rect.max;
    return result;
}

inline v2
get_center(Rect2 rect)
{
    v2 result = 0.5f*(rect.min + rect.max);
    return result;
}

inline v2
get_dim(Rect2 rect)
{
    v2 result = rect.max - rect.min;
    return result;
}

inline b32
are_intersecting(Rect2 a, Rect2 b)
{
    b32 result = !((b.max.x <= a.min.x) || // x axis
                   (b.min.x >= a.max.x) ||
                   (b.max.y <= a.min.y) || // y axis
                   (b.min.y >= a.max.y));
    
    return result;
}

inline v2
get_barycentric(Rect2 rect, v2 p)
{
    v2 result;
    result.x = safe_ratio_0((p.x - rect.min.x), (rect.max.x - rect.min.x));
    result.y = safe_ratio_0((p.y - rect.min.y), (rect.max.y - rect.min.y));
    
    return result;
}


// ================= additional ================

inline Rect2
get_intersection(Rect2 a, Rect2 b)
{
    Rect2 result;
    result.min.x = pick_bigger(a.min.x, b.min.x);
    result.min.y = pick_bigger(a.min.y, b.min.y);
    result.max.x = pick_smaller(a.max.x, b.max.x);
    result.max.y = pick_smaller(a.max.y, b.max.y);
    
    return result;
}

inline Rect2
get_union(Rect2 a, Rect2 b)
{
    // NOTE(mg): Optimistic approximation of the union as rectangle.
    
    Rect2 result;
    result.min.x = pick_smaller(a.min.x, b.min.x);
    result.min.y = pick_smaller(a.min.y, b.min.y);
    result.max.x = pick_bigger(a.max.x, b.max.x);
    result.max.y = pick_bigger(a.max.y, b.max.y);
    
    return result;
}

inline f32
get_clamped_rect_area(Rect2 a)
{
    f32 width = a.max.x - a.min.x;
    f32 height = a.max.y - a.min.y;
    f32 result = 0;
    
    if ((width > 0) && (height > 0))
    {
        result = width*height;
    }
    
    return result;
}

inline b32
has_area(Rect2 a)
{
    b32 result = ((a.min.x < a.max.x) && (a.min.y < a.max.y));
    return result;
}

inline Rect2
inverted_infinity_rect2()
{
    Rect2 result;
    result.min.x = result.min.y = F32_Max;
    result.max.x = result.max.y = -F32_Max;
    return result;
}












//~ ========================== Rect3 ==========================

// Rect3 constructors

inline Rect3
rect_min_max(v3 min, v3 max)
{
    Rect3 output;
    output.min = min;
    output.max = max;
    return output;
};

inline Rect3
rect_min_dim(v3 min, v3 dim)
{
    Rect3 output;
    output.min = min;
    output.max = min + dim;
    return output;
};

inline Rect3
rect_center_half_dim(v3 center, v3 half_dim)
{
    Rect3 output;
    output.min = center - half_dim;
    output.max = center + half_dim;
    return output;
};

inline Rect3
rect_center_dim(v3 center, v3 dim)
{
    Rect3 output = rect_center_half_dim(center, 0.5f*dim);
    return output;
};

// Rect3 functions

inline Rect3
add_radius_to(Rect3 rectangle, v3 radius)
{
    Rect3 result;
    result.min = rectangle.min - radius;
    result.max = rectangle.max + radius;
    
    return result;
}

inline Rect3
get_offset(Rect3 rectangle, v3 offset)
{
    Rect3 result;
    result.min = rectangle.min + offset;
    result.max = rectangle.max + offset;
    
    return result;
}

inline b32
is_in_rectangle(Rect3 rectangle, v3 test)
{
    b32 result = ((test.x >= rectangle.min.x) && 
                  (test.y >= rectangle.min.y) &&
                  (test.z >= rectangle.min.z) &&
                  (test.x < rectangle.max.x) &&
                  (test.y < rectangle.max.y) &&
                  (test.z < rectangle.max.z));
    
    return result;
} 


inline v3
get_min_corner(Rect3 rect)
{
    v3 result = rect.min;
    return result;
}

inline v3
get_max_corner(Rect3 rect)
{
    v3 result = rect.max;
    return result;
}

inline v3
get_center(Rect3 rect)
{
    v3 result = 0.5f*(rect.min + rect.max);
    return result;
}

inline v3
get_dim(Rect3 rect)
{
    v3 result = rect.max - rect.min;
    return result;
}


inline b32
are_intersecting(Rect3 a, Rect3 b)
{
    b32 result = !((b.max.x <= a.min.x) || // x axis
                   (b.min.x >= a.max.x) ||
                   (b.max.y <= a.min.y) || // y axis
                   (b.min.y >= a.max.y) ||
                   (b.max.z <= a.min.z) || // z axis
                   (b.min.z >= a.max.z));
    
    return result;
}

inline v3
get_barycentric(Rect3 rect, v3 p)
{
    v3 result;
    result.x = safe_ratio_0((p.x - rect.min.x), (rect.max.x - rect.min.x));
    result.y = safe_ratio_0((p.y - rect.min.y), (rect.max.y - rect.min.y));
    result.z = safe_ratio_0((p.z - rect.min.z), (rect.max.z - rect.min.z));
    
    return result;
}








//~ ========================== Rect2s ==========================

inline Rect2s
get_intersection(Rect2s a, Rect2s b)
{
    Rect2s result;
    result.min = {
        pick_bigger(a.min.x, b.min.x),
        pick_bigger(a.min.y, b.min.y)
    };
    result.max = {
        pick_smaller(a.max.x, b.max.x),
        pick_smaller(a.max.y, b.max.y)
    };
    return result;
}

inline Rect2s
get_union(Rect2s a, Rect2s b)
{
    // NOTE: "Optimistic" approximation of the union as rectangle.
    Rect2s result;
    result.min = {
        pick_smaller(a.min.x, b.min.x),
        pick_smaller(a.min.y, b.min.y)
    };
    result.max = {
        pick_bigger(a.max.x, b.max.x),
        pick_bigger(a.max.y, b.max.y)
    };
    return result;
}

inline s32
get_clamped_rect_area(Rect2s a)
{
    s32 width = a.max.x - a.min.x;
    s32 height = a.max.y - a.min.y;
    s32 result = 0;
    
    if ((width > 0) && (height > 0))
    {
        result = width*height;
    }
    
    return result;
}

inline b32
has_area(Rect2s a)
{
    b32 result = ((a.min.x < a.max.x) && (a.min.y < a.max.y));
    return result;
}

inline Rect2s
inverted_infinity_rect2s()
{
    Rect2s result;
    result.min.x = result.min.y = S32_Max;
    result.max.x = result.max.y = S32_Min;
    return result;
}






//~ ========================== Colors =========================

inline v4
srgb255_to_linear1(v4 c)
{
    f32 inv_255 = 1.0f / 255.0f;
    
    v4 result;
    result.r = square(inv_255 * c.r);
    result.g = square(inv_255 * c.g);
    result.b = square(inv_255 * c.b);
    result.a = inv_255 * c.a;
    
    return result;
}

inline v4
linear1_to_srgb255(v4 c)
{
    f32 one_255 = 255.0f;
    
    v4 result;
    result.r = one_255 * square_root(c.r);
    result.g = one_255 * square_root(c.g);
    result.b = one_255 * square_root(c.b);
    result.a = one_255 * c.a;
    
    return result;
}






//~ ====================== @Level_Memory ======================
#if Stf0_Level >= 30
//~ ======================== @Level_30 ========================

// ======================= @Memory_Arena ======================
struct Arena
{
    void *base;
    u64 position;
    u64 capacity;
    //
    u64 reserved_capacity;
    //
    u32 page_size;
    s32 stack_count; // NOTE(f0): safety/testing variable
};



// ======================= @Memory_Scope ======================

struct Arena_Scope
{
    Arena *copy;
    u64 position;
};

inline Arena_Scope
create_arena_scope(Arena *arena)
{
    Arena_Scope result = {};
    result.copy = arena;
    result.position = arena->position;
    return result;
}

inline void
reset_arena_position(Arena *arena, u64 position=0)
{
    arena->position = position;
}

inline void
pop_arena_scope(Arena_Scope *scope)
{
    assert(scope);
    assert(scope->copy);
    
    scope->copy->position = scope->position;
}

struct Automatic_Arena_Scope
{
    Arena_Scope scope;
    s32 stack_count;
    u32 _padding;
    
    Automatic_Arena_Scope(Arena* arena)
    {
        scope = create_arena_scope(arena);
        stack_count = arena->stack_count++;
    }
    
    ~Automatic_Arena_Scope()
    {
        pop_arena_scope(&scope);
        scope.copy->stack_count -= 1;
        assert(scope.copy->stack_count == stack_count);
    }
};

#define arena_scope(Arena) Automatic_Arena_Scope glue(automatic_arena_scope_, This_Line_S32)(Arena)






// ======================= @Memory_Push =======================
#define push_array(ArenaPtr, Type, Count)\
(Type *)push_bytes_((ArenaPtr), (sizeof(Type)*(Count)), alignof(Type))

#define push_array_align(ArenaPtr, Type, Count, Align)\
(Type *)push_bytes_((ArenaPtr), (sizeof(Type)*(Count)), Align)

#define push_array_clear(ArenaPtr, Type, Count)\
(Type *)push_bytes_clear_((ArenaPtr), (sizeof(Type)*(Count)), alignof(Type))

#define push_array_clear_align(ArenaPtr, Type, Count, Align)\
(Type *)push_bytes_clear_((ArenaPtr), (sizeof(Type)*(Count)), Align)

////////////////////////////////
#define push_struct(ArenaPtr, Type)       push_array(ArenaPtr, Type, 1)
#define push_struct_align(ArenaPtr, Type) push_array_align(ArenaPtr, Type, 1)
#define push_struct_clear(ArenaPtr, Type) push_array_clear(ArenaPtr, Type, 1)

////////////////////////////////
#define push_array_copy(ArenaPtr, Source, Type, Count)\
(Type *)push_bytes_and_copy_((ArenaPtr), (Source), sizeof(Type)*(Count), alignof(Type))




// ====================== @Memory_Alloca ======================
#define push_stack_array(Type, Count) (Type *)alloca(sizeof(Type)*Count)






// =================== @Memory_Virtual_Arena ==================
inline u64
get_aligment_offset(Arena *arena, u64 aligment)
{
    u64 aligment_offset = 0;
    u64 result_pointer = (u64)arena->base + arena->position;
    u64 aligment_mask = aligment - 1;
    
    if (result_pointer & aligment_mask)
    {
        aligment_offset = aligment - (result_pointer & aligment_mask);
    }
    
    return aligment_offset;
}


inline u64
round_up_to_page_size(Arena *arena, u64 value)
{
    u64 result = align_bin_to(value, (u64)arena->page_size);
    return result;
}

inline void
commit_virtual_memory_(Arena *arena, u64 position_required_to_fit)
{
    assert(position_required_to_fit <= arena->reserved_capacity);
    u64 target_capacity = 2 * position_required_to_fit;
    target_capacity = pick_smaller(target_capacity, arena->reserved_capacity);
    
    u64 commit_end_position = round_up_to_page_size(arena, target_capacity);
    u64 commit_size = commit_end_position - arena->capacity;
    u8 *commit_address = (u8 *)arena->base + arena->capacity;
    
    void *commit_result = VirtualAlloc(commit_address, commit_size, MEM_COMMIT, PAGE_READWRITE);
    assert(commit_result);
    arena->capacity = commit_end_position;
}

function void *
push_bytes_virtual_commit_(Arena *arena, u64 alloc_size, u64 alignment)
{
    assert(arena->base);
    assert(alloc_size > 0);
    u64 alignment_offset = get_aligment_offset(arena, alignment);
    u64 future_position = arena->position + alignment_offset + alloc_size;
    
    if (future_position > arena->capacity)
    {
        commit_virtual_memory_(arena, future_position);
    }
    
    void *result = (u8 *)arena->base + arena->position + alignment_offset;
    arena->position = future_position;
    
    return result;
}

function void *
push_bytes_virtual_commit_unaligned_(Arena *arena, u64 alloc_size)
{
    assert(arena->base);
    u64 future_position = arena->position + alloc_size;
    
    if (future_position > arena->capacity)
    {
        commit_virtual_memory_(arena, future_position);
    }
    
    void *result = (u8 *)arena->base + arena->position;
    arena->position = future_position;
    return result;
}






// ================ @Memory_Arena_Constructors ================

function Arena
create_virtual_arena(u64 target_reserved_capacity = gigabytes(16))
{
    Arena arena = {};
    
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    
    arena.page_size = system_info.dwPageSize;
    arena.reserved_capacity = round_up_to_page_size(&arena, target_reserved_capacity);
    arena.base = VirtualAlloc(nullptr, arena.reserved_capacity, MEM_RESERVE, PAGE_READWRITE);
    
    assert(arena.base);
    return arena;
}

function void
free_virtual_arena(Arena *arena)
{
    b32 result = VirtualFree(arena->base, 0, MEM_RELEASE);
    assert(result);
}

inline void *
push_bytes_(Arena *arena, u64 alloc_size, u64 alignment)
{
    void *result = push_bytes_virtual_commit_(arena, alloc_size, alignment);
    return result;
}

inline void *
push_bytes_clear_(Arena *arena, u64 alloc_size, u64 alignment)
{
    void *result = push_bytes_virtual_commit_(arena, alloc_size, alignment);
    clear_bytes(result, alloc_size);
    return result;
}

inline void *
push_bytes_and_copy_(Arena *arena, void *source, u64 alloc_size, u64 alignment)
{
    void *mem = push_bytes_(arena, alloc_size, alignment);
    copy_bytes(mem, source, alloc_size);
    return mem;
}













// ======================= @Memory_Lists ======================


// =================== @Memory_Virtual_Array ==================
template <typename T>
struct Virtual_Array
{
    union {
        Arena arena;
        T *array;
    };
    u64 count;
    
    inline T *at(u64 index)
    {
        assert(index < count);
        T *result = array + index;
        return result;
    }
    
    inline T *grow(u64 grow_count = 1)
    {
        assert(grow_count > 0);
        T *result = array + count;
        push_bytes_virtual_commit_unaligned_(&arena, sizeof(T)*grow_count);
        count += grow_count;
        return result;
    }
    
    inline void reset(u64 new_count=0)
    {
        assert(new_count <= count);
        count = new_count;
        arena.position = sizeof(T)*new_count;
    }
};


template <typename T>
function Virtual_Array<T>
create_virtual_array(u64 initial_count = 0,
                     u64 target_reserved_capacity = gigabytes(8))
{
    Virtual_Array<T> result = {};
    result.arena = create_virtual_arena(target_reserved_capacity);
    if (initial_count > 0)
    {
        result.grow(initial_count);
    }
    return result;
}


// ================ @Memory_Virtual_Array_Scope ===============

template <typename T>
struct Virtual_Array_Scope
{
    Arena_Scope arena_scope;
    Virtual_Array<T> *copy;
    u64 element_count;
};

template <typename T>
inline Virtual_Array_Scope<T>
create_virtual_array_scope(Virtual_Array<T> *array)
{
    Virtual_Array_Scope<T> result = {};
    result.arena_scope = create_arena_scope(&array->arena);
    result.copy = array;
    result.element_count = array->count;
    return result;
}

template <typename T>
inline void
pop_virtual_array_scope(Virtual_Array_Scope<T> *scope)
{
    assert(scope);
    assert(scope->copy);
    
    pop_arena_scope(&scope->arena_scope);
    scope->copy->count = scope->element_count;
}










// ==================== @Memory_Linked_List ===================
template <typename T>
struct Linked_List_Node
{
    Linked_List_Node<T> *next;
    T item;
};

template <typename T>
struct Linked_List
{
    Linked_List_Node<T> *first;
    Linked_List_Node<T> *last;
    u64 count;
    
    inline Linked_List_Node<T> *push_get_node(Arena *arena)
    {
        if (count)
        {
            assert(first);
            assert(last);
            last->next = push_array(arena, Linked_List_Node<T>, 1);
            last = last->next;
        }
        else
        {
            first = push_array(arena, Linked_List_Node<T>, 1);
            last = first;
        }
        
        ++count;
        last->next = nullptr;
        return last;
    }
    
    inline T *push(Arena *arena)
    {
        auto result = push_get_node(arena);
        return &result->item;
    }
    
    inline T *at(u64 index)
    {
        assert(index < count);
        
        T *result = nullptr;
        
        for (Linked_List_Node<T> *node = first;
             node;
             node = node->next)
        {
            result = &node->item;
            
            if (index == 0) { break; }
            index -= 1;
        }
        
        return result;
    }
};














//~ ======================= @Level_Alloc ======================
#if Stf0_Level >= 40
//~ ======================== @Level_40 =========================


// Types
struct Directory
{
    String *names;
    u64 name_count;
};

struct Path
{
    Directory directory;
    String file_name;
};


typedef Linked_List<Path> Path_List;
typedef Linked_List<Directory> Directory_List;
typedef Linked_List<String> String_List;





// ======================= @Alloc_Basic =======================

inline String
allocate_string(Arena *arena, u64 size)
{
    String result = {};
    if (size > 0) {
        result = {push_array(arena, u8, size), size};
    }
    return result;
}


function String
copy(Arena *arena, String source)
{
    String result = {};
    if (source.size > 0) {
        result = {push_array(arena, u8, source.size), source.size};
        copy_array(result.str, source.str, u8, source.size);
    }
    return result;
}


function char *
copy(Arena *arena, char *source, s64 overwrite_len = -1)
{
    u64 len = (u64)((overwrite_len > 0) ? overwrite_len : length(source));
    char *result = push_array(arena, char, len + 1);
    copy_array(result, source, char, len);
    result[len] = 0;
    return result;
}


function char *
cstr_from_string(Arena *arena, String string)
{
    char *cstr = push_array(arena, char, string.size+1);
    copy_array(cstr, string.str, char, string.size);
    cstr[string.size] = 0;
    return cstr;
}



function Directory
copy(Arena *arena, Directory directory)
{
    Directory result = {};
    
    if (directory.name_count > 0)
    {
        result.names = push_array(arena, String, directory.name_count);
        result.name_count = directory.name_count;
        
        for_u64(name_index, directory.name_count)
        {
            result.names[name_index] = copy(arena, directory.names[name_index]);
        }
    }
    
    return result;
}

function Path
copy(Arena *arena, Path path)
{
    Path result = {};
    result.directory = copy(arena, path.directory);
    result.file_name = copy(arena, path.file_name); // TODO(f0): rename to copy(...)
    return result;
}






// ===================== @Alloc_Directory =====================

inline b32
equals(Directory a, Directory b)
{
    b32 result = false;
    
    if (a.name_count == b.name_count)
    {
        result = true;
        for_u64(name_index, a.name_count)
        {
            String name_a = a.names[name_index];
            String name_b = b.names[name_index];
            b32 names_equal = equals(name_a, name_b);
            
            if (!names_equal) {
                result = false;
                break;
            }
        }
    }
    
    return result;
}

function Directory
directory_from_string(Arena *arena, String source)
{
    Directory result = {};
    result.name_count = count_of(source, lit2str("/\\"));
    if (source.size > 0 && !is_slash(source.str[source.size-1]))
    {
        result.name_count += 1;
    }
    
    if (result.name_count > 0)
    {
        result.names = push_array(arena, String, result.name_count);
        
        u64 current_p = 0;
        for_u64(name_index, result.name_count)
        {
            u64 start_p = current_p;
            String element = string_advance(source, start_p);
            
            for (;
                 current_p < source.size;
                 ++current_p)
            {
                if (is_slash(source.str[current_p]))
                {
                    element.size = (current_p++ - start_p);
                    break;
                }
            }
            
            result.names[name_index] = element;
        }
    }
    
    return result;
}


function Directory
directory_append(Arena *arena, Directory parent_directory, String sub_directory_name)
{
    Directory result = {};
    result.name_count = parent_directory.name_count + 1;
    result.names = push_array(arena, String, result.name_count);
    for_u64(name_index, parent_directory.name_count)
    {
        result.names[name_index] = parent_directory.names[name_index];
    }
    result.names[result.name_count-1] = sub_directory_name;
    return result;
}



inline u64
get_directory_names_length_sum(Directory directory)
{
    u64 result = 0;
    for_u64(name_index, directory.name_count)
    {
        result += directory.names[name_index].size;
    }
    return result;
}

inline u64
get_directory_string_length(Directory directory)
{
    u64 result = get_directory_names_length_sum(directory);
    result += directory.name_count;
    return result;
}













// ======================== @Alloc_Path =======================

function Path
path_from_string(Arena *arena, String source)
{
    Path result = {};
    String_Path_Split parts = split_into_directory_and_file_name(source);
    
    result.directory = directory_from_string(arena, parts.directory);
    result.file_name = parts.file_name;
    
    return result;
}


inline Path
get_path(Directory directory, String file_name)
{
    Path result = {
        directory,
        file_name
    };
    return result;
};


inline b32
equals(Path a, Path b)
{
    b32 result = equals(a.file_name, b.file_name);
    
    if (result) {
        result = equals(a.directory, b.directory);
    }
    
    return result;
}



//~ Big functions

//- Directory
// TODO(f0): convert to fill_buffer
function String
to_string(Arena *arena, Directory directory,
          b32 use_windows_slash = (Native_Slash_Char == '\\'))
{
    u8 slash = (u8)(use_windows_slash ? '\\' : '/');
    
    u64 result_index = 0;
    String result = allocate_string(arena, get_directory_string_length(directory)+1);
    result.size -= 1; // TODO(f0): Convert it to fill function
    // TODO(f0): Make it so string version doesn't overallocate 1 byte
    result.str[result.size] = 0;
    
    for_u64(name_index, directory.name_count)
    {
        String *dir_name = directory.names + name_index;
        for_u64(char_index, dir_name->size)
        {
            result.str[result_index++] = dir_name->str[char_index];
        }
        result.str[result_index++] = slash;
    }
    
    result.str[result.size] = 0;
    assert(result.size == result_index);
    return result;
}

inline char *
to_cstr(Arena *arena, Directory directory,
        b32 use_windows_slash = (Native_Slash_Char == '\\'))
{
    String string = to_string(arena, directory, use_windows_slash);
    char *result = (char *)string.str;
    return result;
}


//- Path
inline u64
get_path_string_length(Path path)
{
    u64 result = get_directory_names_length_sum(path.directory);
    result += path.directory.name_count;
    result += path.file_name.size;
    return result;
}


function u64
fill_buffer_from_path(void *output, u64 output_size,
                      Path path, b32 use_windows_slash = (Native_Slash_Char == '\\'))
{
    u8 slash = (u8)(use_windows_slash ? '\\' : '/');
    u64 out_index = 0;
    
    u8 *out = (u8 *)output;
    b32 full_fill = false;
    
    for_u64(name_index, path.directory.name_count)
    {
        String *dir_name = path.directory.names + name_index;
        for_u64(char_index, dir_name->size)
        {
            if (out_index >= output_size) {
                goto early_exit_label;
            }
            
            out[out_index++] = dir_name->str[char_index];
        }
        
        
        if (out_index >= output_size) {
            goto early_exit_label;
        }
        
        out[out_index++] = slash;
    }
    
    for_u64(char_index, path.file_name.size)
    {
        if (out_index >= output_size) {
            goto early_exit_label;
        }
        
        out[out_index++] = path.file_name.str[char_index];
    }
    
    full_fill = true;
    
    early_exit_label:
    
    assert(full_fill);
    assert(output_size == out_index);
    return out_index;
}


function String
to_string(Arena *arena, Path path,
          b32 use_windows_slash = (Native_Slash_Char == '\\'))
{
    u64 pre_len = get_path_string_length(path);
    String result = allocate_string(arena, pre_len);
    
    u64 post_len = fill_buffer_from_path(result.str, result.size, path, use_windows_slash);
    assert(pre_len == post_len);
    
    return result;
}

inline char *
to_cstr(Arena *arena, Path path,
        b32 use_windows_slash = (Native_Slash_Char == '\\'))
{
    u64 pre_len = get_path_string_length(path);
    char *result = push_array(arena, char, pre_len + 1);
    
    u64 post_len = fill_buffer_from_path(result, pre_len, path, use_windows_slash);
    assert(pre_len == post_len);
    
    result[pre_len] = 0;
    return result;
}
















//~ String builders

// =================== @Alloc_String_Builder ==================
// TODO(f0): Base API on "fill" version
struct Separator_String_Builder_Item
{
    String string;
    b32 skip_following_separator;
    u32 _padding;
};

struct Separator_String_Builder
{
    Linked_List<Separator_String_Builder_Item> list;
    u64 string_length_sum;
    u64 separator_count;
    b32 last_has_separator;
    u32 _padding;
};


// NOTE(f0): Use if you manually mess with builder's items
function void
builder_recalculate_length(Separator_String_Builder *builder)
{
    builder->string_length_sum = 0;
    builder->separator_count = 0;
    builder->last_has_separator = false;
    
    for_linked_list(node, builder->list)
    {
        builder->string_length_sum += node->item.string.size;
        builder->last_has_separator = !(node->item.skip_following_separator);
        builder->separator_count += (builder->last_has_separator ? 1 : 0);
    }
}

function Separator_String_Builder_Item *
builder_add(Arena *arena, Separator_String_Builder *builder,
            String string, b32 skip_following_separator = false)
{
    Separator_String_Builder_Item *item = builder->list.push(arena);
    item->string = string;
    item->skip_following_separator = skip_following_separator;
    
    builder->string_length_sum += string.size;
    builder->last_has_separator = !skip_following_separator;
    if (builder->last_has_separator) {
        builder->separator_count += 1;
    }
    
    return item;
}

function String
build_string(Arena *arena, Separator_String_Builder *builder, String separator = string(" "))
{
    u64 len = builder->string_length_sum + (builder->separator_count * separator.size);
    if (builder->last_has_separator) {
        len -= separator.size;
    }
    
    String result = allocate_string(arena, len+1);
    result.size -= 1;
    
    u64 char_index = 0;
    u64 node_index = 0;
    
    for (auto *node = builder->list.first;
         node;
         node = node->next, ++node_index)
    {
        Separator_String_Builder_Item *item = &node->item;
        copy_bytes(result.str + char_index, item->string.str, item->string.size);
        char_index += item->string.size;
        
        if (!item->skip_following_separator &&
            (node_index + 1) < builder->list.count)
        {
            copy_bytes(result.str + char_index, separator.str, separator.size);
            char_index += separator.size;
        }
    }
    
    assert(char_index == result.size);
    return result;
}


inline char *
build_cstr(Arena *arena, Separator_String_Builder *builder, String separator = string(" "))
{
    String string = build_string(arena, builder, separator);
    char *result = (char *)string.str;
    result[string.size] = 0;
    return result;
}




// =================== @Alloc_Simple_String_Builder ==================
// TODO(f0): Base API on "fill" version
struct Simple_String_Builder_Item
{
    String string;
};

struct Simple_String_Builder
{
    Linked_List<Simple_String_Builder_Item> list;
    u64 string_length_sum;
};



// NOTE(f0): Use if you manually mess with builder's items
function void
builder_recalculate_length(Simple_String_Builder *builder)
{
    builder->string_length_sum = 0;
    
    for_linked_list(node, builder->list)
    {
        builder->string_length_sum += node->item.string.size;
    }
}

function Simple_String_Builder_Item *
builder_add(Arena *arena, Simple_String_Builder *builder, String string)
{
    Simple_String_Builder_Item *item = builder->list.push(arena);
    item->string = string;
    builder->string_length_sum += string.size;
    return item;
}

function String
build_string(Arena *arena, Simple_String_Builder *builder)
{
    u64 len = builder->string_length_sum;
    
    String result = allocate_string(arena, len+1);
    result.size -= 1;
    
    u64 char_index = 0;
    u64 node_index = 0;
    
    for (auto *node = builder->list.first;
         node;
         node = node->next, ++node_index)
    {
        Simple_String_Builder_Item *item = &node->item;
        copy_bytes(result.str + char_index, item->string.str, item->string.size);
        char_index += item->string.size;
    }
    
    assert(char_index == result.size);
    return result;
}

inline char *
build_cstr(Arena *arena, Simple_String_Builder *builder)
{
    String string = build_string(arena, builder);
    char *result = (char *)string.str;
    result[string.size] = 0;
    return result;
}







// ====================== @Alloc_Stringf ======================
function String
stringf(Arena *arena, char *format, ...)
{
    va_list args;
    va_start(args, format);
    s32 len = vsnprintf(0, 0, format, args);
    assert(len >= 0);
    String result = allocate_string(arena, (u64)(len+1));
    vsnprintf((char *)result.str, result.size, format, args);
    --result.size;
    va_end(args);
    return result;
}


function char *
cstrf(Arena *arena, char *format, ...)
{
    va_list args;
    va_start(args, format);
    s32 size = vsnprintf(0, 0, format, args) + 1;
    assert(size >= 0);
    char *result = push_array(arena, char, size);
    vsnprintf(result, (u64)size, format, args);
    result[size-1] = 0;
    va_end(args);
    return result;
}







// =================== @String_Helpers ==================
function String
concatenate(Arena *arena, String first, String second)
{
    String result = allocate_string(arena, first.size + second.size);
    copy_bytes(result.str, first.str, first.size);
    copy_bytes(result.str + first.size, second.str, second.size);
    return result;
}


function String
create_new_file_name_extension(Arena *arena, String file_name, String new_extension)
{
    // Example: new_extension = l2s("txt");
    String file_name_no_extension = trim_from_index_of_reverse(file_name, '.');
    file_name_no_extension.size += 1;
    String result = concatenate(arena, file_name_no_extension, new_extension);
    return result;
}







// ======================== @to_string ========================
function String
to_string(Arena *a, f32 value)
{
    String result = stringf(a, "%.2f", value);
    return result;
}

function String
to_string(Arena *a, Rect2 value)
{
    String result = stringf(a, "{min: {%.2f, %.2f}, max: {%.2f, %.2f}}",
                            value.min.x, value.min.y,
                            value.max.x, value.max.y);
    return result;
}

function String
to_string(Arena *a, s64 value)
{
    String result = stringf(a, "%lld", value);
    return result;
}

function String
to_string(Arena *a, u64 value)
{
    String result = stringf(a, "%llu", value);
    return result;
}

function String
to_string(Arena *a, u32 value)
{
    String result = stringf(a, "%u", value);
    return result;
}

function String
to_string(Arena *a, s32 value)
{
    String result = stringf(a, "%d", value);
    return result;
}








//~ ===================== @Level_Platform =====================
#if Stf0_Level >= 50
//~ ======================== @Level_50 ========================

#if Def_Windows
struct File_Handle
{
    HANDLE handle_;
    // TODO(f0): @maybe keep 1 byte for error check and few bytes for file_name error reporting?
    b32 no_error;
    u32 _padding;
};

struct Pipe_Handle
{
    FILE *handle_;
    b32 no_error;
    u32 _padding;
};

#elif Def_Linux
#    error "not impl"
#endif
//=============================================================

// ===================== @Platform_Global =====================

struct Stf0_Global_State
{
    f32 inv_query_perfomance_freq;
};
global Stf0_Global_State _stf0_global;


function void
stf0_initialize()
{
#if Def_Windows
    //=========
    LARGE_INTEGER large_perfomance_freq;
    b32 return_code_test = QueryPerformanceFrequency(&large_perfomance_freq);
    assert(return_code_test);
    assert(large_perfomance_freq.QuadPart);
    _stf0_global.inv_query_perfomance_freq = 1.f / (f32)large_perfomance_freq.QuadPart;
    //=========
#else
#error "not impl"
#endif
};



// ====================== @Platform_Time ======================
inline Time32ms
platform_get_time32_ms()
{
#if Def_Windows
    Time32ms result = {timeGetTime()};
    
#else
#error "not impl"
    
#endif
    return result;
}

inline Time_Perfomance
platform_get_perfomance_time()
{
    // NOTE(f0): Use get_time_elapsed to get delta between 2 measurements
    // NOTE(f0): Run stf0_initialize() before running get_seconds_elapsed!
#if Def_Windows
    LARGE_INTEGER large;
    QueryPerformanceCounter(&large);
    Time_Perfomance result = {large.QuadPart};
#else
#error "not impl"
#endif
    return result;
}

inline f32
get_seconds_elapsed(Time_Perfomance recent, Time_Perfomance old)
{
    // NOTE(f0): Run stf0_initialize() first!
    f32 result = (f32)(recent.t_ - old.t_) * _stf0_global.inv_query_perfomance_freq;
    return result;
}

inline f32
get_milliseconds_elapsed(Time_Perfomance recent, Time_Perfomance old)
{
    // NOTE(f0): Run stf0_initialize() first!
    f32 result = (f32)((recent.t_ - old.t_) * 1000) * _stf0_global.inv_query_perfomance_freq;
    return result;
}


inline f32
get_microseconds_elapsed(Time_Perfomance recent, Time_Perfomance old)
{
    // NOTE(f0): Run stf0_initialize() first!
    f32 result = (f32)((recent.t_ - old.t_) * 1000000) * _stf0_global.inv_query_perfomance_freq;
    return result;
}



// ===================== @Platform_File_IO ====================
inline b32
is_valid_handle(File_Handle *file)
{
#if Def_Windows
    b32 result = (file->handle_ != INVALID_HANDLE_VALUE);
#if Def_Slow
    if (!result) {
        DWORD error_code = GetLastError();
        debug_break();
    }
#endif
    
#else
#error "not implt"
#endif
    return result;
}

inline b32
no_errors(File_Handle *file)
{
    b32 result = file->no_error;
    return result;
}

inline void
set_error(File_Handle *file,
          char *message = "", char *path_cstr = "")
{
    // TODO(f0): Optional error logging here? Stf0_Log_Io_Errors?
    printf("[file(%s) error] %s;\n", path_cstr, message);
    debug_break();
    file->no_error = false;
}


//=============
function File_Handle
platform_file_open_read(char *path)
{
    File_Handle file = {};
#if Def_Windows
    file.handle_ = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
#else
    file.handle_ = fopen(path, "rb");
#endif
    file.no_error = is_valid_handle(&file);
    return file;
}

function File_Handle
platform_file_open_read(Arena *temp_arena, Path path)
{
    arena_scope(temp_arena);
    char *path_cstr = to_cstr(temp_arena, path);
    File_Handle file = platform_file_open_read(path_cstr);
    return file;
}





//=============
// NOTE(f0): Creates _new_ file (deletes previous content)
function File_Handle
platform_file_open_write(char *path)
{
    File_Handle file = {};
#if Def_Windows
    file.handle_ = CreateFileA(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
#else
    file.handle_ = fopen(path, "wb");
#endif
    file.no_error = is_valid_handle(&file);
    return file;
}

function File_Handle
platform_file_open_write(Arena *temp_arena, Path path)
{
    arena_scope(temp_arena);
    char *path_cstr = to_cstr(temp_arena, path);
    File_Handle file = platform_file_open_write(path_cstr);
    return file;
}






//=============
function File_Handle
platform_file_open_append(char *path)
{
    File_Handle file = {};
#if Def_Windows
    file.handle_ = CreateFileA(path, FILE_APPEND_DATA, 0, nullptr, OPEN_ALWAYS, 0, nullptr);
#else
    file.handle_ = fopen(path, "ab");
#endif
    file.no_error = is_valid_handle(&file);
    return file;
}

function File_Handle
platform_file_open_append(Arena *temp_arena, Path path)
{
    arena_scope(temp_arena);
    char *path_cstr = to_cstr(temp_arena, path);
    File_Handle file = platform_file_open_append(path_cstr);
    return file;
}






//=============
function void
platform_file_close(File_Handle *file)
{
#if Def_Windows
    CloseHandle(file->handle_);
#else
    fclose(file.handle_);
#endif
}






//=============
function b32
platform_file_copy(char *source, char *destination, b32 overwrite)
{
#if Def_Windows
    b32 fail_if_exists = !overwrite;
    b32 result = CopyFileA(source, destination, fail_if_exists);
#else
#error "not impl"
#endif
    return result;
}

function b32
platform_file_copy(Arena *temp_arena, Path source, Path destination, b32 overwrite)
{
    arena_scope(temp_arena);
    char *source_cstr = to_cstr(temp_arena, source);
    char *destination_cstr = to_cstr(temp_arena, destination);
    b32 result = platform_file_copy(source_cstr, destination_cstr, overwrite);
    return result;
}






//=============
function b32
platform_file_hard_link(char *source_path, char *link_path)
{
#if Def_Windows
    b32 result = CreateHardLinkA(link_path, source_path, nullptr);
#else
#error "not impl"
#endif
    return result;
}

function b32
platform_file_hard_link(Arena *temp_arena, Path source_path, Path link_path)
{
    arena_scope(temp_arena);
    char *link_path_cstr = to_cstr(temp_arena, link_path);
    char *source_path_cstr = to_cstr(temp_arena, source_path);
    b32 result = platform_file_hard_link(link_path_cstr, source_path_cstr);
    return result;
}






//=============
function b32
platform_file_delete(char *path)
{
#if Def_Windows
    b32 result = DeleteFileA(path);
#else
#error "not impl"
#endif
    return result;
}

function b32
platform_file_delete(Arena *temp_arena, Path path)
{
    arena_scope(temp_arena);
    char *path_cstr = to_cstr(temp_arena, path);
    b32 result = platform_file_delete(path_cstr);
    return result;
}





//=============
function b32
platform_file_exists(char *path)
{
#if Def_Windows
    DWORD attributes = GetFileAttributesA(path);
    b32 result = ((attributes != INVALID_FILE_ATTRIBUTES) &&
                  ((attributes & FILE_ATTRIBUTE_DIRECTORY) == 0));
#else
#error "not impl"
#endif
    return result;
}

function b32
platform_file_exists(Arena *temp_arena, Path path)
{
    arena_scope(temp_arena);
    char *path_cstr = to_cstr(temp_arena, path);
    b32 result = platform_file_exists(path_cstr);
    return result;
}

function b32
platform_directory_exists(char *dir_cstr)
{
#if Def_Windows
    DWORD attributes = GetFileAttributesA(dir_cstr);
    b32 result = ((attributes != INVALID_FILE_ATTRIBUTES) &&
                  ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0));
#else
#error "not impl"
#endif
    return result;
}

function b32
platform_directory_exists(Arena *temp_arena, Directory dir)
{
    arena_scope(temp_arena);
    char *dir_cstr = to_cstr(temp_arena, dir);
    b32 result = platform_directory_exists(dir_cstr);
    return result;
}






//=============
function u64
platform_file_read(File_Handle *file, void *buffer, u64 buffer_size)
{
    u64 result = 0;
    
    if (no_errors(file))
    {
#if Def_Windows
        DWORD bytes_read = 0;
        b32 success = ReadFile(file->handle_, buffer, safe_truncate_to_u32(buffer_size), &bytes_read, nullptr);
        if (!success) {
            set_error(file, "Failed read");
            result = 0;
        } else {
            result = bytes_read;
        }
        
#else
#error "not impl"
#endif
    }
    
    return result;
}






//=============
#define platform_file_write_array(File, SourcePtr, Type, Count) \
platform_file_write_bytes(File, SourcePtr, ((Count)*sizeof(Type)))


function void
platform_file_write_bytes(File_Handle *file, void *source, u64 size)
{
    assert(size <= U32_Max);
    
    if (no_errors(file))
    {
#if Def_Windows
        DWORD bytes_written = 0;
        b32 result = WriteFile(file->handle_, source, (DWORD)size, &bytes_written, nullptr);
        if (!result || (bytes_written != size)) {
            set_error(file, "Couldn't write");
        }
#else
#error "not impl"
#endif
    }
}
inline void platform_file_write_string(File_Handle *file, String string) {
    platform_file_write_bytes(file, string.str, string.size);
}





//=============
function void
platform_file_set_ending_to_current_distance(File_Handle *file)
{
    if (no_errors(file))
    {
#if Def_Windows
        b32 success = SetEndOfFile(file->handle_);
        if (!success) {
            set_error(file, "Couldn't set file ending");
        }
#else
#error "not impl"
#endif
    }
}


function void
platform_file_set_distance_from_start(File_Handle *file, s64 distance)
{
    if (no_errors(file))
    {
#if Def_Windows
        LARGE_INTEGER offset;
        offset.QuadPart = distance;
        b32 success = SetFilePointerEx(file->handle_, offset, nullptr, FILE_BEGIN);
        if (!success) {
            set_error(file, "Couldn't set file distance pointer from start");
        }
#else
#error "not impl"
#endif
    }
}

function void
platform_file_set_distance_from_current(File_Handle *file, s64 distance)
{
    if (no_errors(file))
    {
#if Def_Windows
        LARGE_INTEGER offset;
        offset.QuadPart = distance;
        b32 success = SetFilePointerEx(file->handle_, offset, nullptr, FILE_CURRENT);
        if (!success) {
            set_error(file, "Couldn't set file distance pointer from start");
        }
#else
#error "not impl"
#endif
    }
}

function void
platform_file_set_distance_from_end(File_Handle *file, s64 distance)
{
    if (no_errors(file))
    {
#if Def_Windows
        LARGE_INTEGER offset;
        offset.QuadPart = distance;
        b32 result = SetFilePointerEx(file->handle_, offset, nullptr, FILE_END);
        if (!result) {
            set_error(file, "Couldn't set file distance pointer from start");
        }
#else
#error "not impl"
#endif
    }
}







struct Distance_Result
{
    s64 value;
    b32 success;
    u32 _padding;
};

function Distance_Result
platform_file_get_distance(File_Handle *file)
{
    Distance_Result result = {};
    if (no_errors(file))
    {
#if Def_Windows
        LARGE_INTEGER distance;
        result.success = SetFilePointerEx(file->handle_, {}, &distance, FILE_CURRENT);
        if (!result.success) {
            set_error(file, "Couldn't get file distance");
        }
        result.value = distance.QuadPart;
#else
#error "not impl"
#endif
    }
    return result;
}















//==================== @Platform_Directory ====================
function b32
platform_directory_create(char *directory_path)
{
    // NOTE(f0): Returns true if directory exists
#if Def_Windows
    s32 code = CreateDirectoryA(directory_path, nullptr);
    b32 result = (code == 0);
    if (!result)
    {
        DWORD error = GetLastError();
        result = (error == ERROR_ALREADY_EXISTS);
    }
    
#else
    s32 code = mkdir(path);
    b32 result = (code == 0);
    if (!result)
    {
        result = (errno == EEXIST); // this errno check won't compile?
    }
#endif
    
    return result;
}

function b32
platform_directory_create(Arena *temp_arena, Directory directory)
{
    arena_scope(temp_arena);
    char *dir_cstr = to_cstr(temp_arena, directory);
    b32 result = platform_directory_create(dir_cstr);
    return result;
}



function b32
platform_directory_set_current(char *path)
{
#if Def_Windows
    b32 result = SetCurrentDirectory(path) != 0;
#else
#error "not impl"
#endif
    
    return result;
}

function b32
platform_directory_set_current(Arena *temp_arena, Directory directory)
{
    arena_scope(temp_arena);
    char *path_cstr = to_cstr(temp_arena, directory);
    b32 result = platform_directory_set_current(path_cstr);
    return result;
}






// ==================== @Platform_File_Data ===================
function u64
platform_file_get_size(File_Handle *file)
{
    // NOTE(f0): Default to zero
    u64 result = 0;
    
    if (no_errors(file))
    {
#if Def_Windows
        LARGE_INTEGER size = {};
        b32 success = GetFileSizeEx(file->handle_, &size);
        if (success) {
            result = safe_truncate_to_u64((s64)size.QuadPart);
        } else {
            set_error(file, "Couldn't get file size");
        }
#else
        /* NOTE(f0): seeking size example
          
          fseek(file, 0, SEEK_END);
          s64 len = ftell(file);
          rewind(file);
          result = push_array(arena, char, len+1);
          fread(result, len, 1, file);
        */
#error "not impl"
#endif
    }
    
    return result;
}

















//~

inline void
platform_open_in_default_program(char *path_cstr)
{
    ShellExecuteA(NULL, "open", path_cstr, NULL, NULL, SW_SHOWNORMAL);
}

inline void
platform_open_in_default_program(Arena *temp_arena, Path path)
{
    arena_scope(temp_arena);
    char *path_cstr = to_cstr(temp_arena, path);
    platform_open_in_default_program(path_cstr);
}

inline void
platform_open_in_default_program(Arena *temp_arena, Directory directory)
{
    arena_scope(temp_arena);
    char *directory_cstr = to_cstr(temp_arena, directory);
    platform_open_in_default_program(directory_cstr);
}








function Path_List
platform_list_files_in_directory(Arena *arena, Directory directory)
{
#if Def_Windows
    Path_List result = {};
    
    Path wildcard_path = get_path(directory, lit2str("*"));
    
    u64 wildcard_len = get_path_string_length(wildcard_path);
    char *wildcard_cstr = push_stack_array(char, wildcard_len + 1);
    fill_buffer_from_path(wildcard_cstr, wildcard_len, wildcard_path);
    wildcard_cstr[wildcard_len] = 0;
    
    
    WIN32_FIND_DATAA data = {};
    HANDLE find_handle = FindFirstFileA(wildcard_cstr, &data);
    if (find_handle != INVALID_HANDLE_VALUE)
    {
        do
        {
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                String file_name_string = copy(arena, string(data.cFileName));
                *result.push(arena) = get_path(directory, file_name_string);
            }
        } while (FindNextFileA(find_handle, &data) != 0);
        FindClose(find_handle);
    }
    
#else
#error "not impl"
#endif
    return result;
}


function void
platform_files_delete_matching(Arena *temp_arena, Path wildcard_file_name_path)
{
    arena_scope(temp_arena);
    
#if Def_Windows
    char *search_cstr = to_cstr(temp_arena, wildcard_file_name_path);
    Path path_copy = wildcard_file_name_path;
    
    
    WIN32_FIND_DATAA data = {};
    HANDLE find_handle = FindFirstFileA(search_cstr, &data);
    if (find_handle != INVALID_HANDLE_VALUE)
    {
        do {
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                path_copy.file_name = string(data.cFileName);
                char *delete_cstr = to_cstr(temp_arena, path_copy);
                DeleteFileA(delete_cstr);
            }
        } while (FindNextFileA(find_handle, &data) != 0);
        FindClose(find_handle);
    }
    
#else
#error "not impl"
#endif
}


function void
platform_directory_delete_all_files(Arena *temp_arena, Directory directory)
{
    arena_scope(temp_arena);
    Path wildcard_path = get_path(directory, l2s("*"));
    platform_files_delete_matching(temp_arena, wildcard_path);
}






// =============== @Platform_Directory_Functions ==============
function Directory
platform_get_current_working_directory(Arena *arena)
{
    // NOTE(f0): ends with Native_Slash;
#if Def_Windows
    u32 buffer_size = GetCurrentDirectory(0, nullptr);
    char *buffer = push_array(arena, char, buffer_size);
    u32 length = GetCurrentDirectory(buffer_size, buffer);
    Directory result = directory_from_string(arena, string(buffer, length));
    
#else
    char buffer[kilobytes(4)]; // this is lame af. Define max linux path
    char *cwd = getcwd(buffer, array_count(buffer));
    if (cwd == buffer)
    {
        s64 length = cstr_length(buffer);
        if (length > 0)
        {
            result = push_array(arena, char, length + 2);
            copy_array(result, buffer, char, length);
            result[length] = Native_Slash_Char;
            result[length + 1] = 0;
        }
    }
#endif
    return result;
}

function Directory
platform_get_current_executable_directory(Arena *arena)
{
#if Def_Windows
    char buffer[kilobytes(4)];
    DWORD len = GetModuleFileNameA(nullptr, buffer, sizeof(buffer));
    String path = copy(arena, string(buffer, len));
    
    String path_no_file_name = trim_file_name_from_path(path);
    Directory result = directory_from_string(arena, path_no_file_name);
#else
#error "not impl"
#endif
    return result;
}

function Path
platform_get_current_executable_path(Arena *arena)
{
#if Def_Windows
    char buffer[kilobytes(4)];
    DWORD len = GetModuleFileNameA(nullptr, buffer, sizeof(buffer));
    String path = copy(arena, string(buffer, len));
    
    String path_no_file_name = trim_file_name_from_path(path);
    Directory dir = directory_from_string(arena, path_no_file_name);
    
    String file_name = file_name_from_path(path);
    Path result = get_path(dir, file_name);
    
#else
#error "not impl"
#endif
    return result;
}












// ====================== @Platform_Pipe ======================
inline b32
is_valid_handle(Pipe_Handle *pipe)
{
    b32 result = (pipe->handle_ != nullptr);
    return result;
}

inline b32
no_errors(Pipe_Handle *pipe)
{
    b32 result = pipe->no_error;
    return result;
}

inline void
set_error(Pipe_Handle *pipe,
          char *message = "", char *command_cstr = "")
{
    // TODO(f0): Optional error logging here? Stf0_Log_Io_Errors?
    printf("\n[pipe(%s) error] %s; ", command_cstr, message);
    debug_break();
    pipe->no_error = false;
}


function Pipe_Handle
platform_pipe_open_read(char *command)
{
#if Def_Windows
    Pipe_Handle result = {_popen(command, "rb")};
#else
    Pipe_Handle result = popen(command, "rb");
#endif
    result.no_error = is_valid_handle(&result);
    return result;
}



function s32
platform_pipe_close(Pipe_Handle *pipe)
{
#if Def_Windows
    s32 return_code = _pclose(pipe->handle_);
#else
    s32 return_code = pclose(pipe);
#endif
    return return_code;
}



function char *
platform_pipe_read_line(Pipe_Handle *pipe, char *buffer, s64 buffer_size)
{
    // NOTE: returns null when no data
    char *result = fgets(buffer, (s32)buffer_size, pipe->handle_);
    return result;
}








// =================== @Platform_Console_App ==================

#define ensure(Condition) do{\
if (!(Condition)){\
printf("[Internal error] %s; ", File_Line);\
exit_error();\
}}while(0)

#define ensure_msg(Condition, ErrorText1) do{\
if (!(Condition)){\
printf("[Error] %s; ", ErrorText1);\
exit_error();\
}}while(0)








#if Stf0_Wide_Char
function wchar_t *
platform_wchar_from_char(Arena *arena, char *cstr)
{
    s32 count1 = MultiByteToWideChar(CP_UTF8, 0, cstr, -1, 0, 0);
    ensure(count1 != ERROR_INSUFFICIENT_BUFFER && count1 != ERROR_INVALID_FLAGS &&
           count1 != ERROR_INVALID_PARAMETER && count1 != ERROR_NO_UNICODE_TRANSLATION);
    
    wchar_t *result = push_array(arena, wchar_t, count1);
    
    s32 count2 = MultiByteToWideChar(CP_UTF8, 0, cstr, -1, result, count1);
    ensure(count2 != ERROR_INSUFFICIENT_BUFFER && count2 != ERROR_INVALID_FLAGS &&
           count2 != ERROR_INVALID_PARAMETER && count2 != ERROR_NO_UNICODE_TRANSLATION);
    ensure(count1 == count2);
    
    return result;
}
#endif








function String_List
save_pipe_output(Arena *arena, Pipe_Handle *pipe)
{
    String_List result = {};
    char line_buffer[1024*8];
    while (platform_pipe_read_line(pipe, line_buffer, sizeof(line_buffer)))
    {
        String line = copy(arena, string(line_buffer, length_trim_white_reverse(line_buffer)));
        *result.push(arena) = line;
    }
    return result;
}





//=============================
struct File_Content
{
    String content;
    b32 no_error;
    u32 _padding;
};

inline b32
no_errors(File_Content *content)
{
    b32 result = content->no_error;
    return result;
}



function File_Content
platform_read_entire_file(Arena *arena, File_Handle *file)
{
    File_Content result = {};
    if (no_errors(file))
    {
        result.content.size = platform_file_get_size(file);
        result.no_error = true;
        
        if (result.content.size > 0)
        {
            result.content.str = push_array(arena, u8, result.content.size);
            u64 bytes_read = platform_file_read(file, result.content.str, result.content.size);
            
            if (bytes_read != result.content.size) {
                set_error(file, "Couldn't read whole file");
            }
            
            result.content.size = bytes_read;
        }
    }
    return result;
}

function File_Content
platform_read_entire_file(Arena *arena, char *file_path)
{
    File_Handle file = platform_file_open_read(file_path);
    File_Content result = platform_read_entire_file(arena, &file);
    platform_file_close(&file);
    return result;
}

inline File_Content
platform_read_entire_file(Arena *arena, Path path)
{
    char *path_cstr = to_cstr(arena, path);
    File_Content result = platform_read_entire_file(arena, path_cstr);
    return result;
}




//=============================
function File_Content
platform_read_entire_file_and_zero_terminate(Arena *arena, File_Handle *file)
{
    File_Content result = {};
    result.content.size = platform_file_get_size(file) + 1;
    result.content.str = push_array(arena, u8, result.content.size);
    
    if (no_errors(file))
    {
        u64 bytes_read = platform_file_read(file, result.content.str, result.content.size);
        
        if (bytes_read != result.content.size-1) {
            set_error(file, "Couldn't read whole file (zero terminate version)");
        } else {
            result.no_error = true;
            result.content.size -= 1;
            result.content.str[result.content.size] = 0;
        }
        
        assert(result.content.size >= bytes_read);
    }
    
    return result;
}

function File_Content
platform_read_entire_file_and_zero_terminate(Arena *arena, char *file_path)
{
    File_Handle file = platform_file_open_read(file_path);
    File_Content result = platform_read_entire_file_and_zero_terminate(arena, &file);
    platform_file_close(&file);
    return result;
}

inline File_Content 
platform_read_entire_file_and_zero_terminate(Arena *arena, Path path)
{
    char *path_cstr = to_cstr(arena, path);
    File_Content result = platform_read_entire_file_and_zero_terminate(arena, path_cstr);
    return result;
}




//~ Magic Print functions
inline void
magic_print(Arena *temp_arena, Directory dir)
{
    arena_scope(temp_arena);
    String dir_str = to_string(temp_arena, dir);
    printf("%.*s", string_expand(dir_str));
}

inline void
magic_print(Arena *temp_arena, Path path)
{
    arena_scope(temp_arena);
    String path_str = to_string(temp_arena, path);
    printf("%.*s", string_expand(path_str));
}





//===========================================================
//===========================================================
#endif // 50: Platform
#endif // 40: String_Alloc
#endif // 30: Memory
#endif // 21: Math
#endif // 20: Basic
#endif // 10: Types
Stf0_Close_Namespace
#endif // Stf0_H
