#!/usr/bin/env python3
"""Records and compares what a set of built artifacts depends on.

A distribution manifest is a lock file for binaries. It pins, for every file
shipped, the facts that decide where that file will actually run: the
container format, the architecture, the shared libraries and symbol versions
it needs, the minimum OS it declares. Regenerating it turns "the build
changed" into a reviewable diff instead of a bug report from whoever has the
oldest machine.

For ET: Legacy the CI collects the artifacts of every platform build into
`./dist/<platform>/*`, and `dist-manifest.json` in the repository root pins
what each of those engine binaries, renderer libraries and mod libraries is
allowed to depend on:

    pixi run dist-manifest-check     assert ./dist against dist-manifest.json
    pixi run dist-manifest-record    rewrite dist-manifest.json from ./dist

Nothing here writes a binary; the four subcommands only ever read one.

    dist-manifest.py show   <path>...            what these files depend on
    dist-manifest.py record <dir>  -o FILE       lock a directory down
    dist-manifest.py check  <dir>  -m FILE       assert it still matches
    dist-manifest.py diff   <a> <b>              compare two of anything

A manifest is a map of relative path to profile, and nothing else, so with
the CI layout every key starts with the platform directory:

    {"files": {"lnx64/qagame.mp.x86_64.so": {"format": "elf", "arch": "x86-64", ...}}}

Only files that are binaries are part of a directory's contents. The
artifacts also carry pk3s, archives, scripts and configs, whose names change
with every version and which none of the readers below could describe
anyway. A file is recognised by its magic number, so a binary that is renamed
still counts; one that breaks badly enough to lose its magic shows up as
missing.

All directory scans use `DIST_FILTERS`, a whitelist of glob paths relative
to the supplied directory (normally `./dist/`). These select the engine,
renderer and mod binaries rather than build dependencies or installers.
Explicit binary paths passed to `show` or `diff` can still name any binary.

`check` reports every file that is missing, every file present but unlisted,
and every key that reads back differently from what was recorded. It does not
judge, because there is nothing left for a separate judgement to catch: a
recorded profile already pins every fact this tool can read, so a rule saying
"the glibc requirement may not exceed 2.3.3" only ever fires where "the glibc
requirement is no longer 2.3.3" already has. Deciding whether a difference is
acceptable is a job for whoever reads the diff.

On a mismatch, `check` prints a complete unified patch to stdout updating
the supplied manifest to match the selected binaries. Diagnostics go to
stderr and the command exits with status 1. The patch can be saved as a
`.patch` or `.diff` file and applied with `patch -p1` from the current
directory. Unreadable binaries are reported, but their existing profiles
are preserved in the patch rather than replaced with guessed values.

Reading the formats
-------------------

The files have to be inspected from whichever host happens to run the check,
and no system tool covers all three containers: readelf cannot read Mach-O,
otool only exists on macOS, and a Windows host has neither. So instead of
shelling out, the readers below walk each container themselves.

`profile()` is the entry point. Which keys come back depends on the format,
because the formats genuinely record different things:

    all                format, arch
    elf                needs, symbol-versions, runpath, min-kernel, interpreter
    pe                 needs, imports, min-os, subsystem-version
    macho              needs, dylib-compat, rpath, macos-min, code-signature
    archive            cxx-runtime
    macho-universal    slices, one full macho profile per architecture
    archive-universal  slices, one archive profile per architecture

`arch` is a string everywhere except for a universal binary, where it is the
sorted list of the architectures the file carries.

Dependency names are whatever the format itself stores: a soname for ELF
(`libc.so.6`), an import table entry for PE (`KERNEL32.dll`), an install name
for Mach-O (`/usr/lib/libSystem.B.dylib`). The one exception is ELF, where a
glibc library the binary takes no symbol from is left out, because whether the
linker keeps it depends on the build host (see `_elf`).

One thing deliberately absent: the CPU baseline the code was generated for.
Nothing in any of these containers records it unless the toolchain emits a
`.note.gnu.property` marker, and most do not, so that has to be pinned at
build time instead of checked afterwards.
"""

import argparse
import difflib
import json
import os
import re
import struct
import sys
from pathlib import Path


class UnknownFormat(Exception):
    """Raised for a file whose container format is not recognised."""


def version_tuple(text):
    """`"2.28"` becomes `(2, 28, 0, 0)`, so versions of unequal length compare."""
    parts = tuple(int(part) for part in text.split("."))
    return parts + (0,) * (4 - len(parts)) if len(parts) < 4 else parts


_ELF_MACHINES = {
    0x03: "x86",
    0x28: "arm",
    0x3E: "x86-64",
    0xB7: "arm64",
    0xF3: "riscv64",
}
_PE_MACHINES = {
    0x014C: "x86",
    0x01C0: "arm",
    0x01C4: "armv7",
    0x8664: "x86-64",
    0xAA64: "arm64",
}
_MACHO_CPUS = {7: "x86", 12: "arm", 0x01000007: "x86-64", 0x0100000C: "arm64"}

