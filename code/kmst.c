#if !defined(INCLUDED_KMST)
#define INCLUDED_KMST

#include "kmst-stuff.c"

/*****************************************************************************/

uint mass_of_heap = 0;

void *acquire_memory(uint size)
{
  void *memory = malloc(size);
  mass_of_heap += size;
  return memory;
}

void release_memory(void *memory, uint size)
{
  free(memory);
  mass_of_heap -= size;
}

/*****************************************************************************/

typedef struct chunk chunk;
struct chunk
{
  uint size;
  uint mass;
  byte *memory;
  chunk *prior;
  alignas(universal_alignment) u8 tailing_memory[];
};

typedef struct
{
  chunk *chunk;
  uint minimum_chunk_size;
  bit enabled_nullify : 1;
} linear_allocator;

uint minimum_chunk_size_of_default_linear_allocator = 4096;

void *push_into_linear_allocator(uint size, uint alignment, linear_allocator *state)
{
  uint forward_alignment = state->chunk ? align_forwards((address)state->chunk->memory + state->chunk->mass, alignment) : 0; 
  if(!state->chunk || state->chunk->mass + forward_alignment + size > state->chunk->size)
  {
    state->minimum_chunk_size = get_maximum(minimum_chunk_size_of_default_linear_allocator, state->minimum_chunk_size);
    uint new_chunk_size = get_maximum(size, state->minimum_chunk_size);
    chunk *new_chunk = acquire_memory(sizeof(chunk) + new_chunk_size);
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

utf8 *get_utf8_from_utf16(const utf16 *utf16_string, uint utf16_string_size)
{
  s32 utf8_string_size = WideCharToMultiByte(CP_UTF8, 0, utf16_string, utf16_string_size, 0, 0, 0, 0);
  if(!utf8_string_size) return 0;
  utf8 *utf8_string = (utf8 *)push(utf8_string_size, universal_alignment);
  WideCharToMultiByte(CP_UTF8, 0, utf16_string, utf16_string_size, utf8_string, utf8_string_size, 0, 0);
  return utf8_string;
}

/*****************************************************************************/

static utf8 *command_line;

static inline void initialize_globals(void) /* for convenience */
{ 
  default_allocator.state = &state_of_default_allocator;
  context.allocator = &default_allocator;
  
  utf16 *utf16_command_line = GetCommandLine();
  uint utf16_command_line_size = wcslen(utf16_command_line);
  command_line = get_utf8_from_utf16(utf16_command_line, utf16_command_line_size);
}

int main(void)
{
  initialize_globals();
  
  printf(command_line);
  return 0;
}

#endif /* !defined(INCLUDED_KMST) */
