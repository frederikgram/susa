
#include <string.h>
#include <stdbool.h>

#define STD_FILE_SYSTEM_SIZE     20480

typedef struct HASHTABLE_NODE {

    mode_t mode; 
    void * entry;
    char * path;

    bool is_linked;
    struct HASHTABLE_NODE * next;
    
} HASHTABLE_NODE;

typedef struct HASHTABLE {
    int size;
    struct HASHTABLE_NODE * table[STD_FILE_SYSTEM_SIZE];

} HASHTABLE;

// Djb2 hash function
unsigned long djb2_hash(char *str) {
        unsigned long hash = 5381;
        int c;
        while ((c = *str++))
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        return hash;
}

/* Creates a HASHTABLE_NODE from a given path and entry and inserts
it into the given HASHTABLE using a computed hash. Handles collisions in a 
linked-list manner. */
int add_entry_to_table(struct HASHTABLE * table, char * path, void * entry) {

    unsigned long index = djb2_hash(path) % table->size;
    
    /* Create a new HASHTABLE_NODE instance */
    struct HASHTABLE_NODE * this = (struct HASHTABLE_NODE * ) malloc(sizeof(struct HASHTABLE_NODE));

    /* We can do this as mode is the first field in both the
    lfs_file and the lfs_directory structs, so it doens't change
    as it has a strictly defined size mode_t */

    this->mode = ((struct lfs_file * ) entry)->mode;
    this->entry = entry;
    this->path = path;
    this->is_linked = 0;
    this->next = NULL;


    /* Collision, handle it using linked-list like behavior */
    if (table->table[index] != NULL) {
        struct HASHTABLE_NODE * node = table->table[index];
        while (node->is_linked == 1) { node = node->next; }
        node->is_linked = 1;
        node->next = this;
    } else {
        table->table[index] = this;
    }
    
    return 0;
}

