#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
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

    sb = (struct ext2_super_block *) BLOCK(1);
    group_num = (sb->s_blocks_count - sb->s_first_data_block - 1) / sb->s_blocks_per_group + 1;
    gd = (struct ext2_group_desc *) BLOCK(2);
    root_inode = get_inode_by_num(2);
    curr_dir_name = "/";
}

struct ext2_inode * get_inode_by_num(int inode_number){
    int g_num = inode_number / sb->s_inodes_per_group;
    int i_num = inode_number % sb->s_inodes_per_group;
    struct ext2_group_desc * target_gd = gd + g_num;
    return (struct ext2_inode *) BLOCK(gd->bg_inode_table) + i_num - 1;
}

int check_inode_directory(struct ext2_inode * inode) {
    if (inode->i_mode & EXT2_S_IFDIR) {
        return 1;
    }

    // Commented since we don't need to check soft link to directories any more

//    else if (inode->i_mode & EXT2_S_IFLNK) {
//        if (inode->i_block[0] != 0) {
//            struct ext2_dir_entry_2 *dir = (struct ext2_dir_entry_2 *) (disk + EXT2_BLOCK_SIZE * inode->i_block[0]);
//            if (dir->file_type == EXT2_FT_DIR && dir->name[0] == '.') {
//                return 1;
//            }
//        }
//    }

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

        if (result == NULL) {
            printf("No such file or directory\n");
            exit(ENOENT);
        }

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
                struct ext2_dir_entry_2 *dir = (struct ext2_dir_entry_2 *) BLOCK(block_num);

                int curr_pos = 0;
                while (curr_pos < EXT2_BLOCK_SIZE) {
                    // Find the target file's inode
                    if (strncmp(file, dir->name, strlen(file)) == 0) {
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
    return NULL;
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
    //check_path_format(path);

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

void set_bit(char bori, int location, unsigned char value) {
    location -= 1; // Change 1 based location to 0 based.

    unsigned int bit_pos = 0;
    unsigned short * count = NULL;

    if (bori == 'b') {
        bit_pos = gd->bg_block_bitmap;
        count = &(gd->bg_free_blocks_count);
    } else if (bori == 'i') {
        bit_pos = gd->bg_inode_bitmap;
        count = &(gd->bg_free_inodes_count);
    }

    unsigned char *bitmap = BLOCK(bit_pos);
    int byte = location / (sizeof(unsigned char) * 8);
    int bit = location % (sizeof(unsigned char) * 8);

    if (value == 0 && get_bit(bori, location + 1)) {
        // Set 1 to 0
        bitmap[byte] = bitmap[byte] & ((unsigned char) 0 << bit);
        (*count)--;
    } else if (value == 1 && !(get_bit(bori, location + 1))) {
        bitmap[byte] = bitmap[byte] | ((unsigned char) 1 << bit);
        (*count)++;
    }
}

int get_bit(char bori, int location) {
    location -= 1; // Change 1 based location to 0 based.

    unsigned int bit_pos = 0;

    if (bori == 'b') {
        bit_pos = gd->bg_block_bitmap;
    } else if (bori == 'i') {
        bit_pos = gd->bg_inode_bitmap;
    }

    unsigned char *bitmap = BLOCK(bit_pos);
    int byte = location / (sizeof(unsigned char) * 8);
    int bit = location % (sizeof(unsigned char) * 8);

    return (bitmap[byte] >> bit) & 1;
}

unsigned int find_free_block() {
    for (unsigned int i = sb->s_r_blocks_count + 1; i <= sb->s_blocks_count; i++) {
        if (get_bit('b', i) == 0) {
            set_bit('b', i, 1);
            return i;
        }
    }

    return 0;
}

unsigned int find_free_inode() {
    for (unsigned int i = 13; i <= sb->s_inodes_count; i++) {
        if (get_bit('i', i) == 0) {
            set_bit('i', i, 1);
            return i;
        }
    }

    return 0;
}

void add_block_to_inode(struct ext2_inode *inode, unsigned int block) {
    if (IBLOCKS(inode) < 12) {
        inode->i_block[IBLOCKS(inode)] = block;
        inode->i_blocks += EXT2_BLOCK_SIZE / 512;
    } else if (IBLOCKS(inode) >= 12 && IBLOCKS(inode) < MAX12) {
        // The single indirection block
        struct block * tblock;

        if (inode->i_block[12] == 0) {
            // Assign new block
            unsigned int nb = find_free_block(); // ???????Will it use some block other process is using????
            tblock = (struct block *) BLOCK(nb);
            memset(tblock, 0, EXT2_BLOCK_SIZE);
            inode->i_block[12] = nb;
        } else {
            tblock = (struct block *) BLOCK(inode->i_block[12]);
        }

        int curr_pos = 0;

        while (curr_pos < EXT2_BLOCK_SIZE / sizeof(int)) {
            int * intptr = (int *) tblock + curr_pos;
            if (*intptr == 0) {
                *intptr = block;
                inode->i_blocks += EXT2_BLOCK_SIZE / 512;
                break;
            }
            curr_pos++;
        }

        if (curr_pos >= EXT2_BLOCK_SIZE / sizeof(int)) {
            fprintf(stderr, "Not enough space for level 12\n");
            exit(1);
        }

    }
}

int get_block_from_inode(struct ext2_inode *inode, unsigned num) {
    if (num < 12) {
        return inode->i_block[num];
    } else if (num >= 12 && num < 12 + EXT2_BLOCK_SIZE / sizeof(int)) {
        int * intptr = (int *) BLOCK(inode->i_block[12]) + num - 12;
        return *intptr;
    }
}

void add_new_directory_entry(struct ext2_inode * dir_inode, unsigned int inode, unsigned char file_type, char * name) {
    // Name must be NULL terminated!
    int req_space = PAD(8 + (int) strlen(name));

    long parsed_args[5] = {0, inode, 0, file_type, (intptr_t) name};

    int curr_bindex = 0;
    //int curr_bindex = dir_inode->i_blocks - 1;
    int curr_block = dir_inode->i_block[curr_bindex];
    struct ext2_dir_entry_2 * dir = (struct ext2_dir_entry_2 *) BLOCK(curr_block);

    int res = -1;
    while (curr_bindex < IBLOCKS(dir_inode)) {
        res = directory_block_iterator(curr_block, add_dir_entry, 5, parsed_args);
        if (res == 1) {
            // Break, successfully inserted
            break;
        }
        curr_bindex++;
    }

    // Not inserted
    if (res == 0) {
        if (curr_bindex >= MAX12) {
            fprintf(stderr, "Not enough block space in directory entry.");
            exit(1);
        }
        // create new block
        unsigned int nblock_num = find_free_block();
        struct block * nblock = (struct block *) BLOCK(nblock_num);
        dir = (struct ext2_dir_entry_2 *) nblock;

        // Set new dir entry
        set_dir_entry(dir, inode, EXT2_BLOCK_SIZE, file_type, name);

        // Add this new block to inode
        add_block_to_inode(dir_inode, nblock_num);
    }

}

int directory_block_iterator(int block_num, dirFunc func, int argc, long * args) {
    // This function performs function func(part, argc, args) on every directory entry of the input block
    // Return value according to mapFunc function, 0 for continue, 1 for abort
    struct ext2_dir_entry_2 * dir_ptr = (struct ext2_dir_entry_2 *) BLOCK(block_num);
    int curr_pos = 0;

    while (curr_pos < EXT2_BLOCK_SIZE) {
        int func_result = -1;
        if ((func_result = func(dir_ptr, argc, args)) != 0) {
            // func did not work well
            return 1;
        }
        curr_pos += dir_ptr->rec_len;
        dir_ptr = (void *) dir_ptr + dir_ptr->rec_len;
    }

    return 0;
}

int add_dir_entry(struct ext2_dir_entry_2 * dir, int argc, long * args) {
    // args:
    // 0 - required space,  1 - inode,  2 - reclen
    // 3 - file_type,       4 - nname

    // Calculate space
    int space = dir->rec_len - PAD(dir->name_len + 8);
    int req_space = args[0];
    if (space >= req_space) {
        set_dir_entry(dir, args[1], args[2], args[3], args[4]);
        return 1;
    }

    return 0;
}

void set_dir_entry(struct ext2_dir_entry_2 * dir, unsigned int inode, unsigned short reclen, unsigned char file_type, char * name) {
    dir->inode = inode;
    dir->rec_len = reclen;
    dir->name_len = (unsigned char) strlen(name);
    dir->file_type = file_type;
    strncpy(dir->name, name, strlen(name));
}