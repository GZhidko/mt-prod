AUTOMAKE_OPTIONS =  foreign subdir-objects

# Define the list of test programs
sbin_PROGRAMS = hashTableTestb natSrcEndAssign coreTestb coreTestNewb natSrcTestb uamConcurrencyTestb

#SUBDIRS = hashTableTestb

AM_CPPFLAGS = -I@top_srcdir@ -I@top_srcdir@/src

AM_CFLAGS = -g -fPIC
AM_YFLAGS = -d
hashTableTestb_LDADD = -lpthread -latomic

hashTableTestb_SOURCES = hashTableTest/hashTableTest.c ../src/lhash.c ../src/spinlock.c ../src/flog.c ../src/lhstat.c ../src/debug.c

included_objects = $(filter-out ../src/sweetspot.o ../src/sweetuam.o ../src/relay_outward.o ../src/flog.o, $(wildcard ../src/*.o))

natSrcEndAssign_LDADD = -lpthread -latomic $(included_objects)

natSrcEndAssign_SOURCES = natSrcEndpoinAsTest/natSrcEndpointAssiTest.c \
$(wildcard ../src/*.c)\
$(wildcard ../src/*.h)


coreTestb_LDADD = -lpthread -latomic $(included_objects)

coreTestb_SOURCES = coreTest/coreTest.c \
$(wildcard ../src/*.c)\
$(wildcard ../src/*.h)


coreTestNewb_LDADD = -lpthread -latomic $(included_objects)

coreTestNewb_SOURCES = coreTest/coreTest_new.c \
$(wildcard ../src/*.c)\
$(wildcard ../src/*.h)


natSrcTestb_LDADD = -lpthread -latomic $(included_objects)

natSrcTestb_SOURCES = natSrcTest/natSrcTest.c \
$(wildcard ../src/*.c)\
$(wildcard ../src/*.h)

uamConcurrencyTestb_LDADD = -lpthread -latomic $(included_objects)

uamConcurrencyTestb_SOURCES = uamConcurrencyTest/uamConcurrencyTest.c \
$(wildcard ../src/*.c)\
$(wildcard ../src/*.h)

clean-local:
	rm -f test.log
	rm -f coreTest/coreTest_new.log
	rm -f coreTest/detail_new.log
	rm -f uamConcurrencyTest/uamConcurrencyTest.log
	rm -f generated*.conf
	rm -f coreTest/generated*.conf
	rm -f uamConcurrencyTest/generated*.conf
	rm -f core
	rm -f core.*
	rm -f *.core
	rm -rf coreTest/filters_new
	rm -rf uamConcurrencyTest/filters

distclean-local: clean-local
