Import("env")

# Inject the shared protocol headers (single source of truth in
# Gateway/src/common) as a RAW, relative include flag.
#
# Why not `build_flags = -I../Gateway/src/common`? PlatformIO rewrites any
# `-I` in build_flags to an absolute path, and the toolchain cannot open a
# -I directory whose absolute path contains non-ASCII chars (this repo lives
# under a Unicode OneDrive path). Appending the flag here keeps it relative;
# SCons runs the compiler from the project root, so the kernel resolves
# "../Gateway/src/common" against the (Unicode) CWD fine.
env.Append(CCFLAGS=["-I../Gateway/src/common"])
