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
        fprintf(stderr, "%s", usage);
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
    gd = (struct ext2_group_desc *) BLOCK(2);
    root_inode = NUM_TO_INODE(2);
    curr_dir_name = "/";
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
    if (file == NULL || source == NULL) {
        return NULL;
    }

    // Cannot find file in a regular file, only can find in directory (Assume symbolic link on directory also turn on directory bit)
    if (INODE_IFDIR(source)) {
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
                        return NUM_TO_INODE(dir->inode);
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
        bitmap[byte] = bitmap[byte] & (~((unsigned char) 1 << bit));
        (*count)++;
    } else if (value == 1 && !(get_bit(bori, location + 1))) {
        bitmap[byte] = bitmap[byte] | ((unsigned char) 1 << bit);
        (*count)--;
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

    return -1;
}

void add_dir_entry_to_block(struct ext2_inode *dir_inode, unsigned int inode, unsigned char file_type, char *name) {
    // Name must be NULL terminated!
    int req_space = PAD(8 + (int) strlen(name));
    long parsed_args[5] = {req_space, inode, 0, file_type, (intptr_t) name};

    int res = directory_block_iterator(dir_inode, add_dir_entry, 5, parsed_args);

    // Not inserted
    if (res == 0) {
        if (IBLOCKS(dir_inode) >= MAX12) {
            fprintf(stderr, "Not enough block space in directory entry.");
            exit(1);
        }
        // create new block
        unsigned int nblock_num = find_free_block();
        struct block * nblock = (struct block *) BLOCK(nblock_num);
        struct ext2_dir_entry_2 * dir = (struct ext2_dir_entry_2 *) nblock;

        // Set new dir entry
        set_dir_entry(dir, inode, EXT2_BLOCK_SIZE, file_type, name);

        // Add this new block to inode
        add_block_to_inode(dir_inode, nblock_num);
    }

}


int directory_block_iterator(struct ext2_inode * dir_inode, dirFunc func, int argc, long * args) {
    // This function performs function func(part, argc, args) on every directory entry of the input block
    // Return value according to mapFunc function, 0 for continue, 1 for abort
    if (!(dir_inode->i_mode & EXT2_S_IFDIR)) {
        return -1;
    }

    struct ext2_dir_entry_2 * dir_ptr;
    int curr_pos = 0;

    for (int i = 0; i < IBLOCKS(dir_inode); i++) {
        int dir_block_num = get_block_from_inode(dir_inode, i);
        dir_ptr = (struct ext2_dir_entry_2 *) BLOCK(dir_block_num);
        dir_ptr = (void *) dir_ptr + curr_pos;

        while (curr_pos < EXT2_BLOCK_SIZE) {
            int res = func(dir_ptr, argc, args);
            if (res) {
                // func returns other than 0, want to abort
                return res;
            }
            curr_pos += dir_ptr->rec_len;
            dir_ptr = (void *) dir_ptr + dir_ptr->rec_len;
        }

        curr_pos = (curr_pos >= EXT2_BLOCK_SIZE) ? curr_pos - EXT2_BLOCK_SIZE : curr_pos;
    }

    return 0;

}

int add_dir_entry(struct ext2_dir_entry_2 * dir, int argc, long * args) {
    // args:
    // 0 - required space,  1 - inode,  2 - reclen
    // 3 - file_type,       4 - name

    // Calculate space
    int space = (dir->rec_len > EXT2_BLOCK_SIZE) ? EXT2_BLOCK_SIZE : (dir->rec_len - PAD(dir->name_len + 8));
    int req_space = (int) args[0];
    if (space >= req_space) {
        args[2] = (args[2] == 0) ? space : args[2];
        dir->rec_len -= space;
        dir = (void *) dir + PAD(dir->name_len + 8);
        set_dir_entry(dir, (unsigned int) args[1], (unsigned short)args[2], (unsigned char) args[3], (char *) args[4]);
        return 1;
    }

    return 0;
}

void set_dir_entry(struct ext2_dir_entry_2 * dir, unsigned int inode, unsigned short reclen, unsigned char file_type, char * name) {
    //memset(dir, 0, reclen);
    dir->inode = inode;
    dir->rec_len = reclen;
    dir->name_len = (unsigned char) strlen(name);
    dir->file_type = file_type;
    strncpy(dir->name, name, strlen(name));
}

struct ext2_dir_entry_2 * prev_dir = NULL;

int remove_dir_entry(struct ext2_dir_entry_2 * dir, int argc, long * args) {
    // Remove inode from dir_entry_block
    // User have to provide an inode number
    // If argc == 0, means no check, delete every entry in the current dir


    unsigned long del_inode_num = 0;
    if (argc != 0) {
        // In this case, function is assigned an inode number to delete, if dir not match with it, continue the loop
        del_inode_num = (unsigned long) args[0];

        if (dir->inode != del_inode_num) {
            prev_dir = dir;
            return 0;
        }
    } else {
        // Not assigned, delete every entry
        del_inode_num = dir->inode;
    }

    // In both cases above, dir matches with the inode number that we want to delete now

    struct ext2_inode * del_inode = NUM_TO_INODE(del_inode_num);

    if (!(CHECKDOT(dir) || CHECKDOTDOT(dir))) {
        if (del_inode->i_mode & EXT2_S_IFDIR) {
            long count = 0;
            directory_block_iterator(del_inode, count_subfolder, 1, &count);
            long hardlink = del_inode->i_links_count - 2 - count;
            if (hardlink == 0) {
                // If dir is . or .. we do not recursively delete the directory, just deduct link count
                directory_block_iterator(del_inode, remove_dir_entry, 0, NULL);
            }
        }
    }

    // Deduct link count
    del_inode->i_links_count--;

    // If no more links, free blocks and inode.
    if (del_inode->i_links_count == 0) {
        // Set blocks to free
        for (unsigned int i = 0; i < IBLOCKS(del_inode); i++) {
            int target_block_num = get_block_from_inode(del_inode, i);
            set_bit('b', target_block_num, 0);
        }

        // Set inode to free
        set_bit('i', del_inode_num, 0);
        memset(del_inode, 0, sb->s_inode_size);
    }

    // If argc == 0, then must recursively delete every dir entry,
    // meaningless to delete every entry one by one, just deduct link count and free inode and blocks
    if (argc == 0) {
        return 0; // 0 since we want the iterator to move on
    }

    // Delete this entry
    if (prev_dir == NULL) {
        // The first entry in the first block
        if (dir->rec_len == EXT2_BLOCK_SIZE) {
            // And this first block has this only entry...
            // Let outer function handle this !!!
            // Didn't actually remove the "." entry (always be the first), however link count is deducted
            return 2;
        } else {
            // Move the next entry to the first
            unsigned short ori_reclen = dir->rec_len;
            struct ext2_dir_entry_2 *next_dir = (void *) dir + dir->rec_len;
            memmove(dir, next_dir, next_dir->rec_len);
            dir->rec_len += ori_reclen;
        }
    } else {
        prev_dir->rec_len += dir->rec_len;
    }


    return 1;

}

int count_subfolder(struct ext2_dir_entry_2 * dir, int argc, long * args) {
    if (!(CHECKDOT(dir) || CHECKDOTDOT(dir)) && dir->file_type == EXT2_FT_DIR) {
        args[0]++;
    }

    return 0;
}