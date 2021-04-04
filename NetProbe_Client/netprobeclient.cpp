/*
	NetProbe project 3
	@author : Wong Cho Hei
	SID : 1155109484
*/

// The default command for using this exe is
// cd C:\Users\Danny Boy\Documents\GitHub\IERG4180_Project3\IERG4180-NetProbe-3\Debug
// netprobe_client.exe [arguments]

#include "../netprobe_core.h"

using namespace std;

int main(int argc, char* argv[]) {
	NetProbeConfig nc;
	nc.mode = NETPROBE_SEND_MODE;	 //Default send mode
	nc.protocol = NETPROBE_UDP_MODE; //Default UDP send

	if (argc == 1) {
		usage_message_client();
		return 0;
	}
	else if (argc >= 2) {
		//Deal with all the input value
		for (int i = 1; i < argc; i++) {
			//cout << "i = " << i << ", thing = ["<< argv[i] <<"]" << endl; //Debug
			if (strcmp(argv[i], "-help") == 0) {
				usage_message_client();
				return 0;
			}
			else if (strcmp(argv[i], "-send") == 0) {
				nc.mode = NETPROBE_SEND_MODE;
			}
			else if (strcmp(argv[i], "-recv") == 0) {
				nc.mode = NETPROBE_RECV_MODE;
			}
			else if (strcmp(argv[i], "-response") == 0 || strcmp(argv[i], "-resp") == 0) {
				nc.mode = NETPROBE_RESP_MODE;
			}
			else if (strcmp(argv[i], "-proto") == 0) {
				if (argv[i + 1] != NULL && argv[i + 1] != "") {
					if (strcmp(strlwr(argv[i + 1]), "udp") == 0) {
						nc.protocol = NETPROBE_UDP_MODE;
					}
					else if (strcmp(strlwr(argv[i + 1]), "tcp") == 0) {
						nc.protocol = NETPROBE_TCP_MODE;
					}
				}
			}
			else if (strcmp(argv[i], "-pktsize") == 0) {
				if (argv[i + 1] != NULL && argv[i + 1] != "") {
					nc.pkt_size_inbyte = atoi(argv[i + 1]);
				}
			}
			else if (strcmp(argv[i], "-pktrate") == 0) {
				if (argv[i + 1] != NULL && argv[i + 1] != "") {
					nc.pkt_rate_bytepersec = atoi(argv[i + 1]);
				}
			}
			else if (strcmp(argv[i], "-pktnum") == 0) {
				if (argv[i + 1] != NULL && argv[i + 1] != "") {
					nc.pkt_num = atoi(argv[i + 1]);
				}
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
			else if (strcmp(argv[i], "-rhost") == 0) {
				if (argv[i + 1] != NULL && argv[i + 1] != "") {
					nc.rhostname = argv[i + 1];
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
		}
	}


	netProbeConfig_info(nc);

	SOCKET s;
	struct sockaddr_in server;

	#if (OSISWINDOWS == true)
		WSADATA wsaData;    // For windows system socket startup
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			printf(" There are no useable winsock.dll\n");
			printf(" Socket Initialisation failure. Error Code: %d.\n", WSAGetLastError());
			return 1;
		}
	#endif
	//Initialisation success

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		#if(OSISWINDOWS==true)
			printf(" Could not create socket : %d\n", WSAGetLastError());
		#else
			perror(" Could not create socket.\n");
		#endif
	}
	//TCP socket create successful
	printf("\n Socket created.\n");

	/// Connect to the Server - TCP Common Code ///

	string ipv4hostname = hostnametoIPV4(nc.rhostname); //lookup the server ip
	if (ipv4hostname != "") { // Successful
		server.sin_addr.s_addr = inet_addr(ipv4hostname.c_str());
		server.sin_family = AF_INET;
		server.sin_port = htons((short)nc.rport);

		//Output info
		cout << "\n Remote host " << nc.rhostname << " lookup successful, IP address is " << ipv4hostname << endl;
		if (nc.sbufsize_inbyte <= 0) {
			nc.sbufsize_inbyte = 65536;
			cout << " Default send buffer size = " << nc.sbufsize_inbyte << " bytes" << endl;
		}
		if (nc.rbufsize_inbyte <= 0) {
			nc.rbufsize_inbyte = 65536;
			cout << " Default recv buffer size = " << nc.rbufsize_inbyte << " bytes" << endl;
		}

		//Set Send Buffer Size
		if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char*)(&nc.sbufsize_inbyte), sizeof(nc.sbufsize_inbyte)) < 0)
		{
			#if(OSISWINDOWS==true)
				printf(" setsockopt() for SO_KEEPALIVE failed with error: %u\n", WSAGetLastError());
			#else
				perror(" setsockopt() for SO_KEEPALIVE failed.\n");
			#endif
			return 5;
		}
	}
	else {
		cout << "\n Remote host " << nc.rhostname << " lookup failed." << endl;
		return 1;
	}

	cout << " Connecting to remote host " << nc.rhostname << " ... ";

	//Connect to remote server
	if (connect(s, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
	{
		puts(" Connection error.");
		closesocket_comp(s);
		return 1;
	}

	cout << "successful" << endl;

	//Sending header to the server
	sendNCbuildMessage(nc, s);
	cout << " Request received at server. " << endl;

	//UDP socket
	if (nc.protocol == NETPROBE_UDP_MODE) {
		closesocket_comp(s); //Close the previous TCP socket

		if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
		{
			#if(OSISWINDOWS==true)
				printf(" Could not create socket : %d\n", WSAGetLastError());
			#else
				perror(" Could not create socket.\n");
			#endif
			return 1;
		}
		printf(" Socket created.\n");

		if (nc.mode == NETPROBE_SEND_MODE || nc.mode == NETPROBE_RESP_MODE) {
			//Sendto IP
			struct sockaddr_in si_other;

			string ipv4hostname = hostnametoIPV4(nc.rhostname);
			if (ipv4hostname != "")//Successful DNS resolved
			{
				cout << "\n Remote host " << nc.rhostname << " lookup successful, IP address is " << ipv4hostname << endl;
				if (nc.sbufsize_inbyte <= 0) {
					nc.sbufsize_inbyte = 65536;
					cout << " Default send buffer size = " << nc.sbufsize_inbyte << " bytes" << endl;
				}
				if (nc.rbufsize_inbyte <= 0) {
					nc.rbufsize_inbyte = 65536;
					cout << " Default recv buffer size = " << nc.rbufsize_inbyte << " bytes" << endl;
				}
			}
			else {
				cout << "\n Remote host " << nc.rhostname << " lookup failed." << endl;
				return 1;
			}
			memset((char*)&si_other, 0, sizeof(si_other));
			si_other.sin_family = AF_INET;
			si_other.sin_port = htons((short)nc.rport);
			si_other.sin_addr.s_addr = inet_addr(ipv4hostname.c_str());

			//Set Send Buffer Size
			if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char*)(&nc.sbufsize_inbyte), sizeof(nc.sbufsize_inbyte)) < 0)
			{
				#if(OSISWINDOWS==true)
					printf(" setsockopt() for SO_KEEPALIVE failed with error: %u\n", WSAGetLastError());
				#else
					perror(" setsockopt() for SO_KEEPALIVE failed\n");
				#endif
				return 5;
			}

			long pkt_num = 0;
			long total_sent_size = 0;
			char* packet_num = (char*)malloc(((sizeof(pkt_num) * CHAR_BIT) + 2) / 3 + 2);
			char* message = (char*)malloc(sizeof(char) * nc.pkt_size_inbyte);

			//Calculate the timeinterval between packets.
			int time_interval_ms = pkt_timeInterval(nc);
			int last_show_var = 0;

			int slen = sizeof(si_other);
			int recv_len = 0;
			unsigned int slen_linux = sizeof(si_other);

			clock_t starting_time = clock();
			clock_t prev_pkt_time = starting_time;
			clock_t recv_pkt_time = starting_time;
			double mean_recv_time = 0;
			double mean_jitter_time = 0;
			float minResponseTime = FLT_MAX;	//minimum response time in ms
			float maxResponseTime = 0;			//maximum response time in ms
			long recvfrom_pkt = 0;

			while (true)
			{
				//cout << " I've reached here!!" << endl;
				if (nc.pkt_num != 0 && pkt_num > nc.pkt_num) break;
				if (((clock() - starting_time) / time_interval_ms) > last_show_var) {
					last_show_var = (clock() - starting_time) / time_interval_ms;
					strcpy(message, "send_pkt_num= ");
					sprintf(packet_num, "%ld", pkt_num);
					strcat(message, packet_num);
					//send the message
					int sendto_size = sendto(s, message, nc.pkt_size_inbyte, 0, (struct sockaddr*) & si_other, sizeof(si_other));
					if ( nc.mode == NETPROBE_RESP_MODE ) {
						ZeroMemory(message, sizeof(message));
						#if(OSISWINDOWS==true)
							recv_len = recvfrom(s, message, nc.pkt_size_inbyte, 0, (sockaddr*)&si_other, &slen);
						#else
							recv_len = recvfrom(s, message, nc.pkt_size_inbyte, 0, (sockaddr*)&si_other, &slen_linux);
						#endif
						recv_pkt_time = clock();
						recvfrom_pkt++;
					}
					
					if (sendto_size == SOCKET_ERROR || sendto_size < 0 || (nc.mode == NETPROBE_RESP_MODE && recv_len <= 0 ))
					{
						#if(OSISWINDOWS==true)
							printf(" sendto() failed with error code : %d", WSAGetLastError());
						#else
							perror(" sendto() failed.\n");
						#endif
						break;
					}
					total_sent_size += sendto_size;
					pkt_num++;
					ZeroMemory(message, sizeof(message));
				}
				if (nc.mode == NETPROBE_SEND_MODE) {
					sendInfo(nc, starting_time, pkt_num, total_sent_size);
				}
				if (nc.mode == NETPROBE_RESP_MODE) {
					//Calculate the jitter time
					clock_t pkt_taken_time = recv_pkt_time - prev_pkt_time;
					float pkt_taken_time_ms = (float)pkt_taken_time / CLOCKS_PER_SEC * 1000;
					if (pkt_taken_time_ms < minResponseTime) {
						minResponseTime = pkt_taken_time_ms;
					}
					if (pkt_taken_time_ms > maxResponseTime) {
						maxResponseTime = pkt_taken_time_ms;
					}
					mean_recv_time = (float)((mean_recv_time * (recvfrom_pkt - 1)) + pkt_taken_time) / (recvfrom_pkt);
					mean_jitter_time = (float)((mean_jitter_time * (recvfrom_pkt - 1)) + (pkt_taken_time - mean_jitter_time)) / (recvfrom_pkt);
					prev_pkt_time = recv_pkt_time;
					response_message_client(starting_time, recvfrom_pkt, minResponseTime, maxResponseTime, mean_recv_time, mean_jitter_time);
				}
			}
			closesocket_comp(s);
		}
		if (nc.mode == NETPROBE_RECV_MODE) {
			//Recvfrom IP
			int recv_len, slen;
			char* buf = (char*)malloc(sizeof(char) * nc.pkt_size_inbyte);
			sockaddr_in si_other;    //The client socketaddr_in
			slen = sizeof(si_other);

			string ipv4hostname = hostnametoIPV4(nc.lhostname);
			if (nc.lhostname == "INADDR_ANY")
				server.sin_addr.s_addr = INADDR_ANY;
			else if (ipv4hostname != "") {
				server.sin_addr.s_addr = inet_addr(ipv4hostname.c_str());
			}
			server.sin_family = AF_INET;
			server.sin_port = htons((short)nc.lport);

			if (nc.sbufsize_inbyte <= 0) {
				nc.sbufsize_inbyte = 65536;
				cout << " Default send buffer size = " << nc.sbufsize_inbyte << " bytes" << endl;
			}
			if (nc.rbufsize_inbyte <= 0) {
				nc.rbufsize_inbyte = 65536;
				cout << " Default recv buffer size = " << nc.rbufsize_inbyte << " bytes" << endl;
			}
			if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char*)(&nc.rbufsize_inbyte), sizeof(nc.rbufsize_inbyte)) < 0)
			{
				#if(OSISWINDOWS==true)
					printf(" setsockopt() for SO_KEEPALIVE failed with error: %u\n", WSAGetLastError());
				#else
					perror(" setsockopt() for SO_KEEPALIVE failed\n");
				#endif
				return 5;
			}

			cout << " Binding local socket to port " << nc.lport;

			//Bind
			if (bind(s, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
			{
				#if(OSISWINDOWS==true)
					cout << " failed with error code : " << WSAGetLastError() << "\n";
				#else
					perror(" failed.\n");
				#endif
				exit(1);
			}

			cout << " successful" << endl;

			printf(" Waiting for incoming datagrames ...\n");

			long expected_pkt_num = 0;
			long recved_pkt_num = 0;
			long recved_pkt_real_num = 0;
			long lost_pkt = 0;
			long total_recv_size = 0;
			bool shown_info = false;

			clock_t starting_time = clock();
			clock_t prev_pkt_time = starting_time;
			double mean_recv_time = 0;
			double mean_jitter_time = 0;

			//keep listening for data
			while (true)
			{
				//Try to receive some data, this is a blocking call
				#if(OSISWINDOWS==true)
					recv_len = recvfrom(s, buf, nc.pkt_size_inbyte, 0, (sockaddr*)&si_other, &slen);
				#else
					unsigned int slen_linux = sizeof(si_other);
					recv_len = recvfrom(s, buf, nc.pkt_size_inbyte, 0, (sockaddr*)&si_other, &slen_linux);
				#endif
					clock_t recv_pkt_time = clock();
					if (recv_len == SOCKET_ERROR || recv_len < 0)
					{
				#if(OSISWINDOWS==true)
					if (WSAGetLastError() == WSAEWOULDBLOCK) { continue; }
					printf(" recvfrom() failed with error code : %d\n", WSAGetLastError());
				#else
					perror(" recvfrom() failed.\n");
				#endif
					break;
				}
				if (!shown_info) {
					printf("Receiving from [%s:%d]:\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
					shown_info = true;
				}

				total_recv_size += recv_len;
				sscanf(buf, "%*s %ld", &recved_pkt_num);
				if (recved_pkt_num != expected_pkt_num) {
					if (recved_pkt_num > expected_pkt_num) {
						lost_pkt += recved_pkt_num - expected_pkt_num;
						expected_pkt_num = recved_pkt_num + 1;
					}
					else {
						lost_pkt += expected_pkt_num - recved_pkt_num;
						expected_pkt_num = recved_pkt_num + 1;
					}
				}
				else {
					expected_pkt_num++;
				}

				//Calculate the jitter time
				clock_t pkt_time_taken = recv_pkt_time - prev_pkt_time;
				mean_recv_time = ((mean_recv_time * recved_pkt_real_num) + (double)pkt_time_taken) / (recved_pkt_real_num + 1);
				mean_jitter_time = ((mean_jitter_time * recved_pkt_real_num) + ((double)pkt_time_taken - mean_jitter_time)) / (recved_pkt_real_num + 1);


				//printf("recv_pkt_time: %ld, prev_pkt_time: %ld\n", recv_pkt_time, prev_pkt_time);    //Debug
				//printf("mean_recv_time: %lf, recv_pkt_time: %ld\n", mean_recv_time, recv_pkt_time);    //Debug

				recvInfo(nc, starting_time, recved_pkt_num, lost_pkt, total_recv_size, mean_jitter_time);
				//printf("Expected: %ld, recved: %ld\n", expected_pkt_num, recved_pkt_num);    //Debug
				recved_pkt_real_num++;

				//Clear the buffer by filling \0, it might have previously received data
				ZeroMemory(buf, sizeof(buf));
				prev_pkt_time = recv_pkt_time;
			}
			closesocket_comp(s);
		}
	}

	//TCP socket
	if (nc.protocol == NETPROBE_TCP_MODE) {
		if (nc.mode == NETPROBE_SEND_MODE || nc.mode == NETPROBE_RESP_MODE) {     //Sending to IP
			//Send some data
			char* packet = (char*)malloc(sizeof(char) * nc.pkt_size_inbyte);
			long sent_pkt = 0;
			int sent_byte = 0;
			long recv_pkt = 0;
			int recv_byte = 0;
			float minResponseTime = FLT_MAX;	//minimum response time in ms
			float maxResponseTime = 0;			//maximum response time in ms
			long total_sent_size = 0;
			char* packet_num = (char*)malloc(((sizeof(sent_pkt) * CHAR_BIT) + 2) / 3 + 2);

			int time_interval_ms = pkt_timeInterval(nc);

			clock_t starting_time = clock();
			clock_t prev_pkt_time = starting_time;
			clock_t recv_pkt_time = starting_time;
			double mean_recv_time = 0;
			double mean_jitter_time = 0;

			int last_show_var = 0;

			while (true) {
				if (((clock() - starting_time) / time_interval_ms) > last_show_var) {
					last_show_var = (clock() - starting_time) / time_interval_ms;
					strncpy(packet, "sending_pkt_num= ", 18);
					sprintf(packet_num, "%ld ", sent_pkt);
					strcat(packet, packet_num);

					sent_byte = send(s, packet, nc.pkt_size_inbyte, 0);
					ZeroMemory(packet, sizeof(packet));
					if (nc.mode == NETPROBE_RESP_MODE) {
						recv_byte = recv(s, packet, nc.pkt_size_inbyte, 0);
						recv_pkt_time = clock();
					}
					if (sent_byte < 0  || (nc.mode == NETPROBE_RESP_MODE && recv_byte < 0))
					{
						#if(OSISWINDOWS==true)
							printf("\n Send failed, error: %u\n", WSAGetLastError());
						#else
							perror("\n Send failed.\n");
						#endif

						break;
					}
					sent_pkt++;
					total_sent_size += sent_byte;
					if (sent_pkt >= nc.pkt_num && nc.pkt_num != 0) break;
				}
				if (nc.mode == NETPROBE_SEND_MODE) {
					sendInfo(nc, starting_time, sent_pkt, total_sent_size);
				}
				if (nc.mode == NETPROBE_RESP_MODE) {
					recv_pkt++;
					//Calculate the jitter time
					clock_t pkt_taken_time = recv_pkt_time - prev_pkt_time;
					float pkt_taken_time_ms = (float)pkt_taken_time / CLOCKS_PER_SEC * 1000;
					if (pkt_taken_time_ms < minResponseTime) {
						minResponseTime = pkt_taken_time_ms;
					}
					if (pkt_taken_time_ms > maxResponseTime) {
						maxResponseTime = pkt_taken_time_ms;
					}
					mean_recv_time = (float)((mean_recv_time * (recv_pkt-1)) + pkt_taken_time) / (recv_pkt);
					mean_jitter_time = (float)((mean_jitter_time * (recv_pkt-1)) + (pkt_taken_time - mean_jitter_time)) / (recv_pkt);
					prev_pkt_time = recv_pkt_time;
					response_message_client(starting_time, recv_pkt, minResponseTime, maxResponseTime, mean_recv_time, mean_jitter_time);
					ZeroMemory(packet, sizeof(packet));
				}
			}
			closesocket_comp(s);
		}
		if (nc.mode == NETPROBE_RECV_MODE) {     //Receiving from IP
			sockaddr_in client_addr;
			int addr_len = sizeof(client_addr);
			int client_addr_len = sizeof(sockaddr);

			#if(OSISWINDOWS==true)
				getpeername(s, (struct sockaddr*) & client_addr, &addr_len);
			#else
				unsigned int addr_len_linux = sizeof(client_addr);
				getpeername(s, (struct sockaddr*) & client_addr, &addr_len_linux);
			#endif
			char* recv_buf = (char*)malloc(sizeof(char) * nc.pkt_size_inbyte);
			int recv_size;
			SOCKET client_socket;

			char* client_ip = inet_ntoa(client_addr.sin_addr);
			int client_port = ntohs(client_addr.sin_port);
			long packet_num = 0;
			long total_recv_size = 0;

			cout << " Connected to [" << client_ip << "], port: " << client_port << endl;

			clock_t starting_time = clock();
			clock_t prev_pkt_time = starting_time;
			double mean_recv_time = 0;
			double mean_jitter_time = 0;

			//Receive from the server
			while (true) {
				if ((recv_size = recv(s, recv_buf, nc.pkt_size_inbyte, 0)) == SOCKET_ERROR)
				{
					#if(OSISWINDOWS==true)
						printf(" Recv Failed. %d\n", WSAGetLastError());
						closesocket(s);
					#else
						perror(" Recv Failed.\n");
						close(s);
					#endif
					break;
				}
				clock_t recv_pkt_time = clock();
				total_recv_size += recv_size;
				sscanf(recv_buf, "%*s %ld %*s", &packet_num);

				//Calculate the jitter time
				clock_t pkt_taken_time = recv_pkt_time - prev_pkt_time;
				mean_recv_time = (double)((mean_recv_time * packet_num) + pkt_taken_time) / (packet_num + 1);
				mean_jitter_time = (double)((mean_jitter_time * packet_num) + (pkt_taken_time - mean_jitter_time)) / (packet_num + 1);
				prev_pkt_time = recv_pkt_time;

				recvInfo(nc, starting_time, packet_num, 0, total_recv_size, mean_jitter_time);
				ZeroMemory(recv_buf, sizeof(recv_buf));
			}
			closesocket_comp(s);
		}
	}
	#if (OSISWINDOWS==true)
		WSACleanup();
	#endif
}