/* CSCI-4061 Fall 2022
 * Group Member #1: Rebecca Hoff hoff1542
 * Group Member #2: Aidan Boyle boyle181
 * Group Member #3: Ji Moua moua0345
 */

#include "wrapper.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <gtk/gtk.h>

/* === PROVIDED CODE === */
// Function Definitions
void new_tab_created_cb(GtkButton *button, gpointer data);
int run_control();
int on_blacklist(char *uri);
int bad_format (char *uri);
void uri_entered_cb(GtkWidget* entry, gpointer data);
void init_blacklist (char *fname);

/* === PROVIDED CODE === */
// Global Definitions
#define MAX_TAB 100                 //Maximum number of tabs allowed
#define MAX_BAD 1000                //Maximum number of URL's in blacklist allowed
#define MAX_URL 100                 //Maximum char length of a url allowed

/* === STUDENTS IMPLEMENT=== */
// HINT: What globals might you want to declare?

char blacklist[MAX_BAD][MAX_URL];
                                      
int tab_count = 0;

/* === PROVIDED CODE === */
/*
 * Name:		          new_tab_created_cb
 *
 * Input arguments:	
 *      'button'      - whose click generated this callback
 *			'data'        - auxillary data passed along for handling
 *			                this event.
 *
 * Output arguments:   void
 * 
 * Description:        This is the callback function for the 'create_new_tab'
 *			               event which is generated when the user clicks the '+'
 *			               button in the controller-tab. The controller-tab
 *			               redirects the request to the parent (/router) process
 *			               which then creates a new child process for creating
 *			               and managing this new tab.
 */
// NO-OP for now
void new_tab_created_cb(GtkButton *button, gpointer data)
{}
 
/* === PROVIDED CODE === */
/*
 * Name:                run_control
 * Output arguments:    void
 * Function:            This function will make a CONTROLLER window and be blocked until the program terminates.
 */
int run_control()
{
  // (a) Init a browser_window object
	browser_window * b_window = NULL;

	// (b) Create controller window with callback function
	create_browser(CONTROLLER_TAB, 0, G_CALLBACK(new_tab_created_cb),
		       G_CALLBACK(uri_entered_cb), &b_window);

	// (c) enter the GTK infinite loop
	show_browser();
	return 0;
}

/* === STUDENTS IMPLEMENT=== */
/* 
    Function: on_blacklist  --> "Check if the provided URI is in the blacklist"
    Input:    char* uri     --> "URI to check against the blacklist"
    Returns:  True  (1) if uri in blacklist
              False (0) if uri not in blacklist
    Hints:
            (a) File I/O
            (b) Handle case with and without "www." (see writeup for details)
            (c) How should you handle "http://" and "https://" ??
*/ 
int on_blacklist (char *uri) {
  //STUDENTS IMPLEMENT

  char b_url[MAX_URL];
  // remove https:// or http:// from uri
  if((strstr(uri, "https") == NULL) ) { // uri does not contain https://, so remove http://
    sscanf(uri, "http://%s", b_url);
  } else {
    sscanf(uri, "https://%s", b_url); // removes https://
  }
  // check if uri has "www."" and removes it
  if(strstr(b_url, "www.") != NULL){
    sscanf(b_url, "www.%s", b_url);
  }

  // loops through blacklist and compares
  for (int i = 0; i < MAX_BAD; i++) {
    if (strlen(blacklist[i]) == 0) {          // reached end of blacklist and returns early
      return 0;
    }
    if (strcmp(b_url, blacklist[i]) == 0) {   // found uri in blacklist
      return 1;
    }
  }
  return 0;
}

/* === STUDENTS IMPLEMENT=== */
/* 
    Function: bad_format    --> "Check for a badly formatted url string"
    Input:    char* uri     --> "URI to check if it is bad"
    Returns:  True  (1) if uri is badly formatted 
              False (0) if uri is well formatted
    Hints:
              (a) String checking for http:// or https://
*/
int bad_format (char *uri) {
  //STUDENTS IMPLEMENT
  const char *format1 = "http://";      // const chars for strstr compare
  const char *format2 = "https://";

   // checks if either "http://" or "https://" is in URI, if not returns 1, otherwise returns 0
  if (strstr(uri, format1) == NULL && strstr(uri, format2) == NULL) {
    return 1;
  }
  return 0;
}

