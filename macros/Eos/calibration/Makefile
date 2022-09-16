ROOTA=$(shell root-config --cflags)
ROOTB=$(shell root-config --libs)
ROOTSTUFF=$(shell root-config --cflags --libs)
RATSTUFF= -I$(RATROOT)/include -L$(RATROOT)/lib -lRATEvent

all:
	g++ $(ROOTA) $(RATSTUFF) sphereProperties.cpp -o sp $(RATSTUFF) $(ROOTB)
