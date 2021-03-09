




inline u64
get_string_hash(String string)
{
    // TODO(f0): real hash function...
    
    u64 hash = 12354145 * string.size;
    for_u32(i, string.size)
    {
        hash <<= 5;
        hash ^= (hash + string.str[i]);
    }
    return hash;
}



inline u64
insert_description_(Description_Table *table, String content)
{
    u64 hash = get_string_hash(content);
    u64 mask = (table->capacity_count - 1);
    u64 key = hash & mask;
    
    Description *candidate = table->array + key;
    
    for (;;)
    {
        if (candidate->hash == 0)
        {
            candidate->hash = hash;
            candidate->str = content.str;
            candidate->size = safe_truncate_to_u32(content.size);
            candidate->is_marked = false;
            
            table->element_count += 1;
            break;
        }
        else if (candidate->hash == hash)
        {
#if Def_Slow
            String table_str = string(candidate->str, candidate->size);
            assert(string_equal(table_str, content));
#endif
            break;
        }
        
        key = (key + 1) & mask;
        candidate = table->array + key;
    }
    
    return hash;
}


inline String
string_from_description(Description *desc)
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
                insert_description_(&new_table, string(desc->str, desc->size));
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


inline u64
add_description(Description_Table *table, String content)
{
    u64 hash = insert_description_(table, content);
    
    if (table->element_count > table->max_element_count) {
        description_table_expand(table);
    }
    
    return hash;
}


inline u64
add_description(Description_Table *table, Token token)
{
    assert(token.type == Token_String);
    u64 hash = add_description(table, string(token.text, token.text_length));
    return hash;
}
    
    
    
    
    
    