#ifndef _HERA_HDF5_HEADER_H
#define HERA_HDF5_HEADER_H

#include "hera_databuf.h"

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
   double *unix_time_array;

}hdf5_header_t;

#endif //_HERA_HDF5_HEADER_H
