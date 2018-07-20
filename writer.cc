#include <string>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <sstream>
#include <fstream>
#include <iostream>

#include <google/protobuf/message_lite.h>
#include <google/protobuf/util/delimited_message_util.h>
#include "packet.pb.h"

using namespace std;

// simple google protocol buffer writer
int main(int argc, char **argv) {
	
	packet::PackPacket pack_packet;
	int i = 0;
	fstream fs("data", ios::out);
	while(1) {
		string src_ip  = "192.168.67.3";
		string dest_ip = "192.168.67.4";
		// uint32_t uint type
		uint32_t src_port  = 3306;
		uint32_t dest_port = 10026;
		uint32_t seq       = 102647 + i;
		uint32_t ack_seq   = 22026 + i;

		void *payload = malloc(4096);
		for(int i=0;i<4096;i++) {
			if(i % 1000 == 0) {
				*((char*) payload+i) = 10;
				continue;
			}
			*( (char*) payload + i ) = 0 + i % 97;
		}

		// prepare google buf protocol
		pack_packet.set_src_ip(src_ip);
		pack_packet.set_dest_ip(dest_ip);
		pack_packet.set_src_port(src_port);
		pack_packet.set_dest_port(dest_port);
		pack_packet.set_seq(seq);
		pack_packet.set_ack_seq(ack_seq);
		pack_packet.set_payload(payload, 4096);

		printf("src ip : %s \n", pack_packet.src_ip().c_str());

		if( !google::protobuf::util::SerializeDelimitedToOstream(pack_packet, &fs) ) break;
		i++;
		usleep(2000);
		if(i>3)
			break;
	}

	fs.close();
	return 0;
}