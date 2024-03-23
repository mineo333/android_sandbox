#ifndef __ART_RESOLVER
#define __ART_RESOLVER
#include "pmparser.hpp"

typedef void* classlinker;
typedef void* art_runtime;
typedef void* art_thread;
typedef jobject (*AddGlobalRef)(art_thread, jobject) ;

extern "C" classlinker getClassLinker();

extern "C" art_runtime getRuntime();

extern "C" void printClasses(struct proc_lib* libart);

extern "C" art_thread getThread();

//This construction is straight stolen from Frida
//https://github.com/frida/frida-java-bridge/blob/1e23abb71fd26726d59627e4da3ad8e10ba849aa/lib/android.js#L1303
struct ClassVisitorVtable {
    void* reserved1; //typeinfo bs
    void* reserved2;
    bool (*func)(jclass klass);
};

struct ClassVisitor{
    struct ClassVisitorVtable* vtable;
    AddGlobalRef AddGlobalRef;
};

/*class ClassVisitor {
 public:
  virtual ~ClassVisitor() {}
  // Return true to continue visiting.
  virtual bool operator()(jclass klass) = 0;
};

class EmuClassVisitor {
    public:
        bool operator()(jclass klass);
};*/


#endif