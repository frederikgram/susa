/* Logic and abstractions for use in lfs.c */

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



/* Prepends "root" onto any given string */
char * prepend_root(char * str) {

    char * pstr = (char *) malloc (strlen(str) + 5);
    strcpy(pstr, "root");
    strcat(pstr, str);
    return pstr;

}

/* Fetches the last segment of a given string, that can
   be separated by using the given delimiter */
char * extract_last_segment(char * str, char delim) {
    char * name = strrchr(str, delim) + 1;
    return name;
}

/* Extracts all segments but the last, that can
   be created by using the given delimiter on the given string */
char * extract_init_segments(char * str, char delim) {

    char * last_segment = extract_last_segment(str, delim)  ;
    char * init_segments = (char *) malloc (strlen(str) - strlen(last_segment) );
    strncat(init_segments, str, (strlen(str) - strlen(last_segment) - 1));
    return init_segments;
}


struct lfs_file * find_file(struct lfs_directory * parent, char * name) {
    for (int i = 0; i < parent->num_files; i++) {
        if (parent->files[i] != NULL && strcmp(parent->files[i]->name, name) == 0) {
            return parent->files[i];
        }
    }

    return NULL;
}



/* Removes a file from its parent directory
and free's all the memory allocated in connection
to the initialization of said file.

Following the deletion, we shift all files in
the parent directory one to the left, as to
avoid having any leading NULL's in the parent
directories array of files.
*/
int remove_file(struct lfs_file * file) {
    bool shift_left = false;
    for (int i = 0; i < file->parent_dir->num_files; i++) {
        if (strcmp(file->parent_dir->files[i]->name, file->name) == 0) {
            file->parent_dir->files[i] = NULL;
            shift_left = true;
        } else {
            if (shift_left) {
                file->parent_dir->files[i-1] = file->parent_dir->files[i];
                file->parent_dir->files[i] = NULL;
            }
        }
    }

    file->parent_dir->num_files -= 1;
    
    free(file->data);
    free(file);
    return 0;

}


/* Recursively rmove a a directory and all
files and directories inside of it, and in turn,
inside their children etc.

Following this, free all the memory allocated in connection
to the initialization of said directory.

Hereafter  shift all files in
the parent directory one to the left, as to
avoid having any leading NULL's in the parent
directories array of directories.
*/
int remove_directory(struct lfs_directory * dir) {

    bool shift_left = false;
    for (int i = 0; i < dir->parent_dir->num_directories; i++) {
        if (strcmp(dir->parent_dir->directories[i]->name, dir->name) == 0) {
            dir->parent_dir->directories[i] = NULL;
            shift_left = true;
        } else {

            if (shift_left) {
                dir->parent_dir->directories[i-1] = dir->parent_dir->directories[i];
                dir->parent_dir->directories[i] = NULL;
            }
        }
    }

    dir->parent_dir->num_directories -= 1;
    
    for (int i = 0; i < dir->num_directories; i++) {
        remove_directory(dir->directories[i]);
    }

    for (int i = 0; i < dir->num_files; i++) {
        free(dir->files[i]->data);
        free(dir->files[i]);
    }
    
    free(dir);
    return 0;

}

/* Removes a directory using remove_directory
if and only if the directory is empty. */
int remove_directory_if_empty(struct lfs_directory * dir) {
    if (dir->num_files == 0 && dir->num_directories == 0) {
        remove_directory(dir);
    } else {
        return -ENOTEMPTY;
    }

    return 0;
    
}


/* Recursively attempts to find a directory at the 
given path, taking basis in the given directory.
If no directory was found, return NULL. */
struct lfs_directory * find_directory(struct lfs_directory * current_dir, char * path) {
    
    /* If the given path is empty (NULL), we know that
    we're inside the correct directory */
    if (path == NULL || strcmp(path, "") == 0){
        
        #ifdef DEBUG
            printf("find_directory: Returning from find_directory with current dir name :%s\n", current_dir->name);
        #endif

        return current_dir;
    }

    char * nonconst_path = strdup(path);
    char * segment = strtok(strdup(path), "/");
    char * tail; 
    
    /* If tail would be out of bounds wrt. the path we've been given
    we set it to NULL instead */
    if (strlen(nonconst_path) <= strlen(segment)) {
        tail = NULL;
    } else {
        tail = nonconst_path + strlen(segment) + 1;
    }
   
    /* Handle Absolute paths wrt. /root/ */
    if (strcmp(segment, "root") == 0) {
        return find_directory(current_dir, tail);
    }

    /* Try to find the next path segment inside the current
    directories directory table  */
    for (int i = 0; i < current_dir->num_directories; i++) {
        if (strcmp(segment, current_dir->directories[i]->name) == 0) {
            return find_directory(current_dir->directories[i], tail);
        }
    }

    return NULL;  
}


/* Attempt to find either a file or directory
from the given path, and return it as 
a void pointer, which can then be re-cast

This function works iteratively, in contrast
to find_directory - for showcase purposes 

See lfs.c - lfs_getattr for the reason behind
this overly complex void pointer stuff
*/

void * find_file_or_directory(char * path) {
    

    struct lfs_directory * last_dir = root_directory; 
    struct lfs_directory * current_dir = root_directory;
    

    char * name = extract_last_segment(path, '/');
    char * segment = strtok(strdup(path), "/");
    char * tail = path + strlen(segment) + 1;

    /* While true is bad, use it anyways */
    while (true) {

        /* Attempt to find a child directory with the current segment as name */
        current_dir = find_directory(current_dir, segment);

        if (current_dir == NULL) {
            struct lfs_file * file = find_file(last_dir, segment);

            /* If we found a file at the current path,
            but not a folder, return the file */
            if (file != NULL) {
                return (void *) file;

            } else if (strcmp(last_dir->name, name) == 0){
                return (void *) last_dir;
            } else {
                return NULL;
            }
        }

        last_dir = current_dir;
        segment = strtok(tail, "/");
        if (segment == NULL) {
            break;
        } else {
            tail = tail + strlen(segment) + 1;
        }
    }
    
    return (void*) last_dir;
}

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
