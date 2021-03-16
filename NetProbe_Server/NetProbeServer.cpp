/*
	NetProbe project 1
	@author : Wong Cho Hei
	SID : 1155109484
*/

// The default command for using this exe is
// cd C:\Users\Danny Boy\Documents\GitHub\IERG4180_Project2\Debug
// netprobe_server.exe [arguments]

#include "../core.h"

using namespace std;

void ConcurrentListenerUsingSelect(const char* port, NetProbeConfig npc)
{
	/// Step 1: Prepare address structures. ///
	sockaddr_in* SER_Addr = new sockaddr_in;
	memset(SER_Addr, 0, sizeof(struct sockaddr_in));
	SER_Addr->sin_family = AF_INET;
	SER_Addr->sin_port = htons(atoi(port));
	SER_Addr->sin_addr.s_addr = INADDR_ANY;

	/// Step 2: Create a socket for incoming connections. ///
	SOCKET Sockfd = socket(AF_INET, SOCK_STREAM, 0);
	bind(Sockfd, (sockaddr*)SER_Addr, sizeof(sockaddr_in));
	listen(Sockfd, 5);

	printf("\n TCP Listen Socket created.\n");

	//Create the UDP socket
	SOCKET UDPfd = socket(AF_INET, SOCK_DGRAM, 0);
	bind(UDPfd, (sockaddr*)SER_Addr, sizeof(sockaddr_in));

	printf(" UDP Socket created.\n");

	/// Step 3: Setup the data structures for multiple connections.
	const int maxSockets = 10; // At most 10 concurrent clients
	long total_recv[maxSockets] = { 0 };
	long total_send[maxSockets] = { 0 };
	sockaddr_in peerinfo[maxSockets];
	int namelen = sizeof(sockaddr_in);
	NetProbeConfig nc[maxSockets];

	SOCKET socketHandles[maxSockets]; // Array for the socket handles
	bool socketValid[maxSockets]; // Bitmask to manage the array
	int numActiveSockets = 1;
	long sent_pkt[maxSockets] = { 0 };
	char* packet_num = (char*)malloc(((sizeof(sent_pkt) * CHAR_BIT) + 2) / 3 + 2);
	for (int i = 0; i < maxSockets; i++) socketValid[i] = false;
	socketHandles[0] = Sockfd;
	socketValid[0] = true;
	char first4char[5];
	struct timeval tv;
	int time_interval_ms[maxSockets] = { 500 };
	int last_show_var[maxSockets] = { 0 };
	socketHandles[1] = UDPfd;
	socketValid[1] = true;

	/// Step 4: Setup the select() function call for I/O multiplexing. ///
	fd_set fdReadSet;
	fd_set fdWriteSet;

	printf(" Socket Listening...\n");
	while (1) {
		// Setup the fd_set
		int topActiveSocket = 0;
		int i = 0;
		FD_ZERO(&fdReadSet);
		FD_ZERO(&fdWriteSet);
		for (i = 0; i < maxSockets; i++) {
			if (socketValid[i] == true) {
				FD_SET(socketHandles[i], &fdReadSet);
				FD_SET(socketHandles[i], &fdWriteSet);
				if (socketHandles[i] > topActiveSocket)
					topActiveSocket = socketHandles[i]; // for Linux compatibility
			}
		}
		// Block on select() //
		int ret;
		//tv.tv_usec = (*min_element(time_interval_ms + 2, time_interval_ms + maxSockets)) * 1000;
		tv.tv_usec = 0;
		if ((ret = select(topActiveSocket + 1, &fdReadSet, &fdWriteSet, NULL, &tv)) == SOCKET_ERROR) {
#if(OSISWINDOWS==true)
			printf("\n select() failed. Error code: %i\n", WSAGetLastError());
#else
			perror("\n select() failed.\n");
#endif
			return;
		}
		//setsockopt(socketHandles[1], SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));
		unsigned long int  noBlock = 1;

#if(OSISWINDOWS==true)
		ioctlsocket(socketHandles[1], FIONBIO, &noBlock);	//To set UDP as non-blocking mode
#else
		ioctl(socketHandles[1], FIONBIO, noBlock);
#endif

		// Process the active sockets //
		for (i = 0; i < maxSockets; i++) {
			if (!socketValid[i]) continue; // Only check for valid sockets.
			//send bypassing FD_ISSET
			if (nc[i].mode == NETPROBE_RECV_MODE && socketValid[i] && nc[i].protocol == NETPROBE_TCP_MODE && i > 1) { //if the client is in recv mode
				//Handle the TCP recv request
				time_interval_ms[i] = pkt_timeInterval(nc[i]);
				if ((clock() / time_interval_ms[i]) > last_show_var[i]) {
					last_show_var[i] = clock() / time_interval_ms[i];
					char* packet = (char*)malloc(sizeof(char) * nc[i].pkt_size_inbyte);
					strncpy(packet, "sending_pkt_num= ", 18);
					sprintf(packet_num, "%ld ", sent_pkt[i]);
					strcat(packet, packet_num);

					//Send the packet to the client
					int sent_byte = send(socketHandles[i], packet, nc[i].pkt_size_inbyte, 0);
					//cout << " A packet sent, pkt_num = " << sent_pkt[i] << endl;

					ZeroMemory(packet, sizeof(packet));
					free(packet);
					sent_pkt[i]++;

					if (sent_byte <= 0 || (sent_pkt[i] > nc[i].pkt_num&& nc[i].pkt_num != 0))  //If the client is disconnected or error
					{
						printf(" Host disconnected...\n");
						time_interval_ms[i] = 1;
						socketValid[i] = false;
						nc[i] = NetProbeConfig();
						sent_pkt[i] = 0;
						last_show_var[i] = 0;
						--numActiveSockets;
						if (numActiveSockets == (maxSockets - 1))
							socketValid[0] = true;
#if(OSISWINDOWS==true)
						closesocket(socketHandles[i]);
#else
						close(socketHandles[i]);
#endif
					}
				}
			}
			if (nc[i].mode == NETPROBE_RECV_MODE && socketValid[i] && nc[i].protocol == NETPROBE_UDP_MODE && i > 1) {
				//Handle the udp recv request
				time_interval_ms[i] = pkt_timeInterval(nc[i]);
				if ((clock() / time_interval_ms[i]) > last_show_var[i]) {
					struct sockaddr_in si_other;
					si_other.sin_family = AF_INET;
					si_other.sin_port = htons((short)nc[i].lport);
					char* client_ip = inet_ntoa(peerinfo[i].sin_addr);
					si_other.sin_addr.s_addr = inet_addr(client_ip);
					last_show_var[i] = clock() / time_interval_ms[i];
					char* message = (char*)malloc(sizeof(char) * nc[i].pkt_size_inbyte);
					strcpy(message, "send_pkt_num= ");
					sprintf(packet_num, "%ld", sent_pkt[i]);
					strcat(message, packet_num);

					//send the message

					int sendto_size = sendto(socketHandles[1], message, nc[i].pkt_size_inbyte, 0, (struct sockaddr*) & si_other, sizeof(si_other));

					//cout << " Sendto " << client_ip << ", port: " << nc[i].lport << endl;	//Debug
					sent_pkt[i]++;
					if ((sent_pkt[i] >= nc[i].pkt_num && nc[i].pkt_num != 0) || sendto_size <= SOCKET_ERROR) {
						cout << "Error here" << endl;
						nc[i] = NetProbeConfig();
						sent_pkt[i] = 0;
						last_show_var[i] = 0;
						time_interval_ms[i] = 1;
						socketValid[i] = false;
					}
					ZeroMemory(message, sizeof(message));
					free(message);
				}
			}
			if (FD_ISSET(socketHandles[i], &fdReadSet)) { // Is socket i active?
				if (i == 0) { // the socket for accept()
					SOCKET newsfd = accept(Sockfd, 0, 0);
					// Find a free entry in the socketHandles[] //
					int j = 2;
					for (; j < maxSockets; j++) {
						if (socketValid[j] == false) {
							socketValid[j] = true;
							socketHandles[j] = newsfd;
							++numActiveSockets;
							if (numActiveSockets == maxSockets) {
								// Ignore new accept()
								socketValid[0] = false;
							}
							break;
						}
					}
				}
				if (i == 1 || (nc[i].protocol == NETPROBE_UDP_MODE && nc[i].mode == NETPROBE_SEND_MODE)) {   //UDP recvfrom
					int recv_len, slen;
					char* buf = (char*)malloc(sizeof(char) * nc[i].pkt_size_inbyte);
					sockaddr_in si_other;    //The client socketaddr_in
					slen = sizeof(si_other);

#if(OSISWINDOWS==true)
					if (ret != 0)
						recv_len = recvfrom(socketHandles[1], buf, nc[i].pkt_size_inbyte, 0, (sockaddr*)&si_other, &slen);
#else
					unsigned int slen_linux = sizeof(si_other);
					recv_len = recvfrom(socketHandles[1], buf, nc[i].pkt_size_inbyte, 0, (sockaddr*)&si_other, &slen_linux);
#endif

					//printf(" Receiving from [%s:%d]:\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));

					ZeroMemory(buf, sizeof(buf));
					free(buf);

					if (recv_len <= 0 && i != 1) {
						// Update the socket array
						if (i != 1) {
							nc[i] = NetProbeConfig();
							sent_pkt[i] = 0;
							last_show_var[i] = 0;
							time_interval_ms[i] = 1;
						}
					}
				}
				else { // sockets for TCP recv()
					if (nc[i].mode == NETPROBE_RECV_MODE) continue;
					int recv_size;
					char* recv_buf = (char*)malloc(sizeof(char) * nc[i].pkt_size_inbyte);

#if(OSISWINDOWS==true)
					getpeername(socketHandles[i], (sockaddr*)&peerinfo[i], &namelen);
#else
					unsigned int namelen_linux = sizeof(sockaddr_in);
					getpeername(socketHandles[i], (sockaddr*)&peerinfo[i], &namelen_linux);
#endif

					char* client_ip = inet_ntoa(peerinfo[i].sin_addr);
					int client_port = ntohs(peerinfo[i].sin_port);
					long packet_num = 0;
					bool isNotifyMessage = false;

					recv_size = recv(socketHandles[i], recv_buf, nc[i].pkt_size_inbyte, 0);
					//printf(" Received message len: %s\n", recv_buf);
					if (recv_size != 0) {
						memcpy(first4char, recv_buf, DEFAULT_NCH_LEN);
						first4char[4] = '\0';
						if (strcmp(first4char, DEFAULT_NCH) == 0) { //Check for a nc header packet
							rebuildFromNHBuilder(recv_buf, &nc[i]);
							isNotifyMessage = true;
							netProbeConnectMessage(nc[i], client_ip, client_port);
							send(socketHandles[i], recv_buf, DEFAULT_CONFIG_HEADER_SIZE, 0);

							if (nc[i].sbufsize_inbyte <= 0) {
								nc[i].sbufsize_inbyte = 65536;
							}
							if (nc[i].rbufsize_inbyte <= 0) {
								nc[i].rbufsize_inbyte = 65536;
							}
							//Set Send Buffer Size
							if (setsockopt(socketHandles[i], SOL_SOCKET, SO_SNDBUF, (char*)(&nc[i].sbufsize_inbyte), sizeof(nc[i].sbufsize_inbyte)) < 0)
							{
#if(OSISWINDOWS==true)
								printf("setsockopt() for SO_KEEPALIVE failed with error: %u\n", WSAGetLastError());
#else
								perror(" setsockopt() for SO_KEEPALIVE failed.\n");
#endif
								return;
							}
						}
						ZeroMemory(recv_buf, sizeof(recv_buf));
						free(recv_buf);
					}
					else {// (recv_size == 0)  // connection closed
						if (!isNotifyMessage && nc[i].protocol != NETPROBE_UDP_MODE)
							printf(" Host disconnected [%s], port: %d \n", client_ip, client_port);
						// Update the socket array
						if (nc[i].protocol != NETPROBE_UDP_MODE) {
							socketValid[i] = false;
							nc[i] = NetProbeConfig();
							--numActiveSockets;
							if (numActiveSockets == (maxSockets - 1))
								socketValid[0] = true;
#if(OSISWINDOWS==true)
							closesocket(socketHandles[i]);
#else
							close(socketHandles[i]);
#endif
						}
					}
				}
				if (--ret == 0) break; // All active sockets processed.
			}
		}
	}
}

