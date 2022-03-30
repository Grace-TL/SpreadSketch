#include "spreadsketch_kmv.hpp"
#include "inputadaptor.hpp"
#include <utility>
#include "datatypes.hpp"
#include "util.h"
#include <fstream>
#include <iomanip>

int main(int argc, char* argv[]) {

    // Configure parameters
    // pcap traces
    const char* filenames = "iptraces.txt";
    // buffer size
    unsigned long long buf_size = 500000000;
    // Superspreader threshold
    double thresh = 0.001;
    // SpreadSketch parameters
    int lgn = 32;
    int cmdepth = 4;
    int cmwidth = 14096;
    int k = 4;
    int total_mem = cmdepth*cmwidth*(k*32+lgn+8)/1024/8;
    std::ifstream tracefiles(filenames);
    if (!tracefiles.is_open()) {
        std::cout << "Error opening file" << std::endl;
        return -1;
    }


    // Result array
    double precision = 0, recall = 0, error = 0, throughput = 0, dtime = 0;
    double avgpre = 0, avgrec = 0, avgerr = 0, avgthr = 0, avgdet = 0;
    int epoch = 0;
    for (std::string file; getline(tracefiles, file);) {
        epoch++;
        InputAdaptor* adaptor =  new InputAdaptor(file, buf_size);
        std::cout << "[Dataset]: " << file << std::endl;
        std::cout << "[Message] Finish read data." << std::endl;
        // Get the ground truth
        mymap ground;
        edgeset sum;
        edge_tp t;
        adaptor->Reset();
        while(adaptor->GetNext(&t) == 1) {
            sum.insert(t);
            ground[t.src_ip].insert(t.dst_ip);
        }
        std::cout << "[Message] Finish Insert hash table" << std::endl;

        // Calculate the threshold value
        double threshold = thresh * sum.size();
        std::cout << "[Message] Total distinct paris: " << sum.size() << "  threshold=" << threshold<< std::endl;

        // Create sketch
        DetectorSSKMV *sketch = new DetectorSSKMV(cmdepth, cmwidth, lgn, k);

        // Update sketch
        double t1=0, t2=0;
        double datasize = adaptor->GetDataSize();
        t1 = now_us();
        adaptor->Reset();
        while(adaptor->GetNext(&t) == 1) {
            sketch->Update(t.src_ip, t.dst_ip, 1);
        }
        t2 = now_us();
        throughput = datasize/(double)(t2-t1)*1000000;

        // Query the result
        t1=0, t2=0;
        myvector results;
        results.clear();
        t1 = now_us();
        sketch->Query(threshold, results);
        t2 = now_us();
        dtime = (double)(t2-t1)/1000000;

        // Calculate accuracy
        int tp = 0, cnt = 0;;
        for (auto it = ground.begin(); it != ground.end(); it++) {
            if (it->second.size() > threshold) {
                cnt++;
                int truth = (int) it->second.size();
                for (auto res = results.begin(); res != results.end(); res++) {
                    if (res->first == it->first) {
                        error += abs((int)res->second - truth)*1.0/truth;
                        tp++;
                    }
                }
            }
        }
        std::cout << "[Message] Total " << cnt << " superspreaders, detect " << tp << std::endl;
        precision = tp*1.0/results.size();
        recall = tp*1.0/cnt;
        error = error/tp;

        avgpre += precision; avgrec += recall; avgerr += error; avgthr += throughput; avgdet += dtime;
        delete sketch;
        delete adaptor;

        //Output to standard output
        std::cout << std::setfill(' ');
        std::cout << std::setw(20) << std::left << "Memory(KB)" << std::setw(20) << std::left << "Precision"
            << std::setw(20) << std::left << "Recall" << std::setw(20)
            << std::left << "Error" << std::setw(20) << std::left << "Throughput" << std::setw(20)
            << std::left << "DTime" << std::endl;
        std::cout << std::setw(20) << std::left << total_mem << std::setw(20) << std::left << precision << std::setw(20)
            << std::left << recall << std::setw(20) << std::left << error << std::setw(20) << std::left << throughput << std::setw(20)
            << std::left << dtime << std::endl;
    }

            //Output to standard output
        std::cout << "---------------------------------  summary ---------------------------------" << std::endl;
        std::cout << std::setfill(' ');
        std::cout << std::setw(20) << std::left << "Memory(KB)" << std::setw(20) << std::left << "Precision"
            << std::setw(20) << std::left << "Recall" << std::setw(20)
            << std::left << "Error" << std::setw(20) << std::left << "Throughput" << std::setw(20)
            << std::left << "DTime" << std::endl;
        std::cout << std::setw(20) << std::left << total_mem << std::setw(20) << std::left << avgpre/epoch << std::setw(20)
            << std::left << avgrec/epoch << std::setw(20) << std::left << avgerr/epoch << std::setw(20) << std::left << avgthr/epoch << std::setw(20)
            << std::left << avgdet/epoch << std::endl;

}

