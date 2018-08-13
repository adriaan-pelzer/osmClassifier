CFLAGS=-g -DDEBUG -Wall -fPIC -I/usr/local/include -I/usr/local/include/json-c
LIBS=-L/usr/local/lib -lNgramsLocations -lClassifier -lParseDir -lCorpusToNgrams -ljson-c

all: osmClassifier

test: osmClassifier
	rm -rf in > /dev/null 2>&1
	rm -rf out > /dev/null 2>&1
	mkdir in
	mkdir out
	node populateInfolder.js
	./osmClassifier -i in -o out -c test-classifier.ngt

osmClassifier: Makefile osmClassifier.o
	gcc ${CFLAGS} -o osmClassifier osmClassifier.o ${LIBS}

osmClassifier.o: Makefile osmClassifier.c
	gcc ${CFLAGS} -c osmClassifier.c -o osmClassifier.o

install:
	cp osmClassifier /usr/local/lib

clean:
	rm *.o; rm core*; rm osmClassifier
