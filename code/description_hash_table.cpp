
inline u64
get_string_hash(String text)
{
    // TODO(f0): real hash function...
    u64 hash = 5381;
    for_u32(i, text.size)
    {
        hash = ((hash << 5) + hash) + text.str[i];
    }
    return hash;
}


internal Description_Table
create_desc_table(Arena *arena, u64 minimal_size)
{
    Description_Table result = {};
    
    if (minimal_size > 0)
    {
        u64 msb = find_most_significant_bit(minimal_size).index;
        result.entry_max_count = (1LL << (msb + 1LL));
        result.mask = (result.entry_max_count - 1);
        result.entries = push_array_clear(arena, Description_Entry, result.entry_max_count);
    }
    
    return result;
}


internal void
add_desc_data(Description_Table *table, String full_text, s32 time)
{
    String short_text = full_text;
    Find_Index find_space = index_of(short_text, ' ');
    if (find_space.found) {
        short_text.size = find_space.index;
    }
    
    u64 mask = table->mask;
    u64 hash = get_string_hash(short_text);
    u64 key = hash & mask;
    
    Description_Entry *entry = table->entries + key;
    
    for (;;)
    {
        b32 entry_slot_found = false;
        
        if (entry->hash == 0)
        {
            entry->hash = hash;
            entry->text = short_text.str;
            entry->text_size = safe_truncate_to_u32(short_text.size);
            entry->time_sum = 0;
            entry->count = 0;
            
            table->unique_count += 1;
            entry_slot_found = true;
        }
        else if (entry->hash == hash)
        {
            String table_str = string(entry->text, entry->text_size);
            if (equals(table_str, short_text))
            {
                entry_slot_found = true;
            }
            else
            {
                debug_break(); // NOTE(f0): Hash collision
            }
        }
        
        if (entry_slot_found)
        {
            entry->text_was_truncated = (entry->text_was_truncated ||
                                         (full_text.size != short_text.size));
            entry->time_sum += time;
            entry->count += 1;
            break;
        }
        
        key = (key + 1) & mask;
        entry = table->entries + key;
    }
}

