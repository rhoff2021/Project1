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

char *blacklist[MAX_BAD];
                                      
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
    Function: on_blacklist  --> "Check if the provided URI is in th blacklist"
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
  
  // thinking about putting a for loop here - Ji
  // char* w_format = "www."
  char uri_copy[MAX_URL];
  strcat(uri, "\n");
  strcpy(uri_copy, uri);
  printf("Uri: %s", uri_copy);
  for(int i = 0; i<MAX_BAD; i++){
    char b_url[MAX_URL] = "www.";
    char b_url_http[MAX_URL] = "http://";
    char b_url_https[MAX_URL] = "https://";

    if(strlen(blacklist[i]) == 0) {
      return 0;
    }
    // printf("%d url is:%s-\n", i,blacklist[i]);
    if(strstr(blacklist[i], "www.") == NULL){
      strcat(b_url, blacklist[i]);
    } else {
      strcpy(b_url, blacklist[i]);
    }
    strcat(b_url_http, b_url);
    strcat(b_url_https, b_url);
    printf("-%s", b_url_http);
    printf("-%s", b_url_https);
    printf("%d\n%d\n",strcmp(uri_copy, b_url_http), strcmp(uri_copy, b_url_https));

    if(strcmp(uri_copy, b_url_http) == 0 || strcmp(uri_copy, b_url_https) == 0) {
      printf("url is on blacklist\n");
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
  
  // first checks if there is a URL, then checks the format
  // if format is good, it will check if its on blacklist
  // if not it will check the # of tabs
  if (strlen(uri) == 0) {                     
  	alert("NO URL.");
  } else if (bad_format(uri) == 1) {
  	alert("BAD FORMAT.");
  } else {  
      init_blacklist("blacklist");
      printf("initializes blacklist\n");                                
      if(on_blacklist(uri) == 1){
        printf("on blacklist\n");
        alert("Url is blacklisted");
      } else if(tab_count+1 == MAX_URL) {
        	alert("MAX TABS REACHED.");
      } else { // URL is valid and can be rendered
          // render page
          pid_t pid = fork();
          int status;
          browser_window *b_window = NULL;
          render_web_page_in_tab(uri, b_window);
          if(pid == -1){
              exit(1);
          } else if (pid == 0) {
              printf("The Url is %s", uri);
              render_web_page_in_tab(uri, b_window);
          } else {
              wait(&status);
          }
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
  FILE* fp = fopen(fname, "r");     // file i/o stuff
  char buf[MAX_URL];
  int count = 0;
  if (ferror(fp)) {
    printf("Error with fopen(). Cannot create blacklist.\n");     // check if fopen succeeded
  } else {
    while(fgets(buf, MAX_URL, fp) != NULL) {
      //*blacklist[count] = *buf;
      blacklist[count] = buf;
      
      printf("size of black list is: %ld\n", sizeof(blacklist));
      printf("buf is: %s\n", buf);
      
      if(count == MAX_BAD){
        break;
      }
      count++;
    }
    fclose(fp);
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

  //STUDENT IMPLEMENT

  // (a) Check arguments for blacklist, error and warn if no blacklist
  if (argc != 2) {
    fprintf (stderr, "browser <blacklist_file>\n");
    exit (0);
  }

  int status;
  pid_t child_process = fork();
  if(child_process == -1) { // child
    perror("fork() failed");
    exit(1);
  } else if (child_process == 0) {
      run_control();
      kill(child_process, 1);
  } else {
    wait(&status); // parent
  }

  return 0;
}