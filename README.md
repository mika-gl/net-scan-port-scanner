# net-scan-port-scanner
A proof of concept TCP scanner that uses the connect() system call to test how a server responds to probes.

# NET\_SCAN.C, A TCP PORT SCANNER IN C
### Video Demo:  <URL HERE>
### Overview:

This program is a command line TCP port scanning tool implemented using the C programming language.\
In its arguments it receives a 'Host', using an IP address or a domain name to identify which device will be scanned, and a 'Port', which serves to identify the device's logical TCP port that will be probed.
```
	./net_scan [Host] [Port]
```
The program supports as input an individual port to scan as well as an inclusive port range (e.g. 22-80).\
After executing along with the required parameters, the program will output the discovered state of the probed ports based on the response, or lack thereof, of the device to the connection attempt. These states can be one of: 'open', 'closed', or 'filtered'.\
A tool such as this one is useful for quickly analyzing the state of TCP ports in a device. These results can then be used to in an assessment of the security of the system and help mitigate possible vulnerabilities or misconfigurations present.\
Commonly this is performed by cybersecurity professionals and system administrators.

### Background on TCP connections

When apps need to connect over a network they usually use one of two transport layer protocols: TCP and UDP.\
TCP is the most prevalent, making up about 80% percent Internet traffic.\
For an application to connect using TCP, (considering relevance to this README) it has to be assigned a local port on the system, to which the responses from the peer will be sent, and it also has to choose to what remote port it will send its own messages.\
The local port is usually a temporary session-only port assigned by the OS, and the remote port (in the case of connecting to a server) is usually a standard number set according to the service 'listening' or waiting for connections on the server.

### TCP three-way handshake

When a client needs to start a connection to communicate with a server (and wants to use the TCP protocol), it performs a TCP handshake.\
The 'handshake' is a series of specific TCP messages sent between the client and the server that signal that communication is intended to start and, if successful, data transfer can begin, as both the server and the client are available to communicate on the requested ports.\
The specific type of messages sent are TCP segments with certain 'flags' set on to signal each part of the handshake. The order of flags sent is as follows:
```
1. Client sends TCP with SYN set, signaling it wants to initiate a connection.
2. Server responds with TCP with both SYN and ACK set, signaling its available to start a connection.
3. Client responds with TCP with ACK set, signaling it acknowledges that the connection was accepted, and data transfer can begin.
```
### Port state

In the case of a successfull TCP handshake, we can assume that the server accepts arbitrary connections on the given port, meaning that it is **'open'**.\
If a connection attempt via a SYN packet is answered with RST instead of SYN/ACK, this means the server is actively refusing connections for that port. We can say that it is **'closed'**.\
Finally if a connection attempt is not responded to at all with any type of TCP message, and we know that the server is up and reachable, it is possible that some type of firewall rule is silently dropping the packets before they reach the OS and trigger the normal response for a closed port (RST). This type of port is commonly referred to as being **'filtered'**.\
Firewalls are often configured to drop packets based on a set of rules to hide the real state of the port, to strengthen security.

### Implementation

