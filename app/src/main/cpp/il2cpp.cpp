#include "il2cpp.h"
#include "log.h"

#include <dlfcn.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

namespace {

void *g_handle = nullptr;

Il2CppDomain *(*p_domain_get)() = nullptr;
const Il2CppAssembly **(*p_domain_get_assemblies)(const Il2CppDomain *, size_t *) = nullptr;
const Il2CppImage *(*p_assembly_get_image)(const Il2CppAssembly *) = nullptr;
const char *(*p_image_get_name)(const Il2CppImage *) = nullptr;
Il2CppClass *(*p_class_from_name)(const Il2CppImage *, const char *, const char *) = nullptr;
FieldInfo *(*p_class_get_field_from_name)(Il2CppClass *, const char *) = nullptr;
const MethodInfo *(*p_class_get_method_from_name)(Il2CppClass *, const char *, int) = nullptr;
size_t (*p_field_get_offset)(FieldInfo *) = nullptr;
Il2CppType *(*p_field_get_type)(FieldInfo *) = nullptr;
char *(*p_type_get_name)(const Il2CppType *) = nullptr;
Il2CppThread *(*p_thread_attach)(Il2CppDomain *) = nullptr;
Il2CppObject *(*p_runtime_invoke)(const MethodInfo *, void *, void **, Il2CppException **) = nullptr;
Il2CppString *(*p_string_new)(const char *) = nullptr;
Il2CppObject *(*p_object_new)(Il2CppClass *) = nullptr;
const Il2CppType *(*p_class_get_type)(Il2CppClass *) = nullptr;
Il2CppObject *(*p_type_get_object)(const Il2CppType *) = nullptr;
Il2CppClass *(*p_object_get_class)(Il2CppObject *) = nullptr;
const char *(*p_class_get_name)(Il2CppClass *) = nullptr;
const MethodInfo *(*p_class_get_methods)(Il2CppClass *, void **) = nullptr;
const char *(*p_method_get_name)(const MethodInfo *) = nullptr;
uint32_t (*p_method_get_param_count)(const MethodInfo *) = nullptr;
Il2CppType *(*p_method_get_param)(const MethodInfo *, uint32_t) = nullptr;

template <typename T>
bool Resolve(T &target, const char *symbol) {
    target = reinterpret_cast<T>(dlsym(g_handle, symbol));
    if (target == nullptr) {
        LOGE("missing il2cpp symbol: %s", symbol);
        return false;
    }
    return true;
}

}

