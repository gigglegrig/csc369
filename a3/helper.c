#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <zconf.h>
#include "helper.h"

void check_argc(char * usage, int in, int target) {
    if(in != target) {
        fprintf(stderr, usage);
        exit(1);
    }
}

void check_path_format(char * path) {
    if (path[0] != '/') {
        printf("No such file or directory\n");
        exit(ENOENT);
    }
}



void read_disk(char * filename) {
    disk_fd = open(filename, O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, disk_fd, 0);
    if(disk == MAP_FAILED) {
        close(disk_fd);
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
    check_path_format(path);

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

int find_last_char_n(char * string, char target) {
    int result = -1;

    int i = 0;
    while (string[i] != '\0') {
        if (string[i] == target) {
            result = i;
        }
        i++;
    }

    return result;
}

void split_last_part_of_path(char * path, char ** out_pathname, char ** out_filename) {
    check_path_format(path);

    int last_slash = find_last_char_n(path, '/');
    if (last_slash + 1 == strlen(path)) {
        // The last part must be a directory
        *out_pathname = malloc(sizeof(char) * (strlen(path) + 1));
        strncpy(*out_pathname, path, strlen(path) + 1);
        (*out_pathname)[strlen(path) + 1] = '\0';
    } else if (last_slash != -1) {
        // The last part may be a name
        *out_pathname = malloc((last_slash + 2) * sizeof(char));
        strncpy(*out_pathname, path, last_slash + 2);
        (*out_pathname)[last_slash + 1] = '\0';

        // Copy last part to newfile name
        int newfile_len = (int) strlen(path) - last_slash - 1;
        *out_filename = malloc((newfile_len + 1) * sizeof(char));
        strncpy(*out_filename, path + last_slash + 1, newfile_len + 1);
        (*out_filename)[newfile_len + 1] = '\0';
    }
}

void change_bitmap(unsigned int bit_pos, int location, unsigned char value) {
    location -= 1; // Change 1 based location to 0 based.

    unsigned char *bitmap = disk + EXT2_BLOCK_SIZE * bit_pos;
    int byte = location / (sizeof(unsigned int) * 8);
    int bit = location % (sizeof(unsigned int) * 8);

    if (value == 0) {
        bitmap[byte] = bitmap[byte] & ((unsigned char) 0 << bit);
    } else if (value == 1) {
        bitmap[byte] = bitmap[byte] | ((unsigned char) 1 << bit);
    }
}

int find_free_block() {

}

void add_block_to_inode(struct ext2_inode *inode, int block) {
    // Change block bitmap to occupied, update infos in bf
}

struct ext2_inode * create_new_inode() {
    // Find free inode section and create a new inode
}