#!/usr/bin/env python3
"""Print the exception code, faulting address and owning module of a Windows minidump.

Standard library only; reads the exception and module-list streams, nothing else, and prints no memory
contents. Usage: minidump_summary.py DUMP [DUMP ...]
"""
import struct
import sys

EXCEPTION_STREAM, MODULE_LIST_STREAM = 6, 4
KNOWN = {
    0xC0000005: "ACCESS_VIOLATION", 0xC0000374: "HEAP_CORRUPTION", 0xC0000409: "STACK_BUFFER_OVERRUN / fail-fast",
    0xC000001D: "ILLEGAL_INSTRUCTION", 0x80000003: "BREAKPOINT", 0xC00000FD: "STACK_OVERFLOW", 0xE06D7363: "C++ exception",
}


def summarize(path):
    with open(path, "rb") as handle:
        data = handle.read()
    if data[:4] != b"MDMP":
        return f"{path}: not a minidump"
    count, directory = struct.unpack_from("<II", data, 8)
    streams = {}
    for index in range(count):
        kind, size, rva = struct.unpack_from("<III", data, directory + index * 12)
        streams.setdefault(kind, (size, rva))
    if EXCEPTION_STREAM not in streams:
        return f"{path}: no exception stream"
    _, rva = streams[EXCEPTION_STREAM]
    thread_id = struct.unpack_from("<I", data, rva)[0]
    code, flags = struct.unpack_from("<II", data, rva + 8)
    address = struct.unpack_from("<Q", data, rva + 24)[0]
    module = "?"
    if MODULE_LIST_STREAM in streams:
        _, mrva = streams[MODULE_LIST_STREAM]
        modules = struct.unpack_from("<I", data, mrva)[0]
        for index in range(modules):
            base, size = struct.unpack_from("<QI", data, mrva + 4 + index * 108)
            name_rva = struct.unpack_from("<I", data, mrva + 4 + index * 108 + 20)[0]
            if base <= address < base + size:
                length = struct.unpack_from("<I", data, name_rva)[0]
                name = data[name_rva + 4:name_rva + 4 + length].decode("utf-16-le", "replace")
                module = f"{name.rsplit(chr(92), 1)[-1]} +0x{address - base:x}"
                break
    return f"{path.rsplit(chr(92), 1)[-1]}: thread {thread_id} {KNOWN.get(code, hex(code))} (0x{code:08x}) at 0x{address:x} in {module}"


if __name__ == "__main__":
    for dump in sys.argv[1:]:
        print(summarize(dump))
