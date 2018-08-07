
all : ext2_ls ext2_cp ext2_ln ext2_mkdir ext2_rm ext2_rm_bonus

ext2_ls :  ext2_ls.o helper.o
	gcc -Wall -g -o ext2_ls $^

ext2_cp :  ext2_cp.o helper.o
	gcc -Wall -g -o ext2_cp $^

ext2_ln :  ext2_ln.o helper.o
	gcc -Wall -g -o ext2_ln $^

ext2_mkdir :  ext2_mkdir.o helper.o
	gcc -Wall -g -o ext2_mkdir $^

ext2_rm :  ext2_rm.o helper.o
	gcc -Wall -g -o ext2_rm $^

ext2_rm_bonus :  ext2_rm_bonus.o helper.o
	gcc -Wall -g -o ext2_rm_bonus $^

helper.o : helper.c helper.h ext2.h
	gcc -Wall -g -c $<

%.o : %.c
	gcc -Wall -g -c $<

.PHONY : clean
clean :
	rm -f *.o ext2_?? ext2_rm_bonus ext2_mkdir
