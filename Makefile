#on Sun you need these
#EXTRA_LIBS=-lnsl -lsocket
EXTRA_LIBS=

BUILD_OPTS=-g

clean:
	rm -f *~ *.bak

all: conn_client.c conn_client.h conn_server.c conn_server.h conn_io.c main.c Makefile
	llvm-gcc ${BUILD_OPTS} -c conn_client.c -o conn_client.o
	llvm-gcc ${BUILD_OPTS} -c conn_server.c -o conn_server.o
	llvm-gcc ${BUILD_OPTS} -c conn_io.c     -o conn_io.o
	llvm-gcc ${BUILD_OPTS} main.c conn_client.o conn_server.o conn_io.o ${EXTRA_LIBS} -o meshnode
