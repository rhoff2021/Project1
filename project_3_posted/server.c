#include "server.h"
#define PERM 0644

//Global Variables [Values Set in main()]
int queue_len           = INVALID;                              //Global integer to indicate the length of the queue
int cache_len           = INVALID;                              //Global integer to indicate the length or # of entries in the cache        
int num_worker          = INVALID;                              //Global integer to indicate the number of worker threads
int num_dispatcher      = INVALID;                              //Global integer to indicate the number of dispatcher threads      
FILE *logfile;                                                  //Global file pointer for writing to log file in worker


/* ************************ Global Hints **********************************/

int cacheIndex = 0;                             //[Cache]           --> When using cache, how will you track which cache entry to evict from array?
bool cacheFull = false;                         // indicator
int cacheTotal = 0;                             // keeps track of current cache entries
int workerIndex = 0;                            //[worker()]        --> How will you track which index in the request queue to remove next?
int dispatcherIndex = 0;                        //[dispatcher()]    --> How will you know where to insert the next request received into the request queue?
int curequest= 0;                               //[multiple funct]  --> How will you update and utilize the current number of requests in the request queue?


pthread_t worker_thread[MAX_THREADS];           //[multiple funct]  --> How will you track the p_thread's that you create for workers?
pthread_t dispatcher_thread[MAX_THREADS];       //[multiple funct]  --> How will you track the p_thread's that you create for dispatchers?
int threadID[MAX_THREADS];                      //[multiple funct]  --> Might be helpful to track the ID's of your threads in a global array


pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;        //What kind of locks will you need to make everything thread safe? [Hint you need multiple]
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_not_empty = PTHREAD_COND_INITIALIZER;  //What kind of CVs will you need  (i.e. queue full, queue empty) [Hint you need multiple]
pthread_cond_t queue_not_full = PTHREAD_COND_INITIALIZER;
request_t req_entries[MAX_QUEUE_LEN];                    //How will you track the requests globally between threads? How will you ensure this is thread safe?


cache_entry_t* cache_entries[MAX_CE];                                  //[Cache]  --> How will you read from, add to, etc. the cache? Likely want this to be global

/**********************************************************************************/


/*
  THE CODE STRUCTURE GIVEN BELOW IS JUST A SUGGESTION. FEEL FREE TO MODIFY AS NEEDED
*/


/* ******************************** Cache Code  ***********************************/

// Function to check whether the given request is present in cache
int getCacheIndex(char *request){
  /* TODO (GET CACHE INDEX)
  *    Description:      return the index if the request is present in the cache otherwise return INVALID
  */
  if (cacheTotal == 0) {
    return INVALID;
  }

  for (int i = 1; i < cache_len; i++) {
    if (strcmp(request, cache_entries[i]->request)) {
      return i;
    }
  }
  return INVALID;
}

// Function to add the request and its file content into the cache
void addIntoCache(char *mybuf, char *memory , int memory_size){
  /* TODO (ADD CACHE)
  *    Description:      It should add the request at an index according to the cache replacement policy
  *                      Make sure to allocate/free memory when adding or replacing cache entries
  */

  // First check if no open space
  if (cacheFull) {
    // Free the space in the current cacheindex  
    free(cache_entries[cacheIndex]->request);
    free(cache_entries[cacheIndex]->content);
    // reallocate the correct memory in the current cacheindex
    cache_entries[cacheIndex]->request = (char *)malloc(sizeof(strlen(mybuf)));
    cache_entries[cacheIndex]->content = (char *)malloc(memory_size);
    // add content to cacheindex
    cache_entries[cacheIndex]->len = memory_size;
    strcpy(cache_entries[cacheIndex]->request, mybuf);
    strcpy(cache_entries[cacheIndex]->content, memory);
  } else { // there is free space, allocate next entry
    cacheTotal++;
    cache_entries[cacheIndex]->request = (char *)malloc(sizeof(strlen(mybuf)));
    cache_entries[cacheIndex]->content = (char *)malloc(memory_size);
    // add content to cacheindex
    cache_entries[cacheIndex]->len = memory_size;
    strcpy(cache_entries[cacheIndex]->request, mybuf);
    strcpy(cache_entries[cacheIndex]->content, memory);
  }

  // check if cache has been filled
  if (cacheTotal == cache_len) {
    cacheFull = true;
  }
  
  // checks if cache has hit the end of cache -> begins at 0
  if(cacheIndex == cache_len) { 
    cacheIndex = 0;
  } else {
    cacheIndex++; // keeps looping through the cache to replace
  }
}

