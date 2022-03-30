#ifndef __DETECTORKMV_H__
#define __DETECTORKMV_H__
#include "bitmap.h"
#include <vector>
#include <utility>
#include "datatypes.hpp"
#include "inputadaptor.hpp"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <iostream>
#include <unordered_set>
#include <fstream>
extern "C"
{
#include "hash.h"
}

class DetectorSSKMV {

    struct CSS_type {

        /*********** parameters for KMV *************/
        //bitmap
        uint32_t ***counts;

        //indicates the heap size 
        int **index;

        int k;

        /************  parameters for SSketch **************/
        //Outer sketch depth
        int depth;

        //Outer sketch width
        int  width;

        //key table
        key_tp** skey;

        //level table
        int** level;

        unsigned long * hash, *scale, *hardner;

        //# key bits
        int lgn;

        int tdepth;
        
    };

public:
    DetectorSSKMV(int depth, int width, int lgn, int k);

    ~DetectorSSKMV();

    void Update(key_tp src, key_tp dst, val_tp weight);

    int PointQuery(uint32_t key);

    int PointQueryMerge(uint32_t key);

    void Query(val_tp thresh, std::vector<std::pair<key_tp, val_tp> > &results);

    void Reset();

    unsigned char** GetTable();

    void Merge(DetectorSSKMV *detector);

    key_tp** GetKey();

    int** GetLevel();

    std::vector<unsigned int> test_v;
//private:

    //Sketch data structure
    CSS_type ss_;
private:
    //SS to store the heavy hitter

    std::vector<std::pair<uint32_t, uint64_t> > heap_;

    int loghash(unsigned long p);

    void Setbit(int n, int bucket,  unsigned char* bmp);

    int Estimate(int i, int j);

    void Copybmp(unsigned char* dst, unsigned char* src, int start, int len);

    void Intersec(unsigned char* dst, unsigned char* src, int start, int len);

    void Mergebmp(unsigned char* bmp1, unsigned char* bmp2, int start, int len);
};


#endif
