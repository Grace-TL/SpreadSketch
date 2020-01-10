#include "inputadaptor.hpp"
#include <unordered_set>
#include <fstream>
#include <iostream>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <pcap.h>


InputAdaptor::InputAdaptor(std::string filename, uint64_t buffersize) {
    data = (adaptor_t*)calloc(1, sizeof(adaptor_t));
    data->databuffer = (unsigned char*)calloc(buffersize, sizeof(unsigned char));
    data->ptr = data->databuffer;
    data->cnt = 0;
    data->cur = 0;
    //Read pcap file
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* pfile = pcap_open_offline(filename.c_str(), errbuf);

    if (pfile == NULL) {
        std::cout << "[Error] Fail to open pcap file" << std::endl;
        exit(-1);
    }

    unsigned char* p = data->databuffer;
    const u_char* rawpkt; // raw packet
    struct pcap_pkthdr hdr;
    struct ip* ip_hdr;
    while ((rawpkt = pcap_next(pfile, &hdr)) != NULL) {
        int eth_len = ETH_LEN;
        if (eth_len == 14) {
            struct ether_header* eth_hdr = (struct ether_header*)rawpkt;
            if (ntohs(eth_hdr->ether_type) == ETHERTYPE_VLAN) {
                eth_len = 18;
            }
        }
        ip_hdr = (struct ip*)(rawpkt + eth_len);
        int srcip = ntohl(ip_hdr->ip_src.s_addr);
        int dstip = ntohl(ip_hdr->ip_dst.s_addr);
        if (p+sizeof(edge_tp) < data->databuffer + buffersize) {
            memcpy(p, &srcip, sizeof(uint32_t));
            memcpy(p+sizeof(uint32_t), &dstip, sizeof(uint32_t));
            p += sizeof(uint8_t)*8;
            data->cnt++;
        }  else break;
    }
    std::cout << "[Message] Read " << data->cnt << " items" << std::endl;
    pcap_close(pfile);
}



InputAdaptor::~InputAdaptor() {
   free(data->databuffer);
   free(data);
}

int InputAdaptor::GetNext(edge_tp* t) {
    if (data->cur > data->cnt) {
        return -1;
    }
    t->src_ip = *((uint32_t*)data->ptr);
    t->dst_ip = *((uint32_t*)(data->ptr+4));
    data->cur++;
    data->ptr += 8;
    return 1;
}

void InputAdaptor::Reset() {
    data->cur = 0;
    data->ptr = data->databuffer;
}

uint64_t InputAdaptor::GetDataSize() {
    return data->cnt;
}


