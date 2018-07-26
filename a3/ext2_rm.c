#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "helper.h"
#include "ext2.h"

int main(int argc, char ** argv) {
    check_argc("Usage: ext2_cp <image file name> <absolute path on ext2 disk>\n", argc, 3);
    read_disk(argv[1]);

    check_path_format(argv[2]);

    char * target_pathname = NULL;
    char * target_filename = NULL;
    split_last_part_of_path(argv[2], &target_pathname, &target_filename);

    struct ext2_inode * tpath_inode = get_inode_by_path(root_inode, target_pathname);
    struct ext2_inode * find_result = find_file(tpath_inode, target_filename);

    if (find_result == NULL) {
        free(target_pathname);
        if (target_filename != NULL) {
            free(target_filename);
        }
        fprintf(stderr, "File not exist\n");
        exit(ENOENT);
    } else {
        if (find_result->i_mode & EXT2_S_IFDIR) {
            free(target_filename);
            free(target_pathname);
            fprintf(stderr, "Target is a directory\n");
            exit(EISDIR);
        }
    }

    // Start deletion
    unsigned long target_inode_num = INODE_TO_NUM(find_result);

    find_result->i_links_count--;
    if (find_result->i_links_count == 0) {
        // Set blocks to free
        for (unsigned int i = 0; i < IBLOCKS(find_result); i++) {
            int target_block_num = get_block_from_inode(find_result, i);
            set_bit('b', target_block_num, 0);
        }

        // Set inode to free
        set_bit('i', target_inode_num, 0);
    }

    // Delete directory entry, since you can't remove . and .. the first dir block will never be empty.
    // Thus do not need to handle return value 2.
    directory_block_iterator(tpath_inode, remove_dir_entry, 1, &target_inode_num);


    free(target_filename);
    free(target_pathname);

    return 0;
}