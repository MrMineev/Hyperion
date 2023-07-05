#include "hyperion/chunk.h"
#include "hyperion/debug.h"
#include "hyperion/HVM.h"

int main(int argc, char* argv[]) {
  init_hvm();

  Chunk chunk;
  create_chunk(&chunk);

  int constant = add_constant(&chunk, 3.14);
  write_chunk(&chunk, OP_CONSTANT, 123);
  write_chunk(&chunk, constant, 123);
  write_chunk(&chunk, OP_NEGATE, 123);

  int constant2 = add_constant(&chunk, 6.28);
  write_chunk(&chunk, OP_CONSTANT, 123);
  write_chunk(&chunk, constant2, 123);

  write_chunk(&chunk, OP_ADD, 123);

  write_chunk(&chunk, OP_RETURN, 123);

  debug_chunk(&chunk, "test chunk");

  interpret(&chunk);
  free_hvm();

  free_chunk(&chunk);

  return 0;
}

