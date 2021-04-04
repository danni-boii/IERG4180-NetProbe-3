all: client server
client: NetProbeClient.o
	g++ -o netprobecli NetProbeClient.o
server: NetProbeServer.o tinycthread.o tinycthread.o tinycthread_pool.o
	g++ -o netprobesrv NetProbeServer.o -lpthread tinycthread.o tinycthread_pool.o
tinycthread.o: ./tinycthread.c ./tinycthread.h
	g++ -c -lpthread ./tinycthread.c tinycthread.h
tinycthread_pool.o: ./tinycthread_pool.c ./tinycthread_pool.h tinycthread.o
	g++ -c -lpthread ./tinycthread_pool.c tinycthread_pool.h tinycthread.o
NetProbeClient.o: ./NetProbe_Client/NetProbeClient.cpp netprobe_core.h
	g++ -c ./NetProbe_Client/NetProbeClient.cpp netprobe_core.h
NetProbeServer.o: ./NetProbe_Server/NetProbeServer.cpp
	g++ -c -lpthread ./NetProbe_Server/NetProbeServer.cpp netprobe_core.h 