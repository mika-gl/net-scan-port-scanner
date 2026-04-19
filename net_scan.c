#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <errno.h>

#define MAXPORT 60000
#define MAXPORT_LEN 5

void print_usage(void);
void explain_err(int);

int isDashInString(char *string);
int isPortRange(char *string, int *range);
int scan_host(char *host, char *port, struct addrinfo *options);
void display_port_state(char *port, int return_code, int errno_code);

void intToString(int i, char* string);
long power(int x, int y);


int main(int argc, char **argv) {
	if (argc != 3) { print_usage(); exit(0); }

	// take in host and port as arguments to pass as 'node' and 'service'
	char *host = argv[1];
	char *port = argv[2];


	// initialize addrinfo struct to pass as 'hints'
	struct addrinfo options = {0};
	options.ai_flags = AI_NUMERICSERV;
	options.ai_family = AF_INET;
	options.ai_socktype = SOCK_STREAM;
	options.ai_protocol = IPPROTO_TCP;


	// check if user entered a port range (using a dash)
	int range[2] = {0};
	int r = isPortRange(port, range);
	printf("Scan results for host: %s\n", host);
	printf("PORT\tSTATE\n");
	if (r) {

		for (int i=range[0]; i<=range[1]; ++i) {
			char p[MAXPORT_LEN];
			intToString(i, p);

			scan_host(host, p, &options);
		}
		exit(0);
	} else {
		//printf("single port specified\n");
		scan_host(host, port, &options);
	}
}

int scan_host(char *host, char *port, struct addrinfo *options) {
	
	// initialize pointer to pass as 'res'
	struct addrinfo *conn_list = NULL;

	// call getaddrinfo() and retrieve linked list with addrinfo pointer
	int r_code = getaddrinfo(host, port, options, &conn_list);

	if ( r_code != 0 ) {
		printf("getaddrinfo() failed, Error code: %i\n", r_code);
		return 1;
	}

	//printf("getaddrinfo() successfull\n");

	// conn_list is now a pointer to the desired addrinfo struct that contains struct sockaddr* which connet() needs

	// create a socket file descriptor as a local endpoint for communication to remote host
	int fd = socket(AF_INET, SOCK_STREAM, 6); // according to /etc/protocols, tcp = 6
	if (fd == -1) {
		printf("socket() failed, Error code: %i\n", fd);
		close(fd);
		return 1;
	}

	int connect_r_code = connect(fd, conn_list->ai_addr, conn_list->ai_addrlen);

	display_port_state(port, connect_r_code, errno);

	//printf("closing file descriptor on socket...\n");
	close(fd);
	freeaddrinfo(conn_list);

	return 0;
}

void print_usage() {
	printf("Usage: ./net_scan [Host] [Port]\n");
}
void display_port_state(char *port, int return_code, int errno_code) {
	if (return_code == 0) {
		// port is open
		printf("%s\topen\n", port);
	} else {
		if (errno_code == ETIMEDOUT) {
			printf("%s\tfiltered\n", port);
		} else if (errno_code == ENETRESET || errno_code == ECONNRESET || errno_code == ECONNREFUSED) {
			printf("%s\tclosed\n", port);
		}
	}
}

void explain_err(int errno_code) {
	switch (errno_code) {
		case ENETDOWN:
			printf("Network is down\n");
			break;
		case ENETUNREACH:	
			printf("Network unreachable\n");
			break;
		case ENETRESET:
			printf("Network dropped connection on reset\n");
			break;
		case ECONNRESET:
			printf("Connection reset by peer\n");
			break;
		case ETIMEDOUT:
			printf("Connection timed out\n");
			break;
		case ECONNREFUSED:
			printf("Connection refused\n");
			break;
	}
}

int isDashInString(char *string) {
	// return index of dash if present, 1-indexed
	for (int i=0; i<strlen(string); ++i) {
		if (*(string + i) == '-') {
			return i + 1;
		}
	}
	return 0;
}
void writeIntsFromRange(char* range_str, int *arr) {
	// arr must be of length 2
	int firstInt = 0;
	int secondInt = 0;
	// assume form: int dash int
	// get position of dash
	int dash_i = isDashInString(range_str) - 1;

	// read  range_str until dash
	for (int i=0; i<strlen(range_str); ++i) {
		if (i < dash_i) {
			// first int
			firstInt += (range_str[i]-'0') * power(10, dash_i - i - 1);
		} else if (i > dash_i) {
			// second int
			// length - index = will skip one char which we want (dash '-')
			int len = strlen(range_str) - dash_i;
			secondInt += (range_str[i]-'0') * power(10, len - (i - dash_i) - 1);
		}
	}
	arr[0] = firstInt;
	arr[1] = secondInt;
}
void intToString(int i, char* string) {
	// string_len must consider NUL byte
	// get length of i
	int k = 1;
	while (i / power(10, k) > 0) {
		++k;
	}
	// k = length of i
	// set each char to each digit
	for (int j=0; j<k; ++j) {
		string[j] = ((i % (power(10, (k-j)))) / power(10, (k-j-1))) + '0';
	}
	string[k] = '\0';
}
long power(int x, int y) {
	long result = 1;
	for (int i=0; i<y; ++i) {
		result = result * x;
	}
	return result;
}
int isPortRange(char *port, int *range) {
	int arr[2] = {0};
	if (isDashInString(port)) {
		writeIntsFromRange(port, arr);
		if (arr[0] > 0 && arr[0] < MAXPORT && arr[1] > 1 && arr[1] < MAXPORT && arr[0] < arr[1]) {
			range[0] = arr[0];
			range[1] = arr[1];
			return 1;	
		} else {
			printf("port range invalid\n");
			exit(1);
		}
	return 0;
	}
}
