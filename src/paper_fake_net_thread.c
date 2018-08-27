/*
 * paper_fake_net_thread.c
 *
 * Routine to write fake data into shared memory blocks.  This allows the
 * processing pipelines to be tested without the network portion of PAPER.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>

#include "hashpipe.h"
#include "paper_databuf.h"

static void *fake_thread_run(hashpipe_thread_args_t * args){
    paper_input_databuf_t *db = (paper_input_databuf_t *)args->obuf;
    hashpipe_status_t st = args->st;
    const char *status_key = args->thread_desc->skey;

    /* Main loop */
    int rv;              // store return of buffer status calls
    uint64_t mcnt = 0;   // mcnt of each block
    uint64_t *data;
    int m,a,p,c,t;
    int block_idx = 0;
    uint64_t fake_data = 0xaa;

    while (run_threads()) {

        hashpipe_status_lock_safe(&st);
        hputs(st.buf, status_key, "waiting");
        hputi4(st.buf, "FAKEBKOUT", block_idx);
        hputi8(st.buf, "FAKEMCNT", mcnt);
        hashpipe_status_unlock_safe(&st);
        sleep(1);
 
        /* Wait for new block to be free, then clear it
         * if necessary and fill its header with new values.
         */
        while ((rv=paper_input_databuf_wait_free(db, block_idx))!= HASHPIPE_OK){
            if (rv==HASHPIPE_TIMEOUT){
                hashpipe_status_lock_safe(&st);
                hputs(st.buf, status_key, "blocked");
                hashpipe_status_unlock_safe(&st);
                continue;
            }else{
                hashpipe_error(__FUNCTION__, "error waiting for free databuf");
                pthread_exit(NULL);
                break;
            }
        }

        hashpipe_status_lock_safe(&st);
        hputs(st.buf, status_key, "receiving");
        hashpipe_status_unlock_safe(&st);

        // Set block header
        db->block[block_idx].header.good_data = 1;
        db->block[block_idx].header.mcnt = mcnt;
        mcnt+=Nm;

        // Set all block data to zero
        data = db->block[block_idx].data;
        memset(data, 0, N_BYTES_PER_BLOCK);

        for(m=0; m<Nm; m++){
           for(a=0; a<Na; a++){
              for(c=0; c<Nc; c++){
                 for(t=0; t<Nt; t++){
                    for(p=0; p<Np; p++){
                       //fprintf(stderr,"mcnt:%d,  ant:%d,  chan:%d,  t:%d,   pol:%d\n",m,a,c,t,p);
                       fake_data = (c%256) + a;
                       memcpy(&data[paper_input_databuf_data_idx(m,a,c,t)], &fake_data, 8);
                    }
                 }
              }
           }
        }
 
        // Mark block as full
        paper_input_databuf_set_filled(db, block_idx);

        // Setup for next block
        block_idx = (block_idx + 1) % db->header.n_block;

        /* Will exit if thread has been cancelled */
        pthread_testcancel();
    }

    // Thread success!
    return THREAD_OK;
}

static hashpipe_thread_desc_t fake_net_thread = {
    name: "paper_fake_net_thread",
    skey: "FAKESTAT",
    init: NULL,
    run:  fake_thread_run,
    ibuf_desc: {NULL},
    obuf_desc: {paper_input_databuf_create}
};

static __attribute__((constructor)) void ctor(){
  register_hashpipe_thread(&fake_net_thread);
}
