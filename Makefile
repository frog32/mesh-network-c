#on Sun you need these
#EXTRA_LIBS=-lnsl -lsocket
EXTRA_LIBS=

BUILD_OPTS=-g

clean:
	rm -f *~ *.bak *.o

all: conn_server.c conn_server.h conn_io.c conn_io.h connection.c connection.h main.c Makefile
	llvm-gcc ${BUILD_OPTS} -c conn_server.c -o conn_server.o
	llvm-gcc ${BUILD_OPTS} -c connection.c -o connection.o
	llvm-gcc ${BUILD_OPTS} -c conn_io.c     -o conn_io.o
	llvm-gcc ${BUILD_OPTS} main.c conn_server.o connection.o conn_io.o ${EXTRA_LIBS} -o meshnode
