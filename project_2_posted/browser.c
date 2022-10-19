#include "wrapper.h"
#include "util.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <signal.h>

#define MAX_TABS 100  // this gives us 99 tabs, 0 is reserved for the controller
#define MAX_BAD 1000
#define MAX_URL 100
#define MAX_FAV 100
#define MAX_LABELS 100


comm_channel comm[MAX_TABS];         // Communication pipes 
char favorites[MAX_FAV][MAX_URL];    // Maximum char length of a url allowed
int num_fav = 0;                     // # favorites

typedef struct tab_list {
  int free;
  int pid; // may or may not be useful
} tab_list;

// Tab bookkeeping
tab_list TABS[MAX_TABS];



/************************/
/* Simple tab functions */
/************************/

// return total number of tabs
int get_num_tabs () {
  int count = 1;
  for (int i = 0; i < MAX_TABS; i++) {            // checks # of tabs in TABS array
    if (TABS[i].free == 0) {
      count++;
    }
  }
  return count;
}

// get next free tab index
int get_free_tab () {
  int free_index;
  for (int i = 0; i < MAX_TABS; i++) {           // looks for TABS[i].free == 1
    if (TABS[i].free == 1) {
      free_index = i;
      break;
    }
  }
  return free_index;
}

// init TABS data structure
void init_tabs () {
  int i;

  for (i=1; i<MAX_TABS; i++)
    TABS[i].free = 1;
  TABS[0].free = 0;
}

/***********************************/
/* Favorite manipulation functions */
/***********************************/

// return 0 if favorite is ok, -1 otherwise
// both max limit, already a favorite (Hint: see util.h) return -1
int fav_ok (char *uri) {
  if (on_favorites(uri)) {      // check if on favorites list (using util.h function)
    return -1;
  } else if (num_fav+1 == MAX_FAV){ // check if we reach MAX_FAV
    return -1;
  }
  return 0;
}


// Add uri to favorites file and update favorites array with the new favorite
void update_favorites_file (char *uri) {
  // Add uri to favorites file

  // Update favorites array with the new favorite
}

// Set up favorites array
void init_favorites (char *fname) {
  FILE *fp = fopen(fname, "w+");
  char buf[MAX_URL];
  int count = 0;

  if (ferror(fp)) {
    printf("File error. Could not initialize favorites array.\n");          // error checking favorites file
  } else {
    while (fgets(buf, MAX_URL, fp) != NULL) {         // fgets to make favorites array
      strtok(buf, "\n\r");
      
      if (count == MAX_FAV) {         //check if favorites array is full
        break;
      } else {
        strcpy(favorites[count], buf);
      }
      count++;
    }
  }
  clearerr(fp);
  fclose(fp);
}

// Make fd non-blocking just as in class!
// Return 0 if ok, -1 otherwise
// Really a util but I want you to do it :-)
int non_block_pipe (int fd) {
  return 0;
}

/***********************************/
/* Functions involving commands    */
/***********************************/

// Checks if tab is bad and url violates constraints; if so, return.
// Otherwise, send NEW_URI_ENTERED command to the tab on inbound pipe
void handle_uri (char *uri, int tab_index) {
  if(bad_format(uri) == 1) { // checks for format first, returns early if bad format
    alert("BAD FORMAT");
    return;
  } else{
    if(on_blacklist(uri) == 1) { // checks if uri is on blacklist and returns
      alert("ON BLACKLIST");
      return;
    }
  }
  write(comm[tab_index].inbound[1], NEW_URI_ENTERED, sizeof(int));         // putting a write here, but i don't know how to go about this
}


// A URI has been typed in, and the associated tab index is determined
// If everything checks out, a NEW_URI_ENTERED command is sent (see Hint)
// Short function
void uri_entered_cb (GtkWidget* entry, gpointer data) {
  
  if(data == NULL) {	
    return;
  }

  // Get the tab (hint: wrapper.h)


  // Get the URL (hint: wrapper.h)


  // Hint: now you are ready to handle_the_uri

}
  

