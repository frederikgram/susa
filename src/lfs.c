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


    return 0;

}

int lfs_mkdir(const char * path, mode_t mode) {
    printf("Trying to create a directory with path: '%s'\n", path);
    
    char * nonconst_path = strdup(path);
    
    // Extract the name of the folder to be created from the absolute path
    char * name = strrchr(nonconst_path, '/') + 1;
    if (name == NULL) {
        printf("mkdir: name was null\n");
        return -1;
    }

    // Extract the parents path from the absolute path
    char * parent_path = (char *) malloc (strlen(nonconst_path) - strlen(name) + 4);
    strcpy(parent_path, "root");
    strncat(parent_path, nonconst_path, (strlen(nonconst_path) - strlen(name) - 1));

    printf("mkdir folder name: %s\n", name);
    printf("mkdir parent name: %s\n", parent_path);

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
	if( strcmp( path, "/" ) == 0 ) {
		stbuf->st_mode  = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
        return res;
	}
    
    char * nonconst_path = strdup(path);
    char * abs_path = (char *) malloc(6 + strlen(path));
    strcpy(abs_path, "root");
    strcat(abs_path, path);
    
    

    
    void * found_struct = find_file_or_directory(abs_path);    
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
        stbuf->st_uid = file->owner_id;
        stbuf->st_gid = file->owner_id;
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
	return 0;
}

int lfs_read( const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi ) {
    printf("read: (path=%s)\n", path);
     
	memcpy( buf, "Hello\n", 6 );
	return 6;
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
    struct lfs_directory * pornstash_dir = initialize_directory(downloads_dir, "pornstash");


	fuse_main( argc, argv, &lfs_oper);


	return 0;
}
