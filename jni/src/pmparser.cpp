#include "pmparser.hpp"

//finds the first segment of a library
extern "C" struct proc_lib* find_library(char* name){
	FILE* maps = fopen("/proc/self/maps", "r");	
	if(!maps){
		printf("Failed to open maps: %s\n", strerror(errno));
		return NULL;
	}

	char* buf = NULL;
	size_t n = 0;
	while(getline(&buf, &n, maps) != EOF){
		size_t start = 0;
		size_t end = 0;
		char perms[8];
		size_t offset = 0;
		int dev_major = 0;
		int dev_minor = 0;
		int inode = 0;
		char path[PATH_MAX];
		//printf("%s", buf);
		sscanf(buf, "%lx-%lx %s %lx %x:%x %d %s", &start, &end, perms, &offset, &dev_major, &dev_minor, &inode, path);
		free(buf);
		buf = NULL;
		//printf("start: %lx, end: %lx perms: %s offset: %lx dev_major: %x dev_minor: %x inode: %d path: %s\n", start, end, perms, offset, dev_major, dev_minor, inode, path);
		if(!strcmp(path, name)){
			struct proc_lib* lib = (struct proc_lib*)malloc(sizeof(*lib));
			lib->start = start;
			lib->end = end;
			memcpy(&lib->perms, &perms, sizeof(perms));
			lib->offset = offset;
			lib->dev_major = dev_major;
			lib->dev_minor = dev_minor;
			lib->inode = inode;
			memcpy(&lib->path, path, sizeof(path));

			return lib;
		}
		
	}
	fclose(maps);



	return NULL;
}






