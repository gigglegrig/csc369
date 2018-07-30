#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zconf.h>
#include "helper.h"

int print_dir_entry(struct ext2_dir_entry_2 * dir, int argc, long * args) {
    // args[0] is all_mode

    if (args[0] == 0) {
        if (CHECKDOT(dir) || CHECKDOTDOT(dir)) {
            return 0;
        }
    }

    char *print_name = malloc(sizeof(char) * dir->name_len + 1);
    strncpy(print_name, dir->name, dir->name_len + 1);
    print_name[dir->name_len] = '\0';
    printf("%s\n", print_name);
    free(print_name);

    return 0;
}

int main(int argc, char **argv) {
    char * usage = "Usage: ext2_ls <image file name> [-a] <absolute path>\n";
    char * in_path;
    long all_mode = 0;

    if (argc == 3) {
        in_path = argv[2];
        all_mode = 0;
    } else if (argc == 4) {
        if (strncmp(argv[2], "-a", 2) != 0) {
            fprintf(stderr, "%s", usage);
            exit(1);
        }
        in_path = argv[3];
        all_mode = 1;
    } else {
        fprintf(stderr, "%s", usage);
        exit(1);
    }

    read_disk(argv[1]);

    struct ext2_inode * target = get_inode_by_path(root_inode, in_path);

    // List filename if regular file
    if (target->i_mode & EXT2_S_IFREG) {
        printf("%s\n", curr_dir_name);
    } else if (target->i_mode & EXT2_S_IFDIR) {
        // Only considered single indirect block according to Piazza
        for (int k = 0; k < IBLOCKS(target); k++) {
            directory_block_iterator(target, print_dir_entry, 1, &all_mode);
        }
    }

    close(disk_fd);

    return 0;
}
