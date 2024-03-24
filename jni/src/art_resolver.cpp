#include <sys/system_properties.h>
#include <stdlib.h>
#include "jenv.h"
#include "jni.h"
#include "elf_parser.hpp"
#include "pmparser.hpp"
#include "art_resolver.hpp"

#define API_PROP "ro.build.version.sdk"
#define ANDROID_VERSION_PROP "ro.build.version.release"

AddGlobalRef ArtResolver::add_global_ref;
visitClasses ArtResolver::visit_classes;
std::vector<jobject>* ArtResolver::classHandles;
std::vector<art_class>* ArtResolver::rawClasses;

extern JavaCTX ctx;

int ArtResolver::getAPI(){
    char buf[PROP_VALUE_MAX];
    __system_property_get(API_PROP, buf);
    return atoi(buf);
}

//buffer must be of size PROP_VALUE_MAX
void ArtResolver::getVersion(char* buf){
    __system_property_get(ANDROID_VERSION_PROP, buf);
}

ArtResolver::ArtResolver(struct proc_lib* libart){
    this->libart = libart;

    if(!ArtResolver::add_global_ref){
       add_global_ref = (AddGlobalRef)find_dyn_symbol(this->libart, "_ZN3art9JavaVMExt12AddGlobalRefEPNS_6ThreadENS_6ObjPtrINS_6mirror6ObjectEEE");
    }

    if(!ArtResolver::visit_classes){
       visit_classes = (visitClasses)find_dyn_symbol(this->libart, "_ZN3art11ClassLinker12VisitClassesEPNS_12ClassVisitorE"); //art::ClassLinker::VisitClasses
    }

    if(!ArtResolver::classHandles){
       ArtResolver::classHandles = new std::vector<jobject>();
    }

    if(!ArtResolver::rawClasses){
       ArtResolver::rawClasses = new std::vector<art_class>();
    }

    
}



art_runtime ArtResolver::getRuntime(){
    //https://cs.android.com/android/platform/superproject/main/+/main:art/runtime/jni/java_vm_ext.h;l=244;bpv=0;bpt=0?q=Java&ss=android%2Fplatform%2Fsuperproject%2Fmain:art%2Fruntime%2F - first element after the JavaVM (Note that JavaVM is a superclass so its elements come first)
    return *(art_runtime*)(((char*)ctx.vm)+8);
}

#define __get_tls() ({ void** __val; __asm__("mrs %0, tpidr_el0" : "=r"(__val)); __val; })

art_thread ArtResolver::getThread(){
    //https://cs.android.com/android/platform/superproject/main/+/main:art/runtime/thread-current-inl.h;l=30;drc=a1df1ca465137bf665204c2757b7353c3a72c546;bpv=0;bpt=1
    void** tls = __get_tls();
#define TLS_SLOT_ART_THREAD_SELF  7
    return (art_thread)tls[TLS_SLOT_ART_THREAD_SELF];

}

classlinker ArtResolver::getClassLinker(){
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

/**
This is based on the technique from frida: https://github.com/frida/frida-java-bridge/blob/1e23abb71fd26726d59627e4da3ad8e10ba849aa/index.js#L230 

This function is also a prototype for: https://cs.android.com/android/platform/superproject/main/+/main:art/runtime/class_linker.h;l=113;drc=a1df1ca465137bf665204c2757b7353c3a72c546?q=ClassVisitor&ss=android%2Fplatform%2Fsuperproject%2Fmain:art%2F

*/
extern "C" bool get_class(struct ClassVisitor* cv, art_class klass){
    //we need to add the class as a global ref because the ART VM won't accept a normal class 
    //https://cs.android.com/android/platform/superproject/main/+/main:art/runtime/scoped_thread_state_change-inl.h;l=89;drc=13d7a47602f63cde716276908531eecb85813f7a;bpv=0;bpt=0 - this is how jobject/jclass are decoded.
    jobject new_ref  = ArtResolver::add_global_ref(ctx.vm, ArtResolver::getThread(), (jobject)klass);
   // printf("new class: %p\n", new_ref);
    ArtResolver::classHandles->push_back(new_ref);   
    ArtResolver::rawClasses->push_back(klass);
    return true;
}

//get all loaded class handles
void ArtResolver::enumerateClasses(){
    struct ClassVisitor* cv = (struct ClassVisitor*)malloc(sizeof(*cv));
    struct ClassVisitorVtable* vtable = (struct ClassVisitorVtable*)malloc(sizeof(*vtable));
    cv->vtable = vtable;
    vtable->func = get_class;
    
    ArtResolver::visit_classes(getClassLinker(), cv);

    free(vtable);
    free(cv);
}


void ArtResolver::printClassNames(){

    jclass javalangclass = ctx.env->FindClass("java/lang/Class");
    jmethodID getname = ctx.env->GetMethodID(javalangclass, "getName", "()Ljava/lang/String;");
    
    for(int i = 0; i < classHandles->size(); i++){
        jboolean isCopy = false;
        jstring nameString = (jstring)ctx.env->CallObjectMethod(classHandles->at(i), getname);

        char* name = (char*)ctx.env->GetStringUTFChars(nameString, &isCopy);
        printf("%s\n", name);
        ctx.env->ReleaseStringUTFChars(nameString, name);
        
        
    }
}

//returns the class handle
jobject ArtResolver::getClassHandle(char* name){
    jclass javalangclass = ctx.env->FindClass("java/lang/Class");
    jmethodID getname = ctx.env->GetMethodID(javalangclass, "getName", "()Ljava/lang/String;");
    for(int i = 0; i < classHandles->size(); i++){
        jboolean isCopy = false;
        jstring nameString = (jstring)ctx.env->CallObjectMethod(classHandles->at(i), getname);

        char* className = (char*)ctx.env->GetStringUTFChars(nameString, &isCopy);
        
        if(!strcmp(name, className)){
            ctx.env->ReleaseStringUTFChars(nameString, name);
            return classHandles->at(i);
        }

        ctx.env->ReleaseStringUTFChars(nameString, name);
    }
}

art_class ArtResolver::getRawClass(char* name){
    jclass javalangclass = ctx.env->FindClass("java/lang/Class");
    jmethodID getname = ctx.env->GetMethodID(javalangclass, "getName", "()Ljava/lang/String;");
    for(int i = 0; i < classHandles->size(); i++){
        jboolean isCopy = false;
        jstring nameString = (jstring)ctx.env->CallObjectMethod(classHandles->at(i), getname);

        char* className = (char*)ctx.env->GetStringUTFChars(nameString, &isCopy);
        
        if(!strcmp(name, className)){
            ctx.env->ReleaseStringUTFChars(nameString, name);
            return rawClasses->at(i);
        }

        ctx.env->ReleaseStringUTFChars(nameString, name);
    }
}

void ArtResolver::printMethods(art_class class){

}