All of the source code for the program is laid out in a single file called 'net\_scan.c'.\
The implementation of the scanning function makes use of the following standard library functions:
```
	int getaddrinfo(const char *restrict node,
		const char *restrict service,
		const struct addrinfo *restrict hints,
		struct addrinfo **restrict res);
```
Used to retrieve an addrinfo struct which will contain the sockaddr struct pointer necessary for the connect() syscall.
```
	int socket(int domain, int type, int protocol);
```
Syscall used to create a local socket to communicate and receive the messages back from the connected remote host.
Returns the socket's file descriptor used for the connect() syscall. 
```
	int connect(int sockfd, const struct sockaddr *addr,
                   socklen_t addrlen);
```
Syscall that tries to establish a connection to the remote host, in this case a TCP handshake.\
From the libraries:
```
	<sys/types.h>
	<sys/socket.h>
	<netdb.h>
```
The source code file contains various helper functions but the main scanning logic resides in: 'scan\_host()'.\
This function is called for each of the ports that will be probed.\
First we initialize a null pointer.
```
	struct addrinfo *conn_list = NULL;
```
That pointer will be filled by passing its address to the getaddrinfo() function, which will allocate an addrinfo struct in memory and put its address in the pointer variable.\
We then call getaddrinfo() passing: Host, Port, options (an addrinfo struct defined earlier that specifies options like address family, protocol, and others that will be used to define the returning addrinfo struct), and &conn\_list.
```
	int r_code = getaddrinfo(host, port, options, &conn_list);
```
After checking if the function failed using its return value, we call socket() to get a socket from the OS and return its file descriptor number.\
We use AF\_INET for IPv4, SOCK\_STREAM for connection based communication since we are using TCP, and 6, an int that represents the protocol TCP.
```
	int fd = socket(AF_INET, SOCK_STREAM, 6); // according to /etc/protocols, tcp = 6
```
Then we call connect() using the provided file descriptor for the socket, and the values that represent the wanted connection:  'ai\_addr' and 'ai\_addrlen' found in the struct \*conn\_list we got from getaddrinfo().
```
	int connect_r_code = connect(fd, conn_list->ai_addr, conn_list->ai_addrlen);
```
Finally we use a helper function to interpret the return value of the connect() call (to check if it succeeded or failed) and the value of 'errno' which is a special variable defined in 'errno.h' that is written to with an integer specific to the error that caused connect() to fail.
```
	display_port_state(port, connect_r_code, errno);
	close(fd);
	freeaddrinfo(conn_list);
```
The display\_port\_state() function will display the port as open if the return value of connect() (connect\_r\_code) is 1, meaning the full TCP handshake was completed.\
If the return value is 0, meaning the call failed, it will check the value of errno for the specific cause, using the ENUMS defined in the libraries for readability as follows:
```
	if (errno_code == ETIMEDOUT) {
		printf("%s\tfiltered\n", port);
	} else if (errno_code == ENETRESET || errno_code == ECONNRESET || errno_code == ECONNREFUSED) {
		printf("%s\tclosed\n", port);
	}
```
After this, the socket is closed and the memory allocated for the struct \*conn\_list by getaddrinfo() is freed using freeaddrinfo().

### Specific design decisions

Some of the functionalities in the tool could have been implemented in different ways, therefore a few design choices are explained in this section:\
***Why use connect() instead of raw SYN packets?***\
The connect() interface was used in order to abstract away the complications of having to manually craft and parse TCP packets as raw packet control was not needed in a project that is about the high level nature of TCP connections.\

***Why restrict to IPv4? (AF\_INET)***\
IPv4 is most widely used than IPv6 and for the necessary functionality of this project (to probe port statuses using TCP connection atempts) there was no need for the extended complexity of IPv6, as IPv4 can accomplish the desired goal.\

***Why chose sequential scanning instead of multithreading?***\
Sequential scanning is easier and faster to develop and works fine for simple port scanning functionality to reproduce TCP concepts, which is the main purpose of the project. Not heavy-load port mapping.\

***Why no UDP scanning?***\
Including UDP scanning would add complications as it is not a connection based protocol and would extend the development time and complexity, though it is considered in the future improvements section along with other extra functionality.

### Possible future improvements

**Format output**\
Supporting multiple output formats (TXT, CSV, JSON) would allow for easier logging of scans.\
**Multithreading**\
This would make the scanning faster and be able to support large ranges of IPs or/and ports.\
**Subnet scans**\
Support for whole subnet scans would make mapping a complete network segment possible (e.g. 192.168.0.0/24 for 1-254 hosts).\
**IPv6 support**\
Supporting IPv6 would allow for security assessments of IPv6 systems which more and more organizations are migrating to.\ 
**Banner grabbing**\
Adding banner grabbing would allow the scanner to fetch data output from an open port and analyze it to be able to determine which specific service is listening on the probed port.\
**Timeout customization**\
Supporting timeout customization would allow users to make a more efficient use of the tool by letting them specify how long the scanner waits before it stops trying to connect and marks the connection as timed out.

### Ethical Considerations

As with most security tools, it is important to consider that probing unauthorized systems is dangerous as port scanning unauthorized systems can trigger security warnings and in some jurisdictions it is considered ilegal.\
This tool should only be used to test owned systems or ones given permission to.