_MACHO_MAGIC = (
    b"\xcf\xfa\xed\xfe",
    b"\xce\xfa\xed\xfe",
    b"\xfe\xed\xfa\xcf",
    b"\xfe\xed\xfa\xce",
)
# FAT_MAGIC and FAT_MAGIC_64, the wrappers around several thin Mach-O images.
_FAT_MAGIC = (b"\xca\xfe\xba\xbe", b"\xca\xfe\xba\xbf")


def is_binary(path):
    """Whether the file at `path` starts like one of the formats read here."""
    with open(path, "rb") as handle:
        head = handle.read(8)
    return (
        head[:4] in (b"\x7fELF",) + _FAT_MAGIC + _MACHO_MAGIC
        or head[:2] == b"MZ"
        or head == b"!<arch>\n"
    )


def profile(path):
    """Everything about the file at `path` that bears on where it will run."""
    with open(path, "rb") as handle:
        data = handle.read()

    if data[:4] == b"\x7fELF":
        return _elf(data)
    if data[:2] == b"MZ":
        return _pe(data)
    if data[:4] in _FAT_MAGIC:
        return _fat(data)
    if data[:4] in _MACHO_MAGIC:
        return _macho(data)
    if data[:8] == b"!<arch>\n":
        return _archive(data)

    raise UnknownFormat(f"{path}: unrecognised binary format")


def _cstring(data, offset):
    return data[offset : data.index(b"\0", offset)].decode("utf-8", "replace")


def _unknown(value):
    return f"unknown({value:#x})"


# --------------------------------------------------------------------------
# ELF
# --------------------------------------------------------------------------

# Symbol version tokens look like `GLIBC_2.16` or `GLIBCXX_3.4.21`. The greedy
# prefix makes the split fall on the last underscore that is followed by a
# plain version, which is what every toolchain library in practice uses.
# `GLIBC_PRIVATE` carries no version and so is skipped rather than parsed.
_VERSION_TOKEN = re.compile(r"^(.+)_([0-9][0-9.]*)$")

# The libraries glibc itself is split into, the dynamic loader included. Each
# symbol they export is versioned, so whether a binary takes anything from one
# of them can be read off .gnu.version_r.
_GLIBC_SONAME = re.compile(
    r"^(lib(c|m|pthread|dl|rt|util|resolv|anl|nsl|crypt|BrokenLocale|mvec)\.so\.[0-9]+"
    r"|ld-linux[-a-z0-9_]*\.so\.[0-9]+|ld\.so\.[0-9]+)$"
)


def _elf_sections(data):
    """Byte order, 64-bit flag, and the sections as (name, type, offset, size, link)."""
    is_64 = data[4] == 2
    order = "<" if data[5] == 1 else ">"

    # Section headers are the shortest route to the tables below; going through
    # the program headers instead would mean translating virtual addresses.
    if is_64:
        table = struct.unpack_from(order + "Q", data, 0x28)[0]
        entry_size, count, name_index = struct.unpack_from(order + "HHH", data, 0x3A)
        fields = (
            order + "II8x8xQQI"
        )  # sh_name, sh_type, .., sh_offset, sh_size, sh_link
    else:
        table = struct.unpack_from(order + "I", data, 0x20)[0]
        entry_size, count, name_index = struct.unpack_from(order + "HHH", data, 0x2E)
        fields = order + "II4x4xIII"

    raw = [
        struct.unpack_from(fields, data, table + index * entry_size)
        for index in range(count)
    ]

    strings = raw[name_index][2] if count else 0
    return (
        order,
        is_64,
        [(_cstring(data, strings + entry[0]), *entry[1:]) for entry in raw],
    )


def _elf(data):
    order, is_64, sections = _elf_sections(data)

    entry_format = order + ("qQ" if is_64 else "iI")
    entry_width = struct.calcsize(entry_format)

    needs = []
    runpath = []
    for _name, kind, offset, size, link in sections:
        if kind != 6:  # SHT_DYNAMIC
            continue

        # sh_link of .dynamic points at the string table holding the names.
        strings = sections[link][2]

        for position in range(offset, offset + size, entry_width):
            tag, value = struct.unpack_from(entry_format, data, position)
            if tag == 0:  # DT_NULL terminates the array
                break
            if tag == 1:  # DT_NEEDED
                needs.append(_cstring(data, strings + value))
            elif tag in (15, 29):  # DT_RPATH, DT_RUNPATH
                runpath += [
                    part for part in _cstring(data, strings + value).split(":") if part
                ]

    versions, bound = _elf_symbol_versions(data, order, sections)

    # Whether an unused glibc library still gets a DT_NEEDED entry depends on
    # the toolchain rather than on the code: a linker run with --as-needed
    # drops every one the binary takes nothing from, one run with
    # --no-as-needed (the default of several distributions) keeps them all,
    # and a glibc of 2.34 or newer merged most of them into libc. Such an entry
    # changes nothing about where the file runs, since every glibc system has
    # all of them, so it is left out to make the profile read the same from
    # every host. A glibc library the code really uses stays in, because its
    # versioned symbols put it into .gnu.version_r. Without any version
    # information there is nothing to go on, and everything is kept.
    if bound:
        needs = [
            name for name in needs if name in bound or not _GLIBC_SONAME.match(name)
        ]

    result = {
        "format": "elf",
        "arch": _ELF_MACHINES.get(
            struct.unpack_from(order + "H", data, 0x12)[0],
            _unknown(struct.unpack_from(order + "H", data, 0x12)[0]),
        ),
        "needs": sorted(needs),
        "symbol-versions": versions,
        "runpath": sorted(runpath),
        # null for a statically linked image. Without this a static binary and
        # a dynamic one that happens to need nothing look identical, and the
        # path itself is architecture specific, so it is worth pinning down.
        "interpreter": _elf_interpreter(data, sections),
    }

    kernel = _elf_min_kernel(data, order, sections)
    if kernel is not None:
        result["min-kernel"] = kernel
    return result


