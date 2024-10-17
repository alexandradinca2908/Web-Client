Tema 4 - Client web. Comunicatie cu REST API

Note: for this homework, the base skeleton of the algorithm is taken from Lab 9. I have incorporated the implementations of get/post functions, as well as all the helper functions. From there, I added new code that solves this homework's task.

Note 2: for parsing JSON replies from the server, I have picked the parson.c library, solely because the tasks suggested that it would be best recommended for C implementations, and the homework is done entirely in C.

client.c: 
    There is only one function here, the main(). It operates the entire client, with the help of auxiliary functions stored in the other source files. Firstly, it declares a bunch of variables used to store up-to-date data regarding client's status (and a few others):
        logged_in remembers whether the client is logged or not
        library_access remembers whether the client has access to the book collection or not
        session_cookie and jwt_token store the tokens received from the server, used in communication with it; they prove the clients' status to the server and allows them to receive data and manipulate the information on their account (session_cookie proves client is logged in, jwt_token proves client can access the library)

        side-note: for faster performance, we don't rely only on tokens; with the numerical variables we can check in a more optimized way if our command will work or not; and in case those checks get bypassed, the server's reply is checked at any given command, always.

    Then, the execution itself starts with the while(1). At every iteration, we wait for user input, which then enters one of the possible ramifications of the if(strcmp(buff, "command")). Moreover, we constantly open and close the socket at every valid command to keep the connection updated and minimize the risk of timeout.

    Commands:
        register: program retrieves user's input, checks the format, creates a JSON serialized object of it and sends it to the server via POST request. Feedback is checked with strstr and, depending on the content, return an appropriate message to the client

        login: program checks if user is already logged in, retrieves user's input, checks the format, creates a JSON serialized object of it and sends it to the server via POST request. Feedback is checked with strstr and, depending on the content, return an appropriate message to the client; also, if the operation was successful, we store the current's session cookie, as well as remember that the user is logged in (logged_in = 1)

        enter_library: program checks if user already has access and if he is logged in and sends a GET request to the server in order for the client to enter the library. Feedback is checked with strstr and, depending on the content, return an appropriate message to the client. Furthermore, if the operation was successful, we extract the given JWT Token and store it and update the user's library status locally (library_access = 1)

        get_books:  program checks if user has access to the library and sends a GET request to the server in order for the client to view the books. Feedback is checked with strstr and, depending on the content, return an appropriate message to the client. If the command was executed successfully, the JSON Array is extracted from the feedback and parsed, printing all fields for every book

        get_book: program checks if user has access to the library, takes and checks user's input regarding the book's ID and sends a GET request to the server in order for the client to view the book. Feedback is checked with strstr and, depending on the content, return an appropriate message to the client. Furthermore, if the operation was successful, we extract the JSON Object and print it for the user in an easy to read format.

        add_book: program checks if user has access to the library, takes and checks user's input regarding the new book's fields and sends a POST request to the server in order for the client to add the book. Feedback is checked with strstr and, depending on the content, return an appropriate message to the client.

        delete_book: program checks if user has access to the library, takes and checks user's input regarding the book's ID and sends a DELETE request to the server in order for the client to remove the book. Feedback is checked with strstr and, depending on the content, return an appropriate message to the client. The request affects the session's cookie, therefore a copy needs to be made everytime and at the end of the command, the cookie is restored.

        logout: program checks if user is logged in and sends a GET request to the server to log out the client. Feedback is checked with strstr and, depending on the content, return an appropriate message. In case the outcome is positive, all variables are reset until the next session starts.

        Any other command will be replied to with a prompt that describes the server's available operations.

helpers.c: 
    In this source file I have added 4 auxiliary funtions that mainly check the user's input and also format it into a serialized JSON string.

        valid_buff: checks for whitespaces
        get_user_data: takes user input, checks for spaces and if everything is correct then format to serialized JSON
        valid_book: individually checks the specified field for any exceptions, depending on the data type
        get_book_data: takes user input, checks for any invalid data and if everything is correct then format to serialized JSON

request.c:
    Here I only added the compute_delete_request, which is identical to the GET request, apart from method name

    