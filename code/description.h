
struct Description
{
    u64 hash;
    
    u8 *str;
    u32 size;
    
    b32 is_marked;
};

struct Description_Table
{
    union {
        Arena arena;
        Description *array;
    };
    u64 capacity_count;
    u64 element_count;
    u64 max_element_count;
    Description empty_description;
};
