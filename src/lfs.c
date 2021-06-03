#include <fuse.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "structs.h"
#include "hash.h"
#include "search.h"

struct HASHTABLE * fs_table;


// Definitions for file system opeunction ‘add_entry_to_table’:
int lfs_getattr  ( const char *, struct stat * );
int lfs_readdir  ( const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info * );
int lfs_open     ( const char *, struct fuse_file_info * );
int lfs_read     ( const char *, char *, size_t, off_t, struct fuse_file_info * );
int lfs_release  ( const char *, struct fuse_file_info * );
int lfs_mkdir    ( const char *, mode_t);
int lfs_write    ( const char *, const char *, size_t, off_t, struct fuse_file_info *);
int lfs_mknod    ( const char *, mode_t, dev_t);

// Defines operations for file operations to Fuse
static struct fuse_operations lfs_oper = {
	.getattr	= lfs_getattr,
	.readdir	= lfs_readdir,
	.mknod      = lfs_mknod,
	.mkdir      = lfs_mkdir,
	.rmdir      = NULL,
    .unlink     = NULL,
	.truncate   = NULL,
	.open	    = NULL,
	.read	    = NULL,
	.release    = NULL,
	.write      = NULL,
	.utime      = NULL,
    .destroy    = NULL,
};


int lfs_write(const char * path, const char * buffer, size_t size, off_t offset, struct fuse_file_info * fi) {
    /* Fetch the pointer to the file, inserted into the
    fuse_file_info struct during lfs_open */
    struct lfs_file * file = (struct lfs_file *) fi->fh;

    if (file == NULL) {
        fprintf(stderr, "write: Could not find any file at path '%s'\n", path);
        return -ENOENT;
    }

    int new_size;
    if (offset + size > file->size) {
        new_size = file->size + offset+size - file->size;
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

    printf("mkdir: (path=%s)\n", path);

    /* Extract the first n-1 and the nth segment from the given path */
    char * path_dup = strdup(path);
    char * last_segment = strrchr(path_dup, '/') + 1;
    char * init_segments = (char *) malloc (strlen(path_dup) - strlen(last_segment));
    strncat(init_segments, path_dup, (strlen(path_dup) - strlen(last_segment)) - 1);

    /* if mkdir is trying to create a node in the root of the mount-point,
    we manually specify the path of the parent to be "/",
    otherwise we use n-1 segments from the given path, delimited by '/'
    as the parent path */
    char * parent_path = strcmp(init_segments, "") == 0 ? "/" : init_segments;
    struct HASHTABLE_NODE * node = find_node_from_path(fs_table, parent_path);
    
    /* No node was found at the parent path*/
    if (node == NULL) { return -ENOENT; } 
    
    /* The node at the parent path is not a directory, thus we cannot use it as a parent */
    if (node->mode != 16877) {
        return -ENOENT;
    }
   
    struct lfs_directory * parent = (struct lfs_directory * ) node->entry;

    /* Initialize the new file and add it to the file system */
    struct lfs_dir * dir = initialize_directory(parent, last_segment);
    add_entry_to_table(fs_table, path_dup, (void *) dir);
    return 0;
}

int lfs_mknod(const char * path, mode_t mode, dev_t rdev) {

    printf("mknod: (path=%s)\n", path);

    /* Extract the first n-1 and the nth segment from the given path */
    char * path_dup = strdup(path);
    char * last_segment = strrchr(path_dup, '/') + 1;
    char * init_segments = (char *) malloc (strlen(path_dup) - strlen(last_segment));
    strncat(init_segments, path_dup, (strlen(path_dup) - strlen(last_segment)) - 1);

    /* if mknod is trying to create a node in the root of the mount-point,
    we manually specify the path of the parent to be "/",
    otherwise we use n-1 segments from the given path, delimited by '/'
    as the parent path */
    char * parent_path = strcmp(init_segments, "") == 0 ? "/" : init_segments;
    struct HASHTABLE_NODE * node = find_node_from_path(fs_table, parent_path);
    
    /* No node was found at the parent path*/
    if (node == NULL) { return -ENOENT; } 
    
    /* The node at the parent path is not a directory, thus we cannot use it as a parent */
    if (node->mode != 16877) {
        return -ENOENT;
    }
   
    struct lfs_directory * parent = (struct lfs_directory * ) node->entry;

    /* Initialize the new file and add it to the file system */
    struct lfs_file * file = initialize_file(parent, last_segment);
    add_entry_to_table(fs_table, path_dup, (void *) file);
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

    struct HASHTABLE_NODE * node = find_node_from_path(fs_table, path);
    
    /* No node was found with the given path */
    if (node == NULL) { return -ENOENT; } 

    /* Node is a directory */
    if (node->mode == 16877) {
        stbuf->st_mode = 16877;
        struct lfs_directory * dir = (struct lfs_directory *) node->entry;
        stbuf->st_atime = dir->last_accessed;
        stbuf->st_mtime = dir->last_modified;
        stbuf->st_nlink = 2;

    /* Node is a file */
    } else if (node->mode == 33279){
        struct lfs_file * file = (struct lfs_file *) node->entry;
        stbuf->st_mode = file->mode;
        stbuf->st_size = file->size;
        stbuf->st_atime = file->last_accessed;
        stbuf->st_mtime = file->last_modified;
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

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

    struct HASHTABLE_NODE * node = find_node_from_path(fs_table, path);
    
    /* No node was found with the given path */
    if (node == NULL) { return -ENOENT; } 
    
    /* The node at the given path is not a directory, we we cannot read it as one */
    if (node->mode != 16877) {
        return -ENOENT;
    }

    /* Extract the lfs_directory struct from the HASHTABLE_NODE 
    and iterate over its children */
    struct lfs_directory * dir = (struct lfs_directory *) node->entry;
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

    struct HASHTABLE_NODE * node = find_node_from_path(fs_table, path);
    
    /* No node was found with the given path */
    if (node == NULL) { return -ENOENT; } 

    /* The node at the given path is a directory and can thus not be opened */
    if (node->mode == 16877) {
        return -ENOENT;
    }

    struct lfs_file * file = (struct lfs_file *) node->entry;
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
	return 0;
}

int main( int argc, char *argv[] ) {

    /* Setup Initial Filesystem Table */
    fs_table = (struct HASHTABLE *) malloc(sizeof(struct HASHTABLE));
    fs_table->size = STD_FILE_SYSTEM_SIZE; 

    /* Setup Initial Filesystem root directory*/
    struct lfs_directory * root_directory = initialize_directory(NULL, "/");
    add_entry_to_table(fs_table, "/", (void *) root_directory);

	fuse_main( argc, argv, &lfs_oper);

	return 0;
}
