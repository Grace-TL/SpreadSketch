#include "spreadsketch_lc.hpp"


DetectorSSLC::DetectorSSLC(int depth, int width, int lgn, int m) {
    ss_.depth = depth;
    ss_.width = width;
    ss_.lgn = lgn;
    ss_.m = m;
    ss_.segment = (ss_.m + 7) >> 3;
    ss_.tdepth = depth > 4 ? 4 : depth;
    //init linear counting
    ss_.counts = new unsigned char*[ss_.depth]();
    for (int i = 0; i < ss_.depth; i++) {
        ss_.counts[i] = new unsigned char[ss_.width*ss_.segment]();
    }
    //init SSketch
    ss_.skey = new key_tp*[depth];
    ss_.level = new int*[depth];
    for (int i = 0; i < depth; i++) {
        ss_.skey[i] = new key_tp[width]();
        ss_.level[i] = new int[width]();
    }
    ss_.hash = new unsigned long[depth];
    char name[] = "DetectorSSLC";
    unsigned long seed = AwareHash((unsigned char*)name, strlen(name), 13091204281, 228204732751, 6620830889);
    for (int i = 0; i < depth; i++) {
        ss_.hash[i] = GenHashSeed(seed++);
    }
}

DetectorSSLC::~DetectorSSLC() {
    delete [] ss_.hash;
    for (int i = 0; i < ss_.depth; i++) {
        delete [] ss_.skey[i];
        delete [] ss_.level[i];
        delete [] ss_.counts[i];
    }
    delete [] ss_.counts;
    delete [] ss_.skey;
    delete [] ss_.level;
}

int DetectorSSLC::loghash(unsigned long p) {
    int ret = 0;
    while ((p&0x00000001) == 1) {
        p >>= 1;
        ret++;
    }
    return ret+1;
}

//Optimized update
void DetectorSSLC::Update(key_tp src, key_tp dst, val_tp weight) {

    unsigned long edge = src;
    edge = (edge << 32) | dst;
    int tmplevel = 0;
    //Update sketch
    unsigned long p = MurmurHash64A((unsigned char*)(&edge), ss_.lgn/8*2, ss_.hash[0]);
    tmplevel = loghash(p);
    unsigned pos = ((p >> 32) * ss_.m) >> 32;
    unsigned bucket = 0;
    uint32_t key[3] = {src, src, src};
    uint32_t hashval[4] = {0};
    MurmurHash3_x64_128 ( (unsigned char*)key, 12, ss_.hash[0], (unsigned char*)hashval);
    for (int i = 0; i < ss_.tdepth; i++) {
        bucket = (hashval[i] * (unsigned long)ss_.width) >> 32;
        if (ss_.level[i][bucket] < tmplevel) {
            ss_.level[i][bucket] = tmplevel;
            ss_.skey[i][bucket] = src;
        }
        //Update distinct counter
        unsigned hindex = pos+ss_.segment*8*bucket;
        setbit(hindex, ss_.counts[i]);
    }
    for (int i = 4; i < ss_.depth; i++) {
        bucket = MurmurHash64A((unsigned char*)(&src), ss_.lgn/8, ss_.hash[i]);
        bucket = (bucket * (unsigned long)ss_.width) >> 32;
        if (ss_.level[i][bucket] < tmplevel) {
            ss_.level[i][bucket] = tmplevel;
            ss_.skey[i][bucket] = src;
        }
        unsigned hindex = pos+ss_.segment*8*bucket;
        setbit(hindex, ss_.counts[i]);
    }
}


int DetectorSSLC::Estimate(int bucket, unsigned char* bmp){
    int pos = bucket*ss_.segment*8;
    int zeros = countzerobits(bmp, pos, pos + ss_.m);
    if(zeros == 0) zeros = 1;
    double est = 1.0 * ss_.m * log(1.0*ss_.m/zeros);
    return est;
}

int DetectorSSLC::PointQuery(uint32_t key) {
    uint32_t hkey[3] = {key, key, key};
    uint32_t hashval[4] = {0};
    MurmurHash3_x64_128 ( (unsigned char*)hkey, 12, ss_.hash[0], (unsigned char*)hashval);
    int ret=0;
    int degree=0;
    for (int i=0; i<ss_.tdepth; i++) {
        unsigned bucket = (uint32_t )hashval[i] * (unsigned long)ss_.width >> 32;
        if (i==0) {
            ret = Estimate(bucket, ss_.counts[i]);
        }
        degree = Estimate(bucket, ss_.counts[i]);
        ret = ret > degree ? degree : ret;
    }
    for (int i=4; i<ss_.depth; i++) {
        unsigned bucket = MurmurHash64A((unsigned char*)(&key), ss_.lgn/8, ss_.hash[i]);
        bucket = (bucket * (unsigned long)ss_.width) >> 32;
        degree = Estimate(bucket, ss_.counts[i]);
        ret = ret > degree ? degree : ret;
    }
    return ret;
}

