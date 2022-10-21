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
  int free; // 0 = false, 1 = true
  int pid; // may or may not be useful
} tab_list;

// Tab bookkeeping
tab_list TABS[MAX_TABS];



/************************/
/* Simple tab functions */
/************************/

// return total number of used tabs
int get_num_tabs () {
  int count = 0;
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
  for (int i = 1; i < MAX_TABS; i++) {           // looks for TABS[i].free == 1
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
  TABS[0].free = 1;
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
  FILE *fp = fopen(".favorites", "w+");
  printf("in update_favs\n");
  // Add uri to favorites file
  fprintf(fp, "%s", uri);
  // Update favorites array with the new favorite
  strcpy(favorites[num_fav++], uri);
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
  int nFlags;
  
  if ((nFlags = fcntl(fd, F_GETFL, 0)) < 0)
    return -1;
  if ((fcntl(fd, F_SETFL, nFlags | O_NONBLOCK)) < 0)
    return -1;
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
  req_t req;
  strcpy(req.uri, uri);
  req.type = NEW_URI_ENTERED;
  req.tab_index = tab_index;


  // Writes the uri to the tab indicated, for rendering once sent through the pipe
  write(comm[tab_index].inbound[1], &req, sizeof(req_t));         
}


// A URI has been typed in, and the associated tab index is determined
// If everything checks out, a NEW_URI_ENTERED command is sent (see Hint)
// Short function
void uri_entered_cb (GtkWidget* entry, gpointer data) {
  if(data == NULL) {	
    return;
  }

  // Get the tab (hint: wrapper.h)
  int cur_tab = query_tab_id_for_request(entry, data); 

  // Get the URL (hint: wrapper.h)
  char *uri = get_entered_uri(entry);
  // Hint: now you are ready to handle_the_uri
  handle_uri(uri, cur_tab);
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
  if (get_num_tabs() >= MAX_TABS) {           // counts tabs using get_num_tabs
    alert("MAX TABS REACHED.");
    return;
  }

  // Get a free tab
  int index = get_free_tab();             // uses get_free_tab to get index for new tab


  // Create communication pipes for this tab 
  pipe(comm[index].inbound);
  pipe(comm[index].outbound);

  // Make the read ends non-blocking
  non_block_pipe(comm[index].inbound[0]);
  non_block_pipe(comm[index].outbound[0]);

  // fork and create new render tab
  pid_t tab_process = fork();

  if(tab_process == -1) { //error checking
    perror("fork() failed");
    exit(1);
  } else if (tab_process == 0) { // Child process that runs the newly created tab
      char arg1[4];
      char arg2[20];
      sprintf(arg1, "%d", index);
      sprintf(arg2, "%d %d %d %d", comm[index].inbound[0], comm[index].inbound[1], comm[index].outbound[0], comm[index].outbound[1]);
      execl("./render", (char*)"render", arg1, arg2, (char*)NULL);
  } else { // Parent process, updates TABS struct
     TABS[index].free = 0;
     TABS[index].pid = tab_process;
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
      nRead = read(comm[i].outbound[0], &req, sizeof(req_t));
      // if(nRead == -1 && errno == EAGAIN) {
      //   alert("Failed to read outbound pipe");
      // }

      // Check that nRead returned something before handling cases

      // // Case 1: PLEASE_DIE
      // if (strcmp(nRead, "PLEASE_DIE") == 0) {
        
      // }
      // // Case 2: TAB_IS_DEAD
      // if (strcmp(nRead, "TAB_IS_DEAD") == 0) {
        
      // }
      // Case 3: IS_FAV
      if (req.type == 1) {
        if (fav_ok(req.uri) == 0) {
          update_favorites_file(req.uri);
          add_uri_to_favorite_menu(b_window, req.uri);
        }
      }
    }
    usleep(1000);
  }
  return 0;
}


int main(int argc, char **argv)
{
  if(argc != 1) {
    fprintf(stderr, "browser <no_args>\n");
    exit(0);
  }
  init_tabs ();
  init_blacklist(".blacklist");
  init_favorites(".favorites");

  // init blacklist (see util.h), and favorites (write this, see above)
  
  int status;
  pid_t child = fork();
  if (child < 0){
    perror("fork() failed");
    exit(1);
  } else if (child == 0) {
    // child creates a pipe for itself
    pipe(comm[0].outbound);
    run_control();

  } else {
    wait(&status);
    exit(0);
  }
  // Fork controller
  // Child creates a pipe for itself comm[0]
  // then calls run_control ()
  // Parent waits ...
}