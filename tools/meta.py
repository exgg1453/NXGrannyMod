import struct
import sys

PATH = "x/assets/bin/Data/Managed/Metadata/global-metadata.dat"

with open(PATH, "rb") as f:
    data = f.read()

magic, version = struct.unpack_from("<Ii", data, 0)
assert magic == 0xFAB11BAF
size = len(data)

string_offset, string_count = struct.unpack_from("<ii", data, 0x18)


def get_string(index):
    if index < 0:
        return ""
    start = string_offset + index
    end = data.index(b"\x00", start)
    return data[start:end].decode("utf-8", "replace")


header_ints = struct.unpack_from("<%di" % 64, data, 0)

TYPE_SIZE = 88


def score_type_table(offset, count):
    if offset <= 0 or count <= 0:
        return -1
    if count % TYPE_SIZE != 0:
        return -1
    if offset + count > size:
        return -1
    n = count // TYPE_SIZE
    if n < 100 or n > 200000:
        return -1
    good = 0
    for i in range(0, min(n, 300)):
        base = offset + i * TYPE_SIZE
        name_index, ns_index = struct.unpack_from("<ii", data, base)
        if 0 <= name_index < string_count and 0 <= ns_index < string_count:
            name = get_string(name_index)
            if name and all(32 <= ord(c) < 127 for c in name):
                good += 1
    return good


best = None
for i in range(2, 62):
    off = header_ints[i]
    cnt = header_ints[i + 1]
    s = score_type_table(off, cnt)
    if s > 0 and (best is None or s > best[0]):
        best = (s, off, cnt, i)

if best is None:
    print("type table not found")
    sys.exit(1)

_, type_off, type_cnt, type_idx = best
type_count = type_cnt // TYPE_SIZE
print("typeDefinitions header index=%d offset=%d count=%d types=%d" % (type_idx, type_off, type_cnt, type_count))

method_off, method_cnt = struct.unpack_from("<ii", data, 0x30)


def try_method_size(msize):
    if method_cnt % msize != 0:
        return -1
    n = method_cnt // msize
    good = 0
    for i in range(0, min(n, 300)):
        base = method_off + i * msize
        name_index, decl = struct.unpack_from("<ii", data, base)
        if 0 <= name_index < string_count and 0 <= decl < type_count:
            name = get_string(name_index)
            if name and all(32 <= ord(c) < 127 for c in name):
                good += 1
    return good


msize = None
best_m = -1
for candidate in (32, 28, 36):
    s = try_method_size(candidate)
    if s > best_m:
        best_m = s
        msize = candidate

method_count = method_cnt // msize
print("methods offset=%d count=%d size=%d methods=%d" % (method_off, method_cnt, msize, method_count))

methods = []
for i in range(method_count):
    base = method_off + i * msize
    name_index, decl, ret = struct.unpack_from("<iii", data, base)
    if msize == 32:
        token = struct.unpack_from("<I", data, base + 24)[0]
    else:
        token = struct.unpack_from("<I", data, base + 20)[0]
    methods.append((get_string(name_index), decl, token))

types = []
for i in range(type_count):
    base = type_off + i * TYPE_SIZE
    name_index, ns_index = struct.unpack_from("<ii", data, base)
    method_start = struct.unpack_from("<i", data, base + 36)[0]
    method_count_t = struct.unpack_from("<H", data, base + 64)[0]
    field_start = struct.unpack_from("<i", data, base + 32)[0]
    field_count_t = struct.unpack_from("<H", data, base + 68)[0]
    types.append({
        "name": get_string(name_index),
        "ns": get_string(ns_index),
        "method_start": method_start,
        "method_count": method_count_t,
        "field_start": field_start,
        "field_count": field_count_t,
        "index": i,
    })

field_off, field_cnt = struct.unpack_from("<ii", data, 0x60)
FIELD_SIZE = 12
field_total = field_cnt // FIELD_SIZE
fields = []
for i in range(field_total):
    base = field_off + i * FIELD_SIZE
    name_index, type_index, token = struct.unpack_from("<iiI", data, base)
    fields.append(get_string(name_index))

with open("classes.txt", "w") as out:
    for t in types:
        full = (t["ns"] + "." + t["name"]) if t["ns"] else t["name"]
        out.write("=== %s (typeIndex=%d)\n" % (full, t["index"]))
        if 0 <= t["field_start"]:
            for j in range(t["field_start"], t["field_start"] + t["field_count"]):
                if 0 <= j < field_total:
                    out.write("    field %s\n" % fields[j])
        if 0 <= t["method_start"]:
            for j in range(t["method_start"], t["method_start"] + t["method_count"]):
                if 0 <= j < method_count:
                    out.write("    method %s (token=0x%08X, methodIndex=%d)\n" % (methods[j][0], methods[j][2], j))

print("wrote classes.txt")
