/*
 * implementation of the multiple resolution bitmap algorithm
 * Details see the paper: Bitmap algorithms for counting active flows on
 * high-speed links, ToN, 2006
 * */


#ifndef MRB_H
#define MRB_H

#include "bitmap.h"
#include "stdint.h"


#define MAX_COMPONENTS  32

class DiscounterMRB {
 public:
  DiscounterMRB(int b, int c, int memory, int flowIDsize);
  ~DiscounterMRB();
  void Update(unsigned char* flowID){new_packet(flowID,bmp);}
  void Update(unsigned hashval, int crt){new_packet(hashval, crt, bmp);}
  void new_packet(unsigned hashval, int crt, bitmap components);
  void new_packet(unsigned char* flowID, bitmap components);
  uint64_t Query(){return estimate(bmp);}
  int estimate(bitmap components);
  void Reset(){reset(bmp);}
  void reset(bitmap components);
  void Merge(DiscounterMRB *sketch);
  void Intersec(DiscounterMRB *sketch);
  void PrintBmp();
 private:
  int b;     // The size of a normal component
  int c;     // The number of components
  int lastb; // The size of the last component
  int setmax;// Used to select the base component
  bitmap bmp;// The actual bits
  //h3 hash;   // The hash function
  unsigned long hash; // The hash seed;
  int offsets[MAX_COMPONENTS+1]; // The bit offsets of the components
  int lgn; //The number of bits in a key
};

#endif //MRB_H
