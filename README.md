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

--------------
## For Windows
--------------
> The client.exe file is placed at `\IERG4180-NetProbe-3\Debug\netprobe_client.exe`
> 
> The server.exe file is placed at `\IERG4180-NetProbe-3\Debug\netprobe_server.exe`
> 
> The client source file is located at `\IERG4180-NetProbe-3\NetProbe_Client\netprobe_client.cpp`
> 
> The server source file is located at `\IERG4180-NetProbe-3\NetProbe_Server\netprobe_server.cpp`
> 
> The common header file is located at `\IERG4180-NetProbe-3\core.h`

To run the executable file

> Go in to the folder containing the exe file `cd \IERG4180-NetProbe-3\Debug`
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
> - \NetProbe_Client\netprobe_client.cpp
> - \NetProbe_Server\netprobe_server.cpp
> - \core.h
> - \makefile
> 
> `make all` to compile the executable files
> 
> The executable files are compiled as 'netprobecli' (Client) and 'netprobesrv' (Server)
>
> Go in to the folder containing the executables `cd /IERG4180-NetProbe-3/Debug`
>
> run the file `./netprobecli -send -proto tcp`
>
> type `./netprobecli -help` for all the possible parameters.
> 
--------------