def _elf_symbol_versions(data, order, sections):
    """Highest version required per token namespace, e.g. `{"GLIBC": "2.16"}`,
    and the set of libraries any versioned symbol is bound to.

    Keyed on the namespace rather than on the library, because that is the part
    that decides compatibility: `GLIBC_2.16` means glibc 2.16 or newer no
    matter which of libc/libm/libpthread happened to pull it in.
    """
    highest = {}
    bound = set()
    for _name, kind, offset, _size, link in sections:
        if kind != 0x6FFFFFFE:  # SHT_GNU_verneed
            continue

        strings = sections[link][2]
        position = offset
        while True:
            # Elf_Verneed: version, aux count, file name, offset to the first
            # aux entry, offset to the next verneed. Both offsets are relative
            # to the record they are stored in.
            _version, count, file, aux, next_entry = struct.unpack_from(
                order + "HHIII", data, position
            )
            bound.add(_cstring(data, strings + file))

            aux_position = position + aux
            for _ in range(count):
                # Elf_Vernaux: hash, flags, other, name, offset to the next.
                _hash, _flags, _other, name, aux_next = struct.unpack_from(
                    order + "IHHII", data, aux_position
                )

                match = _VERSION_TOKEN.match(_cstring(data, strings + name))
                if match:
                    namespace, found = match.group(1), match.group(2)
                    if namespace not in highest or version_tuple(found) > version_tuple(
                        highest[namespace]
                    ):
                        highest[namespace] = found

                if aux_next == 0:
                    break
                aux_position += aux_next

            if next_entry == 0:
                break
            position += next_entry

    return dict(sorted(highest.items())), bound


def _elf_interpreter(data, sections):
    """The dynamic loader named in .interp, or None for a static binary."""
    for name, kind, offset, _size, _link in sections:
        if kind == 1 and name == ".interp":  # SHT_PROGBITS
            return _cstring(data, offset)
    return None


def _elf_min_kernel(data, order, sections):
    """Oldest Linux kernel the binary declares it runs on, from .note.ABI-tag."""
    for name, kind, offset, _size, _link in sections:
        if kind != 7 or name != ".note.ABI-tag":  # SHT_NOTE
            continue

        name_size, _desc_size, note_type = struct.unpack_from(
            order + "III", data, offset
        )
        if note_type != 1:  # NT_GNU_ABI_TAG
            continue

        # The note name is padded out to a four byte boundary.
        descriptor = offset + 12 + (name_size + 3) // 4 * 4
        os_id, major, minor, patch = struct.unpack_from(
            order + "IIII", data, descriptor
        )
        if os_id != 0:  # 0 is Linux; the tag is also used for other kernels
            continue
        return f"{major}.{minor}.{patch}"

    return None


# --------------------------------------------------------------------------
# PE
# --------------------------------------------------------------------------


def _pe(data):
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe_offset : pe_offset + 4] != b"PE\0\0":
        raise UnknownFormat("not a PE image")

    coff = pe_offset + 4
    machine = struct.unpack_from("<H", data, coff)[0]
    section_count = struct.unpack_from("<H", data, coff + 2)[0]
    optional_size = struct.unpack_from("<H", data, coff + 16)[0]

    optional = coff + 20
    magic = struct.unpack_from("<H", data, optional)[0]

    # PE32 spends four bytes on BaseOfData where PE32+ widens ImageBase, so the
    # version fields land at the same place in both; only the data directory
    # array that follows the optional header proper moves.
    os_major, os_minor, _image_major, _image_minor, subsystem_major, subsystem_minor = (
        struct.unpack_from("<HHHHHH", data, optional + 40)
    )
    directories = optional + (112 if magic == 0x20B else 96)

    imports = _pe_imports(
        data,
        optional + optional_size,
        section_count,
        struct.unpack_from("<I", data, directories + 8)[0],
        magic == 0x20B,
    )

    return {
        "format": "pe",
        "arch": _PE_MACHINES.get(machine, _unknown(machine)),
        "needs": sorted(imports),
        # Which Windows version a PE actually requires is not recorded anywhere
        # in the file; min-os and the subsystem version below are whatever the
        # linker was told to write. The function names are the evidence, since
        # an API that did not exist yet is what really stops the image from
        # loading, so they are listed rather than summarised away.
        "imports": {name: sorted(imports[name]) for name in sorted(imports)},
        # The loader refuses to start an image whose subsystem version is above
        # the running Windows; min-os is advisory but recorded alongside it.
        "min-os": f"{os_major}.{os_minor}",
        "subsystem-version": f"{subsystem_major}.{subsystem_minor}",
    }


