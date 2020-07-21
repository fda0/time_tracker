struct Memory_Arena
{
    size_t size;
    u8* base;
    size_t used;
};

internal void initialize_arena(Memory_Arena *arena, size_t size, u8 *base)
{
    arena->size = size;
    arena->base = base;
}


#define Push_Struct(Arena, Type) ((Type *)push_size_(Arena, sizeof(Type)))
#define Push_Array(Arena, Count, Type) ((Type *)push_size_(Arena, (Count) * sizeof(Type)))
void *push_size_(Memory_Arena *arena, size_t size)
{
    Assert((arena->used + size) <= arena->size);
    void *result = arena->base + arena->used;
    arena->used += size;
    return result;
}