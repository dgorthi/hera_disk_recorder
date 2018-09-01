ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS     =

AM_CPPFLAGS += -I/usr/local/lib/
AM_CPPFLAGS += -I/usr/include/hdf5/serial/

# AM_CFLAGS is used for all C compiles
#AM_CFLAGS = -ggdb -fPIC -O3 -Wall -Werror -fno-strict-aliasing -mavx2
AM_CFLAGS = -fPIC -O3 -Wall -Werror -fno-strict-aliasing -mavx2 -march=native -funroll-loops -Wdate-time -g -O2 -Wformat -Wl, -Bsymbolic-functions -Wl,-z,relro

# Convenience variables to group source files
headers = paper_databuf.h   \
          hera_hdf5_header.h

threads = paper_fake_net_thread.c       \
	      paper_databuf.c               \
		  paper_strip_thread.c          \
		  hera_pktsock_thread.c         \
          paper_write_thread.c

# This is the paper_gpu plugin itself
lib_LTLIBRARIES = paper_gpu.la
paper_gpu_la_SOURCES  = $(headers) $(threads)
paper_gpu_la_LIBADD   = -lrt -lpthread -lsz -lz -ldl -lm -lhashpipe
paper_gpu_la_LDFLAGS  = -avoid-version -module -shared -export-dynamic
paper_gpu_la_LDFLAGS += -L/usr/lib/x86_64-linux-gnu/hdf5/serial
paper_gpu_la_LDFLAGS += -lhdf5_hl -lhdf5
#paper_gpu_la_LDFLAGS += -L"@HASHPIPE_LIBDIR@" -Wl,-rpath,"@HASHPIPE_LIBDIR@"
#paper_gpu_la_LDFLAGS += -L"@XGPU_LIBDIR@" -Wl,-rpath,"@XGPU_LIBDIR@"
#paper_gpu_la_LDFLAGS += -L/usr/local/lib/libhashpipe.la -Wl,-rpath -Wl,/usr/lib/x86_64-linux-gnu/hdf5/serial

# Installed scripts
dist_bin_SCRIPTS = init.sh

## Non-installed test programs
#noinst_PROGRAMS = paper_fluff_test hera_write_hdf5
#paper_fluff_test_SOURCES = paper_fluff_test.c $(paper_fluff)
## The paper_fluff_test_CFLAGS is an autotools workaround.
## http://www.gnu.org/software/automake/manual/html_node/Objects-created-both-with-libtool-and-without.html
#paper_fluff_test_CFLAGS = $(AM_CFLAGS)
#paper_fluff_test_LDADD = -lrt

#paper_write_hdf5_SOURCES =  paper_write_hdf5 $(data_write)
## The hera_write_hdf5_CFLAGS is an autotools workaround.
## http://www.gnu.org/software/automake/manual/html_node/Objects-created-both-with-libtool-and-without.html
#paper_write_hdf5_CFLAGS = -O3 -Wall -Werror -fno-strict-aliasing -mavx2 -march=native -funroll-loops
#paper_write_hdf5_LDADD = -lrt -lpthread -lsz -lz -ldl -lm
#paper_write_hdf5_LDFLAGS = -L/usr/lib/x86_64-linux-gnu/hdf5/serial /usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5_hl.a /usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5.a

# vi: set ts=8 noet :
