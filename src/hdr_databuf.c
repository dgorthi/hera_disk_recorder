/* hdr_databuf.c
 *
 * Routines for creating and accessing main data transfer
 * buffer in shared memory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <time.h>

#ifdef DEBUG_SEMS
#include <pthread.h>
static struct timespec start;
static struct timespec now;
#define ELAPSED_NS(stop) \
  (((int64_t)stop.tv_sec-start.tv_sec)*1000*1000*1000+(stop.tv_nsec-start.tv_nsec))

#define SEMLOG(pd, msg)                                        \
  do {                                                         \
    int orig_errno = errno;                                    \
    clock_gettime(CLOCK_MONOTONIC, &now);                      \
    fprintf(stderr, "%13ld tid %lu " msg " %d (%lx)\n",        \
        ELAPSED_NS(now), pthread_self(), block_id,             \
        hashpipe_databuf_total_mask((hashpipe_databuf_t *)pd)); \
    errno = orig_errno;                                        \
  } while(0)

#else
#define SEMLOG(pd, msg)
#endif // DEBUG_SEMS

#include "hdr_databuf.h"

/*
 * Since the first element of hdr_input_databuf_t is a hashpipe_databuf_t, a
 * pointer to a hdr_input_databuf_t is also a pointer to a
 * hashpipe_databuf_t.  This allows a pointer to a hdr_input_databuf_t to be
 * passed, with appropriate casting, to functions that accept a pointer to a
 * hashpipe_databuf_t.  This allows the reuse of many of the functions in
 * hashpipe_databuf.c.  This is a form of inheritence: a hdr_input_databuf_t
 * is a hashpipe_databuf_t (plus additional specializations).
 *
 * For hashpipe_databuf.c function...
 *
 *   hashpipe_databuf_xyzzy(hashpipe_databuf_t *d...)
 *
 * ...a corresponding hdr_databuf.c function...
 *
 *   hdr_input_databuf_xyzzy(hdr_input_databuf_t *d...)
 *
 * ...can be created that passes its d parameter to hashpipe_databuf_xyzzy with
 * appropraite casting.  In some cases (e.g. hashpipe_databuf_attach), that's all
 * that's needed, but other cases may require additional functionality in the
 * hdr_input_buffer function.
 *
 * The same comments apply to hdr_output_databuf_t.
 */


/* -----------------
 *   INPUT BUFFERS
 * ----------------- 
 */

/*
 * Create, if needed, and attach to hdr_input_databuf shared memory.
 */
hashpipe_databuf_t *hdr_input_databuf_create(int instance_id, int databuf_id)
{
#ifdef DEBUG_SEMS
    // Init clock variables
    if(databuf_id==1) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        now.tv_sec = start.tv_sec;
        now.tv_nsec = start.tv_nsec;
    }
#endif

    /* Calc databuf sizes */
    size_t header_size = sizeof(hashpipe_databuf_t)
                       + sizeof(hashpipe_databuf_cache_alignment);
    size_t block_size  = sizeof(hdr_input_block_t);
    int    n_block = N_INPUT_BLOCKS + N_DEBUG_INPUT_BLOCKS;

    return hashpipe_databuf_create(
        instance_id, databuf_id, header_size, block_size, n_block);
}

int hdr_input_databuf_wait_free(hdr_input_databuf_t *d, int block_id)
{
    int rv;
    SEMLOG(d, "wait free");
    rv = hashpipe_databuf_wait_free((hashpipe_databuf_t *)d, block_id);
    SEMLOG(d, "got  free");
    return rv;
}

int hdr_input_databuf_busywait_free(hdr_input_databuf_t *d, int block_id)
{
    int rv;
    SEMLOG(d, "busy-wait free");
    rv = hashpipe_databuf_busywait_free((hashpipe_databuf_t *)d, block_id);
    SEMLOG(d, "busy-got  free");
    return rv;
}

