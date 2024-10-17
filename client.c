#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "parson.h"
#include "helpers.h"
#include "requests.h"

//  SERVER INFO
#define SERVER_IP "34.246.184.49"
#define SERVER_PORT 8080

//  SERVER PATHS
#define REGISTER_PATH "/api/v1/tema/auth/register"
#define LOGIN_PATH "/api/v1/tema/auth/login"
#define ENTER_LIBRARY_PATH "/api/v1/tema/library/access"
#define GET_BOOKS_PATH "/api/v1/tema/library/books"
#define ADD_BOOK_PATH "/api/v1/tema/library/books"
#define DELETE_BOOK_PATH "/api/v1/tema/library/books"
#define LOGOUT_PATH "/api/v1/tema/auth/logout"

#define PAYLOAD_TYPE "application/json"

int main(int argc, char *argv[]) {
    int logged_in = 0;
    int library_access = 0;
    char session_cookie[BUFLEN];
    char jwt_token[BUFLEN];

    //  Cleanup memory
    memset(session_cookie, 0, BUFLEN);
    memset(jwt_token, 0, BUFLEN);

    //  Init socket
    int sockfd;

    //  Syscalls checker
    char *check;

    //  Begin execution
    while (1) {
        //  Get client input
        char buff[BUFLEN];
        check = fgets(buff, BUFLEN, stdin);

        if (check == NULL) {
            error("Couldn't read client input.\n");
        }

        //  EXIT
        if (strcmp(buff, "exit\n") == 0) {
            close_connection(sockfd);
            break;

        //  REGISTER
        } else if (strcmp(buff, "register\n") == 0) {
            sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0){
                error("Socket unavailable. Connection refused.\n");
            }

            //  Check login status
            if (logged_in == 1) {
                printf("Can't register. You are already logged in with another user!\n");
                continue;
            }

            //  Process user data
            char *serialized_string = get_user_data();
            if (serialized_string == NULL) {
                continue;
            }

            //  Compute and send to server
            char host_ip[] = SERVER_IP;
            char url[] = REGISTER_PATH;
            char payload_type[] = PAYLOAD_TYPE;
            int fields = 1;

            char *message = compute_post_request(host_ip, url, payload_type, &serialized_string,
                                                fields, NULL, NULL);
            send_to_server(sockfd, message);

            //  Receive feedback from server
            char *feedback = receive_from_server(sockfd);
            
            //  Bad request
            if (strstr(feedback, "error")) {
                //  Username is taken
                if (strstr(feedback, "is taken")) {
                    printf("ERROR: Couldn't register. The username is already taken!\n");
                } else {
                    printf("ERROR: Bad request.\n");
                }
            //  Registration successful    
            } else if (strstr(feedback, "ok")) {
                printf("SUCCESS: Registration complete.\n");
            } else {
                printf("No response from server.\n");
            }

            //  Close socket
            close_connection(sockfd);

        //  LOGIN    
        } else if (strcmp(buff, "login\n") == 0) {
            sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0){
                error("Socket unavailable. Connection refused.\n");
            }

            //  Check login status
            if (logged_in == 1) {
                printf("You are already logged in!\n");
                continue;
            }

            //  Process user data
            char *serialized_string = get_user_data();
            if (serialized_string == NULL) {
                continue;
            }

            //  Compute and send to server
            char host_ip[] = SERVER_IP;
            char url[] = LOGIN_PATH;
            char payload_type[] = PAYLOAD_TYPE;
            int fields = 1;

            char *message = compute_post_request(host_ip, url, payload_type, &serialized_string,
                                                fields, NULL, NULL);
            send_to_server(sockfd, message);

            //  Receive feedback from server
            char *feedback = receive_from_server(sockfd);

            //  Bad request
            if (strstr(feedback, "error")) {
                //  Account doesn't exist
                if (strstr(feedback, "No account with this username!")) {
                    printf("ERROR: Couldn't login. Account doesn't exist.\n");
                //  Wrong credentials
                } else if (strstr(feedback, "Credentials are not good!")) {
                    printf("ERROR: Couldn't login. Password is incorrect!\n");
                } else {
                    printf("ERROR: Bad request.\n");
                }
            //  Login successful    
            } else if (strstr(feedback, "ok")) {
                //  Store cookie
                char *cookie = strstr(feedback, "Set-Cookie: ");
                cookie += strlen("Set-Cookie: ");

                for (int i = 0; i < strlen(cookie); i++) {
                    if (cookie[i] == ';') {
                        cookie[i] = '\0';
                    }

                    if (cookie[i] == '\0') {
                        break;
                    }
                }
                strcpy(session_cookie, cookie);

                //  Check if cookie is valid
                if (session_cookie != NULL) {
                    printf("SUCCESS: You are now logged in.\n");
                    logged_in = 1;
                } else {
                    printf("Please log in again.\n");
                }
            } else {
                printf("No response from server.\n");
            }

            //  Close socket
            close_connection(sockfd);

        //  ENTER LIBRARY
        } else if (strcmp(buff, "enter_library\n") == 0) {
            sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0){
                error("Socket unavailable. Connection refused.\n");
            }

            //  Must be logged in
            if (logged_in == 0) {
                printf("You are not logged in!\n");
                continue;
            }

            //  Must not have access already
            if (library_access == 1) {
                printf("You already have access to the library!\n");
                continue;
            }

            //  Compute and send to server
            char host_ip[] = SERVER_IP;
            char url[] = ENTER_LIBRARY_PATH;

            char *message = compute_get_request(host_ip, url, NULL, session_cookie, NULL);
            send_to_server(sockfd, message);

            //  Receive feedback from server
            char *feedback = receive_from_server(sockfd);

            //  Bad request
            if (strstr(feedback, "error")) {
                //  Invalid cookie
                if (strstr(feedback, "You are not logged in!")) {
                    printf("ERROR: Couldn't enter library. Session failed.\n");
                } else {
                    printf("ERROR: Bad request.\n");
                }
            //  Successfuly entered the library
            } else if (strstr(feedback, "{\"token\":\"")) {
                //  Extract JWT Token
                char *jwt = strstr(feedback, "{\"token\":\"");
                jwt += strlen("{\"token\":\"");

                char *final_char = strchr(jwt, '\"');
                if (final_char != NULL) {
                    *final_char = '\0';
                }

                strcpy(jwt_token, jwt);

                if (jwt_token != NULL) {
                    printf("SUCCESS: Library access granted.\n");
                    library_access = 1;
                } else {
                    printf("Access denied. Please try again.\n");
                }
            } else {
                printf("No response from server.\n");
            }
        
            //  Close socket
            close_connection(sockfd);

        //  GET BOOKS
        } else if (strcmp(buff, "get_books\n") == 0) {
            sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0){
                error("Socket unavailable. Connection refused.\n");
            }

            //  Must have library access
            if (library_access == 0) {
                printf("You do not have access to the library!\n");
                continue;
            }

            //  Compute and send to server
            char host_ip[] = SERVER_IP;
            char url[] = GET_BOOKS_PATH;

            char *message = compute_get_request(host_ip, url, NULL, session_cookie, jwt_token);
            send_to_server(sockfd, message);

            //  Receive feedback from server
            char *feedback = receive_from_server(sockfd);

            //  Bad request
            if (strstr(feedback, "error")) {
                //  Invalid JWT token
                if (strstr(feedback, "Error when decoding token")) {
                    printf("ERROR: Couldn't get books. Error when decoding token.\n");
                } else {
                    printf("ERROR: Bad request.\n");
                }
            //  Successfuly retrieved books
            } else {
                //  Get the serialized JSON
                char *books = strstr(feedback, "\n[{\"id\":");

                //  Check if there are any books
                if (books == NULL) {
                    printf("No books in library!\n");
                } else {
                    //  Transform string to JSON array
                    JSON_Value *json_value = json_parse_string(books);
                    JSON_Array *json_books = json_value_get_array(json_value);

                    //  Parse each object and print the details
                    for (int i = 0; i < json_array_get_count(json_books); i++) {
                        JSON_Object *book = json_array_get_object(json_books, i);

                        printf("\nid= %d\n", (int)json_object_get_number(book, "id"));
                        printf("title= %s\n", json_object_get_string(book, "title"));
                    }
                }
            }

            //  Close socket
            close_connection(sockfd);
        
        } else if (strcmp(buff, "get_book\n") == 0) {
            sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0){
                error("Socket unavailable. Connection refused.\n");
            }

            //  Must have library access
            if (library_access == 0) {
                printf("You do not have access to the library!\n");
                continue;
            }

            //  Get book ID
            char id[BUFLEN];
            memset(id, 0, BUFLEN);
            //  Read
            printf("id=");
            fgets(id, BUFLEN, stdin);
            //  Remove \n
            id[strlen(id) - 1] = '\0';

            if (!valid_book(id, "id")) {
                continue;
            }

            //  Compute and send to server
            char host_ip[] = SERVER_IP;
            char url[] = GET_BOOKS_PATH;
            strcat(url, "/");
            strcat(url, id);

            char *message = compute_get_request(host_ip, url, NULL, session_cookie, jwt_token);
            send_to_server(sockfd, message);

            //  Receive feedback from server
            char *feedback = receive_from_server(sockfd);

            //  Bad request
            if (strstr(feedback, "error")) {
                //  Wrong ID
                if (strstr(feedback, "No book was found!")) {
                    printf("ERROR: Couldn't get book. ID doesn't match.\n");
                } else {
                    printf("ERROR: Bad request.\n");
                }
            //  Successfuly retrieved book
            } else if (strstr(feedback, "{\"id\":")) {
                //  Get the serialized JSON
                char *book = strstr(feedback, "{\"id\":");

                //  Check for errors
                if (book == NULL) {
                    printf("Something went wrong. Couldn't get book, please try again.\n");

                //  Extract and print fields
                } else {
                    JSON_Value *json_value = json_parse_string(book);
                    JSON_Object *json_book = json_value_get_object(json_value);

                    printf("\nid: %d\n", (int)json_object_get_number(json_book, "id"));
                    printf("title: %s\n", json_object_get_string(json_book, "title"));
                    printf("author: %s\n", json_object_get_string(json_book, "author"));
                    printf("publisher: %s\n", json_object_get_string(json_book, "publisher"));
                    printf("genre: %s\n", json_object_get_string(json_book, "genre"));
                    printf("page_count: %d\n", (int)json_object_get_number(json_book, "page_count"));
                }
            } else {
                printf("No response from server.\n");
            }

            //  Close socket
            close_connection(sockfd);

        //  ADD BOOK
        } else if (strcmp(buff, "add_book\n") == 0) {
            sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0){
                error("Socket unavailable. Connection refused.\n");
            }
            
            //  Must have library access
            if (library_access == 0) {
                printf("You do not have access to the library!\n");
                continue;
            }

            //  Process book data
            char *serialized_string = get_book_data();
            if (serialized_string == NULL) {
                continue;
            }

            //  Compute and send to server
            char host_ip[] = SERVER_IP;
            char url[] = ADD_BOOK_PATH;
            char payload_type[] = PAYLOAD_TYPE;
            int fields = 1;
            
            char *message = compute_post_request(host_ip, url, payload_type, &serialized_string,
                                                fields, session_cookie, jwt_token);
            send_to_server(sockfd, message);
            
            //  Receive feedback from server
            char *feedback = receive_from_server(sockfd);

            //  Bad request
            if (strstr(feedback, "error")) {
                //  Invalid JWT token
                if (strstr(feedback, "Error when decoding token")) {
                    printf("ERROR: Couldn't add book. Error when decoding token.\n");
                } else {
                    printf("ERROR: Bad request.\n");
                }
            //  Successfuly retrieved books
            } else if (strstr(feedback, "ok")) {
                printf("SUCCESS: Your book has been added to the library.\n");
            } else {
                printf("No response from server.\n");
            }

            //  Close socket
            close_connection(sockfd);

        //  DELETE_BOOK
        } else if (strcmp(buff, "delete_book\n") == 0) {
            sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0){
                error("Socket unavailable. Connection refused.\n");
            }
            
            //  Must have library access
            if (library_access == 0) {
                printf("You do not have access to the library!\n");
                continue;
            }

            //  Copy session_cookie; DELETE request overwrites it
            char copy_cookie[BUFLEN];
            strcpy(copy_cookie, session_cookie);

            //  Get book ID
            char id[BUFLEN];
            memset(id, 0, BUFLEN);
            //  Read
            printf("id=");
            fgets(id, BUFLEN, stdin);
            //  Remove \n
            id[strlen(id) - 1] = '\0';

            if (!valid_book(id, "id")) {
                continue;
            }

            //  Compute and send to server
            char host_ip[] = SERVER_IP;
            char url[] = DELETE_BOOK_PATH;
            strcat(url, "/");
            strcat(url, id);

            char *message = compute_delete_request(host_ip, url, session_cookie, jwt_token);
            send_to_server(sockfd, message);

            //  Receive feedback from server
            char *feedback = receive_from_server(sockfd);

            //  Bad request
            if (strstr(feedback, "error")) {
                //  Wrong ID
                if (strstr(feedback, "No book was deleted!")) {
                    printf("ERROR: Couldn't delete book. ID doesn't match.\n");
                } else {
                    printf("ERROR: Bad request.\n");
                }
            //  Successfuly retrieved books
            } else if (strstr(feedback, "ok")) {
                printf("SUCCESS: Your requested book has been deleted.\n");
            } else {
                printf("No response from server.\n");
            }

            //  Restore cookie
            strcpy(session_cookie, copy_cookie);

            //  Close socket
            close_connection(sockfd);

        //  LOGOUT
        } else if (strcmp(buff, "logout\n") == 0) {
            sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0){
                error("Socket unavailable. Connection refused.\n");
            }

            //  Must be logged in
            if (logged_in == 0) {
                printf("You are not logged in!\n");
                continue;
            }

            //  Compute and send to server
            char host_ip[] = SERVER_IP;
            char url[] = LOGOUT_PATH;

            char *message = compute_get_request(host_ip, url, NULL, session_cookie, NULL);
            send_to_server(sockfd, message);

            //  Receive feedback from server
            char *feedback = receive_from_server(sockfd);

            //  Bad request
            if (strstr(feedback, "error")) {
                //  Invalid cookie
                if (strstr(feedback, "You are not logged in!")) {
                    printf("ERROR: You are not logged in!\n");
                } else {
                    printf("ERROR: Bad request.\n");
                }
            //  Successfuly logged out
            } else if (strstr(feedback, "ok")) {
                printf("SUCCESS: You are now logged out of your account.\n");

                //  Clear data of previous session
                logged_in = 0;
                library_access = 0;
                memset(jwt_token, 0, BUFLEN);
                memset(session_cookie, 0, BUFLEN);
            }

            //  Close socket
            close_connection(sockfd);

        } else {
            printf("\nUnknown command. Possible commands are:\n\n");
            printf("register login enter_library get_books \n");
            printf("get_book add_book delete_book logout exit\n\n");
        }
    }
    return 0;
}
