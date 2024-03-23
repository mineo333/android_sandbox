#include <sys/system_properties.h>
#include <stdlib.h>
#include "jenv.h"
#include "jni.h"
#include "elf_parser.hpp"
#include "pmparser.hpp"
#include "art_resolver.hpp"

#define API_PROP "ro.build.version.sdk"
#define ANDROID_VERSION_PROP "ro.build.version.release"

typedef void (*visitClasses)(classlinker, struct ClassVisitor*);

extern JavaCTX ctx;

extern "C" int getAPI(){
    char buf[PROP_VALUE_MAX];
    __system_property_get(API_PROP, buf);
    return atoi(buf);
}


//buffer must be of size PROP_VALUE_MAX
extern "C" void getVersion(char* buf){
    __system_property_get(ANDROID_VERSION_PROP, buf);
}
//https://cs.android.com/android/platform/superproject/main/+/main:art/runtime/jni/java_vm_ext.h;l=244;bpv=0;bpt=0?q=Java&ss=android%2Fplatform%2Fsuperproject%2Fmain:art%2Fruntime%2F - first element after the runtime
extern "C" art_runtime getRuntime(){
    return *(art_runtime*)(((char*)ctx.vm)+8);
}

#define __get_tls() ({ void** __val; __asm__("mrs %0, tpidr_el0" : "=r"(__val)); __val; })
//https://cs.android.com/android/platform/superproject/main/+/main:art/runtime/thread-current-inl.h;l=30;drc=a1df1ca465137bf665204c2757b7353c3a72c546;bpv=0;bpt=1
extern "C" art_thread getThread(){
    void** tls = __get_tls();
#define TLS_SLOT_ART_THREAD_SELF  7
    return (art_thread)tls[7];

}

extern "C" classlinker getClassLinker(){
    art_runtime runtime = getRuntime();
    void** iter = (void**)runtime;
    int api = getAPI();
    char version[PROP_VALUE_MAX];
    getVersion(version);

    while(true){
        //we're aiming to find this element: https://cs.android.com/android/platform/superproject/main/+/main:art/runtime/runtime.h;l=1338;drc=f13bf4bc97e7df89822d4a35376f6f26d65fa3ce. This is the same as the VM passed back from: https://cs.android.com/android/platform/superproject/main/+/main:art/runtime/jni/java_vm_ext.cc;l=1255;bpv=0;bpt=0?q=JNI_GetCreatedJavaVMs&ss=android%2Fplatform%2Fsuperproject%2Fmain:art%2F 
        if(*iter == ctx.vm){
            break;
        }
        iter++;
    }

    //Using this technique, we can now backtrack to find ClassLinker. We utilize the method found in Frida: https://github.com/frida/frida-java-bridge/blob/1e23abb71fd26726d59627e4da3ad8e10ba849aa/lib/android.js#L607
    
    if(api >= 33){
        return *(classlinker*)(iter-4);
    }


    return NULL;
}




//https://github.com/frida/frida-java-bridge/blob/1e23abb71fd26726d59627e4da3ad8e10ba849aa/index.js#L230 
//this is based on the above technique from frida
bool print_class(jclass klass){
    printf("new class: %p\n", *(void**)klass);
    //use Class::getName to get the name as a string and then print it out
   // jobject globalRef = ctx.env->NewGlobalRef(klass);
   // printf("%p\n", globalRef);

   // ctx.env->DeleteLocalRef(globalRef);
    return true;
}


extern "C" void printClasses(struct proc_lib* libart){
    //EmuClassVisitor* cv = new EmuClassVisitor();
    struct ClassVisitor* cv = (struct ClassVisitor*)malloc(sizeof(*cv));
    struct ClassVisitorVtable* vtable = (struct ClassVisitorVtable*)malloc(sizeof(*vtable));
    cv->vtable = vtable;
   // class_to_find = ctx.env->FindClass("android/app/ActivityThread");
    cv->AddGlobalRef = (AddGlobalRef)find_dyn_symbol(libart, "_ZN3art9JavaVMExt12AddGlobalRefEPNS_6ThreadENS_6ObjPtrINS_6mirror6ObjectEEE"); //art::JavaVMExt::AddGlobalRef
    vtable->func = print_class;
    visitClasses cb = (visitClasses)find_dyn_symbol(libart, "_ZN3art11ClassLinker12VisitClassesEPNS_12ClassVisitorE"); //art::ClassLinker::VisitClasses
    printf("art::ClassLinker::VisitClasses: %p\n", cb);
    cb(getClassLinker(), cv);

    

    
}