// Function to clear the memory allocated to the cache
void deleteCache(){
  /* TODO (CACHE)
  *    Description:      De-allocate/free the cache memory
  */
  for (int i = 0; i < cache_len; i++) {
    free(&cache_entries[i]->len);
    free(cache_entries[i]->request);
    free(cache_entries[i]->content);
    free(cache_entries[i]);
  }
  cacheFull = false;
  cacheIndex = 0;
  cacheTotal = 0;
}

// Function to initialize the cache
void initCache(){
  /* TODO (CACHE)
  *    Description:      Allocate and initialize an array of cache entries of length cache size
  */
  for (int i = 0; i < cache_len; i++) {
    cache_entries[i] = malloc(sizeof(cache_entry_t));
    cache_entries[i]->content = (char*)malloc(sizeof(char*));
    cache_entries[i]->request = (char*)malloc(sizeof(char*));


    cache_entries[i]->content = NULL;
    cache_entries[i]->request = NULL;
    cache_entries[i]->len = -1;
  }
  cacheFull = true;
}

/**********************************************************************************/

/* ************************************ Utilities ********************************/
// Function to get the content type from the request
char* getContentType(char *mybuf) {
  /* TODO (Get Content Type)
  *    Description:      Should return the content type based on the file type in the request
  *                      (See Section 5 in Project description for more details)
  *    Hint:             Need to check the end of the string passed in to check for .html, .jpg, .gif, etc.
  */
  const char c[2] = ".";
  char *content_type = strtok(mybuf, c);

   //TODO remove this line and return the actual content type
  if (strcmp(content_type, "html")) {
    return "text/html";
  } else if (strcmp(content_type, "jpeg")) {
    return "image/jpeg";
  } else if (strcmp(content_type, "gif")) {
    return "image/gif";
  }  else {
    return "text/plain";
  }
}

// Function to open and read the file from the disk into the memory. Add necessary arguments as needed
// Hint: caller must malloc the memory space
int readFromDisk(int fd, char *mybuf, void **memory) {
  //    Description: Try and open requested file, return INVALID if you cannot meaning error

  FILE *fp = fopen(mybuf+1, "r");
  if(fp == NULL){
     fprintf (stderr, "ERROR: Fail to open the file.\n");
    return INVALID;
  }

  fprintf (stderr,"The requested file path is: %s\n", mybuf);
  
  /* TODO 
  *    Description:      Find the size of the file you need to read, read all of the contents into a memory location and return the file size
  *    Hint:             Using fstat or fseek could be helpful here
  *                      What do we do with files after we open them?
  */

  struct stat buf;

  if (fstat(fd, &buf) == -1) {
    printf("fstat error.\n");
    fclose(fp);
    return INVALID;
  }
  *memory = malloc(stat);
  printf("here!\n");
  //*memory = malloc(buf.st_size);

  fclose(fp);
  return buf.st_size;
}

/**********************************************************************************/

