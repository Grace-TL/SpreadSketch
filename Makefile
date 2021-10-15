all: $(EXEC)

CFLAGS = -Wall -std=c++11 -O3 
HEADER += hash.h datatypes.hpp util.h inputadaptor.hpp bitmap.h 
SRC += hash.c inputadaptor.cpp  bitmap.c spreadsketch.cpp
SKETCHHEADER += spreadsketch.hpp mrbmp.hpp
SKETCHSRC += mrbmp.cpp

HPSKETCHHEADER += spreadsketch_HP.hpp mrbmp.hpp


LIBS= -lpcap -lm

main_ss: main_ss.cpp $(SRC) $(HEADER) $(SKETCHHEADER)   
	g++ $(CFLAGS) $(INCLUDES) -o $@ $< $(SKETCHSRC) $(SRC) $(LIBS)

main_hpss: main_hpss.cpp $(SRC) $(HEADER) $(HPSKETCHHEADER)   
	g++ $(CFLAGS) -DHH $(INCLUDES) -o $@ $< $(SKETCHSRC) $(SRC) $(LIBS)



clean:
	rm -rf $(EXEC)
	rm -rf *log*
	rm -rf *out*




