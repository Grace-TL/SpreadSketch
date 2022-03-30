#include "spreadsketch_kmv.hpp"


DetectorSSKMV::DetectorSSKMV(int depth, int width, int lgn, int k) {
    ss_.depth = depth;
    ss_.width = width;
    ss_.lgn = lgn;
    ss_.tdepth = depth > 4 ? 4 : depth;
    ss_.k = k;
    //init KMV
    ss_.index = new int *[depth];
    ss_.counts = new uint32_t **[depth];
    for (int i = 0; i < ss_.depth; i++) {
        ss_.index[i] = new int [width]();
        ss_.counts[i] = new uint32_t *[width];
        for (int j = 0; j < ss_.width; j++) {
            ss_.counts[i][j] = new uint32_t [k]();
        }
    }

    //init SSketch
    ss_.skey = new key_tp*[depth];
    ss_.level = new int*[depth];
    for (int i = 0; i < depth; i++) {
        ss_.skey[i] = new key_tp[width]();
        ss_.level[i] = new int[width]();
    }
    ss_.hash = new unsigned long [depth];
    char name[] = "KMinimumValue";
    unsigned long seed = AwareHash((unsigned char*)name, strlen(name), 13091204281, 228204732751, 6620830889);
    for (int i = 0; i < depth; i++) {
        ss_.hash[i] = GenHashSeed(seed++);
    }
}

DetectorSSKMV::~DetectorSSKMV() {
    delete [] ss_.hash;
    for (int i = 0; i < ss_.depth; i++) {
        for(int j = 0; j < ss_.width; j++) {
            delete [] ss_.counts[i][j];
        }
        delete [] ss_.skey[i];
        delete [] ss_.level[i];
        delete [] ss_.counts[i];
        delete [] ss_.index[i];
    }
    delete [] ss_.index;
    delete [] ss_.counts;
    delete [] ss_.skey;
    delete [] ss_.level;
}

//right most zero
int DetectorSSKMV::loghash(unsigned long p) {
    int ret = 0;
    while ((p&0x00000001) == 1) {
        p >>= 1;
        ret++;
    }
    return ret;
}

// For width <= 4
void DetectorSSKMV::Update(key_tp src, key_tp dst, val_tp weight) {
    unsigned long long edge = src;
    edge = (edge << 32) | dst;
    int tmplevel = 0;
    //Update sketch
    unsigned long p = MurmurHash64A((unsigned char*)(&edge), ss_.lgn/8*2, ss_.hash[0]);
    tmplevel = loghash(p);
    uint32_t key[3] = {src, src, src};
    uint32_t hashval[4] = {0};
    MurmurHash3_x64_128 ( (unsigned char*)key, 12, ss_.hash[0], (unsigned char*)hashval);

    for (int i = 0; i < ss_.tdepth; i++) {
        unsigned j = (hashval[i] * (unsigned long)ss_.width) >> 32;
        //int j = hashval[i] % ss_.width;
        if (ss_.level[i][j] < tmplevel) {
            ss_.level[i][j] = tmplevel;
            ss_.skey[i][j] = src;
        }
        //Update distinct counter
        uint32_t bucket = MurmurHash2(&dst, ss_.lgn/8, ss_.hash[i]);
        int l = 0;
        int r = ss_.index[i][j] - 1;
        int m = 0;
        // If kmv is full, skip if element larger than head
        // TODO make bool setting
        if (ss_.index[i][j] >= ss_.k && ss_.counts[i][j][0] <= bucket) {
            //std::cout << "COUNTINUE" << std::endl;
            continue;
        }
        //Just insert if empty
        if (ss_.index[i][j] == 0) {
            ss_.counts[i][j][0] = bucket;
            ss_.index[i][j]++;
            //std::cout << "INSERT FINISHED" << std::endl;
            continue;
        }

        //Perform search
        while (1) {
            //Find new middle
            m = (l + r) / 2;
            //Not found, insert
            if (l > r || m >= ss_.index[i][j]) {
                //If kmv is full, shift up
                if (ss_.index[i][j] >= ss_.k) {
                    memmove(ss_.counts[i][j], ss_.counts[i][j]+1, m*sizeof(uint32_t));
                    //Insert the element
                    ss_.counts[i][j][m] = bucket;
                    break;
                } else {
                    //Else shift values down to make space
                    if (ss_.counts[i][j][m] > bucket) {
                        m++;
                    }
                    memmove(ss_.counts[i][j]+m+1, ss_.counts[i][j]+m, (ss_.index[i][j] - m)*sizeof(uint32_t));
                    //Insert the element
                    ss_.counts[i][j][m] = bucket;
                    ss_.index[i][j]++;
                    break;
                }
                break;
            }
            if (ss_.counts[i][j][m] > bucket) {
                l = m + 1;
                continue;
            } else if (ss_.counts[i][j][m] < bucket) {
                r = m - 1;
                continue;
            }
            break;
        }
    }
    for (int i = 4; i < ss_.depth; i++) {
        unsigned j = MurmurHash64A((unsigned char*)(&src), ss_.lgn/8, ss_.hash[i]);
        j = (j * (unsigned long)ss_.width) >> 32;
        //int j = hashval[i] % ss_.width;
        if (ss_.level[i][j] < tmplevel) {
            ss_.level[i][j] = tmplevel;
            ss_.skey[i][j] = src;
        }
        //Update distinct counter
        uint32_t bucket = MurmurHash2(&dst, ss_.lgn/8, ss_.hash[i]);
        int l = 0;
        int r = ss_.index[i][j] - 1;
        int m = 0;
        // If kmv is full, skip if element larger than head
        // TODO make bool setting
        if (ss_.index[i][j] >= ss_.k && ss_.counts[i][j][0] <= bucket) {
            //std::cout << "COUNTINUE" << std::endl;
            continue;
        }
        //Just insert if empty
        if (ss_.index[i][j] == 0) {
            ss_.counts[i][j][0] = bucket;
            ss_.index[i][j]++;
            //std::cout << "INSERT FINISHED" << std::endl;
            continue;
        }

        //Perform search
        while (1) {
            //Find new middle
            m = (l + r) / 2;
            //Not found, insert
            if (l > r || m >= ss_.index[i][j]) {
                //If kmv is full, shift up
                if (ss_.index[i][j] >= ss_.k) {
                    memmove(ss_.counts[i][j], ss_.counts[i][j]+1, m*sizeof(uint32_t));
                    //Insert the element
                    ss_.counts[i][j][m] = bucket;
                    break;
                } else {
                    //Else shift values down to make space
                    if (ss_.counts[i][j][m] > bucket) {
                        m++;
                    }
                    memmove(ss_.counts[i][j]+m+1, ss_.counts[i][j]+m, (ss_.index[i][j] - m)*sizeof(uint32_t));
                    //Insert the element
                    ss_.counts[i][j][m] = bucket;
                    ss_.index[i][j]++;
                    break;
                }
                break;
            }
            if (ss_.counts[i][j][m] > bucket) {
                l = m + 1;
                continue;
            } else if (ss_.counts[i][j][m] < bucket) {
                r = m - 1;
                continue;
            }
            break;
        }
    }
}

