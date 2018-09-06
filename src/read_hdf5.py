# Test script to read the hashpipeline written hdf5 files

import numpy as np
import h5py
import argparse
import matplotlib.pyplot as plt

parser = argparse.ArgumentParser(description='Read a hdf5 file written by hashpipeline.',
                                 formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument('filename',type=str,
                    help= 'Name of the input hdf5 file')
parser.add_argument('-t',dest='test', action='store_true', default=False,
                    help='Sampling frequency [MHz] used to collect data')
args = parser.parse_args()

with h5py.File(args.filename,'r') as fp:
    data = fp['data'][:]

print np.shape(data)

plt.plot(data[:,0,0,0])
plt.show()
