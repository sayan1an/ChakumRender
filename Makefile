CC = g++
ASAN = -g3 -O0 -fno-omit-frame-pointer -fsanitize=address
CFLAGS = -Wall -fopenmp
ROOT=$(realpath $(dir $(lastword $(MAKEFILE_LIST))))

INCLUDES = $(ROOT)/headers
INCLUDEC = -I$(INCLUDES) -I$(ROOT)/externLib/objectLoader -I$(ROOT)/externLib/XMLParser
SRC = $(ROOT)/src
OBJ = $(ROOT)/src

#make will look for files in these path apart from current location of makefile
VPATH = $(SRC) $(INCLUDES) $(OBJ) $(ROOT)/externLib/sharedLib $(ROOT)/externLib/objectLoader $(ROOT)/externLib/XMLParser

#-Wl,-R,$(ROOT)/ is to suggest linker to look into $(ROOT) at runtime for loading dynamic libraries.
main: domainSampler.o mathPrimitives.o random.o main.o materialTypes.o geometryPrimitives.o objects.o shader.o lightSources.o lightSources_CV.o toon.o transformations.o dummyAccel.o bvhAccel.o mis.o ppm.o sceneParser.o prepareScene.o libobjLoader.so.1.0.1 libXMLParser.so.1.0.1
	$(CC) $(CFLAGS) $(OBJ)/domainSampler.o $(OBJ)/mathPrimitives.o $(OBJ)/random.o $(OBJ)/materialTypes.o $(OBJ)/geometryPrimitives.o $(OBJ)/objects.o $(OBJ)/shader.o $(OBJ)/lightSources.o \
	$(OBJ)/transformations.o $(OBJ)/dummyAccel.o $(OBJ)/bvhAccel.o $(OBJ)/mis.o $(OBJ)/toon.o $(OBJ)/lightSources_CV.o $(OBJ)/ppm.o $(OBJ)/sceneParser.o $(OBJ)/prepareScene.o $(OBJ)/main.o \
	-L$(ROOT)/externLib/sharedLib/ -lXMLParser -lobjLoader -Wl,-R,$(ROOT)/ 

main.o: main.cpp
	$(CC) $(CFLAGS) $(INCLUDEC) -O -c $(SRC)/main.cpp -o $(OBJ)/main.o

random.o: random.h random.cpp
	$(CC) $(CFLAGS) $(INCLUDEC) -O -c $(SRC)/random.cpp -o $(OBJ)/random.o

mathPrimitives.o: mathPrimitives.h mathPrimitives.cpp
	$(CC) $(CFLAGS) $(INCLUDEC) -O -c $(SRC)/mathPrimitives.cpp -o $(OBJ)/mathPrimitives.o

domainSampler.o: domainSampler.cpp domainSampler.h
	$(CC) $(CFLAGS) $(INCLUDEC) -O -c $(SRC)/domainSampler.cpp -o $(OBJ)/domainSampler.o

materialTypes.o: materialTypes.cpp materialTypes.h ltc.h
	$(CC) $(CFLAGS) $(INCLUDEC) -O -c $(SRC)/materialTypes.cpp -o $(OBJ)/materialTypes.o

geometryPrimitives.o: geometryPrimitives.cpp geometryPrimitives.h
	$(CC) $(CFLAGS) $(INCLUDEC) -O -c $(SRC)/geometryPrimitives.cpp -o $(OBJ)/geometryPrimitives.o

objects.o: objects.h objects.cpp
	$(CC) $(CFLAGS) $(INCLUDEC) -O -c $(SRC)/objects.cpp -o $(OBJ)/objects.o

shader.o: shader.h shader.cpp
	$(CC) $(CFLAGS) $(INCLUDEC) -O -c $(SRC)/shader.cpp -o $(OBJ)/shader.o

lightSources.o: lightSources.h lightSources.cpp
	$(CC) $(CFLAGS) $(INCLUDEC) -O -c $(SRC)/lightSources.cpp -o $(OBJ)/lightSources.o

lightSources_CV.o: lightSources.h lightSources_CV.cpp
	$(CC) $(CFLAGS) $(INCLUDEC) -O -c $(SRC)/lightSources_CV.cpp -o $(OBJ)/lightSources_CV.o

toon.o: lightSources.h toon.cpp
	$(CC) $(CFLAGS) $(INCLUDEC) -O -c $(SRC)/toon.cpp -o $(OBJ)/toon.o

transformations.o: transformations.h transformations.cpp
	$(CC) $(CFLAGS) $(INCLUDEC) -O -c $(SRC)/transformations.cpp -o $(OBJ)/transformations.o