// Function to receive the path request from the client and add to the queue
void * dispatch(void *arg) {

  /********************* DO NOT REMOVE SECTION - TOP     *********************/

  /* TODO (B.I)
  *    Description:      Get the id as an input argument from arg, set it to ID
  */
  //int id = *(int *) arg;
  //threadID[id] = pthread_self();
  while (1) {
    /* TODO (FOR INTERMEDIATE SUBMISSION)
    *    Description:      Receive a single request and print the conents of that request
    *                      The TODO's below are for the full submission, you do not have to use a 
    *                      buffer to receive a single request 
    *    Hint:             Helpful Functions: int accept_connection(void) | int get_request(int fd, char *filename
    *                      Recommend using the request_t structure from server.h to store the request. (Refer section 15 on the project write up)
    */
    // request_t req;

    /* TODO (B.II)
    *    Description:      Accept client connection
    *    Utility Function: int accept_connection(void) //utils.h => Line 24
    */
    int fd = accept_connection();
    req_entries[curequest].fd = fd;

    /* TODO (B.III)
    *    Description:      Get request from the client
    *    Utility Function: int get_request(int fd, char *filename); //utils.h => Line 41
    */
    char buff[BUFF_SIZE];
    if(fd > 0) {
      get_request(fd, buff);
    }

    fprintf(stderr, "Dispatcher Received Request: fd[%d] request[%s]\n", fd, buff);
    /* TODO (B.IV)
    *    Description:      Add the request into the queue
    */

        //(1) Copy the filename from get_request into allocated memory to put on request queue
    req_entries[curequest].request = (char*)malloc(sizeof(strlen(buff)+1));    // malloc memory to the length of the string in buff
    strcpy(req_entries[curequest].request, buff);
    // memcpy(req_entries[curequest].request, buff, (strlen(buff)));              // memcpy buff string to req.request

        //(2) Request thread safe access to the request queue
    pthread_mutex_lock(&lock);
        //(3) Check for a full queue... wait for an empty one which is signaled from req_queue_notfull
    if (curequest == queue_len) {
      pthread_cond_wait(&queue_not_full, &lock);
    }
    
        //(4) Insert the request into the queue
    strcpy(req_entries[curequest].request, buff);                   // put request into queue
        
        //(5) Update the queue index in a circular fashion
    if(curequest == queue_len) {                             // if at max queue length, set queue index to beginning
      curequest = 0;                                            // else set queue index to the next index.
    }  else {
      curequest++;
    }

        //(6) Release the lock on the request queue and signal that the queue is not empty anymore
    pthread_cond_signal(&queue_not_empty);
    pthread_mutex_unlock(&lock);
  }

  return NULL;
}

