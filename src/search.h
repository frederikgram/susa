#include <string.h>
#include <stdbool.h>


struct HASHTABLE_NODE * find_node_from_path(struct HASHTABLE * table, char * str) {

    unsigned long hash = djb2_hash(str);
    unsigned long clamped = hash % STD_FILE_SYSTEM_SIZE;

    /* No node with the given path exists in the filesystem */
    if (table->table[clamped] == NULL) {
        return NULL;
    }

    struct HASHTABLE_NODE * node = table->table[clamped];
    
    /* A node was found, this index has had no collisions
    and can simply be returned as is. */
    if (node->is_linked == 0) {
        return node;

    /* We found a node at an index where there has been collisions,
    iterate through the linked-list to find the node fitting our
    specific path and return it. */
    } else {
        while (strcmp(node->path, str) == 1) {
            if (node->is_linked == 1) {
                node = node->next;
            } else {
                return NULL;
            }
        }
        return node;
    }   

    return NULL;
} 

