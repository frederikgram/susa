#include <fuse.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "lfs_controller.h"


// Definitions for file system operations 
int lfs_getattr  ( const char *, struct stat * );
int lfs_readdir  ( const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info * );
int lfs_open     ( const char *, struct fuse_file_info * );
int lfs_read     ( const char *, char *, size_t, off_t, struct fuse_file_info * );
int lfs_release  ( const char *path, struct fuse_file_info *fi);
int lfs_mkdir    ( const char *, mode_t);
int lfs_write    ( const char *, const char *, size_t, off_t, struct fuse_file_info *);
int lfs_rmdir    ( const char *);
int lfs_mknod    ( const char *, mode_t, dev_t);
int lfs_truncate ( const char *, off_t);
int lfs_utime    (const char * , const struct timespec ts[2]);
int lfs_unlink   (const char *);

// Defines operations for file operations to Fuse
static struct fuse_operations lfs_oper = {
	.getattr	= lfs_getattr,
	.readdir	= lfs_readdir,
	.mknod      = lfs_mknod,
	.mkdir      = lfs_mkdir,
	.rmdir      = lfs_rmdir,
    .unlink     = lfs_unlink,
	.truncate   = lfs_truncate,
	.open	    = lfs_open,
	.read	    = lfs_read,
	.release    = lfs_release,
	.write      = lfs_write,
	.utime      = lfs_utime,
};



int lfs_unlink(const char * path) {
    char * nonconst_path = strdup(path);
    
    #ifdef DEBUG
        printf("unlink: Trying to unlink (remove) the file at path '%s'\n", path);
    #endif
    
    /* Extract the name of the file to be created from the absolute path */
    char * name = extract_last_segment(nonconst_path, '/');
    if (name == NULL) {
        fprintf(stderr, "unlink: Could not extract name from the given path '%s'\n", path);
        return -EINVAL;
    }

    /* Extract the parents path and prepend the root path to it */
    char * parent_segments = extract_init_segments(nonconst_path, '/');
    char * parent_path = prepend_root(parent_segments);

    // Find the parent lfs_directory instance 
    struct lfs_directory * parent = find_directory(root_directory, parent_path);
    if (parent == NULL) {
        fprintf(stderr, "unlink: Could not find a parent folder for new entry at path '%s'\n", path);
        return -ENOENT;    
    } 
    
    struct lfs_file* file = find_file(parent, name);
    if (file == NULL) {
        return -ENOENT;
    }        

    remove_file(file);
    
    return 0;
}

int lfs_utime(const char * path, const struct timespec ts[2]) {
    char * nonconst_path = strdup(path);
    
    /* Extract the name of the file to be created from the absolute path */
    char * name = extract_last_segment(nonconst_path, '/');
    if (name == NULL) {
        fprintf(stderr, "utime: Could not extract name from the given path '%s'\n", path);
        return -EINVAL;
    }

    #ifdef DEBUG
        printf("utime: Trying to modify the access and modification times of the entry at path '%s' \
               to the values: %ld, %ld respectively\n", path, ts[0].tv_sec, ts[1].tv_sec);
    #endif

    /* Extract the parents path and prepend the root path to it */
    char * parent_segments = extract_init_segments(nonconst_path, '/');
    char * parent_path = prepend_root(parent_segments);

    // Find the parent lfs_directory instance 
    struct lfs_directory * parent = find_directory(root_directory, parent_path);
    if (parent == NULL) {
        fprintf(stderr, "utime: Could not find a parent folder for new entry at path '%s'\n", path);
        return -ENOENT;    
    } 
    
    struct lfs_file* file = find_file(parent, name);
    if (file == NULL) {
        return -ENOENT;
    }        

    file->last_accessed = ts[0].tv_sec;
    file->last_modified = ts[1].tv_sec;

    return 0;
    
}

int lfs_truncate(const char * path, off_t size) {
    char * nonconst_path = strdup(path);
    
    /* Extract the name of the file to be created from the absolute path */
    char * name = extract_last_segment(nonconst_path, '/');
    if (name == NULL) {
        fprintf(stderr, "truncate: Could not extract name from the given path '%s'\n", path);
        return -EINVAL;
    }

    /* Extract the parents path and prepend the root path to it */
    char * parent_segments = extract_init_segments(nonconst_path, '/');
    char * parent_path = prepend_root(parent_segments);

    // Find the parent lfs_directory instance 
    struct lfs_directory * parent = find_directory(root_directory, parent_path);
    if (parent == NULL) {
        fprintf(stderr, "truncate: Could not find a parent folder for new entry at path '%s'\n", path);
        return -ENOENT;    
    } 
    
    struct lfs_file* file = find_file(parent, name);
    if (file == NULL) {
        return -ENOENT;
    }        

     
    file->data = realloc(file->data, size);
    file->size = size;
    file->last_modified = file->last_accessed;

    return 0;


}


