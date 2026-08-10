#include "unity.h"
#include "log.h"

#include <cmath>

namespace {

const char *kCoreModule = "UnityEngine.CoreModule.dll";

typedef void *(*CreatePrimitiveFn)(int, const MethodInfo *);
typedef void *(*GetTransformFn)(void *, const MethodInfo *);
typedef void *(*GetGameObjectFn)(void *, const MethodInfo *);
typedef void (*SetActiveFn)(void *, bool, const MethodInfo *);
typedef void (*SetNameFn)(void *, Il2CppString *, const MethodInfo *);
typedef Vector3 (*GetVectorFn)(void *, const MethodInfo *);
typedef void (*SetVectorFn)(void *, Vector3, const MethodInfo *);
typedef void (*SetParentFn)(void *, void *, const MethodInfo *);
typedef void (*RotateFn)(void *, Vector3, const MethodInfo *);
typedef void (*DestroyFn)(void *, const MethodInfo *);
typedef void (*LoadSceneFn)(int, const MethodInfo *);
typedef float (*DeltaTimeFn)(const MethodInfo *);

CreatePrimitiveFn f_createPrimitive = nullptr;
GetTransformFn f_getTransform = nullptr;
GetGameObjectFn f_getGameObject = nullptr;
SetActiveFn f_setActive = nullptr;
SetNameFn f_setName = nullptr;
GetVectorFn f_getPosition = nullptr;
SetVectorFn f_setPosition = nullptr;
GetVectorFn f_getLocalScale = nullptr;
SetVectorFn f_setLocalScale = nullptr;
SetParentFn f_setParent = nullptr;
RotateFn f_rotate = nullptr;
DestroyFn f_destroy = nullptr;
LoadSceneFn f_loadScene = nullptr;
DeltaTimeFn f_deltaTime = nullptr;

MethodInfo *m_createPrimitive = nullptr;
MethodInfo *m_getTransform = nullptr;
MethodInfo *m_getGameObject = nullptr;
MethodInfo *m_setActive = nullptr;
MethodInfo *m_setName = nullptr;
MethodInfo *m_getPosition = nullptr;
MethodInfo *m_setPosition = nullptr;
MethodInfo *m_getLocalScale = nullptr;
MethodInfo *m_setLocalScale = nullptr;
MethodInfo *m_setParent = nullptr;
MethodInfo *m_rotate = nullptr;
MethodInfo *m_destroy = nullptr;
MethodInfo *m_loadScene = nullptr;
MethodInfo *m_deltaTime = nullptr;

bool g_ready = false;

template <typename T>
bool Bind(T &function, MethodInfo *&storage, Il2CppClass *klass, const char *name, int argCount) {
    storage = il2cpp::FindMethod(klass, name, argCount);
    if (storage == nullptr) {
        LOGE("unity method not found: %s", name);
        return false;
    }
    function = reinterpret_cast<T>(il2cpp::MethodPointer(storage));
    if (function == nullptr) {
        LOGE("unity method pointer null: %s", name);
        return false;
    }
    return true;
}

}

