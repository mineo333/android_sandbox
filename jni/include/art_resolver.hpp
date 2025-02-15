#ifndef __ART_RESOLVER
#define __ART_RESOLVER
#include "pmparser.hpp"
#include "jni.h"
#include <vector> 
#include <stdint.h>
#include <iterator>
#include "util.hpp"

typedef void* classlinker;
typedef void* art_runtime;
typedef void* art_thread;
typedef void* jni_id_manager;
/**
A heap reference is a compressed 32 bit pointer: https://cs.android.com/android/platform/superproject/main/+/main:art/runtime/mirror/object_reference.h;l=216;drc=13d7a47602f63cde716276908531eecb85813f7a

Notice how in the compress-store function the pointer is casted to a uint32_t - https://cs.android.com/android/platform/superproject/main/+/main:art/runtime/mirror/object_reference.h;drc=13d7a47602f63cde716276908531eecb85813f7a;bpv=0;bpt=1;l=99

Decompress simply casts it back to its mirror type: https://cs.android.com/android/platform/superproject/main/+/main:art/runtime/mirror/object_reference.h;l=105;drc=13d7a47602f63cde716276908531eecb85813f7a;bpv=0;bpt=1 

*/
typedef uint32_t heap_ref; 
template <typename T>
class LengthPrefixedArray {
    public:
    /**
    The data is the type itself: https://cs.android.com/android/platform/superproject/main/+/main:art/libartbase/base/length_prefixed_array.h;l=97;drc=06bd5d4af5ec3ac61904303953b6f96bdf6eae4c (notice how we are casting the offset to the pointer type)
    */
    class iterator {
        private:
            T* ptr;
        public: 
            using iterator_category = std::forward_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = T*;
            using reference = T&;

            explicit iterator(T* p) : ptr(p) {}


            T& operator*() { return *ptr; }
            T* operator->() { return ptr; }
            T* operator&() { return ptr; }

            iterator& operator++() {
                ++ptr;
                return *this;
            }

            bool operator==(const iterator& other) const { return ptr == other.ptr; }
            bool operator!=(const iterator& other) const { return ptr != other.ptr; }
    };


    //utter woke C++ code 
    iterator begin() { return iterator((T*)((char*)this + ALIGN(offsetof(LengthPrefixedArray, data), alignof(T)))); }
    iterator end() { return iterator((T*)((char*)this + ALIGN(offsetof(LengthPrefixedArray, data), alignof(T))) + size); }

    
    uint32_t size;
    uint8_t data[0];  //the data contains the actual array elements themselvs
};


//Based on: https://cs.android.com/android/platform/superproject/main/+/main:art/runtime/mirror/object.h;l=805;drc=06bd5d4af5ec3ac61904303953b6f96bdf6eae4c
typedef struct _Object {
    heap_ref klass;
    uint32_t monitor;
} Object;




//based on: https://cs.android.com/android/platform/superproject/main/+/main:art/runtime/art_method.h;l=87;drc=06bd5d4af5ec3ac61904303953b6f96bdf6eae4c?q=ArtMethod&ss=android%2Fplatform%2Fsuperproject%2Fmain
typedef struct _ArtMethod {
    uint32_t declaring_class;
    uint32_t access_flags;
    uint32_t dex_method_idx;
    uint16_t method_index;
    union {
        uint16_t hotness_count;
        uint16_t imt_index;
    };

      struct PtrSizedFields {
        void* data;
        void* entry_point_from_quick_compiled_code;
    } ptr_sized_fields_;
} ArtMethod;


typedef struct _Throwable {
    Object object; //object is the superclass;
    heap_ref backtrace;
    heap_ref cause;
    heap_ref detail_message;
    heap_ref stack_trace;
    heap_ref suppressed_exceptions;

    void printMessage();
} Throwable;

//https://cs.android.com/android/platform/superproject/main/+/main:art/runtime/art_field.h;l=262;drc=7e0718c02d9396171bfed752e95c84f1dbc979e6
typedef struct _ArtField {
    heap_ref declaring_class;
    uint32_t access_flags;
    uint32_t field_dex_idx;
    uint32_t offset;
} ArtField;


