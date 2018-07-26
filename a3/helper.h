#include "ext2.h"

#ifndef CSC369_HELPER_H
#define CSC369_HELPER_H

#define BLOCK(x) (disk + EXT2_BLOCK_SIZE * (x))
#define PAD(x) ((x) + ((4 - ((x) % 4)) % 4))
#define MAX12 (12 + EXT2_BLOCK_SIZE / sizeof(int))
#define IBLOCKS(x) (((x)->i_blocks) / (EXT2_BLOCK_SIZE / 512))
#define INODE_TO_NUM(x) ((struct ext2_inode *) (x) + 1 - (struct ext2_inode *) BLOCK(gd->bg_inode_table))
#define NUM_TO_INODE(x) ((struct ext2_inode *) BLOCK(gd->bg_inode_table) + (x) - 1)
#define INODE_IFDIR(x) ((x)->i_mode & EXT2_S_IFDIR)
#define CHECKDOT(x) ((x)->name_len == 1 && (x)->name[0] == '.')
#define CHECKDOTDOT(x) ((x)->name_len == 2 && (x)->name[0] == '.' && (x)->name[1] == '.')

int disk_fd;
unsigned char * disk; // Disk
struct ext2_super_block *sb; // Super block
int group_num; // Block group number
struct ext2_group_desc *gd; // (First) Group descriptor
char * curr_dir_name;
struct ext2_inode * root_inode;
//struct ext2_dir_entry_2 * prev_dir_entry = NULL;

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
unsigned int find_free_block();
unsigned int find_free_inode();
void add_block_to_inode(struct ext2_inode *inode, unsigned int block);
void add_dir_entry_to_block(struct ext2_inode *dir_inode, unsigned int inode, unsigned char file_type, char *name);
int get_block_from_inode(struct ext2_inode *inode, unsigned num);
void set_dir_entry(struct ext2_dir_entry_2 * dir, unsigned int inode, unsigned short reclen, unsigned char file_type, char * name);

// Experiments
typedef int (*dirFunc)(struct ext2_dir_entry_2 *, int, long *);
//int directory_block_iterator(int block_num, dirFunc func, int argc, long * args);
int directory_block_iterator(struct ext2_inode * dir_inode, dirFunc func, int argc, long * args);
int add_dir_entry(struct ext2_dir_entry_2 * dir, int argc, long * args);
int remove_dir_entry(struct ext2_dir_entry_2 * dir, int argc, long * args);
int count_subfolder(struct ext2_dir_entry_2 * dir, int argc, long * args);

#endif //CSC369_HELPER_H
