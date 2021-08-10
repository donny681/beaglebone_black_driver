obj-m+=ebbchar.o
CC = arm-linux-gnueabihf-gcc
all:
	# make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	make -C /home/yates/beagleboard/linux/ M=$(PWD) modules
	$(CC) testcdev.c -o test
	cp ebbchar.ko /home/yates/nfs 
	cp test /home/yates/nfs 
clean:
	# make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	make -C /home/yates/beagleboard/linux/ M=$(PWD) clean
	# rm test
