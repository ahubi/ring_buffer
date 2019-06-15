#include "ring_buffer.h"
/**
 * ring_buffer_init - allocates a new rbuf using a preallocated buffer
 * @buffer: the preallocated buffer to be used.
 * @size: the size of the internal buffer, this have to be a power of 2.
 * @lock: the lock to be used to protect the rbuf buffer
 */
ring_buffer *ring_buffer_init(void *buffer, uint size) {
  int sts = 0;
  ring_buffer *rbuf;

  rbuf = (ring_buffer *)malloc(sizeof(ring_buffer));

  if (!rbuf) {
    LOG(0, "Failed to allocate memory for ring buffer struct");
    return NULL;
  }

  rbuf->buffer = buffer;
  rbuf->size = size;
  rbuf->in = rbuf->out = rbuf->count = 0;

  sts = pthread_mutex_init(&rbuf->mtx, NULL);

  if (sts != 0) {
    LOG(0, "Failed to init mutex (%s)", strerror(sts));
    free(rbuf);
    return NULL;
  }

  sts = pthread_cond_init(&rbuf->cnd, NULL);

  if (sts != 0) {
    LOG(0, "Failed to init condition (%s)", strerror(sts));
    free(rbuf);
    return NULL;
  }

  return rbuf;
}

/**
 * ring_buffer_alloc - allocates a new rbuf and its internal buffer
 * @size: the size of the internal buffer to be allocated.
 * @lock: the lock to be used to protect the rbuf buffer
 */
ring_buffer *ring_buffer_alloc(uint size) {
  void *buffer;
  ring_buffer *ret;

  buffer = malloc(size);

  if (!buffer) {
    LOG(0, "Failed to allocate memory of size: %d", size);
    return NULL;  // Need error handling here
  }

  ret = ring_buffer_init(buffer, size);

  if (!ret) free(buffer);

  return ret;
}

/**
 * ring_buffer_free - frees the rbuf
 * @rbuf: the rbuf to be freed.
 */
void ring_buffer_free(ring_buffer *rbuf) {
  free(rbuf->buffer);
  free(rbuf);
}

/**
 * ring_buffer_put - puts some data into the rbuf, no locking version
 * @rbuf: the rbuf to be used.
 * @buffer: the data to be added.
 * @len: the length of the data to be added.
 *
 * This function copies at most @len bytes from the @buffer into
 * the rbuf, old data will be overwritten, and returns the number of
 * bytes copied.
 */
unsigned int ring_buffer_put(ring_buffer *rbuf, void *buffer, uint len) {
  uint l;

  len = min(len, rbuf->size);

  /* first put the data starting from rbuf->in to buffer end */
  l = min(len, rbuf->size - rbuf->in);
  memcpy(rbuf->buffer + rbuf->in, buffer, l);

  /* then put the rest (if any) at the beginning of the buffer */
  memcpy(rbuf->buffer, buffer + l, len - l);

  // Move the in pointer, it will wrap at the end of the buffer
  rbuf->in = (rbuf->in + len) % rbuf->size;

  // Check overflow conditions in the buffer
  if (rbuf->count + len > rbuf->size) {
    // move 'out' index to handle overflow conditions
    rbuf->out = rbuf->in;
    rbuf->count = rbuf->size;

    LOG(0, "ring buffer overflow occured len: %d count: %d out: %d in: %d", len,
        rbuf->count, rbuf->out, rbuf->in);

  } else {
    rbuf->count = rbuf->count + len;
  }

  LOG(1, "ring_buffer_put len: %d count: %d out: %d in: %d", len, rbuf->count,
      rbuf->out, rbuf->in);

  // Wake up clients waiting on the buffer
  // BUFFER_BROADCAST(rbuffer);

  return len;
}

/**
 * ring_buffer_get - gets some data from the rbuf, no locking version
 * @rbuf: the rbuf to be used.
 * @buffer: where the data must be copied.
 * @len: the size of the destination buffer.
 *
 * This function copies at most @len bytes from the rbuf into the
 * @buffer and returns the number of copied bytes.
 */
uint ring_buffer_get(ring_buffer *rbuf, void *buffer, uint len) {
  uint l;

  // Calculate available bytes
  len = min(len, rbuf->count); 
  /* first get the data from rbuf->out until the end of the buffer */
  l = min(len, rbuf->size - rbuf->out);
  memcpy(buffer, rbuf->buffer + rbuf->out, l);

  /* then get the rest (if any) from the beginning of the buffer */
  memcpy(buffer + l, rbuf->buffer, len - l);

  rbuf->out = (rbuf->out + len) % rbuf->size;
  rbuf->count -= len;

  return len;
}