def _pe_imports(data, section_table, section_count, import_rva, is_64):
    """The import table as `{"KERNEL32.dll": ["CloseHandle", ...]}`."""
    if import_rva == 0:
        return {}

    sections = []
    for index in range(section_count):
        base = section_table + index * 40
        virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from(
            "<IIII", data, base + 8
        )
        sections.append((virtual_address, max(virtual_size, raw_size), raw_offset))

    def to_offset(rva):
        for virtual_address, size, raw_offset in sections:
            if virtual_address <= rva < virtual_address + size:
                return raw_offset + (rva - virtual_address)
        raise UnknownFormat(f"RVA {rva:#x} is outside every section")

    # Thunks are pointer sized, and the top bit means "imported by ordinal"
    # rather than "RVA of a hint/name entry".
    thunk_format = "<Q" if is_64 else "<I"
    thunk_width = struct.calcsize(thunk_format)
    ordinal_flag = 1 << (thunk_width * 8 - 1)

    found = {}
    descriptor = to_offset(import_rva)
    while True:
        # A 20 byte all-zero descriptor terminates the import directory.
        lookup_rva, _timestamp, _forwarder, name_rva, address_rva = struct.unpack_from(
            "<IIIII", data, descriptor
        )
        if not any((lookup_rva, _timestamp, _forwarder, name_rva, address_rva)):
            break

        # The lookup table is the one that survives binding; fall back to the
        # address table for images that have none.
        functions = []
        thunk = to_offset(lookup_rva or address_rva)
        while True:
            value = struct.unpack_from(thunk_format, data, thunk)[0]
            if value == 0:
                break
            if value & ordinal_flag:
                functions.append(f"#{value & 0xFFFF}")
            else:
                # A hint/name entry is a two byte hint followed by the name.
                functions.append(_cstring(data, to_offset(value) + 2))
            thunk += thunk_width

        found[_cstring(data, to_offset(name_rva))] = functions
        descriptor += 20

    return found


# --------------------------------------------------------------------------
# Mach-O
# --------------------------------------------------------------------------

# LC_LOAD_DYLIB, LC_LOAD_WEAK_DYLIB, LC_REEXPORT_DYLIB, LC_LOAD_UPWARD_DYLIB
_MACHO_DYLIB_COMMANDS = {0x0C, 0x80000018, 0x8000001F, 0x80000023}
_LC_VERSION_MIN_MACOSX = 0x24
_LC_BUILD_VERSION = 0x32
_LC_CODE_SIGNATURE = 0x1D
_LC_RPATH = 0x8000001C
_PLATFORM_MACOS = 1


def _macho_version(packed):
    return f"{packed >> 16}.{(packed >> 8) & 0xFF}.{packed & 0xFF}"


def _macho_order(magic):
    """Struct byte order prefix for a thin Mach-O with this magic."""
    return "<" if magic in (b"\xcf\xfa\xed\xfe", b"\xce\xfa\xed\xfe") else ">"


def _macho(data):
    magic = data[:4]
    order = _macho_order(magic)
    is_64 = magic in (b"\xcf\xfa\xed\xfe", b"\xfe\xed\xfa\xcf")

    cpu_type, cpu_subtype = struct.unpack_from(order + "iI", data, 4)
    command_count = struct.unpack_from(order + "I", data, 16)[0]
    position = 32 if is_64 else 28  # mach_header_64 has a trailing reserved field

    needs = []
    compatibility = {}
    rpath = []
    macos_min = None
    signed = False

    for _ in range(command_count):
        command, size = struct.unpack_from(order + "II", data, position)

        if command in _MACHO_DYLIB_COMMANDS:
            # The name is an lc_str: an offset from the start of the command.
            name_offset, _timestamp, _current, compat = struct.unpack_from(
                order + "IIII", data, position + 8
            )
            name = _cstring(data, position + name_offset)
            needs.append(name)
            # The loader rejects the image if the installed dylib is older than
            # this, so it is part of what decides compatibility.
            compatibility[name] = _macho_version(compat)
        elif command == _LC_RPATH:
            rpath.append(
                _cstring(
                    data,
                    position + struct.unpack_from(order + "I", data, position + 8)[0],
                )
            )
        elif command == _LC_BUILD_VERSION:
            platform_id, minimum, _sdk, _tools = struct.unpack_from(
                order + "IIII", data, position + 8
            )
            if platform_id == _PLATFORM_MACOS:
                macos_min = _macho_version(minimum)
        elif command == _LC_VERSION_MIN_MACOSX:
            # Superseded by LC_BUILD_VERSION, still emitted by older linkers.
            macos_min = _macho_version(
                struct.unpack_from(order + "I", data, position + 8)[0]
            )
        elif command == _LC_CODE_SIGNATURE:
            signed = True

        position += size

    result = {
        "format": "macho",
        "arch": _macho_arch(cpu_type, cpu_subtype),
        "needs": sorted(needs),
        "dylib-compat": dict(sorted(compatibility.items())),
        "rpath": sorted(rpath),
        # arm64 macOS refuses to execute an unsigned binary outright, so this
        # is a hard runs / does-not-run bit rather than a hardening detail.
        "code-signature": signed,
    }
    if macos_min is not None:
        result["macos-min"] = macos_min
    return result


