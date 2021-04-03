#ifndef __NETPROBE_CORE__
#define __NETPROBE_CORE__

#pragma once
#pragma warning(disable:4996)   //Disable the warning for depracated functions

#include <stdio.h>
#include <string.h> //memset
#include <sstream>  //ostringstream
#include <iomanip>
#include <ctime>
#include <iostream>
#include <limits.h>
#include <chrono> 
#include <thread>
#include <algorithm>    //min_element()
#include <math.h>       //Ceil()

/*
* If [DEFAULT_NCH] is being modified,
* [DEFAULT_NCH_LEN] should also change
*/
#define DEFAULT_NCH "NCH:"                  //The default starting string of a NetprobeHeader
#define DEFAULT_NCH_LEN 5 - 1               //The length of default starting string, do not use strlen() to replace this constant 
                                            //Actually, we can use [constexpr] to calcuate the len at the compile stage (C11 features)

#define DEFAULT_CONFIG_HEADER_SIZE 45 + DEFAULT_NCH_LEN   // The size of NetprobeHeader is set to 45+4 Bytes

#define NETPROBE_UDP_MODE 1                 //Netprobe UDP protocol
#define NETPROBE_TCP_MODE 2                 //Netprobe TCP protocol
#define NETPROBE_NULL_PROTO 3               //Placeholder netprobe protocol reserved for server use only

#define NETPROBE_SEND_MODE 1                //Netprobe Sender mode
#define NETPROBE_RECV_MODE 2                //Netprobe Receiver mode
#define NETPROBE_HOST_MODE 3                //Depracted mode
#define NETPROBE_SERV_PLACEHOLDER_MODE 4    //Placeholder netprobe mode reserved for server use only
#define NETPROBE_RESP_MODE 5                //Netprobe Response mode

#define NETPROBE_SERVMODEL_SELECT 1     //Using select() server model
#define NETPROBE_SERVMODEL_THEDPL 2     //Using threadpool server model

#define InfinityLoop while(1)           //Same as while(1)/for(;;) , just a nicer name.

#if _WIN32
    #define OSISWINDOWS true    // True when os is a windows system, false when it is a linux(Not 100% confirm)
    #include <winsock2.h>
    #pragma comment (lib, "ws2_32.lib")
#else   //_Linux_ 
    #define OSISWINDOWS false   // True when os is a windows system, false when it is a linux(Not 100% confirm)
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <sys/ioctl.h>
    #include <netinet/in.h>
    #include <netdb.h>
    #include <stdlib.h> //exit(0);
    #include <sched.h>      //Scheduling API
    #define ZeroMemory(Destination,Length) memset((Destination),0,(Length))  //The easier memset 0 method inheritanced from windows c++
    #define SOCKET_ERROR -1
    #define INVALID_SOCKET ~0
    typedef int SOCKET;
    char* strlwr(char* s)   //This function is not a standard linux C function
    {
        char* str;
        str = s;
        while (*str != '\0')
        {
            if (*str >= 'A' && *str <= 'Z') {
                *str += 'a' - 'A';
            }
            str++;
        }
        return s;
    }
#endif

using namespace std;

/**
 * @brief The configuartion of an NetProbe object
*/
class NetProbeConfig {
public:
    int mode;   // send mode = 1, recv mode = 2, host mode = 3, 4 is reserved for server use, 5 is response mode
    int stat_displayspeed_perms;
    string rhostname;
    int rport;
    int protocol; // Use UDP for default , 1 is UDP, 2 is TCP, 3 is reserved for server use
    int server_model;   //Use Select() as default, 1 is Select(), 2 is threadpool
    int poolsize;   //Only used for thread-pool server model.   //maximum is 9999 since we pass it as a 4 digits string
    int pkt_size_inbyte;
    int pkt_rate_bytepersec; // "0" = as fast as possible
    int pkt_num; // Default "0" = infiinite
    int sbufsize_inbyte;
    string lhostname;
    int lport;
    int rbufsize_inbyte;
    string hhostname;

