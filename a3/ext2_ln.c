#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <zconf.h>
#include <string.h>
#include <errno.h>
#include "helper.h"

int main(int argc, char ** argv){
    char * usage = "Usage: ext2_ln <image file name> [-s] <original file path> <target file path>\n";
    char * in_path;
    char * out_path;
    int IS_HARDLINK = 1;

    if (argc == 4) {
        // hardlink
        in_path = argv[2];
        out_path = argv[3];
        IS_HARDLINK = 1;
    } else if (argc == 5) {
        if (strncmp(argv[2], "-s", 2) != 0) {
            fprintf(stderr, "%s", usage);
            exit(1);
        }
        in_path = argv[3];
        out_path = argv[4];
        IS_HARDLINK = 0;
    } else {
        fprintf(stderr, "%s", usage);
        exit(1);
    }

    check_path_format(in_path);
    check_path_format(out_path);

    read_disk(argv[1]);

    char * target_pathname = NULL;
    char * target_filename = NULL;
    char * original_pathname = NULL;
    char * original_filename = NULL;

    // source path cannot be directory path
    split_last_part_of_path(out_path, &original_pathname, &original_filename);
    if (original_filename == NULL) {
        fprintf(stderr, "Original path cannot be directory path.\n");
        free(original_pathname);
        exit(EISDIR);
    }

    // according to piazza @ 506, check file name length
    if (strlen(original_filename) >  NAME_MAX) {
        fprintf(stderr, "Original file name too long\n");
        free(original_filename);
        free(original_pathname);
        exit(ENAMETOOLONG);
    }

    // source file doesn't exist
    struct ext2_inode * opath_inode = get_inode_by_path(root_inode, original_pathname);     //src path dir_inode
    struct ext2_inode * find_original = find_file(opath_inode, original_filename);     // src file_inode

    if (find_original == NULL) {
        free(original_filename);
        free(original_pathname);
        fprintf(stderr, "Original file doesn't exist\n");
        exit(ENOENT);
    }

    // target path cannot be directory path
    split_last_part_of_path(in_path, &target_pathname, &target_filename);
    if (target_filename == NULL) {
        fprintf(stderr, "Target path cannot be directory path");
        free(original_filename);
        free(original_pathname);
        free(target_pathname);
        exit(EISDIR);
    }

    // according to piazza @ 506, check file name length
    if (strlen(target_filename) > NAME_MAX) {
        fprintf(stderr, "Target file name too long\n");
        free(target_filename);
        free(target_pathname);
        free(original_filename);
        free(original_pathname);
        exit(ENAMETOOLONG);
    }

    // target file cannot exist
    struct ext2_inode * tpath_inode = get_inode_by_path(root_inode, target_pathname);   // target path dir_inode (should exist)
    struct ext2_inode * find_target = find_file(tpath_inode, target_filename);     // target file_inode (should be NULL)

    if (find_target != NULL) {
        fprintf(stderr, "Target file cannot exist\n");
        free(target_filename);
        free(target_pathname);
        free(original_filename);
        free(original_pathname);
        exit(EEXIST);
    }

    unsigned int oinode_num;
    oinode_num = INODE_TO_NUM(find_original);

    // input are valid, then create hardlink by default
    if (IS_HARDLINK) {
        add_dir_entry_to_block(tpath_inode, oinode_num, EXT2_FT_REG_FILE, target_filename);     // new dir_entry pointing to same inode
        find_original->i_links_count++;     // increment original file_inode link_count
    } else if (!IS_HARDLINK) {
        // path name length cannot exceed block size
        if (strlen(in_path) + 1 > EXT2_BLOCK_SIZE ) {
            fprintf(stderr, "Path name too long\n");
            free(target_filename);
            free(target_pathname);
            free(original_filename);
            free(original_pathname);
            exit(ENAMETOOLONG);
        }

        // find new softlink_inode and init to NULL
        unsigned int inum = find_free_inode();
        struct ext2_inode * slink_inode = NUM_TO_INODE(inum);
        memset(slink_inode, 0, sb->s_inode_size);

        // find a empty block for path and init to NULL
        unsigned int bnum = find_free_block();
        struct block * tblock = (struct block *) BLOCK(bnum);
        memset(tblock->byte, 0, EXT2_BLOCK_SIZE);

        // modify softlink_inode
        memcpy(tblock->byte, in_path, (strlen(in_path) + 1) );
        add_block_to_inode(slink_inode, bnum);
        slink_inode->i_mode = EXT2_S_IFLNK;

        unsigned int size = (unsigned int) (strlen(in_path) + 1);
        slink_inode->i_size = size;
        slink_inode->i_links_count = 1;
        slink_inode->i_atime = slink_inode->i_ctime = slink_inode->i_atime = current_time();

        // add dir_entry to target path inode
        add_dir_entry_to_block(tpath_inode, oinode_num, EXT2_FT_SYMLINK, target_filename);
    }

    free(target_filename);
    free(target_pathname);
    free(original_filename);
    free(original_pathname);

    return 0;
}