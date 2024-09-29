#if !defined(INCLUDED_KMST)
#define INCLUDED_KMST

#include "kmst_stuff.c"

static utf8 *command_line;

typedef struct
{
  const utf8 *file_path;
  uints file_path_size;
  handle file_handle;
  uint text_size;
  uint text_capacity;
  byte *text;
} text_buffer;

void read_file_into_text_buffer(const utf8 *file_path, text_buffer *state)
{
  state->file_path = file_path;
  state->file_path_size = get_string_size(file_path);
  log_note("file_path: %.*s\n", state->file_path_size, state->file_path);
  log_note("file_path_size: %u\n", state->file_path_size);
  state->file_handle = open_file(file_path);
  {
    uintl file_size = get_file_size(state->file_handle);
    assert(file_size <= maximum_value_of_uint);
    state->text_size = file_size;
    log_note("text_size: %u\n", state->text_size);
  }
  state->text_capacity = align_forwards(state->text_size, memory_page_size);
  log_note("text_capacity: %u\n", state->text_capacity);
  state->text = (byte *)allocate_memory(state->text_capacity);
  read_from_file(state->text, state->text_size, state->file_handle);
}

void write_into_text_buffer(uint offset, const utf8 *text, uint size, text_buffer *state)
{
  if(state->text_size + size > state->text_capacity)
  {
    uint new_text_capacity = align_forwards(state->text_capacity + size, memory_page_size);
    state->text = reallocate_memory(new_text_capacity, state->text, state->text_capacity);
    state->text_capacity = new_text_capacity;
  }
  byte *pointer = state->text + offset;
  move_memory(pointer + size, pointer, state->text_size - offset);
  copy_memory(pointer, text, size);
  state->text_size += size;
}

void erase_from_text_buffer(uint offset, uint size, text_buffer *state)
{
  if(size > offset) offset = size;
  byte *pointer = state->text + offset;
  move_memory(pointer - size, pointer, state->text_size - offset);
  state->text_size -= size;
}

static inline void initialize_globals(void) /* for convenience */
{
  /* initialize context */
  default_allocator.state = &state_of_default_allocator;
  context.allocator = &default_allocator;

  /* get the command line*/  
  utf16 *utf16_command_line = GetCommandLine();
  uint utf16_command_line_size = wcslen(utf16_command_line);
  command_line = make_utf8_from_utf16(utf16_command_line, utf16_command_line_size);
}

int main(void)
{
  initialize_globals();
  log_note("maximum_size_for_path: %u\n", maximum_size_for_path);
  log_note("command_line: %s\n", command_line);

  text_buffer text_buffer;
  read_file_into_text_buffer("./test.txt", &text_buffer);
  printf("%.*s", text_buffer.text_size, text_buffer.text);

  uint offset = 0;
  for(;;)
  {
    bool ok = true;

    char cmd[32], arg[32];
    uint cmdsz=0, argsz=0;
    bool oncmd=1;
    for(char c = getchar(); c != '\n' && c != EOF; c = getchar()) {
      if(oncmd) {
        if(c == ' ') {
          oncmd = 0;
          continue;
        }
        cmd[cmdsz++] = c;
      } else arg[argsz++] = c;
    }
    cmd[cmdsz] = arg[argsz] = 0;

    if(!strcmp(cmd, "print")){
      log_note("print()\n");
      printf("%.*s\n", text_buffer.text_size, text_buffer.text);
    } else if(!strcmp(cmd, "write")) {
      log_note("write(%s)\n", arg);
      write_into_text_buffer(offset, arg, argsz, &text_buffer);
      offset += argsz;
    } else if(!strcmp(cmd, "erase")) {
      if(!offset) {
        log_note("offset is 0.\n");
        ok=0;
      } else {
        char *end;
        uint erasesz = strtoul(arg, &end, 10);
        log_note("erase(%u)\n", erasesz);
        erase_from_text_buffer(offset, erasesz, &text_buffer);
        offset -= erasesz;
      }
    } else if(!strcmp(cmd, "exit")) {
      break;
    } else {
      ok = false;
      log_note("unknown command: %s(%s)\n", cmd, arg);
    }
    cmdsz = argsz = 0;
  }
  
  return 0;
}

#endif /* !defined(INCLUDED_KMST) */
