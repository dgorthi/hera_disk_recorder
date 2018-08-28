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

struct hera_hdf5_header *initialize_header(){
   int i;
   struct hera_hdf5_header *header;
   header = calloc(1, sizeof(hera_hdf5_header_t));

   header->Nants = N_ANTS;
   header->Nants_data = N_ANTS;
   header->Npols = 2;
   header->Nfreqs = N_STRP_CHANS_PER_X;
   header->channel_width = 250.0/8192.0;
   header->Ntimes = 131072;  // 32 per block* 4096 blocks

   for(i=0; i<N_ANTS; i++)
      header->ant_array[i] = i;
}

void write_hdf5_header(hera_hdf5_header_t *header, hid_t file_id){
   hid_t group_id, dataset_id, dataspace_id;
   hsize_t dims[4];

   group_id = H5Gcreate2(file_id, "header", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

   dims[0] = 1;
   H5_WRITE_HEADER_I64(group_id, "Nants", header->Nants, dims);
   H5_WRITE_HEADER_I64(group_id, "Nants_data", header->Nants_data, dims);
   H5_WRITE_HEADER_I64(group_id, "Nfreqs", header->Nfreqs, dims);
   H5_WRITE_HEADER_I64(group_id, "Ntimes", header->Ntimes, dims);
   H5_WRITE_HEADER_I64(group_id, "Npols", header->Npols, dims);
   H5_WRITE_HEADER_I64(group_id, "chan_width", header->channel_width, dims);
   H5Gclose(groud_id);
}

void *paper_write_thread_run(hashpipe_thread_args_t *args){
    paper_stripper_databuf_t *idb = (paper_stripper_databuf_t *)args->ibuf;
    hashpipe_status_t st = args->st;
    const char *status_key = args->thread_desc->skey;

    /*Main loop*/
    int rv;
    uint64_t mcnt = 0;
    uint64_t *data;
    int block_id;
    uint64_t nblks = 0;

    while (run_threads()){
       
       hashpipe_status_lock_safe(&st);
       hputi4(st.buf, "WRITEIN", block_id);
       hputs(st.buf, status_key, "waiting");
       hputi8(st.buf, "WRITEMCNT", mcnt);
       hashpipe_status_unlock_safe(&st);
       sleep(1);

       /*Check if you need to create a new file. Populate the header.*/
       if (nblks%20 ==0){
          filename = "/data/foo.hdf5"
          struct hdf5_header *header;
          header = initialize_header();
          
          hid_t file_id;
          herr_t status;

          file_id = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
          
          write_hdf5_header(header, file_id);

          status = H5Fclose(file_id);
          fprintf(stderr, "%d", (int)status);
       }
              

       /*Wait for new block*/
       while ((rv=paper_stripper_databuf_wait_filled(idb, block_id))!=HASHPIPE_OK){
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

      /*Reopen hdf5 file with filename in append mode and write more data.*/

    }

    

}
