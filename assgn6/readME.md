# Custom Protocol using Raw Sockets

## Description
This project implements a server-client communication system using raw sockets. The server and client programs are built separately and require specific arguments to run.

## Usage
1. Compile the server and client programs using make:
   ```
   make server
   make client
   ```
### Server
To run the server program, use the following command:
```
sudo ./server [interface]
```
Replace `[interface]` with the network interface you want the server to listen on.

### Client
To run the client program, use the following command:
```
sudo ./client [destination_ip] [destination_mac] [interface]
```
Replace `[destination_ip]` with the IP address of the destination, `[destination_mac]` with the MAC address of the destination, and `[interface]` with the network interface to use for communication.

## Example
### Server
```
sudo ./server eth0
```
This command starts the server on the `eth0` network interface.

### Client
```
sudo ./client 192.168.1.100 00:11:22:33:44:55 eth0
```
This command runs the client, specifying `192.168.1.100` as the destination IP, `00:11:22:33:44:55` as the destination MAC address, and `eth0` as the network interface.
