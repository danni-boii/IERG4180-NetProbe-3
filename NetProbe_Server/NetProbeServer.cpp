/*
	NetProbe project 3
	@author : Wong Cho Hei
	SID : 1155109484
*/

// The default command for using this exe is
// cd C:\Users\Danny Boy\Documents\GitHub\IERG4180_Project3\IERG4180-NetProbe-3\Debug
// netprobe_server.exe [arguments]

#include "../netprobe_core.h"

using namespace std;

/**
* This structure is only used for the tinycthread_pool related function 
* since they do not support passing multiple arguments to the thread easily.
* 
* @brief A compact structre to pass multiple datatype to threadpool function.
* 
*/
typedef struct socketWithMtx {
	SOCKET s;
	mtx_t lock;
}socketWithMtx;

int TCP_connetion_counter = 0;	//The total TCP connection of the server.
int UDP_connetion_counter = 0;	//The total UDP connection of the server.
mtx_t lock;


void RequestHandler_thread(void* arg) {
	socketWithMtx* swm = (socketWithMtx*)arg;
	SOCKET client_sock = swm->s;
	mtx_t thread_lock = swm->lock;

	NetProbeConfig nc;
	int recv_size;
	char* recv_buf = (char*)malloc(sizeof(char) * nc.pkt_size_inbyte);

	sockaddr_in peerinfo;
	int namelen = sizeof(sockaddr_in);
	#if(OSISWINDOWS==true)
		getpeername(client_sock, (sockaddr*)&peerinfo, &namelen);
	#else
		unsigned int namelen_linux = sizeof(sockaddr_in);
		getpeername(socketHandles[i], (sockaddr*)&peerinfo, &namelen_linux);
	#endif

	char* client_ip = inet_ntoa(peerinfo.sin_addr);
	int client_port = ntohs(peerinfo.sin_port);
	bool isNotifyMessage = false;
	char first4char[5];

	ZeroMemory(recv_buf, sizeof(recv_buf));
	free(recv_buf);
	recv_size = recv(client_sock, recv_buf, nc.pkt_size_inbyte, 0);

	/**
	 * Rebuild the netprobe config from the incoming packet
	*/

	if (recv_size != 0) {
		memcpy(first4char, recv_buf, DEFAULT_NCH_LEN);
		first4char[4] = '\0';
		if (strcmp(first4char, DEFAULT_NCH) == 0) { //Check for a nc header packet
			rebuildFromNHBuilder(recv_buf, &nc);
			isNotifyMessage = true;
			netProbeConnectMessage(nc, client_ip, client_port);
			send(client_sock, recv_buf, DEFAULT_CONFIG_HEADER_SIZE, 0);

			if (nc.sbufsize_inbyte <= 0) {
				nc.sbufsize_inbyte = 65536;
			}
			if (nc.rbufsize_inbyte <= 0) {
				nc.rbufsize_inbyte = 65536;
			}
			//Set Send Buffer Size
			if (setsockopt(client_sock, SOL_SOCKET, SO_SNDBUF, (char*)(&nc.sbufsize_inbyte), sizeof(nc.sbufsize_inbyte)) < 0)
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
		closesocket_comp(client_sock);
	}
	else {
		closesocket_comp(client_sock);
	}

	/**
	* The NetProbe Config Initalization is done.
	* 
	* Do the real work load here
	*/

	int time_interval_ms = 0;
	int last_show_var = 0;
	long sent_pkt = { 0 };
	char* packet_num = (char*)malloc(((sizeof(sent_pkt) * CHAR_BIT) + 2) / 3 + 2);

	if (nc.protocol == NETPROBE_TCP_MODE) {
		mtx_lock(&(thread_lock));
		TCP_connetion_counter++;
		mtx_unlock(&thread_lock);
		if (nc.mode == NETPROBE_SEND_MODE || nc.mode == NETPROBE_RESP_MODE) {
			int send_size = 0;
			InfinityLoop{
				char* recv_buf = (char*)malloc(sizeof(char) * nc.pkt_size_inbyte);
				recv_size = recv(client_sock, recv_buf, nc.pkt_size_inbyte, 0);
				if (nc.mode == NETPROBE_RESP_MODE) {
					send_size = send(client_sock, recv_buf, nc.pkt_size_inbyte, 0);
				}
				//printf(" Received message len: %s\n", recv_buf);
				ZeroMemory(recv_buf, sizeof(recv_buf));
				free(recv_buf);
				if (recv_size <= 0 || (nc.mode == NETPROBE_RESP_MODE && send_size <= 0)) // (recv_size == 0)  // connection closed
				{	
					cout << " Host disconnected or error... tid: " << this_thread::get_id() << endl;
					closesocket_comp(client_sock);
					break;
				}
			}
		}
		else if (nc.mode == NETPROBE_RECV_MODE) {
			//The client is in receive mode, so we need to send the data
			InfinityLoop{
				time_interval_ms = pkt_timeInterval(nc);
				if ((clock() / time_interval_ms) > last_show_var) {
					last_show_var = clock() / time_interval_ms;
					char* packet = (char*)malloc(sizeof(char) * nc.pkt_size_inbyte);
					strncpy(packet, "sending_pkt_num= ", 18);
					sprintf(packet_num, "%ld ", sent_pkt);
					strcat(packet, packet_num);

					//Send the packet to the client
					int sent_byte = send(client_sock, packet, nc.pkt_size_inbyte, 0);
					//cout << " A packet sent, pkt_num = " << sent_pkt[i] << endl;

					ZeroMemory(packet, sizeof(packet));
					free(packet);
					sent_pkt++;

					if (sent_byte < 0 || (sent_pkt > nc.pkt_num && nc.pkt_num != 0))  //If the client is disconnected or error
					{
						cout << " Host disconnected or error... tid: " << this_thread::get_id() << endl;
						closesocket_comp(client_sock);
						break;
					}
				}
			}
		}
		mtx_lock(&(thread_lock));
		TCP_connetion_counter--;
		mtx_unlock(&thread_lock);
		return;
	}
	else if (nc.protocol == NETPROBE_UDP_MODE) {
		mtx_lock(&(thread_lock));
		UDP_connetion_counter++;
		mtx_unlock(&thread_lock);
		if (nc.mode == NETPROBE_SEND_MODE || nc.mode == NETPROBE_RESP_MODE) {
			int recv_len,send_len;
			char* buf = (char*)malloc(sizeof(char) * nc.pkt_size_inbyte);
			sockaddr_in si_other;    //The client socketaddr_in
			int slen = sizeof(si_other);
			unsigned int slen_linux = sizeof(si_other);

			InfinityLoop{
				#if(OSISWINDOWS==true)
					recv_len = recvfrom(client_sock, buf, nc.pkt_size_inbyte, 0, (sockaddr*)&si_other, &slen);
				#else
					recv_len = recvfrom(client_sock, buf, nc.pkt_size_inbyte, 0, (sockaddr*)&si_other, &slen_linux);
				#endif

				if (nc.mode == NETPROBE_RESP_MODE) {
					send_len = sendto(client_sock, buf, nc.pkt_size_inbyte, 0, (struct sockaddr*)&si_other, sizeof(si_other));
				}

				//printf(" Receiving from [%s:%d]:\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));

				ZeroMemory(buf, sizeof(buf));
				if(recv_len <= 0)	//Error occurs
				{
					cout << " Host disconnected or error... tid: " << this_thread::get_id() << endl;
					free(buf);
					closesocket_comp(client_sock);
					break;
				}
			}
		}
		else if (nc.mode == NETPROBE_RECV_MODE) {
			struct sockaddr_in si_other;
			si_other.sin_family = AF_INET;
			si_other.sin_port = htons((short)nc.lport);
			char* client_ip = inet_ntoa(peerinfo.sin_addr);
			si_other.sin_addr.s_addr = inet_addr(client_ip);
			time_interval_ms = pkt_timeInterval(nc);
			InfinityLoop{
				if ((clock() / time_interval_ms) > last_show_var) {
					last_show_var = clock() / time_interval_ms;
					char* message = (char*)malloc(sizeof(char) * nc.pkt_size_inbyte);
					strcpy(message, "send_pkt_num= ");
					sprintf(packet_num, "%ld", sent_pkt);
					strcat(message, packet_num);

					//send the message

					int sendto_size = sendto(client_sock, message, nc.pkt_size_inbyte, 0, (struct sockaddr*)&si_other, sizeof(si_other));

					//cout << " Sendto " << client_ip << ", port: " << nc.lport << endl;	//Debug
					ZeroMemory(message, sizeof(message));
					free(message);
					sent_pkt++;
					if ((sent_pkt >= nc.pkt_num && nc.pkt_num != 0) || sendto_size <= SOCKET_ERROR) {
						cout << " Host disconnected or error... tid: " << this_thread::get_id() << endl;
						closesocket_comp(client_sock);
						break;
					}
				}
			}
		}
		mtx_lock(&(thread_lock));
		UDP_connetion_counter--;
		mtx_unlock(&thread_lock);
		return;
	}
	return;
}

void AdminThread(void* threadpool) {
	threadpool_t* pool = (threadpool_t*)threadpool;
	timespec sleep_time; sleep_time.tv_sec = 1; sleep_time.tv_nsec = 1000000;
	clock_t adminclock = clock();
	clock_t startTimer = clock();
	while (pool->shutdown == 0) {
		//Pause the thread for a while
		thrd_sleep(&sleep_time, NULL);
		//set the thread size to half after 60secs of 50% lower usage.
		if (((float)pool->busy_thread / pool->thread_count) < 0.5) {
			if (clock() > adminclock) {
				if (((clock() - adminclock) / CLOCKS_PER_SEC) >= 60) {
					if ((pool->thread_count / 2) >= MIN_THREADS) {
						pool->thread_count /= 2;
					}
					else {
						pool->thread_count = 2;
					}
					adminclock = clock();
				}
			}
			else {
				adminclock = clock();
			}
		}
		startTimer = clock();
		server_stats(startTimer / CLOCKS_PER_SEC, pool->thread_count, pool->busy_thread, TCP_connetion_counter, UDP_connetion_counter);
	}
}

/**
 * @brief This is the function to handle all the client request using ThreadPool
 * @param port - the main listening port
 * @param npc - the server NetProbe config
*/
void ConcurrentListenerUsingThreadPool(const char* port, NetProbeConfig npc) {
	/// Step 0: Create a threadpool
	threadpool_t* thp = threadpool_create(npc.poolsize, 100, 0);
	if (thp == NULL) {
		printf("Error on creating threadpool.\n");
		return;
	}
	threadpool_add(thp, AdminThread, (void*)thp, 0);

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

	InfinityLoop{
		int len = sizeof(SOCKADDR);
		SOCKET client_sock = accept(Sockfd, (sockaddr*)&SER_Addr, &len);
		if (client_sock < 0)
		{
			printf("Error on accept(), Error code : %u\n", WSAGetLastError());
			return;
		}
		socketWithMtx* swm = (socketWithMtx*)malloc(sizeof(socketWithMtx));
		swm->s = client_sock;
		swm->lock = lock;
		threadpool_add(thp, RequestHandler_thread, (void*)swm,  0);
	}
	threadpool_destroy(thp, 0);
	mtx_destroy(&lock);
	closesocket_comp(Sockfd);
}

/**
 * @brief This is the function to handle all the client request using Select()
 * @param port - the main listening port
 * @param npc - the server NetProbe config
*/
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
	InfinityLoop{
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

						if (sent_byte <= 0 || (sent_pkt[i] > nc[i].pkt_num && nc[i].pkt_num != 0))  //If the client is disconnected or error
						{
							printf(" Host disconnected...\n");
							time_interval_ms[i] = 1;
							socketValid[i] = false;
							nc[i] = NetProbeConfig();
							sent_pkt[i] = 0;
							last_show_var[i] = 0;
							--numActiveSockets;
							if (numActiveSockets == (maxSockets - 1))
							{
								socketValid[0] = true;
							}
							closesocket_comp(socketHandles[i]);
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

						int sendto_size = sendto(socketHandles[1], message, nc[i].pkt_size_inbyte, 0, (struct sockaddr*)&si_other, sizeof(si_other));

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
							if (ret != 0) recv_len = recvfrom(socketHandles[1], buf, nc[i].pkt_size_inbyte, 0, (sockaddr*)&si_other, &slen);
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
									if (numActiveSockets == (maxSockets - 1)) {
										socketValid[0] = true;
									}
									closesocket_comp(socketHandles[i]);
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
	mtx_init(&lock, NULL);

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
			else if (strcmp(argv[i], "-stat") == 0) {
				if (argv[i + 1] != NULL && argv[i + 1] != "") {
					nc.stat_displayspeed_perms = atoi(argv[i + 1]);
				}
			}
			else if (strcmp(argv[i], "-sbufsize") == 0) {
				if (argv[i + 1] != NULL && argv[i + 1] != "") {
					nc.sbufsize_inbyte = atoi(argv[i + 1]);
				}
			}
			else if (strcmp(argv[i], "-lhost") == 0) {
				if (argv[i + 1] != NULL && argv[i + 1] != "") {
					nc.lhostname = argv[i + 1];
				}
			}
			else if (strcmp(argv[i], "-lport") == 0) {
				if (argv[i + 1] != NULL && argv[i + 1] != "") {
					nc.lport = atoi(argv[i + 1]);
				}
			}
			else if (strcmp(argv[i], "-rbufsize") == 0) {
				if (argv[i + 1] != NULL && argv[i + 1] != "") {
					nc.rbufsize_inbyte = atoi(argv[i + 1]);
				}
			}
			else if (strcmp(argv[i], "-servermodel") == 0) {
				if (argv[i + 1] != NULL && argv[i + 1] != "") {
					if (strcmp(strlwr(argv[i + 1]), "select") == 0 || atoi(argv[i + 1]) == 1) {
						nc.server_model = 1;
					}
					else if (strcmp(strlwr(argv[i + 1]), "threadpool") == 0 || atoi(argv[i + 1]) == 2) {
						nc.server_model = 2;
					}
				}
			}
			else if (strcmp(argv[i], "-poolsize") == 0) {
				if (argv[i + 1] != NULL && argv[i + 1] != "") {
					int temp_pool_size = atoi(argv[i + 1]);
					if (temp_pool_size < 0) temp_pool_size = 0;
					nc.rbufsize_inbyte = temp_pool_size;
				}
			}
		}
	}

	/**
	 * Test settings
	*/
	nc.mode = NETPROBE_SERV_PLACEHOLDER_MODE;
	nc.protocol = NETPROBE_NULL_PROTO;
	//test the multithread mode
	nc.server_model = NETPROBE_SERVMODEL_THEDPL;
	nc.poolsize = 8;

	netProbeConfig_info(nc);

	#if (OSISWINDOWS == true)
		WSADATA wsaData;    // For windows system socket startup
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			printf(" There are no useable winsock.dll\n");
			printf(" Socket Initialisation failure. Error Code: %d.\n", WSAGetLastError());
			return 1;
		}
	#endif

	if (nc.server_model == NETPROBE_SERVMODEL_SELECT) {
		ConcurrentListenerUsingSelect(intToString(nc.lport).c_str(), nc);
	}
	if (nc.server_model == NETPROBE_SERVMODEL_THEDPL) {
		ConcurrentListenerUsingThreadPool(intToString(nc.lport).c_str(), nc);
	}

	#if (OSISWINDOWS == true)
		WSACleanup();
	#endif

	return 0;
}