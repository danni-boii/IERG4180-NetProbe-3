all: client server
client: netprobeclient.o
	g++ -o netprobecli netprobeclient.o
server: netprobeserver.o
	g++ -o netprobesrv netprobeserver.o
netprobeclient.o: ./netprobe_client/netprobeclient.cpp ./core.h
	g++ -c ./netprobe_client/netprobeclient.cpp core.h
netprobeserver.o: ./netprobe_server/NetProbeServer.cpp ./core.h
	g++ -c ./netprobe_server/NetProbeServer.cpp core.h