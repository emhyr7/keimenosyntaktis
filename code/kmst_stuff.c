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

#if !defined(__FUNCTION__)
  #if defined(__func__)
    #define __FUNCTION__ __func__
  #endif
#endif

/*****************************************************************************/

#define log_note(...)    printf("[note] " __VA_ARGS__)
#define log_error(...)   printf("[error] " __VA_ARGS__)
#define log_caution(...) printf("[caution] " __VA_ARGS__)

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

enum
{
  maximum_value_of_uint = ~(uint)0,
};

/*****************************************************************************/

inline uint get_maximum(uint a, uint b)
{
  return a >= b ? a : b;
}

/*****************************************************************************/

typedef char     utf8;
typedef char16_t utf16;
typedef char32_t utf32;

uint get_string_size(const char *string)
{
  return strlen(string);
}

utf8 *make_utf8_from_utf16(const utf16 *utf16_string, uint utf16_string_size);

/*****************************************************************************/

typedef uintptr_t address;

typedef max_align_t universal_alignment_type;
enum
{
  universal_alignment = alignof(universal_alignment_type),
  default_alignment   = alignof(void *),
};

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

inline void copy_memory(void *destination, const void *source, uint size)
{
  memcpy(destination, source, size);
}

inline void fill_memory(void *destination, uint size, byte value)
{
  memset(destination, value, size);
}

inline void move_memory(void *destination, const void *source, uint size)
{
  memmove(destination, source, size);
}

/*****************************************************************************/

enum
{
  kibibyte = 1024,
  mebibyte = kibibyte * kibibyte,

  memory_page_size = 4 * kibibyte,
};

uint mass_of_global_memory = 0;

void *allocate_memory(uint size)
{
  void *memory = malloc(size);
  mass_of_global_memory += size;
  return memory;
}

void *reallocate_memory(uint new_size, void *memory, uint size)
{
  memory = realloc(memory, new_size);
  mass_of_global_memory -= size;
  mass_of_global_memory += new_size;
  return memory;
}

void release_memory(void *memory, uint size)
{
  free(memory);
  mass_of_global_memory -= size;
}

/*****************************************************************************/

typedef struct chunk chunk;
struct chunk
{
  uint size;
  uint mass;
  byte *memory;
  chunk *prior;
  alignas(universal_alignment) byte tailing_memory[];
};

typedef struct
{
  chunk *chunk;
  uint minimum_chunk_size;
  bit enabled_nullify : 1;
} linear_allocator;

uint defualt_minimum_chunk_size_for_linear_allocator = 4096;

void *push_into_linear_allocator(uint size, uint alignment, linear_allocator *state)
{
  uint forward_alignment = state->chunk ? align_forwards((address)state->chunk->memory + state->chunk->mass, alignment) : 0; 
  if(!state->chunk || state->chunk->mass + forward_alignment + size > state->chunk->size)
  {
    state->minimum_chunk_size = get_maximum(defualt_minimum_chunk_size_for_linear_allocator, state->minimum_chunk_size);
    uint new_chunk_size = get_maximum(size, state->minimum_chunk_size);
    chunk *new_chunk = allocate_memory(sizeof(chunk) + new_chunk_size);
    new_chunk->size = new_chunk_size;
    new_chunk->mass = 0;
    new_chunk->memory = new_chunk->tailing_memory;
    new_chunk->prior = state->chunk;
    state->chunk = new_chunk;
  }
  forward_alignment = get_forward_alignment((address)state->chunk->memory + state->chunk->mass, alignment);
  void *memory = state->chunk->memory + state->chunk->mass + forward_alignment;
  if(state->enabled_nullify) fill_memory(memory, size, 0);
  state->chunk->mass += forward_alignment + size;
  return memory;
}

void pop_from_linear_allocator(uint size, uint alignment, linear_allocator *state)
{
  while(state->chunk && size)
  {
    uint mass = state->chunk->mass;
    size += get_backward_alignment((address)state->chunk->memory + mass, alignment);
    bool releasable_chunk = (sintl)mass - size <= 0;
    state->chunk->mass -= size;
    if(releasable_chunk)
    {
      chunk *releasable_chunk = state->chunk;
      state->chunk = state->chunk->prior;
      release_memory(releasable_chunk, releasable_chunk->size);
      size -= mass;
    }
    else size = 0;
  }
}

/*****************************************************************************/

typedef enum
{
  ALLOCATOR_linear = 0x01,
} allocator_type;

typedef void *push_procedure(uint size, uint alignment, void *state);
typedef void pop_procedure(uint size, uint alignment, void *state);

typedef struct
{
  allocator_type type;
  void *state;
  push_procedure *push;
  pop_procedure *pop;
} allocator;

thread_local linear_allocator state_of_default_allocator;

thread_local allocator default_allocator =
{
  .type  = ALLOCATOR_linear,
  .state = 0,
  .push  = (push_procedure *)&push_into_linear_allocator,
  .pop   = (pop_procedure *)&pop_from_linear_allocator,
};

/*****************************************************************************/

/* used for convenience so we don't have invoke every procedure with common
   arguments */
thread_local struct
{
  allocator *allocator;
} context;

inline void *push(uint size, uint alignment)
{
  allocator *allocator = context.allocator;
  return allocator->push(size, alignment, allocator->state);
}

#define push_type(type, count) (type *)push(count * sizeof(type), alignof(type))

inline void pop(uint size, uint alignment)
{
  allocator *allocator = context.allocator;
  allocator->pop(size, alignment, allocator->state);
}

#define pop_type(type, count) pop(count * sizeof(type), alignof(type))

/*****************************************************************************/

utf8 *make_utf8_from_utf16(const utf16 *utf16_string, uint utf16_string_size)
{
  s32 utf8_string_size = WideCharToMultiByte(CP_UTF8, 0, utf16_string, utf16_string_size, 0, 0, 0, 0);
  if(!utf8_string_size) return 0;
  utf8 *utf8_string = (utf8 *)push(utf8_string_size, universal_alignment);
  WideCharToMultiByte(CP_UTF8, 0, utf16_string, utf16_string_size, utf8_string, utf8_string_size, 0, 0);
  return utf8_string;
}

/*****************************************************************************/

#if defined(ON_PLATFORM_WIN32)
typedef HANDLE handle;
#endif

enum { maximum_size_for_path = MAX_PATH };

handle open_file(const char *file_path)
{
  OFSTRUCT of;
  HFILE file_handle = OpenFile(file_path, &of, OF_READWRITE);
  assert(file_handle != HFILE_ERROR);
  return (handle)file_handle;
}

uintl get_file_size(handle file_handle)
{
  LARGE_INTEGER large_integer;
  assert(GetFileSizeEx(file_handle, &large_integer));
  return large_integer.QuadPart;
}

uint read_from_file(void *buffer, uint buffer_size, handle file_handle)
{
  DWORD read_size;
  assert(ReadFile(file_handle, buffer, buffer_size, &read_size, 0));
  return read_size;
  
}

#endif /* !defined(INCLUDED_KMST_STUFF) */
