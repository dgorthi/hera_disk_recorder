ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS     =

AM_CPPFLAGS += -I/usr/local/lib/
AM_CPPFLAGS += -I/usr/include/hdf5/serial/

# AM_CFLAGS is used for all C compiles
#AM_CFLAGS = -ggdb -fPIC -O3 -Wall -Werror -fno-strict-aliasing -mavx2
AM_CFLAGS = -fPIC -O3 -Wall -Werror -fno-strict-aliasing -mavx2 -march=native -funroll-loops -Wdate-time -g -O2 -Wformat -Wl, -Bsymbolic-functions

# Convenience variables to group source files
headers = paper_databuf.h   \
          hera_hdf5_header.h

threads = paper_fake_net_thread.c       \
	      paper_databuf.c               \
		  paper_strip_thread.c          \
		  hera_pktsock_thread.c         \
          paper_write_thread.c

# This is the paper_gpu plugin itself
lib_LTLIBRARIES = hera_disk_recorder.la
hera_disk_recorder_la_SOURCES  = $(headers) $(threads)
hera_disk_recorder_la_LIBADD   = -lrt -lpthread -lsz -lz -ldl -lm -lhashpipe
hera_disk_recorder_la_LDFLAGS  = -avoid-version -module -shared -export-dynamic
hera_disk_recorder_la_LDFLAGS += -L/usr/lib/x86_64-linux-gnu/hdf5/serial
hera_disk_recorder_la_LDFLAGS += -lhdf5_hl -lhdf5
#paper_gpu_la_LDFLAGS += -L"@HASHPIPE_LIBDIR@" -Wl,-rpath,"@HASHPIPE_LIBDIR@"

# Installed scripts
dist_bin_SCRIPTS = init.sh

# vi: set ts=8 noet :
