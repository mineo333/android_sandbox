#include "bpf_syscall.hpp"
#include <linux/prctl.h>  /* Definition of PR_* constants */
#include <sys/prctl.h>
#include <linux/seccomp.h>  /* Definition of SECCOMP_* constants */
#include <linux/filter.h>   /* Definition of struct sock_fprog */
#include <linux/audit.h>    /* Definition of AUDIT_* constants */
#include <linux/filter.h>
#include <sys/syscall.h>    /* Definition of SYS_* constants */
#include <unistd.h>
#include <vector>
#include <stdio.h>
/**

The theory here is simple. seccomp bpf applies to the current task_struct (NOT task_group) and any sub-processes that the current task generates.

As such, we can pthread_create (Which is basically a glorified clone) a new 
*/



BpfHook::BpfHook(std::vector<uint32_t> syscall_numbers) :
    syscall_numbers(syscall_numbers)
{
    //calculate the number of instructions:
    if(syscall_numbers.size() + 3 > 256) {
        fprintf(stderr, "Too many syscall numbers");
        abort();
    }
    uint8_t total_instructions = 3 + syscall_numbers.size(); //3 for the load, and seccomp ret trap/seccomp ret allow
    struct sock_filter* insns = new struct sock_filter[total_instructions];
    uint8_t trap_idx = total_instructions-1;
    uint8_t allow_idx = total_instructions-2;
    insns[0] = BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr));
    for(uint8_t i = 0; i<syscall_numbers.size(); i++) {
        uint32_t cur_syscall = syscall_numbers[i];
        uint8_t cur_idx = i+1; 
        insns[i+1] = BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, cur_syscall, (uint8_t)(trap_idx-cur_idx-1), 0);
    }
    //if the jmp falls through every single jeq then we're good. seccomp_ret_allow
    insns[allow_idx] = BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW);
    insns[trap_idx] = BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_TRAP);
    prog.filter = insns;
    prog.len = total_instructions;
}


extern "C" void trap_handler(int signo, siginfo_t *info, void *context) {
    fprintf(stderr, "Trapped syscall: %d\n", info->si_syscall);
    exit(1);
}


//we shouldn't need a destructor because a BpfHook should never be destroyed
int BpfHook::install() {
    return prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog);
}


int BpfHook::init() {
    struct bpf_sigaction act = {
        0
    };

    act.sa_sigaction = trap_handler,
    act.sa_flags = SA_SIGINFO,
    printf("%d\n", syscall(SYS_rt_sigaction, SIGSYS, &act, NULL));
    perror("Failed");
    return 0;
}

