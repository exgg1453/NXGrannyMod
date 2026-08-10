import struct
import json

SO = "x/lib/arm64-v8a/libil2cpp.so"
META = "x/assets/bin/Data/Managed/Metadata/global-metadata.dat"

so = open(SO, "rb").read()
meta = open(META, "rb").read()

e_shoff = struct.unpack_from("<Q", so, 0x28)[0]
e_shentsize = struct.unpack_from("<H", so, 0x3A)[0]
e_shnum = struct.unpack_from("<H", so, 0x3C)[0]
e_shstrndx = struct.unpack_from("<H", so, 0x3E)[0]

sections = []
for i in range(e_shnum):
    o = e_shoff + i * e_shentsize
    name, typ, flags, addr, off, size, link, info, align, entsize = struct.unpack_from("<IIQQQQIIQQ", so, o)
    sections.append({"name": name, "type": typ, "addr": addr, "off": off, "size": size})

strtab = sections[e_shstrndx]


def section_name(n):
    s = strtab["off"] + n
    e = so.index(b"\x00", s)
    return so[s:e].decode()


for s in sections:
    s["sname"] = section_name(s["name"])

by_name = {s["sname"]: s for s in sections}


def vaddr_to_off(vaddr):
    for s in sections:
        if s["type"] == 8:
            continue
        if s["addr"] != 0 and s["addr"] <= vaddr < s["addr"] + s["size"]:
            return s["off"] + (vaddr - s["addr"])
    return None


relocs = {}
for sec_name in (".rela.dyn", ".rela.plt"):
    sec = by_name.get(sec_name)
    if sec is None:
        continue
    count = sec["size"] // 24
    for i in range(count):
        o = sec["off"] + i * 24
        r_offset, r_info, r_addend = struct.unpack_from("<QQq", so, o)
        if (r_info & 0xFFFFFFFF) == 1027:
            relocs[r_offset] = r_addend

print("relative relocations:", len(relocs))


def read_ptr(vaddr):
    if vaddr in relocs:
        return relocs[vaddr]
    off = vaddr_to_off(vaddr)
    if off is None:
        return 0
    return struct.unpack_from("<Q", so, off)[0]


def read_u32(vaddr):
    off = vaddr_to_off(vaddr)
    if off is None:
        return 0
    return struct.unpack_from("<I", so, off)[0]


target = b"Assembly-CSharp.dll\x00"
str_positions = []
start = 0
while True:
    idx = so.find(target, start)
    if idx < 0:
        break
    str_positions.append(idx)
    start = idx + 1

str_vaddrs = []
for pos in str_positions:
    for s in sections:
        if s["type"] == 8 or s["addr"] == 0:
            continue
        if s["off"] <= pos < s["off"] + s["size"]:
            str_vaddrs.append(s["addr"] + (pos - s["off"]))
            break

print("Assembly-CSharp.dll string vaddrs:", [hex(v) for v in str_vaddrs])

candidates = []
for r_offset, addend in relocs.items():
    if addend in str_vaddrs:
        candidates.append(r_offset)

print("pointers to that string:", [hex(c) for c in candidates])

module = None
for c in candidates:
    count = read_u32(c + 8)
    pointers = read_ptr(c + 16)
    if 1000 < count < 200000 and pointers != 0:
        print("candidate module struct at 0x%x methodPointerCount=%d methodPointers=0x%x" % (c, count, pointers))
        module = (c, count, pointers)

if module is None:
    raise SystemExit("code gen module not found")

_, method_pointer_count, method_pointers_addr = module

magic, version = struct.unpack_from("<Ii", meta, 0)
string_offset, string_count = struct.unpack_from("<ii", meta, 0x18)
method_off, method_cnt = struct.unpack_from("<ii", meta, 0x30)
MSIZE = 36
method_total = method_cnt // MSIZE


def get_string(index):
    s = string_offset + index
    e = meta.index(b"\x00", s)
    return meta[s:e].decode("utf-8", "replace")


type_off, type_cnt = struct.unpack_from("<ii", meta, 0xA0)
TSIZE = 88
type_count = type_cnt // TSIZE

methods = []
for i in range(method_total):
    base = method_off + i * MSIZE
    name_index, decl, ret, ret_token, param_start, generic = struct.unpack_from("<iiiIii", meta, base)
    token = struct.unpack_from("<I", meta, base + 24)[0]
    methods.append({"name": get_string(name_index), "declaring": decl, "token": token, "index": i})

types = []
for i in range(type_count):
    base = type_off + i * TSIZE
    name_index, ns_index = struct.unpack_from("<ii", meta, base)
    method_start = struct.unpack_from("<i", meta, base + 36)[0]
    mcount = struct.unpack_from("<H", meta, base + 64)[0]
    types.append({"name": get_string(name_index), "method_start": method_start, "method_count": mcount})

print("metadata methods:", method_total, "types:", type_count)
print("sample tokens:", [hex(m["token"]) for m in methods[30260:30264]])

result = {}
for t in types:
    if t["method_start"] < 0:
        continue
    for j in range(t["method_start"], t["method_start"] + t["method_count"]):
        if not (0 <= j < method_total):
            continue
        m = methods[j]
        row = (m["token"] & 0x00FFFFFF) - 1
        if not (0 <= row < method_pointer_count):
            continue
        ptr = read_ptr(method_pointers_addr + row * 8)
        if ptr == 0:
            continue
        result.setdefault(t["name"], []).append((m["name"], ptr))

print("classes with resolved pointers:", len(result))
for name in ("EnemyAIGranny", "theEndCarEscape", "bedTrigger", "startTheCar"):
    if name in result:
        print("== " + name)
        for mn, p in result[name]:
            print("   %-28s 0x%x" % (mn, p))

json.dump({k: [[a, b] for a, b in v] for k, v in result.items()}, open("methods.json", "w"))
print("wrote methods.json")
