#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "helper.h"

void check_argc(char * usage, int in, int target) {
    if(in != target) {
        fprintf(stderr, usage);
        exit(1);
    }
}


void read_disk(char * filename) {
    int fd = open(filename, O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    sb = (struct ext2_super_block *)(disk + 1024);
    group_num = (sb->s_blocks_count - sb->s_first_data_block - 1) / sb->s_blocks_per_group + 1;
    gd = (struct ext2_group_desc *)(disk + 2 * EXT2_BLOCK_SIZE);
    root_inode = get_inode_by_num(2);
    curr_dir_name = "/";
}

struct ext2_inode * get_inode_by_num(int inode_number){
    int g_num = inode_number / sb->s_inodes_per_group;
    int i_num = inode_number % sb->s_inodes_per_group;
    struct ext2_group_desc * target_gd = gd + g_num;
    return (struct ext2_inode *) (disk + EXT2_BLOCK_SIZE * gd->bg_inode_table) + i_num - 1;
}

int check_inode_directory(struct ext2_inode * inode) {
    if (inode->i_mode & EXT2_S_IFDIR) {
        return 1;
    } else if (inode->i_mode & EXT2_S_IFLNK) {
        if (inode->i_block[0] != 0) {
            struct ext2_dir_entry_2 *dir = (struct ext2_dir_entry_2 *) (disk + EXT2_BLOCK_SIZE * inode->i_block[0]);
            if (dir->file_type == EXT2_FT_DIR && dir->name[0] == '.') {
                return 1;
            }
        }
    }

    return 0;
}


struct ext2_inode * get_inode_by_path(struct ext2_inode * root, char * path) {
    if (path[0] != '/') {
        // All path start from root
        printf("No such file or directory\n");
        exit(ENOENT);
    }

    struct ext2_inode * result = root;

    // Split directory name until reach end
    curr_dir_name = strtok(path, "/");
    while (curr_dir_name != NULL) {
        //printf("Find directory name '%s'\n", curr_dir_name);
        result = find_file(result, curr_dir_name);
        // Continue splitting string
        char * temp = strtok(NULL, "/");
        if (temp == NULL) {
            break;
        } else {
            curr_dir_name = temp;
        }
    }
    return result;
}

struct ext2_inode * find_file(struct ext2_inode * source, char * file) {
    // Cannot find file in a regular file, only can find in directory (Assume symbolic link on directory also turn on directory bit)
    if (check_inode_directory(source)) {
        // Only considered single indirect block according to Piazza
        for (int k = 0; k < 12; k++) {
            // If block exists
            if (source->i_block[k]) {
                int block_num = source->i_block[k];
                struct ext2_dir_entry_2 *dir = (struct ext2_dir_entry_2 *) (disk + EXT2_BLOCK_SIZE * block_num);

                int curr_pos = 0;
                while (curr_pos < EXT2_BLOCK_SIZE) {
                    // Find the target file's inode
                    if (strncmp(file, dir->name, dir->name_len) == 0) {
                        // Since inode is 1-based counting, we deduct 1 here
                        return get_inode_by_num(dir->inode);
                    }

                    curr_pos = curr_pos + dir->rec_len;
                    dir = (void*) dir + dir->rec_len;
                }
            }
        }
    }

    // If after loop function did not return
    printf("No such file or directory\n");
    exit(ENOENT);
}