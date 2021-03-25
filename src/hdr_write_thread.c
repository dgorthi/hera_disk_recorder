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
#include "hdr_hdf5_header.h"
#include "hdr_databuf.h"

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
   //header->time_units = (char *)malloc(128, sizeof(char));
   strcpy(header->time_units, "millisec");

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
   H5_WRITE_HEADER_I64(group_id, "time_units", header->time_units, dims);
   H5Gclose(group_id);
}

void *hdr_write_thread_run(hashpipe_thread_args_t *args){
    hdr_stripper_databuf_t *idb = (hdr_stripper_databuf_t *)args->ibuf;
    hashpipe_status_t st = args->st;
    const char *status_key = args->thread_desc->skey;

    /*Main loop*/
    int rv;
    uint64_t mcnt = 0;
    uint64_t *data;
    int block_id;
    uint64_t nblks = N_BLOCK_PER_FILE;
    char filename[4096];
    struct timeval tv;
    uint64_t now;

    /* Datasets */
    hid_t h5file, h5data, h5time;
    /* Dataspaces */
    hid_t h5ds_data_file, h5ds_data_mem, h5ds_data_block, 
          h5ds_time, h5ds_time_entry;
    /* Dimensions */
    hsize_t mem_dim[] = {MEM_DIM};    hsize_t file_dim[] = {FILE_DIM};
    hsize_t time_dim[] = {TIME_DIM};  hsize_t block_dim[] = {BLOCK_DIM};
    hsize_t time_entry_dim[] = {1};

    /* The strategy for writing hdf5 files here is to build the entire file first
       and then replace a chunk of the file as each block arrives and ready to 
       be written. This is easy because once the size of the file is fixed, 
       the number of blocks are set and the dataset does not need to be dynamically
       expanded.
    */

    hsize_t dcnt[] = {DCNT};   // Number of blocks in each dimension
    hsize_t dstd[] = {DSTD};   // Don't skip any points 
    hsize_t dblk[] = {DBLK};   // Define block
 
    hsize_t tcnt[] = {TCNT};
    hsize_t tstd[] = {TSTD};
    hsize_t tblk[] = {TBLK};
    herr_t status;

    uint8_t *init_data;
    init_data = (uint8_t *)calloc(MEM_DIM,sizeof(uint8_t));

    uint64_t *times;
    times = (uint64_t *)calloc(N_TIME_PER_FILE, sizeof(uint64_t));

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
          printf("New file: %s\n\n",filename);
          hid_t h5file = H5Fcreate(filename, H5F_ACC_TRUNC, 
                                   H5P_DEFAULT, H5P_DEFAULT);
          hdf5_header_t *header = initialize_header();
          write_hdf5_header(header, h5file);

          h5ds_time      = H5Screate_simple(1,    time_dim, NULL);
          h5ds_data_mem  = H5Screate_simple(MEM_DATA_RANK,  mem_dim,  NULL); 
          h5ds_data_file = H5Screate_simple(FILE_DATA_RANK, file_dim, NULL);

          h5data = H5Dcreate(h5file, "data", H5T_STD_U8BE, h5ds_data_file,
                             H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
          status = H5Dwrite(h5data, H5T_NATIVE_UINT8, h5ds_data_mem, h5ds_data_file,
                            H5P_DEFAULT, init_data);

          h5time = H5Dcreate(h5file, "time", H5T_STD_U64BE, h5ds_time,
                             H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
          status = H5Dwrite(h5time, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, 
                            H5P_DEFAULT, times);

          printf("Wrote init dataset to file\n");

          status = H5Sclose(h5ds_data_file);
          status = H5Sclose(h5ds_data_mem);
          status = H5Sclose(h5ds_time);
          status = H5Dclose(h5data);
          status = H5Dclose(h5time);
          status = H5Fclose(h5file);      
          status = status;

          nblks = 0;
       }

       /*Wait for new block*/
       while ((rv=hdr_stripper_databuf_wait_filled(idb, block_id))!=HASHPIPE_OK){
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

      /*Reopen hdf5 file and write the received block of data.*/
      h5file = H5Fopen(filename, H5F_ACC_RDWR, H5P_DEFAULT);
      h5data = H5Dopen(h5file, "data", H5P_DEFAULT);
      h5time = H5Dopen(h5file, "time", H5P_DEFAULT);

      hsize_t doffset[4] = {0, 0, 0, nblks*N_TIME_PER_BLOCK};
      hsize_t toffset[] = {nblks};
      h5ds_data_block = H5Screate_simple(1, block_dim, NULL);
      h5ds_time_entry = H5Screate_simple(1, time_entry_dim, NULL);

      h5ds_data_file = H5Dget_space(h5data);
      status = H5Sselect_hyperslab(h5ds_data_file, H5S_SELECT_SET, 
                                   doffset, dstd, dcnt, dblk);

      h5ds_time = H5Dget_space(h5time);
      status = H5Sselect_hyperslab(h5ds_time, H5S_SELECT_SET, 
                                   toffset, tstd, tcnt, tblk);

      /*Copy data over*/
      data = (uint64_t *)idb->block[block_id].data;
      status = H5Dwrite(h5data, H5T_NATIVE_UINT8, h5ds_data_block, h5ds_data_file, 
                        H5P_DEFAULT, data);

      gettimeofday(&tv, NULL);
      now = (uint64_t)(tv.tv_sec*1000) + (uint64_t)(tv.tv_usec/1000);
      status = H5Dwrite(h5time, H5T_NATIVE_UINT64, h5ds_time_entry, h5ds_time, 
                        H5P_DEFAULT, &now);

      status = H5Sclose(h5ds_data_file);
      status = H5Sclose(h5ds_data_block);
      status = H5Sclose(h5ds_time_entry);
      status = H5Sclose(h5ds_time);
      status = H5Dclose(h5data);
      status = H5Dclose(h5time);
      status = H5Fclose(h5file);

      // Mark input block as free, output block as filled
      hdr_stripper_databuf_set_free(idb, block_id);

      // Setup for next block
      block_id = (block_id + 1)%idb->header.n_block;
      printf("Number of blocks: %ld\n",nblks);
      nblks++;

      /* Will exit if thread has been cancelled */
      pthread_testcancel();
    }

    // Thread success!
    return THREAD_OK;
}


hashpipe_thread_desc_t hdr_write_thread = {
    name: "hdr_write_thread",
    skey: "WRITESTAT",
    init: NULL,
    run: hdr_write_thread_run,
    ibuf_desc: {hdr_stripper_databuf_create},
    obuf_desc: {NULL} 
};

static __attribute__((constructor)) void ctor(){
    register_hashpipe_thread(&hdr_write_thread);
}
