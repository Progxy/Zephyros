#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <unistd.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
#endif //_WIN32

/* #include "../zephyros.h" */

#define FALSE 0
#define TRUE  1

#define SERVER_ADDR 0x7F000001 // Localhost: 127.0.0.1
#define SERVER_PORT 6969

static size_t str_len(const char* str) {
    if (str == NULL) return 0;
    const char* str_c = str;
	while (*str++);
    return (size_t) (str - str_c - 1);
}

static int str_tok(const char* str, const char* delim) {
     for (size_t i = 0, j = 0; i <= str_len(str); ++i) {
         if (delim[j] == str[i]) {
            if ((j + 1) == str_len(delim)) return i - j;
            j++;
         } else if (j > 0) i -= j, j = 0;
    }
    return -1;
}

#define CAST_PTR(ptr, type) ((type*) (ptr))

static void* mem_move(void* dest, const void* src, size_t size) {
    if (dest == NULL || src == NULL || size == 0) return NULL;
    
	size_t s = size;
	unsigned char* temp = (unsigned char*) calloc(size, sizeof(unsigned char));
	if (temp == NULL) {
		printf("Failed to allocate the temp buffer for mem_move.\n");
		return NULL;
	}
	
	unsigned char* dp = CAST_PTR(dest, unsigned char);
	unsigned char* sp = (unsigned char*) src;

	while (size--) *temp++ = *sp++;
	temp -= s, size = s;
	while (s--)	*dp++ = *temp++;
	
	free(temp - size);
    
    return dest;
}


#define mem_set(ptr, value, size)    mem_set_var(ptr, value, size, sizeof(unsigned char))
#define mem_set_32(ptr, value, size) mem_set_var(ptr, value, size, sizeof(unsigned int))
#define mem_set_64(ptr, value, size) mem_set_var(ptr, value, size, sizeof(unsigned long long int))
static void mem_set_var(void* ptr, long long int value, size_t size, size_t val_size) {
	if (ptr == NULL) return;
	unsigned char* p = (unsigned char*) ptr;
	const size_t s = size;
	while (size--) *p++ = CAST_PTR(&value, unsigned char)[(s - size) % val_size]; 
	return;
}

static void parse_json_request(const char* query, const int size) {
	if (size < 0) {
		printf("Failed to retrieve data\n");
		return;
	}
	
	printf("\nquery:\n\"%s\"\n", query);

	return;
}

#ifdef _WIN32

typedef SOCKET socket_zt;
#define close_socket closesocket
#define SOCK_CLEANUP() WSACleanup()
#define IS_INVALID_SOCK(s) ((s) == INVALID_SOCKET)

static int init_server(SOCKET* sock) {
	WSADATA wsaData = {0};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        perror("WSAStartup failed");
        return -1;
    }

    *sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (*sock == INVALID_SOCKET) {
        perror("Socket creation failed");
        WSACleanup();
        return -1;
    }

    int opt = 1;
    if (setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) == SOCKET_ERROR) {
        perror("setsockopt failed");
        closesocket(*sock);
        WSACleanup();
        return -1;
    }

    struct sockaddr_in serverAddr = {0};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(SERVER_ADDR);
    serverAddr.sin_port = htons(SERVER_PORT);
    
	if (bind(*sock, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        perror("bind failed");
        closesocket(*sock);
        WSACleanup();
        return -1;
    }

    if (listen(*sock, SOMAXCONN) == SOCKET_ERROR) {
        perror("listen failed");
        closesocket(*sock);
        WSACleanup();
        return -1;
    }

	return 0;
}

#else

typedef int socket_zt;
#define close_socket close
#define SOCK_CLEANUP()
#define IS_INVALID_SOCK(sock) ((sock) < 0)

static int init_server(int* sock) {
    *sock = socket(AF_INET, SOCK_STREAM, 0);
	if (*sock < 0) {
		perror("Failed to create the socket");
		return -1;
	}
	
	int val = 1;
	if (setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
		perror("Failed to set sock option, because");
		close(*sock);
		return -1;
	}
	
	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);
	addr.sin_addr = (struct in_addr) { .s_addr = htonl(SERVER_ADDR) };
	
	if (bind(*sock, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
		perror("Failed to bind the socket");
		return -1;
	}

	if (listen(*sock, 1) < 0) {
		perror("Failed to listen to the socket");
		return -1;
	}

	return 0;
}

#endif //_WIN32

