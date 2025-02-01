#include <elf.h>
#include "pmparser.hpp"

//the base address always has the ehdr
extern "C" size_t get_dynsym_size(const struct proc_lib* lib, Elf64_Ehdr* ehdr){
    
    int fd = open(lib->path, O_RDONLY);

    if(fd < 0){
        printf("Failed to open: %s, %s\n", lib->path, strerror(errno));
        return 0;
    }

    Elf64_Shdr* shdr_base = (Elf64_Shdr*)calloc(ehdr->e_shnum, sizeof(Elf64_Shdr));
    lseek(fd,ehdr->e_shoff, SEEK_SET);
    read(fd, shdr_base, ehdr->e_shnum*sizeof(Elf64_Shdr));
    

    for(int i = 0; i<ehdr->e_shnum; i++){
        Elf64_Shdr* shdr = shdr_base+i;
        if(shdr->sh_type == SHT_DYNSYM){
            return (size_t)shdr->sh_size;
        }
    }

    free(shdr_base);
    
    close(fd);

    return 0;

}

//finds a symbol in dynsym
extern "C" void* find_dyn_symbol(const struct proc_lib* lib, char* symbol_name){
    Elf64_Ehdr* ehdr = (Elf64_Ehdr*)lib->start;
    if(ehdr->e_ident[0] != ELFMAG0 || ehdr->e_ident[1] != ELFMAG1 || ehdr->e_ident[2] != ELFMAG2 || ehdr->e_ident[3] != ELFMAG3){
        printf("ELF not found\n");
        return NULL;
    }

    Elf64_Phdr* phdr_base = (Elf64_Phdr*)(lib->start + ehdr->e_phoff);
    Elf64_Phdr* dynamic_phdr = NULL;
    for(uint16_t i = 0; i<ehdr->e_phnum; i++){
        Elf64_Phdr* phdr = phdr_base+i;

        if(phdr->p_type == PT_DYNAMIC){
            dynamic_phdr = phdr;
            break;
        }
    }

    if(!dynamic_phdr){
        printf("Dynamic section not found (What the fuck)\n");
        return NULL;
    }



    Elf64_Dyn* dyn = (Elf64_Dyn*)(lib->start + dynamic_phdr->p_vaddr);
    char* strtab = NULL;
    Elf64_Sym* symtab = NULL;
    while(dyn->d_tag != DT_NULL){
        if(dyn->d_tag == DT_STRTAB){
            strtab = (char*)(lib->start + dyn->d_un.d_ptr);
        }

        if(dyn->d_tag == DT_SYMTAB){
            symtab = (Elf64_Sym*)(lib->start + dyn->d_un.d_ptr);
        }

        dyn++;
    }
    
    if(symtab == NULL || strtab == NULL){
        printf("dynstr or dynsym not found\n");
    }

    size_t dynsym_size = get_dynsym_size(lib, ehdr);
    //printf("DYMSYM SIZE: 0x%lx\n", dynsym_size);
    size_t sym_num = dynsym_size/sizeof(Elf64_Sym);

    for(size_t i = 0; i<sym_num; i++){
        Elf64_Sym* sym = symtab+i;
       // printf("Found: %s\n", strtab + sym->st_name);
        if(!strcmp(symbol_name, strtab + sym->st_name)){
            return (void*)(lib->start + sym->st_value);
        }
        
    }

    return NULL;
}

extern "C" void dump_plt(struct proc_lib* lib) {
    
}

