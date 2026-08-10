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

void *InitializeThread(void *argument) {
    (void) argument;

    config::Load();

    if (!il2cpp::Initialize()) {
        LOGE("il2cpp initialize failed");
        return nullptr;
    }

    Il2CppDomain *domain = nullptr;
    for (int attempt = 0; attempt < 600; ++attempt) {
        domain = il2cpp::DomainGet();
        if (domain != nullptr) {
            break;
        }
        usleep(100000);
    }
    if (domain == nullptr) {
        LOGE("il2cpp domain never became available");
        return nullptr;
    }

    il2cpp::ThreadAttach(domain);

    for (int attempt = 0; attempt < 600; ++attempt) {
        if (unity::Initialize()) {
            break;
        }
        usleep(100000);
    }

    for (int attempt = 0; attempt < 600; ++attempt) {
        if (mod::Install()) {
            LOGI("NXGrannyMod ready");
            return nullptr;
        }
        usleep(100000);
    }

    LOGE("mod install failed");
    return nullptr;
}

}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    (void) vm;
    (void) reserved;

    shadowhook_init(SHADOWHOOK_MODE_UNIQUE, false);

    pthread_t thread;
    pthread_create(&thread, nullptr, InitializeThread, nullptr);
    pthread_detach(thread);

    return JNI_VERSION_1_6;
}