dummyAccel.o: dummyAccel.h dummyAccel.cpp
	$(CC) $(CFLAGS) $(INCLUDEC) -O -c $(SRC)/dummyAccel.cpp -o $(OBJ)/dummyAccel.o

bvhAccel.o: bvhAccel.cpp bvhAccel.h
	$(CC) $(CFLAGS) $(INCLUDEC) -O -c $(SRC)/bvhAccel.cpp -o $(OBJ)/bvhAccel.o

mis.o: mis.cpp mis.h
	$(CC) $(CFLAGS) $(INCLUDEC) -O -c $(SRC)/mis.cpp -o $(OBJ)/mis.o

ppm.o: ppm.cpp ppm.h
	$(CC) $(CFLAGS) $(INCLUDEC) -O -c $(SRC)/ppm.cpp -o $(OBJ)/ppm.o
	
sceneParser.o: sceneParser.cpp sceneParser.h
	$(CC) $(CFLAGS) $(INCLUDEC) -O -c $(SRC)/sceneParser.cpp -o $(OBJ)/sceneParser.o

prepareScene.o: prepareScene.cpp prepareScene.h
	$(CC) $(CFLAGS) $(INCLUDEC) -O -c $(SRC)/prepareScene.cpp -o $(OBJ)/prepareScene.o

libXMLParser.so.1.0.1: tinyxml2.o
	-mkdir $(ROOT)/externLib/sharedLib
	$(CC) $(CFLAGS) -shared -Wl,-soname,libXMLParser.so.1 -o $(ROOT)/externLib/sharedLib/libXMLParser.so.1.0.1  $(ROOT)/externLib/XMLParser/tinyxml2.o
	ln -s $(ROOT)/externLib/sharedLib/libXMLParser.so.1.0.1 $(ROOT)/libXMLParser.so.1
	ln -s $(ROOT)/externLib/sharedLib/libXMLParser.so.1.0.1 $(ROOT)/externLib/sharedLib/libXMLParser.so

tinyxml2.o: tinyxml2.cpp tinyxml2.h
	$(CC) -I$(ROOT)/externLib/XMLParser -c -fPIC $(ROOT)/externLib/XMLParser/tinyxml2.cpp -o $(ROOT)/externLib/XMLParser/tinyxml2.o

libobjLoader.so.1.0.1: objLoader.o list.o objParser.o stringExtra.o
	-mkdir $(ROOT)/externLib/sharedLib
	$(CC) $(CFLAGS) -shared -Wl,-soname,libobjLoader.so.1 -o $(ROOT)/externLib/sharedLib/libobjLoader.so.1.0.1  $(ROOT)/externLib/objectLoader/objLoader.o \
	$(ROOT)/externLib/objectLoader/list.o $(ROOT)/externLib/objectLoader/objParser.o $(ROOT)/externLib/objectLoader/stringExtra.o
	ln -s $(ROOT)/externLib/sharedLib/libobjLoader.so.1.0.1 $(ROOT)/libobjLoader.so.1
	ln -s $(ROOT)/externLib/sharedLib/libobjLoader.so.1.0.1 $(ROOT)/externLib/sharedLib/libobjLoader.so

objLoader.o: objLoader.cpp objLoader.h
	$(CC) -I$(ROOT)/externLib/objectLoader -c -fPIC $(ROOT)/externLib/objectLoader/objLoader.cpp -o $(ROOT)/externLib/objectLoader/objLoader.o

list.o: list.cpp list.h
	$(CC) -I$(ROOT)/externLib/objectLoader -c -fPIC $(ROOT)/externLib/objectLoader/list.cpp -o $(ROOT)/externLib/objectLoader/list.o

objParser.o: objParser.cpp objParser.h
	$(CC) -I$(ROOT)/externLib/objectLoader -c -fPIC $(ROOT)/externLib/objectLoader/objParser.cpp -o $(ROOT)/externLib/objectLoader/objParser.o

stringExtra.o: stringExtra.cpp stringExtra.h
	$(CC) -I$(ROOT)/externLib/objectLoader -c -fPIC $(ROOT)/externLib/objectLoader/stringExtra.cpp -o $(ROOT)/externLib/objectLoader/stringExtra.o
	
.PHONY: clean cleanall

clean:
	cd $(OBJ) && rm *.o
	cd $(ROOT) && rm *.out

cleanall:
	-cd $(OBJ) && rm *.o
	-cd $(ROOT)/externLib/objectLoader && rm *.o
	-cd $(ROOT)/externLib/XMLParser && rm *.o
	-cd $(ROOT)/externLib/sharedLib/ && rm *.*
	-cd $(ROOT) && rm *.so.*
	-cd $(ROOT) && rm *.so
	cd $(ROOT) && rm *.out
