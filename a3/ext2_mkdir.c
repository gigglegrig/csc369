#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <zconf.h>
#include <string.h>
#include <errno.h>
#include "helper.h"

int main(int argc, char **argv) {
    check_argc("Usage: ext2_mkdir <image file name> <absolute path on ext2 disk>\n", argc, 3);
    check_path_format(argv[2]);
    read_disk(argv[1]);    

    //if the original absolute path ends with '/', we need to delete the '/'.
    int path_len = strlen(argv[2]);
    char original_pathname[255];
    strncpy(original_pathname, argv[2],path_len);
    original_pathname[path_len] = '\0';
    int idx = path_len - 1;
    while(original_pathname[idx] == '/' && idx >= 0){
    	original_pathname[idx] = '\0';
        idx --;
    }

    //parsing the target path and get the name of the new directory and the path
    char * target_pathname = NULL;
    char * target_dirname = NULL;	
    split_last_part_of_path(original_pathname, &target_pathname, &target_dirname);

    //when the absolute path is '/', target_dirname is NULL
    if (target_dirname == NULL) {
        fprintf(stderr, "File exists\n");
	free(target_pathname);
        free(target_dirname);       
        return EEXIST;
    }
    //check the name length of the new directory
    if(strlen(target_dirname) > NAME_MAX){
       fprintf(stderr, "File name too long.\n");
       free(target_pathname);
       free(target_dirname);
       return ENAMETOOLONG;
    }

    //check whether the path and the directory exist
    struct ext2_inode * tpath_inode;
    struct ext2_inode * find_result;
    tpath_inode = get_inode_by_path(root_inode, target_pathname);
    find_result = find_file(tpath_inode, target_dirname);

    // if the path dose not exist return the error message
    if(tpath_inode == NULL) {
        fprintf(stderr, "No such file or directory.\n");
        free(target_pathname);
        free(target_dirname);	
        return ENOENT;
    }
    // if the directory already exists, return the error message 
    if(find_result != NULL && find_result -> i_mode & EXT2_S_IFDIR){
        fprintf(stderr, "File exists.\n");
	free(target_pathname);
        free(target_dirname);
        return EEXIST;
    }

    //start create new directory
    //find a new inode
    unsigned int inum = find_free_inode();
    if(inum == 0){// run out of space
        return ENOSPC;
    }
    struct ext2_inode * newdir_inode = NUM_TO_INODE(inum);
    memset(newdir_inode, 0, sb->s_inode_size);
     
    newdir_inode -> i_mode = EXT2_S_IFDIR;
    newdir_inode -> i_size = EXT2_BLOCK_SIZE;//The new directory uses one block
    newdir_inode -> i_links_count = 2;
    unsigned int timestamp = current_time();
    newdir_inode -> i_atime = timestamp;
    newdir_inode -> i_ctime = timestamp;
    newdir_inode -> i_mtime = timestamp;
     
    //add new directory entry to the parent directory's block
    
    add_dir_entry_to_block(tpath_inode, inum, EXT2_FT_DIR, target_dirname);
    tpath_inode -> i_links_count += 1;
    tpath_inode -> i_mtime = timestamp;

    //add. and ..to the new directory    
    add_dir_entry_to_block(newdir_inode, inum, EXT2_FT_DIR, ".");
    add_dir_entry_to_block(newdir_inode, INODE_TO_NUM(tpath_inode), EXT2_FT_DIR, "..");
    
    free(target_pathname);
    free(target_dirname);
   
    return 0;

}