/**********************************************************************************/
// Function to retrieve the request from the queue, process it and then return a result to the client
void * worker(void *arg) {
  /********************* DO NOT REMOVE SECTION - BOTTOM      *********************/

  #pragma GCC diagnostic ignored "-Wunused-variable"      //TODO --> Remove these before submission and fix warnings
  #pragma GCC diagnostic push                             //TODO --> Remove these before submission and fix warnings


  // Helpful/Suggested Declarations
  int num_request = 0;                                    //Integer for tracking each request for printing into the log
  bool cache_hit  = false;                                //Boolean flag for tracking cache hits or misses if doing 
  int filesize    = 0;                                    //Integer for holding the file size returned from readFromDisk or the cache
  void *memory    = NULL;                                 //memory pointer where contents being requested are read and stored
  int fd          = INVALID;                              //Integer to hold the file descriptor of incoming request
  char mybuf[BUFF_SIZE];                                  //String to hold the file path from the request

  #pragma GCC diagnostic pop                              //TODO --> Remove these before submission and fix warnings



  /* TODO (C.I)
  *    Description:      Get the id as an input argument from arg, set it to ID
  */
  int id = *(int *) arg;
  //threadID[id] = pthread_self();

  while (1) {
    /* TODO (C.II)
    *    Description:      Get the request from the queue and do as follows
    */
          //(1) Request thread safe access to the request queue by getting the req_queue_mutex lock
    pthread_mutex_lock(&lock);
          //(2) While the request queue is empty conditionally wait for the request queue lock once the not empty signal is raised
    while (curequest == 0) {
      pthread_cond_wait(&queue_not_empty, &lock);
    }
          //(3) Now that you have the lock AND the queue is not empty, read from the request queue
    //printf("req_entries[%d] is: %d\n", curequest, req_entries->fd);
    fd = req_entries[workerIndex].fd;                          // set fd to first request's fd
    strcpy(mybuf, req_entries[workerIndex].request);            // set mybuf to first request's uri

          //(4) Update the request queue remove index in a circular fashion
    if (curequest == 0) {                   // if curequest is at beginning of the queue, set queue index to the last index
      workerIndex = 0;
      curequest = (queue_len-1);                // else decrement to the next array
    } else {
      workerIndex++;
      curequest--;
    }

          //(5) Check for a path with only a "/" if that is the case add index.html to it
    if (strcmp(mybuf, "/") == 0) {
      strcat(mybuf, "index.html");
    }

          //(6) Fire the request queue not full signal to indicate the queue has a slot opened up and release the request queue lock
    pthread_cond_signal(&queue_not_full);
    pthread_mutex_unlock(&lock);

    /* TODO (C.III)
    *    Description:      Get the data from the disk or the cache 
    *    Local Function:   int readFromDisk(//necessary arguments//);
    *                      int getCacheIndex(char *request);  
    *                      void addIntoCache(char *mybuf, char *memory , int memory_size);  
    */
    int cache = getCacheIndex(mybuf);

    if (cache != INVALID) {
      printf("in cache\n");
      cache_hit = true;
      filesize = cache_entries[cache]->len;
      memory = cache_entries[cache]->content;
      //filesize = atoi(cache_entries[cacheIndex]->content);
    } else {
      printf("in disk\n");
      filesize = readFromDisk(fd, mybuf, memory);
      addIntoCache(mybuf, memory, filesize);
    } else {
      cache_hit = true;
      filesize = cache_entries[cacheIndex]->len;
      strcpy(memory, cache_entries[cacheIndex]->content);
    }
    
    /* TODO (C.IV)
    *    Description:      Log the request into the file and terminal
    *    Utility Function: LogPrettyPrint(FILE* to_write, int threadId, int requestNumber, int file_descriptor, char* request_str, int num_bytes_or_error, bool cache_hit);
    *    Hint:             Call LogPrettyPrint with to_write = NULL which will print to the terminal
    *                      You will need to lock and unlock the logfile to write to it in a thread safe manor
    */
    pthread_mutex_lock(&lock);
    logfile = fopen(LOG_FILE_NAME, "w+");
    LogPrettyPrint(logfile, id, num_request, fd, mybuf, filesize, cache_hit);
    num_request++;
    fclose(logfile);
    pthread_mutex_unlock(&lock);

    /* TODO (C.V)
    *    Description:      Get the content type and return the result or error
    *    Utility Function: (1) int return_result(int fd, char *content_type, char *buf, int numbytes); //look in utils.h 
    *                      (2) int return_error(int fd, char *buf); //look in utils.h 
    */
    if (return_result(fd, getContentType(mybuf), memory, filesize) != 0) {
      return_error(fd, mybuf);
    }
    if (cache_hit == false) {
      free(memory);
    }
  }

  return NULL;
}

/**********************************************************************************/

