#include "config.h"
#include "log.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>

namespace {

NXConfig g_config = {
        true,
        false,
        true,
        3.2f,
        10.0f,
        2.4f,
        4.2f,
        3.0f,
        true,
        true,
        true,
        3.5f,
        20.0f,

        true,
        true,
        {0.0f, 0.0f, 30.0f},
        {0.0f, 0.0f, 12.0f},
        1.0f,
        3.0f,
        1.8f,
        4.0f,
        true,
        2.0f
};

const char *kConfigPaths[] = {
        "/sdcard/Android/data/com.dvloper.granny/files/nxgranny.json",
        "/sdcard/NXGrannyMod/nxgranny.json",
        "/storage/emulated/0/NXGrannyMod/nxgranny.json"
};

std::string ReadFile(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == nullptr) {
        return std::string();
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size <= 0 || size > 262144) {
        fclose(file);
        return std::string();
    }
    std::string buffer;
    buffer.resize(static_cast<size_t>(size));
    size_t read = fread(&buffer[0], 1, static_cast<size_t>(size), file);
    fclose(file);
    buffer.resize(read);
    return buffer;
}

bool FindNumber(const std::string &text, const char *key, float *out) {
    std::string needle = std::string("\"") + key + "\"";
    size_t pos = text.find(needle);
    if (pos == std::string::npos) {
        return false;
    }
    pos = text.find(':', pos + needle.size());
    if (pos == std::string::npos) {
        return false;
    }
    *out = strtof(text.c_str() + pos + 1, nullptr);
    return true;
}

bool FindBool(const std::string &text, const char *key, bool *out) {
    std::string needle = std::string("\"") + key + "\"";
    size_t pos = text.find(needle);
    if (pos == std::string::npos) {
        return false;
    }
    pos = text.find(':', pos + needle.size());
    if (pos == std::string::npos) {
        return false;
    }
    size_t cursor = pos + 1;
    while (cursor < text.size() && (text[cursor] == ' ' || text[cursor] == '\t')) {
        ++cursor;
    }
    if (text.compare(cursor, 4, "true") == 0) {
        *out = true;
        return true;
    }
    if (text.compare(cursor, 5, "false") == 0) {
        *out = false;
        return true;
    }
    return false;
}

void FindVector(const std::string &text, const char *prefix, Vector3 *out) {
    std::string keyX = std::string(prefix) + "X";
    std::string keyY = std::string(prefix) + "Y";
    std::string keyZ = std::string(prefix) + "Z";
    FindNumber(text, keyX.c_str(), &out->x);
    FindNumber(text, keyY.c_str(), &out->y);
    FindNumber(text, keyZ.c_str(), &out->z);
}

}

namespace config {

void Load() {
    std::string text;
    for (const char *path : kConfigPaths) {
        text = ReadFile(path);
        if (!text.empty()) {
            LOGI("config loaded from %s", path);
            break;
        }
    }
    if (text.empty()) {
        LOGI("config file not found at any known path, using built-in defaults");
        LOGI("expected: /sdcard/Android/data/com.dvloper.granny/files/nxgranny.json");
        return;
    }
    FindBool(text, "enabled", &g_config.enabled);
    FindBool(text, "safeMode", &g_config.safeMode);
    FindBool(text, "impossibleMode", &g_config.impossibleMode);
    FindNumber(text, "walkSpeed", &g_config.walkSpeed);
    FindNumber(text, "followSpeed", &g_config.followSpeed);
    FindNumber(text, "walkAnimSpeed", &g_config.walkAnimSpeed);
    FindNumber(text, "followAnimSpeed", &g_config.followAnimSpeed);
    FindNumber(text, "attackDistance", &g_config.attackDistance);
    FindBool(text, "alwaysSeePlayer", &g_config.alwaysSeePlayer);
    FindBool(text, "instantHidingCheck", &g_config.instantHidingCheck);
    FindBool(text, "zeroSearchTimers", &g_config.zeroSearchTimers);
    FindNumber(text, "hidingCheckTrigger", &g_config.hidingCheckTrigger);
    FindNumber(text, "howLongFollow", &g_config.howLongFollow);

    FindBool(text, "helicopterEnabled", &g_config.helicopterEnabled);
    FindBool(text, "helicopterRequiresKey", &g_config.helicopterRequiresKey);
    FindVector(text, "helicopterPosition", &g_config.helicopterPosition);
    FindVector(text, "helicopterKeyPosition", &g_config.helicopterKeyPosition);
    FindNumber(text, "helicopterScale", &g_config.helicopterScale);
    FindNumber(text, "escapeRadius", &g_config.escapeRadius);
    FindNumber(text, "keyPickupRadius", &g_config.keyPickupRadius);
    FindNumber(text, "liftDuration", &g_config.liftDuration);
    FindBool(text, "spawnRelativeToPlayer", &g_config.spawnRelativeToPlayer);
    FindNumber(text, "positionLogInterval", &g_config.positionLogInterval);
}

const NXConfig &Get() {
    return g_config;
}

}
