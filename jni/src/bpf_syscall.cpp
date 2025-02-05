#include "bpf_syscall.hpp"
#include <linux/prctl.h>  /* Definition of PR_* constants */
#include <sys/prctl.h>
#include <linux/seccomp.h>  /* Definition of SECCOMP_* constants */
#include <linux/filter.h>   /* Definition of struct sock_fprog */
#include <linux/audit.h>    /* Definition of AUDIT_* constants */
#include <linux/filter.h>
#include <sys/syscall.h>    /* Definition of SYS_* constants */
#include <signal.h>
#include <unistd.h>
#include <vector>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <ucontext.h>

/**

The theory here is simple. seccomp bpf applies to the current task_struct (NOT task_group) and any sub-processes that the current task generates.

As such, we can pthread_create (Which is basically a glorified clone) a new thread that 
*/


BpfHook* g_bpfhook = NULL; //we only have 1 global bpf_hook


SyscallReq::SyscallReq(siginfo_t* info, struct ucontext* context){
    syscall_number = info->si_syscall;
    args[0] = context->uc_mcontext.regs[0]; //get x0->x5 as the arguments
    args[1] = context->uc_mcontext.regs[1];
    args[2] = context->uc_mcontext.regs[2];
    args[3] = context->uc_mcontext.regs[3];
    args[4] = context->uc_mcontext.regs[4];
    args[5] = context->uc_mcontext.regs[5];
}


BpfHook::BpfHook(std::vector<uint32_t> syscall_numbers) :
    syscall_numbers(syscall_numbers)
{
    //calculate the number of instructions:
    if(g_bpfhook) {
        fprintf(stderr, "Cannot have more than one bpf hook per program\n");
        abort();
    }

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


extern "C" void trap_handler(int signo, siginfo_t *info, void* context) {
    struct ucontext* u_context = (struct ucontext*)context;
    SyscallReq req(info, u_context);
    assert(g_bpfhook != NULL);
    printf("hit trap: %d\n", info->si_syscall);
    std::unique_lock<std::mutex> queue_lock(g_bpfhook->queue_mutex); //take the locks
    std::unique_lock<std::mutex> req_lock(req.req_mutex);
    g_bpfhook->queue.push_back(&req);
    queue_lock.unlock();
    g_bpfhook->available.notify_all(); 
    req.done.wait(req_lock);
    u_context->uc_mcontext.regs[0] = req.ret;
}


//we shouldn't need a destructor because a BpfHook should never be destroyed

int BpfHook::thread_handler() {
    std::unique_lock<std::mutex> queue_lock(queue_mutex);
    
    for(;;) {
        if(!queue.size())
            available.wait(queue_lock); //release the lock here and wait until available is done
        //at this point the lock is taken again
        SyscallReq* req = queue[0];
        std::unique_lock<std::mutex> req_lock(req->req_mutex); 
        req->ret = syscall(req->syscall_number, req->args[0], req->args[1], req->args[2], req->args[3], req->args[4], req->args[5]);
        queue.erase(queue.begin()); //remove the request from the queue
        req->done.notify_one(); //notify the owner of the request that we're done
    }

    return 0;
}



int BpfHook::install() {
    g_bpfhook = this;
    thread = new std::thread(&BpfHook::thread_handler, this);
    return prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog);
}


int BpfHook::init() {
    struct sigaction act = {
        0
    };

    act.sa_sigaction = trap_handler,
    act.sa_flags = SA_SIGINFO;
    
    int ret = sigaction(SIGSYS, &act, NULL);
    return ret;
}