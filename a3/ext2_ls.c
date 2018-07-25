#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zconf.h>
#include "ext2.h"
#include "helper.h"



int print_dir_entry(struct ext2_dir_entry_2 * dir, int argc, long * args) {
    char *print_name = malloc(sizeof(char) * dir->name_len + 1);
    strncpy(print_name, dir->name, dir->name_len + 1);
    print_name[dir->name_len] = '\0';
    printf("%s\n", print_name);
    free(print_name);

    return 0;
}

int main(int argc, char **argv) {

    check_argc("Usage: ext2_ls <image file name> <absolute path>\n", argc, 3);
    read_disk(argv[1]);

    struct ext2_inode * target = get_inode_by_path(root_inode, argv[2]);

    // List filename if regular file
    if (target->i_mode & EXT2_S_IFREG) {
        printf("%s\n", curr_dir_name);
    } else if (target->i_mode & EXT2_S_IFDIR) {
        // Only considered single indirect block according to Piazza
        for (int k = 0; k < IBLOCKS(target); k++) {
            directory_block_iterator(get_block_from_inode(target, k), print_dir_entry, 0, NULL);
        }
    }

    close(disk_fd);

    return 0;
}
