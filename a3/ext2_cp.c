
#include <stdio.h>
#include <stdlib.h>
#include "ext2.h"
#include "helper.h"

int main(int argc, char **argv) {
    check_argc("Usage: ext2_cp <image file name> <file on native os> <absolute path on ext2 disk>\n", argc, 4);
    read_disk(argv[1]);
}
