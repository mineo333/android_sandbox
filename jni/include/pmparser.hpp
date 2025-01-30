
#ifndef H_PMPARSER
#define H_PMPARSER
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/limits.h>

struct proc_lib {
	size_t start;
	size_t end;
	char perms[8];
	size_t offset;
	int dev_major;
	int dev_minor;
	int inode;
	char path[PATH_MAX];
};

extern "C" struct proc_lib* find_library(char* name);


#endif