/* === STUDENTS IMPLEMENT=== */
/*
 * Name:                uri_entered_cb
 *
 * Input arguments:     
 *                      'entry'-address bar where the url was entered
 *			                'data'-auxiliary data sent along with the event
 *
 * Output arguments:     void
 * 
 * Function:             When the user hits the enter after entering the url
 *			                 in the address bar, 'activate' event is generated
 *			                 for the Widget Entry, for which 'uri_entered_cb'
 *			                 callback is called. Controller-tab captures this event
 *			                 and sends the browsing request to the router(/parent)
 *			                 process.
 * Hints:
 *                       (a) What happens if data is empty? No Url passed in? Handle that
 *                       (b) Get the URL from the GtkWidget (hint: look at wrapper.h)
 *                       (c) Print the URL you got, this is the intermediate submission
 *                       (d) Check for a bad url format THEN check if it is in the blacklist
 *                       (e) Check for number of tabs! Look at constraints section in lab
 *                       (f) Open the URL, this will need some 'forking' some 'execing' etc. 
 */
void uri_entered_cb(GtkWidget* entry, gpointer data)
{
  //STUDENTS IMPLEMENT
  char *uri = get_entered_uri(entry);
  if (strlen(uri) == 0 || entry == NULL) {   // first checks if there is a URL,                   
  	alert("NO URL.");
  } else if (bad_format(uri) == 1) {         // then checks the format
  	alert("BAD FORMAT.");
  } else {
      if (on_blacklist(uri) == 1){           //  check if its on blacklist
        alert("URL IS BLACKLISTED.");
      } else if (tab_count == MAX_TAB) {     // check the # of tabs
        	alert("MAX TABS REACHED."); 
      } else {                               // URL is valid and can be rendered
          pid_t pid = fork();
          char cnum[4];

          sprintf(cnum, "%d", tab_count);

          if(pid == -1){                     // check if new forked child has error 
              perror("failed to fork");
              exit(1);
          } else if (pid == 0) {             // otherwise it renders a new tab window
              execl("./render", "render", cnum,uri,(char*)NULL); 
              perror("child failed to exec");

          }
          tab_count++;
      }
    }
  return;
}

/* === STUDENTS IMPLEMENT=== */
/* 
    Function: init_blacklist --> "Open a passed in filepath to a blacklist of url's, read and parse into an array"
    Input:    char* fname    --> "file path to the blacklist file"
    Returns:  void
    Hints:
            (a) File I/O (fopen, fgets, etc.)
            (b) If we want this list of url's to be accessible elsewhere, where do we put the array?
*/
void init_blacklist (char *fname) {
  //STUDENTS IMPLEMENT
  FILE* fp = fopen(fname, "r");           // opens file full of blacklisted urls
  char buf[MAX_URL];
  int count = 0;
  if (ferror(fp)) {                       // check if fopen succeeded
    printf("Error with fopen(). Cannot create blacklist.\n");     
  } else {
    while(fgets(buf, MAX_URL, fp) != NULL ) {
      strtok(buf, "\n\r");                // gets rid of special characters

      if (strstr(buf, "www.") != NULL) {  // checks if there is a www. at the begining of url's in blacklist
        sscanf(buf, "www.%s", blacklist[count]); // if there is it removes www. and copies url in local blacklist
      } else {                            // if there was not a www., copies url into local blacklist
        strcpy(blacklist[count], buf); 
      }

      if(count == MAX_BAD){               // checks if the max amount of urls allowed in blacklist has been reached
        break;
      }
      count++;
    }
    fclose(fp);                           // closes file when done looping through it
  }
  clearerr(fp);
  return;
}

/* === STUDENTS IMPLEMENT=== */
/* 
    Function: main 
    Hints:
            (a) Check for a blacklist file as argument, fail if not there [PROVIDED]
            (b) Initialize the blacklist of url's
            (c) Create a controller process then run the controller
                (i)  What should controller do if it is exited? Look at writeup (KILL! WAIT!)
            (d) Parent should not exit until the controller process is done 
*/
int main(int argc, char **argv)
{
  // (a) Check arguments for blacklist, error and warn if no blacklist
  if (argc != 2) {
    fprintf (stderr, "browser <blacklist_file>\n");
    exit (0);
  }
  // (b) Initialize the blacklist of url's
  init_blacklist(argv[1]);
  // (c) Create a controller process then run the controller
  int status;
  pid_t child_process = fork();
  if(child_process == -1) {           // if error found, exits
    perror("fork() failed");
    exit(1);
  } else if (child_process == 0) {    // running child process
      run_control();
      kill(child_process, 1);
  } else {                            // running parent process
    wait(&status);
  }
  return 0;
}