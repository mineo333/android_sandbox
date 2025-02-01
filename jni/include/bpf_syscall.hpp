//using bpf to hook syscalls

#ifndef __BPF_HOOK
#define __BPF_HOOK

#include <linux/seccomp.h>  /* Definition of SECCOMP_* constants */
#include <linux/filter.h>   /* Definition of struct sock_fprog */
#include <linux/audit.h>    /* Definition of AUDIT_* constants */
#include <sys/ptrace.h>     /* Definition of PTRACE_* constants */
#include <sys/syscall.h>    /* Definition of SYS_* constants */
#include <unistd.h>
#include <vector>

struct bpf_sigaction {
    void     (*sa_handler)(int);
    void     (*sa_sigaction)(int, siginfo_t *, void *);
    sigset_t   sa_mask;
    int        sa_flags;
    void     (*sa_restorer)(void);
};


class BpfHook {
    public:
 

        BpfHook(std::vector<uint32_t> syscall_numbers);
        int install();
        static int init();

        std::vector<uint32_t> syscall_numbers; //syscalls to hook
        struct sock_fprog prog;

};


#endif