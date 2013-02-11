/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
//#include <unordered_map>
#include <vector>
#include <fcntl.h>
#include <stdio.h>
#include <fstream>
#include <utility>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include "http-request.h"
#include <netdb.h>
using namespace std;

void print_error(string error) {
    cerr << error << endl;
}

void send_outgoing(int clientfd, HttpRequest req) {
//Fork now to create child and parent thread
    int pid = fork();
    if (pid == 0)
    {
        struct hostent *hp;     
        struct sockaddr_in servaddr;   

        /* fill in the server's address and data */
        memset((char*)&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(req.GetPort());
        const char* host = req.GetHost().c_str();
        /* look up the address of the server given its name */
        hp = gethostbyname(host);
        if (!hp) {
            fprintf(stderr, "could not obtain address of %s\n", host);
            return;
        }

        /* put the host's address into the server address structure */
        memcpy((void *)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);


        struct sockaddr_in my_addr;    
        int reqfd = socket(AF_INET, SOCK_STREAM, 0);
        memset((char*)&my_addr, 0, sizeof(my_addr));  
        my_addr.sin_family = AF_INET;   
        my_addr.sin_port = htons(0);
        my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        
        if (bind(reqfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
	        print_error("bind failed");
     	    exit(1);
        }

        /* connect to server */
        if (connect(reqfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
            print_error("connect failed");
        } else {
            int sizeOfBuffer = req.GetTotalLength();
            char* reqbuffer = (char* ) malloc(sizeOfBuffer);
            req.FormatRequest(reqbuffer);
            int byteWrite = 0;
            while((byteWrite += (write(reqfd, reqbuffer, sizeOfBuffer))) != 0) {
                reqbuffer += byteWrite;
            }
            sizeOfBuffer = 1024;
            char* readbuffer = (char* ) malloc(sizeOfBuffer);
            char* readIn = readbuffer;
            int byteRead = 0;
            while((byteRead += (read(reqfd, readIn, 1024))) != EOF) {
                if (byteRead > sizeOfBuffer) {
                    readbuffer = (char*) realloc(readbuffer, sizeOfBuffer * 2);
                    readIn = readbuffer + sizeOfBuffer;
                    sizeOfBuffer *= 2;
                }
            }
            readbuffer[byteRead] = 0;
            byteWrite = 0;
            while((byteWrite += (write(clientfd, readbuffer, sizeOfBuffer))) != 0) {
                readbuffer += byteWrite;
            }
/*
            string url = req.GetHost() + req.GetPath();
            ofstream* cachefile;
            cachefile->open(url.c_str(), ios_base::trunc);
            cachefile->

            cachefile->close();
*/
        }
    }
}   

void make_server() {
//Initial setup, create hashmap for cache, to keep track of requests already asked for
//Create hashset for requests in progress, do not ask for same request if previous one is already running
    //unordered_map<string, string> cache;
    vector<pair<int, string> > waiting;
    char* buffer;
    int port = 14805;	 
    int rqst;       
    socklen_t alen;       
    struct sockaddr_in my_addr;    
    struct sockaddr_in client_addr;  
    int sockoptval = 1;
    int svc;

	// create a socket
    if ((svc = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	    print_error("cannot create socket");
        exit(1);
    }

	// reuse the port
    setsockopt(svc, SOL_SOCKET, SO_REUSEADDR, &sockoptval, sizeof(int));

        // bind socket to address
    memset((char*)&my_addr, 0, sizeof(my_addr));  
    my_addr.sin_family = AF_INET;   
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(svc, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
	    print_error("bind failed");
     	exit(1);
    }

	// listen with backlog of 10
    if (listen(svc, 10) < 0) {
	    print_error("listen failed");
	    exit(1);
    }

	// loop, accepting connection requests 
	// If accepted, read in to buffer and parse the Request into an HTTPRequest object
    for (;;) {
        while ((rqst = accept(svc, 
               (struct sockaddr *)&client_addr, &alen)) < 0) {
	/* break out on error and try again*/
            if ((errno != ECHILD) && (errno != ERESTART) && (errno != EINTR)) {
                 print_error("accept failed");
                 exit(1);
            }
            int sizeOfBuffer = 1024;
            buffer = (char* ) malloc(sizeOfBuffer);
            char* readIn = buffer;
            int byteRead = 0;
            while((byteRead += (read(rqst, readIn, 1024))) != EOF) {
                if (byteRead > sizeOfBuffer) {
                    buffer = (char*) realloc(buffer, sizeOfBuffer * 2);
                    readIn = buffer + sizeOfBuffer;
                    sizeOfBuffer *= 2;
                }
            }

            HttpRequest req;

            // TODO: CATCH EXCEPTIONS
            req.ParseRequest(buffer, strlen(buffer));
                /*
            string url = req.GetHost() + req.GetPath();
            if (cache.find(url) != cache.end()) {
                int byteWrite = 0;
                while((byteWrite += (write(reqfd, reqbuffer, sizeOfBuffer))) != 0) {
                    reqbuffer += byteWrite;
                }
                
                continue;
            }

            for (int i = 0; i < waiting.size(); i++) {
                string url = waiting[i].second;
                ifstream file;
                file.open(url);
                if (file.is_open()) {
                    file.seekg(0, ios::end);
                    length = file.tellg();
                    file.seekg(0, ios::beg);

                    char* buffer = new char[length];
                    file.read(buffer, length);
                    cache.insert(Pair(url, string(buffer)));
                    int byteWrite = 0;
                    while((byteWrite += (write(waiting[i].first, reqbuffer, sizeOfBuffer))) != 0) {
                        reqbuffer += byteWrite;
                    delete[] buffer;
                }
            }
            */

            send_outgoing(rqst, req);
            //inProgress.insert(url);
        }
		/* the socket for this accepted connection is rqst */
    }
}

int main (int argc, char *argv[])
{
	make_server();  
    return 0;
}

