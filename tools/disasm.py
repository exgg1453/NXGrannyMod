import struct
import sys
import json
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN

SO = "x/lib/arm64-v8a/libil2cpp.so"
so = open(SO, "rb").read()

e_shoff = struct.unpack_from("<Q", so, 0x28)[0]
e_shentsize = struct.unpack_from("<H", so, 0x3A)[0]
e_shnum = struct.unpack_from("<H", so, 0x3C)[0]
sections = []
for i in range(e_shnum):
    o = e_shoff + i * e_shentsize
    name, typ, flags, addr, off, size, link, info, align, entsize = struct.unpack_from("<IIQQQQIIQQ", so, o)
    sections.append({"type": typ, "addr": addr, "off": off, "size": size})


def vaddr_to_off(vaddr):
    for s in sections:
        if s["type"] == 8 or s["addr"] == 0:
            continue
        if s["addr"] <= vaddr < s["addr"] + s["size"]:
            return s["off"] + (vaddr - s["addr"])
    return None


methods = json.load(open("methods.json"))


def method_range(class_name, method_name):
    entries = []
    for cls, items in methods.items():
        for mn, addr in items:
            entries.append(addr)
    entries = sorted(set(entries))
    start = None
    for mn, addr in methods[class_name]:
        if mn == method_name:
            start = addr
    if start is None:
        return None, None
    idx = entries.index(start)
    end = entries[idx + 1] if idx + 1 < len(entries) else start + 0x2000
    return start, end


def read_float(vaddr):
    off = vaddr_to_off(vaddr)
    if off is None:
        return None
    return struct.unpack_from("<f", so, off)[0]


def disassemble(class_name, method_name, limit=100000):
    start, end = method_range(class_name, method_name)
    if start is None:
        print("method not found")
        return
    size = min(end - start, limit)
    off = vaddr_to_off(start)
    code = so[off:off + size]
    md = Cs(CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN)
    md.detail = False
    print("== %s::%s  0x%x  size=%d bytes" % (class_name, method_name, start, size))
    constants = []
    for insn in md.disasm(code, start):
        text = "%s %s" % (insn.mnemonic, insn.op_str)
        if insn.mnemonic == "fmov" and "#" in insn.op_str:
            value = insn.op_str.split("#")[-1].strip()
            constants.append(("fmov", insn.address, value))
        if insn.mnemonic in ("ldr", "ldur") and "[pc," in insn.op_str.replace(" ", ""):
            try:
                disp = int(insn.op_str.split("#")[-1].rstrip("]"), 0)
                literal = insn.address + disp
                value = read_float(literal)
                constants.append(("pool", insn.address, "%.6f" % value if value is not None else "?"))
            except Exception:
                pass
    return constants


if __name__ == "__main__":
    cls = sys.argv[1]
    mtd = sys.argv[2]
    limit = int(sys.argv[3]) if len(sys.argv) > 3 else 100000
    consts = disassemble(cls, mtd, limit)
    seen = []
    for kind, addr, value in consts:
        seen.append((kind, hex(addr), value))
    print("float constants in order:")
    for kind, addr, value in seen:
        print("   %-5s %-12s %s" % (kind, addr, value))
