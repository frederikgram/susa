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
int lfs_mkdir 	( const char * name);

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
	.write      = NULL,
	.rename     = NULL,
	.utime      = NULL
};


int lfs_mkdir(const char * name) {
    printf("%s\n",name);
    return -1;
}

// Prints the list of attributes of the file or of the files of the current directory
int lfs_getattr( const char *path, struct stat *stbuf ) {
	int res = 0;

    printf("getattr: (path=%s)\n", path);

	memset(stbuf, 0, sizeof(struct stat));
	if( strcmp( path, "/" ) == 0 ) {
		stbuf->st_mode  = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if( strcmp( path, "/hello" ) == 0 ) {
		stbuf->st_mode  = S_IFREG | 0777;
		stbuf->st_nlink = 1;
		stbuf->st_size  = 12;
	} else
		res = -ENOENT;

	return res;
}

int lfs_readdir( const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi ) {
	(void) offset;
	(void) fi;
	printf("readdir: (path=%s)\n", path);

	if(strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, "hello", NULL, 0);

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
    struct lfs_directory * home_dir = initialize_directory(root_directory, "home");
    struct lfs_directory * fgk_dir = initialize_directory(home_dir, "fgk");
    struct lfs_directory * downloads_dir = initialize_directory(fgk_dir, "downloads");
    struct lfs_directory * videos_dir = initialize_directory(fgk_dir, "videos");


    struct lfs_directory * found_dir = find_directory(root_directory, "/root/home/fgk/videos");
    printf("%s\n", found_dir->name);

	fuse_main( argc, argv, &lfs_oper );


	return 0;
}
