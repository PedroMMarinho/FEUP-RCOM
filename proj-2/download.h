#ifndef  download_H
#define  download_H


#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <regex.h>
#include <fcntl.h>

#define MAX_LEN 500

#define SERVER_PORT 21
#define CODE_SIZE 4
#define WELCOME_CODE 220
#define USER_CODE 331
#define PASSWORD_CODE 230
#define PASSIVE_CODE 227
#define MIN_RETREIVE_CODE 100
#define MAX_RETREIVE_CODE 199
#define RETREIVE_CODE_WITH_SIZE 150
#define MIN_TRANSFER_CODE 200
#define BINARY_CODE 200
#define GOODBYE_CODE 221

typedef struct 
{
    char ip[16];
    char user[MAX_LEN];
    char password[MAX_LEN];
    char host[MAX_LEN];
    char url_path[MAX_LEN];
    char filename[MAX_LEN]; 
} URL;

typedef struct 
{
    char message[1024];
    int code;

} response;


int create_socket(char *ip,int port);
int get_ip(char* hostname, URL *url);
int parse_URL(URL *url,char *url_str);
int close_socket(int sockfd);
int receiveResponse(int socketfd, response *res);
int readUntilNewline(int socketfd, char *buf);
int readCode(int socketfd, char *code);
void showResponse(response *res);
void reset_response(response *newMessage);
int writeMessage(int sockfd, response *message);
int calculate_new_port(char *passiveMsg, URL url);
int readfile(int sockfd, char *filename, long long file_size);
long long getFileSize(char * message);

#endif