    NetProbeConfig() {
        //Initailize variables
        mode = 0;
        stat_displayspeed_perms = 500;
        rhostname = "localhost";
        rport = 4180;
        protocol = NETPROBE_TCP_MODE;
        pkt_size_inbyte = 1000;
        pkt_rate_bytepersec = 1000;
        pkt_num = 0;
        sbufsize_inbyte = 0;
        lhostname = "INADDR_ANY";
        lport = 4180;
        rbufsize_inbyte = 0;
        hhostname = "localhost";
        server_model = 1;
        poolsize = 8;
    }
};

/**
 * @brief Convert some int value into a string
 * @param int value
 * @return string
*/
string intToString(int value) {
    ostringstream strs;
    strs << value;
    return strs.str();
}

/**
 * @brief Prepend the character before the string (default to prepend a digit 0)
 * @param str - the target string
 * @param totallength - the total length of the result
 * @param ch - the prepend character
 * @return string
*/
string string_prependZero(string str, int totallength, char ch = '0') {
    string new_str = string(totallength - str.size(), ch) + str;
    return new_str;
}

/**
 * @brief Build and return a [DEFAULT_CONFIG_HEADER_SIZE] bytes netprobe_info in string
 * @param nc - the source netprobe config
 * @return string
*/
string NH_builder(NetProbeConfig nc) {
    string mode = intToString(nc.mode);
    string proto = intToString(nc.protocol);
    string pktnum = string_prependZero(intToString(nc.pkt_num), 10);
    string pktsize = string_prependZero(intToString(nc.pkt_size_inbyte), 10);
    string pktrate = string_prependZero(intToString(nc.pkt_rate_bytepersec), 10);
    string lport = string_prependZero(intToString(nc.lport), 8);
    string servmodel = intToString(nc.server_model);
    string poolsize = string_prependZero(intToString(nc.poolsize),4);

    //Then the DEFAULT_CONFIG_HEADER_SIZE should be 1 + 1 + 10 + 10 + 10 + 8 + 1 + 4

    string builder = DEFAULT_NCH + mode + proto + pktnum + pktsize + pktrate + lport + servmodel + poolsize + "\0";
    //cout << "Builder = " << builder << endl;
    return builder;
}

/**
 * @brief This function return a NetProbeConfig object by rebuilding one using info received from the client
 * @param NH_str - the string built by NH_builder() and received from the client
 * @param nc - the rebuild netprobe config target
*/
void rebuildFromNHBuilder(char* NH_str, NetProbeConfig* nc) {
    char buf[DEFAULT_CONFIG_HEADER_SIZE];
    memcpy(buf, NH_str, DEFAULT_CONFIG_HEADER_SIZE);
    char temp_sub_buf[11];
    memcpy(temp_sub_buf, &buf[0 + DEFAULT_NCH_LEN], 1);
    temp_sub_buf[1] = '\0';
    nc->mode = atoi(temp_sub_buf);

    //cout << "nc mode = " << nc->mode << ", temp_sub_buf = " << temp_sub_buf << endl;  //Debug

    memcpy(temp_sub_buf, &buf[1 + DEFAULT_NCH_LEN], 1);
    temp_sub_buf[1] = '\0';
    nc->protocol = atoi(temp_sub_buf);
    memcpy(temp_sub_buf, &buf[2 + DEFAULT_NCH_LEN], 10);
    temp_sub_buf[10] = '\0';
    nc->pkt_num = atoi(temp_sub_buf);
    memcpy(temp_sub_buf, &buf[12 + DEFAULT_NCH_LEN], 10);
    temp_sub_buf[10] = '\0';
    nc->pkt_size_inbyte = atoi(temp_sub_buf);
    memcpy(temp_sub_buf, &buf[22 + DEFAULT_NCH_LEN], 10);
    temp_sub_buf[10] = '\0';
    nc->pkt_rate_bytepersec = atoi(temp_sub_buf);
    memcpy(temp_sub_buf, &buf[32 + DEFAULT_NCH_LEN], 8);
    temp_sub_buf[8] = '\0';
    nc->lport = atoi(temp_sub_buf);
    memcpy(temp_sub_buf, &buf[40 + DEFAULT_NCH_LEN], 1);
    temp_sub_buf[1] = '\0';
    nc->server_model = atoi(temp_sub_buf);
    memcpy(temp_sub_buf, &buf[41 + DEFAULT_NCH_LEN], 4);
    temp_sub_buf[4] = '\0';
    nc->poolsize = atoi(temp_sub_buf);
}