int main(int argc, char* argv[])
{
	NetProbeConfig nc;

	if (argc == 0) {
		usage_message_server();
	}
	if (argc >= 2) {
		//Deal with all the input value
		for (int i = 1; i < argc; i++) {
			//cout << "i = " << i << ", thing = ["<< argv[i] <<"]" << endl; //Debug
			if (strcmp(argv[i], "-help") == 0) {
				usage_message_server();
				return 0;
			}
			if (strcmp(argv[i], "-send") == 0) {
				nc.mode = 1;
			}
			if (strcmp(argv[i], "-recv") == 0) {
				nc.mode = 2;
			}
			if (strcmp(argv[i], "-proto") == 0) {
				if (argv[i + 1] != NULL && argv[i + 1] != "") {
					if (strcmp(strlwr(argv[i + 1]), "udp") == 0) {
						nc.protocol = 1;
					}
					if (strcmp(strlwr(argv[i + 1]), "tcp") == 0) {
						nc.protocol = 2;
					}
				}
			}
			if (strcmp(argv[i], "-pktsize") == 0) {
				if (argv[i + 1] != NULL && argv[i + 1] != "") {
					nc.pkt_size_inbyte = atoi(argv[i + 1]);
				}
			}
			if (strcmp(argv[i], "-pktrate") == 0) {
				if (argv[i + 1] != NULL && argv[i + 1] != "") {
					nc.pkt_rate_bytepersec = atoi(argv[i + 1]);
				}
			}
			if (strcmp(argv[i], "-pktnum") == 0) {
				if (argv[i + 1] != NULL && argv[i + 1] != "") {
					nc.pkt_num = atoi(argv[i + 1]);
				}
			}
			if (strcmp(argv[i], "-stat") == 0) {
				if (argv[i + 1] != NULL && argv[i + 1] != "") {
					nc.stat_displayspeed_perms = atoi(argv[i + 1]);
				}
			}
			if (strcmp(argv[i], "-sbufsize") == 0) {
				if (argv[i + 1] != NULL && argv[i + 1] != "") {
					nc.sbufsize_inbyte = atoi(argv[i + 1]);
				}
			}
			if (strcmp(argv[i], "-lhost") == 0) {
				if (argv[i + 1] != NULL && argv[i + 1] != "") {
					nc.lhostname = argv[i + 1];
				}
			}
			if (strcmp(argv[i], "-lport") == 0) {
				if (argv[i + 1] != NULL && argv[i + 1] != "") {
					nc.lport = atoi(argv[i + 1]);
				}
			}
			if (strcmp(argv[i], "-rbufsize") == 0) {
				if (argv[i + 1] != NULL && argv[i + 1] != "") {
					nc.rbufsize_inbyte = atoi(argv[i + 1]);
				}
			}
		}
	}

	nc.mode = 4;
	nc.protocol = 3;

	netProbeConfig_info(nc);

#if (OSISWINDOWS == true)
	WSADATA wsaData;    // For windows system socket startup
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf(" There are no useable winsock.dll\n");
		printf(" Socket Initialisation failure. Error Code: %d.\n", WSAGetLastError());
		return 1;
	}
#endif

	ConcurrentListenerUsingSelect(intToString(nc.lport).c_str(), nc);

#if (OSISWINDOWS == true)
	WSACleanup();
#endif

	return 0;
}