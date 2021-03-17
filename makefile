all: client server
client: netprobeclient.o
	g++ -o netprobecli netprobeclient.o
server: netprobeserver.o
	g++ -o netprobesrv netprobeserver.o
netprobeclient.o: ./netprobe_client/netprobeclient.cpp ./netprobe_core.h
	g++ -c ./netprobe_client/netprobeclient.cpp netprobe_core.h tinycthread.h
netprobeserver.o: ./netprobe_server/NetProbeServer.cpp ./netprobe_core.h
	g++ -c ./netprobe_server/NetProbeServer.cpp netprobe_core.h tinycthread.h