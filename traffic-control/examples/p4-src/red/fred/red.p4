/* -*- P4_16 -*- */
#include <core.p4>
//#include <v1model.p4>
#include "simple_pipe.p4"

typedef bit<32> QueueDepth_t;
const bit<16> TYPE_IPV4 = 0x800;
#define NUM_FLOW_ENTRIES 512

// This value specifies size for table calc_red_drop_probability.
const bit<32> NUM_RED_DROP_VALUES = 1<<16; // 2^16

/*************************************************************************
*********************** H E A D E R S  ***********************************
*************************************************************************/

struct metadata {
    /* empty */
}

typedef bit<48>  EthernetAddress;

header Ethernet_h {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

struct Parsed_packet {
    Ethernet_h    ethernet;
}

struct headers {
    /* empty */
}

/*************************************************************************
*********************** P A R S E R  ***********************************
*************************************************************************/

parser MyParser(packet_in packet,
                out headers hdr,
                inout metadata meta,
                inout standard_metadata_t standard_metadata) {

    state start {
        pkt.extract(hdr.ethernet); // We need ethernet data to generate hash to flow identification
        transition accept;
    }

}

/*************************************************************************
************   C H E C K S U M    V E R I F I C A T I O N   *************
*************************************************************************/

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {   
    apply {  }
}


/*************************************************************************
**************  I N G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control MyIngress(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {

    bit<9> drop_prob;
	
	FlowID_t flow_id;

	
	register<QueueDepth_t>(NUM_FLOW_ENTRIES) qlen; // queue length per flow
	QueueDepth_t qlen_old;
	QueueDepth_t qlen_new;
	QueueDepth_t size_to_add;
	register<QueueDepth_t>(NUM_FLOW_ENTRIES) strike;// strike per flow
	QueueDepth_t stiker_old;
	QueueDepth_t strike_new;

    action set_drop_probability (bit<9> drop_probability) {
        drop_prob = drop_probability;
    }

    table calc_red_drop_probability {
        key = {
            standard_metadata.avg_qdepth : exact;
        }
        actions = {
            set_drop_probability;
        }
        const default_action = set_drop_probability(0);
        size = NUM_RED_DROP_VALUES;
    }
    
    apply {
		flow_id = standard_metadata.flow_hash;
		size_to_add = standard_metadata.pkt_len;
		
		qlen.read(qlen_old, flow_id);
		qlen_new = qlen_old |+| size_to_add;
		qlen.write(flow_id, qlen_new)
		
        calc_red_drop_probability.apply();
        bit<8> rand_val;
        random<bit<8>>(rand_val, 0, 255);
        // Of course someone might choose to do RED on multicast
        // packets in some way, too.  I am just avoiding the question
        // of what that might mean for this example.  One might also
        // wish to only apply RED for TCP packets, in which case the
        // 'if' condition would need to be changed to specify that.

        // drop_prob=0 means never do RED drop.  drop_prob in the
        // range [1, 256] means to drop a fraction (drop_prob/256) of
        // the time.  It is 9 bits in size so we can represent the
        // "always drop" case, desirable when the average queue depth
        // is high enough.
        if ( (bit<9>) rand_val < drop_prob) {
            standard_metadata.drop = 1;
        }
    }
}

/*************************************************************************
****************  E G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control MyEgress(inout headers hdr,
                 inout metadata meta,
                 inout standard_metadata_t standard_metadata) {
					 
	register<QueueDepth_t>(NUM_FLOW_ENTRIES) qlen; // queue length per flow
	QueueDepth_t qlen_old;
	QueueDepth_t qlen_new;
	QueueDepth_t size_to_remove;
	
    apply { 
		flow_id = standard_metadata.flow_hash;
		size_to_remove = standard_metadata.pkt_len;
		
		qlen.read(qlen_old, flow_id);
		qlen_new = qlen_old |-| size_to_remove;
		qlen.write(flow_id, qlen_new)
		}
}

/*************************************************************************
*************   C H E C K S U M    C O M P U T A T I O N   **************
*************************************************************************/

control MyComputeChecksum(inout headers  hdr, inout metadata meta) {
     apply { }
}

/*************************************************************************
***********************  D E P A R S E R  *******************************
*************************************************************************/

control MyDeparser(packet_out packet, in headers hdr) {
    apply { }
}

/*************************************************************************
***********************  S W I T C H  *******************************
*************************************************************************/

V1Switch(
MyParser(),
MyVerifyChecksum(),
MyIngress(),
MyEgress(),
MyComputeChecksum(),
MyDeparser()
) main;