int hdr_input_databuf_wait_filled(hdr_input_databuf_t *d, int block_id)
{
    int rv;
    SEMLOG(d, "wait fill");
    rv = hashpipe_databuf_wait_filled((hashpipe_databuf_t *)d, block_id);
    SEMLOG(d, "got  fill");
    return rv;
}

int hdr_input_databuf_busywait_filled(hdr_input_databuf_t *d, int block_id)
{
    int rv;
    SEMLOG(d, "busy-wait fill");
    rv = hashpipe_databuf_busywait_filled((hashpipe_databuf_t *)d, block_id);
    SEMLOG(d, "busy-got  fill");
    return rv;
}

int hdr_input_databuf_set_free(hdr_input_databuf_t *d, int block_id)
{
    SEMLOG(d, "set  free");
    return hashpipe_databuf_set_free((hashpipe_databuf_t *)d, block_id);
}

int hdr_input_databuf_set_filled(hdr_input_databuf_t *d, int block_id)
{
    SEMLOG(d, "set  fill");
    return hashpipe_databuf_set_filled((hashpipe_databuf_t *)d, block_id);
}

/* --------------------
 *   STRIPPER BUFFERS
 * --------------------
 */

/*
 * Create, if needed, and attach to hdr_stripper_databuf shared memory.
 */
hashpipe_databuf_t *hdr_stripper_databuf_create(int instance_id, int databuf_id)
{
#ifdef DEBUG_SEMS
    // Init clock variables
    if(databuf_id==1) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        now.tv_sec = start.tv_sec;
        now.tv_nsec = start.tv_nsec;
    }
#endif

    /* Calc databuf sizes */
    size_t header_size = sizeof(hashpipe_databuf_t)
                       + sizeof(hashpipe_databuf_cache_alignment);
    size_t block_size  = sizeof(hdr_stripper_block_t);
    int    n_block = N_INPUT_BLOCKS + N_DEBUG_INPUT_BLOCKS;

    return hashpipe_databuf_create(
        instance_id, databuf_id, header_size, block_size, n_block);
}

int hdr_stripper_databuf_wait_free(hdr_stripper_databuf_t *d, int block_id)
{
    int rv;
    SEMLOG(d, "wait free");
    rv = hashpipe_databuf_wait_free((hashpipe_databuf_t *)d, block_id);
    SEMLOG(d, "got  free");
    return rv;
}

int hdr_stripper_databuf_busywait_free(hdr_stripper_databuf_t *d, int block_id)
{
    int rv;
    SEMLOG(d, "busy-wait free");
    rv = hashpipe_databuf_busywait_free((hashpipe_databuf_t *)d, block_id);
    SEMLOG(d, "busy-got  free");
    return rv;
}

int hdr_stripper_databuf_wait_filled(hdr_stripper_databuf_t *d, int block_id)
{
    int rv;
    SEMLOG(d, "wait fill");
    rv = hashpipe_databuf_wait_filled((hashpipe_databuf_t *)d, block_id);
    SEMLOG(d, "got  fill");
    return rv;
}

int hdr_stripper_databuf_busywait_filled(hdr_stripper_databuf_t *d, int block_id)
{
    int rv;
    SEMLOG(d, "busy-wait fill");
    rv = hashpipe_databuf_busywait_filled((hashpipe_databuf_t *)d, block_id);
    SEMLOG(d, "busy-got  fill");
    return rv;
}

int hdr_stripper_databuf_set_free(hdr_stripper_databuf_t *d, int block_id)
{
    SEMLOG(d, "set  free");
    return hashpipe_databuf_set_free((hashpipe_databuf_t *)d, block_id);
}

int hdr_stripper_databuf_set_filled(hdr_stripper_databuf_t *d, int block_id)
{
    SEMLOG(d, "set  fill");
    return hashpipe_databuf_set_filled((hashpipe_databuf_t *)d, block_id);
}

