#include "log.h"

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <pthread.h>

namespace {

pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

const char *kLogPaths[] = {
        "/sdcard/Android/data/com.dvloper.granny/files/nxgranny.log",
        "/sdcard/NXGrannyMod/nxgranny.log",
        "/storage/emulated/0/NXGrannyMod/nxgranny.log"
};

FILE *OpenLog() {
    for (const char *path : kLogPaths) {
        FILE *file = fopen(path, "a");
        if (file != nullptr) {
            return file;
        }
    }
    return nullptr;
}

}

void nx_log_write(const char *level, const char *format, ...) {
    pthread_mutex_lock(&g_mutex);
    FILE *file = OpenLog();
    if (file != nullptr) {
        time_t now = time(nullptr);
        struct tm parts = {};
        localtime_r(&now, &parts);
        fprintf(file, "%02d:%02d:%02d %s ", parts.tm_hour, parts.tm_min, parts.tm_sec, level);
        va_list args;
        va_start(args, format);
        vfprintf(file, format, args);
        va_end(args);
        fputc('\n', file);
        fclose(file);
    }
    pthread_mutex_unlock(&g_mutex);
}
