/* -* P4_16 -*- */
/* * This is the P4_16 impelementation of SpreadSketch. The parameter setting of
 * the sketch is as follows:
 * depth = 3
 * with = 2048
 * multiresolution bitmap: c = 3, b = 32, m = 128
 * 
 */

#include <core.p4>
#include <v1model.p4>

#define BUCKET 32w2048

const bit<16> TYPE_IPV4 = 0x800;

/*************************************************************
 *****************   HEADERS *********************************
 **************************************************************/

typedef bit<9> egressSpec_t;
typedef bit<48> macAddr_t;
typedef bit<32> ip4Addr_t;

header ethernet_t {
    macAddr_t dstAddr;
    macAddr_t srcAddr;
    bit<16> etherType;
}


header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    ip4Addr_t srcAddr;
    ip4Addr_t dstAddr;
}

//define register arrays
// each multiresolution bitmap contains 128 bits
register<bit<1>>(262144) ss_bmp;
register<bit<1>>(262144) ss_bmp1;
register<bit<1>>(262144) ss_bmp2;

//key fields
register<bit<32>>(2048) ss_key;
register<bit<32>>(2048) ss_key1;
register<bit<32>>(2048) ss_key2;

//level fields
register<int<8>>(2048) ss_level;
register<int<8>>(2048) ss_level1;
register<int<8>>(2048) ss_level2;

struct metadata {
    bit<32> hash;
    bit<32> hash1;
    bit<32> hash2;
    bit<32> tempkey;
    int<8> templevel;
    int<32> bmp_pos; 
    int<32> bmp_offset; 
    int<32> bmp_index;
    int<32> bmp_index1;
    int<32> bmp_index2;
    int<32> bmp_array_offset;
    int<32> bmp_array_offset1;
    int<32> bmp_array_offset2;
    int<32> bmp_hash;
    bit<32> hashstr;
    int<8> read_level;
}

struct headers {
    ethernet_t ethernet;
    ipv4_t ipv4;
}


/*********************************************************
 ***************** PARSER *********************************
 **********************************************************/

parser MyParser(packet_in packet, 
        out headers hdr,
        inout metadata meta,
        inout standard_metadata_t standard_metadata) {

    state start {
        transition parse_ethernet;
    }

    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
TYPE_IPV4: parse_ipv4;
            default: accept;
        }
    }

    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition accept;
    }
}


/*********************************************************
 ***************** CHECKSUM VERIFICATION *******************
 **********************************************************/

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply { }
}

/*********************************************************
 ***************** INGRESS PROCESSING *******************
 **********************************************************/

