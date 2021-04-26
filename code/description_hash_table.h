struct Description_Entry
{
    u64 hash;
    
    u8 *text;
    
    u32 text_size;
    s32 time_sum;
    
    u32 count;
    u32 padding_;
};

struct Description_Table
{
    Description_Entry *entries;
    u64 entry_count;
    u64 mask;
    
    u64 unique_count;
};
