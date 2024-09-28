#if !defined(INCLUDED_KMST_STUFF)
#define INCLUDED_KMST_STUFF

#if defined(_WIN32)
  #define ON_PLATFORM_WIN32 1
#endif

#define _UNICODE
#include <assert.h>
#include <memory.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <uchar.h>
#include <wchar.h>

#if defined(ON_PLATFORM_WIN32)
  #include <tchar.h>

  #define UNICODE
  #include <Windows.h>

  #pragma comment(lib, "User32.lib")
#endif

/*****************************************************************************/

#define alignof(x) _Alignof(x)
#define alignas(x) _Alignas(x)

/*****************************************************************************/

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef u16 uints;
typedef u32 uint;
typedef u64 uintl;

typedef s16 sints;
typedef s32 sint;
typedef s64 sintl;

typedef u8 byte;
typedef u8 bit;

/*****************************************************************************/

inline uint get_maximum(uint a, uint b)
{
  return a >= b ? a : b;
}

/*****************************************************************************/

typedef char     utf8;
typedef char16_t utf16;
typedef char32_t utf32;

/*****************************************************************************/

typedef uintptr_t address;

typedef max_align_t universal_alignment_type;
enum { universal_alignment = alignof(universal_alignment_type) };

inline uint get_backward_alignment(address x, uint a)
{
  return a ? x & (a - 1) : 0;
}

inline uint get_forward_alignment(address x, uint a)
{
  uint m = get_backward_alignment(x, a);
  return m ? a - m : 0;
}

inline address align_forwards(address x, uint a)
{
  return x + get_forward_alignment(x, a);
}

/*****************************************************************************/

inline void copy_memory(void *destination, void *source, uint size)
{
  memcpy(destination, source, size);
}

inline void fill_memory(void *destination, uint size, byte value)
{
  memset(destination, value, size);
}

/*****************************************************************************/

#endif /* !defined(INCLUDED_KMST_STUFF) */