namespace il2cpp {

bool Initialize() {
    if (g_handle != nullptr) {
        return true;
    }
    for (int attempt = 0; attempt < 1200; ++attempt) {
        g_handle = dlopen("libil2cpp.so", RTLD_NOW | RTLD_NOLOAD);
        if (g_handle != nullptr) {
            LOGI("libil2cpp.so already loaded by the game after %d ticks", attempt);
            break;
        }
        usleep(100000);
    }
    if (g_handle == nullptr) {
        LOGE("libil2cpp.so not loaded");
        return false;
    }
    bool ok = true;
    ok &= Resolve(p_domain_get, "il2cpp_domain_get");
    ok &= Resolve(p_domain_get_assemblies, "il2cpp_domain_get_assemblies");
    ok &= Resolve(p_assembly_get_image, "il2cpp_assembly_get_image");
    ok &= Resolve(p_image_get_name, "il2cpp_image_get_name");
    ok &= Resolve(p_class_from_name, "il2cpp_class_from_name");
    ok &= Resolve(p_class_get_field_from_name, "il2cpp_class_get_field_from_name");
    ok &= Resolve(p_class_get_method_from_name, "il2cpp_class_get_method_from_name");
    ok &= Resolve(p_field_get_offset, "il2cpp_field_get_offset");
    ok &= Resolve(p_field_get_type, "il2cpp_field_get_type");
    ok &= Resolve(p_type_get_name, "il2cpp_type_get_name");
    ok &= Resolve(p_thread_attach, "il2cpp_thread_attach");
    ok &= Resolve(p_runtime_invoke, "il2cpp_runtime_invoke");
    ok &= Resolve(p_string_new, "il2cpp_string_new");
    ok &= Resolve(p_object_new, "il2cpp_object_new");
    ok &= Resolve(p_class_get_type, "il2cpp_class_get_type");
    ok &= Resolve(p_type_get_object, "il2cpp_type_get_object");
    ok &= Resolve(p_object_get_class, "il2cpp_object_get_class");
    ok &= Resolve(p_class_get_name, "il2cpp_class_get_name");
    ok &= Resolve(p_class_get_methods, "il2cpp_class_get_methods");
    ok &= Resolve(p_method_get_name, "il2cpp_method_get_name");
    ok &= Resolve(p_method_get_param_count, "il2cpp_method_get_param_count");
    ok &= Resolve(p_method_get_param, "il2cpp_method_get_param");
    return ok;
}

Il2CppDomain *DomainGet() {
    if (p_domain_get == nullptr) {
        return nullptr;
    }
    return p_domain_get();
}

Il2CppThread *ThreadAttach(Il2CppDomain *domain) {
    if (p_thread_attach == nullptr || domain == nullptr) {
        return nullptr;
    }
    return p_thread_attach(domain);
}

Il2CppImage *FindImage(const char *name) {
    Il2CppDomain *domain = DomainGet();
    if (domain == nullptr) {
        return nullptr;
    }
    size_t count = 0;
    const Il2CppAssembly **assemblies = p_domain_get_assemblies(domain, &count);
    if (assemblies == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < count; ++i) {
        const Il2CppImage *image = p_assembly_get_image(assemblies[i]);
        if (image == nullptr) {
            continue;
        }
        const char *imageName = p_image_get_name(image);
        if (imageName != nullptr && strcmp(imageName, name) == 0) {
            return const_cast<Il2CppImage *>(image);
        }
    }
    return nullptr;
}

Il2CppClass *FindClass(const char *imageName, const char *ns, const char *name) {
    Il2CppImage *image = FindImage(imageName);
    if (image == nullptr) {
        LOGE("image not found: %s", imageName);
        return nullptr;
    }
    Il2CppClass *klass = p_class_from_name(image, ns, name);
    if (klass == nullptr) {
        LOGE("class not found: %s::%s", ns, name);
    }
    return klass;
}

FieldInfo *FindField(Il2CppClass *klass, const char *name) {
    if (klass == nullptr) {
        return nullptr;
    }
    return p_class_get_field_from_name(klass, name);
}

MethodInfo *FindMethod(Il2CppClass *klass, const char *name, int argCount) {
    if (klass == nullptr) {
        return nullptr;
    }
    return const_cast<MethodInfo *>(p_class_get_method_from_name(klass, name, argCount));
}

uint32_t FieldOffset(FieldInfo *field) {
    if (field == nullptr) {
        return 0;
    }
    return static_cast<uint32_t>(p_field_get_offset(field));
}

const char *FieldTypeName(FieldInfo *field) {
    if (field == nullptr) {
        return nullptr;
    }
    Il2CppType *type = p_field_get_type(field);
    if (type == nullptr) {
        return nullptr;
    }
    return p_type_get_name(type);
}

void *MethodPointer(MethodInfo *method) {
    if (method == nullptr) {
        return nullptr;
    }
    return *reinterpret_cast<void **>(method);
}

Il2CppObject *Invoke(MethodInfo *method, void *instance, void **args) {
    if (method == nullptr) {
        return nullptr;
    }
    Il2CppException *exception = nullptr;
    Il2CppObject *result = p_runtime_invoke(method, instance, args, &exception);
    if (exception != nullptr) {
        LOGE("managed exception during invoke");
    }
    return result;
}

Il2CppString *NewString(const char *value) {
    if (p_string_new == nullptr) {
        return nullptr;
    }
    return p_string_new(value);
}

Il2CppObject *NewObject(Il2CppClass *klass) {
    if (p_object_new == nullptr || klass == nullptr) {
        return nullptr;
    }
    return p_object_new(klass);
}

Il2CppType *ClassType(Il2CppClass *klass) {
    if (klass == nullptr) {
        return nullptr;
    }
    return const_cast<Il2CppType *>(p_class_get_type(klass));
}

Il2CppObject *TypeObject(Il2CppType *type) {
    if (type == nullptr) {
        return nullptr;
    }
    return p_type_get_object(type);
}

MethodInfo *FindMethodByParamType(Il2CppClass *klass, const char *name, const char *paramTypeName) {
    if (klass == nullptr || p_class_get_methods == nullptr) {
        return nullptr;
    }
    void *iterator = nullptr;
    while (true) {
        const MethodInfo *method = p_class_get_methods(klass, &iterator);
        if (method == nullptr) {
            break;
        }
        const char *methodName = p_method_get_name(method);
        if (methodName == nullptr || strcmp(methodName, name) != 0) {
            continue;
        }
        if (p_method_get_param_count(method) != 1) {
            continue;
        }
        Il2CppType *param = p_method_get_param(method, 0);
        if (param == nullptr) {
            continue;
        }
        const char *typeName = p_type_get_name(param);
        if (typeName != nullptr && strcmp(typeName, paramTypeName) == 0) {
            LOGI("resolved overload %s(%s)", name, typeName);
            return const_cast<MethodInfo *>(method);
        }
    }
    LOGE("overload not found: %s(%s)", name, paramTypeName);
    return nullptr;
}

void LogOverloads(Il2CppClass *klass, const char *name) {
    if (klass == nullptr || p_class_get_methods == nullptr) {
        return;
    }
    void *iterator = nullptr;
    while (true) {
        const MethodInfo *method = p_class_get_methods(klass, &iterator);
        if (method == nullptr) {
            break;
        }
        const char *methodName = p_method_get_name(method);
        if (methodName == nullptr || strcmp(methodName, name) != 0) {
            continue;
        }
        uint32_t count = p_method_get_param_count(method);
        char buffer[512];
        int written = snprintf(buffer, sizeof(buffer), "%s(", name);
        for (uint32_t i = 0; i < count && written < (int) sizeof(buffer) - 2; ++i) {
            Il2CppType *param = p_method_get_param(method, i);
            const char *typeName = param != nullptr ? p_type_get_name(param) : "?";
            written += snprintf(buffer + written, sizeof(buffer) - written, "%s%s",
                                i > 0 ? ", " : "", typeName != nullptr ? typeName : "?");
        }
        snprintf(buffer + written, sizeof(buffer) - written, ")");
        LOGI("  available overload %s", buffer);
    }
}

const char *ObjectClassName(void *object) {
    if (object == nullptr || p_object_get_class == nullptr || p_class_get_name == nullptr) {
        return nullptr;
    }
    Il2CppClass *klass = p_object_get_class(object);
    if (klass == nullptr) {
        return nullptr;
    }
    return p_class_get_name(klass);
}

}

