#ifndef INPUTADAPTOR_H
#define INPUTADAPTOR_H
#include "datatypes.hpp"

#define ETH_LEN 0
#define MAX_CAPLEN 0
#define IP_CHECK 0

class InputAdaptor {


    typedef struct adaptor_t {
        unsigned char* databuffer;
        unsigned long* timestamp;
        uint64_t cnt=0;
        uint64_t cur=0;
        unsigned char* ptr;
    } adaptor_t;

    public:
        InputAdaptor(std::string filename, uint64_t buffersize);

        ~InputAdaptor();

        int GetNext(edge_tp* t);

        void Reset();

        uint64_t GetDataSize();

    private:

        adaptor_t* data;

        inline static unsigned short in_chksum_ip(unsigned short* w, int len) {
            long sum = 0;
            while (len > 1) {
                sum += *w++;
                if (sum & 0x80000000)    /* if high order bit set, fold */
                    sum = (sum & 0xFFFF) + (sum >> 16);
                len -= 2;
            }
            if (len)                    /* take care of left over byte */
                sum += *w;
            while (sum >> 16)
                sum = (sum & 0xFFFF) + (sum >> 16);
            return ~sum;
        }

};
#endif
