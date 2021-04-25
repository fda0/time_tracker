struct Parse_Date_Result
{
    date64 date;
    b32 is_valid;
};

struct Parse_Complex_Date_Result
{
    date64 date;
    Status status;
};

struct Parse_Time_Result
{
    s32 value;
    b32 is_valid;
};

struct Parse_Number_Result
{
    s32 value;
    b32 is_valid;
};
