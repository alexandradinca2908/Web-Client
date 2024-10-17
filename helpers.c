#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "buffer.h"
#include "parson.h"

#define HEADER_TERMINATOR "\r\n\r\n"
#define HEADER_TERMINATOR_SIZE (sizeof(HEADER_TERMINATOR) - 1)
#define CONTENT_LENGTH "Content-Length: "
#define CONTENT_LENGTH_SIZE (sizeof(CONTENT_LENGTH) - 1)

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void compute_message(char *message, const char *line)
{
    strcat(message, line);
    strcat(message, "\r\n");
}

int open_connection(char *host_ip, int portno, int ip_type, int socket_type, int flag)
{
    struct sockaddr_in serv_addr;
    int sockfd = socket(ip_type, socket_type, flag);
    if (sockfd < 0)
        error("ERROR opening socket");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = ip_type;
    serv_addr.sin_port = htons(portno);
    inet_aton(host_ip, &serv_addr.sin_addr);

    /* connect the socket */
    if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    return sockfd;
}

void close_connection(int sockfd)
{
    close(sockfd);
}

void send_to_server(int sockfd, char *message)
{
    int bytes, sent = 0;
    int total = strlen(message);

    do
    {
        bytes = write(sockfd, message + sent, total - sent);
        if (bytes < 0) {
            error("ERROR writing message to socket");
        }

        if (bytes == 0) {
            break;
        }

        sent += bytes;
    } while (sent < total);
}

char *receive_from_server(int sockfd)
{
    char response[BUFLEN];
    buffer buffer = buffer_init();
    int header_end = 0;
    int content_length = 0;

    do {
        int bytes = read(sockfd, response, BUFLEN);

        if (bytes < 0){
            error("ERROR reading response from socket");
        }

        if (bytes == 0) {
            break;
        }

        buffer_add(&buffer, response, (size_t) bytes);
        
        header_end = buffer_find(&buffer, HEADER_TERMINATOR, HEADER_TERMINATOR_SIZE);

        if (header_end >= 0) {
            header_end += HEADER_TERMINATOR_SIZE;
            
            int content_length_start = buffer_find_insensitive(&buffer, CONTENT_LENGTH, CONTENT_LENGTH_SIZE);
            
            if (content_length_start < 0) {
                continue;           
            }

            content_length_start += CONTENT_LENGTH_SIZE;
            content_length = strtol(buffer.data + content_length_start, NULL, 10);
            break;
        }
    } while (1);
    size_t total = content_length + (size_t) header_end;
    
    while (buffer.size < total) {
        int bytes = read(sockfd, response, BUFLEN);

        if (bytes < 0) {
            error("ERROR reading response from socket");
        }

        if (bytes == 0) {
            break;
        }

        buffer_add(&buffer, response, (size_t) bytes);
    }
    buffer_add(&buffer, "", 1);
    return buffer.data;
}

char *basic_extract_json_response(char *str)
{
    return strstr(str, "{\"");
}

int valid_buff(char *buff) {
    if (strchr(buff, ' ')) {
        return 0;
    }

    return 1;
}

char *get_user_data() {
    //  Username
    //  Init
    char username[BUFLEN];
    memset(username, 0, BUFLEN);
    //  Read
    printf("username=");
    fgets(username, BUFLEN, stdin);
    //  Remove \n
    username[strlen(username) - 1] = '\0';

    //  Password
    //  Init
    char password[BUFLEN];
    memset(password, 0, BUFLEN);
    //  Read
    printf("password=");
    fgets(password, BUFLEN, stdin);
    //  Remove \n
    password[strlen(password) - 1] = '\0';

    //  Check
    if (!valid_buff(username)) {
        printf("ERROR: Username can't contain spaces. Redo operation.\n");
        return NULL;
    }
    //  Check
    if (!valid_buff(password)) {
        printf("ERROR: Password can't contain spaces. Redo operation.\n");
        return NULL;
    }

    //  Init JSON object
    JSON_Value *json_value = json_value_init_object();
    JSON_Object *json_object = json_value_get_object(json_value);

    //  Set fields
    json_object_set_string(json_object, "username", username);
    json_object_set_string(json_object, "password", password);

    //  Return serialized JSON
    return json_serialize_to_string(json_value);
}

