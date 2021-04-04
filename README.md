# IERG4180_Project3
## NetProbe Client-Server Multithreaded Edition
### SID: 1155109484
### Name: Wong Cho Hei
--------------

The project was developed using Visual Studio 2019 version-16.9.3 (Windows)

All the function comments are in Doxygen Comment Style

This project used the following libery (with my own modification)

- [TinyCThread v1.2](https://github.com/tinycthread/tinycthread)
- [tinycthreadpool](https://github.com/enbandari/tinycthreadpool)

Thank you for the contribution for this project

The experiment report is named `Experiment-Asg3.docx`

--------------
## For Windows
--------------
> The client.exe file is placed at `\IERG4180-NetProbe-3\Debug\NetProbe_Client.exe`
> 
> The server.exe file is placed at `\IERG4180-NetProbe-3\Debug\NetProbe_Server.exe`
> 
> The client source file is located at `\IERG4180-NetProbe-3\NetProbe_Client\NetProbeClient.cpp`
> 
> The server source file is located at `\IERG4180-NetProbe-3\NetProbe_Server\NetProbeServer.cpp`
> 
> The common header file is located at `\IERG4180-NetProbe-3\netprobe_core.h`
>
> The included header files are located at `\IERG4180-NetProbe-3\tinycthread*.*`
>
>
To run the programme
>
> Go into the folder containing the exe file `cd \IERG4180-NetProbe-3\Debug`
>
> run exe with arguments `netprobe_client.exe -send -proto tcp`
>
> type `netprobe_client.exe -help` for all the possible parameters.
--------------
## For Linux
--------------
> Thr programme was only tested on Ubuntu 20.04 LTS 64bit
> 
> Please git clone this project or download the following file
> - \NetProbe_Client\NetProbeClient.cpp
> - \NetProbe_Server\NetProbeServer.cpp
> - \netprobe_core.h
> - \tinycthread.h
> - \tinycthread.c
> - \tinycthread_pool.h
> - \tinycthread_pool.c
> - \makefile
> 
> Go into the downloaded folder `cd /IERG4180-NetProbe-3`
>
> type `make all` to compile the executable files
> 
> The executable files are compiled as 'netprobecli' (Client) and 'netprobesrv' (Server)
>
To run the programme
>
> run the file `./netprobecli -send -proto tcp`
>
> type `./netprobecli -help` for all the possible parameters.
> 
--------------
## Sample Video

![Demo](/Demo.mp4)