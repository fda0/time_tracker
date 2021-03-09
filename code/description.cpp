




inline u64
get_description_hash(Description *desc)
{
    assert(desc); // TODO(f0): real hash function...
    u64 hash = 12354145 * desc->size;
    for_u32(i, desc->size)
    {
        hash <<= 5;
        hash ^= (hash + desc->str[i]);
    }
    return hash;
}



inline void
insert_element_(Description_Table *table, Description *desc)
{
    u64 hash = get_description_hash(desc);
    u64 mask = (table->capacity_count - 1);
    u64 key = hash & mask;
    
    Description *candidate = table->array + key;
    
    for (;;)
    {
        if (candidate->hash == 0)
        {
            table->element_count += 1;
            break;
        }
        else if (candidate->hash == hash)
        {
#if Def_Slow
            String table_str = string(candidate->str, candidate->size);
            String desc_str = string(desc->str, desc->size);
            assert(string_equal(table_str, desc_str));
#endif
            break;
        }
        
        key = (key + 1) & mask;
        candidate = table->array + key;
    }
}


inline String
string_from_desc(Description *desc)
{
    String result = {desc->str, desc->size};
    return result;
}


inline Description *
get_description(Description_Table *table, u64 hash)
{
    u64 key = hash & (table->capacity_count - 1);
    Description *result = &global_stubs.description;
    
    for (;;)
    {
        Description *candidate = table->array + key;
        if (candidate->hash == 0)
        {
            break;
        }
        else if (candidate->hash == hash)
        {
            result = candidate;
            break;
        }
    }
    
    return result;
}



internal Description_Table
create_description_table(u64 initial_capacity_count = 4096)
{
    assert(is_non_zero_power_of_two(initial_capacity_count));
    
    Description_Table result = {};
    result.capacity_count = initial_capacity_count;
    
    u64 memory_size = sizeof(Description) * result.capacity_count;
    result.arena = create_virtual_arena(memory_size);
    push_bytes_virtual_commit_unaligned_(&result.arena, memory_size); 
    
    return result;
}



internal void
description_table_expand(Description_Table *old_table)
{
    u64 new_capacity_count = (old_table->capacity_count << 1);
    assert(is_non_zero_power_of_two(new_capacity_count));
    
    if (new_capacity_count > old_table->capacity_count)
    {
        Description_Table new_table = create_description_table(new_capacity_count);
        
        for_u64(element_index, old_table->capacity_count)
        {
            Description *desc = old_table->array + element_index;
            if (desc->hash != 0)
            {
                insert_element_(&new_table, desc);
            }
        }
        
        free_virtual_arena(&old_table->arena);
        *old_table = new_table;
    }
    else
    {
        assert(0);
    }
}


inline void
add_element(Description_Table *table, Description *desc)
{
    insert_element_(table, desc);
    
    if (table->element_count > table->max_element_count)
    {
        description_table_expand(table);
    }
}