namespace unity {

bool Initialize() {
    if (g_ready) {
        return true;
    }
    Il2CppClass *gameObject = il2cpp::FindClass(kCoreModule, "UnityEngine", "GameObject");
    Il2CppClass *transform = il2cpp::FindClass(kCoreModule, "UnityEngine", "Transform");
    Il2CppClass *object = il2cpp::FindClass(kCoreModule, "UnityEngine", "Object");
    Il2CppClass *component = il2cpp::FindClass(kCoreModule, "UnityEngine", "Component");
    Il2CppClass *time = il2cpp::FindClass(kCoreModule, "UnityEngine", "Time");
    Il2CppClass *sceneManager = il2cpp::FindClass(kCoreModule, "UnityEngine.SceneManagement", "SceneManager");

    if (gameObject == nullptr || transform == nullptr || object == nullptr || time == nullptr) {
        return false;
    }

    bool ok = true;
    ok &= Bind(f_createPrimitive, m_createPrimitive, gameObject, "CreatePrimitive", 1);
    ok &= Bind(f_getTransform, m_getTransform, gameObject, "get_transform", 0);
    ok &= Bind(f_setActive, m_setActive, gameObject, "SetActive", 1);
    ok &= Bind(f_setName, m_setName, object, "set_name", 1);
    ok &= Bind(f_getPosition, m_getPosition, transform, "get_position", 0);
    ok &= Bind(f_setPosition, m_setPosition, transform, "set_position", 1);
    ok &= Bind(f_getLocalScale, m_getLocalScale, transform, "get_localScale", 0);
    ok &= Bind(f_setLocalScale, m_setLocalScale, transform, "set_localScale", 1);
    ok &= Bind(f_setParent, m_setParent, transform, "SetParent", 1);
    m_rotate = il2cpp::FindMethodByParamType(transform, "Rotate", "UnityEngine.Vector3");
    if (m_rotate != nullptr) {
        f_rotate = reinterpret_cast<RotateFn>(il2cpp::MethodPointer(m_rotate));
    } else {
        ok = false;
    }
    m_destroy = il2cpp::FindMethodByParamType(object, "Destroy", "UnityEngine.Object");
    if (m_destroy != nullptr) {
        f_destroy = reinterpret_cast<DestroyFn>(il2cpp::MethodPointer(m_destroy));
    } else {
        ok = false;
    }
    ok &= Bind(f_deltaTime, m_deltaTime, time, "get_deltaTime", 0);

    if (component != nullptr) {
        Bind(f_getGameObject, m_getGameObject, component, "get_gameObject", 0);
    }
    if (sceneManager != nullptr) {
        m_loadScene = il2cpp::FindMethodByParamType(sceneManager, "LoadScene", "System.Int32");
        if (m_loadScene != nullptr) {
            f_loadScene = reinterpret_cast<LoadSceneFn>(il2cpp::MethodPointer(m_loadScene));
        }
    }

    g_ready = ok;
    return ok;
}

void *CreatePrimitive(int primitiveType) {
    if (f_createPrimitive == nullptr) {
        return nullptr;
    }
    return f_createPrimitive(primitiveType, m_createPrimitive);
}

void *GetTransform(void *gameObject) {
    if (f_getTransform == nullptr || gameObject == nullptr) {
        return nullptr;
    }
    return f_getTransform(gameObject, m_getTransform);
}

void *GetGameObject(void *component) {
    if (f_getGameObject == nullptr || component == nullptr) {
        return nullptr;
    }
    return f_getGameObject(component, m_getGameObject);
}

void SetActive(void *gameObject, bool active) {
    if (f_setActive == nullptr || gameObject == nullptr) {
        return;
    }
    f_setActive(gameObject, active, m_setActive);
}

void SetName(void *gameObject, const char *name) {
    if (f_setName == nullptr || gameObject == nullptr) {
        return;
    }
    Il2CppString *managed = il2cpp::NewString(name);
    if (managed == nullptr) {
        return;
    }
    f_setName(gameObject, managed, m_setName);
}

Vector3 GetPosition(void *transform) {
    Vector3 result = {0.0f, 0.0f, 0.0f};
    if (f_getPosition == nullptr || transform == nullptr) {
        return result;
    }
    return f_getPosition(transform, m_getPosition);
}

void SetPosition(void *transform, Vector3 position) {
    if (f_setPosition == nullptr || transform == nullptr) {
        return;
    }
    f_setPosition(transform, position, m_setPosition);
}

Vector3 GetLocalScale(void *transform) {
    Vector3 result = {1.0f, 1.0f, 1.0f};
    if (f_getLocalScale == nullptr || transform == nullptr) {
        return result;
    }
    return f_getLocalScale(transform, m_getLocalScale);
}

void SetLocalScale(void *transform, Vector3 scale) {
    if (f_setLocalScale == nullptr || transform == nullptr) {
        return;
    }
    f_setLocalScale(transform, scale, m_setLocalScale);
}

void SetParent(void *transform, void *parent) {
    if (f_setParent == nullptr || transform == nullptr) {
        return;
    }
    f_setParent(transform, parent, m_setParent);
}

void Rotate(void *transform, Vector3 eulerAngles) {
    if (f_rotate == nullptr || transform == nullptr) {
        return;
    }
    f_rotate(transform, eulerAngles, m_rotate);
}

void Destroy(void *object) {
    if (f_destroy == nullptr || object == nullptr) {
        return;
    }
    f_destroy(object, m_destroy);
}

void LoadScene(int buildIndex) {
    if (f_loadScene == nullptr) {
        return;
    }
    f_loadScene(buildIndex, m_loadScene);
}

float DeltaTime() {
    if (f_deltaTime == nullptr) {
        return 0.016f;
    }
    return f_deltaTime(m_deltaTime);
}

float Distance(Vector3 a, Vector3 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

}