int main (void) {
	const char response[] = "HTTP/1.1 204 No Content\r\n"
	"Access-Control-Allow-Origin: *\r\n"
	"Access-Control-Allow-Methods: POST, OPTIONS\r\n"
	"Access-Control-Allow-Headers: content-type\r\n"
	"Content-Length: 0\r\n"
	"Connection: keep-alive\r\n"
	"\r\n";

	const char data_response[] = "HTTP/1.1 200 OK\r\n"
	"Access-Control-Allow-Origin: *\r\n"
	"Content-Length: 939\r\n"
	"Connection: keep-alive\r\n"
	"Content-Type: application/json\r\n"
	"\r\n"
	"["
	"{\"city\":\"Perugia\",\"type\":1,\"temperature\":\"22°\",\"humidity\":\"69%\",\"pressure\":\"1015 hPa\",\"wind_velocity\":12,\"wind_direction\":\"120° SE\"},"
    "{\"city\":\"Perugia\",\"type\":2,\"temperature\":\"-2°\",\"humidity\":\"80%\",\"pressure\":\"1420 hPa\",\"wind_velocity\":12,\"wind_direction\":\"110° E\"},"
    "{\"city\":\"Perugia\",\"type\":3,\"temperature\":\"12°\",\"humidity\":\"40%\",\"pressure\":\"1069 hPa\",\"wind_velocity\":5,\"wind_direction\":\"130° NE\"},"
    "{\"city\":\"Perugia\",\"type\":4,\"temperature\":\"5°\",\"humidity\":\"30%\",\"pressure\":\"1014 hPa\",\"wind_velocity\":120,\"wind_direction\":\"20° S\"},"
    "{\"city\":\"Perugia\",\"type\":5,\"temperature\":\"22°\",\"humidity\":\"42%\",\"pressure\":\"1078 hPa\",\"wind_velocity\":12,\"wind_direction\":\"13° SW\"},"
    "{\"city\":\"Perugia\",\"type\":1,\"temperature\":\"32°\",\"humidity\":\"69%\",\"pressure\":\"1015 hPa\",\"wind_velocity\":69,\"wind_direction\":\"12° NW\"},"
    "{\"city\":\"Perugia\",\"type\":2,\"temperature\":\"-45°\",\"humidity\":\"80%\",\"pressure\":\"1420 hPa\",\"wind_velocity\":42,\"wind_direction\":\"71° W\"}"
    "]";
	
	socket_zt sock = 0;
	if (init_server(&sock) < 0) {
		printf("An error occurred while initializing the server.\n");
		return 1;
	}

	printf("Listening on http://localhost:6969 ...\n\n");
	
	unsigned int i = 0;
	while(TRUE) {
		struct sockaddr_in client_addr = {0};
		socklen_t client_addr_size = sizeof(client_addr);
		
		socket_zt client_sock = accept(sock, (struct sockaddr*) &client_addr, &client_addr_size);
		if (IS_INVALID_SOCK(client_sock)) {	
		#ifdef _WIN32
			int err = WSAGetLastError();
			char *msg = NULL;
			FormatMessageA(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL, err, 0, (LPSTR)&msg, 0, NULL);
			printf("Error message: %s\n", msg);
			LocalFree(msg);
		#else 
			perror("Failed to accept incoming connection");
		#endif //_WIN32
			return 1;
		}
		
		printf("--------------------------\n");
		printf("New connection accepted.\n");
		printf("--------------------------\n");

		printf("Receving:\n\"");

		int ret = 0;
		char buffer[256] = {0};
		unsigned long long int buf_cnt = 0;
		while ((ret = recv(client_sock, buffer, sizeof(buffer) - 1, 0)) >= 0) {
			buf_cnt += ret;
			printf("%s", buffer);
			int pos = 0;
			if ((pos = str_tok(buffer, "\r\n\r\n")) >= 0) {
				if (i == 0) break;
				parse_json_request((const char*) buffer + pos + 4, ret);
				break;
			}
			mem_set(buffer, 0, sizeof(buffer));
		}

		if (ret < 0) {
			perror("An error occurred while reading, because");
			return 1;
		}
		
		printf("\"\nRead %llu bytes.\n", buf_cnt);

		int res_len = 0;
		if (i == 0) {
			if ((res_len = send(client_sock, response, sizeof(response), 0)) != sizeof(response)) {
				if (res_len >= 0) printf("\nSent %d bytes out of %lu.\n", res_len, sizeof(response));
				perror("Failed to send the response");
				return 1;
			}
		} else {
			if ((res_len = send(client_sock, data_response, sizeof(data_response), 0)) != sizeof(data_response)) {
				if (res_len >= 0) printf("\nSent %d bytes out of %lu.\n", res_len, sizeof(data_response));
				perror("Failed to send the data_response");
				return 1;
			}
		}

		printf("Sent response back (%d)!\n", res_len);

		close_socket(client_sock);

		printf("-------------------\n");
		printf("Connection closed.\n");
		printf("-------------------\n");
		
		i = (i + 1) % 2;
	}
		
	close_socket(sock);
	SOCK_CLEANUP();

   	return 0;
}

