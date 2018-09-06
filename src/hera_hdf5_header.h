#ifndef _HERA_HDF5_HEADER_H
#define HERA_HDF5_HEADER_H

#include "paper_databuf.h"
#include <hdf5.h>
#include <time.h>

#define N_BLOCK_PER_FILE 32
#define N_TIME_PER_FILE  (N_BLOCK_PER_FILE * N_TIME_PER_BLOCK)

/* The strategy for writing hdf5 files here is to build the entire file first
   and then replace a chunk of the file as each block arrives and ready to 
   be written. This is easy because once the size of the file is fixed, 
   the number of blocks are set and the dataset does not need to be dynamically
   expanded.
*/

#define FILE_DATA_RANK   4
#define MEM_DATA_RANK    1

#define DIM0             N_ANTS
#define DIM1             2                   //pols
#define DIM2             N_STRP_CHANS_PER_X
#define DIM3             N_TIME_PER_FILE

#define DIM0_BLK         N_ANTS
#define DIM1_BLK         2
#define DIM2_BLK         N_STRP_CHANS_PER_X
#define DIM3_BLK         N_TIME_PER_BLOCK

#define MEM_DIM          (DIM0*DIM1*DIM2*DIM3)
#define TIME_DIM         N_BLOCK_PER_FILE
#define FILE_DIM         DIM0, DIM1, DIM2, DIM3
#define BLOCK_DIM        (DIM0_BLK* DIM1_BLK* DIM2_BLK* DIM3_BLK)

/* Parameters for hyperslab-bing the hdf5 file */
/* Data */
#define DCNT              1, 1, 1, 1         // Count- number of blocks in each dimension
#define DSTD              1, 1, 1, 1         // Stride- number of points to skip
// Block - unit of data in each dimension
#define DBLK              N_ANTS, \
                          2,      \
                          N_STRP_CHANS_PER_X, \
                          N_TIME_PER_BLOCK
/* Time */
#define TCNT              1
#define TSTD              1
#define TBLK              1               

#define H5_WRITE_HEADER_I64(group_id, h5_name, val, dims) \
  dataspace_id = H5Screate_simple(1, dims, NULL); \
  dataset_id = H5Dcreate2(group_id, h5_name, H5T_STD_I64LE, dataspace_id,\
                        H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT); \
  H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(val)); \
  H5Sclose(dataspace_id); \
  H5Dclose(dataset_id)

typedef struct hdf5_extra_keywords{
    uint64_t kwargs;
} hdf5_extra_keywords_t;

typedef struct hdf5_header{

   int64_t Nants;               // Number of antennas in each block
   int64_t Nants_data;          // Number of antennas with valid data
   int64_t Nfreqs;              // Number of frequency channels
   double  freq_array[N_STRP_CHANS_PER_X];  //Freq channel centers
   int64_t Npols;               // Number of polarizations
   int64_t Ntimes;              // Number of time samples
   int64_t ant_array[N_ANTS];    // Order of antenna numbers in data
   double channel_width;
   char time_units[64];

}hdf5_header_t;

#endif //_HERA_HDF5_HEADER_H
