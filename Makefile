CC = g++
CFLAGS = -Wall -std=c++11 -pthread -I/usr/local/include -L/usr/local/lib -lprotobuf -pthread
all: reader writer

reader: reader.cc packet.pb.cc
	$(CC) reader.cc packet.pb.cc $(CFLAGS)  -o $@
writer: writer.cc packet.pb.cc
	$(CC) writer.cc packet.pb.cc $(CFLAGS)  -o $@

clean:
	rm -rf main