int DetectorSSKMV::Estimate(int i, int j){
    int ans;
    if (ss_.k > ss_.index[i][j]) {
        //if width > km_.index[i], just return km_.index[i]
        ans = ss_.index[i][j];
    } else {
        if (ss_.k > 1) {
            //ans[i+1] = (km_.width - 1)*1.0*UINT64_MAX/ km_.counts[i*km_.width+0];
            ans = (ss_.k - 1)*1.0*UINT32_MAX/ ss_.counts[i][j][0];
        } else {
            //ans[i+1] = km_.width * 1.0 * UINT64_MAX / km_.counts[i*km_.width+0];
            ans = ss_.k * 1.0 * UINT32_MAX / ss_.counts[i][j][0];
        }
    }
    return ans;
}

int DetectorSSKMV::PointQuery(uint32_t key) {
    return 1;
}

//copy len bits from src that starts at start to dst,
void DetectorSSKMV::Copybmp(unsigned char* dst, unsigned char* src, int start, int len) {

}

//Intersec two bitmaps
void DetectorSSKMV::Intersec(unsigned char* dst, unsigned char* src, int start, int len) {

}

//Merge then query
int DetectorSSKMV::PointQueryMerge(uint32_t key) {
    int ret = INT32_MAX;
    //int *ans = new int [ss_.depth]();
    uint32_t hkey[3] = {key, key, key};
    uint32_t hashval[4] = {0};
    MurmurHash3_x64_128 ( (unsigned char*)hkey, 12, ss_.hash[0], (unsigned char*)hashval);
    for(int i = 0; i < ss_.tdepth; i++) {
        unsigned j = (hashval[i] * (unsigned long)ss_.width) >> 32;
        int tmp = Estimate(i, j);
        if(tmp < ret) ret = tmp;
        //ans[i] = tmp;
    }
    for(int i = 4; i < ss_.depth; i++) {
        unsigned j = MurmurHash64A((unsigned char*)(&key), ss_.lgn/8, ss_.hash[i]);
        j = (j * (unsigned long)ss_.width) >> 32;
        int tmp = Estimate(i, j);
        if(tmp < ret) ret = tmp;
        //ans[i] = tmp;
    }
    // std::sort(ans, ans+ss_.depth);
    // if(ss_.depth % 2 == 0) ret = (ans[ss_.depth/2] + ans[ss_.depth/2-1])/2;
    // else ret = ans[ss_.depth/2];
    return ret;
}

void DetectorSSKMV::Query(val_tp thresh, std::vector<std::pair<key_tp, val_tp> > &results) {

    std::unordered_set<key_tp> reset;
    for (int i = 0; i < ss_.depth; i++) {
        for (int j = 0; j < ss_.width; j++) {
            val_tp est = Estimate(i, j);
            if (est >= thresh) {
                reset.insert(ss_.skey[i][j]);
            }
        }
    }

    for (auto it = reset.begin(); it != reset.end(); it++) {
        val_tp degree = PointQueryMerge(*it);
        if (degree >= thresh) {
            std::pair<key_tp, val_tp> node;
            node.first = *it;
            node.second = degree;
            results.push_back(node);
        }
    }
}

void DetectorSSKMV::Reset() {
}

key_tp** DetectorSSKMV::GetKey() {
    return ss_.skey;
}

int** DetectorSSKMV::GetLevel() {
    return ss_.level;
}

//Union bmp2 to bmp1
//assume bmp1 and bmp2 have the same offset
void DetectorSSKMV::Mergebmp(unsigned char* bmp1, unsigned char* bmp2,
        int start, int len) {

}

void DetectorSSKMV::Merge(DetectorSSKMV *detector) {

}

