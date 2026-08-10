#pragma once

#include <cstdint>
#include <cstddef>

typedef void Il2CppDomain;
typedef void Il2CppAssembly;
typedef void Il2CppImage;
typedef void Il2CppClass;
typedef void Il2CppObject;
typedef void Il2CppString;
typedef void Il2CppType;
typedef void Il2CppThread;
typedef void FieldInfo;
typedef void MethodInfo;
typedef void Il2CppException;

struct Vector3 {
    float x;
    float y;
    float z;
};

struct Quaternion {
    float x;
    float y;
    float z;
    float w;
};

namespace il2cpp {

bool Initialize();

Il2CppDomain *DomainGet();
Il2CppThread *ThreadAttach(Il2CppDomain *domain);
Il2CppImage *FindImage(const char *name);
Il2CppClass *FindClass(const char *imageName, const char *ns, const char *name);
FieldInfo *FindField(Il2CppClass *klass, const char *name);
MethodInfo *FindMethod(Il2CppClass *klass, const char *name, int argCount);
uint32_t FieldOffset(FieldInfo *field);
const char *FieldTypeName(FieldInfo *field);
void *MethodPointer(MethodInfo *method);
Il2CppObject *Invoke(MethodInfo *method, void *instance, void **args);
Il2CppString *NewString(const char *value);
Il2CppType *ClassType(Il2CppClass *klass);
Il2CppObject *TypeObject(Il2CppType *type);
const char *ObjectClassName(void *object);
MethodInfo *FindMethodByParamType(Il2CppClass *klass, const char *name, const char *paramTypeName);

}

namespace field {

bool SetByName(void *instance, Il2CppClass *klass, const char *name, float value);
bool SetBoolByName(void *instance, Il2CppClass *klass, const char *name, bool value);
bool GetFloatByName(void *instance, Il2CppClass *klass, const char *name, float *out);
void *GetPointerByName(void *instance, Il2CppClass *klass, const char *name);

}