//based on: https://cs.android.com/android/platform/superproject/main/+/main:art/runtime/mirror/class.h;l=1445;drc=13d7a47602f63cde716276908531eecb85813f7a 
typedef struct _ArtClass {
    //no vtable
    Object object; //object is a superclass
    heap_ref class_loader;
    heap_ref component_type;
    heap_ref dex_cache; 
    heap_ref ext_data;
    heap_ref iftable;
    heap_ref name;
    heap_ref superclass;
    heap_ref vtable; //java vtable
    LengthPrefixedArray<ArtField>* fields;
    LengthPrefixedArray<ArtMethod>* methods; //this is a LengthPrefixedArray<ArtMethod>*: https://cs.android.com/android/platform/superproject/main/+/main:art/runtime/mirror/class-inl.h;drc=13d7a47602f63cde716276908531eecb85813f7a;l=200 
} ArtClass; //this is usually passed as a ObjPtr<mirror::Class>


//Difference between jni_env and exception: https://cs.android.com/android/platform/superproject/main/+/main:art/runtime/thread.h;l=2177;drc=06bd5d4af5ec3ac61904303953b6f96bdf6eae4c
#define SIZE_OF_MANAGED_STACK (3)
#define EXCEPTION_DIFF (3 + SIZE_OF_MANAGED_STACK)
//offset is EXCEPTION_OFFSET*sizeof(size_t)



typedef jobject (*AddGlobalRef)(JavaVM*, art_thread, jobject); //https://cs.android.com/android/platform/superproject/main/+/main:art/runtime/jni/java_vm_ext.cc;l=697?q=AddGlobalRef&ss=android%2Fplatform%2Fsuperproject%2Fmain:art%2F
typedef void (*visitClasses)(classlinker, struct ClassVisitor*); //https://cs.android.com/android/platform/superproject/main/+/main:art/runtime/class_linker.cc;l=2543;drc=f13bf4bc97e7df89822d4a35376f6f26d65fa3ce;bpv=0;bpt=1
typedef std::string (*PrettyMethod)(ArtMethod*, bool); //https://cs.android.com/android/platform/superproject/main/+/main:art/runtime/art_method.h;l=1026?q=Pretty&sq=&ss=android%2Fplatform%2Fsuperproject%2Fmain:art%2F
typedef std::string (*PrettyField)(ArtField*, bool); //https://cs.android.com/android/platform/superproject/main/+/main:art/runtime/art_field.h;l=248;drc=7e0718c02d9396171bfed752e95c84f1dbc979e6?q=ArtField&ss=android%2Fplatform%2Fsuperproject%2Fmain

typedef jmethodID (*GetCanonicalMethod)(ArtMethod*, size_t); //https://cs.android.com/android/platform/superproject/main/+/main:art/runtime/art_method.cc;l=69;drc=06bd5d4af5ec3ac61904303953b6f96bdf6eae4c;bpv=0;bpt=1

//Note the above doesn't actually return jmethodID, but we end up casting it here: https://cs.android.com/android/platform/superproject/main/+/main:art/runtime/jni/jni_internal.h;l=138;drc=06bd5d4af5ec3ac61904303953b6f96bdf6eae4c

typedef std::string (*Dump)(Throwable*); //https://cs.android.com/android/platform/superproject/main/+/main:art/runtime/mirror/throwable.h;l=40;drc=b7225f7d6ea0bdf6524922a6d852d2477fc6881c;bpv=0;bpt=1

extern "C" classlinker getClassLinker();

extern "C" art_runtime getRuntime();

extern "C" void printClasses(struct proc_lib* libart);

extern "C" art_thread getThread();

//This construction is straight stolen from Frida
//https://github.com/frida/frida-java-bridge/blob/1e23abb71fd26726d59627e4da3ad8e10ba849aa/lib/android.js#L1303
struct ClassVisitorVtable {
    void* reserved1; //typeinfo bs
    void* reserved2;
    bool (*func)(struct ClassVisitor*, ArtClass* klass);
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
    static Throwable* getException();
    static int getAPI();
    static void getVersion(char* buf);
    static void printException();
    void enumerateClasses();
    void printClassNames();
    void printFields(char* name);
    jobject getClassHandle(char* name);
    jfieldID findField(char* class_name, char* field_name);
    ArtClass* getRawClass(char* name);
    jmethodID findMethodClass(char* className, char* methodName);
    void printMethodsForClass(char* name);
    static AddGlobalRef add_global_ref; 
    static visitClasses visit_classes;
    static PrettyMethod pretty_method;
    static PrettyField pretty_field;
    static GetCanonicalMethod get_canonical_method;
    static Dump dump_exception;
    static std::vector<jobject>* classHandles;
    static std::vector<ArtClass*>* rawClasses;
private:
    struct proc_lib* libart; 
};


#endif