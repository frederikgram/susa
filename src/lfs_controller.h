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

    mode_t mode;
    struct lfs_directory * parent_dir;
    char * name;
    char * data;
    bool read_only;
    struct fuse_file_info * fi;
    uid_t owner_id;
    unsigned int size;
    unsigned int created_at;
    unsigned int last_accessed;
    unsigned int last_modified;
    
} lfs_file;

/* Structure defining in-memory defition of a directory */
typedef struct lfs_directory {

    mode_t mode;
    struct lfs_directory * parent_dir;
    char * name;
    char * path;
    
    /* Pointers to child files and directories
    residing inside this directory */
    struct lfs_file * files[STD_DIRECTORY_FILES_SIZE];
    int num_files;

    struct lfs_directory * directories[STD_DIRECTORY_TABLE_SIZE];
    int num_directories;

    unsigned int created_at;
    unsigned int last_accessed;
    unsigned int last_modified;

} lfs_directory;



struct lfs_file * find_file(struct lfs_directory * parent, char * name) {

    for (int i = 0; i < parent->num_files; i++) {
        if (parent->files[i] != NULL && strcmp(parent->files[i]->name, name) == 0) {
            return parent->files[i];
        }
    }

    return NULL;
}

struct lfs_directory * find_directory(struct lfs_directory * current_dir, char * path) {
    
    printf("current_dir name = %s\n", current_dir->name);

    /* If the given path is empty, we know that
    we're inside the correct directory */
    if (strcmp(path, "") == 0 || strcmp(path, current_dir->name) == 0) {
        printf("Given directory: '%s' is also the current directory and was successfully found\n", current_dir->name);
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


void * find_file_or_directory(char * path) {
    

    struct lfs_directory * last_dir = root_directory; 
    struct lfs_directory * current_dir = root_directory;
    
    char * nonconst_path = strdup(path);
    char * segment = strtok(strdup(nonconst_path), "/");
    char * tail = nonconst_path + strlen(segment) + 1;

    printf("total path: %s\n", nonconst_path);

    while (true) {
        printf("current segment: %s\n", nonconst_path);

        current_dir = find_directory(current_dir, segment);
        if (current_dir == NULL) {
            printf("current_dir is null, trying to return file\n");
            return (void*) find_file(last_dir, segment);

        } else { last_dir = current_dir; }
        
        segment = strtok(tail, "/");
        if (segment == NULL) {
            break;
        } else { tail = tail + strlen(segment) + 1; }
    }
    
    printf("trying to return directory\n");
    return (void*) last_dir;
}

/* Shorthands for setting up a file or directories */
struct lfs_file * initialize_file(struct lfs_directory * parent, char * name, char * buffer,
                    size_t size, struct fuse_file_info * fi) {


    printf("Initializing file with name: '%s'\n", name);
    struct lfs_file * file = malloc(sizeof(struct lfs_file));
    file->parent_dir = parent;
    file->name = name;
    memcpy(&file->data, &buffer, size);
    file->size = size;
    file->mode = S_IFREG | 0777;
    file->fi = fi;
    
    file->created_at    = (unsigned long)time(NULL);
    file->last_accessed  = file->created_at; 
    file->last_modified = file->created_at;


    parent->files[parent->num_files] = file; 
    parent->num_files++;

    return file;
    //S_IFREG | 0777;

}

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



    dir->created_at    = (unsigned long)time(NULL);
    dir->last_accessed  = dir->created_at; 
    dir->last_modified = dir->created_at;

    dir->mode = S_IFDIR | 0755;
    dir->name = name;
    dir->num_files = 0;
    dir->num_directories = 0;
    return dir; 
}
