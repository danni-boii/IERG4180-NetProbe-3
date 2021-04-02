all: client server
client: netprobeclient.o
	g++ -lpthread -o netprobecli netprobeclient.o
server: netprobeserver.o
	g++ -lpthread -o netprobesrv netprobeserver.o
netprobeclient.o: ./netprobe_client/netprobeclient.cpp ./netprobe_core.h
	g++ -c -lpthread ./netprobe_client/netprobeclient.cpp netprobe_core.h tinycthread.h
netprobeserver.o: ./netprobe_server/NetProbeServer.cpp ./netprobe_core.h
	g++ -c -lpthread ./netprobe_server/NetProbeServer.cpp netprobe_core.h tinycthread.h