namespace {

bool TypeIsBool(const char *typeName) {
    return typeName != nullptr && strcmp(typeName, "System.Boolean") == 0;
}

bool TypeIsFloat(const char *typeName) {
    return typeName != nullptr && strcmp(typeName, "System.Single") == 0;
}

bool TypeIsInt(const char *typeName) {
    return typeName != nullptr && strcmp(typeName, "System.Int32") == 0;
}

}

namespace field {

bool SetByName(void *instance, Il2CppClass *klass, const char *name, float value) {
    if (instance == nullptr) {
        return false;
    }
    FieldInfo *info = il2cpp::FindField(klass, name);
    if (info == nullptr) {
        return false;
    }
    uint32_t offset = il2cpp::FieldOffset(info);
    if (offset == 0) {
        return false;
    }
    const char *typeName = il2cpp::FieldTypeName(info);
    uint8_t *base = reinterpret_cast<uint8_t *>(instance) + offset;
    if (TypeIsFloat(typeName)) {
        *reinterpret_cast<float *>(base) = value;
        return true;
    }
    if (TypeIsInt(typeName)) {
        *reinterpret_cast<int32_t *>(base) = static_cast<int32_t>(value);
        return true;
    }
    if (TypeIsBool(typeName)) {
        *reinterpret_cast<uint8_t *>(base) = value != 0.0f ? 1 : 0;
        return true;
    }
    return false;
}

bool SetBoolByName(void *instance, Il2CppClass *klass, const char *name, bool value) {
    return SetByName(instance, klass, name, value ? 1.0f : 0.0f);
}

bool GetFloatByName(void *instance, Il2CppClass *klass, const char *name, float *out) {
    if (instance == nullptr || out == nullptr) {
        return false;
    }
    FieldInfo *info = il2cpp::FindField(klass, name);
    if (info == nullptr) {
        return false;
    }
    uint32_t offset = il2cpp::FieldOffset(info);
    if (offset == 0) {
        return false;
    }
    const char *typeName = il2cpp::FieldTypeName(info);
    uint8_t *base = reinterpret_cast<uint8_t *>(instance) + offset;
    if (TypeIsFloat(typeName)) {
        *out = *reinterpret_cast<float *>(base);
        return true;
    }
    if (TypeIsInt(typeName)) {
        *out = static_cast<float>(*reinterpret_cast<int32_t *>(base));
        return true;
    }
    if (TypeIsBool(typeName)) {
        *out = *reinterpret_cast<uint8_t *>(base) != 0 ? 1.0f : 0.0f;
        return true;
    }
    return false;
}

void *GetPointerByName(void *instance, Il2CppClass *klass, const char *name) {
    if (instance == nullptr) {
        return nullptr;
    }
    FieldInfo *info = il2cpp::FindField(klass, name);
    if (info == nullptr) {
        return nullptr;
    }
    uint32_t offset = il2cpp::FieldOffset(info);
    if (offset == 0) {
        return nullptr;
    }
    return *reinterpret_cast<void **>(reinterpret_cast<uint8_t *>(instance) + offset);
}

}
