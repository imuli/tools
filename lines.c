#include <stdlib.h>
#include <unistd.h>

const char usage[] = "usage: lines {start} (end)\n";

int
main(int argc, char **argv){
	int start, lines, count, offset, more;
	char buf[512];

	start = 1;
	if(argc == 3) {
		start = atoi(argv[1]);
		argc--;
		argv++;
	}
	if(argc != 2) {
		write(2, usage, sizeof(usage));
		return 1;
	}
	lines = atoi(argv[1]);

	for(more = 1, count = 0; count < lines && more;){
		for(offset = 0; offset < sizeof(buf);){
			more = read(0, buf+offset, 1);

			if(!more) /* eof */
				break;

			offset++;
			if(buf[offset-1] == '\n'){
				count++;
			   	break;
			}
		}
		if(count >= start)
			write(1, buf, offset);
	}
	return 0;
}
