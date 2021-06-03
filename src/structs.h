
#include <string.h>
#include <stdbool.h>

#define STD_DIRECTORY_TABLE_SIZE 1024
#define STD_DIRECTORY_FILES_SIZE 4096

typedef struct lfs_file lfs_file;
typedef struct lfs_directory  lfs_directory;
struct lfs_directory * root_directory;


typedef struct lfs_file {

    mode_t mode;
    struct lfs_directory * parent_dir;
    char * name;
    char * data;

    unsigned int size;
    time_t created_at;
    time_t last_accessed;
    time_t last_modified;
    
} lfs_file;

typedef struct lfs_directory {

    mode_t mode;
    struct lfs_directory * parent_dir;
    char * name;
    
    /* Pointers to child files and directories
    residing inside this directory */
    struct lfs_file * files[STD_DIRECTORY_FILES_SIZE];
    struct lfs_directory * directories[STD_DIRECTORY_TABLE_SIZE];

    int num_files;
    int num_directories;

    time_t created_at;
    time_t last_accessed;
    time_t last_modified;

} lfs_directory;


/* Shorthand for setting up a file */
struct lfs_file * initialize_file(struct lfs_directory * parent, char * name) {


    #ifdef DEBUG
        printf("Initializing file with name: '%s'\n", name);
    #endif

    struct lfs_file * file = malloc(sizeof(struct lfs_file));

    file->parent_dir = parent;
    file->name = name;
    file->data = (char *) malloc(0);
    file->size = 0;
    file->mode = S_IFREG | 0777;
    file->created_at    = time(NULL);
    file->last_accessed = file->created_at; 
    file->last_modified = file->created_at;

    parent->files[parent->num_files] = file; 
    parent->num_files++;

    return file;

}

/* Shorthand for setting up a directory */
struct lfs_directory * initialize_directory(struct lfs_directory * parent, char * name) {
    struct lfs_directory * dir;
    dir = malloc(sizeof(struct lfs_directory));

    dir->parent_dir = parent;

    if (parent != NULL) {
        parent->directories[parent->num_directories] = dir;
        parent->num_directories++; 
    }

    #ifdef DEBUG
        printf("Initialized directory with name: '%s'\n", name);
    #endif

    dir->created_at    = time(NULL);
    dir->last_accessed = dir->created_at; 
    dir->last_modified = dir->created_at;

    dir->mode = S_IFDIR | 0755;
    dir->name = name;
    dir->num_files = 0;
    dir->num_directories = 0;
    return dir; 
}