def _fat(data):
    """A universal binary: several thin images concatenated into one file.

    Apple wraps static libraries the same way, so a slice is either a thin
    Mach-O image or an entire ar archive.
    """
    # The fat header is always big endian, whatever the slices inside it are.
    is_64 = data[:4] == b"\xca\xfe\xba\xbf"
    count = struct.unpack_from(">I", data, 4)[0]
    entry = ">iiQQII" if is_64 else ">iiIII"
    width = struct.calcsize(entry)

    # `cafebabe` is also the Java class file magic, where this field holds a
    # version number instead. A plausible slice count plus in-range slices is
    # what tells the two apart.
    if not 0 < count <= 64:
        raise UnknownFormat("not a universal binary")

    slices = {}
    for index in range(count):
        cpu_type, cpu_subtype, offset, size = struct.unpack_from(
            entry, data, 8 + index * width
        )[:4]
        if offset + size > len(data):
            raise UnknownFormat("universal binary slice runs past the end of the file")
        body = data[offset : offset + size]
        slices[_macho_arch(cpu_type, cpu_subtype)] = (
            _archive(body) if body[:8] == b"!<arch>\n" else _macho(body)
        )

    # A fat container full of archives is not a loadable image, and calling it
    # macho-universal would suggest it is, so the two are named apart.
    kind = (
        "archive" if all(s["format"] == "archive" for s in slices.values()) else "macho"
    )
    return {
        "format": f"{kind}-universal",
        "arch": sorted(slices),
        "slices": dict(sorted(slices.items())),
    }


def _macho_arch(cpu_type, cpu_subtype):
    name = _MACHO_CPUS.get(cpu_type, _unknown(cpu_type))
    # The subtype can narrow the architecture to CPUs newer than the baseline.
    variant = cpu_subtype & 0x00FFFFFF
    if name == "x86-64" and variant == 8:
        return "x86-64h"
    if name == "arm64" and variant == 2:
        return "arm64e"
    return name


# --------------------------------------------------------------------------
# ar archives
# --------------------------------------------------------------------------

# Neither the symbol table nor the long-name table is an object file. The
# first two spellings are GNU's (`/` and `//`), the rest are BSD's, which is
# what zig ar emits for macOS targets.
_ARCHIVE_SPECIAL = {"", "/SYM64", "__.SYMDEF", "__.SYMDEF SORTED"}


def _archive_members(data):
    """Yields the bytes of each archive member that is an object file."""
    position = 8
    while position + 60 <= len(data):
        header = data[position : position + 16].decode("ascii", "replace").strip()
        size = int(
            data[position + 48 : position + 58].decode("ascii", "replace").strip() or 0
        )
        body = position + 60
        member_end = body + size + (size % 2)  # members are padded to even offsets

        if header.startswith("#1/"):
            # BSD archives put a long member name in the first bytes of the
            # member data and record only its length in the header field.
            name_length = int(header[3:])
            name = (
                data[body : body + name_length].rstrip(b"\0").decode("ascii", "replace")
            )
            body += name_length
            size -= name_length
        else:
            name = header

        if name.rstrip("/") not in _ARCHIVE_SPECIAL:
            yield data[body : body + size]

        position = member_end


def _archive(data):
    """A static library records no dependencies of its own, so what can be
    recovered is what its members are: the architecture they were built for,
    and which C++ standard library they expect to be linked against."""
    arch = None
    undefined = []
    for member in _archive_members(data):
        if arch is None:
            arch = _member_arch(member)
        undefined += _member_undefined(member)

    return {
        "format": "archive",
        "arch": arch or "unknown",
        "cxx-runtime": _cxx_runtime(undefined),
    }


def _cxx_runtime(undefined):
    """Which C++ standard library the undefined symbols come from, if any.

    The three are not link compatible, and an archive says nothing about which
    one it was built against, so a consumer that assumes the wrong one only
    finds out at link time. libc++ wraps everything in the inline namespace
    `std::__1`, which mangles as `St3__1`; libstdc++ has no such namespace;
    MSVC does not use Itanium mangling at all.
    """
    evidence = set()
    for name in undefined:
        if "St3__1" in name:
            evidence.add("libc++")
        elif name.startswith("?") and "@std@@" in name:
            evidence.add("msvc-stl")
        elif name.startswith("_Z") and "St" in name:
            evidence.add("libstdc++")

    # A handful of `std::` symbols mangle identically under libc++ and
    # libstdc++, so the specific finding has to win over the generic one.
    for candidate in ("libc++", "msvc-stl", "libstdc++"):
        if candidate in evidence:
            return candidate
    return None


