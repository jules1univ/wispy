#include "chunk_group.h"

w_chunkgroup *create_chunkgroup(unsigned int position) {
  if (position >= (UINT_MAX - CHUNK_GROUP_LEN)) {
    LOG("max chunk group position reached");
    return NULL;
  }

  w_chunkgroup *grp = malloc(sizeof(w_chunkgroup));
  if (grp == NULL) {
    LOG("failed to allocate memory for chunk group");
    return NULL;
  }

  grp->position = position - CHUNK_GROUP_MID_LEN;
  for (unsigned int x = 0; x < CHUNK_GROUP_LEN; x++) {
    grp->chunks[x] = create_chunk(grp->position + x, false);
  }

  LOG("creating chunk group: %u", grp->position);
  return grp;
}

void destroy_chunkgroup(w_chunkgroup *grp) {
  if (grp == NULL) {
    LOG("chunk group (null) already destroyed");
    return;
  }

  LOG("destroying chunk group: %u", grp->position);
  for (unsigned int i = 0; i < CHUNK_GROUP_LEN; i++) {
#if defined(WISPY_WINDOWS)
    if (grp->chunks[i]->handle != INVALID_HANDLE_VALUE) {
      WaitForSingleObject(grp->chunks[i]->handle, INFINITE);
    }
#elif defined(WISPY_LINUX)
    if (grp->chunks[i]->handle != 0) {
      pthread_join(grp->chunks[i]->handle, NULL);
    }
#endif

    free(grp->chunks[i]);
  }
  free(grp);
}

void next_chunkgroup(w_chunkgroup *grp) {
  grp->position += CHUNK_GROUP_MID_LEN;
  LOG("loading next chunk group: %d", grp->position);
  for (unsigned int x = 0; x < CHUNK_GROUP_MID_LEN; x++) {
    free(grp->chunks[x]);
    grp->chunks[x] = grp->chunks[x + CHUNK_GROUP_MID_LEN];
    grp->chunks[x + CHUNK_GROUP_MID_LEN] =
        create_chunk(grp->position + CHUNK_GROUP_MID_LEN + x, true);
  }
}

void prev_chunkgroup(w_chunkgroup *grp) {
  grp->position -= CHUNK_GROUP_MID_LEN;
  LOG("loading prev chunk group: %d", grp->position);
  for (unsigned int x = 0; x < CHUNK_GROUP_MID_LEN; x++) {
    free(grp->chunks[x + CHUNK_GROUP_MID_LEN]);
    grp->chunks[x + CHUNK_GROUP_MID_LEN] = grp->chunks[x];
    grp->chunks[x] = create_chunk(grp->position + x, true);
  }
}

int need_chunkgroup_update(w_chunkgroup *grp, unsigned int position) {

  if (grp->position != 0 &&
      position <= grp->position + CHUNK_GROUP_UPDATE_DIST) {
    return -1;
  } else if (position != UINT_MAX && position > grp->position +
                                                    CHUNK_GROUP_LEN -
                                                    CHUNK_GROUP_UPDATE_DIST) {
    return 1;
  }
  return 0;
}

w_chunk *get_chunkgroup_chunk(w_chunkgroup *grp, unsigned int position) {
  if (position < grp->position ||
      position > (grp->position + CHUNK_GROUP_LEN - 1)) {
    return NULL;
  }
  return grp->chunks[position - grp->position];
}
