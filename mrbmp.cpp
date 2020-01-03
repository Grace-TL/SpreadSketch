#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "mrbmp.hpp"
#include "assume.h"
#include "hash.h"
#include "string.h"

static const double bitsetratio = 0.93105; //(1-1/exp(rhomax)) rhomax = 2.6744

DiscounterMRB::DiscounterMRB(int bb, int cc, int memory, int flowIDsize){
  int i;
  b=bb;
  c=cc;
  assume(b>1,"Components must have at least two bits\n");
  assume(c>0,"The number of components must be positive\n");
  lastb=(memory>0?memory-(c-1)*b:b*2);
  assume3(lastb>=b*2,"Not enough memory for last component: b=%d c=%d mem=%d",
	  b,c,memory);
  assume2(log(b)+log(2)*c<32*log(2),
	  "Multiresolition bitmap too fine for 32 bit hash: b=%d c=%d\n",b,c);
  if(log(b)+log(2)*c>27*log(2)){
    fprintf(stderr,"Unsafe DiscounterMRB for a 32 bit hash: b=%d c=%d\n",b,c);
  }
  setmax=(int)(b*bitsetratio+0.5);
  for(i=0;i<c;i++){offsets[i]=i*b;}
  //TL: the size of the whole bitmap
  offsets[c]=offsets[c-1]+lastb;
  bmp=allocbitmap(offsets[c]);
  assumenotnull(bmp);
  fillzero(bmp,offsets[c]);
  lgn = flowIDsize;
  char name[] = "MultiResolutionBitmap";
  unsigned long seed = AwareHash((unsigned char*)name, strlen(name), 13091204281, 228204732751, 6620830889);
  hash = GenHashSeed(seed++);
}

DiscounterMRB::~DiscounterMRB(){
  free(bmp);
}

void DiscounterMRB::PrintBmp(){
    for (int i  = 0; i < offsets[c]; i++) {
        int bit = (getbit(i, bmp) > 0);
        printf("%d", bit);
    }
    printf("\n");
}

//key is the hash value
void DiscounterMRB::new_packet(unsigned key, int crt, bitmap components){

    int pos;
    unsigned tempkey = key;
    key = key >> (crt+1);
    if(crt<c-1){
        pos = (((uint64_t)tempkey) * ((uint64_t)b)) >> 32;
        //pos=key%b;
        setbit(pos+offsets[crt],components);
    } else {
        pos = ((uint64_t)tempkey * (uint64_t)lastb) >> 32;
        //pos=key%lastb;
        setbit(pos+offsets[c-1],components);
    }
}



void DiscounterMRB::new_packet(unsigned char* flowID, bitmap components){
    unsigned key;
    MurmurHash3_x86_32((unsigned char*)(flowID), lgn/8, hash, &key);

    int pos,crt;
    //sample flow with probability 1/(2^crt)
    for(crt=0;crt<c-1;crt++){
        //TL: if key & 1 == 0, find the least significant 0 in hash
        if(!(key&1)){/*this is the component*/
            key=key>>1;
            break;
        } else {
            key=key>>1;
        }
    }
    if(crt<c-1){
        pos=key%b;
        setbit(pos+offsets[crt],components);
    } else {
        pos=key%lastb;
        setbit(pos+offsets[c-1],components);
    }
}

int DiscounterMRB::estimate(bitmap components){
  int base,i,factor,z;
  double m;
  factor=1;
  for(base=0;base<c-1;base++){
    z=countzerobits(components,offsets[base],offsets[base]+b);
    //TL: if the bitmap is not full
    if(b-z <= setmax){
      break;
    } else {
      factor=factor*2;
    }
  }
  m=0;

  /******************** Add by TANG LU ******************************/
  int pos = base-1;
  for(i=base;i<c-1;i++){
    z=countzerobits(components,offsets[i],offsets[i]+b);
    m+=b*(log(b)-log(z));
    if (z == 0 || b-z > setmax) {
      pos = i;
      m = 0;
    }
  }
  factor=factor*(1 << (pos-base+1));
  /******************************************************************/
  z=countzerobits(components,offsets[c-1],offsets[c-1]+lastb);
  if(z==0){
    //Add by TANG LU
    m+=lastb*(log(lastb));
  } else {
    m+=lastb*(log(lastb)-log(z));
  }
  return (int)(factor*m+0.5);
}

void DiscounterMRB::reset(bitmap components){
  fillzero(components,offsets[c]);
}

//get the intersection
void DiscounterMRB::Merge(DiscounterMRB *sketch){
    int byte = (offsets[c] + 7) >> 3;
    for (int i = 0; i < byte; i++) {
        bmp[i] |= sketch->bmp[i];
    }
}

void DiscounterMRB::Intersec(DiscounterMRB *sketch){
    int byte = (offsets[c] + 7) >> 3;
    for (int i = 0; i < byte; i++) {
        bmp[i] &= sketch->bmp[i];
    }
}