control MyIngress(inout headers hdr, 
        inout metadata meta,
        inout standard_metadata_t standard_metadata) {
    action drop() {
        mark_to_drop();
    }

    //forward all packets to the specified port
    action set_egr(egressSpec_t port) {
        standard_metadata.egress_spec = port;
    }

    //action: calculate hash functions
    //hash srouce to buckets
    action compute_bucket_hash() {
        hash(meta.hash, HashAlgorithm.crc32, 32w0, {hdr.ipv4.srcAddr, 7w11}, BUCKET); 
        hash(meta.hash1, HashAlgorithm.crc32, 32w0, {3w5, hdr.ipv4.srcAddr, 5w3}, BUCKET);
        hash(meta.hash2, HashAlgorithm.crc32, 32w0, {2w0, hdr.ipv4.srcAddr, 1w1}, BUCKET);
    }

    // hash destination to bitmaps
    action compute_hash_string() {
        hash(meta.hashstr, HashAlgorithm.crc32, 32w0, {hdr.ipv4.srcAddr, hdr.ipv4.dstAddr}, 32w0xffffffff);
    }


    //action: calculate level value
    action compute_level(int<8> level) {
        meta.templevel = level;	

    }

    table cal_level {
        key = {
            meta.hashstr: lpm;	
        }
        actions = {
            compute_level;
        }
    }

    //action: calculate bmpindex
    action compute_bmp_offset_32() {
        //get the offset of multiresolution bitmp
        meta.bmp_offset = (int<32>)meta.templevel << 5;
        //get the hash position
        meta.bmp_hash = (int <32>)meta.hashstr & 0x0000001f;
    } 

    action compute_bmp_offset_64() {
        meta.bmp_offset = 64;
        //get the hash position
        meta.bmp_hash = (int <32>)meta.hashstr & 0x0000003f;
    } 

    action compute_bmp_pos() {
        meta.bmp_pos = (int <32>)meta.bmp_offset + (int <32>)meta.bmp_hash;
    }

    action compute_bmp_array_offset() {
        meta.bmp_array_offset = (int <32>)meta.hash << 7; 
        meta.bmp_array_offset1 = (int <32>)meta.hash1 << 7; 
        meta.bmp_array_offset2 = (int <32>)meta.hash2 << 7; 
    }

    action compute_bmp_index() {
        meta.bmp_index = meta.bmp_pos + meta.bmp_array_offset; 
        meta.bmp_index1 = meta.bmp_pos + meta.bmp_array_offset1; 
        meta.bmp_index2 = meta.bmp_pos + meta.bmp_array_offset2; 
    }


    table forward {
        key = {
            standard_metadata.ingress_port: exact;
        }
        actions = {
            set_egr;
            drop;
        }
        size = 1024;
        default_action = drop();
    }

    //calculate bucket hash values
    table hash_index {
        actions = {
            compute_bucket_hash;      
        }
        default_action = compute_bucket_hash();
    }

    table hash_bmp {
        actions = {
            compute_hash_string;      
        }
        default_action = compute_hash_string();
    }


    table bmp_offset_32 {
        actions = {
            compute_bmp_offset_32;      
        }
        default_action = compute_bmp_offset_32();
    }

    table bmp_offset_64 {
        actions = {
            compute_bmp_offset_64;      
        }
        default_action = compute_bmp_offset_64();
    }

    table bmp_pos {
        actions = {
            compute_bmp_pos;      
        }
        default_action = compute_bmp_pos();
    }

    table bmp_array_offset {
        actions = {
            compute_bmp_array_offset;      
        }
        default_action = compute_bmp_array_offset();
    }

    table bmp_index {
        actions = {
            compute_bmp_index;      
        }
        default_action = compute_bmp_index();
    }


    apply {
        if (hdr.ipv4.isValid()) {

            //1. calculate hash values 
            //calculate the bucket index	
            hash_index.apply();
            //calculate a random hash string for the (src, dst) pair
            hash_bmp.apply(); 
            //calculate the level value for the hash string
            cal_level.apply();

            //calculate the position of (src, dst) in bitmap
            if (meta.templevel == 0 || meta.templevel == 1) {
                bmp_offset_32.apply();	
            } else {
                bmp_offset_64.apply();
            }
            bmp_pos.apply();
            bmp_array_offset.apply();
            bmp_index.apply();

            //2. update the first row of SpreadSketch
            //perform update 
            ss_bmp.write((bit<32>)meta.bmp_index, (bit<1>)1);
            ss_level.read(meta.read_level, meta.hash);
            if (meta.read_level <= meta.templevel) {
                ss_key.write(meta.hash, hdr.ipv4.srcAddr);
                ss_level.write(meta.hash, meta.templevel);
            }

            //3. update the second row of SpreadSketch
            ss_bmp1.write((bit<32>)meta.bmp_index1, (bit<1>)1);
            ss_level1.read(meta.read_level, meta.hash1);
            if (meta.read_level <= meta.templevel) {
                ss_key1.write(meta.hash1, hdr.ipv4.srcAddr);
                ss_level1.write(meta.hash1, meta.templevel);
            }

            //4. update the third row of SpreadSketch
            ss_bmp2.write((bit<32>)meta.bmp_index2, (bit<1>)1);
            ss_level2.read(meta.read_level, meta.hash2);
            if (meta.read_level <= meta.templevel) {
                ss_key2.write(meta.hash2, hdr.ipv4.srcAddr);
                ss_level2.write(meta.hash2, meta.templevel);
            }

            forward.apply(); 
        }
    }
}



/*********************************************************
 ***************** EGRESS PROCESSING *******************
 **********************************************************/

control MyEgress(inout headers hdr,
        inout metadata meta,
        inout standard_metadata_t standard_metadata) {
    apply { }
}

/*********************************************************
 ***************** CHECKSUM COMPUTATION *******************
 **********************************************************/

control MyComputeChecksum(inout headers hdr, inout metadata meta) {
    apply{
        update_checksum(
                hdr.ipv4.isValid(),
                { hdr.ipv4.version,
                hdr.ipv4.ihl,
                hdr.ipv4.diffserv,
                hdr.ipv4.totalLen,
                hdr.ipv4.identification,
                hdr.ipv4.flags,
                hdr.ipv4.fragOffset,
                hdr.ipv4.ttl,
                hdr.ipv4.protocol,
                hdr.ipv4.srcAddr,
                hdr.ipv4.dstAddr},
                hdr.ipv4.hdrChecksum,
                HashAlgorithm.csum16);
    }
}


/*********************************************************
 ***************** DEPARSER *******************************
 **********************************************************/

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
    }
}


/*********************************************************
 ***************** SWITCH *******************************
 **********************************************************/

    V1Switch(
            MyParser(),
            MyVerifyChecksum(),
            MyIngress(),
            MyEgress(),
            MyComputeChecksum(),
            MyDeparser()
            ) main;