void DetectorSSLC::Copybmp(unsigned char* dst, unsigned char* src, int start, int len) {
    int curlen = 0;
    while (curlen < len) {
        unsigned char temp = 0;
        int n = start + curlen;
        int index = n >> 3;
        int pos = n&0x07;
        int pos2 = curlen&0x07;
        if (pos2 < pos) {
            temp |= src[index] & (0xff >> pos);
            temp <<= (pos-pos2);
            dst[curlen>>3] |= temp;
            curlen += 8-pos;
        } else {
            temp |= src[index] & (0xff >> pos);
            temp >>= (pos2-pos);
            dst[curlen>>3] |= temp;
            curlen += 8-pos2;
        }
    }
}

//Intersec two bitmaps
void DetectorSSLC::Intersec(unsigned char* dst, unsigned char* src, int start, int len) {

    int curlen = 0;
    while (curlen < len) {
        unsigned char temp = 255;
        int n = start + curlen;
        int index = n >> 3;
        int pos = n&0x07;
        int pos2 = curlen&0x07;
        if (pos2 < pos) {
            temp &= src[index] & (0xff >> pos);
            temp <<= (pos-pos2);
            temp |= ((1 << (pos-pos2)) -1);
            dst[curlen>>3] &= temp;
            curlen += 8-pos;
        } else {
            temp &= src[index] & (0xff >> pos);
            temp >>= (pos2-pos);
            temp |= 256 - (1 << (8-pos2));
            dst[curlen>>3] &= temp;
            curlen += 8-pos2;
        }
    }
}

int DetectorSSLC::PointQueryMerge(uint32_t key) {
    uint32_t qkey[3] = {key, key, key};
    uint32_t hashval[4] = {0};
    MurmurHash3_x64_128((unsigned char*)qkey, 12, ss_.hash[0], (unsigned char*)hashval);
    int ret=0;
    unsigned char* mcount = (unsigned char*)malloc(ss_.segment*sizeof(unsigned char)); 
    memset(mcount, 0, ss_.segment*sizeof(unsigned char)); 
    unsigned bucket = ((uint32_t )hashval[0] * (unsigned long)ss_.width) >> 32;
    Copybmp(mcount, ss_.counts[0], bucket*ss_.segment*8, ss_.segment*8);
    for (int i=1; i<ss_.tdepth; i++) {
        bucket = ((uint32_t )hashval[i] * (unsigned long)ss_.width) >> 32;
        Intersec(mcount, ss_.counts[i], bucket*ss_.segment*8, ss_.segment*8);
    }
    for (int i = 4; i < ss_.depth; i++) {
        unsigned bucket = MurmurHash64A((unsigned char*)(&key), ss_.lgn/8, ss_.hash[i]);
        bucket = (bucket * (unsigned long)ss_.width) >> 32;
        Intersec(mcount, ss_.counts[i], bucket*ss_.segment*8, ss_.segment*8);
    }
    ret = Estimate(0, mcount);
    //std::cout << ret << std::endl;
    free(mcount); 
    return ret;
}

void DetectorSSLC::Query(val_tp thresh, std::vector<std::pair<key_tp, val_tp> > &results) {
    std::unordered_set<key_tp> reset;
    for (int i = 0; i < ss_.depth; i++) {
        for (int j = 0; j < ss_.width; j++) {
            val_tp est = Estimate(j, ss_.counts[i]);
            if (est >= thresh) {
                reset.insert(ss_.skey[i][j]);
            }
        }
    }
    for (auto it = reset.begin(); it != reset.end(); it++) {
        //val_tp degree = PointQuery(*it);
        val_tp degree = PointQueryMerge(*it);
        if ( degree >= thresh) {
            std::pair<key_tp, val_tp> node;
            node.first = *it;
            node.second = degree;
            results.push_back(node);
        }
    }
}




void DetectorSSLC::Reset() {
}


unsigned char** DetectorSSLC::GetTable() {
    return ss_.counts;
}

key_tp** DetectorSSLC::GetKey() {
    return ss_.skey;
}

int** DetectorSSLC::GetLevel() {
    return ss_.level;
}

void DetectorSSLC::Merge(DetectorSSLC *detector) {
    DetectorSSLC* ss = (DetectorSSLC*) detector;
    unsigned char ** counts  = ss->GetTable();
    key_tp** skey = ss->GetKey();
    int** level =  ss->GetLevel();
    for (int i = 0; i < ss_.depth; i++) {
        for (int j = 0; j < ss_.width; j++) {
            for (int k = 0; k < ss_.m; k++) {
                int index = j*ss_.m+k;
                ss_.counts[i][index] = ss_.counts[i][index] > counts[i][index] ? ss_.counts[i][index] : counts[i][index];
            }
            if (ss_.level[i][j] <= level[i][j]) {
                ss_.skey[i][j] = skey[i][j];
                ss_.level[i][j] = level[i][j];
            }
        }
    }
}


