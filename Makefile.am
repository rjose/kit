bin_PROGRAMS=kit

kit_SOURCES=kit.c forth.l dictionary.c globals.c param.c stack.c entry.c \
            ec_basic.c return_stack.c ext_sequence.c ext_sqlite.c \
            ext_notes.c
kit_CFLAGS = -include allheads.h $(DEPS_CFLAGS) -Wall
kit_LDADD = $(DEPS_LIBS)

if HAVE_DOXYGEN
doc:
	doxygen doxygen.config
endif