int main(int argc, char **argv) {

  /********************* DO NOT REMOVE SECTION - TOP     *********************/
  // Error check on number of arguments
  if(argc != 7){
    printf("usage: %s port path num_dispatcher num_workers queue_length cache_size\n", argv[0]);
    return -1;
  }


  int port            = -1;
  char path[BUFF_SIZE] = "no path set\0";
  num_dispatcher      = -1;                               //global variable
  num_worker          = -1;                               //global variable
  queue_len           = -1;                               //global variable
  cache_len           = -1;                               //global variable


  /********************* DO NOT REMOVE SECTION - BOTTOM  *********************/
  /* TODO (A.I)
  *    Description:      Get the input args --> (1) port (2) path (3) num_dispatcher (4) num_workers  (5) queue_length (6) cache_size
  */
  port = atoi(argv[1]);
  strcpy(path, argv[2]);
  num_dispatcher = atoi(argv[3]);
  num_worker = atoi(argv[4]);
  queue_len = atoi(argv[5]);
  cache_len = atoi(argv[6]);

  /* TODO (A.II)
  *    Description:     Perform error checks on the input arguments
  *    Hints:           (1) port: {Should be >= MIN_PORT and <= MAX_PORT} | (2) path: {Consider checking if path exists (or will be caught later)}
  *                     (3) num_dispatcher: {Should be >= 1 and <= MAX_THREADS} | (4) num_workers: {Should be >= 1 and <= MAX_THREADS}
  *                     (5) queue_length: {Should be >= 1 and <= MAX_QUEUE_LEN} | (6) cache_size: {Should be >= 1 and <= MAX_CE}
  */
  
  if (port < MIN_PORT || port > MAX_PORT) {
    return -1;
  // } else if (path ) {     // idk what to do
    // return -1;
  } else if (num_dispatcher < 1 || num_dispatcher > MAX_THREADS) {
    return -1;
  } else if (num_worker < 1 || num_worker > MAX_THREADS) {
    return -1;
  }  else if (queue_len < 1 || queue_len > MAX_QUEUE_LEN) {
    return -1;
  }  else if (cache_len < 1 || cache_len > MAX_CE) {
    return -1;
  }
 


  /********************* DO NOT REMOVE SECTION - TOP    *********************/
  printf("Arguments Verified:\n\
    Port:           [%d]\n\
    Path:           [%s]\n\
    num_dispatcher: [%d]\n\
    num_workers:    [%d]\n\
    queue_length:   [%d]\n\
    cache_size:     [%d]\n\n", port, path, num_dispatcher, num_worker, queue_len, cache_len);
  /********************* DO NOT REMOVE SECTION - BOTTOM  *********************/


  /* TODO (A.III)
  *    Description:      Open log file
  *    Hint:             Use Global "File* logfile", use "web_server_log" as the name, what open flags do you want?
  */
  logfile = fopen("web_server_log", "r+");

  /* TODO (A.IV)
  *    Description:      Change the current working directory to server root directory
  *    Hint:             Check for error!
  */
  chdir(path);

  /* TODO (A.V)
  *    Description:      Initialize cache  
  *    Local Function:   void    initCache();
  */
  initCache();

  /* TODO (A.VI)
  *    Description:      Start the server
  *    Utility Function: void init(int port); //look in utils.h 
  */
  init(port);

  /* TODO (A.VII)
  *    Description:      Create dispatcher and worker threads 
  *    Hints:            Use pthread_create, you will want to store pthread's globally
  *                      You will want to initialize some kind of global array to pass in thread ID's
  *                      How should you track this p_thread so you can terminate it later? [global]
  */
  
  int dispatch_args[num_dispatcher];
  int worker_args[num_worker];

  for (int i = 0; i < MAX_THREADS; i++) {
    threadID[i] = i;
  }

  // creating dispatcher threads
  for(int i = 0; i < num_dispatcher; i++) {
    if(pthread_create(&dispatcher_thread[i], NULL, dispatch, (void*) &threadID[i]) != 0) {
      fprintf(stderr,"Error creating dispatcher thread\n");
		  exit(1);
    }
  }

  // creating worker threads
  for(int i = 0; i < num_worker; i++) {
    if(pthread_create(&worker_thread[i], NULL, worker, (void*) &threadID[i]) != 0) {
      fprintf(stderr,"Error creating worker thread\n");
      exit(1);
    }
  }

  //fclose(logfile);

  // Wait for each of the threads to complete their work
  // Threads (if created) will not exit (see while loop), but this keeps main from exiting
  int i;
  for(i = 0; i < num_worker; i++){
    fprintf(stderr, "JOINING WORKER %d \n",i);
    if((pthread_join(worker_thread[i], NULL)) != 0){
      printf("ERROR : Fail to join worker thread %d.\n", i);
    }
  }
  for(i = 0; i < num_dispatcher; i++){
    fprintf(stderr, "JOINING DISPATCHER %d \n",i);
    if((pthread_join(dispatcher_thread[i], NULL)) != 0){
      printf("ERROR : Fail to join dispatcher thread %d.\n", i);
    }
  }
  fprintf(stderr, "SERVER DONE \n");  // will never be reached in SOLUTION
}