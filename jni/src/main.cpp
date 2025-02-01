#include <stdio.h>
#include <stdlib.h>
#include "include/jni.h"
#include "include/jenv.h"
#include <sys/mman.h>
#include <sys/syscall.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <dlfcn.h>
#include "pmparser.hpp"
#include "elf_parser.hpp"
#include "art_resolver.hpp"
#include "bpf_syscall.hpp"


#define LIBART (char*)"/apex/com.android.art/lib64/libart.so" 


JavaCTX ctx;
struct JNINativeInterface jni_vtable; //our own custom jni_vtable

char* options[] = {(char*)"-Djava.class.path=/data/local/tmp/droidguard.apk", (char*)"-Djava.library.path=/data/local/tmp", (char*)"-verbose:jni"};

void create_vm(){
  int res = init_java_env(&ctx, (char**)&options, 3);

  if(res != JNI_OK){
    exit(-1);
  }

}

jint (*Original_RegisterNatives)(JNIEnv*, jclass, const JNINativeMethod*, jint) = NULL;
jclass (*Original_FindClass)(JNIEnv*, const char*) = NULL;

jint RegisterNatives_hook(JNIEnv* env, jclass clazz, const JNINativeMethod* method, jint nMethods){
  for(int i = 0; i < nMethods; i++){
    printf("NEW METHOD:\nname: %s\nsig: %s\naddr: %p\n", method[i].name, method[i].signature, method[i].fnPtr);
  }

  return Original_RegisterNatives(env, clazz, method, nMethods);
}

jclass FindClass_hook(JNIEnv* env, const char* name){
  printf("Looking for class: %s\n", name);

  return Original_FindClass(env, name);
}



int main(){
  
  printf("%s\n", options[0]);
  printf("Calling create_vm\n");
  create_vm();
  //THIS IS CPP BTW
  Original_RegisterNatives = ctx.env->functions->RegisterNatives;
  Original_FindClass = ctx.env->functions->FindClass; //get original functions

  
  memcpy(&jni_vtable, ctx.env->functions, sizeof(jni_vtable)); //copy the vtable to our own custom vtable

  jni_vtable.RegisterNatives = &RegisterNatives_hook; //replace the function pointer
  jni_vtable.FindClass = &FindClass_hook;
  ctx.env->functions = &jni_vtable; //replace the vtable
  printf("Calling onLoad\n");
  JNI_OnLoad(ctx.vm, NULL);
  /*jclass activity_thread = ctx.env->FindClass("android/app/ActivityThread");
  printf("Activity Thread: %p\n", activity_thread);
  printf("%p\n", ctx.env->GetStaticMethodID(activity_thread, "attach", "(ZJ)V"));*/
  
  //system("cat /proc/$PPID/maps");
  struct proc_lib* libart = find_library(LIBART);

  JavaVMAttachArgs args;
  args.version = JNI_VERSION_1_6; // choose your JNI version
  args.name = NULL; // you might want to give the java thread a name
  args.group = NULL; // you might want to assign the java thread to a ThreadGroup
  JNIEnv* jni_env;

  
  printf("Runtime: %p\n", ArtResolver::getRuntime());
  printf("ClassLinker: %p\n", ArtResolver::getClassLinker());
  printf("Thread: %p\n", ArtResolver::getThread());

  ArtResolver art_resolver(libart);

  art_resolver.enumerateClasses();

  
  //art_resolver.printMethodsForClass("android.app.ActivityThread");
  jmethodID systemMain = art_resolver.findMethodClass((char*)"android.app.ActivityThread", (char*)"systemMain");
  jmethodID prepareMainLooper = art_resolver.findMethodClass((char*)"android.os.Looper", (char*)"prepareMainLooper");
  
  
  printf("system main: %p\n", systemMain);
  jclass activity_thread = ctx.env->FindClass((char*)"android/app/ActivityThread");
  jclass looper = ctx.env->FindClass((char*)"android/os/Looper");
  
  printf("prepareMainLooper: %p\n", prepareMainLooper);
  //jobject activityThreadObj = ctx.env->NewObject(activity_thread, ctr);
  ctx.env->CallStaticObjectMethod(looper, prepareMainLooper);
  jobject activityThreadObj = ctx.env->CallStaticObjectMethod(activity_thread, systemMain);
 // printf("Activity Thread: %p\n", activityThreadObj);
  jfieldID context_impl = art_resolver.findField((char*)"android.app.ActivityThread", (char*)"mSystemContext");
  
  jobject context = ctx.env->GetObjectField(activityThreadObj, context_impl);
  printf("Context: %p\n", context);

  Throwable* exception = ArtResolver::getException();

  if(exception)
    exception->printMessage();

  printf("pid: %d\n", getpid());
  printf("pid: %d\n", syscall(172));
  std::vector<uint32_t> syscalls = { 172 };
  BpfHook::init();
  BpfHook hook = BpfHook(syscalls);
  hook.install();
  syscall(172);

  
  return 0;
}