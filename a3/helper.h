#include "ext2.h"

#ifndef CSC369_HELPER_H
#define CSC369_HELPER_H

unsigned char * disk; // Disk
struct ext2_super_block *sb; // Super block
int group_num; // Block group number
struct ext2_group_desc *gd; // (First) Group descriptor
char * curr_dir_name;
struct ext2_inode * root_inode;


void check_argc(char * usage, int in, int target);
void read_disk(char *);
struct ext2_inode * get_inode_by_num(int);
int check_inode_directory(struct ext2_inode *);
struct ext2_inode * find_file(struct ext2_inode *, char *);
struct ext2_inode * get_inode_by_path(struct ext2_inode *, char *);

#endif //CSC369_HELPER_H
