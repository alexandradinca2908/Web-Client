#ifndef _HELPERS_
#define _HELPERS_

#define BUFLEN 4096
#define LINELEN 1000

// shows the current error
void error(const char *msg);

// adds a line to a string message
void compute_message(char *message, const char *line);

// opens a connection with server host_ip on port portno, returns a socket
int open_connection(char *host_ip, int portno, int ip_type, int socket_type, int flag);

// closes a server connection on socket sockfd
void close_connection(int sockfd);

// send a message to a server
void send_to_server(int sockfd, char *message);

// receives and returns the message from a server
char *receive_from_server(int sockfd);

// extracts and returns a JSON from a server response
char *basic_extract_json_response(char *str);


//  ADDED FUNCTIONS ASIDE FROM LAB 9

//  checks if a buff contains spaces
int valid_buff(char *buff);

//  retrieves data from users while also checking if it is properly formatted;
//  returns a serialized json
char *get_user_data();

//  checks each field of a book, depending on its type and returns 1 if it s ok, 0 if not
int valid_book(char *field, char *type);

//  retrieves book data from users while also checking if it is properly formatted
//  returns a serialized json
char *get_book_data();
#endif
