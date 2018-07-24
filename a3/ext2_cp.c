
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <zconf.h>
#include <string.h>
#include "ext2.h"
#include "helper.h"


int main(int argc, char **argv) {
    check_argc("Usage: ext2_cp <image file name> <file on native os> <absolute path on ext2 disk>\n", argc, 4);
    read_disk(argv[1]);

    FILE * file = fopen(argv[2], "rb");
    if (file == NULL) {
        fprintf(stderr, "No such file on native os.\n");
        perror("fopen");
        exit(1);
    }

    // Parsing target path, we would want the second to last path, then we can determine if the user want to create a new file
    char * target_pathname = NULL;
    char * target_filename = NULL;
    split_last_part_of_path(argv[3], &target_pathname, &target_filename);
    if (target_filename == NULL) {
        char * native_path;
        char * native_filename;
        split_last_part_of_path(argv[2], &native_path, &native_filename);
        free(native_path);
        target_filename = native_filename;
    }


    struct ext2_inode * tpath_inode = get_inode_by_path(root_inode, target_pathname);

    // Find a new inode
    int inum = find_free_inode();
    struct ext2_inode * newfile_inode = get_inode_by_num(inum);
    memset(newfile_inode, 0, sb->s_inode_size);

    unsigned int bnum;
    struct block * tblock = NULL;
    int tblock_offset = 0;

    // Copy file in binary
    unsigned int size = 0;
    unsigned int iblocks = 0;
    unsigned char c = fgetc(file);
    while (!feof(file)) {
        if (tblock_offset == 0) {
            bnum = find_free_block();
            tblock = (struct block *) disk + EXT2_BLOCK_SIZE * bnum;
        }

        tblock->byte[tblock_offset++] = c;
        size++;
        c = fgetc(file);
        if (tblock_offset == EXT2_BLOCK_SIZE) {
            // Get a new block
            add_block_to_inode(newfile_inode, bnum);
            iblocks++;
            tblock_offset = 0;
        }
    }

    // Fill left space with 0
    if (tblock_offset != 0) {
        memset(tblock->byte + tblock_offset, 0, EXT2_BLOCK_SIZE - tblock_offset);
    }

    newfile_inode->i_mode = EXT2_S_IFREG;
    newfile_inode->i_size = size;
    newfile_inode->i_links_count = 1;
    newfile_inode->i_blocks = iblocks;

    // Add directory entry


    free(target_pathname);
    free(target_filename);
    fclose(file);

    return 0;
}

