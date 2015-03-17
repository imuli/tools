#include <stdlib.h>
#include <unistd.h>

const char usage[] = "usage: lines (lines)\n";

int
main(int argc, char **argv){
	int lines, count, more;
	char buf[512];

	if(argc != 2) {
		write(2, usage, sizeof(usage));
		return 1;
	}
	lines = atoi(argv[1]);

	count = 0;
	more = 1;
	while(lines > 0 && more){
		more = read(0, buf+count, 1);
		lines -= (buf[count] == '\n');
		count += more;
		if(count == sizeof(buf)){
			write(1, buf, count);
			count=0;
		}
	}
	if(lines == 0 || !more)
		write(1, buf, count);
	return 0;
}

