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

int lfs_mkdir(const char * name, mode_t mode) {
    printf("%s\n",name);
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
        return -ENOENT;
    }    

    // Found a directory
    if (((struct lfs_directory * )found_struct)->mode == 16877) {
        stbuf->st_mode = 16877;
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
        return -ENOENT;
    }
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


    char * nonconst_path = strdup(path);
    char * abs_path = (char *) malloc(6 + strlen(path));
    strcpy(abs_path, "root");
    strcat(abs_path, path);

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
    struct lfs_directory * home_dir = initialize_directory(root_directory, "home");
    struct lfs_directory * fgk_dir = initialize_directory(home_dir, "fgk");
    struct lfs_directory * lassan_dir = initialize_directory(home_dir, "lassan");
    struct lfs_directory * downloads_dir = initialize_directory(fgk_dir, "downloads");
    struct lfs_directory * videos_dir = initialize_directory(fgk_dir, "videos");

   /* 
    printf("===Test initialization of a nested file===\n");
    char * tmp_buffer = "lolololo";
    struct lfs_file * new_file = initialize_file(videos_dir, "myfile.elf", tmp_buffer, strlen(tmp_buffer) + 1, NULL);
    printf("> filename: %s\n", new_file->name);
    printf("> filedata: %s\n", new_file->data);
    printf("> parent name: %s\n", new_file->parent_dir->name);
    printf("> own name from parent: %s\n", videos_dir->files[0]->name);

    printf("===Test dynamic casting of void pointers with a file ===\n");
    void * result = find_file_or_directory("root/home/fgk/videos/myfile.elf");
    printf("> mode cast from file: %d\n", ((struct lfs_file *) result)->mode);
    printf("> mode cast from dir: %d\n", ((struct lfs_directory *) result)->mode);

    printf("===Test dynamic casting of void pointers with a directory ===\n");
    result = find_file_or_directory("root/home/fgk/videos/");
    printf("> mode cast from file: %d\n", ((struct lfs_file *) result)->mode);
    printf("> mode cast from dir: %d\n", ((struct lfs_directory *) result)->mode);

    printf("===Test dynamic casting of void pointers with a second directory ===\n");
    result = find_file_or_directory("root/home/lassan");
    printf("> mode cast from file: %d\n", ((struct lfs_file *) result)->mode);
    printf("> mode cast from dir: %d\n", ((struct lfs_directory *) result)->mode);
    // struct file_or_dir
        // is_dir
        // void *

    // struct lfs_directory * found_dir = find_directory(root_directory, "/root/home/fgk/videos");
*/
	fuse_main( argc, argv, &lfs_oper);


	return 0;
}
