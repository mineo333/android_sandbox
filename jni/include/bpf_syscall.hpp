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
#include <mutex>
#include <thread>
#include <condition_variable>
#include <ucontext.h>




class SyscallReq {
    public:
        uint32_t syscall_number;
        uint64_t args[6];
        uint32_t ret;
        std::condition_variable done;
        std::mutex req_mutex;
        SyscallReq(siginfo_t* siginfo, struct ucontext* context);
};

class BpfHook {
    public:
        BpfHook(std::vector<uint32_t> syscall_numbers);
        int thread_handler();
        int install();
        static int init();

        std::vector<uint32_t> syscall_numbers; //syscalls to hook
        struct sock_fprog prog;
        std::mutex queue_mutex;
        std::vector<SyscallReq*> queue; 
        std::condition_variable available; //if there's anything available on the queue
        std::thread* thread; //servicer thread
        
};

#endif