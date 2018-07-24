
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

    int tfree_block = find_free_block();
    int tblock_offset = 0;

    // Copy file in binary
    char c = fgetc(file);
    while (!feof(file)) {
        *(disk + EXT2_BLOCK_SIZE * tfree_block + tblock_offset) = c;
        c = fgetc(file);
        tblock_offset++;
        if (tblock_offset == EXT2_BLOCK_SIZE) {
            // Get a new block
            add_block_to_inode(tfree_block);
            tfree_block = find_free_block();
            tblock_offset = 0;
        }
    }

    // Fill left space with 0
    while (tblock_offset != 0) {
        *(disk + EXT2_BLOCK_SIZE * tfree_block + tblock_offset) = 0;
        tblock_offset++;
        if (tblock_offset == EXT2_BLOCK_SIZE) {
            // Get a new block
            add_block_to_inode(tfree_block);
            tblock_offset = 0;
        }
    }


    free(target_pathname);
    free(target_filename);
    fclose(file);

    return 0;
}

