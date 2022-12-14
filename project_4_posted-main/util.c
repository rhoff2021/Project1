#ifndef _REENTRANT
#define _REENTRANT
#endif

#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

int master_fd = -1;
pthread_mutex_t accept_con_mutex = PTHREAD_MUTEX_INITIALIZER;



/**********************************************
 * init
   - port is the number of the port you want the server to be
     started on
   - initializes the connection acception/handling system
   - if init encounters any errors, it will call exit().
************************************************/
void init(int port) {
  int fd;
  struct sockaddr_in addr;
   
   /**********************************************
    * IMPORTANT!
    * ALL TODOS FOR THIS FUNCTION MUST BE COMPLETED FOR THE INTERIM SUBMISSION!!!!
    **********************************************/

   // TODO: Create a socket and save the file descriptor to sd (declared above)
   // This socket should be for use with IPv4 and for a TCP connection.
  if((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    fprintf(stderr,"Failed to create new socket%s\n", strerror(errno));
    exit(1);
  }
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);     // host -> network byte order conversions
  addr.sin_port = htons(port);

   // TODO: Change the socket options to be reusable using setsockopt(). 
  int enable = 1;
  if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*) &enable, sizeof(int)) == -1) {            // error checks for socket creation
    fprintf(stderr,"Failed to change socket options to be reusable%s\n", strerror(errno));
    exit(1);
  }

   // TODO: Bind the socket to the provided port.
  if(bind(fd, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
    fprintf(stderr,"Failed to bind socket%s\n", strerror(errno));
    exit(1);
  }

   // TODO: Mark the socket as a pasive socket. (ie: a socket that will be used to receive connections)
  if(listen(fd, 20) == -1) {
    fprintf(stderr,"Failed mark socket passive%s\n", strerror(errno));
    exit(1);
  }       
   
   // We save the file descriptor to a global variable so that we can use it in accept_connection().
  master_fd = fd;
  printf("UTILS.O: Server Started on Port %d\n", port);
}

/**********************************************
 * accept_connection - takes no parameters
   - returns a file descriptor for further request processing.
     DO NOT use the file descriptor on your own -- use
     get_request() instead.
   - if the return value is negative, the thread calling
     accept_connection must should ignore request.
***********************************************/
int accept_connection(void) {
  int newsock;
  struct sockaddr_in new_addr;
  socklen_t addr_len = sizeof(struct sockaddr_in);
  
   /**********************************************
    * IMPORTANT!
    * ALL TODOS FOR THIS FUNCTION MUST BE COMPLETED FOR THE INTERIM SUBMISSION!!!!
    **********************************************/

   // TODO: Aquire the mutex lock
  if (pthread_mutex_lock(&accept_con_mutex) < 0) {
    fprintf(stderr,"Failed to lock%s\n", strerror(errno));
  }

   // TODO: Accept a new connection on the passive socket and save the fd to newsock
  if((newsock = accept(master_fd, (struct sockaddr*) &new_addr, &addr_len)) < 0) {
        fprintf(stderr,"Failed to accept new connection%s\n", strerror(errno));
  }

   // TODO: Release the mutex lock
  if (pthread_mutex_unlock(&accept_con_mutex) < 0) {
    fprintf(stderr,"Failed to unlock%s\n", strerror(errno));
  }
   // TODO: Return the file descriptor for the new client connection
  return newsock;
}

/**********************************************
 * get_request
   - parameters:
      - fd is the file descriptor obtained by accept_connection()
        from where you wish to get a request
      - filename is the location of a character buffer in which
        this function should store the requested filename. (Buffer
        should be of size 1024 bytes.)
   - returns 0 on success, nonzero on failure. You must account
     for failures because some connections might send faulty
     requests. This is a recoverable error - you must not exit
     inside the thread that called get_request. After an error, you
     must NOT use a return_request or return_error function for that
     specific 'connection'.
************************************************/
int get_request(int fd, char *filename) {

      /**********************************************
    * IMPORTANT!
    * THIS FUNCTION DOES NOT NEED TO BE COMPLETE FOR THE INTERIM SUBMISSION, BUT YOU WILL NEED
    * CODE IN IT FOR THE INTERIM SUBMISSION!!!!! 
    **********************************************/

  char buf[2048];   

   // INTERIM TODO: Read the request from the file descriptor into the buffer
  if(read(fd, buf, sizeof(buf)) == -1) {
    fprintf(stderr,"Failed to read request%s\n", strerror(errno));
  }

   // TODO: Ensure that the incoming request is a properly formatted HTTP "GET" request
   // The first line of the request must be of the form: GET <file name> HTTP/1.0 
   // or: GET <file name> HTTP/1.1
  char *token = strtok_r(buf, "\n", &token); // get string before \n in buf

  char format1[100], filenameCopy[1024], format2[100];
  sscanf(token, "%s %s %s", format1, filenameCopy, format2);    // TODO: Extract the file name from the request

  if(strcmp(format1, "GET") != 0) { // check first format requirement
    printf("incorrect format1");
    return -1;
  }

  if((strcmp(format2, "HTTP/1.0") != 0) && (strcmp(format2, "HTTP/1.1") != 0)) { // check for correct http format
    printf("incorrect format2");
    return -1;
  }
  
   // TODO: Ensure the file name does not contain with ".." or "//"
   // FILE NAMES WHICH CONTAIN ".." OR "//" ARE A SECURITY THREAT AND MUST NOT BE ACCEPTED!!!
  if(strstr(filenameCopy,"//") != NULL || strstr(filenameCopy,"..") != NULL) {
    printf("invalid filename");
    return -1;
  }

   // TODO: Copy the file name to the provided buffer
  strcpy(filename, filenameCopy);
  return 0;
}

