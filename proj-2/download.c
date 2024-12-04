#include "download.h"


int main(int argc, char *argv[]) {

    if(argc != 2) {
        perror("Invalid call to download.");
        return -1;
    }
    
    URL url;
    memset(&url, 0, sizeof(URL));

    if(parse_URL(&url, argv[1]) == -1) {
        perror("parse_URL()");
        return -1;
    }
    printf("User: %s\n", url.user);
    printf("Password: %s\n", url.password);
    printf("Host: %s\n", url.host);
    printf("URL Path: %s\n", url.url_path);
    printf("Filename: %s\n", url.filename);
    
    if(get_ip(url.host, &url) == -1) {
        perror("get_ip()");
        return -1;
    }
    int sockfd;

    if((sockfd = create_socket(url.ip, SERVER_PORT)) == -1) {
        perror("create_socket()");
        return -1;
    }
    response newMessage;

    reset_response(&newMessage);

    int ret = receiveResponse(sockfd,&newMessage);
    
    if(ret == -1) {
        close(sockfd);
        perror("receiveResponse()");
        return -1;
    }
    
    if(newMessage.code != WELCOME_CODE) {
        close(sockfd);
        perror("Server did not respond with 220.");
        return -1;
    }    
    
    showResponse(&newMessage);

    /*send username*/

    reset_response(&newMessage);
    newMessage.code = USER_CODE;
    strncpy(newMessage.message, "user ", sizeof(newMessage.message) - 1);
    strncat(newMessage.message, url.user, sizeof(newMessage.message) - strlen(newMessage.message) - 1);
    
    if(!writeMessage(sockfd,&newMessage)){
        close(sockfd);
        perror("Failed to send username");
        return -1;
    }

    /*Send password*/

    reset_response(&newMessage);
    newMessage.code = PASSWORD_CODE;
    strncpy(newMessage.message, "pass ", sizeof(newMessage.message) - 1);
    strncat(newMessage.message, url.password, sizeof(newMessage.message) - strlen(newMessage.message) - 1);

    if(!writeMessage(sockfd,&newMessage)){
        close(sockfd);
        perror("Failed to send password");
        return -1;
    }
    /*Send binary format*/

    reset_response(&newMessage);
    newMessage.code = BINARY_CODE;
    strncpy(newMessage.message, "type I", sizeof(newMessage.message) - 1);

    if(!writeMessage(sockfd,&newMessage)){
        close(sockfd);
        perror("Failed to pass binary format");
        return -1;
    }


    /*Send passive mode*/

    reset_response(&newMessage);
    newMessage.code = PASSIVE_CODE;
    strncpy(newMessage.message, "pasv", sizeof(newMessage.message));

    if(!writeMessage(sockfd,&newMessage)){
        close(sockfd);
        perror("Failed to activate passivemode");
        return -1;
    }
    int port2;

    if((port2 = calculate_new_port(newMessage.message,url)) == - 1){
        close(sockfd);
        perror("Failed to calculate port");
        return -1;
    }
    int sockfd2;
    if ((sockfd2 = create_socket(url.ip,port2)) == -1){
        perror("Failed to create socket");
        return -1;
    }

    /*Send retreive*/
    reset_response(&newMessage);
    newMessage.code = RETREIVE_CODE;
    strncpy(newMessage.message, "retr ", sizeof(newMessage.message) - 1);
    strncat(newMessage.message, url.url_path, sizeof(newMessage.message) - strlen(newMessage.message) - 1);

    if(!writeMessage(sockfd,&newMessage)){
        close(sockfd);
        perror("Failed to retreive");
        return -1;
    }

    printf("Filename: %s\n",url.filename);

    long long fileSize = getFileSize(newMessage.message);
    if(fileSize == -1){
        perror("Failed to get the file size");
        return -1;
    }

    if(readfile(sockfd2,url.filename,fileSize) == -1){
        close(sockfd);
        return -1;
    }

    reset_response(&newMessage);
    
    ret = receiveResponse(sockfd,&newMessage);

    if(ret == -1) {
        close(sockfd);
        perror("receiveResponse()");
        return -1;
    }
    
    if(newMessage.code != TRANSFER_CODE) {
        close(sockfd);
        perror("Transfer went wrong.");
        return -1;
    }    
    
    printf("Transfer completed successfully\n");
    close(sockfd);
    close(sockfd2);

    return 0;
}

void reset_response(response *res) {
    if (res == NULL) return; // Safety check
    res->code = 0; // Reset the response code
    memset(res->message, 0, sizeof(res->message)); // Clear the message buffer
}

int create_socket(char *ip,int port) {  

    int sockfd;
    struct sockaddr_in server_addr;

    /*server address handling*/
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);    /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(port);        /*server TCP port must be network byte ordered */

    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return -1;
    }
    /*connect to the server*/
    if (connect(sockfd,
                (struct sockaddr *) &server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect()");
        return -1;
    }



    return sockfd;
}


