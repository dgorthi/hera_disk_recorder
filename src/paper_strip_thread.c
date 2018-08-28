/*
 * paper_strip_thread.c
 *
 * This thread will remove all but 
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

static void *paper_strip_thread_run(hashpipe_thread_args_t * args){
    paper_input_databuf_t    *idb = (paper_input_databuf_t *)args->ibuf;  
    paper_stripper_databuf_t *odb = (paper_stripper_databuf_t *)args->obuf;
    hashpipe_status_t st = args->st;
    const char *status_key = args->thread_desc->skey;

    /* Main loop */
    int rv;              // store return of buffer status calls
    uint64_t mcnt = 0;   // mcnt of each block
    uint8_t *indata;     // typecast the data block into a char pointer
    uint8_t *outdata;    // to allow incrementing by a byte.
    uint64_t inoffset, outoffset;
    int m,a,p,c,t;
    int iblk = 0;
    int oblk = 0;

    while (run_threads()) {

        hashpipe_status_lock_safe(&st);
        hputi4(st.buf, "STRPBKIN", iblk);
        hputs(st.buf, status_key, "waiting");
        hputi4(st.buf, "STRPBKOUT", oblk);
        hputi8(st.buf, "STRPMCNT", mcnt);
        hashpipe_status_unlock_safe(&st);
        sleep(1);
 
        /* Wait for new block to be filled, then copy the
         * relevant data and clear it.
         */
        while ((rv=paper_input_databuf_wait_filled(idb, iblk))!= HASHPIPE_OK){
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

        fprintf(stderr, "Got new data!  in_blk:%d  out_blk:%d\n", iblk, oblk);
        /*Got new data! Copy into new buffer*/
        hashpipe_status_lock_safe(&st);
        hputs(st.buf, status_key, "stripping");
        hashpipe_status_unlock_safe(&st);
        mcnt = idb->block[iblk].header.mcnt;

        /* Cast data pointer to char to increment
           in 8 bits instead of 64 bits. */
        indata = (uint8_t *)idb->block[iblk].data;
        outdata = (uint8_t *)odb->block[oblk].data;

        fprintf(stderr,"Input shared mem loc:%p\n",indata);
        fprintf(stderr,"Output shared mem loc:%p\n",outdata);
        
        odb->block[oblk].header.good_data = 1;
        odb->block[oblk].header.mcnt = mcnt;

        for(m=0; m<Nm; m++){
          for(a=0; a<Na; a++){
            for(c=0; c<Nsc; c++){
              for(t=0; t<Nt; t++){
                for(p=0; p<Np; p++){
                  inoffset  = paper_input_databuf_data_idx8(m,a,p,c,t); 
                  outoffset = paper_stripper_databuf_data_idx8(m,a,p,c,t);
                  memcpy(outdata+outoffset, indata+inoffset, 1);
                }
              }
            }
          }
        }
        

        // Mark input block as free, output block as filled
        paper_stripper_databuf_set_filled(odb, oblk);
        paper_input_databuf_set_free(idb, iblk);

        // Setup for next block
        iblk = (iblk + 1)%idb->header.n_block;
        oblk = (oblk + 1)%odb->header.n_block;

        /* Will exit if thread has been cancelled */
        pthread_testcancel();
    }

    // Thread success!
    return THREAD_OK;
}


hashpipe_thread_desc_t paper_strip_thread = {
    name: "paper_strip_thread",
    skey: "STRPSTAT",
    init: NULL,
    run: paper_strip_thread_run,
    ibuf_desc: {paper_input_databuf_create},
    obuf_desc: {paper_stripper_databuf_create}
};

static __attribute__((constructor)) void ctor(){
    register_hashpipe_thread(&paper_strip_thread);
}