def _member_undefined(member):
    """Externally undefined symbol names of one archive member."""
    if member[:4] == b"\x7fELF":
        return _elf_undefined(member)
    if member[:4] in _MACHO_MAGIC:
        return _macho_undefined(member)
    if (
        len(member) >= 20
        and struct.unpack_from("<H", member, 0)[0] in _PE_MACHINES
        and struct.unpack_from("<H", member, 16)[0] == 0
    ):
        return _coff_undefined(member)
    return []


def _elf_undefined(data):
    """SHT_SYMTAB entries whose section index is SHN_UNDEF."""
    order, is_64, sections = _elf_sections(data)
    # Elf64_Sym is name, info, other, shndx, value, size; Elf32_Sym orders the
    # same fields differently, hence the two index positions below.
    entry = order + ("IBBHQQ" if is_64 else "IIIBBH")
    width = struct.calcsize(entry)
    shndx_at = 3 if is_64 else 5

    names = []
    for _name, kind, offset, size, link in sections:
        if kind != 2:  # SHT_SYMTAB
            continue
        strings = sections[link][2]
        for position in range(offset, offset + size, width):
            fields = struct.unpack_from(entry, data, position)
            if fields[shndx_at] == 0 and fields[0]:
                names.append(_cstring(data, strings + fields[0]))
    return names


_LC_SYMTAB = 0x02


def _macho_undefined(data):
    """LC_SYMTAB entries that are external and of type N_UNDF."""
    magic = data[:4]
    order = _macho_order(magic)
    is_64 = magic in (b"\xcf\xfa\xed\xfe", b"\xfe\xed\xfa\xcf")
    command_count = struct.unpack_from(order + "I", data, 16)[0]
    position = 32 if is_64 else 28

    entry = order + ("IBBHQ" if is_64 else "IBBhI")
    width = struct.calcsize(entry)

    names = []
    for _ in range(command_count):
        command, size = struct.unpack_from(order + "II", data, position)
        if command == _LC_SYMTAB:
            symbols, count, strings, _length = struct.unpack_from(
                order + "IIII", data, position + 8
            )
            for index in range(count):
                name, kind = struct.unpack_from(entry, data, symbols + index * width)[
                    :2
                ]
                # N_TYPE is bits 1 to 3; 0 there is N_UNDF, and bit 0 is N_EXT.
                if kind & 0x0E == 0 and kind & 0x01 and name:
                    names.append(_cstring(data, strings + name))
        position += size
    return names


def _coff_undefined(data):
    """COFF symbols with section number 0 and external storage class."""
    table, count = struct.unpack_from("<II", data, 8)
    if table == 0:
        return []
    strings = table + 18 * count

    names = []
    index = 0
    while index < count:
        record = table + index * 18
        section, _kind, storage, auxiliary = struct.unpack_from(
            "<hHBB", data, record + 12
        )
        if section == 0 and storage == 2:  # IMAGE_SYM_CLASS_EXTERNAL
            # A name of eight characters or less sits in the record; a longer
            # one is a zero word followed by an offset into the string table.
            if struct.unpack_from("<I", data, record)[0] == 0:
                names.append(
                    _cstring(
                        data, strings + struct.unpack_from("<I", data, record + 4)[0]
                    )
                )
            else:
                names.append(
                    data[record : record + 8].rstrip(b"\0").decode("utf-8", "replace")
                )
        index += 1 + auxiliary  # auxiliary records follow their symbol
    return names


def _member_arch(member):
    if member[:4] == b"\x7fELF":
        order = "<" if member[5] == 1 else ">"
        machine = struct.unpack_from(order + "H", member, 0x12)[0]
        return _ELF_MACHINES.get(machine, _unknown(machine))

    if member[:4] in _MACHO_MAGIC:
        cpu_type, cpu_subtype = struct.unpack_from(
            _macho_order(member[:4]) + "iI", member, 4
        )
        return _macho_arch(cpu_type, cpu_subtype)

    # A COFF object has no magic; a known machine plus the optional header size
    # being zero, as it always is for an object file, is enough to recognise it.
    if len(member) >= 20:
        machine, _sections = struct.unpack_from("<HH", member, 0)
        if machine in _PE_MACHINES and struct.unpack_from("<H", member, 16)[0] == 0:
            return _PE_MACHINES[machine]

    return None


# --------------------------------------------------------------------------
# Manifests
# --------------------------------------------------------------------------

# Paths are rooted at ./dist/, or the directory supplied to any subcommand.
# Each * matches one path component, so build trees are not included.
DIST_FILTERS = [
    # Linux engines, dynamic renderers and mod libraries.
    "lnx*/etl.*",
    "lnx*/etlded.*",
    "lnx*/librenderer_*.so",
    "lnx*/legacy/cgame.mp.*.so",
    "lnx*/legacy/qagame.mp.*.so",
    "lnx*/legacy/tvgame.mp.*.so",
    "lnx*/legacy/ui.mp.*.so",
    # Windows executables and DLLs, excluding installers and import libraries.
    "win*/etl.exe",
    "win*/etlded.exe",
    "win*/renderer_*.dll",
    "win*/legacy/cgame_mp_*.dll",
    "win*/legacy/qagame_mp_*.dll",
    "win*/legacy/tvgame_mp_*.dll",
    "win*/legacy/ui_mp_*.dll",
    # macOS build outputs, including the client inside its app bundle.
    "osx/ET Legacy.app/Contents/MacOS/ET Legacy",
    "osx/etlded",
    "osx/librenderer_*_mac",
    "osx/legacy/cgame_mac",
    "osx/legacy/qagame_mac",
    "osx/legacy/tvgame_mac",
    "osx/legacy/ui_mac",
    # Android libraries extracted from the APK, grouped by ABI.
    "android/lib/*/libetl.so",
    "android/lib/*/libcgame.mp.android.*.so",
    "android/lib/*/libqagame.mp.android.*.so",
    "android/lib/*/libtvgame.mp.android.*.so",
    "android/lib/*/libui.mp.android.*.so",
]


