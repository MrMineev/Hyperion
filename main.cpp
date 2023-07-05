#include "BytecodeChunk/chunk.h"
#include "BytecodeChunk/debug.h"

int main(int argc, char* argv[]) {
  Chunk chunk;
  create_chunk(&chunk);
  write_chunk(&chunk, OP_RETURN);

  debug_chunk(&chunk, "test chunk");
  free_chunk(&chunk);
  return 0;
}

