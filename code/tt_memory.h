struct Memory_Arena
{
    size_t size;
    u8* base;
    size_t used;
    u32 count;
};


inline void clear_arena(Memory_Arena *arena)
{
    arena->used = 0;
    arena->count = 0;
}

internal void alocate_arena(Memory_Arena *arena, size_t size)
{
    arena->base = (u8 *)malloc(size);
    if (arena->base == NULL)
    {
        printf("Failed to allocate memory\n");
        exit(1);
    }
    
    arena->size = size;
    clear_arena(arena);
}


// NOTE(mateusz): Gives back aligned pointer with size allocated for any type.
#define Push_Struct(Arena, Type) \
((Type *)push_size_aligned_(Arena, sizeof(Type), alignof(Type), 1))

// NOTE(mateusz): Gives back aligned pointer with size allocated for array of any type.
#define Push_Array(Arena, Count, Type) \
((Type *)push_size_aligned_(Arena, (Count) * sizeof(Type), alignof(Type), (u32)Count))

// NOTE(mateusz): Internal raw allocation call.
void *push_size_aligned_(Memory_Arena *arena, size_t size, u32 aligment, u32 count)
{
    arena->count += count;
    
    size_t needed_size = (arena->used + size);
    if (needed_size > arena->size)
    {
        size_t new_size = needed_size*2;
        
        arena->base = (u8 *)realloc(arena->base, new_size);
        if (arena->base == NULL)
        {
            printf("Failed to allocate memory\n");
            exit(1);
        }
        
        arena->size = new_size;
    }
    
    size_t aligned_used = arena->used + (arena->used % aligment);
    
    void *result = arena->base + aligned_used;
    arena->used = aligned_used + size;
    return result;
}