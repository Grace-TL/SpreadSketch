all: $(EXEC)

CFLAGS = -Wall -std=c++11 -O3 
HEADER += hash.h datatypes.hpp util.h inputadaptor.hpp bitmap.h 
SRC += hash.c inputadaptor.cpp  bitmap.c
SKETCHHEADER += spreadsketch.hpp mrbmp.hpp
SKETCHSRC += mrbmp.cpp

LIBS= -lpcap -lrt -lm

main_ss: main_ss.cpp $(SRC) $(HEADER) $(SKETCHHEADER)   
	g++ $(CFLAGS) $(INCLUDES) -o $@ $< $(SKETCHSRC) $(SRC) $(LIBS)

clean:
	rm -rf $(EXEC)
	rm -rf *log*
	rm -rf *out*




