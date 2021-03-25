

enum Token_Type
{
    Token_Unknown,
    
    Token_Semicolon,
    
    Token_String,
    Token_Identifier,
    Token_Date,
    Token_Time,
    Token_Offset,
    
    Token_End_Of_Stream
};



struct Line
{
    u8 *current_line_start;
    u32 row;
};


struct Token
{
    Token_Type type;
    String text;
    Line line;
};

struct Lexer
{
    u8 *at;
    
#define Max_Token_Peek 8
    Token tokens[32];
    u32 current_token_index;
    u32 next_refresh_start_index;
    
    u32 line_row;
};