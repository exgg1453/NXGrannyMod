# NXGrannyMod

Native content mod for Granny 1.8.12 (Unity 2022.3 / IL2CPP, arm64-v8a and armeabi-v7a).

Adds two features to the existing game:

1. **Impossible difficulty** - Granny always knows the player position, moves at extreme speed, checks hiding spots instantly and never idles.
2. **Helicopter escape** - a new escape route built at runtime from primitives, gated behind a new pickup, ending in the game's own EndScene.

## How it works

`libil2cpp.so` in this build exports the full IL2CPP runtime API and `global-metadata.dat` is not encrypted. Every class, field and method is therefore resolved by name at runtime, so no hardcoded RVAs are used and the mod survives minor game updates.

`EnemyAIGranny::FixedUpdate` is hooked with ShadowHook. All mod work runs inside that hook on the Unity main thread.

### Impossible difficulty

Fields written on `EnemyAIGranny` every frame:

| Field | Effect |
| --- | --- |
| `walkSpeed`, `grannysFollowSpeed`, `speed` | movement speed |
| `walkAnimSpeed`, `grannysAnimFollowSpeed` | animation playback speed |
| `attackDistance` | reach |
| `seePlayer`, `huntPlayer`, `grannyIsFollow`, `grannyHearPlayer` | permanent awareness |
| `waypointWaitTime`, `timerSearch`, `safeTimer`, `safeTimerStandStill`, `timerSee` | removes all idle and cooldown windows |
| `timerBed`, `grannyLookUnderBed` | instant hiding spot check when `playerHiding*` is set |

Field types are read from metadata at runtime, so bool, int and float fields are all written correctly.

### Helicopter escape

Six cube primitives form the airframe, two of them spin as rotors. A separate cube acts as the helicopter key. When the player is inside `keyPickupRadius` of the key it is collected; when the player is then inside `escapeRadius` of the helicopter, the rotors spin up, the airframe lifts for `liftDuration` seconds and `SceneManager.LoadScene(3)` loads EndScene.

## Configuration

Every value is read at startup from the first file found:

```
/sdcard/Android/data/com.dvloper.granny/files/nxgranny.json
/sdcard/NXGrannyMod/nxgranny.json
/storage/emulated/0/NXGrannyMod/nxgranny.json
```

`nxgranny.json` in this repository is the default. Coordinates are relative to the player spawn when `spawnRelativeToPlayer` is true, which is how the helicopter position should be tuned first.

## Building

```
./gradlew :app:assembleRelease
```

Output libraries land in `app/build/intermediates/merged_native_libs/release/*/out/lib/`.

## Patching the APK

```
python3 tools/patch_apk.py \
  --apk granny-1.8.12.apk \
  --lib-dir out \
  --output granny-1.8.12-nxmod.apk
```

The script decodes the APK, adds `com.nx.grannymod.NXLoader`, injects a call to it at the top of the launcher activity `onCreate`, copies the native libraries into every ABI directory, rebuilds, zipaligns and signs with v1, v2 and v3.

Requires `apktool`, `zipalign`, `apksigner` and `keytool` on PATH.

## Scene indices

| Index | Scene |
| --- | --- |
| 0 | SplashScreen |
| 1 | Menu |
| 2 | Scene |
| 3 | EndScene |

## Logcat

```
adb logcat -s NXGrannyMod
```
