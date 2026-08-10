import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

LOADER_SMALI = """.class public Lcom/nx/grannymod/NXLoader;
.super Ljava/lang/Object;

.method static constructor <clinit>()V
    .locals 2

    :try_start_0
    const-string v0, "nxgranny"

    invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V
    :try_end_0
    .catch Ljava/lang/Throwable; {:try_start_0 .. :try_end_0} :catch_0

    goto :goto_0

    :catch_0
    move-exception v1

    :goto_0
    return-void
.end method

.method public constructor <init>()V
    .locals 0

    invoke-direct {p0}, Ljava/lang/Object;-><init>()V

    return-void
.end method

.method public static init()V
    .locals 0

    return-void
.end method
"""

INJECTED_CALL = "    invoke-static {}, Lcom/nx/grannymod/NXLoader;->init()V\n\n"


def run(command, cwd=None):
    print("+ " + " ".join(command))
    result = subprocess.run(command, cwd=cwd)
    if result.returncode != 0:
        raise SystemExit("command failed: " + " ".join(command))


def find_launcher_activity(manifest_path):
    with open(manifest_path, "r", encoding="utf-8") as handle:
        manifest = handle.read()
    activities = re.findall(r"<activity[^>]*android:name=\"([^\"]+)\"(.*?)</activity>", manifest, re.S)
    for name, body in activities:
        if "android.intent.action.MAIN" in body and "android.intent.category.LAUNCHER" in body:
            return name
    short = re.findall(r"<activity[^>]*android:name=\"([^\"]+)\"[^>]*/>", manifest)
    if short:
        return short[0]
    return None


def resolve_package(manifest_path):
    with open(manifest_path, "r", encoding="utf-8") as handle:
        manifest = handle.read()
    match = re.search(r"package=\"([^\"]+)\"", manifest)
    if match is None:
        raise SystemExit("package name not found in manifest")
    return match.group(1)


def smali_path_for(decoded_dir, class_name):
    relative = class_name.replace(".", "/") + ".smali"
    for entry in sorted(os.listdir(decoded_dir)):
        if not entry.startswith("smali"):
            continue
        candidate = os.path.join(decoded_dir, entry, relative)
        if os.path.exists(candidate):
            return candidate
    return None


def inject_loader_call(smali_file):
    with open(smali_file, "r", encoding="utf-8") as handle:
        content = handle.read()

    if "NXLoader" in content:
        print("loader call already present")
        return content

    oncreate = re.search(r"(\.method[^\n]*\bonCreate\(Landroid/os/Bundle;\)V\n(?:\s*\.locals \d+\n|\s*\.registers \d+\n)?)", content)
    if oncreate is None:
        raise SystemExit("onCreate not found in " + smali_file)

    insert_at = oncreate.end()
    patched = content[:insert_at] + "\n" + INJECTED_CALL + content[insert_at:]
    with open(smali_file, "w", encoding="utf-8") as handle:
        handle.write(patched)
    return patched


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--apk", required=True)
    parser.add_argument("--lib-dir", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--apktool", default="apktool")
    parser.add_argument("--keystore", default=None)
    parser.add_argument("--keystore-pass", default="android")
    parser.add_argument("--key-alias", default="nxkey")
    args = parser.parse_args()

    workdir = tempfile.mkdtemp(prefix="nxgranny_")
    decoded = os.path.join(workdir, "decoded")

    run([args.apktool, "d", "-f", "-o", decoded, args.apk])

    manifest_path = os.path.join(decoded, "AndroidManifest.xml")
    package = resolve_package(manifest_path)
    activity = find_launcher_activity(manifest_path)
    if activity is None:
        raise SystemExit("launcher activity not found")
    if activity.startswith("."):
        activity = package + activity
    print("package: " + package)
    print("launcher activity: " + activity)

    smali_root = None
    for entry in sorted(os.listdir(decoded)):
        if entry.startswith("smali"):
            smali_root = os.path.join(decoded, entry)
    if smali_root is None:
        raise SystemExit("no smali directory produced")

    loader_dir = os.path.join(smali_root, "com", "nx", "grannymod")
    os.makedirs(loader_dir, exist_ok=True)
    with open(os.path.join(loader_dir, "NXLoader.smali"), "w", encoding="utf-8") as handle:
        handle.write(LOADER_SMALI)

    activity_smali = smali_path_for(decoded, activity)
    if activity_smali is None:
        raise SystemExit("smali not found for " + activity)
    inject_loader_call(activity_smali)

    for abi in os.listdir(args.lib_dir):
        source = os.path.join(args.lib_dir, abi)
        if not os.path.isdir(source):
            continue
        target = os.path.join(decoded, "lib", abi)
        os.makedirs(target, exist_ok=True)
        for name in os.listdir(source):
            if not name.endswith(".so"):
                continue
            shutil.copy2(os.path.join(source, name), os.path.join(target, name))
            print("added lib/%s/%s" % (abi, name))

    unsigned = os.path.join(workdir, "unsigned.apk")
    run([args.apktool, "b", "-f", "-o", unsigned, decoded])

    aligned = os.path.join(workdir, "aligned.apk")
    run(["zipalign", "-p", "-f", "4", unsigned, aligned])

    keystore = args.keystore
    if keystore is None:
        keystore = os.path.join(workdir, "nx.keystore")
        run([
            "keytool", "-genkeypair", "-v",
            "-keystore", keystore,
            "-alias", args.key_alias,
            "-keyalg", "RSA",
            "-keysize", "2048",
            "-validity", "10000",
            "-storepass", args.keystore_pass,
            "-keypass", args.keystore_pass,
            "-dname", "CN=NX Team, O=Novatex, C=TR"
        ])

    run([
        "apksigner", "sign",
        "--ks", keystore,
        "--ks-pass", "pass:" + args.keystore_pass,
        "--key-pass", "pass:" + args.keystore_pass,
        "--ks-key-alias", args.key_alias,
        "--v1-signing-enabled", "true",
        "--v2-signing-enabled", "true",
        "--v3-signing-enabled", "true",
        "--out", args.output,
        aligned
    ])

    run(["apksigner", "verify", "--print-certs", args.output])
    print("done: " + args.output)


if __name__ == "__main__":
    main()
