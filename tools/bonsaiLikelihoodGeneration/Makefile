INCS=-I$(ROOTSYS)/include -I$(ROOTSYS)/include/root  -I .
LIBSROOT=-L$(ROOTSYS)/lib -lCore  -lRIO -lNet -lHist -lGraf -lGraf3d -lGpad -lTree -lRint -lPostscript -lMatrix -lPhysics -lMathCore -lThread -lGui

all: create_like showlike

binfile.o: binfile.cc binfile.h
	g++ -c binfile.cc

create_like: create_like.cc binfile.o
	g++ -o create_like -I. create_like.cc binfile.o

showlike: showlike.cc binfile.o
	g++  `root-config --cflags --libs` -o showlike $(INCS) showlike.cc binfile.o $(LIBSROOT)

clean:
	$(RM) *.o *~ create_like showlike
