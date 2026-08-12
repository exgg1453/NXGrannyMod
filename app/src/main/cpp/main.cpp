#include "il2cpp.h"
#include "unity.h"
#include "config.h"
#include "mod.h"
#include "log.h"

#include <jni.h>
#include <pthread.h>
#include <unistd.h>
#include <shadowhook.h>

namespace {

const char *kAssemblyCSharp = "Assembly-CSharp.dll";

bool WaitForRuntime() {
    for (int attempt = 0; attempt < 1200; ++attempt) {
        if (il2cpp::DomainGet() != nullptr) {
            LOGI("il2cpp domain available after %d ticks", attempt);
            return true;
        }
        usleep(100000);
    }
    LOGE("il2cpp domain never became available");
    return false;
}

bool WaitForGameAssembly() {
    for (int attempt = 0; attempt < 1200; ++attempt) {
        if (il2cpp::FindImage(kAssemblyCSharp) != nullptr) {
            LOGI("game assembly available after %d ticks", attempt);
            return true;
        }
        usleep(100000);
    }
    LOGE("game assembly never became available");
    return false;
}

void *InitializeThread(void *argument) {
    (void) argument;

    sleep(3);

    config::Load();
    if (!config::Get().enabled) {
        LOGI("mod disabled by config");
        return nullptr;
    }

    if (!il2cpp::Initialize()) {
        return nullptr;
    }

    if (!WaitForRuntime()) {
        return nullptr;
    }

    sleep(3);

    if (!WaitForGameAssembly()) {
        return nullptr;
    }

    bool unityReady = false;
    for (int attempt = 0; attempt < 60; ++attempt) {
        if (unity::Initialize()) {
            LOGI("unity bindings ready");
            unityReady = true;
            break;
        }
        if (attempt == 0) {
            LOGE("unity bindings incomplete, retrying quietly");
        }
        usleep(500000);
    }
    if (!unityReady) {
        LOGE("continuing with partial unity bindings");
    }

    for (int attempt = 0;; ++attempt) {
        if (mod::Install()) {
            LOGI("NXGrannyMod ready");
            return nullptr;
        }
        if (attempt % 25 == 0) {
            LOGI("waiting for game scene, attempt %d", attempt);
        }
        usleep(400000);
    }
}

}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    (void) vm;
    (void) reserved;

    LOGI("NXGrannyMod loading");

    if (shadowhook_init(SHADOWHOOK_MODE_UNIQUE, false) != 0) {
        LOGE("shadowhook_init failed: %s", shadowhook_to_errmsg(shadowhook_get_errno()));
    }

    pthread_t thread;
    pthread_create(&thread, nullptr, InitializeThread, nullptr);
    pthread_detach(thread);

    return JNI_VERSION_1_6;
}