/**
 * @brief This function calculate the time interval between sending each packet
 * @param nc - the current netprobe config
 * @return int - time interval in ms
*/
int pkt_timeInterval(NetProbeConfig nc) {
    int time_interval_ms;
    if (nc.pkt_rate_bytepersec > 0) {
        time_interval_ms = (int)ceil(((double)nc.pkt_size_inbyte / (double)nc.pkt_rate_bytepersec * CLOCKS_PER_SEC));
    }
    else {
        time_interval_ms = 0;
    }
    if (time_interval_ms <= 0) time_interval_ms = 1;
    return time_interval_ms;
}

/**
 * @brief Convert a predefined ncMode name into a string
 * @param ncMode - the mode that needed to convert
 * @return string - the full name of the ncMode
*/
string ncModeConverttoString(int ncMode) {

    string modestr = "";
    switch (ncMode) {
    case NETPROBE_SEND_MODE:
        modestr = "SEND";
        break;
    case NETPROBE_RECV_MODE:
        modestr = "RECV";
        break;
    case NETPROBE_HOST_MODE:
        modestr = "HOST";
        break;
    case NETPROBE_SERV_PLACEHOLDER_MODE:
        modestr = "SERVER";
        break;
    case NETPROBE_RESP_MODE:
        modestr = "RESPONSE";
        break;
    default:
        modestr = "ERROR";
        break;
    }
    return modestr;
}

/**
 * @brief Convert a predefined ncProto name into a string
 * @param ncProto - the mode that needed to convert
 * @return string - the full name of the ncProto
*/
string ncProtoConverttoString(int ncProto) {

    string protostr = "";
    switch (ncProto) {
    case NETPROBE_TCP_MODE:
        protostr = "TCP";
        break;
    case NETPROBE_UDP_MODE:
        protostr = "UDP";
        break;
    case NETPROBE_NULL_PROTO:
        protostr = "BOTH";  //Or  protostr = "TCP/UDP";
        break;
    default:
        protostr = "ERROR";
        break;
    }
    return protostr;
}

//Print the netprobe config info
void netProbeConfig_info(NetProbeConfig nc) {
    string proto = ncProtoConverttoString(nc.protocol);
    string mode = ncModeConverttoString(nc.mode);
    string lhost = "";
    
    (nc.lhostname == "") ? lhost = "not specified" : lhost = nc.lhostname;

    cout << " NetProbe Configurations:\n";
    cout << "  Mode: " << mode << "    Protocol: " << proto << endl;
    cout << "  -stat = " << nc.stat_displayspeed_perms << endl;
    cout << "  -pktsize = " << nc.pkt_size_inbyte << endl;
    cout << "  -pktrate = " << nc.pkt_rate_bytepersec << endl;
    cout << "  -pktnum = " << nc.pkt_num << endl;
    cout << "  -sbufsize = " << nc.sbufsize_inbyte << endl;
    cout << "  -rbufsize = " << nc.rbufsize_inbyte << endl;
    cout << "  -lhost = " << lhost << endl;
    cout << "  -lport = " << nc.lport << endl;
    cout << "  -rhost = " << nc.rhostname << endl;
    cout << "  -rport = " << nc.rport << endl;
}

//Print a connection message
void netProbeConnectMessage(NetProbeConfig nc, const char* client_ip, int client_port) {
    string proto = ncProtoConverttoString(nc.protocol);
    string mode = ncModeConverttoString(nc.mode);
    cout << " Connected to [" << client_ip << "], port: " << client_port << ", " << mode << ", " << proto << ", " << nc.pkt_rate_bytepersec << " Bps" << endl;
}

//Print the help info for using netprobe_server
void usage_message_server() {
    cout << " NetProbeServer <more parameters depended on mode>, see below:\n";
    cout << "    <-lhost hostname>   hostname to bind to. (Default late binding)\n";
    cout << "    <-lport portnum>    port number to bind to. (Default '4180')\n";
    cout << "    <-servermodel select|threadpool>   Server Model  (Default Select() )\n";
    cout << "    <-poolsize psize>   The the thread pool size (Default 8, only valid in threadpool server model)\n";
    cout << "    <-rbufsize bsize>   set the incoming socket buffer size to bsize bytes.\n";
    cout << "    <-sbufsize bsize>   set the outgoing socket buffer size to bsize bytes.\n\n";
}

