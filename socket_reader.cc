// general utils
#include <string>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

// thread utils
#include <mutex>
#include <condition_variable>
#include <pthread.h>

// socket utils
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
#include <arpa/inet.h>


// protocol buf utils
#include <google/protobuf/message_lite.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "packet.pb.h"

#define HOST_SOCKET_PORT 1234
#define PROTOCOL_PAYLOAD_SIZE_SIZE 4

using namespace std;
using namespace google::protobuf::io;

// payload size is var int
google::protobuf::uint32 readHdr(char *buf) {
	google::protobuf::uint32 payload_size;

	google::protobuf::io::ArrayInputStream ais(buf, PROTOCOL_PAYLOAD_SIZE_SIZE);
	CodedInputStream coded_input(&ais);
	coded_input.ReadVarint32(&payload_size);
	printf("payload size is : %u\n", payload_size);
	return payload_size;
}

void worker(int csock, int buffer_size) {
	while(1) {
		// parse packet
		char packet_buffer[buffer_size];
		int byte_count;
		packet::PackPacket pkt;
		google::protobuf::uint32 payload_size;

		if((byte_count = recv(csock, (void *)packet_buffer, buffer_size, MSG_WAITALL))== -1){
			printf("recv data error !%d\n", errno);
		} 
		else {
			google::protobuf::io::ArrayInputStream ais(packet_buffer, buffer_size);
			CodedInputStream coded_input(&ais);
			//Read an unsigned integer with Varint encoding, truncating to 32 bits.
			coded_input.ReadVarint32(&payload_size);
			//After the message's length is read, PushLimit() is used to prevent the CodedInputStream 
			//from reading beyond that length.Limits are used when parsing length-delimited 
			//embedded messages
			google::protobuf::io::CodedInputStream::Limit msgLimit = coded_input.PushLimit(payload_size);
			//De-Serialize
			pkt.ParseFromCodedStream(&coded_input);
			//Once the embedded message has been parsed, PopLimit() is called to undo the limit
			coded_input.PopLimit(msgLimit);
			//Print the message
			printf("Message is %s \n", pkt.DebugString().c_str());
			break;
		}
	}
}

void *socketHandler(void *args) {
	int csock = *((int *)args);
	char byte_buffer[PROTOCOL_PAYLOAD_SIZE_SIZE];
	memset(byte_buffer, 0, PROTOCOL_PAYLOAD_SIZE_SIZE);
	int byte_count;

	while(1) {
		// MSG_PEEK means the data is treated as unread and next recv shall recv this data
		if( (byte_count = recv(csock, byte_buffer, PROTOCOL_PAYLOAD_SIZE_SIZE, MSG_PEEK)) == -1 ) {
			printf("recv data error !\n");
			exit(1);
		}

		if(byte_count == 0)
			break;

		google::protobuf::uint32 payload_size = readHdr(byte_buffer);
		printf("payload size is : %u\n", payload_size);
		int buffer_size = payload_size + PROTOCOL_PAYLOAD_SIZE_SIZE;
		worker(csock, buffer_size);
	}
}

int streamSocket() {
	int sock;
	int sock_setting = 1;
	struct sockaddr_in haddr;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0) {
		printf("create socket error !\n");
		exit(1);
	}
	// set socket opts
	if( setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*) &sock_setting, sizeof(int)) == -1 ||
		setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char*) &sock_setting, sizeof(int)) == -1 ) {
		printf("set socketopts error !\n");
		exit(1);
	}

	haddr.sin_family = AF_INET;
	haddr.sin_port = htons(HOST_SOCKET_PORT);
	memset(&(haddr.sin_zero), 0, 8);
	haddr.sin_addr.s_addr = INADDR_ANY;

	if(bind(sock, (sockaddr*) &haddr, sizeof(haddr)) == -1) {
		printf("bind to socket error !\n");
		exit(1);
	}
	
	if(listen(sock, 10) == -1) {
		printf("listen to socket error !\n");
		exit(1);
	}

	printf("create socket success !, %d\n", sock);
	return sock;
}

// simple google protocol buffer writer
int main(int argc, char **argv) {
	// create socket
	int hsock = streamSocket();
	socklen_t addr_size = sizeof(sockaddr_in);;
	sockaddr_in sadr;
	int *csock = (int*) malloc(sizeof(int));

	printf("start parsing \n");
	
	while(true) {
		if((*csock = accept( hsock, (sockaddr*)&sadr, &addr_size)) == -1) continue;
		printf("---------------------\nReceived connection from %s\n",inet_ntoa(sadr.sin_addr));
		pthread_t handler_thread;
		int handler_pid = pthread_create(&handler_thread, NULL, &socketHandler, (void*) csock);
		pthread_detach(handler_pid);
	}

	free(csock);
	return 0;
}