#ifndef _PAPER_DATABUF_H
#define _PAPER_DATABUF_H

#include <stdint.h>
#include "hashpipe_databuf.h"
#include "config.h"

// Xeng memory params
#define PAGE_SIZE       4096
#define CACHE_ALIGNMENT 128

// Determined by F engine
#define N_CHAN_TOTAL 6144
#define N_FENGINES   192              //64?
#define N_CHAN_PER_F N_CHAN_TOTAL

// Number of separate X-engines which deal with
// alternate time chunks
#define TIME_DEMUX 2

// Determined by F engine packetizer
#define N_INPUTS_PER_PACKET  6
#define N_CHAN_PER_PACKET    384
#define N_TIME_PER_PACKET    2
// N_BYTES_PER_PACKET excludes header!
#define N_BYTES_PER_PACKET  (N_INPUTS_PER_PACKET*N_CHAN_PER_PACKET*N_TIME_PER_PACKET)

// X engine sizing (from xGPU)
#define N_ANTS               192           //    XGPU_NSTATION
#define N_INPUTS          (2*192)          // (2*XGPU_NSTATION)
#define N_TIME_PER_BLOCK      32           //    XGPU_NTIME
#define N_CHAN_PER_X         384           //    XGPU_NFREQUENCY  (16 Xengs for each bank) 

#define N_BYTES_PER_BLOCK            (N_TIME_PER_BLOCK * N_CHAN_PER_X * N_INPUTS)
#define N_PACKETS_PER_BLOCK          (N_BYTES_PER_BLOCK / N_BYTES_PER_PACKET)
#define N_PACKETS_PER_BLOCK_PER_F    (N_PACKETS_PER_BLOCK * N_INPUTS_PER_PACKET / 2 / N_FENGINES)

// Validate packet dimensions
#if    N_BYTES_PER_PACKET != (N_TIME_PER_PACKET*N_CHAN_PER_PACKET*N_INPUTS_PER_PACKET)
#error N_BYTES_PER_PACKET != (N_TIME_PER_PACKET*N_CHAN_PER_PACKET*N_INPUTS_PER_PACKET)
#endif

// The HERA correlator is based largely on the PAPER correlator.  The main 
// difference will be in the F engines.  The ROACH2 based F engines are being
// replaced by SNAP based F engines.  Because SNAPs have a different number of
// inputs, the F engine packet format must change. 
// Even though the X engines are the same, the network thread and the
// fluffing threads for HERA are somewhat different.  The data buffer format
// for the GPU thread's input buffer is exactly the same.  The fluffing
// thread's input buffer is therefore sized the same (i.e. half the size of the
// GPU input buffer), but the format is different since the packets stored by
// the network thread will be different size/format.  Also changed is the
// number of time samples per packet i(and therefore per mcount).

/*
 * INPUT BUFFER STRUCTURES
 */

// For HERA:
//
// * HERA net thread output
// * HERA fluffer thread input
// * Ordered sequence of packets in "data" field:
//
//   +-- mcount
//   |       +-- Antenna
//   |       |       |<-packets->|
//   V       V       |           |
//   m       a       |c        t |
//   ==      ==      |==       ==|
//   m0 }--> a0 }--> |c0 }---> t0|\.
//   m1      a1      |c1       t1| \.
//   m2      a2      |c2       t2|  \ chan 0
//   :       :       |:        : |  / packet
//   :       :       |:        : | /.
//   :       :       |:        : |/.
//   :       :       +---- ------+
//   :       :       |Nc/2 }-> t0|\.
//   :       :       |:        t1| \.
//   :       :       |:        t2|  \ chan Nc/2
//   :       :       |:        : |  / packet
//   :       :       |:        : | /.
//   :       :       |:        : |/.
//   ==      ==       ===      ==
//   Nm      Na       Nc       Nt
//
//   Each time sample is 2 bytes comprising two 4b+4b complex values (for 2 pols).
//   Note that the bits may not all be contiguous to facilitate fluffing.
//
// m = packet's mcount - block's first mcount
// a = packet's antenna number
// c = channel within within X engine
// t = time sample within channel (0 to Nt-1)

#define Nm (N_TIME_PER_BLOCK/N_TIME_PER_PACKET)
#define Na N_ANTS
#define Nc N_CHAN_PER_X
#define Nt N_TIME_PER_PACKET
// Number of pols = 2
#define Np 2
// Number of complex elements = 2
#define Nx 2

// Computes hdr_input_databuf_t data word (uint64_t) offset for complex data
// word corresponding to the given parameters for HERA F engine packets.

