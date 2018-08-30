CC                  = gcc
HPDEMO_LIB_CCFLAGS  = -g -O3 -O2 -fPIC -shared -lstdc++ -mavx -msse4 \
                     -I. -I/usr/local/include -I/usr/include/hdf5/serial/\
                     -I/home/deepthi/anaconda3/include                   \
                     -L. -L/usr/local/lib -L/usr/lib/x86_64-linux-gnu/hdf5/serial \
                     -L/usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5_hl.a \
                     -L/usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5.a \
                     -L/home/deepthi/anaconda3/lib  \
                     -L/home/deepthi/anaconda3/lib/libhdf5_hl.a \
                     -L/home/deepthi/anaconda3/lib/libhdf5.a    \
                     -lhashpipe -lrt -lm -lpthread -lsz -lz -ldl
HPDEMO_LIB_TARGET   = hera_disk_recorder.so
HPDEMO_LIB_SOURCES  = paper_fake_net_thread.c \
                      paper_databuf.c         \
		              paper_strip_thread.c    \
		              paper_write_thread.c 
HPDEMO_LIB_INCLUDES = paper_databuf.h         \
                      hera_hdf5_header.h      \
                      config.h

all: $(HPDEMO_LIB_TARGET)

$(HPDEMO_LIB_TARGET): $(HPDEMO_LIB_SOURCES) $(HPDEMO_LIB_INCLUDES)
	$(CC) -o $(HPDEMO_LIB_TARGET) $(HPDEMO_LIB_SOURCES) $(HPDEMO_LIB_CCFLAGS)

tags:
	ctags -R .
clean:
	rm -f $(HPDEMO_LIB_TARGET) tags

prefix=/usr/local
LIBDIR=$(prefix)/lib
BINDIR=$(prefix)/bin
install-lib: $(HPDEMO_LIB_TARGET)
	mkdir -p "$(DESTDIR)$(LIBDIR)"
	install -p $^ "$(DESTDIR)$(LIBDIR)"
install: install-lib

.PHONY: all tags clean install install-lib
# vi: set ts=8 noet :
