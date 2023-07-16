
#ifndef LIBC_VERSION
#error "LIBC_VERSION not defined! (e.g. \"8\")"
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) // TODO: test this
#define LD_ARGS()                                                                                                      \
    {                                                                                                                  \
        LD_PATH,                                                                                                       \
                "-dynamic-linker"                                                                                      \
                "-L" STATICLIB_DIR,                                                                                    \
                LLVM_LDFLAGS, "-L" STATICLIB_DIR "" _SNOWBALL_LIBRARY_OBJ, p_input, "-lSnowballRuntime", "-lc",        \
                "-lgcc", "-lm"                                                                                         \
    }
#elif __APPLE__
#define LD_ARGS()                                                                                                      \
    {                                                                                                                  \
        LD_PATH, "-syslibroot", "/Library/Developer/CommandLineTools/SDKs/MacOSX13.sdk/",                              \
                "-dynamic"                                                                                             \
                "-macosx_version_min",                                                                                 \
                "13.0.0", "-L" STATICLIB_DIR PATH_SEPARATOR _SNOWBALL_LIBRARY_OBJ, p_input, "-lSnowballRuntime",       \
                "-lc", "-lSystem", "-lm"                                                                               \
    }
#elif __linux__
#if defined(_SNOWBALL_BUILD_FOR_CE)
#define LD_ARGS()                                                                                                      \
    {                                                                                                                  \
        LD_PATH, "-dynamic-linker", "/lib64/ld-linux-x86-64.so.2", "/usr/lib/x86_64-linux-gnu/crt1.o",                 \
                "/usr/lib/x86_64-linux-gnu/crti.o", "/usr/lib/x86_64-linux-gnu/crtn.o", LLVM_LDFLAGS,                  \
                "-L" + (std::string)(((fs::path)utils::get_lib_folder()) / ".." / _SNOWBALL_LIBRARY_OBJ), p_input,     \
                "-lSnowballRuntime", "-lc", "-lm"                                                                      \
    }
#else
#define LD_ARGS()                                                                                                      \
    {                                                                                                                  \
        LD_PATH, "-dynamic-linker", "/lib64/ld-linux-x86-64.so.2", "/usr/lib/x86_64-linux-gnu/crt1.o",                 \
                "/usr/lib/x86_64-linux-gnu/crti.o", "/usr/lib/x86_64-linux-gnu/crtn.o", LLVM_LDFLAGS,                  \
                "-L" STATICLIB_DIR PATH_SEPARATOR _SNOWBALL_LIBRARY_OBJ, p_input, "-lSnowballRuntime", "-lc", "-lm"    \
    }
#endif
#else
#error "Unknown compiler"
#endif