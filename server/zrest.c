#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

#define SAFE_FREE(ptr) do { if ((ptr) != NULL) free(ptr), (ptr) = NULL; } while(0)

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

static int str_n_tok(const char* str, const char* delim, const size_t size) {
     for (size_t i = 0, j = 0; i < size; ++i) {
         if (delim[j] == str[i]) {
            if ((j + 1) == str_len(delim)) return i - j;
            j++;
         } else if (j > 0) i -= j, j = 0;
    }
    return -1;
}

#define CAST_PTR(ptr, type) ((type*) (ptr))

static void* mem_cpy(void* dest, const void* src, size_t size) {
	if (dest == NULL || src == NULL) return NULL;
	unsigned char* dp = (unsigned char*) dest;
	unsigned char* sp = (unsigned char*) src;
	while (size--) *dp++ = *sp++;
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

#define IS_A_NUM(chr) ((chr >= '0') && (chr <= '9'))
#define CHR_TO_NUM(chr) chr - '0'
static long long int str_to_num(const char* str, const char* delim, const int size) {
	long long int ret = 0;
	ssize_t len = str_n_tok(str, delim, size);
	if (len == -1) len = size;

	for (ssize_t i = 0; i < len; ++i) {
		if (IS_A_NUM(str[i])) {
			ret *= 10;
			ret += CHR_TO_NUM(str[i]);
		} else return -1;
	}

	return ret;
}

static int str_n_cmp(const char* str1, const char* str2, size_t n) {
    // Null Checks
    if (str1 == NULL && str2 == NULL) return 0;
    if (str1 == NULL) return -1;
    else if (str2 == NULL) return 1;

    size_t i = 0;
    while ((str1[i] != '\0' || str2[i] != '\0') && i < n) {
        if (str1[i] != str2[i]) return str1[i] - str2[i];
        ++i;
    }

	return 0;
}

typedef enum RequestTypes { PREFLIGHT_CORS = 1, POST } RequestTypes;

static int check_request_type(const char* buffer) {
	if (str_n_cmp(buffer, "OPTIONS", 7) == 0) return PREFLIGHT_CORS;
	else if (str_n_cmp(buffer, "POST", 4) == 0) return POST;

	int pos = 0;
	if ((pos = str_tok(buffer, "HTTP/1.1")) < 0) {
		printf("Failed to find request type.\n");
		return -1;
	}
	
	printf("Unknown type \"%*.s\"\n", pos, buffer);
	
	return -1;
}

static int parse_json_request(const char* query, const int size, char** city, char** key, unsigned char* day) {
	printf("query: %*.s\n\n", size, query);

	int pos = 0;
	if ((pos = str_n_tok(query, "city", size)) < 0) {
		printf("No field \"city\" found in the JSON.\n");
		return -1;
	}

	int len = 0;
	if ((len = str_n_tok(query + pos + 7, "\"", size - pos - 7)) < 0) {
		printf("missing \" in JSON.\n");
		return -1;
	}
	
	*city = calloc(len + 1, sizeof(char));
	if (*city == NULL) {
		printf("Failed to allocate buffer.\n");
		return -1;
	}
	
	mem_cpy(*city, query + pos + 7, len);

	pos = 0;
	if ((pos = str_n_tok(query, "key", size)) < 0) {
		printf("No field \"key\" found in the JSON.\n");
		return -1;
	}

	len = 0;
	if ((len = str_n_tok(query + pos + 6, "\"", size - pos - 6)) < 0) {
		printf("missing \" in JSON.\n");
		return -1;
	}
	
	*key = calloc(len + 1, sizeof(char));
	if (*key == NULL) {
		printf("Failed to allocate buffer.\n");
		return -1;
	}
	
	mem_cpy(*key, query + pos + 6, len);

	if ((pos = str_n_tok(query, "day", size)) < 0) {
		printf("No field \"day\" found in the JSON.\n");
		SAFE_FREE(*city);
		return -1;
	}
	
	long long int val = str_to_num(query + pos + 6, "\"", size - pos - 6);
	if (val < 0) {
		printf("missing \" in JSON.\n");
		SAFE_FREE(*city);
		return -1;
	}
	
	*day = val;

	return 0;
}

static int check_content_size(const char* buffer, const int size) {
	int pos = str_n_tok(buffer, "Content-Length:", size);
	if (pos < 0) {
		printf("ERROR: no content-length specifier found.\n");
		return -1;
	}

	long long int content_len = str_to_num(buffer + pos + 16, "\r\n", size - pos - 16);
	if (content_len < 0) {
		printf("ERROR: No value found at the end of the content field.\n");
		return -1;
	}

	return content_len;
}

typedef struct Weather {
	unsigned char type;
	int temperature;
	unsigned int pressure;
	unsigned int humidity;
	unsigned int wind_velocity;
	unsigned int wind_direction;
} Weather;

static Weather gen_weather(void) {
	Weather w = {0};
	w.type = (rand() % 5) + 1;
	w.temperature = (rand() % 50) * ((rand() % 2) * 1);
	w.pressure = (rand() % 256) + 1000;
	w.humidity = rand() % 100;
	w.wind_velocity = rand() % 100;
	w.wind_direction = rand() % 180;
	return w;
}

static int generate_response(char** response, char* city, unsigned char day) {
	unsigned int content_capacity = 256;
	char* content = (char*) calloc(content_capacity, sizeof(char));
	if (content == NULL) {
		printf("Failed to allocate the content buffer.\n");
		return -1;
	}
	
	mem_set(content, '[', sizeof(char));

	unsigned int content_size = 1;

	for (unsigned char i = 0; i <= day; ++i) {
		content_capacity += 256;
		
		content = (char*) realloc(content, content_capacity);
		if (content == NULL) {
			printf("Failed to reallocate the content buffer.\n");
			return -1;
		}
		
		mem_set(content + content_size, 0, content_capacity - content_size);
	
		Weather weather = gen_weather();
		content_size += snprintf(content + content_size, content_capacity - content_size, "{\"city\":\"%s\",\"type\":%u,\"temperature\":\"%d°\",\"humidity\":\"%u%%\",\"pressure\":\"%u hPa\",\"wind_velocity\":%u,\"wind_direction\":\"%u° SE\"}", city, weather.type, weather.temperature, weather.humidity, weather.pressure, weather.wind_velocity, weather.wind_direction);
		
		if (i < day) mem_set(content + content_size, ',', sizeof(char)), content_size++;
	}
	
	mem_set(content + content_size, ']', sizeof(char));
	content_size++;

	*response = (char*) calloc(256 + content_size, sizeof(char));
	if (*response == NULL) {
		SAFE_FREE(content);
		printf("Failed to allocate response buffer.\n");
		return -1;
	}
	
	int response_size = snprintf(*response, 256 + content_size, "HTTP/1.1 200 OK\r\n"
	"Access-Control-Allow-Origin: *\r\n"
	"Content-Length: %d\r\n"
	"Connection: keep-alive\r\n"
	"Content-Type: application/json\r\n"
	"\r\n"
	"%s", content_size, content);
	
	printf("Response(%d):\n'%s'\n", response_size, *response);
	
	SAFE_FREE(content);

	return response_size;
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


static const char response_cors[] = "HTTP/1.1 204 No Content\r\n"
"Access-Control-Allow-Origin: *\r\n"
"Access-Control-Allow-Methods: POST, OPTIONS\r\n"
"Access-Control-Allow-Headers: content-type\r\n"
"Content-Length: 0\r\n"
"Connection: keep-alive\r\n"
"\r\n";

int main (void) {
	srand(time(0));
	
	socket_zt sock = 0;
	if (init_server(&sock) < 0) {
		printf("An error occurred while initializing the server.\n");
		return 1;
	}

	printf("Listening on http://localhost:6969 ...\n\n");
	
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
				NULL, err, 0, (LPSTR) &msg, 0, NULL);
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

#define BUFF_CHUNK 256

		int ret = 0;
		char* buffer = (char*) calloc(BUFF_CHUNK, sizeof(unsigned char));
		if (buffer == NULL) {
			printf("Failed to reallocate the buffer.\n");
			return 1;
		}
		
		char* content = NULL;
		int content_size = 0;

		int request_type = 0;
		unsigned long long int buf_cnt = 0;
		while ((ret = recv(client_sock, buffer + buf_cnt, BUFF_CHUNK, 0)) >= 0) {
			buf_cnt += ret;

			int pos = 0;
			if ((pos = str_n_tok(buffer, "\r\n\r\n", buf_cnt)) >= 0) {
				if ((request_type = check_request_type(buffer)) < 0) {
					printf("Failed to check the request type.\n");
					return 1;
				}

				printf("request_type: %d\n", request_type);

				if (request_type == PREFLIGHT_CORS) {
					SAFE_FREE(buffer);
					break;
				}
				
				content_size = 0;
				if ((content_size = check_content_size(buffer, buf_cnt)) < 0) {
					printf("\n\n-------\nBuffer(%llu): \"%s\"\n-------\n\n", buf_cnt, buffer);
					free(buffer);
					printf("An error occurred while checking the content length option field.\n");
					return 1;
				}
				
				if (content_size == 0) {
					free(buffer);
					break;
				}

				content = (char*) calloc(content_size, sizeof(char));
				if (content == NULL) {
					free(buffer);
					printf("An error occrured while allocating content buffer.\n");
					return 1;
				}
				
				int already_received_content = (buf_cnt - pos - 4);
				mem_cpy(content, buffer + pos + 4, already_received_content);
				SAFE_FREE(buffer);
				
				int remaining_content = content_size - already_received_content;
				if (remaining_content && (ret = recv(client_sock, content + already_received_content, remaining_content, 0)) != remaining_content) {
					SAFE_FREE(content);
					printf("ERROR: Expected %d bytes but received %d\n", remaining_content, ret);
					return 1;
				}
				
				break;
			}

			if ((buffer = realloc(buffer, buf_cnt + BUFF_CHUNK)) == NULL) {
				printf("Failed to reallocate the buffer.\n");
				return 1;
			}
		}

		if (ret < 0) {
			SAFE_FREE(content);
			perror("An error occurred while reading, because");
			return 1;
		}
		
		printf("Read %llu bytes.\n", buf_cnt);

		if (request_type == PREFLIGHT_CORS) {
			int res_len = 0;
			if ((res_len = send(client_sock, response_cors, sizeof(response_cors), 0)) != sizeof(response_cors)) {
				if (res_len >= 0) printf("\nSent %d bytes out of %lu.\n", res_len, (unsigned long) sizeof(response_cors));
				perror("Failed to send the response");
				return 1;
			}
			
			printf("Sent response back (%d)!\n", res_len);
			
			close_socket(client_sock);

			printf("-------------------\n");
			printf("Connection closed.\n");
			printf("-------------------\n");
			
			continue;
		}

		char* city = NULL;
		char* key = NULL;
		unsigned char day = 0;
		if (parse_json_request((const char*) content, content_size, &city, &key, &day) < 0) {
			printf("An error occurred while parsing the query.\n");
			SAFE_FREE(content);
			return 1;
		}
		
		SAFE_FREE(content);

		printf("Queried, city: '%s', day: %u with key: '%s'\n", city, day, key);
		SAFE_FREE(key);
		
		char* response = NULL;
		int response_size = generate_response(&response, city, day);
	
		SAFE_FREE(city);
		
		if (response_size < 0) {
			printf("Failed to generate the response.\n");
			return 1;
		} 

		int res_len = 0;
		if ((res_len = send(client_sock, response, response_size, 0)) != response_size) {
			SAFE_FREE(response);
			if (res_len >= 0) printf("\nSent %d bytes out of %d.\n", res_len, response_size);
			perror("Failed to send the data_response");
			return 1;
		}
		
		SAFE_FREE(response);

		printf("Sent response back (%d)!\n", res_len);

		close_socket(client_sock);

		printf("-------------------\n");
		printf("Connection closed.\n");
		printf("-------------------\n");
	}
		
	close_socket(sock);
	SOCK_CLEANUP();

   	return 0;
}

