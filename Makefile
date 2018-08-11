CFLAGS=-g -Wall -fPIC -I/usr/local/include
LIBS=-L/usr/local/lib -lNgramsLocations

all: libOsmClassifier

test: testOsmClassifier
	./testOsmClassifier test-map.ngt

testOsmClassifier: Makefile osmClassifier.h testOsmClassifier.c
	gcc ${CFLAGS} -o testOsmClassifier testOsmClassifier.c ${LIBS} -lOsmClassifier

libOsmClassifier: Makefile osmClassifier.o osmClassifier.h
	gcc -shared -o libOsmClassifier.so.1.0 osmClassifier.o ${LIBS}

osmClassifier.o: Makefile osmClassifier.h osmClassifier.c
	gcc ${CFLAGS} -c osmClassifier.c -o osmClassifier.o

install:
	cp libOsmClassifier.so.1.0 /usr/local/lib
	ln -sf /usr/local/lib/libOsmClassifier.so.1.0 /usr/local/lib/libOsmClassifier.so.1
	ln -sf /usr/local/lib/libOsmClassifier.so.1.0 /usr/local/lib/libOsmClassifier.so
	ldconfig /usr/local/lib
	cp osmClassifier.h /usr/local/include/osmClassifier.h

clean:
	rm *.o; rm *.so*; rm core*; rm testOsmClassifier
