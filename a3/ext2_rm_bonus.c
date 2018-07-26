#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <memory.h>
#include "helper.h"

#define CHECKDOT(x) ((x)->name_len == 1 && (x)->name[0] == '.')
#define CHECKDOTDOT(x) ((x)->name_len == 2 && (x)->name[0] == '.' && (x)->name[1] == '.')

int main(int argc, char ** argv) {
    char * usage = "Usage: ext2_rm_bonus <image file name> [-r] <absolute path on ext2 disk>\n";
    char * in_path;
    int rec_mode = 0;

    if (argc == 3) {
        in_path = argv[2];
        rec_mode = 0;
    } else if (argc == 4) {
        if (strncmp(argv[2], "-r", 2) != 0) {
            fprintf(stderr, "%s", usage);
            exit(1);
        }
        in_path = argv[3];
        rec_mode = 1;
    } else {
        fprintf(stderr, "%s", usage);
        exit(1);
    }


    check_path_format(in_path);

    read_disk(argv[1]);

    char * target_pathname = NULL;
    char * target_filename = NULL;
    split_last_part_of_path(in_path, &target_pathname, &target_filename);
    if (target_filename == NULL) {
        if (target_pathname[strlen(target_pathname) - 1] == '/') {
            target_pathname[strlen(target_pathname) - 1] = '\0';
            char * tmp_path = NULL;
            char * tmp_file = NULL;
            split_last_part_of_path(target_pathname, &tmp_path, &tmp_file);
            free(target_pathname);
            target_filename = tmp_file;
            target_pathname = tmp_path;
        }
    }

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
        if ((find_result->i_mode & EXT2_S_IFDIR) && rec_mode == 0) {
            free(target_filename);
            free(target_pathname);
            fprintf(stderr, "Target is a directory, use -r flag\n");
            exit(EISDIR);
        }
    }


    // Start deletion
    long target_inode_num = INODE_TO_NUM(find_result);
    directory_block_iterator(tpath_inode, remove_dir_entry, 1, &target_inode_num);


    free(target_filename);
    free(target_pathname);

    return 0;
}