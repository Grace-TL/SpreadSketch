#ifndef __DETECTORSSLC_H__
#define __DETECTORSSLC_H__
#include "bitmap.h"
#include <vector>
#include <utility>
#include "datatypes.hpp"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <iostream>
#include <unordered_set>
extern "C"
{
#include "hash.h"
}

class DetectorSSLC {

    struct CSS_type {

        /************  parameters for Linear Counting **************/
        //The size of a bitmap
        int m;

        //Used to construct bitmap
        int segment;

        //bitmaps
        unsigned char ** counts;


        /************  parameters for SSketch **************/
        //Outer sketch depth
        int depth;

        //Outer sketch width
        int width;

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
    DetectorSSLC(int depth, int width, int lgn, int m);

    ~DetectorSSLC();

    void Update(key_tp src, key_tp dst, val_tp weight);

    int PointQuery(uint32_t key);

    int PointQueryMerge(uint32_t key);

    void Copybmp(unsigned char* dst, unsigned char* src, int start, int len);

    void Intersec(unsigned char* dst, unsigned char* src, int start, int len);

    void Query(val_tp thresh, std::vector<std::pair<key_tp, val_tp> > &results);

    void Reset();

    unsigned char ** GetTable();

    void Merge(DetectorSSLC *detector);

    key_tp** GetKey();

    int** GetLevel();


private:
    //Sketch data structure
    CSS_type ss_;

    int loghash(unsigned long p);

    int Estimate(int bucket, unsigned char* bmp);
};
#endif

