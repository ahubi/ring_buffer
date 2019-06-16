#include "ring_buffer.h"
/**
 * ring_buffer_init - allocates a new rbuf using a preallocated buffer
 * @buffer: the preallocated buffer to be used.
 * @size: the size of the internal buffer, this have to be a power of 2.
 * @return: pointer to allocated ring buffer, otherwise NULL
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
 * @return: pointer to allocated ring buffer, otherwise NULL
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
  if (ring_buffer_lock(rbuf) == 0) {
    free(rbuf->buffer);
    free(rbuf);
  }
}

/**
 * ring_buffer_put - puts some data into the rbuf
 * @rbuf: the rbuf to be used.
 * @buffer: the data to be added.
 * @len: the length of the data to be added.
 *
 * This function copies at most @len bytes from the @buffer into
 * the @rbuf, old data might be overwritten, returns number of
 * bytes copied.
 */
uint ring_buffer_put(ring_buffer *rbuf, void *buffer, uint len) {
  uint l;
  if (ring_buffer_lock(rbuf) == 0) {
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

      LOG(0, "overflow occured len: %d count: %d out: %d in: %d", len,
          rbuf->count, rbuf->out, rbuf->in);

    } else {
      rbuf->count += len;
    }

    LOG(1, "len: %d count: %d out: %d in: %d", len, rbuf->count, rbuf->out,
        rbuf->in);

    if (ring_buffer_unlock(rbuf) != 0) LOG(0, "Failed to unlock ring_buffer");

    // Wake up clients waiting on the buffer
    if (ring_buffer_broadcast(rbuf) != 0)
      LOG(0, "Failed to broadcast ring_buffer");
    return len;
  }
  return 0;
}

/**
 * ring_buffer_get - gets some data from the rbuf
 * @rbuf: the rbuf to be used.
 * @buffer: destination buffer to copy data into
 * @len: the size of the destination buffer.
 *
 * This function copies at most @len bytes from the rbuf into the
 * @buffer and returns the number of copied bytes.
 */
uint ring_buffer_get(ring_buffer *rbuf, void *buffer, uint len) {
  uint l;
  if (ring_buffer_lock(rbuf) == 0) {
    // Wait till data is put into buffer
    if (rbuf->count <= 0) ring_buffer_wait(rbuf);

    // Calculate available bytes
    len = min(len, rbuf->count);

    /* first get the data from rbuf->out until the end of the buffer */
    l = min(len, rbuf->size - rbuf->out);
    memcpy(buffer, rbuf->buffer + rbuf->out, l);

    /* then get the rest (if any) from the beginning of the buffer */
    memcpy(buffer + l, rbuf->buffer, len - l);

    rbuf->out = (rbuf->out + len) % rbuf->size;
    rbuf->count -= len;
    if (ring_buffer_unlock(rbuf) != 0) LOG(0, "failed to unlock ring_buffer");
    return len;
  }
  return 0;
}
