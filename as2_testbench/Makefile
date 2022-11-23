
PROGRAM=as2_mockup
TESTER=as2_testbench
CFLAGS=-O2 -g -Wall -pedantic

all: ${TESTER} ${PROGRAM}


.PHONY: test
test: all
	./${TESTER} ${PROGRAM}

.PHONY: clean
clean:
	rm -rf *.o *~ ${TESTER} ${PROGRAM}