def contents(directory):
    """Whitelisted binaries, keyed by their directory-relative paths."""
    directory = Path(directory)
    if not directory.is_dir():
        raise NotADirectoryError(f"{directory}: not a directory")
    # Deduplicate overlapping patterns before opening or profiling any files.
    paths = {path for pattern in DIST_FILTERS for path in directory.glob(pattern)}
    return {
        path.relative_to(directory).as_posix(): path
        for path in sorted(paths)
        if path.is_file() and is_binary(path)
    }


def record(directory):
    """Records profiles of binaries selected by `DIST_FILTERS`.

    A binary that cannot be parsed is left to fail, because a manifest that
    silently skipped what it could not read would go on claiming the
    directory was fully described.
    """
    return {
        "files": {name: profile(path) for name, path in contents(directory).items()}
    }


def check(directory, manifest, current=None):
    """Asserts `directory` against `manifest`. Returns the list of problems.

    Every recorded file has to still be there and still read back the same
    facts, and no selected binary may be unlisted. If supplied, `current`
    receives the observed manifest for generating a patch without rereading
    the binaries.
    """
    problems = []
    recorded_files = manifest.get("files", {})
    present = contents(directory)
    found_files = {}

    # Inspect added binaries too, so the patch contains their full profiles.
    for name, path in present.items():
        try:
            found = profile(path)
        except (UnknownFormat, OSError, ValueError, struct.error) as error:
            problems.append(f"{name}: {error}")
            # A read failure cannot justify replacing an existing profile.
            if name in recorded_files:
                found_files[name] = recorded_files[name]
            continue

        found_files[name] = found
        if name in recorded_files:
            problems += _differences(
                name, recorded_files[name], found, ("manifest", "file")
            )
        else:
            problems.append(f"{name}: present but not listed in the manifest")

    for name in sorted(recorded_files.keys() - present.keys()):
        problems.append(f"{name}: missing")

    if current is not None:
        current.update(manifest)
        current["files"] = found_files

    return problems


def diff(left, right):
    """Compares two manifests, two directories or two single binaries.

    Either side may be any of the three, so a directory of fresh builds can be
    held against a manifest from a release, or one project's binary against
    another's.
    """
    left_files, left_name = _side(left)
    right_files, right_name = _side(right)

    # Two single files are the interesting cross-project case, and there their
    # names almost never agree, so they are paired up positionally instead.
    if len(left_files) == 1 and len(right_files) == 1:
        (only_left,) = left_files.values()
        (only_right,) = right_files.values()
        return _differences("", only_left, only_right, (left_name, right_name))

    problems = []
    for name in sorted(set(left_files) | set(right_files)):
        if name not in left_files:
            problems.append(f"{name}: only in {right_name}")
        elif name not in right_files:
            problems.append(f"{name}: only in {left_name}")
        else:
            problems += _differences(
                name, left_files[name], right_files[name], (left_name, right_name)
            )
    return problems


def _side(source):
    """One side of a diff as (profiles by name, what to call it)."""
    path = Path(source)
    if path.is_dir():
        return (
            {name: profile(found) for name, found in contents(path).items()},
            path.name or str(path),
        )

    # A manifest is the only JSON this tool reads, so the suffix is enough to
    # tell it from a binary that happens to be passed alongside one.
    if path.suffix == ".json":
        return json.loads(path.read_text()).get("files", {}), path.name

    return {path.name: profile(path)}, path.name


def _differences(label, left, right, names):
    """Every leaf at which `left` and `right` disagree, one problem each.

    Descending rather than printing the two sides whole is what keeps a
    changed dependency from arriving as both profiles printed whole.
    """
    if isinstance(left, dict) and isinstance(right, dict):
        problems = []
        for key in sorted(set(left) | set(right)):
            where = f"{label}.{key}" if label else key
            if key not in left:
                problems.append(f"{where}: only {names[1]} has it, {right[key]!r}")
            elif key not in right:
                problems.append(f"{where}: only {names[0]} has it, {left[key]!r}")
            else:
                problems += _differences(where, left[key], right[key], names)
        return problems

    if isinstance(left, list) and isinstance(right, list):
        # A list of plain values is a set of facts, where saying what came and
        # went reads far better than a positional diff. A list of maps is an
        # ordered structure, so that one is walked position by position.
        if all(not isinstance(item, (dict, list)) for item in left + right):
            return _list_differences(label, left, right, names)

        problems = []
        for index in range(max(len(left), len(right))):
            where = f"{label}[{index}]"
            if index >= len(left):
                problems.append(f"{where}: only {names[1]} has it, {right[index]!r}")
            elif index >= len(right):
                problems.append(f"{where}: only {names[0]} has it, {left[index]!r}")
            else:
                problems += _differences(where, left[index], right[index], names)
        return problems

    if left != right:
        where = f"{label}: " if label else ""
        return [f"{where}{names[0]} has {left!r}, {names[1]} has {right!r}"]
    return []


