#pragma once

#include <android/log.h>

#define NX_TAG "NXGrannyMod"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, NX_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, NX_TAG, __VA_ARGS__)
