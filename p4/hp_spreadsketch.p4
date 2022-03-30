/* -*- P4_14 -*- */
#ifdef __TARGET_TOFINO__
#include <tofino/constants.p4>
#include <tofino/intrinsic_metadata.p4>
#include <tofino/primitives.p4>
//Include the blackbox definition
#include <tofino/stateful_alu_blackbox.p4>
#else
#warning This program is intended for Tofino P4 architecture only
#endif

#define BUCKETS 1024
#define HPFILTER 256

/*--*--* HEADERS *--*--*/
header_type Ethernet {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type Ipv4 {
    fields {
        version : 4;
        ihl : 4;
        diffserv : 8;
        totalLen : 16;
        identification : 16;
        flags : 3;
        fragOffset : 13;
        ttl : 8;
        protocol : 8;
        hdrChecksum : 16;
        srcAddr : 32;
        dstAddr : 32;
    }
}

header_type SSMeta {
    fields {
      //store the level value of current flow
      tmplevel : 8;
      //bit position in the bmp
      pos : 32;
      pos1: 32;
      pos2: 32;
      //store the filter hash
      fhash: 32;
      xorkey: 32;
      passfilter: 32;
      //store the edge hash
      ehash: 32;
      //store the srouce hash
      shash: 32;
      shash1: 32;
      shash2: 32;
      //store the srouce hash
      shifthash: 32;
      shifthash1: 32;
      shifthash2: 32;
      //store the hashed bit position
      hashpos: 32;
      bmpoffset: 32;
      addoffset: 32;
    }
}

header Ethernet ethernet;
header Ipv4 ipv4;
metadata SSMeta meta;


/*--*--* PARSERS *--*--*/
parser start {
    return parse_ethernet;
}

parser parse_ethernet {
    extract(ethernet);
    return parse_ipv4;
}

parser parse_ipv4 {
    extract(ipv4);
    return ingress;
}





/*--*--* REGISTERS *--*--*/
//each multiresolution bitmap contains 128 bits
register ss_bmp {
	width: 1;
	instance_count : 262144;
}

register ss_bmp1 {
	width: 1;
	instance_count : 262144;
}

register ss_bmp2 {
	width: 1;
	instance_count : 262144;
}
//hi_32: srouce key
//lo_32: level value 
register ss_field {
	width: 40;
	instance_count : 2048;
}

register ss_field1 {
	width: 40;
	instance_count : 2048;
}

register ss_field2 {
	width: 40;
	instance_count : 2048;
}
//hi_32: key
//lo_32: indicator
register ss_key {
    width: 64;
    instance_count: 256;
}

/*--*--* Hash *--*--*/
field_list hash_list {
    ipv4.srcAddr;
}

field_list_calculation ss_hash {
    input { hash_list; }
    algorithm : crc_32_bzip2;
    output_width : 11;
}

//used to calculate the level value
field_list edge_list {
    ipv4.srcAddr;
    ipv4.dstAddr;
}

field_list_calculation edge_hash {
    input { edge_list; }
    algorithm : crc_64_we;
    output_width : 32;
}

field_list_calculation filter_hash {
    input { edge_list; }
    algorithm : crc_64_we;
    output_width : 8;
}



/*--*--* actions for all steps *--*--*/

action ac_cal_xorkey() {
    bit_xor(meta.xorkey, ipv4.dstAddr, ipv4.srcAddr);
}

table cal_xorkey {
    actions {
        ac_cal_xorkey;
    }
    default_action: ac_cal_xorkey();
}

blackbox stateful_alu bb_check_pair {
    reg: ss_key;
    condition_lo: register_lo == 0;
    condition_hi: register_hi == meta.xorkey;

    output_predicate: condition_hi;
    output_value: ipv4.srcAddr;
    output_dst: meta.passfilter;	

    //key = (src,dst), count++
	  update_lo_1_predicate: condition_lo or condition_hi;
	  update_lo_1_value: register_lo + 1; 
	  //else count-- and get its abs	
	  update_lo_2_predicate: not condition_lo and not condition_hi;
	  update_lo_2_value: register_lo - 1;
		//register = 0 and key != (src,dst)
	  update_hi_1_predicate: condition_lo and not condition_hi;	
	  update_hi_1_value: meta.xorkey;
    
    initial_register_lo_value: 0;
    initial_register_hi_value: 0;
}

action ac_check_pair() {
    bb_check_pair.execute_stateful_alu(0);
}

table check_pair {
    actions {
        ac_check_pair;
    }
    default_action: ac_check_pair();
}


blackbox stateful_alu bb_update_field {
    reg: ss_field;
    condition_lo: register_lo <= meta.tmplevel;
    
    update_lo_1_predicate: condition_lo;
    update_lo_1_value: meta.tmplevel;

    update_hi_1_predicate: condition_lo;
    update_hi_1_value: ipv4.srcAddr;
}

action ac_update_field() {
    bb_update_field.execute_stateful_alu(meta.shash);
}

table update_field {
    actions {
        ac_update_field;
    }
    default_action: ac_update_field();
}


blackbox stateful_alu bb_update_field1 {
    reg: ss_field1;
    condition_lo: register_lo <= meta.tmplevel;
    
    update_lo_1_predicate: condition_lo;
    update_lo_1_value: meta.tmplevel;

    update_hi_1_predicate: condition_lo;
    update_hi_1_value: ipv4.srcAddr;
}

action ac_update_field1() {
    bb_update_field1.execute_stateful_alu(meta.shash);
}

table update_field1 {
    actions {
        ac_update_field1;
    }
    default_action: ac_update_field1();
}

blackbox stateful_alu bb_update_field2 {
    reg: ss_field2;
    condition_lo: register_lo <= meta.tmplevel;
    
    update_lo_1_predicate: condition_lo;
    update_lo_1_value: meta.tmplevel;

    update_hi_1_predicate: condition_lo;
    update_hi_1_value: ipv4.srcAddr;
}

action ac_update_field2() {
    bb_update_field2.execute_stateful_alu(meta.shash);
}

table update_field2 {
    actions {
        ac_update_field2;
    }
    default_action: ac_update_field2();
}

//update bitmap
blackbox stateful_alu bb_update_bmp {
    reg: ss_bmp;

    update_lo_1_value: set_bitc;
}

action ac_update_bmp() {
    bb_update_bmp.execute_stateful_alu(meta.pos);
}

table update_bmp {
    actions {
        ac_update_bmp;
    }
    default_action: ac_update_bmp();
}

blackbox stateful_alu bb_update_bmp1 {
    reg: ss_bmp1;

    update_lo_1_value: set_bitc;
}

action ac_update_bmp1() {
    bb_update_bmp1.execute_stateful_alu(meta.pos);
}

table update_bmp1 {
    actions {
        ac_update_bmp1;
    }
    default_action: ac_update_bmp1();
}

blackbox stateful_alu bb_update_bmp2 {
    reg: ss_bmp2;

    update_lo_1_value: set_bitc;
}

action ac_update_bmp2() {
    bb_update_bmp2.execute_stateful_alu(meta.pos);
}

table update_bmp2 {
    actions {
        ac_update_bmp2;
    }
    default_action: ac_update_bmp2();
}
//calculate hash value
action ac_cal_hash() {
  modify_field_with_hash_based_offset(meta.ehash, 0, edge_hash, 4294967296);
}

table cal_hash {
  actions {
    ac_cal_hash;
  }
  default_action: ac_cal_hash();
}

//calculate hash for the source node

//filter hash
action ac_cal_fhash() {
    modify_field_with_hash_based_offset(meta.fhash, 0, filter_hash, 256);
}

table cal_fhash {
  actions {
    ac_cal_fhash;
  }
  default_action: ac_cal_fhash();
}

action ac_cal_shash() {
  modify_field_with_hash_based_offset(meta.shash, 0, ss_hash, 2048);
}

table cal_shash {
  actions {
    ac_cal_shash;
  }
  default_action: ac_cal_shash();
}

action ac_cal_shash1() {
  modify_field_with_hash_based_offset(meta.shash1, 0, ss_hash, 2048);
}

table cal_shash1 {
  actions {
    ac_cal_shash1;
  }
  default_action: ac_cal_shash1();
}

action ac_cal_shash2() {
  modify_field_with_hash_based_offset(meta.shash2, 0, ss_hash, 2048);
}

table cal_shash2 {
  actions {
    ac_cal_shash2;
  }
  default_action: ac_cal_shash2();
}

action ac_shift_shash() {
  shift_left(meta.shifthash, meta.shash, 7);
}

table shift_shash {
  actions {
    ac_shift_shash;
  }
  default_action: ac_shift_shash();
}

action ac_shift_shash1() {
  shift_left(meta.shifthash1, meta.shash1, 7);
}

table shift_shash1 {
  actions {
    ac_shift_shash1;
  }
  default_action: ac_shift_shash1();
}

action ac_shift_shash2() {
  shift_left(meta.shifthash2, meta.shash2, 7);
}

table shift_shash2 {
  actions {
    ac_shift_shash2;
  }
  default_action: ac_shift_shash2();
}

action ac_cal_hashpos_64() {
   modify_field(meta.hashpos, meta.ehash, 0x0000003f);
   modify_field(meta.bmpoffset, 64, 0xffffffff);
}

table cal_hashpos_64 {
  actions {
    ac_cal_hashpos_64;
  }
  default_action: ac_cal_hashpos_64();
}

action ac_cal_hashpos_32() {
   modify_field(meta.hashpos, meta.ehash, 0x0000001f);
   shift_left(meta.bmpoffset, meta.tmplevel, 5);
}

table cal_hashpos_32 {
  actions {
    ac_cal_hashpos_32;
  }
  default_action: ac_cal_hashpos_32();
}



action nop() {
}


action ac_set_level(level) {
    modify_field(meta.tmplevel, level, 0xffffffff);
}

table cal_level {
  reads {
    meta.ehash: lpm;
  }
  actions {
    ac_set_level; nop;
  }
  default_action: nop();
}

action ac_cal_pos() {
  bit_or(meta.pos, meta.addoffset, meta.shifthash);
}

table cal_pos {
  actions {
    ac_cal_pos;
  }
  default_action: ac_cal_pos();
}

action ac_cal_pos1() {
  bit_or(meta.pos1, meta.addoffset, meta.shifthash1);
}

table cal_pos1 {
  actions {
    ac_cal_pos1;
  }
  default_action: ac_cal_pos1();
}

action ac_cal_pos2() {
  bit_or(meta.pos2, meta.addoffset, meta.shifthash2);
}

table cal_pos2 {
  actions {
    ac_cal_pos2;
  }
  default_action: ac_cal_pos2();
}

action set_egr(egress_spec) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_spec);
}