// Called when + tab is hit
// Check tab limit ... if ok then do some heavy lifting (see comments)
// Create new tab process with pipes
// Long function
void new_tab_created_cb (GtkButton *button, gpointer data) {
  if (data == NULL) {
    return;
  }

  // at tab limit?
  if (get_num_tabs() == MAX_TABS) {           // counts tabs using get_num_tabs
    alert("MAX TABS REACHED.");
    return;
  }

  // Get a free tab
  int index = get_free_tab();             // uses get_free_tab to get index for new tab

  // Create communication pipes for this tab 
  pipe(comm[index].inbound);
  pipe(comm[index].outbound);

  // Make the read ends non-blocking
  int inbound_flags = fcntl(comm[index].inbound[0], F_GETFL, 0);
  fcntl(comm[index].inbound[0], F_SETFL, inbound_flags | O_NONBLOCK);
  int outbound_flags = fcntl(comm[index].outbound[0], F_GETFL, 0);
  fcntl(comm[index].outbound[0], F_SETFL, outbound_flags | O_NONBLOCK);

  // fork and create new render tab
  int tab_process = fork();
  int status;
  char arg1[4];
  char arg2[8];

  sprintf(arg1, "%d", index);
  sprintf(arg2, "%d %d %d %d", comm[index].inbound[0], comm[index].inbound[1], comm[index].outbound[0], comm[index].outbound[2]);


  if(tab_process == -1) {
    perror("fork() failed");
    exit(1);
  } else if (tab_process == 0) {
      execl("./render", "render", arg1, arg2, (char*)NULL);
  } else {
      wait(&status);
  }

  // Note: render has different arguments now: tab_index, both pairs of pipe fd's
  // (inbound then outbound) -- this last argument will be 4 integers "a b c d"
  // Hint: stringify args

  // Controller parent just does some TABS bookkeeping
}

// This is called when a favorite is selected for rendering in a tab
// Hint: you will use handle_uri ...
// However: you will need to first add "https://" to the uri so it can be rendered
// as favorites strip this off for a nicer looking menu
// Short
void menu_item_selected_cb (GtkWidget *menu_item, gpointer data) {

  if (data == NULL) {
    return;
  }
  
  // Note: For simplicity, currently we assume that the label of the menu_item is a valid url
  // get basic uri
  char *basic_uri = (char *)gtk_menu_item_get_label(GTK_MENU_ITEM(menu_item));

  // append "https://" for rendering
  char uri[MAX_URL];
  sprintf(uri, "https://%s", basic_uri);

  // Get the tab (hint: wrapper.h)

  // Hint: now you are ready to handle_the_uri

  return;
}


// BIG CHANGE: the controller now runs an loop so it can check all pipes
// Long function
int run_control() {
  browser_window * b_window = NULL;
  int i, nRead;
  req_t req;

  //Create controller window
  create_browser(CONTROLLER_TAB, 0, G_CALLBACK(new_tab_created_cb),
		 G_CALLBACK(uri_entered_cb), &b_window, comm[0]);

  // Create favorites menu
  create_browser_menu(&b_window, &favorites, num_fav);
  
  while (1) {
    process_single_gtk_event();

    // Read from all tab pipes including private pipe (index 0)
    // to handle commands:
    // PLEASE_DIE (controller should die, self-sent): send PLEASE_DIE to all tabs
    // From any tab:
    //    IS_FAV: add uri to favorite menu (Hint: see wrapper.h), and update .favorites
    //    TAB_IS_DEAD: tab has exited, what should you do?

    // Loop across all pipes from VALID tabs -- starting from 0
    for (i=0; i<MAX_TABS; i++) {
      if (TABS[i].free) continue;
      // nRead = read(comm[i].outbound[0], &req, sizeof(req_t));
      // if(nRead == -1 && errno == EAGAIN) {
      //   alert("Failed to read outbound pipe");
      // }

      // Check that nRead returned something before handling cases

      // Case 1: PLEASE_DIE

      // Case 2: TAB_IS_DEAD
	    
      // Case 3: IS_FAV
    }
    usleep(1000);
  }
  return 0;
}


int main(int argc, char **argv)
{
  init_tabs ();
  init_blacklist(".blacklist");
  init_favorites(".favorites");

  // init blacklist (see util.h), and favorites (write this, see above)
  
  int status;
  pid_t child = fork();
  if (child == -1){
    perror("fork() failed");
    exit(1);
  } else if (child == 0) {
    int comm[2]; // child creates a pipe for itself
    int child_pipe = pipe(comm);
    if(child_pipe == -1) {
      perror("error creating child pipe");
      exit(-1);
    }
    run_control();
  } else {
    wait(&status);
  }
  // Fork controller
  // Child creates a pipe for itself comm[0]
  // then calls run_control ()
  // Parent waits ...
}
