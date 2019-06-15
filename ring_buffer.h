#ifndef RING_BUFFER_H_INCLUDED
#define RING_BUFFER_H_INCLUDED

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define LOGLEVEL 5
#define LOG(level, fmt, args...)                                  \
  do {                                                            \
    if (level < LOGLEVEL)                                         \
      printf("rb: %s(%d) " fmt "\n", __func__, __LINE__, ##args); \
  } while (0)
#define min(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a < _b ? _a : _b;      \
  })
typedef unsigned int uint;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ring_buffer_ {
  void *buffer;         /* the buffer holding the data */
  uint size;            /* the size of the allocated buffer */
  uint in;              /* data is added at offset (in % size) */
  uint out;             /* data is extracted from off. (out % size) */
  uint count;           /* number of elements available in the buffer*/
  pthread_mutex_t mtx;  /* protects concurrent modifications */
  pthread_cond_t cnd;   /*Condition consumer threads wait on*/
} ring_buffer;

static inline int ring_buffer_lock(ring_buffer *rbuffer) {
  return pthread_mutex_lock(&rbuffer->mtx);
}

static inline int ring_buffer_unlock(ring_buffer *rbuffer) {
  return pthread_mutex_unlock(&rbuffer->mtx);
}

static inline int ring_buffer_wait(ring_buffer *rbuffer) {
  return pthread_cond_wait(&rbuffer->cnd, &rbuffer->mtx);
}

static inline int ring_buffer_broadcast(ring_buffer *rbuffer) {
  return pthread_cond_broadcast(&rbuffer->cnd);
}

// #define BUFFER_LOCK( buffer ) ring_buffer_lock(buffer)
// #define BUFFER_UNLOCK( buffer ) ring_buffer_unlock(buffer)
// #define BUFFER_WAIT( buffer ) ring_buffer_wait(buffer)
// #define BUFFER_BROADCAST( buffer ) ring_buffer_broadcast(buffer)

ring_buffer *ring_buffer_init(void *buffer, uint size);
ring_buffer *ring_buffer_alloc(unsigned int size);
void ring_buffer_free(ring_buffer *rbuffer);
uint ring_buffer_put(ring_buffer *rbuffer, void *buffer, uint len);
uint ring_buffer_get(ring_buffer *rbuffer, void *buffer, uint len);

/**
 * ring_buffer_reset - removes the entire rbuffer contents
 * @rbuffer: the rbuffer to be emptied.
 */
#define ring_buffer_reset(rbuffer) \
  ((rbuffer)->in = (rbuffer)->out = (rbuffer)->count = 0)

/**
 * ring_buffer_len - returns the number of bytes available in the rbuffer
 * @rbuffer: the rbuffer to be used.
 */
#define ring_buffer_len(rbuffer) ((rbuffer)->count)

#ifdef __cplusplus
}
#endif

#endif
