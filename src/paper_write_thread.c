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
#include "hera_hdf5_header.h"
#include "paper_databuf.h"

struct hdf5_header *initialize_header(){
   int i;
   struct hdf5_header *header;
   header = calloc(1, sizeof(hdf5_header_t));

   header->Nants = N_ANTS;
   header->Nants_data = N_ANTS;
   header->Npols = 2;
   header->Nfreqs = N_STRP_CHANS_PER_X;
   header->channel_width = 250.0/8192.0;
   header->Ntimes = 131072;  // 32 per block* 4096 blocks

   for(i=0; i<N_ANTS; i++)
      header->ant_array[i] = i;

   return header;
}

void write_hdf5_header(hdf5_header_t *header, hid_t file_id){
   hid_t group_id, dataset_id, dataspace_id;
   hsize_t dims[1];

   group_id = H5Gcreate2(file_id, "header", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

   dims[0] = 1;
   H5_WRITE_HEADER_I64(group_id, "Nants", header->Nants, dims);
   H5_WRITE_HEADER_I64(group_id, "Nants_data", header->Nants_data, dims);
   H5_WRITE_HEADER_I64(group_id, "Nfreqs", header->Nfreqs, dims);
   H5_WRITE_HEADER_I64(group_id, "Ntimes", header->Ntimes, dims);
   H5_WRITE_HEADER_I64(group_id, "Npols", header->Npols, dims);
   H5_WRITE_HEADER_I64(group_id, "chan_width", header->channel_width, dims);
   H5Gclose(group_id);
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
    uint64_t nblks = N_BLOCK_PER_FILE;
    char filename[4096];

    hid_t file_id, dataset_id, ds_file_id, ds_mem_id, ds_block_id;
    hsize_t mem_dims[1] = {(DIM0*DIM1*DIM2*DIM3)};
    
    hsize_t dims[4] = {DIM0, DIM1, DIM2, DIM3};
    hsize_t dimsm[1] = {(DIM0_SUB*DIM1_SUB*DIM2_SUB*DIM3_SUB)};
    herr_t status;
    hsize_t count[] = {1,1,1,1};
    hsize_t stride[] = {1,1,1,1};
    hsize_t block[] = {N_ANTS, 2, N_STRP_CHANS_PER_X, N_TIME_PER_BLOCK};
    uint8_t *init_data;

    init_data = (uint8_t *)calloc(DIM0*DIM1*DIM2*DIM3,sizeof(uint8_t));
    ds_mem_id = H5Screate_simple(1, mem_dims, NULL); 
    printf("Bytes per block: %d\n", N_BYTES_PER_STRP_BLOCK);
    printf("Blocks per file: %d\n", N_BLOCK_PER_FILE);
    printf("Bytes per file: %lu\n", sizeof(init_data));

    ds_file_id = H5Screate_simple(RANK, dims, NULL);

    while (run_threads()){
       
       hashpipe_status_lock_safe(&st);
       hputi4(st.buf, "WRITEIN", block_id);
       hputs(st.buf, status_key, "waiting");
       hputi8(st.buf, "WRITEMCNT", mcnt);
       hashpipe_status_unlock_safe(&st);
       sleep(1);

       /*Create a new file. Populate the header.*/
       if (nblks == N_BLOCK_PER_FILE){

          /*Create a hdf5 file, set header, initialize a dataset and wait for data*/
          sprintf(filename, "hera_volt_data_%lu.h5", (unsigned long)time(NULL));
          file_id = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

          hdf5_header_t *header = initialize_header();
          write_hdf5_header(header, file_id);

          dataset_id = H5Dcreate(file_id, "data", H5T_STD_U8BE, ds_file_id,
                                 H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

          status = H5Dwrite(dataset_id, H5T_NATIVE_UINT8, ds_mem_id, ds_file_id,
                            H5P_DEFAULT, init_data);

          printf("Wrote init dataset to file\n");

          status = H5Sclose(ds_file_id);
          status = H5Sclose(ds_mem_id);
          status = H5Dclose(dataset_id);
          status = H5Fclose(file_id);      
          status = status;

          nblks = 0;
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

      /*Reopen hdf5 file with filename and rewrite the received block of data.*/
      file_id = H5Fopen(filename, H5F_ACC_RDWR, H5P_DEFAULT);
      dataset_id = H5Dopen(file_id, "data", H5P_DEFAULT);

      hsize_t offset[4] = {0, 0, 0, nblks*N_TIME_PER_BLOCK};
      ds_block_id = H5Screate_simple(1, dimsm, NULL);

      ds_file_id = H5Dget_space(dataset_id);
      status = H5Sselect_hyperslab(ds_file_id, H5S_SELECT_SET, offset,
                                   stride, count, block);

      /*Copy data over*/
      data = (uint64_t *)idb->block[block_id].data;
      status = H5Dwrite(dataset_id, H5T_NATIVE_UINT8, ds_block_id, ds_file_id, 
                        H5P_DEFAULT, data);

      status = H5Sclose(ds_file_id);
      status = H5Sclose(ds_block_id);
      status = H5Dclose(dataset_id);
      status = H5Fclose(file_id);

      // Mark input block as free, output block as filled
      paper_stripper_databuf_set_free(idb, block_id);

      // Setup for next block
      block_id = (block_id + 1)%idb->header.n_block;
      nblks++;

      /* Will exit if thread has been cancelled */
      pthread_testcancel();
    }

    // Thread success!
    return THREAD_OK;
}


hashpipe_thread_desc_t paper_write_thread = {
    name: "paper_write_thread",
    skey: "WRITESTAT",
    init: NULL,
    run: paper_write_thread_run,
    ibuf_desc: {paper_stripper_databuf_create},
    obuf_desc: {NULL} 
};

static __attribute__((constructor)) void ctor(){
    register_hashpipe_thread(&paper_write_thread);
}
