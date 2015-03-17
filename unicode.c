#include <stdio.h>
#include <stdlib.h>

enum {
	UTF8MAX = 8,
};

typedef unsigned long Rune;

int
utf8rune(unsigned char *u, Rune r){
	unsigned char *ub;
	unsigned char buf[UTF8MAX];
	unsigned long mask;
	int i;

	ub = u;
	if(r < 0x80){
		/* common special case */
		*u++ = r;
	} else {
		/* everything else */
		i = 0;
		mask = 0x80;
		while(r >= (256 - mask)/2){
			buf[i] = 0x80 | (r & 0x3f);
			r >>= 6;
			i++;
			mask |= mask>>1;
		}
		*u++ = mask | r;
		do{
			*u++ = buf[--i];
		}while(i);
	}
	*u = '\0';
	return u-ub;
}

int
show(char *s){
	unsigned char u[UTF8MAX];
	char *se;
	Rune r;

	r = strtoul(s, &se, 16);
	if(s == se)
		return 0;

	utf8rune(u, r);
	fputs((char *)u, stdout);
	return 1;
}

int
main(int argc, char **argv){
	int i;
	for(i=1;i<argc;i++){
		show(argv[i]);
	}
	return 0;
}
