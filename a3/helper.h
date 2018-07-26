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
struct ext2_group_desc *gd; // (First) Group descriptor
char * curr_dir_name;
struct ext2_inode * root_inode;

struct block {
    unsigned char byte[EXT2_BLOCK_SIZE];
};


void check_argc(char *, int, int);
void check_path_format(char *);
void read_disk(char *);
struct ext2_inode * find_file(struct ext2_inode *, char *);
struct ext2_inode * get_inode_by_path(struct ext2_inode *, char *);
void set_bit(char, int, unsigned char);
int get_bit(char, int);
void split_last_part_of_path(char *, char **, char **);
unsigned int find_free_block();
unsigned int find_free_inode();
void add_block_to_inode(struct ext2_inode *, unsigned int);
void add_dir_entry_to_block(struct ext2_inode *, unsigned int, unsigned char, char *);
int get_block_from_inode(struct ext2_inode *, unsigned);
void set_dir_entry(struct ext2_dir_entry_2 *, unsigned int, unsigned short, unsigned char, char *);

// dirFunc functions
typedef int (*dirFunc)(struct ext2_dir_entry_2 *, int, long *);
int add_dir_entry(struct ext2_dir_entry_2 *, int, long *);
int remove_dir_entry(struct ext2_dir_entry_2 *, int, long *);
int count_subfolder(struct ext2_dir_entry_2 *, int, long *);

// iterator function
int directory_block_iterator(struct ext2_inode * dir_inode, dirFunc func, int argc, long * args);


#endif //CSC369_HELPER_H