table forward {
   reads {
	ig_intr_md.ingress_port : exact;
    } 
    actions {
        set_egr; nop;
    }
    default_action: nop();
}

action ac_add_offset() {
  add(meta.addoffset, meta.hashpos, meta.bmpoffset);
}

table add_offset {
    actions {
        ac_add_offset;
    }
    default_action: ac_add_offset();
}


/*--*--* CONTROL BLOCKS *--*--*/
control ingress {
        apply(cal_fhash);//calculate the pos of HP-Filter
        apply(cal_xorkey);//calculate the xor of src and dst
        apply(check_pair);//calculate filter and check pass
        if(meta.passfilter == ipv4.srcAddr) {
        } else {         
            //apply(test_table);
            //Step 1: calculate the level value
            apply(cal_hash);
            apply(cal_level);
            apply(cal_shash);
            apply(cal_shash1);
            apply(cal_shash2);
            apply(shift_shash);
            apply(shift_shash1);
            apply(shift_shash2);
            //Step 2: calculate the pos in bitmap
            if (meta.tmplevel == 0 or meta.tmplevel == 1) {
              apply(cal_hashpos_32);
            } else {
              apply(cal_hashpos_64);
            }
            apply(add_offset);
            apply(cal_pos);
            apply(cal_pos1);
            apply(cal_pos2);
            //Step 3: update the bitmap
            apply(update_bmp);
            apply(update_bmp1);
            apply(update_bmp2);
            //Step 4: update fields
            apply(update_field);
            apply(update_field1);
            apply(update_field2);
            apply(forward);
        } 
}


control egress {
}

