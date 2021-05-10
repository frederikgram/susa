#include <fuse.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "lfs_controller.h"


// Definitions for file system operations 
int lfs_getattr ( const char *, struct stat * );
int lfs_readdir ( const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info * );
int lfs_open    ( const char *, struct fuse_file_info * );
int lfs_read    ( const char *, char *, size_t, off_t, struct fuse_file_info * );
int lfs_release ( const char *path, struct fuse_file_info *fi);
int lfs_mkdir   ( const char *, mode_t);
int lfs_write   ( const char *, const char *, size_t, off_t, struct fuse_file_info *);


// Defines operations for file operations to Fuse
static struct fuse_operations lfs_oper = {
	.getattr	= lfs_getattr,
	.readdir	= lfs_readdir,
	.mknod      = NULL,
	.mkdir      = lfs_mkdir,
	.unlink     = NULL,
	.rmdir      = NULL,
	.truncate   = NULL,
	.open	    = lfs_open,
	.read	    = lfs_read,
	.release    = lfs_release,
	.write      = lfs_write,
	.rename     = NULL,
	.utime      = NULL
};


int lfs_write(const char * path, const char * buffer, size_t size, off_t offset, struct fuse_file_info * fi) {
    printf("Trying to write to file at: '%s'\n", path);

    struct lfs_file * file = (struct lfs_file *) fi->fh;

    if (offset >= file->size + size) {
        return 0;
    }
    
    file->data = realloc(file->data, file->size + size);
    memcpy(file->data + offset, buffer, size);
    file->size = file-size + size;
    return size;
}

int lfs_mkdir(const char * path, mode_t mode) {
    printf("Trying to create a directory with path: '%s'\n", path);
    
    char * nonconst_path = strdup(path);
    
    /* Extract the name of the folder to be created from the absolute path */
    char * name = extract_last_segment(nonconst_path, '/');
    if (name == NULL) {
        printf("mkdir: name was null\n");
        return -1;
    }

    /* Extract the parents path and prepend the root path to it */
    char * parent_segments = extract_init_segments(nonconst_path, '/');
    char * parent_path = prepend_root(parent_segments);

    // Find the parent lfs_directory instance 
    struct lfs_directory * parent = find_directory(root_directory, parent_path);
    if (parent == NULL) {
        printf("could not find parent directory in mkdir\n");
        return -1;    
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
    printf("getattr: (abs path=%s)\n", abs_path);

    

    /* Instead of using this function, we could've simply
    extracted the parent path (the first n-1) segments of the given path,
    and found the directory thereto, and hereafter looked in that directories
    array of both files and directories, to find the structure we're looking for.

    However, we've opted to go for this approach, as wanted to utilize
    void pointer casting, to learn a little more about possible usecases.
    as this is for all intents and purposes, a learning experience. */

    void * found_struct = find_file_or_directory(abs_path, false);    
    if (found_struct == NULL) {
        printf("Getattr did not find struct for path:'%s'\n", abs_path);
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
        printf("Getattr not working on path:'%s'\n", abs_path);
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

	printf("readdir: (abs path=%s)\n", abs_path);

    struct lfs_directory * dir = find_directory(root_directory, abs_path);
    if (dir == NULL) { return -ENOENT; }

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
    void * found_struct = find_file_or_directory(abs_path, false);    

    /* The file we're trying to open does not exist. */
    if (found_struct == NULL || ((struct lfs_file *)found_struct)->mode != 33279) {
        printf("file not found\n");
        return -ENOENT;
    } 

    struct lfs_file * file = (struct lfs_file *) found_struct;
    fi->fh = (uint64_t) file;

	return 0;
}

int lfs_read( const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi ) {
    printf("read: (path=%s)\n", path);
     

    struct lfs_file * file = (struct lfs_file *) fi->fh;

    if (offset >= file->size + size) {
        return 0;
    }


    int bytes_to_read = file->size < size ? file->size : size;

    memcpy(buf, file->data + offset, bytes_to_read);
    printf("read %ld bytes from file '%s' representing the string '%s'\n", bytes_to_read, path, buf);
    return bytes_to_read;
      
}

int lfs_release(const char *path, struct fuse_file_info *fi) {
	printf("release: (path=%s)\n", path);
	return 0;
}

int main( int argc, char *argv[] ) {

    /* Setup root directory for our filesystem */
    root_directory = initialize_directory(NULL, "root");
    struct lfs_directory * home_dir      = initialize_directory(root_directory, "home");
    struct lfs_directory * fgk_dir       = initialize_directory(home_dir, "fgk");
    struct lfs_directory * lassan_dir    = initialize_directory(home_dir, "lassan");
    struct lfs_directory * videos_dir    = initialize_directory(fgk_dir, "videos");
    struct lfs_directory * downloads_dir = initialize_directory(fgk_dir, "downloads");


	fuse_main( argc, argv, &lfs_oper);


	return 0;
}