//#define hdr_input_databuf_data_idx(m,a,c,t) \ //foo
//  (((((m * Na + a) * (Nc) + c)*Nt + t) * 2) / sizeof(uint64_t))
#define hdr_input_databuf_data_idx(m,a,c,t) \
  ((((m) * Na*Nc*Nt*Np) + ((a) * Nc*Nt*Np) + ((c)*Nt*Np) + ((t)*Np)) / sizeof(uint64_t))

#define hdr_input_databuf_data_idx8(m,a,p,c,t) \
  (((m) * Na*Nc*Nt*Np) + ((a) * Nc*Nt*Np) + ((c)*Nt*Np) + ((t)*Np) + (p))

#define hdr_input_databuf_data_idx256(m,a,c,t) \
  ((((((m) * Na) + (a))*Nc + (c))*Nt + (t))*Np / sizeof(__m256i))
  //((((m) * Na*Nc*Nt*Np) + ((a) * Nc*Nt*Np) + ((c)*Nt*Np) + ((t)*Np)) / sizeof(__m256i))
//TODO  (((((m * Na + a) * (Nc) + c)*Nt + t) * N_INPUTS_PER_PACKET) / sizeof(uint64_t))


#define N_INPUT_BLOCKS 4
#ifndef N_DEBUG_INPUT_BLOCKS
#define N_DEBUG_INPUT_BLOCKS 0
#endif

typedef struct hdr_input_header {
  int64_t good_data; // functions as a boolean, 64 bit to maintain word alignment
  uint64_t mcnt;     // mcount of first packet
} hdr_input_header_t;

typedef uint8_t hdr_input_header_cache_alignment[
  CACHE_ALIGNMENT - (sizeof(hdr_input_header_t)%CACHE_ALIGNMENT)
];

typedef struct hdr_input_block {
  hdr_input_header_t header;
  hdr_input_header_cache_alignment padding; // Maintain cache alignment
  uint64_t data[N_BYTES_PER_BLOCK/sizeof(uint64_t)];
} hdr_input_block_t;

// Used to pad after hashpipe_databuf_t to maintain cache alignment
typedef uint8_t hashpipe_databuf_cache_alignment[
  CACHE_ALIGNMENT - (sizeof(hashpipe_databuf_t)%CACHE_ALIGNMENT)
];

typedef struct hdr_input_databuf {
  hashpipe_databuf_t header;
  hashpipe_databuf_cache_alignment padding; // Maintain cache alignment
  hdr_input_block_t block[N_INPUT_BLOCKS+N_DEBUG_INPUT_BLOCKS];
} hdr_input_databuf_t;


/*
 * INPUT BUFFER FUNCTIONS
 */

hashpipe_databuf_t *hdr_input_databuf_create(int instance_id, int databuf_id);

static inline hdr_input_databuf_t *hdr_input_databuf_attach(int instance_id, int databuf_id)
{
    return (hdr_input_databuf_t *)hashpipe_databuf_attach(instance_id, databuf_id);
}

static inline int hdr_input_databuf_detach(hdr_input_databuf_t *d)
{
    return hashpipe_databuf_detach((hashpipe_databuf_t *)d);
}

static inline void hdr_input_databuf_clear(hdr_input_databuf_t *d)
{
    hashpipe_databuf_clear((hashpipe_databuf_t *)d);
}

static inline int hdr_input_databuf_block_status(hdr_input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_block_status((hashpipe_databuf_t *)d, block_id);
}

static inline int hdr_input_databuf_total_status(hdr_input_databuf_t *d)
{
    return hashpipe_databuf_total_status((hashpipe_databuf_t *)d);
}


int hdr_input_databuf_wait_free(hdr_input_databuf_t *d, int block_id);

int hdr_input_databuf_busywait_free(hdr_input_databuf_t *d, int block_id);

int hdr_input_databuf_wait_filled(hdr_input_databuf_t *d, int block_id);

int hdr_input_databuf_busywait_filled(hdr_input_databuf_t *d, int block_id);

int hdr_input_databuf_set_free(hdr_input_databuf_t *d, int block_id);

int hdr_input_databuf_set_filled(hdr_input_databuf_t *d, int block_id);



/*
 *  STRIPPED DATA BUFFER STRUCTURES
 */


// * HERA stripper thread output
// * HERA writer thread input
// * Ordered sequence of packets in "data" field:
//
//   +-- Antenna
//   |       +-- Pol
//   |       |       +-- Channel
//   V       V       |          
//   a       p       c        m -- mcnt 
//   ==      ==      ==       ==
//   a0 }--> p0 }--> c0 }---> m0 }--> t0
//   a1      p1      c1       m1      t1
//   a2      p2      c2       m2 
//   :       :       :        :  
//   :       :       :        :  
//   :       :       :        : 
//   :       :       :        :
//   :       :       :        : 
//   :       :       :        : 
//   :       :       :        :
//   :       :       :        :
//   :       :       :        :
//   :       :       :        : 
//   ==      ==      ===      ==     ==
//   Np*Na   Np      Nsc      Nm     Nt
//
// m = mcount - block's first mcount
// a = antenna number
// c = channel
// t = time sample within channel (0 to Nt-1)

