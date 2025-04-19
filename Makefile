OBJECTS= ./build/compiler.o ./build/cprocess.o
INCLUDES= -I./

all: ${OBJECTS}
	gcc main.c ${INCLUDES} ${OBJECTS} -g -o ./main

main.o: ${OBJECTS}
	gcc main.c ${INCLUDES} ${OBJECTS} -g -c

./build/compiler.o: ./compiler.c
	gcc ./compiler.c ${INCLUDES} -o ./build/compiler.o -g -c

./build/cprocess.o: ./cprocess.c
	gcc ./cprocess.c ${INCLUDES} -o ./build/cprocess.o -g -c

clean:
	rm ./main
	rm -rf ${OBJECTS}
	rm ./main.o
