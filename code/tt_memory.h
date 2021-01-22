struct Memory_Arena
{
    size_t size;
    u8 *base;
    size_t used;
};


inline void
clear_arena(Memory_Arena *arena)
{
    arena->used = 0;
}

internal void
alocate_arena(Memory_Arena *arena, u8 *memory_address, size_t size)
{
    arena->base = memory_address;
    if (arena->base == NULL)
    {
        printf("Failed to allocate memory\n");
        exit(1);
    }
    
    arena->size = size;
    clear_arena(arena);
}


internal size_t
get_aligment_offset(Memory_Arena *arena, size_t aligment)
{
    size_t aligment_offset = 0;
    size_t pointer = (size_t)arena->base + arena->used;
    
    size_t aligment_mask = aligment - 1;
    if (pointer & aligment_mask)
    {
        aligment_offset = aligment - (pointer & aligment_mask);
    }
    
    return aligment_offset;
}

// NOTE: Gives back aligned pointer with size allocated for any type.
#define Push_Struct(Arena, Type) ((Type *)push_size_aligned_(Arena, sizeof(Type), alignof(Type)))

// NOTE: Gives back aligned pointer with size allocated for array of any type.
#define Push_Array(Arena, Count, Type) ((Type *)push_size_aligned_(Arena, (Count) * sizeof(Type), alignof(Type)))

// NOTE: Internal raw allocation call.
void *
push_size_aligned_(Memory_Arena *arena, size_t size, s64 aligment)
{
    size_t needed_size = (arena->used + size);
    if (needed_size > arena->size)
    {
        // TODO: (possible in 64bit)
        // 1. Use VirtualAlloc and allocate memory in "high" address space.
        // 2. Get more (continuous) memory pages when out of memory.
        printf("Program ran out of memory\n");
        exit(1);
    }
    
    
    size_t aligment_offset = get_aligment_offset(arena, aligment);
    void *result = arena->base + arena->used + aligment_offset;
    
    size_t effective_size = size + aligment_offset;
    arena->used += effective_size;
    
    return result;
}


template <typename T> struct Dynamic_Array
{
    Memory_Arena arena_;
    s64 count;
    
    void initialize(u8 *memory_address, size_t size)
    {
        alocate_arena(&arena, memory_address, size);
    }
    
    void clear()
    {
        clear_arena(&arena_);
        count = 0;
    }
    
    
    //~ allocate
    
    T *push_struct()
    {
        T *result = Push_Struct(&arena_, T);
        ++count;
        
        return result;
    }
    
    T *push_array(s64 element_count)
    {
        T *result = Push_Array(&arena_, element_count, T);
        count += element_count;
        
        return result;
    }
    
    
    
    //~ get
    
    T *at(s64 index)
    {
        T *result = ((T *)arena_.base) + index;
        
#if BUILD_INTERNAL
        if (!(index >= 0 && index < count))
        {
            Invalid_Code_Path;
        }
#endif
        
        return result;
    }
    
    
    
    //~ add
    
    T *add(T *element)
    {
        T *result = push_struct();
        *result = *element;
        return result;
    }
    
    
    T *insert_at(T *element, s64 index)
    {
        T *new_elem = push_struct();
        
        T *prev = new_elem;
        
        for (s64 data_index = count - 2; data_index >= index; --data_index)
        {
            T *old_data = at(data_index);
            *prev = *old_data;
            prev = old_data;
        }
        
        *prev = *element;
        return prev;
    }
};
