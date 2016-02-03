#if defined __LP64__
#define SIZEOF_LONG 8
#define SIZEOF_VOID_P 8
#else
#define SIZEOF_LONG 4
#define SIZEOF_VOID_P 4
#endif

#if defined __BIG_ENDIAN__
#define WORDS_BIGENDIAN 1
#elif defined __linux__
#include <endian.h>
#if __BYTE_ORDER == __BIG_ENDIAN
#define WORDS_BIGENDIAN 1
#endif
#endif

#define HAVE_FCNTL_H 1
#define HAVE_UNISTD_H 0
#define HAVE_SIGACTION 1
#define HAVE_SIGNAL 1
#define HAVE_GETTIMEOFDAY 1

#if defined __APPLE__ && defined __MACH__
#define HAVE_MACH_VM 1
#else
#define HAVE_MMAP_VM 1
#endif

#ifdef HAVE_MACH_VM
#define HAVE_MACH_EXCEPTIONS 1

#define HAVE_VM_ALLOCATE 1
#define HAVE_VM_DEALLOCATE 1
#define HAVE_VM_PROTECT 1
#define HAVE_MACH_TASK_SELF 1
#define HAVE_MACH_VM_MAP_H 1
#endif

#ifdef HAVE_MMAP_VM
#define HAVE_SIGINFO_T 1

#define HAVE_MMAP 1
#define HAVE_MUNMAP 1
#define HAVE_MPROTECT 1
#define HAVE_SYS_MMAN_H 1
#if defined(__linux__)
#define HAVE_MMAP_ANON 1
#define HAVE_MMAP_ANONYMOUS 1
#endif
#endif
