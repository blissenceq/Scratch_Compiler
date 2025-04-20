OBJECTS= ./build/compiler.o ./build/cprocess.o ./helpers/buffer.o ./helpers/vector.o
INCLUDES= -I./

all: ${OBJECTS}
	gcc main.c ${INCLUDES} ${OBJECTS} -g -o ./main

./build/compiler.o: ./compiler.c
	gcc ./compiler.c ${INCLUDES} -o ./build/compiler.o -g -c

./build/cprocess.o: ./cprocess.c
	gcc ./cprocess.c ${INCLUDES} -o ./build/cprocess.o -g -c

./helpers/buffer.o: ./buffer.c
	gcc ./buffer.c ${INCLUDES} -o ./helpers/buffer.o -g -c

./helpers/vector.o: ./vector.c
	gcc ./vector.c ${INCLUDES} -o ./helpers/vector.o -g -c

clean:
	rm ./main
	rm -rf ${OBJECTS}
