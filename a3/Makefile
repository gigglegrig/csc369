
all : ext2_ls ext2_cp

ext2_ls :  ext2_ls.o helper.o
	gcc -Wall -g -o ext2_ls $^

ext2_cp :  ext2_cp.o helper.o
	gcc -Wall -g -o ext2_cp $^

helper.o : helper.c helper.h ext2.h
	gcc -Wall -g -c $<

%.o : %.c
	gcc -Wall -g -c $<

.PHONY : clean
clean :
	rm -f *.o ext2_??