int lfs_mknod(const char * path, mode_t mode, dev_t rdev) {

    char * nonconst_path = strdup(path);
    
    /* Extract the name of the file to be created from the absolute path */
    char * name = extract_last_segment(nonconst_path, '/');
    if (name == NULL) {
        fprintf(stderr, "mkdir: Could not extract name from the given path '%s'\n", path);
        return -1;
    }

    /* Extract the parents path and prepend the root path to it */
    char * parent_segments = extract_init_segments(nonconst_path, '/');
    char * parent_path = prepend_root(parent_segments);

    // Find the parent lfs_directory instance 
    struct lfs_directory * parent = find_directory(root_directory, parent_path);
    if (parent == NULL) {
        fprintf(stderr, "mknod: Could not find a parent folder for new entry at path '%s'\n", path);
        return -ENOENT;
    } 
        
    initialize_file(parent, name);
    return 0;
}

int lfs_rmdir(const char * path) {

    char * nonconst_path = strdup(path);
    char * abs_path = (char *) malloc(6 + strlen(path));
    strcpy(abs_path, "root");
    strcat(abs_path, path);

    struct lfs_directory * dir = find_directory(root_directory, abs_path);
    if (dir == NULL) {
        fprintf(stderr, "rmdir: Could not find any directory to remove at path '%s'\n", path);
        return -ENOENT;
    }

    return remove_directory_if_empty(dir);
}

int lfs_write(const char * path, const char * buffer, size_t size, off_t offset, struct fuse_file_info * fi) {

    #ifdef DEBUG
        printf("write: Trying to write to %ld bytes to file at: '%s'\n", size, path);
    #endif

    /* Fetch the pointer to the file, inserted into the
    fuse_file_info struct during lfs_open */
    struct lfs_file * file = (struct lfs_file *) fi->fh;

    if (file == NULL) {
        fprintf(stderr, "write: Could not find any file at path '%s'\n", path);
        return -ENOENT;
    }

/*
    if (offset >= file->size + size) {
        fprintf(stderr, "write: Could not write at offset '%ld' as it would be out of bounds\n", offset);
        return 0;
    }
*/
    #ifdef DEBUG
        printf("write: Resizing file at path '%s' to be '%ld' bytes long\n", path, file->size + size + offset);
    #endif
    
    int new_size;
    if (offset + size > file->size) {
        new_size = file->size + abs(offset+size - file->size);
    } else {
        new_size = file->size + size - offset;
    }

    file->data = realloc(file->data, new_size);
    memcpy(file->data + offset, buffer, size);

    file->size = new_size;
    file->last_modified = file->last_accessed;
    return size;

}

int lfs_mkdir(const char * path, mode_t mode) {

    #ifdef DEBUG
        printf("mkdir: Trying to create a directory with path: '%s'\n", path);
    #endif
    
    char * nonconst_path = strdup(path);
    
    /* Extract the name of the folder to be created from the absolute path */
    char * name = extract_last_segment(nonconst_path, '/');
    if (name == NULL) {
        fprintf(stderr, "mkdir: Could not extract name of folder from the given path '%s'\n", path);
        return -EINVAL;
    }

    /* Extract the parents path and prepend the root path to it */
    char * parent_segments = extract_init_segments(nonconst_path, '/');
    char * parent_path = prepend_root(parent_segments);

    // Find the parent lfs_directory instance 
    struct lfs_directory * parent = find_directory(root_directory, parent_path);
    if (parent == NULL) {
        fprintf(stderr, "mkdir: Could not find a parent folder for the new directory at path '%s'\n", path);
        return -ENOENT;    
    } 
        
    initialize_directory(parent, name);

    return 0;
}

