//using bpf to hook syscalls

#ifndef __BPF_HOOK
#define __BPF_HOOK

#include <linux/seccomp.h>  /* Definition of SECCOMP_* constants */
#include <linux/filter.h>   /* Definition of struct sock_fprog */
#include <linux/audit.h>    /* Definition of AUDIT_* constants */
#include <sys/ptrace.h>     /* Definition of PTRACE_* constants */
#include <signal.h>
#include <sys/syscall.h>    /* Definition of SYS_* constants */
#include <unistd.h>
#include <vector>


class BpfHook {
    public:
 

        BpfHook(std::vector<uint32_t> syscall_numbers);
        int install();
        static int init();

        std::vector<uint32_t> syscall_numbers; //syscalls to hook
        struct sock_fprog prog;

};


#endif