int get_ip(char* hostname, URL *url) {
    struct hostent *h;

    if ((h = gethostbyname(hostname)) == NULL) {
        perror("gethostbyname()");
        return -1;
    }

    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n", inet_ntoa(*((struct in_addr *) h->h_addr)));
    strncpy(url->ip, inet_ntoa(*((struct in_addr *) h->h_addr)), sizeof(url->ip) - 1);
    return 0;
}


int parse_URL(URL *url, char *url_str) {
    // Regular expression to match the URL format
    const char *regex_pattern = "^ftp://(([^:@/]+)(:([^@/]+))?@)?([^/]+)/(.+)$";

    regex_t regex;
    regmatch_t matches[7];  // There are 7 capture groups in the regex

    // Compile the regular expression
    if (regcomp(&regex, regex_pattern, REG_EXTENDED) != 0) {
        fprintf(stderr, "Failed to compile regex.\n");
        return -1;
    }

    // Execute the regular expression on the URL string
    if (regexec(&regex, url_str, 7, matches, 0) != 0) {
        fprintf(stderr, "Invalid URL format.\n");
        regfree(&regex);
        return -1;
    }

    // Extract user and password (if present)
    if (matches[1].rm_so != -1) {
        // If user and password are present
        if (matches[2].rm_so != -1) {
            strncpy(url->user, url_str + matches[2].rm_so, matches[2].rm_eo - matches[2].rm_so);
            url->user[matches[2].rm_eo - matches[2].rm_so] = '\0';
        } else {
            strncpy(url->user, "anonymous", sizeof(url->user) - 1);
        }

        if (matches[4].rm_so != -1) {
            strncpy(url->password, url_str + matches[4].rm_so, matches[4].rm_eo - matches[4].rm_so);
            url->password[matches[4].rm_eo - matches[4].rm_so] = '\0';
        } else {
            strncpy(url->password, "password", sizeof(url->password) - 1); // No password
        }
    } else {
        strncpy(url->user, "anonymous", sizeof(url->user) - 1);   // No user
        strncpy(url->password, "password", sizeof(url->password) - 1); // No password
    }

    // Extract host
    if (matches[5].rm_so != -1) {
        strncpy(url->host, url_str + matches[5].rm_so, matches[5].rm_eo - matches[5].rm_so);
        url->host[matches[5].rm_eo - matches[5].rm_so] = '\0';
    } else {
        url->host[0] = '\0'; // Handle missing host
    }

    // Extract URL path
    if (matches[6].rm_so != -1) {
        strncpy(url->url_path, url_str + matches[6].rm_so, matches[6].rm_eo - matches[6].rm_so);
        url->url_path[matches[6].rm_eo - matches[6].rm_so] = '\0';
    } else {
        url->url_path[0] = '\0'; // Handle missing URL path
    }

    // Extract filename from the path (if exists)
        char *last_slash = strrchr(url->url_path, '/');
    if (last_slash && *(last_slash + 1) != '\0') {
        // There is a filename after the last slash
        strncpy(url->filename, last_slash + 1, MAX_LEN - 1);
        url->filename[MAX_LEN - 1] = '\0';
    } else {
        // No filename, use the entire path as the filename
        strncpy(url->filename, url->url_path, MAX_LEN - 1);
        url->filename[MAX_LEN - 1] = '\0';
    }

    // Set IP to an empty string (no parsing logic for IP)
    url->ip[0] = '\0';

    // Free the regex memory
    regfree(&regex);

    return 0;
}

int close_socket(int sockfd) {
    if(close(sockfd) < 0) {
        perror("close()");
        return -1;
    }
    return 0;
}


int receiveResponse(int socketfd, response *res) {
    int end = 1;
    char codeBuf[4];

    while (end) {
        // Read the 4-character code
        if (readCode(socketfd, codeBuf) == -1) return -1; // Error reading code

        // Check if the last character of the code is '-'
        end = (codeBuf[3] == '-');

        // Read until a newline character is encountered
        if (readUntilNewline(socketfd, res->message) == -1) return -1; // Error reading until newline
    }

    printf("Code: %s\n", codeBuf);
    res->code = atoi(codeBuf); // Convert the code to an integer
    return 0; // Return the code as an integer
}

int readCode(int socketfd, char *code) {
    for (int j = 0; j < CODE_SIZE; j++) {
        int res = read(socketfd, &code[j], 1);
        if (res < 0) return -1; // Handle read error
    }
    return 0; // Successfully read the code
}

// Function to read data until a newline character is encountered
int readUntilNewline(int socketfd, char *buf) {
    int i = 0;
    int res;
    do {
        res = read(socketfd, &buf[i++], 1);
        if (res < 0) return -1; // Handle read error
    } while (buf[i - 1] != '\n');
    return 0; // Successfully read until newline
}


