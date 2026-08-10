import struct
import json

SO = "x/lib/arm64-v8a/libil2cpp.so"
META = "x/assets/bin/Data/Managed/Metadata/global-metadata.dat"

so = open(SO, "rb").read()
meta = open(META, "rb").read()

e_shoff = struct.unpack_from("<Q", so, 0x28)[0]
ess = struct.unpack_from("<H", so, 0x3A)[0]
shnum = struct.unpack_from("<H", so, 0x3C)[0]
secs = []
for i in range(shnum):
    o = e_shoff + i * ess
    _, typ, _, addr, off, size, _, _, _, _ = struct.unpack_from("<IIQQQQIIQQ", so, o)
    secs.append((typ, addr, off, size))


def v2o(v):
    for t, a, o, s in secs:
        if t == 8 or a == 0:
            continue
        if a <= v < a + s:
            return o + (v - a)
    return None


def o2v(fileoff):
    for t, a, o, s in secs:
        if t == 8 or a == 0:
            continue
        if o <= fileoff < o + s:
            return a + (fileoff - o)
    return None


relocs = {}
for t, a, o, s in secs:
    if t == 4:
        for i in range(s // 24):
            ro, ri, ra = struct.unpack_from("<QQq", so, o + i * 24)
            if (ri & 0xFFFFFFFF) == 1027:
                relocs[ro] = ra


def rptr(v):
    if v in relocs:
        return relocs[v]
    off = v2o(v)
    return struct.unpack_from("<Q", so, off)[0] if off is not None else 0


def ru32(v):
    off = v2o(v)
    return struct.unpack_from("<I", so, off)[0] if off is not None else 0


string_offset, string_count = struct.unpack_from("<ii", meta, 0x18)


def gs(i):
    s = string_offset + i
    e = meta.index(b"\x00", s)
    return meta[s:e].decode("utf-8", "replace")


type_off, type_cnt = struct.unpack_from("<ii", meta, 0xA0)
TSIZE = 88
type_count = type_cnt // TSIZE

method_off, method_cnt = struct.unpack_from("<ii", meta, 0x30)
MSIZE = 36
method_total = method_cnt // MSIZE

image_off, image_cnt = struct.unpack_from("<ii", meta, 0xA8)
ISIZE = 40
image_count = image_cnt // ISIZE
print("images:", image_count)

methods = []
for i in range(method_total):
    base = method_off + i * MSIZE
    name_index = struct.unpack_from("<i", meta, base)[0]
    token = struct.unpack_from("<I", meta, base + 24)[0]
    methods.append((gs(name_index), token))

types = []
for i in range(type_count):
    base = type_off + i * TSIZE
    name_index, ns_index = struct.unpack_from("<ii", meta, base)
    mstart = struct.unpack_from("<i", meta, base + 36)[0]
    mcount = struct.unpack_from("<H", meta, base + 64)[0]
    types.append((gs(name_index), gs(ns_index), mstart, mcount))

module_structs = {}
dll_positions = {}
start = 0
while True:
    idx = so.find(b".dll\x00", start)
    if idx < 0:
        break
    begin = idx
    while begin > 0 and 32 <= so[begin - 1] < 127:
        begin -= 1
    name = so[begin:idx + 4].decode("utf-8", "replace")
    v = o2v(begin)
    if v is not None and name.endswith(".dll"):
        dll_positions[v] = name
    start = idx + 1

name_to_module = {}
for r_offset, addend in relocs.items():
    if addend in dll_positions:
        count = ru32(r_offset + 8)
        pointers = rptr(r_offset + 16)
        if 0 < count < 500000 and pointers != 0:
            name_to_module[dll_positions[addend]] = (count, pointers)

print("code gen modules found:", len(name_to_module))

addr_to_name = {}
for i in range(image_count):
    base = image_off + i * ISIZE
    name_index, assembly_index, type_start, type_count_img = struct.unpack_from("<iiiI", meta, base)
    image_name = gs(name_index)
    if image_name not in name_to_module:
        continue
    mp_count, mp_addr = name_to_module[image_name]
    for t in range(type_start, type_start + type_count_img):
        if not (0 <= t < type_count):
            continue
        tname, tns, mstart, mcount = types[t]
        if mstart < 0:
            continue
        for j in range(mstart, mstart + mcount):
            if not (0 <= j < method_total):
                continue
            mname, token = methods[j]
            row = (token & 0x00FFFFFF) - 1
            if not (0 <= row < mp_count):
                continue
            ptr = rptr(mp_addr + row * 8)
            if ptr == 0:
                continue
            full = (tns + "." if tns else "") + tname + "::" + mname
            addr_to_name.setdefault(ptr, full)

print("resolved method addresses:", len(addr_to_name))
json.dump({hex(k): v for k, v in addr_to_name.items()}, open("addrmap.json", "w"))
print("wrote addrmap.json")

for probe in (0x2a47994, 0x2a478e0, 0x2a947a4, 0x2a9deb4, 0x2a9ec6c, 0x1336890):
    print("  0x%x -> %s" % (probe, addr_to_name.get(probe, "UNKNOWN")))