/**********************************************
 * return_result
   - returns the contents of a file to the requesting client
   - parameters:
      - fd is the file descriptor obtained by accept_connection()
        to where you wish to return the result of a request
      - content_type is a pointer to a string that indicates the
        type of content being returned. possible types include
        "text/html", "text/plain", "image/gif", "image/jpeg" cor-
        responding to .html, .txt, .gif, .jpg files.
      - buf is a pointer to a memory location where the requested
        file has been read into memory (the heap). return_result
        will use this memory location to return the result to the
        user. (remember to use -D_REENTRANT for CFLAGS.) you may
        safely deallocate the memory after the call to
        return_result (if it will not be cached).
      - numbytes is the number of bytes the file takes up in buf
   - returns 0 on success, nonzero on failure.
************************************************/
int return_result(int fd, char *content_type, char *buf, int numbytes) {

   // TODO: Prepare the headers for the response you will send to the client.
  char header[2048];

  if (sprintf(header, "HTTP/1.0 200 OK\nContent-Length: %d\nContent-Type: %s\nConnection: Close\n\n", numbytes, content_type) == -1) { // format header
    fprintf(stderr, "failed to form header\n");
    return -1;
  }
   
   /* EXAMPLE HTTP RESPONSE.
    * 
    * HTTP/1.0 200 OK
    * Content-Length: <content length>
    * Content-Type: <content type>
    * Connection: Close
    * 
    * <File contents>
    */
    
  // TODO: Send the HTTP headers to the client
  if (send(fd, header, strlen(header), 0) == -1) {
      fprintf(stderr,"Failed to send header: %s\n", strerror(errno));
      return -1;
  }

  // TODO: Send the file contents to the client
  if (send(fd, buf, numbytes, 0) == -1) {
    fprintf(stderr,"Failed to send file content: %s\n", strerror(errno));
    return -1;
  }
  // TODO: Close the connection to the client
  if (close(fd) == -1) {
    fprintf(stderr,"Failed to to close socket: %s\n", strerror(errno));
    return -1;
  }
  
  return 0;
}

/**********************************************
 * return_error
   - returns an error message in response to a bad request
   - parameters:
      - fd is the file descriptor obtained by accept_connection()
        to where you wish to return the error
      - buf is a pointer to the location of the error text
   - returns 0 on success, nonzero on failure.
************************************************/
int return_error(int fd, char *buf) {

  // TODO: Prepare the headers to send to the client
  char error_header[2048];
  int contentLength = strlen(buf);

  if (sprintf(error_header, "HTTP/1.0 404 Not Found\nContent-Length: %d\nConnection: Close\n\n", contentLength) == -1) {
    fprintf(stderr, "failed to form error header\n");
    return -1;
  }  
   
   /* EXAMPLE HTTP ERROR RESPONSE
    * 
    * HTTP/1.0 404 Not Found
    * Content-Length: <content length>
    * Connection: Close
    * 
    * <Error Message>
    */

  // TODO: Send headers to the client
  if (send(fd, error_header, strlen(error_header), 0) == -1) {
    fprintf(stderr,"Failed to send header: %s\n", strerror(errno));
    return -1;
  }

  // TODO: Send the error message to the client
  if (send(fd, buf, contentLength, 0) == -1) {
    fprintf(stderr,"Failed to send file content: %s\n", strerror(errno));
    return -1;
  }
  
  // TODO: Close the connection with the client.
  if (close(fd) == -1) {
    fprintf(stderr,"Failed to to close socket: %s\n", strerror(errno));
    return -1;
  }
  
  return 0;
}
