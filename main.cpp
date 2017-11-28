//#define _XOPEN_SOURCE 600
///////////////////////
// C++ Headers
///////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>
#include <fstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <iomanip>
#include <pthread.h>
#include <map>
#include <queue>
#include <inttypes.h>
#include <unordered_set>
using namespace std;
///////////////////////
// User Headers
///////////////////////
#define MAX_CONNECTIONS 60000
#include "HTTPParser.h"
#include "servercache.h"
#include "log.h"
//#define NON_BLOCKING_SOCKET_IO

///////////////////////
// Globals
///////////////////////

unsigned int ACTIVE_THREADS = 200;
unsigned int WAIT_THREADS = 100;
queue<int> WaitQ;
unsigned int ACTIVE_THREADS_CNT;
unsigned int WAIT_THREADS_CNT;
pthread_cond_t Thread_cond;
pthread_mutex_t Req_Handler_mutex;
const static int SUCCESS = 0;
const static int FAILURE = 0;
string Root_Path;

unsigned int Socket_Timeout = 30; // in seconds
unsigned int Recv_Timeout = 20; // in seconds

static unordered_set<string> supported_ft = {
    "jpg", "png", "gif", "pdf", "htm", "html",
};
///////////////////////
// Functions
///////////////////////
int send_tcp_data(int sockfd, const char* buf, size_t size) {
    size_t rem = size;
    size_t total = 0;
    int c;
    int pkt_size;
    while(total < size) {
        pkt_size = (rem < 4096) ? rem : 4096;
        c = send(sockfd, buf + total, pkt_size, MSG_NOSIGNAL);
        if (c < 0) return -1;
        total += c;
        rem -= c;
    }
    return 0;
}
void* Req_Handler(void * sock) {

    intptr_t req_socket = (intptr_t) sock;
    char* file_buffer;
    string HttpResp_Hdr;
    string req_file_path, not_found_path;
    HttpResp_Header RespHeader;
    HttpReq_Header ReqHeader;
    LOG_INFO("REQUEST: %ld", req_socket);
    char read_buffer[1024];
    int count;
    int r;

#ifdef HTTP_V11
    timeval recv_timeout;
    recv_timeout.tv_sec = Recv_Timeout; // 10 seconds
    recv_timeout.tv_usec = 0;

    if (setsockopt(req_socket, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout)) < 0){
        cout << "Error: setting options for socket:" << req_socket << ". Error No: " << strerror(errno) << endl;
    }

    // Set accept to non blocking
    //if(fcntl(req_socket, F_SETFL, fcntl(req_socket, F_GETFL) | O_NONBLOCK) < 0) {
    //    cout << "Error setting recv to non-blocking" << endl;
    //}
