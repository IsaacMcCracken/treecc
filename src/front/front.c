#include <front/front.h>

typedef struct {
    SrcFiles files;
} State;

extern State s = { 0 };

State s; // important global variable

SrcFiles *get_src_files(void) {
    return &s.files;
}

void add_src_file(String filename) {
    os_rwmutex_lock(s.files.lock, RWMutexMode_Write)

    // get file data
    SrcHashEntry *entry = arena_push(s.file.arena, SrcHashEntry);
    String text = os_read_entire_file(s.file.arena, filename);
    U32 token_count = 0;
    Token *tokens = tokenize(s.files.arena, &token_count, text.ptr, text.len);

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
        s.files.tail.next = entry;
        s.files.tail = entry;
    } else {
        s.files.tail = entry;
        s.files.head = entry;
    }

    os_rwmutex_unlock(s.files.lock);
}

void frontend_init(void) {
    Arena *arena = arena_init(GIGABYTE(8), ArenaFlag_LargePage);
    s.files.lookup = arena_push_array(arena, SrcHashEntry*, 1009);
    s.files.entry_cap = 1009;
    s.files.arena = arena;
    s.files.lock = os_rwmutex_alloc();
    os_rwmutex_unlock(s.files.lock);
}

void frontend_deinit(void) {
    arena_deinit(s.files.arena);
    os_rwutex_free(s.files.lock);
}
