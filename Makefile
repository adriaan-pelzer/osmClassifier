CFLAGS=-g -DDEBUG -Wall -fPIC -I/usr/local/include
LIBS=-L/usr/local/lib -lNgramsLocations -lClassifier -lParseDir -lCorpusToNgrams

test: osmClassifier
	node populateInfolder.js
	./osmClassifier -d in -c test-classifier.ngt

all: osmClassifier

osmClassifier: Makefile osmClassifier.o
	gcc ${CFLAGS} -o osmClassifier osmClassifier.o ${LIBS}

osmClassifier.o: Makefile osmClassifier.c
	gcc ${CFLAGS} -c osmClassifier.c -o osmClassifier.o

install:
	cp osmClassifier /usr/local/lib

clean:
	rm *.o; rm core*; rm osmClassifier
