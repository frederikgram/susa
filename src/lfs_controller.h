/* .... */

#include <string.h>
#include <stdbool.h>

#define STD_DIRECTORY_TABLE_SIZE 1024
#define STD_DIRECTORY_FILES_SIZE 1024

typedef struct lfs_file lfs_file;
typedef struct lfs_directory  lfs_directory;

struct lfs_directory * root_directory;


/* Structure defining in-memory definition of a file */
typedef struct lfs_file {

    struct lfs_directory * parent_dir;

    char * name;
    char * data;

    bool read_only;
      
    unsigned int size;
    unsigned int created_at;
    unsigned int last_accessed;
    unsigned int last_modified;
    
} lfs_file;

/* Structure defining in-memory defition of a directory */
typedef struct lfs_directory {

    struct lfs_directory * parent_dir;
    char * name;
    char * path;
    
    /* Pointers to child files and directories
    residing inside this directory */
    struct lfs_file * files[STD_DIRECTORY_FILES_SIZE];
    int num_files;

    struct lfs_directory * directories[STD_DIRECTORY_TABLE_SIZE];
    int num_directories;

} lfs_directory;

/* Start of utility functions */

/*
    /root/home/fgk/videos/porn.mp3 
*/
struct lfs_file * find_file_from_path(char * path, char * name) {

    

};

struct lfs_file * find_file_from_directory(struct lfs_directory * current_dir, char * name) {
    for (int i = 0; i < current_dir->num_files; i++) {
        if (strcmp(name, current_dir->files[i]->name) == 0) {
            return current_dir->files[i];
        }
    } 
    return NULL;
}


struct lfs_directory * find_directory(struct lfs_directory * current_dir, char * path) {
    
    printf("current_dir name = %s\n", current_dir->name);

    /* If the given path is empty, we know that
    we're inside the correct directory */
    if (strcmp(path, "") == 0 || strcmp(path, current_dir->name) == 0) {
        printf("Given directory: '%s' is also the current directory\n", current_dir->name);
        return current_dir;
    }

    char * nonconst_path = strdup(path);
    char * segment = strtok(strdup(path), "/");


    char * tail = nonconst_path + strlen(segment) + 1;

    /* Handle Absolute paths wrt. /root/ */
    if (strcmp(segment, current_dir->name) == 0) {
        printf("Searching for root inside root\n");
        return find_directory(current_dir, tail);

    }

    printf("path = %s\n", nonconst_path);
    printf("segment = %s\n", segment);
    printf("tail = %s\n\n\n----\n\n", tail);
    

    /* Try to find the next path segment inside the current
    directories directory table  */
    for (int i = 0; i < current_dir->num_directories; i++) {
        printf("Looking at current directories sub-dir: %d called: %s\n", i, current_dir->directories[i]->name);
        if (strcmp(segment, current_dir->directories[i]->name) == 0) {
            return find_directory(current_dir->directories[i], tail);
        }
    }

    return NULL;  
}

/* Shorthand for setting up a directory pointer */
struct lfs_directory * initialize_directory(struct lfs_directory * parent, char * name) {
    struct lfs_directory * dir;
    dir = malloc(sizeof(struct lfs_directory));

    dir->parent_dir = parent;

    if (parent == NULL) {
        dir->path = "root";
    } else {
        dir->path = strdup(parent->path);
        strcat(dir->path, "/");
        strcat(dir->path, name);

        parent->directories[parent->num_directories] = dir;
        parent->num_directories++; 

    }
    printf("Initialized directory with path: '%s'\n", dir->path);

    dir->name = name;
    dir->num_files = 0;
    dir->num_directories = 0;
    return dir; 
}
