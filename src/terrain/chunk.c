#include "chunk.h"

w_chunk *create_chunk(unsigned int position, bool thread) {
  w_chunk *chunk = malloc(sizeof(w_chunk));
  if (chunk == NULL) {
    LOG("failed to allocate chunk");
    return NULL;
  }
  chunk->position = position;

#if defined(WISPY_WINDOWS)
  chunk->handle = INVALID_HANDLE_VALUE;
#else
  chunk->handle = 0;
#endif

  if (thread) {
#if defined(WISPY_WINDOWS)
    chunk->handle = CreateThread(NULL, 0, &create_chunk_thread, chunk, 0, 0);
    if (chunk->handle == INVALID_HANDLE_VALUE)
#else
    if (pthread_create(&chunk->handle, NULL, &create_chunk_thread, chunk) != 0)
#endif
    {
      LOG("failed to create chunk thread");
      free(chunk);
      return NULL;
    }

    LOG("creating chunk thread: %u", position);
    return chunk;
  }

  create_chunk_thread(chunk);
  LOG("creating chunk: %u", position);
  return chunk;
}

#if defined(WISPY_WINDOWS)
DWORD WINAPI create_chunk_thread(PVOID arg)
#else
void *create_chunk_thread(void *arg)
#endif
{
  if (!arg) {
#if defined(WISPY_WINDOWS)
    return EXIT_FAILURE;
#else
    return NULL;
#endif
  }

  LOG("creating chunk");
  w_chunk *chunk = arg;

  Image base = GenImagePerlinNoise(CHUNK_W, BLOCK_TOP_LAYER_H,
                                   chunk->position * CHUNK_W, 0, 6.f);
  Image mineral =
      GenImageCellular(CHUNK_W, CHUNK_MID_H - BLOCK_TOP_LAYER_H, 10);

  for (unsigned int x = 0; x < CHUNK_W; x++) {
    unsigned int level = 0;
    bool firstLayer = false;

    for (unsigned int y = 0; y < CHUNK_H; y++) {
      w_blocktype type = BLOCK_AIR;
      if (y >= CHUNK_MID_H) {
        if (y < CHUNK_MID_H + BLOCK_TOP_LAYER_H) {
          unsigned int value = GetImageColor(base, x, (y - CHUNK_MID_H)).r;

          if (level == 0) {
            if (y > CHUNK_MID_H + BLOCK_TOP_LAYER_H / 3 || (value < 105)) {
              type = BLOCK_GRASS;
              level++;
            }
          } else if (level < 10) {
            type = BLOCK_DIRT;
            if (value < 40) {
              type = BLOCK_STONE;
            }
            level++;
          } else {
            type = BLOCK_STONE;
            if (value < 60) {
              type = BLOCK_MINERAL;
            }
          }
        } else {
          unsigned int value =
              GetImageColor(mineral, x, y - (CHUNK_MID_H + BLOCK_TOP_LAYER_H))
                  .r;
          type = BLOCK_MINERAL;
          if (!firstLayer) {
            type = BLOCK_STONE;
            if (value < 130 && value > 80) {
              firstLayer = true;
            }
          } else {
            if (value >= 70 && value <= 75) {
              type = BLOCK_MINERAL_IRON;
            } else if (value <= 20) {
              type = BLOCK_MINERAL_OR;
            }
          }
        }
      }

      chunk->blocks[y * CHUNK_W + x] =
          (w_block){.type = type, .is_background = false};
    }
  }

  UnloadImage(base);
  UnloadImage(mineral);

#if defined(WISPY_WINDOWS)
  chunk->handle = INVALID_HANDLE_VALUE;
#else
  chunk->handle = 0;
#endif

  LOG("chunk created: %u", chunk->position);

#if defined(WISPY_WINDOWS)
  return EXIT_SUCCESS;
#else
  return NULL;
#endif
}