#endif

    bool wait_next = true;
    while (wait_next) {
        bzero(read_buffer, 1024);
        count = recv(req_socket, read_buffer, 1023, 0);
        if (count < 0) {
            LE("Receiving request header");
            break;
        } else {
            LOG_INFO("Read bytes: %d", n);
            LOG("%s", read_buffer);

            if (not HttpReq_Parser(&ReqHeader, string(read_buffer))) {
                LE("Invalid Request header %s", read_buffer);
                break;
            }
            if (ReqHeader.path == "/")
                ReqHeader.path = "/index.html";

            req_file_path = Root_Path + ReqHeader.path;
            not_found_path = Root_Path + "/Not_Found.html";

            string ext = req_file_path.substr(req_file_path.find_last_of(".") + 1);
            RespHeader.contType = get_mime_type(ext);

            if (supported_ft.find(ext) == supported_ft.end()) {
                req_file_path = not_found_path;
            }
            // ////////////////////////
            // READ FILE
            // ////////////////////////
            unsigned int file_size = 0;
            file_buffer = file_read(&req_file_path, &file_size);
            if (file_buffer != nullptr) {
                RespHeader.status_code = "200";
                RespHeader.status_string = "OK";
            } else {
                file_buffer = file_read(&not_found_path, &file_size);
                RespHeader.status_code = "404";
                RespHeader.status_string = "Not Found";
                if (file_buffer == nullptr) {
                    char NotFound[] = "<!DOCTYPE html>\n<html><body><h1>Not Found</h1>\n<p>Sorry, the requested path was not found.</p>\n</body></html>\n";
                    file_buffer = NotFound;
                    file_size = sizeof(NotFound);
                }
            }
            // ////////////////////////
            // BUILD RESPONSE
            // ////////////////////////
            RespHeader.version = ReqHeader.version;
            RespHeader.contLength = to_string(file_size);
            RespHeader.connection = ReqHeader.connection;
            HttpResp_Hdr = HttpResp_Builder(&RespHeader);
            // ////////////////////////
            // SEND Header
            // ////////////////////////
            r = send_tcp_data(req_socket, HttpResp_Hdr.c_str(), HttpResp_Hdr.length());
            if (r) {
                LE("Sending response header");
                break;
            }
            r = send_tcp_data(req_socket, file_buffer, file_size);
            if (r) {
                LE("Sending data");
                break;
            }
#ifdef CACHE
            file_close(req_file_path);
#else
            file_close(file_buffer);
#endif

            if (RespHeader.connection == "close")
                break;
        }
    }
    LOG_INFO("REQ HANDLER %ld THREAD KILLED", Req_Socket);
    close(req_socket);
    pthread_exit(NULL);
}
void * SocketIO (void * port_no){
    int portno = (intptr_t) port_no;
    int Req_Count = 0;
    int sockfd, newsockfd, Err;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    // Create your TCP socket
    sockfd =  socket(AF_INET, SOCK_STREAM, 0);
    // The commented code below would create a UDP socket
    // sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)
    if (sockfd < 0) {
        cout << "Error: error creating socket for listening" << endl;
        cout << "SOCKETIO THREAD KILLED" << endl;
        pthread_exit(NULL);
    }

    // Setup your server's socket address structure
    // Clear it out, and set its parameters
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;  // use IPv4
    serv_addr.sin_addr.s_addr = INADDR_ANY; // fill in server IP address
    serv_addr.sin_port = htons(portno); // convert port number to network byte order and set port

    //cout <<serv_addr.sin_addr.s_addr;
    // Bind socket to IP address and port on server
    if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        cout << "Error: error binding an IP address and port on the server to our socket" << endl;
        cout << "SOCKETIO THREAD KILLED" << endl;
        pthread_exit(NULL);
    }

    // Listen on the socket for new connections to be made
    listen(sockfd, MAX_CONNECTIONS);

    // The accept() call actually accepts an incoming connection
    clilen = sizeof(cli_addr);

#ifdef NON_BLOCKING_SOCKET_IO
    if(fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK) < 0) {
        cout << "Error setting socket to non-blocking" << endl;
    }

    // Check settings again        
    if(!(fcntl(sockfd, F_GETFL) & O_NONBLOCK))
        LOG_INFO("Blocking socket IO");
    else
        LOG_INFO("Non-blocking socket IO");
#endif

    // accept a connection from a client, returns a new client socket for communication with client
    while (true) {
        //pthread_mutex_lock(&Req_Handler_mutex);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            //LE("Error creating client socket");
        } else {
            LOG_INFO("New connection from %s on port %" PRId16, 
                    inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            pthread_t* tid = (pthread_t*) malloc(sizeof(pthread_t));
            Err = pthread_create(tid, NULL, Req_Handler, (void *) (intptr_t) newsockfd);
            pthread_detach(*tid);
            free(tid);
            if(Err)
                LE("ERROR, in creating req thread. Function exited with %d", Err);
            Req_Count++;
        }
        //pthread_mutex_unlock(&Req_Handler_mutex);
    }
    LOG_INFO("SOCKETIO THREAD KILLED");
}
int main(int argc, char *argv[]) {
    pthread_t SocketIO_t;
    int port = 9000;
    int Err;
    if (argc < 5) { 
        LE("Usage is %s -dir <path> -port <num>", argv[0]);
        exit(EXIT_FAILURE);
    } else { 
        for (int i = 1; i < argc; i=i+2){
            string option = string(argv[i]);
            string nextoption = string(argv[i+1]);
            //cout << option << nextoption << endl;
            if (option == "-dir")
                Root_Path = nextoption;
            else {
                if (option == "-port")
                    port = stoi(nextoption,nullptr,10);
            }
        }
        if ((port == 0) || (Root_Path.empty())) {
            cout << "Not enough or invalid arguments." << endl;
            exit(EXIT_FAILURE);
        }
    }

    LI("Server started at path %s, port = %d", Root_Path.c_str(), port);

    Err = pthread_create(&SocketIO_t, NULL, SocketIO, (void *) (intptr_t) port);
    if (Err)
        cout << "ERROR, in creating socketIO thread. Error: " << Err << endl;
    pthread_exit(NULL);
}