// Prints the list of attributes of the file or of the files of the current directory
int lfs_getattr( const char *path, struct stat *stbuf ) {
	int res = 0;

    printf("getattr: (path=%s)\n", path);

	memset(stbuf, 0, sizeof(struct stat));
    
	if( strcmp( path, "/" ) == 0) {
		stbuf->st_mode  = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
        return res;
	}

    char * nonconst_path = strdup(path);
    char * abs_path = (char *) malloc(6 + strlen(path));
    strcpy(abs_path, "root");
    strcat(abs_path, path);

    

    /* Instead of using this function, we could've simply
    extracted the parent path (the first n-1) segments of the given path,
    and found the directory thereto, and hereafter looked in that directories
    array of both files and directories, to find the structure we're looking for.

    However, we've opted to go for this approach, as wanted to utilize
    void pointer casting, to learn a little more about possible usecases.
    as this is for all intents and purposes, a learning experience. */

    void * found_struct = find_file_or_directory(abs_path);    
    if (found_struct == NULL) {
        fprintf(stderr, "getattr: Could not find an entry (file or directory) at path '%s'\n", path);
        return -ENOENT;
    }    

    // Found a directory
    if (((struct lfs_directory * )found_struct)->mode == 16877) {
        struct lfs_directory * dir = ((struct lfs_directory * )found_struct);

        stbuf->st_mode = 16877;
        stbuf->st_atime = dir->last_accessed;
        stbuf->st_mtime = dir->last_modified;
        stbuf->st_nlink = 2;

    } else if (((struct lfs_file * )found_struct)->mode == 33279) {
        struct lfs_file * file = ((struct lfs_file * )found_struct);

        stbuf->st_mode = file->mode;
        stbuf->st_size = file->size;
        stbuf->st_atime = file->last_accessed;
        stbuf->st_mtime = file->last_modified;
        stbuf->st_nlink = 1;
    } else {
        fprintf(stderr, "getattr: Encountered and unexpected error while trying to find attributes for entry at path '%s'\n", path);
        return -ENOENT;
    }

    return res;

}

int lfs_readdir( const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi ) {
	(void) offset;
	(void) fi;
	printf("readdir: (path=%s)\n", path);

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

    char * nonconst_path = strdup(path);
    char * abs_path = (char *) malloc(6 + strlen(path));
    strcpy(abs_path, "root");
    strcat(abs_path, path);


    struct lfs_directory * dir = find_directory(root_directory, abs_path);
    if (dir == NULL) {
        fprintf(stderr, "readdir: Could not find a directory at path '%s' to read\n", path);
        return -ENOENT;
    }

    for (int i = 0; i < dir->num_directories; i++) {
        filler(buf, dir->directories[i]->name, NULL, 0);
    }

    for (int i = 0; i < dir->num_files; i++) {
        filler(buf, dir->files[i]->name, NULL, 0);
    }

	return 0;
}

//Permission
int lfs_open( const char *path, struct fuse_file_info *fi ) {

    printf("open: (path=%s)\n", path);

    char * nonconst_path = strdup(path);
    char * abs_path = prepend_root(nonconst_path);
    void * found_struct = find_file_or_directory(abs_path);    

    /* The file we're trying to open does not exist. */
    if (found_struct == NULL || ((struct lfs_file *)found_struct)->mode != 33279) {
        fprintf(stderr, "open: file at path '%s' could not be found\n", path);
        return -ENOENT;
    } 

    struct lfs_file * file = (struct lfs_file *) found_struct;
    file->last_accessed = time(NULL);
    fi->fh = (uint64_t) file;

	return 0;
}

int lfs_read( const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi ) {
    printf("read: (path=%s)\n", path);
     

    /* Fetch the pointer to the file, inserted into the
    fuse_file_info struct during lfs_open */
    struct lfs_file * file = (struct lfs_file *) fi->fh;
    if (offset + size >= file->size + size) {
        return 0;
    }

    int bytes_to_read = file->size < size ? file->size : size;
    memcpy(buf, file->data + offset, bytes_to_read);
    printf("read %d bytes from file '%s'\n", bytes_to_read, path);
    return bytes_to_read;
      
}

int lfs_release(const char *path, struct fuse_file_info *fi) {
	printf("release: (path=%s)\n", path);

    /* Since our filesystem is purely in-memory, and we don't utilize
    any temporary allocations for interacting with files, we don't need
    release functionality, so because of that we left this empty. */

	return 0;
}

int main( int argc, char *argv[] ) {

    /* Setup root directory for our filesystem */
    root_directory = initialize_directory(NULL, "root");

    /* Initialization of folders for testing purposes */
    #ifdef DEBUG
        struct lfs_directory * home_dir      = initialize_directory(root_directory, "home");
        struct lfs_directory * fgk_dir       = initialize_directory(home_dir, "fgk");
        struct lfs_directory * lassan_dir    = initialize_directory(home_dir, "lassan");
        struct lfs_directory * videos_dir    = initialize_directory(fgk_dir, "videos");
        struct lfs_directory * downloads_dir = initialize_directory(fgk_dir, "downloads");
    #endif


	fuse_main( argc, argv, &lfs_oper);


	return 0;
}
