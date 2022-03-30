#ifndef __DETECTORSSH_H__
#define __DETECTORSSH_H__
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

#define MAX_COMPONENTS 32
static const double bitsetratio = 0.93105; //(1-1/exp(rhomax)) rhomax = 2.6744

class DetectorSSHLL {

    struct CSS_type {

        /*********** parameters for hyperloglog *************/
        //number of bucket
        int m;

        //Used to select the base component
        int setmax;

        double alpha;

        //bitmaps
        short ** counts;


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
    DetectorSSHLL(int depth, int width, int lgn, int m);

    ~DetectorSSHLL();

    void Update(key_tp src, key_tp dst, val_tp weight);

    int PointQuery(uint32_t key);

    void Query(val_tp thresh, std::vector<std::pair<key_tp, val_tp> > &results);

    void Reset();

    short** GetTable();

    void Merge(DetectorSSHLL *detector);

    key_tp** GetKey();

    int** GetLevel();


private:
    //Sketch data structure
    CSS_type ss_;

    int loghash(unsigned long p);

    int Estimate(int row, int bucket);
};


#endif

