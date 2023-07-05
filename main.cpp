#include "hyperion/chunk.h"
#include "hyperion/debug.h"

int main(int argc, char* argv[]) {
  Chunk chunk;
  create_chunk(&chunk);

  int constant = add_constant(&chunk, 3.14);
  write_chunk(&chunk, OP_CONSTANT, 123);
  write_chunk(&chunk, constant, 123);
  write_chunk(&chunk, OP_RETURN, 123);

  debug_chunk(&chunk, "test chunk");
  free_chunk(&chunk);
  return 0;
}

