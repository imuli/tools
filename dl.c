#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

static int errors = 0;

void
error_at(char *base, char *sep, char *name){
	fprintf(stderr, "dl: %s%s%s: %s\n", base, sep, name, strerror(errno));
	errors++;
}

const char type[] = "?pc?d?b?f?l?s???";

void
print_stat(char *name, struct stat *st){
	struct tm *t = gmtime(&st->st_mtime);
	printf("%c %04o %04u-%02u-%02u %02u:%02u:%02u %lx %lu %s\n",
		type[st->st_mode/S_IFIFO], st->st_mode%S_IFIFO,
		t->tm_year+1900, t->tm_mon+1, t->tm_mday,
		t->tm_hour, t->tm_min, t->tm_sec,
		st->st_rdev, st->st_size, name);
}

void
print_dir(char *name){
	DIR *d;
	int dfd;
	struct dirent *ent;
	struct stat st;
	if((d = opendir(name)) == NULL)
		error_at("", "", name);
	dfd = dirfd(d);
	while((ent = readdir(d)) != NULL){
		if(fstatat(dfd, ent->d_name, &st, AT_SYMLINK_NOFOLLOW) < 0)
			error_at(name, "/", ent->d_name);
		print_stat(ent->d_name, &st);
	}
	closedir(d);
}

void
print_entry(char *name){
	struct stat st;
	if(fstatat(AT_FDCWD, name, &st, AT_SYMLINK_NOFOLLOW) < 0)
		error_at("", "", name);
	if((st.st_mode&S_IFMT) == S_IFDIR){
		print_dir(name);
	}
	print_stat(name, &st);
}

int
main(int argc, char **argv){
	int i;
	if(argc == 1)		print_entry(".");
	for(i=1;i<argc;i++)	print_entry(argv[i]);
	return errors;
}