//Print the help info for using netprobe_client
void usage_message_client() {
    cout << " NetProbeServer [mode] <more parameters depended on mode>, see below:\n";
    cout << " [mode]:   -send means sending mode;\n";
    cout << "           -recv means receiving mode;\n";
    cout << "           -resp means response mode;\n";
    cout << " If [mode] = -send / -resp then the following are the supported parameters:\n";
    cout << "    <-stat yyy>         update statistics once every yyy ms. (Default = 500 ms)\n";
    cout << "    <-rhost hostname>   send data to host specified by hostname. (Default 'localhost')\n";
    cout << "    <-rport portnum>    send data to remote host at port number portnum. (Default '4180')\n";
    cout << "    <-proto tcp|udp>          send data using TCP or UDP. (Default UDP)\n";
    cout << "    <-pktsize bsize>    send message of bsize bytes. (Default 1000 bytes)\n";
    cout << "    <-pktrate txrate>   send data at a data rate of txrate bytes per second,\n";
    cout << "                        0 means as fast as possible. (Default 1000 bytes/second)\n";
    cout << "    <-pktnum num>       send or receive a total of num messages. (Default = infinite)\n";
    cout << "    <-sbufsize bsize>   set the outgoing socket buffer size to bsize bytes.\n\n";
    cout << " else if [mode] = -recv then\n";
    cout << "    <-stat yyy>         update statistics once every yyy ms. (Default = 500 ms)\n";
    cout << "    <-lhost hostname>   hostname to bind to. (Default late binding)\n";
    cout << "    <-lport portnum>    local received port number to bind to. (Default '4180')\n";
    cout << "    <-proto tcp|udp>          send data using TCP or UDP. (Default UDP)\n";
    cout << "    <-pktsize bsize>    send message of bsize bytes. (Default 1000 bytes)\n";
    cout << "    <-rbufsize bsize>   set the incoming socket buffer size to bsize bytes.\n\n";
}

/**
 * @brief Check the hostname and return an IPV4 dot address string
 * @param hostname
 * @return string - Empty string (Invalid hostname) | IPV4 address
*/
string hostnametoIPV4(string hostname) {
    unsigned long ipaddr = inet_addr(hostname.c_str());
    struct hostent* pHost = NULL;

    // It is an IP address, reverse lookup the hostname first
    if (ipaddr != -1) {
        pHost = gethostbyaddr((char*)(&ipaddr), sizeof(ipaddr), AF_INET);
        if ((pHost != NULL) && (pHost->h_name)) {
            hostname = pHost->h_name;
            hostname[hostname.length() - 1] = 0; // Guarantee null-termination
        }
    #if (OSISWINDOWS == true)
        else if (WSAGetLastError() == WSANO_DATA) {
            printf("\n No DNS Record for the IP address found.\n");
        }
        else { printf("gethostbyaddr() failed with code %i\n", WSAGetLastError()); }
    #endif
    }
    // Resolve the hostname in pRemoteHost
    pHost = gethostbyname(hostname.c_str());
    char* ptr = NULL;
    if (pHost != NULL) ptr = pHost->h_addr_list[0];
    else return "";
    string returner = inet_ntoa(*((struct in_addr*)(ptr)));
    return returner;
}

/**
 * @brief Convert the bytes into bps string format and print it directly
 * @param bytes - The total tranfered bytes
 * @param now_time_safe - The total time duration ( > 0 )
*/
void bytesTobps(long bytes, clock_t now_time_safe) {
    double thoughput = ((double)bytes * 8) / (now_time_safe);
    string bps;

    if (thoughput > 1000) {
        if (thoughput > 1000000) {
            if (thoughput > 1000000000) {
                thoughput /= 1000000000;
                bps = "Gbps";
            }
            else {
                thoughput /= 1000000;
                bps = "Mbps";
            }
        }
        else {
            thoughput /= 1000;
            bps = "Kbps";
        }
    }
    else {
        bps = "bps";
    }
    cout << setprecision(4) << thoughput << bps;
}

/// Statistics Display ///