def _list_differences(label, left, right, names):
    gone = [item for item in left if item not in right]
    added = [item for item in right if item not in left]

    problems = []
    if gone:
        problems.append(f"{label}: only {names[0]} has {gone!r}")
    if added:
        problems.append(f"{label}: only {names[1]} has {added!r}")
    # Search paths are consulted in the order they are stored, so a reordering
    # with the same entries is still a difference and has to be said out loud.
    if not problems and left != right:
        problems.append(f"{label}: the same entries in a different order")
    return problems


# --------------------------------------------------------------------------
# Command line
# --------------------------------------------------------------------------


def _github_annotation(message, level="error"):
    """Emit one annotation, escaping data so it stays inside the message."""
    message = message.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")
    sys.stdout.flush()
    print(f"::{level} ::{message}", file=sys.stderr)


def _report(problems, heading, github=False):
    """Prints the problems, if any, and returns the process exit status."""
    if not problems:
        return 0
    # The commands print to stdout before calling this; without the flush the
    # two streams arrive interleaved whenever either one is redirected.
    sys.stdout.flush()
    print(heading, file=sys.stderr)
    for problem in problems:
        print(f"  {problem}", file=sys.stderr)
        if github:
            _github_annotation(problem)
    return 1


def _command_show(args):
    print(json.dumps({str(path): profile(path) for path in args.path}, indent=4))
    return 0


def _command_record(args):
    manifest = record(args.directory)
    text = json.dumps(manifest, indent=4) + "\n"
    if args.output:
        Path(args.output).write_text(text)
        print(f"wrote {len(manifest['files'])} files to {args.output}")
    else:
        print(text, end="")
    return 0


def _command_check(args):
    original = Path(args.manifest).read_text()
    manifest = json.loads(original)
    current = {}
    problems = check(args.directory, manifest, current)
    if problems:
        # Diff the actual file text so the output applies even when its JSON
        # indentation or key ordering differs from record's output.
        replacement = json.dumps(current, indent=4) + "\n"
        patch_path = Path(os.path.relpath(args.manifest)).as_posix()
        for line in difflib.unified_diff(
            original.splitlines(keepends=True),
            replacement.splitlines(keepends=True),
            fromfile=f"a/{patch_path}\t",
            tofile=f"b/{patch_path}\t",
        ):
            sys.stdout.write(line)
            # difflib leaves unterminated lines untouched; patch needs the
            # explicit marker to distinguish them from the next diff line.
            if not line.endswith("\n"):
                sys.stdout.write("\n\\ No newline at end of file\n")

    status = _report(
        problems,
        f"\n{args.directory} does not match {args.manifest}",
        args.github,
    )
    if status == 0:
        print(f"ok: {len(manifest.get('files', {}))} files in {args.directory}")
    return status


def _command_diff(args):
    problems = diff(args.left, args.right)
    for problem in problems:
        print(problem)
        if args.github:
            _github_annotation(problem, "warning")
    if not problems:
        print("identical")
    return 0


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    show = subparsers.add_parser("show", help="print what the given files depend on")
    show.add_argument("path", nargs="+", type=Path)
    show.set_defaults(func=_command_show)

    record_parser = subparsers.add_parser(
        "record", help="write a manifest of whitelisted binaries in a directory"
    )
    record_parser.add_argument("directory", type=Path)
    record_parser.add_argument("-o", "--output", help="write here instead of stdout")
    record_parser.set_defaults(func=_command_record)

    check_parser = subparsers.add_parser(
        "check", help="assert a directory against a manifest"
    )
    check_parser.add_argument("directory", type=Path)
    check_parser.add_argument("-m", "--manifest", required=True)
    check_parser.set_defaults(func=_command_check)

    diff_parser = subparsers.add_parser(
        "diff", help="compare two manifests, directories or binaries"
    )
    diff_parser.add_argument("left")
    diff_parser.add_argument("right")
    diff_parser.set_defaults(func=_command_diff)

    # Accept the flag on either side of the subcommand without an omitted
    # subcommand flag overwriting the value supplied to the root parser.
    for command_parser in (parser, show, record_parser, check_parser, diff_parser):
        command_parser.add_argument(
            "--github",
            action="store_true",
            default=False if command_parser is parser else argparse.SUPPRESS,
            help="also emit GitHub Actions warning/error annotations on stderr",
        )

    args = parser.parse_args()
    try:
        return args.func(args)
    except (UnknownFormat, OSError, ValueError) as error:
        message = f"dist-manifest.py: {error}"
        print(message, file=sys.stderr)
        if args.github:
            _github_annotation(message)
        return 1


if __name__ == "__main__":
    sys.exit(main())
