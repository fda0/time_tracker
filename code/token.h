
struct Lines
{
    u8 *current_line_start;
    u64 count;
};

struct Lexer
{
    u8 *at;
    
    Lines lines;
};


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

struct Token
{
    Token_Type type;
    String text;
};

struct Forward_Token
{
    Lexer *lexer_;
    Lexer peek_lexer_;
    Token token;
    Token peek;
};