void recvInfo(NetProbeConfig nc, clock_t starting_time, long recved_pkt, long lost_pkt, double total_recv_size, double jitter) {
    //cout << " Debug, clock is now : " << clock() << ", nc_stat is : " << nc.stat_displayspeed_perms << endl;
    static int last_show_var = 0;

    if (((clock() - starting_time)/nc.stat_displayspeed_perms) > last_show_var) {
        last_show_var = (clock() - starting_time) / nc.stat_displayspeed_perms;
        clock_t now_time = (clock() - starting_time) / CLOCKS_PER_SEC;    //Calculate the elapsed time in second
        clock_t now_time_safe = now_time;
        if (now_time <= 0)
            now_time_safe = 1;
        double lost_rate = (double)lost_pkt / ((recved_pkt + lost_pkt) * 100);
        if ((recved_pkt + lost_pkt) * 100 <= 0) lost_rate = 0.0;    //Prevent the overflow
        printf("\r Recv: Elapsed [%lds] Pkts [%ld] Lost [%ld, %.2lf%%]", now_time, recved_pkt, lost_pkt, lost_rate);
        printf(" Rate ["); bytesTobps(total_recv_size, now_time_safe);
        printf("] Jitter [%.2lfms]          ", jitter);
    }
}

void sendInfo(NetProbeConfig nc, clock_t starting_time, long sent_pkt, long total_sent_size) {
    static int last_show_var = 0;

    if (((clock() - starting_time) / nc.stat_displayspeed_perms) > last_show_var) {
        last_show_var = (clock() - starting_time) / nc.stat_displayspeed_perms;
        clock_t now_time = (clock() - starting_time) / CLOCKS_PER_SEC;    //Calculate the elapsed time in second
        clock_t now_time_safe = now_time;
        if (now_time <= 0)
            now_time_safe = 1;
        printf("\r Send: Elapsed [%lds] Pkts [%ld] Rate [", now_time, sent_pkt);
        bytesTobps(total_sent_size, now_time_safe);
        printf("]          ");
    }
}

//Print the response message for server
void response_message_server(NetProbeConfig nc) {
    //TODO: Goal = "Elapsed [120s] ThreadPool [32|25] TCP Clients [10] UDP Clients [15]"
    string proto = ncProtoConverttoString(nc.protocol);
    string mode = ncModeConverttoString(nc.mode);

    //printf("\r Response: Elapsed [%ds] ThreadPool [%d|%d] TCP Clients [%d] UDP Clients [%d]");
}

//Print the response message for client
void response_message_client(NetProbeConfig nc) {
    //TODO: Goal = "Elapsed [120s] Replies [1234] Min [4.3ms] Max [10.2ms] Avg [8.0ms] Jitter [5.2ms]"
    string proto = ncProtoConverttoString(nc.protocol);
    string mode = ncModeConverttoString(nc.mode);

    //printf("\r Response: Elapsed [%ds] Replies [%d] Min [%.2f] ax [%.2fms] Avg [%.2fs] Jitter [%.2fms]");
}

/**
 * @brief Send a NH builder pkt to the server and receive the response
 * @param nc - The netprobe config to tranfer
 * @param s - The TCP socket to the server
 * @return 0 ( Success case ) | 1 ( Send Failed ) | 2 ( Receive Failed )
*/
int sendNCbuildMessage(NetProbeConfig nc, SOCKET s) {
    char send_buf[DEFAULT_CONFIG_HEADER_SIZE];
    char recv_buf[DEFAULT_CONFIG_HEADER_SIZE];
    int sent_byte = 0;
    int recv_size = 0;
    strncpy(send_buf, NH_builder(nc).c_str(), DEFAULT_CONFIG_HEADER_SIZE);
    if ((sent_byte = send(s, send_buf, DEFAULT_CONFIG_HEADER_SIZE, 0)) == SOCKET_ERROR) {
        printf("\n Send failed\n");
        return 1;
    }
    if ((recv_size = recv(s, recv_buf, DEFAULT_CONFIG_HEADER_SIZE, 0)) == SOCKET_ERROR) {
        printf(" Recv Failed.\n");
        return 2;
    }
    return 0;
}

/**
 * @brief A multiplatform version of close socket
 * @param s - Close this socket
*/
void closesocket_comp(SOCKET s) {
    #if(OSISWINDOWS==true)
        closesocket(s);
    #else
        close(s);
    #endif
}

#endif