// Message formating is weird
void showResponse(response *res) {
    printf("Message: %s\n", res->message);
}


int writeMessage(int sockfd, response *message){
    if (sockfd < 0) {
        perror("Invalid socket descriptor");
        return -1;
    }

    if (strlen(message->message) == 0) {
        fprintf(stderr, "Invalid message to write\n");
        return -1;
    }

    printf("Sending message: %s\n", message->message);

    size_t bytes_sent = 0;
    size_t to_send = strlen(message->message);
    
    // Write the message in chunks if necessary
    while (bytes_sent < to_send) {
        ssize_t result = write(sockfd, message->message + bytes_sent, to_send - bytes_sent);
        if (result < 0) {
            perror("Error writing to socket");
            return -1;
        }
        bytes_sent += result;
    }

    if (write(sockfd, "\r\n", 2) < 0) {
        perror("Error writing CRLF to socket");
        return -1;
    }

    // Receive the server's response
    response res;
    memset(&res, 0, sizeof(res));

    if (receiveResponse(sockfd, &res) == -1) {
        perror("receiveResponse failed");
        return -1;
    }

    printf("Response code: %d \nMessage: %s\n", res.code, res.message);

    strncpy(message->message, res.message, sizeof(message->message) - 1);
    message->message[sizeof(message->message) - 1] = '\0';

    return res.code == message->code;
}

int calculate_new_port(char *passiveMsg, URL url){
    int ip1, ip2, ip3, ip4, port1, port2;

    int parsed = sscanf(passiveMsg, "Entering Passive Mode (%d,%d,%d,%d,%d,%d).", &ip1, &ip2, &ip3, &ip4, &port1, &port2);
    
    if (parsed != 6) {
        perror("Failed to parse passive message");
        return -1;
    }

    // Compare the extracted IP with the provided IP (url.ip should be in the same format as the parsed IP)
    // Assuming url.ip is in the format "ip1.ip2.ip3.ip4"
    int url_ip1, url_ip2, url_ip3, url_ip4;
    if (sscanf(url.ip, "%d.%d.%d.%d", &url_ip1, &url_ip2, &url_ip3, &url_ip4) != 4) {
        perror("Failed to parse URL IP");
        return -1;
    }

    // If the IPs don't match, return an error
    if (ip1 != url_ip1 || ip2 != url_ip2 || ip3 != url_ip3 || ip4 != url_ip4) {
        perror("IP addresses do not match");
        return -1;
    }

    int port = port1*256 + port2;
    
    return port;
}


int readfile(int sockfd, char *filename, long long file_size) {
    int file_fd;
    off_t bytes_received = 0;
    ssize_t res;

    // Open the file in write-only mode. Create it if it doesn't exist.
    // Set permissions to rw-r--r-- (user can read/write, group and others can read).
    if ((file_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
        perror("Open file");
        return -1;
    }

    // Buffer for receiving data from the socket
    char buf[1024];  // Larger buffer to read multiple bytes at once

    printf("Receiving file...\n");

    // Loop to read from the socket and write to the file
    while (1) {
        // Read data from the socket
        res = read(sockfd, buf, sizeof(buf));
        
        if (res == 0) {
            // End of transmission
            break;
        }
        if (res < 0) {
            // Error reading from the socket
            perror("Failed to read from socket");
            close(file_fd);
            return -1;
        }

        // Write the received data into the file
        if (write(file_fd, buf, res) < 0) {
            // Error writing to the file
            perror("Failed to write to file");
            close(file_fd);
            return -1;
        }

        // Update bytes received
        bytes_received += res;
        // Print loading bar based on the total file size
        float progress = (float)bytes_received / file_size * 100; // Progress based on actual file size
        int bar_width = 50; // Width of the progress bar
        int pos = bar_width * progress / 100;
        // Ensure that progress does not exceed 100%
        if (progress > 100.0f) {
            progress = 100.0f;
        }
        printf("[");

        for (int i = 0; i < bar_width; ++i) {
            if (i < pos) {
                printf("=");
            } else if (i == pos) {
                printf(">");
            } else {
                printf(" ");
            }
        }
        printf("] %.2f%%\r", progress);
        fflush(stdout);  // Flush to update the progress bar immediately
    }

    printf("\nFile received and written to %s\n", filename);

    // Close the file
    close(file_fd);
    return 0;
}

long long getFileSize(char * message) {
    long long bytes = -1;  

    
    char *start = strrchr(message, '(');  
    if (start != NULL) {
        char *end = strchr(start, ')');  
        if (end != NULL) {
            
            *end = '\0';

            
            if (sscanf(start + 1, "%lld", &bytes) == 1) {  
                return bytes;  
            } else {
                printf("Falha ao extrair o número de bytes\n");
            }
        }
    }
    return -1;  // Return -1 
}