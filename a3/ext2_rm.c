#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "helper.h"

int main(int argc, char ** argv) {
    check_argc("Usage: ext2_rm <image file name> <absolute path on ext2 disk>\n", argc, 3);

    check_path_format(argv[2]);

    read_disk(argv[1]);

    char * target_pathname = NULL;
    char * target_filename = NULL;
    split_last_part_of_path(argv[2], &target_pathname, &target_filename);

    struct ext2_inode * tpath_inode = get_inode_by_path(root_inode, target_pathname);
    struct ext2_inode * find_result = find_file(tpath_inode, target_filename);

    if (find_result == NULL) {
        free(target_pathname);
        if (target_filename != NULL) {
            free(target_filename);
            fprintf(stderr, "File not exist\n");
            exit(ENOENT);
        } else {
            fprintf(stderr, "Target is a directory\n");
            exit(EISDIR);
        }
    } else {
        if (find_result->i_mode & EXT2_S_IFDIR) {
            free(target_filename);
            free(target_pathname);
            fprintf(stderr, "Target is a directory\n");
            exit(EISDIR);
        }
    }

    // Start deletion
    long target_inode_num = INODE_TO_NUM(find_result);

    // Delete directory entry, as well as free inode and blocks
    // since you can't remove . and .. the first dir block will never be empty.
    // Thus do not need to handle return value 2.
    directory_block_iterator(tpath_inode, remove_dir_entry, 1, &target_inode_num);


    free(target_filename);
    free(target_pathname);

    return 0;
}