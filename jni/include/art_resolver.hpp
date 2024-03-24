#ifndef __ART_RESOLVER
#define __ART_RESOLVER
#include "pmparser.hpp"
#include "jni.h"
#include <vector> 


typedef void* classlinker;
typedef void* art_runtime;
typedef void* art_thread;
typedef void* art_class;

typedef jobject (*AddGlobalRef)(JavaVM*, art_thread, jobject) ;
typedef void (*visitClasses)(classlinker, struct ClassVisitor*);

extern "C" classlinker getClassLinker();

extern "C" art_runtime getRuntime();

extern "C" void printClasses(struct proc_lib* libart);

extern "C" art_thread getThread();

//This construction is straight stolen from Frida
//https://github.com/frida/frida-java-bridge/blob/1e23abb71fd26726d59627e4da3ad8e10ba849aa/lib/android.js#L1303
struct ClassVisitorVtable {
    void* reserved1; //typeinfo bs
    void* reserved2;
    bool (*func)(struct ClassVisitor*, art_class klass);
};

struct ClassVisitor{
    struct ClassVisitorVtable* vtable;
    AddGlobalRef AddGlobalRef;
};

class ArtResolver {
public:
    ArtResolver(struct proc_lib* libart);
    static classlinker getClassLinker();
    static art_runtime getRuntime();
    static art_thread getThread();
    static int getAPI();
    static void getVersion(char* buf);
    void enumerateClasses();
    void printClassNames();
    jobject getClassHandle(char* name);
    art_class getRawClass(char* name);
    static AddGlobalRef add_global_ref; 
    static visitClasses visit_classes;
    static std::vector<jobject>* classHandles;
    static std::vector<art_class>* rawClasses;
private:
    struct proc_lib* libart; 
};


#endif