int valid_book(char *field, char *type) {
    if (strcmp(type, "page_count") == 0 || strcmp(type, "id") == 0) {
        //  Check if the field starts with -
        if (field[0] == '-') {
            if (strcmp(type, "page_count")) {
                printf("ERROR: Page count can not have a negative value!\n");
            } else {
                printf("ERROR: ID can not have a negative value!\n");
            }
            
            return 0;
        }

        //  Check if field starts with 0
        if (field[0] == '0') {
            if (strcmp(type, "page_count")) {
                printf("ERROR: Page count can not begin with 0 or be 0!\n");
            } else {
                printf("ERROR: ID can not begin with 0 or be 0!\n");
            }
            return 0;
        }

        //  Check if the field is a natural number
        for (int i = 0; i < strlen(field); i++) {
            if (field[i] < '0' || field[i] > '9') {
                if (strcmp(type, "page_count")) {
                    printf("ERROR: Page count is not a natural number.\n");
                } else {
                    printf("ERROR: ID is not a natural number.\n");
                }

                return 0;
            }
        }
    } else {
        //  Check for empty field
        if (strlen(field) == 0) {
            if (strcmp(type, "title") == 0) {
                printf("ERROR: Title can not be empty.\n");
            } else if (strcmp(type, "author") == 0) {
                printf("ERROR: Author name can not be empty.\n");
            } else if (strcmp(type, "genre") == 0) {
                printf("ERROR: Genre field can not be empty.\n");
            } else {
                printf("ERROR: Publisher name can not be empty.\n");
            }

            return 0;
        }

        //  Check for field that only contains non-letter chars
        int ok = 0;
        for (int i = 0; i < strlen(field); i++) {
            if (field[i] >= 'A' && field[i] <= 'z') {
                ok = 1;
                break;
            }
        }
        if (ok == 0) {
            if (strcmp(type, "title") == 0) {
                printf("ERROR: Title must contain letters.\n");
            } else if (strcmp(type, "author") == 0) {
                printf("ERROR: Author name must contain letters.\n");
            } else if (strcmp(type, "genre") == 0) {
                printf("ERROR: Genre field must contain letters.\n");
            } else {
                printf("ERROR: Publisher name must contain letters.\n");
            }

            return 0;
        }
    }

    return 1;
}

char *get_book_data() {
    //  Title
    //  Init
    char title[BUFLEN];
    memset(title, 0, BUFLEN);
    //  Read
    printf("title=");
    fgets(title, BUFLEN, stdin);
    //  Remove \n
    title[strlen(title) - 1] = '\0';
    
    //  Author
    //  Init
    char author[BUFLEN];
    memset(author, 0, BUFLEN);
    //  Read
    printf("author=");
    fgets(author, BUFLEN, stdin);
    //  Remove \n
    author[strlen(author) - 1] = '\0';

    //  Genre
    //  Init
    char genre[BUFLEN];
    memset(genre, 0, BUFLEN);
    //  Read
    printf("genre=");
    fgets(genre, BUFLEN, stdin);
    //  Remove \n
    genre[strlen(genre) - 1] = '\0';

    //  Page count
    //  Init
    char page_count[BUFLEN];
    int pages;
    memset(page_count, 0, BUFLEN);
    //  Read
    printf("page_count=");
    fgets(page_count, BUFLEN, stdin);
    //  Remove \n
    page_count[strlen(page_count) - 1] = '\0';

    //  Publisher
    //  Init
    char publisher[BUFLEN];
    memset(publisher, 0, BUFLEN);
    //  Read
    printf("publisher=");
    fgets(publisher, BUFLEN, stdin);
    //  Remove \n
    publisher[strlen(publisher) - 1] = '\0';

    //  Check title
    if (!valid_book(title, "title")) {
        return NULL;
    }
    //  Check author
    if (!valid_book(author, "author")) {
        return NULL;
    }
    //  Check genre
    if (!valid_book(genre, "genre")) {
        return NULL;
    }
    //  Check page count
    if (!valid_book(page_count, "page_count")) {
        return NULL;
    } else {
        pages = atoi(page_count);
    }
    //  Check publisher
    if (!valid_book(publisher, "publisher")) {
        return NULL;
    }

    //  Init JSON object
    JSON_Value *json_value = json_value_init_object();
    JSON_Object *json_object = json_value_get_object(json_value);

    //  Set fields
    json_object_set_string(json_object, "title", title);
    json_object_set_string(json_object, "author", author);
    json_object_set_string(json_object, "genre", genre);
    json_object_set_string(json_object, "publisher", publisher);
    json_object_set_number(json_object, "page_count", pages);

    //  Return serialized JSON
    return json_serialize_to_string_pretty(json_value);
}
