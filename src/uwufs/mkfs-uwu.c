/**
 *	Formats block device in the uwufs format.
 *
 *	Author: Joseph
 */


#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>


static int init_uwufs(int fd)
{
	// TODO:
	printf("TODO: init uwufs\n");
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Usage: %s [block device]\n", argv[0]);
		return 1;
	}

	int fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		perror("Failed to access block device");
		return 1;
	}

	int ret = init_uwufs(fd);

	close(fd);
	return ret;
}
