#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#if LONG_BIT == 32
typedef unsigned long Rune;
#else
typedef unsigned int Rune;
#endif
#define RUNE_MAX 0xffffffff
#define UTF8_MAX 7
enum {
	IncRune = 0xfffe,
	BadRune = 0xffff,
};

static const unsigned int biti_32v[32] = {
   0, 1,28, 2,29,14,24, 3,	30,22,20,15,25,17, 4, 8,
  31,27,13,23,21,19,16, 7,	26,12,18, 6,11, 5,10, 9};
static const unsigned long biti_32m = 0x077cb531;
static int
bit_index(unsigned r) {
  unsigned i = r;
  i |= i >> 1; i |= i >> 2; i |= i >> 4;
  i |= i >> 8; i |= i >> 16;
  i=i/2+1;
  return biti_32v[((i * biti_32m) >> 27)%32];
}

int
utf8char(char *u){
	int i = bit_index(0xff&~*u);
	if(i>5) return i-6;
	return 7-i;
}

int
runechar(Rune r){
	int i = bit_index(r);
	if(i<7) return 1;
	return (i-1)/5+1;
}

int
rune_utf8(Rune *rr, char *u){
	unsigned long mask;
	int n = utf8char(u);
	int i;
	Rune r;
	/* check for spurious 10xxxxxx bytes */
	if(n == 0){
		*rr = BadRune;
		return 1;
	}
	mask = 0xff >> n;
	r = *u & mask;
	for(i=1;i<n;i++){
		/* check for complete characters */
		if((u[i] & 0xc0) != 0x80){
			*rr = IncRune;
			return i;
		}
		r <<= 6;
		r |= u[i] & 0x3f;
	}
	/* enforce minimum length utf8 encoding */
	if(i != runechar(r))
		r = BadRune;
	*rr = r;
	return i;
}

void
do_exit(int n){
	fprintf(stdout, "\n");
	exit(n);
}

static Rune
get_rune(void){
        static char utf[UTF8_MAX + 1];
        static int utflen;
        int len = 0, n;
        Rune r = IncRune;

        if(utflen)
                len = rune_utf8(&r, utf);
        while(r == IncRune && len == utflen){
                n = read(0, utf+utflen, 1);
		if(n!=1) do_exit(n);
                utf[++utflen]='\0';
                len = rune_utf8(&r, utf);
        }
        utflen -= len;
        memmove(utf, utf+len, utflen+1);
        return r;
}

int
main(int argc, char **argv){
	Rune r;
	int i;
	char *s;
	if(argc == 1){
		while(1)
			fprintf(stdout, "%02x ", get_rune());
	} else {
		for(i=1;i<argc;i++){
			if(i>1)
				fprintf(stdout, "20 ");
			for(s = argv[i]; *s != '\0';){
				s += rune_utf8(&r, s);
				fprintf(stdout, "%02x ", r);
			}
		}
		do_exit(0);
	}
	return 0;
}
