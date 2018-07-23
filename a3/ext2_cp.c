
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

    int file = open(argv[2], 'r');
    if (file == NULL) {
        fprintf(stderr, "No such file on native os.\n");
        perror("open");
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



    free(target_pathname);
    free(target_filename);
    close(file);

    return 0;
}