#define N_STRP_CHANS_PER_X        8
#define Nsc                   N_STRP_CHANS_PER_X
#define N_STRP_BLOCKS            4
#ifndef N_DEBUG_STRP_BLOCKS
#define N_DEBUG_STRP_BLOCKS      0
#endif

#define N_BYTES_PER_STRP_BLOCK    (N_TIME_PER_BLOCK*N_STRP_CHANS_PER_X*N_INPUTS)

/* The difference between hdr_input_databuf and this buffer is 
 * the ordering of data in the data field and the reduced number of 
 * channels i.e, the indexing of the data will be different. 
 */

#define hdr_stripper_databuf_data_idx(m,a,p,c,t) \
  ((((a)*Nsc*Nm*Nt*Np) + ((p)*Nsc*Nm*Nt) + ((c)*Nm*Nt) + ((m)*Nt) + (t))/ sizeof(uint64_t))

#define hdr_stripper_databuf_data_idx8(m,a,p,c,t) \
  (((a)*Nsc*Nm*Nt*Np) + ((p)*Nsc*Nm*Nt) + ((c)*Nm*Nt) + ((m)*Nt) + (t))
  
#define hdr_stripper_databuf_data_idx256(m,a,p,c,t) \
  ((((a)*Nsc*Nm*Nt*Np) + ((p)*Nsc*Nm*Nt) + ((c)*Nm*Nt) + ((m)*Nt) + (t))/ sizeof(__m256i))

typedef struct hdr_stripper_header{
   int64_t good_data;  // boolean
   uint64_t mcnt;      //mcount of the first packet
} hdr_stripper_header_t;

typedef uint8_t hdr_stripper_header_cache_alignment[
    CACHE_ALIGNMENT - (sizeof(hdr_stripper_header_t)%CACHE_ALIGNMENT)
];

typedef struct hdr_stripper_block{
    hdr_stripper_header_t header;
    hdr_stripper_header_cache_alignment padding;
    uint64_t data[N_BYTES_PER_STRP_BLOCK/(sizeof(uint64_t))];
} hdr_stripper_block_t;

typedef uint8_t hdr_stripper_block_cache_alignment[
   CACHE_ALIGNMENT - (sizeof(hdr_stripper_block_t)%CACHE_ALIGNMENT)
];

typedef struct hdr_stripper_databuf{
  hashpipe_databuf_t header;
  hashpipe_databuf_cache_alignment padding;
  hdr_stripper_block_t block[N_STRP_BLOCKS + N_DEBUG_STRP_BLOCKS];
} hdr_stripper_databuf_t;

/*
 * STRIPPED BUFFER FUNCTIONS
 */

hashpipe_databuf_t *hdr_stripper_databuf_create(int instance_id, int databuf_id);

static inline hdr_stripper_databuf_t *hdr_stripper_databuf_attach(int instance_id, int databuf_id)
{
    return (hdr_stripper_databuf_t *)hashpipe_databuf_attach(instance_id, databuf_id);
}

static inline int hdr_stripper_databuf_detach(hdr_stripper_databuf_t *d)
{
    return hashpipe_databuf_detach((hashpipe_databuf_t *)d);
}

static inline void hdr_stripper_databuf_clear(hdr_stripper_databuf_t *d)
{
    hashpipe_databuf_clear((hashpipe_databuf_t *)d);
}

static inline int hdr_stripper_databuf_block_status(hdr_stripper_databuf_t *d, int block_id)
{
    return hashpipe_databuf_block_status((hashpipe_databuf_t *)d, block_id);
}

static inline int hdr_stripper_databuf_total_status(hdr_stripper_databuf_t *d)
{
    return hashpipe_databuf_total_status((hashpipe_databuf_t *)d);
}


int hdr_stripper_databuf_wait_free(hdr_stripper_databuf_t *d, int block_id);

int hdr_stripper_databuf_busywait_free(hdr_stripper_databuf_t *d, int block_id);

int hdr_stripper_databuf_wait_filled(hdr_stripper_databuf_t *d, int block_id);

int hdr_stripper_databuf_busywait_filled(hdr_stripper_databuf_t *d, int block_id);

int hdr_stripper_databuf_set_free(hdr_stripper_databuf_t *d, int block_id);

int hdr_stripper_databuf_set_filled(hdr_stripper_databuf_t *d, int block_id);

#endif // _PAPER_DATABUF_H
