#ifndef __ELF_PARSER
#define __ELF_PARSER

extern "C" void* find_dyn_symbol(const struct proc_lib* lib, char* symbol_name);
extern "C" size_t get_dynsym_size(const struct proc_lib* lib);

#endif 