#pragma once

#include "il2cpp.h"

struct NXConfig {
    bool enabled;
    bool safeMode;
    bool impossibleMode;
    float walkSpeed;
    float followSpeed;
    float walkAnimSpeed;
    float followAnimSpeed;
    float attackDistance;
    bool alwaysSeePlayer;
    bool instantHidingCheck;
    bool zeroSearchTimers;
    float hidingCheckTrigger;
    float howLongFollow;

    bool helicopterEnabled;
    bool helicopterRequiresKey;
    Vector3 helicopterPosition;
    Vector3 helicopterKeyPosition;
    float helicopterScale;
    float escapeRadius;
    float keyPickupRadius;
    float liftDuration;
    bool spawnRelativeToPlayer;
};

namespace config {

void Load();
const NXConfig &Get();

}
