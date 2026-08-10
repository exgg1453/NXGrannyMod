#include "mod.h"
#include "il2cpp.h"
#include "unity.h"
#include "config.h"
#include "log.h"

#include <shadowhook.h>
#include <cstring>

namespace {

const char *kAssemblyCSharp = "Assembly-CSharp.dll";

Il2CppClass *g_grannyClass = nullptr;

typedef void (*FixedUpdateFn)(void *, const MethodInfo *);

FixedUpdateFn g_originalFixedUpdate = nullptr;
void *g_hookStub = nullptr;
MethodInfo *g_fixedUpdateMethod = nullptr;

enum HelicopterState {
    kHelicopterNotSpawned = 0,
    kHelicopterIdle = 1,
    kHelicopterLifting = 2,
    kHelicopterDone = 3
};

struct HelicopterPart {
    void *transform;
    Vector3 offset;
    bool isRotor;
};

struct ModState {
    void *lastGranny;
    int helicopterState;
    HelicopterPart parts[8];
    int partCount;
    void *keyTransform;
    bool keyCollected;
    Vector3 basePosition;
    Vector3 keyPosition;
    float liftTimer;
    bool anchorResolved;
};

ModState g_state = {};

void ResetState() {
    memset(&g_state, 0, sizeof(g_state));
    g_state.helicopterState = kHelicopterNotSpawned;
}

void *ResolvePlayerTransform(void *granny) {
    void *player = field::GetPointerByName(granny, g_grannyClass, "playerPos");
    if (player == nullptr) {
        player = field::GetPointerByName(granny, g_grannyClass, "player");
    }
    if (player == nullptr) {
        return nullptr;
    }
    const char *className = il2cpp::ObjectClassName(player);
    if (className == nullptr) {
        return nullptr;
    }
    if (strcmp(className, "Transform") == 0) {
        return player;
    }
    if (strcmp(className, "GameObject") == 0) {
        return unity::GetTransform(player);
    }
    void *gameObject = unity::GetGameObject(player);
    if (gameObject != nullptr) {
        return unity::GetTransform(gameObject);
    }
    return nullptr;
}

void ApplyImpossibleMode(void *granny) {
    const NXConfig &cfg = config::Get();
    if (!cfg.impossibleMode) {
        return;
    }

    field::SetByName(granny, g_grannyClass, "grannysFollowSpeed", cfg.followSpeed);
    field::SetByName(granny, g_grannyClass, "grannysAnimFollowSpeed", cfg.followSpeed * 0.5f);
    field::SetByName(granny, g_grannyClass, "walkSpeed", cfg.walkSpeed);
    field::SetByName(granny, g_grannyClass, "walkAnimSpeed", cfg.walkSpeed * 0.5f);
    field::SetByName(granny, g_grannyClass, "howLongFollow", cfg.howLongFollow);
    field::SetByName(granny, g_grannyClass, "attackDistance", cfg.attackDistance);
    field::SetBoolByName(granny, g_grannyClass, "dontHitPlayer", false);

    if (cfg.alwaysSeePlayer) {
        field::SetBoolByName(granny, g_grannyClass, "seePlayer", true);
        field::SetBoolByName(granny, g_grannyClass, "grannyIsFollow", true);
        field::SetBoolByName(granny, g_grannyClass, "huntPlayer", true);
        field::SetBoolByName(granny, g_grannyClass, "grannyHearPlayer", true);
    }

    if (cfg.zeroSearchTimers) {
        field::SetByName(granny, g_grannyClass, "waypointWaitTime", 0.0f);
        field::SetByName(granny, g_grannyClass, "safeTimer", 0.0f);
        field::SetByName(granny, g_grannyClass, "safeTimerStandStill", 0.0f);
    }

    if (cfg.instantHidingCheck) {
        bool hiding = false;
        float value = 0.0f;
        static const char *kHidingFields[] = {
                "playerHidingUnderBed",
                "playerHidingInCoffin",
                "playerHidingInCoffinBackyard",
                "playerHidingInCar",
                "playerHiding"
        };
        for (const char *name : kHidingFields) {
            if (field::GetFloatByName(granny, g_grannyClass, name, &value) && value != 0.0f) {
                hiding = true;
                break;
            }
        }
        if (hiding) {
            field::SetByName(granny, g_grannyClass, "timerBed", cfg.hidingCheckTrigger);
            field::SetBoolByName(granny, g_grannyClass, "grannyIsFollow", true);
        }
    }
}

void *SpawnPart(Vector3 localScale, const char *name) {
    void *gameObject = unity::CreatePrimitive(kPrimitiveCube);
    if (gameObject == nullptr) {
        return nullptr;
    }
    unity::SetName(gameObject, name);
    unity::RemoveCollider(gameObject);
    void *transform = unity::GetTransform(gameObject);
    if (transform == nullptr) {
        return nullptr;
    }
    unity::SetLocalScale(transform, localScale);
    return transform;
}

void AddPart(void *transform, Vector3 offset, bool isRotor) {
    if (transform == nullptr || g_state.partCount >= 8) {
        return;
    }
    g_state.parts[g_state.partCount].transform = transform;
    g_state.parts[g_state.partCount].offset = offset;
    g_state.parts[g_state.partCount].isRotor = isRotor;
    ++g_state.partCount;
}

void SpawnHelicopter(Vector3 anchor) {
    const NXConfig &cfg = config::Get();
    float s = cfg.helicopterScale;

    g_state.basePosition = anchor;
    g_state.partCount = 0;

    Vector3 bodyScale = {2.0f * s, 1.4f * s, 4.5f * s};
    Vector3 tailScale = {0.4f * s, 0.4f * s, 3.5f * s};
    Vector3 rotorScale = {9.0f * s, 0.15f * s, 0.6f * s};
    Vector3 tailRotorScale = {0.2f * s, 2.0f * s, 0.6f * s};
    Vector3 skidScale = {0.2f * s, 0.2f * s, 3.5f * s};

    Vector3 bodyOffset = {0.0f, 0.0f, 0.0f};
    Vector3 tailOffset = {0.0f, 0.3f * s, -3.8f * s};
    Vector3 rotorOffset = {0.0f, 1.1f * s, 0.0f};
    Vector3 tailRotorOffset = {0.3f * s, 0.9f * s, -5.3f * s};
    Vector3 skidLeftOffset = {-0.9f * s, -1.0f * s, 0.0f};
    Vector3 skidRightOffset = {0.9f * s, -1.0f * s, 0.0f};

    AddPart(SpawnPart(bodyScale, "NX_Helicopter_Body"), bodyOffset, false);
    AddPart(SpawnPart(tailScale, "NX_Helicopter_Tail"), tailOffset, false);
    AddPart(SpawnPart(rotorScale, "NX_Helicopter_Rotor"), rotorOffset, true);
    AddPart(SpawnPart(tailRotorScale, "NX_Helicopter_TailRotor"), tailRotorOffset, true);
    AddPart(SpawnPart(skidScale, "NX_Helicopter_SkidLeft"), skidLeftOffset, false);
    AddPart(SpawnPart(skidScale, "NX_Helicopter_SkidRight"), skidRightOffset, false);

    if (cfg.helicopterRequiresKey) {
        Vector3 keyScale = {0.25f, 0.25f, 0.6f};
        g_state.keyTransform = SpawnPart(keyScale, "NX_Helicopter_Key");
        if (g_state.keyTransform != nullptr) {
            unity::SetPosition(g_state.keyTransform, g_state.keyPosition);
        }
    } else {
        g_state.keyCollected = true;
    }

    LOGI("helicopter spawned at %.2f %.2f %.2f with %d parts", anchor.x, anchor.y, anchor.z, g_state.partCount);
}

void UpdateHelicopterTransforms() {
    for (int i = 0; i < g_state.partCount; ++i) {
        HelicopterPart &part = g_state.parts[i];
        if (part.transform == nullptr) {
            continue;
        }
        Vector3 position = {
                g_state.basePosition.x + part.offset.x,
                g_state.basePosition.y + part.offset.y,
                g_state.basePosition.z + part.offset.z
        };
        unity::SetPosition(part.transform, position);
    }
}

void SpinRotors(float speed) {
    float delta = unity::DeltaTime();
    for (int i = 0; i < g_state.partCount; ++i) {
        HelicopterPart &part = g_state.parts[i];
        if (!part.isRotor || part.transform == nullptr) {
            continue;
        }
        Vector3 rotation = {0.0f, speed * delta, 0.0f};
        unity::Rotate(part.transform, rotation);
    }
}

void UpdateHelicopter(void *granny) {
    const NXConfig &cfg = config::Get();
    if (!cfg.helicopterEnabled) {
        return;
    }

    void *playerTransform = ResolvePlayerTransform(granny);
    if (playerTransform == nullptr) {
        return;
    }
    Vector3 playerPosition = unity::GetPosition(playerTransform);

    if (g_state.helicopterState == kHelicopterNotSpawned) {
        Vector3 anchor = cfg.helicopterPosition;
        Vector3 keyAnchor = cfg.helicopterKeyPosition;
        if (cfg.spawnRelativeToPlayer) {
            anchor.x += playerPosition.x;
            anchor.y += playerPosition.y;
            anchor.z += playerPosition.z;
            keyAnchor.x += playerPosition.x;
            keyAnchor.y += playerPosition.y;
            keyAnchor.z += playerPosition.z;
        }
        g_state.keyPosition = keyAnchor;
        SpawnHelicopter(anchor);
        UpdateHelicopterTransforms();
        g_state.helicopterState = kHelicopterIdle;
        return;
    }

    if (g_state.helicopterState == kHelicopterIdle) {
        SpinRotors(120.0f);
        if (!g_state.keyCollected && g_state.keyTransform != nullptr) {
            Vector3 keyRotation = {0.0f, 90.0f * unity::DeltaTime(), 0.0f};
            unity::Rotate(g_state.keyTransform, keyRotation);
            if (unity::Distance(playerPosition, g_state.keyPosition) <= cfg.keyPickupRadius) {
                g_state.keyCollected = true;
                void *keyObject = unity::GetGameObject(g_state.keyTransform);
                if (keyObject != nullptr) {
                    unity::Destroy(keyObject);
                }
                g_state.keyTransform = nullptr;
                LOGI("helicopter key collected");
            }
        }
        if (g_state.keyCollected &&
            unity::Distance(playerPosition, g_state.basePosition) <= cfg.escapeRadius) {
            g_state.helicopterState = kHelicopterLifting;
            g_state.liftTimer = 0.0f;
            LOGI("helicopter escape triggered");
        }
        return;
    }

    if (g_state.helicopterState == kHelicopterLifting) {
        SpinRotors(1400.0f);
        float delta = unity::DeltaTime();
        g_state.liftTimer += delta;
        g_state.basePosition.y += 4.0f * delta;
        UpdateHelicopterTransforms();
        if (g_state.liftTimer >= cfg.liftDuration) {
            g_state.helicopterState = kHelicopterDone;
            unity::LoadScene(3);
            LOGI("loading end scene");
        }
    }
}

void HookedFixedUpdate(void *thiz, const MethodInfo *method) {
    if (g_originalFixedUpdate != nullptr) {
        g_originalFixedUpdate(thiz, method);
    }
    if (thiz == nullptr) {
        return;
    }
    if (g_state.lastGranny != thiz) {
        ResetState();
        g_state.lastGranny = thiz;
        LOGI("granny instance changed, mod state reset");
    }
    if (config::Get().safeMode) {
        return;
    }
    ApplyImpossibleMode(thiz);
    UpdateHelicopter(thiz);
}

}

namespace mod {

bool Install() {
    g_grannyClass = il2cpp::FindClass(kAssemblyCSharp, "", "EnemyAIGranny");
    if (g_grannyClass == nullptr) {
        LOGE("EnemyAIGranny class not found");
        return false;
    }

    g_fixedUpdateMethod = il2cpp::FindMethod(g_grannyClass, "FixedUpdate", 0);
    if (g_fixedUpdateMethod == nullptr) {
        LOGE("EnemyAIGranny::FixedUpdate not found");
        return false;
    }

    void *target = il2cpp::MethodPointer(g_fixedUpdateMethod);
    if (target == nullptr) {
        LOGE("FixedUpdate method pointer null");
        return false;
    }

    ResetState();

    g_hookStub = shadowhook_hook_func_addr(
            target,
            reinterpret_cast<void *>(HookedFixedUpdate),
            reinterpret_cast<void **>(&g_originalFixedUpdate));

    if (g_hookStub == nullptr) {
        LOGE("shadowhook failed: %s", shadowhook_to_errmsg(shadowhook_get_errno()));
        return false;
    }

    LOGI("hook installed at %p", target);
    return true;
}

}
