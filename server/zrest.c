#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "../zephyros.h"

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

int main (void) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Failed to create the socket");
		return 1;
	}
	
	int val = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
	
	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);
	addr.sin_addr = (struct in_addr) { .s_addr = htonl(SERVER_ADDR) };
	
	if (bind(sock, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
		perror("Failed to bind the socket");
		return 1;
	}

	if (listen(sock, 1) < 0) {
		perror("Failed to listen to the socket");
		return 1;
	}

	const char response[] = "HTTP/1.1 204 No Content\r\n"
	"Access-Control-Allow-Origin: *\r\n"
	"Access-Control-Allow-Methods: POST, OPTIONS\r\n"
	"Access-Control-Allow-Headers: content-type\r\n"
	"Content-Length: 0\r\n"
	"Connection: close\r\n"
	"\r\n";

	const char data_response[] = "HTTP/1.1 200 OK\r\n"
	"Access-Control-Allow-Origin: *\r\n"
	"Content-Length: 676\r\n"
	"Connection: close\r\n"
	"Content-Type: application/json\r\n"
	"\r\n"
	"["
    "{\"city\":\"Perugia\",\"type\":1,\"temperature\":\"22°C\",\"humidity\":\"69%\",\"pressure\":\"1015 hPa\",\"wind_velocity\":12,\"wind_direction\":\"120° SE\"},"
    "{\"city\":\"Perugia\",\"type\":2,\"temperature\":\"-2°C\",\"humidity\":\"80%\",\"pressure\":\"1420 hPa\",\"wind_velocity\":12,\"wind_direction\":\"110° E\"},"
    "{\"city\":\"Perugia\",\"type\":3,\"temperature\":\"12°C\",\"humidity\":\"40%\",\"pressure\":\"1069 hPa\",\"wind_velocity\":5,\"wind_direction\":\"130° NE\"},"
    "{\"city\":\"Perugia\",\"type\":4,\"temperature\":\"5°C\",\"humidity\":\"30%\",\"pressure\":\"1014 hPa\",\"wind_velocity\":120,\"wind_direction\":\"20° S\"},"
    "{\"city\":\"Perugia\",\"type\":5,\"temperature\":\"22°C\",\"humidity\":\"42%\",\"pressure\":\"1078 hPa\",\"wind_velocity\":12,\"wind_direction\":\"13° SW\"}"
    "]";

	printf("Listening on http://localhost:6969 ...\n\n");
	
	unsigned int i = 0;
	while(TRUE) {
		int client_sock = 0;
		socklen_t client_addr_size = 0;
		struct sockaddr_in client_addr = {0};
		if ((client_sock = accept(sock, (struct sockaddr*) &client_addr, &client_addr_size)) < 0) {
			perror("Failed to accept incoming connection");
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

		close(client_sock);

		printf("-------------------\n");
		printf("Connection closed.\n");
		printf("-------------------\n");
		
		i = (i + 1) % 2;
	}
		
	close(sock);

   	return 0;
}
