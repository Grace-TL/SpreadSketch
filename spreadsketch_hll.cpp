#include "spreadsketch_hll.hpp"

DetectorSSHLL::DetectorSSHLL(int depth, int width, int lgn, int m) {
    ss_.depth = depth;
    ss_.width = width;
    ss_.lgn = lgn;
    ss_.tdepth = depth > 4 ? 4 : depth;
    //init hyperloglog
    ss_.m = m;
    ss_.alpha = 0.7213*m*m/(1+1.079/m);
    ss_.counts = new short*[ss_.depth]();
    for (int i = 0; i < ss_.depth; i++) {
        ss_.counts[i] = new short[ss_.width*ss_.m]();
    }
    ss_.setmax = ss_.m*0.07;
    //init SSketch
    ss_.skey = new key_tp*[depth];
    ss_.level = new int*[depth];
    for (int i = 0; i < depth; i++) {
        ss_.skey[i] = new key_tp[width]();
        ss_.level[i] = new int[width]();
    }
    ss_.hash = new unsigned long[depth];
    char name[] = "DetectorSSHLL";
    unsigned long seed = AwareHash((unsigned char*)name, strlen(name), 13091204281, 228204732751, 6620830889);
    for (int i = 0; i < depth; i++) {
        ss_.hash[i] = GenHashSeed(seed++);
    }
}

DetectorSSHLL::~DetectorSSHLL() {
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

//right most zero
int DetectorSSHLL::loghash(unsigned long p) {
    int ret = 0;
    while ((p&0x00000001) == 1) {
        p >>= 1;
        ret++;
    }
    return ret+1;
}



//Optimized update
void DetectorSSHLL::Update(key_tp src, key_tp dst, val_tp weight) {

    unsigned long edge = src;
    edge = (edge << 32) | dst;
    int tmplevel = 0;
    //Update sketch
    unsigned long p = MurmurHash64A((unsigned char*)(&edge), ss_.lgn/8*2, ss_.hash[0]);
    tmplevel = loghash(p);
    unsigned bucket = 0;
    uint32_t key[3] = {src, src, src};
    uint32_t hashval[4] = {0};
    MurmurHash3_x64_128 ( (unsigned char*)key, 12, ss_.hash[0], (unsigned char*)hashval);
    unsigned pos = ((p >> 32) * ss_.m) >> 32;
    for (int i = 0; i < ss_.tdepth; i++) {
        bucket = (hashval[i] * (unsigned long)ss_.width) >> 32;
        if (ss_.level[i][bucket] < tmplevel) {
            ss_.level[i][bucket] = tmplevel;
            ss_.skey[i][bucket] = src;
        }
        //Update distinct counter
        unsigned hindex = pos+ss_.m*bucket;
        ss_.counts[i][hindex] = ss_.counts[i][hindex] < tmplevel ? tmplevel : ss_.counts[i][hindex];
    }
    for (int i = 4; i < ss_.depth; i++) {
        bucket = MurmurHash64A((unsigned char*)(&src), ss_.lgn/8, ss_.hash[i]);
        bucket = (bucket * (unsigned long)ss_.width) >> 32;
        if (ss_.level[i][bucket] < tmplevel) {
            ss_.level[i][bucket] = tmplevel;
            ss_.skey[i][bucket] = src;
        }
        //Update distinct counter
        unsigned hindex = pos+ss_.m*bucket;
        ss_.counts[i][hindex] = ss_.counts[i][hindex] < tmplevel ? tmplevel : ss_.counts[i][hindex];
    }
}


int DetectorSSHLL::Estimate(int row, int bucket){
    double res = 0;
    int pos = bucket*ss_.m;
    int zeros = 0;
    for (int i = 0; i < ss_.m; i++) {
        res += 1.0/((unsigned long)1 << ss_.counts[row][pos+i]);
        zeros = ss_.counts[row][pos+i] == 0 ? zeros+1 : zeros;
    }
    if (zeros > ss_.setmax) return ss_.m*log(ss_.m/zeros);
    return (int)(ss_.alpha/res);
}

int DetectorSSHLL::PointQuery(uint32_t key) {
    int ret=0;
    int degree=0;
    uint32_t qkey[3] = {key, key, key};
    uint32_t hashval[4] = {0};
    MurmurHash3_x64_128((unsigned char*)qkey, 12, ss_.hash[0], (unsigned char*)hashval);
    for (int i=0; i<ss_.tdepth; i++) {
        unsigned bucket = ((uint32_t )hashval[i] * (unsigned long)ss_.width) >> 32;
        if (i==0) {
            ret = Estimate(0, bucket);
        }
        degree = Estimate(i, bucket);
        ret = ret > degree ? degree : ret;
    }
    for (int i=4; i<ss_.depth; i++) {
        unsigned bucket = MurmurHash64A((unsigned char*)(&key), ss_.lgn/8, ss_.hash[i]);
        bucket = (bucket * (unsigned long)ss_.width) >> 32;
        degree = Estimate(i, bucket);
        ret = ret > degree ? degree : ret;
    }
    return ret;
}



void DetectorSSHLL::Query(val_tp thresh, std::vector<std::pair<key_tp, val_tp> > &results) {

    std::unordered_set<key_tp> reset;
    for (int i = 0; i < ss_.depth; i++) {
        for (int j = 0; j < ss_.width; j++) {
            key_tp est = Estimate(i, j);
            if (est >= thresh) {
                reset.insert(ss_.skey[i][j]);
            }
        }
    }

    for (auto it = reset.begin(); it != reset.end(); it++) {
        val_tp degree = PointQuery(*it);
        if ( degree >= thresh) {
            std::pair<key_tp, val_tp> node;
            node.first = *it;
            node.second = degree;
            results.push_back(node);
        }
    }
}




void DetectorSSHLL::Reset() {
}


short** DetectorSSHLL::GetTable() {
    return ss_.counts;
}

key_tp** DetectorSSHLL::GetKey() {
    return ss_.skey;
}

int** DetectorSSHLL::GetLevel() {
    return ss_.level;
}

void DetectorSSHLL::Merge(DetectorSSHLL *detector) {
    DetectorSSHLL* ss = (DetectorSSHLL*) detector;
    short** counts  = ss->GetTable();
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


