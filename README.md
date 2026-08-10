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
| `grannysFollowSpeed` | chase speed fed into `navComponent.speed` |
| `walkSpeed` | patrol speed |
| `grannysAnimFollowSpeed`, `walkAnimSpeed` | animation playback speed |
| `attackDistance` | reach used by the catch check |
| `seePlayer`, `grannyIsFollow`, `huntPlayer`, `grannyHearPlayer` | permanent awareness |
| `waypointWaitTime`, `safeTimer`, `safeTimerStandStill` | removes idle and cooldown windows |
| `timerBed` | raised to the trigger threshold while the player is hiding |

All of the above was verified against the shipped 1.8.12 binary rather than inferred, using the tools in `tools/`.

`EnemyAIGranny::Start` reads `PlayerPrefs.GetInt("DiffData")` and writes the speed pair accordingly:

| DiffData | `grannysFollowSpeed` | `grannysAnimFollowSpeed` | `howLongFollow` |
| --- | --- | --- | --- |
| 0 and 4 | 3.8 | 1.9 | 8.0 |
| 1 | 2.5 | 1.25 | 6.0 |
| 2 | 5.0 | 2.5 | 10.0 |
| 3 | 8.5 | 4.25 | 12.0 |

Animation speed is always exactly half of movement speed. `Start` also does `navComponent.speed = walkSpeed` followed by `walkAnimSpeed = navComponent.speed * 0.5`, so the mod derives both animation fields from their movement counterpart instead of exposing them separately.

The default `followSpeed` of 12.0 sits above the stock extreme value of 8.5 while staying inside the range the NavMeshAgent handles without overshooting corners.

`grannyLookUnderBed` is an output flag, not an input. The game gates most of its own logic behind `!grannyLookUnderBed`, so the mod never writes it. Instead `timerBed` is raised past the threshold the game checks and the game's own code plays the `lookBed` animation immediately. Disassembly of `FixedUpdate` confirms the threshold is still `3.0f` in 1.8.12, in four separate branches covering `playerHidingUnderBed`, `playerHidingInCoffin`, `playerHidingInCoffinBackyard` and `playerHidingInCar`.

Field types are read from metadata at runtime, so bool, int and float fields are all written correctly.

### Helicopter escape

Six cube primitives form the airframe, two of them spin as rotors. A separate cube acts as the helicopter key. When the player is inside `keyPickupRadius` of the key it is collected; when the player is then inside `escapeRadius` of the helicopter, the rotors spin up, the airframe lifts for `liftDuration` seconds and `SceneManager.LoadScene(3)` loads EndScene.

## Analysis tools

`libil2cpp.so` here is stripped, so `tools/` contains a self-contained pipeline that needs no .NET runtime:

| Tool | Purpose |
| --- | --- |
| `meta.py` | parses `global-metadata.dat` into a class, field and method listing |
| `coderegs.py` | locates the Assembly-CSharp `Il2CppCodeGenModule` and resolves method addresses |
| `addrmap.py` | resolves method addresses across all 70 modules so call targets can be named |
| `disasm.py` | disassembles a named method and extracts float constants |

Field offsets come from `Il2CppMetadataRegistration.fieldOffsets`, found by matching the two counts that equal `typeDefinitionsCount`.

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
