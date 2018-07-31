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
    
    //parsing the target path and get the name of the new directory and the path
    char * target_pathname = NULL;
    char * target_dirname = NULL;

    int path_len = strlen(argv[2]);
    char original_pathname[255];
    strncpy(original_pathname, argv[2],path_len);
    original_pathname[path_len] = '\0';
    int idx = path_len - 1;
    while(original_pathname[idx] == '/' && idx >= 0){
    	original_pathname[idx] = '\0';
        idx --;
    }
    printf("%s orig_name\n",original_pathname);


    split_last_part_of_path(original_pathname, &target_pathname, &target_dirname);
    
    printf("%s pathname\n",target_pathname);
    
    printf("%s directory\n",target_dirname);
    
    //check whether the path exists, check whether the directory exists
    struct ext2_inode * tpath_inode;
    struct ext2_inode * find_result;
    tpath_inode = get_inode_by_path(root_inode, target_pathname);
    find_result = find_file(tpath_inode, target_dirname);
    if(tpath_inode == NULL) {// if the path dose not exist return the error message
        fprintf(stderr, "Path does not exist.\n");
        return ENOENT;
    }
    if(find_result != NULL && find_result -> i_mode & EXT2_S_IFDIR){// if the directory already exists, return the error message 
        fprintf(stderr, "Directory already exist.\n");
        return EEXIST;
    }
    
   
    //find a new inode
    unsigned int inum = find_free_inode();
    if(inum == 0){// run out of space
        return ENOSPC;
    }
    struct ext2_inode * newdir_inode = NUM_TO_INODE(inum);
    memset(newdir_inode, 0, sb->s_inode_size);
    
    //find a new block and set directory entry 
    //unsigned int bnum = find_free_block();
    //struct block * tblock = (struct block *) BLOCK(bnum);
    //struct ext2_dir_entry_2 * newdir = (struct ext2_dir_entry_2 *) tblock;  

    newdir_inode -> i_mode = EXT2_S_IFDIR;
    newdir_inode -> i_size = PAD(8 + (int) strlen(target_dirname));//in bytes???
    newdir_inode -> i_links_count = 2;
    add_dir_entry_to_block(tpath_inode, inum, EXT2_FT_DIR, target_dirname);
}
