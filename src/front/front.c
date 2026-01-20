#include <front/front.h>

typedef struct {
    SrcFiles files;
} State;

State s = { 0 };

SrcFiles *get_src_files(void) {
    return &s.files;
}

SrcFile *add_src_file(String filename) {
    os_rwmutex_lock(s.files.lock, RWMutexMode_Write);

    // get file data
    SrcHashEntry *entry = arena_push(s.files.arena, SrcHashEntry);
    String text = os_read_entire_file(s.files.arena, filename);
    U32 token_count = 0;
    Token *tokens = tokenize(s.files.arena, &token_count, (Byte*)text.ptr, text.len);

    SrcFile file = (SrcFile){
        .name = filename,
        .text = text,
        .tokens = tokens,
        .tok_count = token_count,
    };

    entry->file = file;

    // put file into hash map

    U64 hash = string_hash(filename);
    U64 index = hash % s.files.entry_cap;
    SrcHashEntry **slot = &s.files.lookup[index];

    while (*slot) {
        slot = &(*slot)->next_hash;
    }

    // insertion;
    *slot = entry;


    // add to file list
    if (s.files.tail) {
        s.files.tail->next = entry;
        s.files.tail = entry;
    } else {
        s.files.tail = entry;
        s.files.head = entry;
    }

    os_rwmutex_unlock(s.files.lock);

    return &entry->file;
}

SrcFile *get_src_file(String filename) {
    os_rwmutex_lock(s.files.lock, RWMutexMode_Read);
    SrcFile *file = 0;
    U64 hash = string_hash(filename);
    U64 index = hash % s.files.entry_cap;

    SrcHashEntry *slot = s.files.lookup[index];

    while (slot) {
        if (string_cmp(filename, slot->file.name) == 0) {
            file = &slot->file;
            break;
        }

        slot = slot->next_hash;
    }




    os_rwmutex_unlock(s.files.lock);
    return file;
}

void frontend_init(void) {
    Arena *arena = arena_init(GIGABYTE(8));
    s.files.lookup = arena_push_array(arena, SrcHashEntry*, 1009);
    s.files.entry_cap = 1009;
    s.files.arena = arena;
    s.files.lock = os_rwmutex_alloc();
    // os_rwmutex_unlock(s.files.lock);
}

void frontend_deinit(void) {
    arena_deinit(s.files.arena);
    os_rwmutex_free(s.files.lock);
}
