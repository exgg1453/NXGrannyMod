#pragma once

#include <android/log.h>

#define NX_TAG "NXGrannyMod"

void nx_log_write(const char *level, const char *format, ...);

#define LOGI(...)                                                    \
    do {                                                             \
        __android_log_print(ANDROID_LOG_INFO, NX_TAG, __VA_ARGS__);  \
        nx_log_write("I", __VA_ARGS__);                              \
    } while (0)

#define LOGE(...)                                                    \
    do {                                                             \
        __android_log_print(ANDROID_LOG_ERROR, NX_TAG, __VA_ARGS__); \
        nx_log_write("E", __VA_ARGS__);                              \
    } while (0)
