all: $(EXEC)

CFLAGS = -Wall -std=c++11 -O3 
HEADER += hash.h datatypes.hpp util.h inputadaptor.hpp bitmap.h 
SRC += hash.c inputadaptor.cpp  bitmap.c spreadsketch.cpp spreadsketch_hll.cpp spreadsketch_lc.cpp spreadsketch_kmv.cpp
SKETCHHEADER += spreadsketch.hpp spreadsketch_hll.hpp spreadsketch_lc.hpp spreadsketch_kmv.hpp 



LIBS= -lpcap -lm

main_ss: main_ss.cpp $(SRC) $(HEADER) $(SKETCHHEADER)   
	g++ $(CFLAGS) $(INCLUDES) -o $@ $< $(SKETCHSRC) $(SRC) $(LIBS)

main_ss_hp: main_ss.cpp $(SRC) $(HEADER) $(SKETCHHEADER)   
	g++ $(CFLAGS) -DHH $(INCLUDES) -o $@ $< $(SKETCHSRC) $(SRC) $(LIBS)

main_hll: main_hll.cpp $(SRC) $(HEADER) $(SKETCHHEADER)   
	g++ $(CFLAGS) $(INCLUDES) -o $@ $< $(SKETCHSRC) $(SRC) $(LIBS)

main_lc: main_lc.cpp $(SRC) $(HEADER) $(SKETCHHEADER)   
	g++ $(CFLAGS) $(INCLUDES) -o $@ $< $(SKETCHSRC) $(SRC) $(LIBS)

main_kmv: main_kmv.cpp $(SRC) $(HEADER) $(SKETCHHEADER)   
	g++ $(CFLAGS) $(INCLUDES) -o $@ $< $(SKETCHSRC) $(SRC) $(LIBS)


clean:
	rm -rf $(EXEC)
	rm -rf *log*
	rm -rf *out*




