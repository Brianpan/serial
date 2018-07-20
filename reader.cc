#include <string>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
//
#include <sstream>
#include <fstream>
#include <iostream>

// thread utils
#include <mutex>
#include <condition_variable>
#include <pthread.h>

// socket utils
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <google/protobuf/message_lite.h>
#include <google/protobuf/util/delimited_message_util.h>
#include "packet.pb.h"

using namespace std;

mutex mtx;
condition_variable cv;
bool worker_flag = false;
bool clean_eof = true;

void *worker_thread(void *args) {
	unique_lock<mutex> lk(mtx);
	cv.wait(lk, []{return worker_flag;});
	worker_flag = false;
	fstream *fs = (fstream*) args; 
	// check there is new data
	while(fs->peek() == EOF);
	clean_eof = true;
	lk.unlock();
	cv.notify_one();
}

// simple google protocol buffer writer
int main(int argc, char **argv) {
	
	packet::PackPacket pack_packet;
	string file = "/home/ubuntu/tcpsniffer/packet.log";

	struct stat buffer;
	if (stat (file.c_str(), &buffer) == 0)
		ofstream o(file);

	fstream fs(file, ios::in);
	
	packet::PackPacket pkt;

	google::protobuf::io::IstreamInputStream zstream(&fs);
	pthread_t worker;
	int worker_pid = pthread_create(&worker, NULL, &worker_thread, (void*) &fs);
	while(true) {
		unique_lock<mutex> lk(mtx);
		printf("start parsing \n");
		while( google::protobuf::util::ParseDelimitedFromZeroCopyStream(&pkt, &zstream, &clean_eof) ) {
			printf("src ip: %s\n", pkt.src_ip().c_str());
			clean_eof = true;
		}
		clean_eof = false;
		worker_flag = true;
		cv.notify_one();
		cv.wait(lk, []{return clean_eof;});
	}
	return 0;
}