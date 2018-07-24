#include "ext2.h"

#ifndef CSC369_HELPER_H
#define CSC369_HELPER_H

int disk_fd;
unsigned char * disk; // Disk
struct ext2_super_block *sb; // Super block
int group_num; // Block group number
struct ext2_group_desc *gd; // (First) Group descriptor
char * curr_dir_name;
struct ext2_inode * root_inode;

struct block {
    unsigned char byte[EXT2_BLOCK_SIZE];
};


void check_argc(char * usage, int in, int target);
void check_path_format(char * path);
void read_disk(char *);
struct ext2_inode * get_inode_by_num(int);
int check_inode_directory(struct ext2_inode *);
struct ext2_inode * find_file(struct ext2_inode *, char *);
struct ext2_inode * get_inode_by_path(struct ext2_inode *, char *);
void set_bit(char bori, int location, unsigned char value);
int get_bit(char bori, int location);
void split_last_part_of_path(char *, char **, char **);
int find_free_block();
int find_free_inode();
void add_block_to_inode(struct ext2_inode *inode, int block);

#endif //CSC369_HELPER_H
