#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h> 

#include <termios.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

// --- ANSI escape code macros ---
#define CUR_UP(n)      "\x1b[" #n "A"      // fixed-N version (compile-time)
#define CLR_LINE       "\x1b[2K"           // clear entire current line
#define CUR_UP_N       "\x1b[%dA"          // printf-style, for runtime N
#define HIDE_CURSOR    "\x1b[?25l"
#define SHOW_CURSOR    "\x1b[?25h"

// Reset
#define RESET   "\033[0m"

// Regular colors
#define BLACK   "\033[30m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"

// Bold
#define BOLD_BLACK   "\033[1;30m"
#define BOLD_RED     "\033[1;31m"
#define BOLD_GREEN   "\033[1;32m"
#define BOLD_YELLOW  "\033[1;33m"
#define BOLD_BLUE    "\033[1;34m"
#define BOLD_MAGENTA "\033[1;35m"
#define BOLD_CYAN    "\033[1;36m"
#define BOLD_WHITE   "\033[1;37m"

// Bright
#define BRIGHT_BLACK   "\033[90m"
#define BRIGHT_RED     "\033[91m"
#define BRIGHT_GREEN   "\033[92m"
#define BRIGHT_YELLOW  "\033[93m"
#define BRIGHT_BLUE    "\033[94m"
#define BRIGHT_MAGENTA "\033[95m"
#define BRIGHT_CYAN    "\033[96m"
#define BRIGHT_WHITE   "\033[97m"

struct termios orig;


void raw_off(void) {
  tcsetattr(STDIN_FILENO, TCSANOW, &orig);
  fprintf(stderr, SHOW_CURSOR);
  fflush(stderr);
}


void raw_on(void) {
  tcgetattr(STDIN_FILENO, &orig);
  struct termios raw = orig;
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSANOW, &raw);
  fprintf(stderr, HIDE_CURSOR);
  atexit(raw_off);
}


int collect_directories(const char *dir_path, char ***out_array) {
  DIR *dir = opendir(dir_path);
  if (!dir) {
    return -1; 
  }
  
  struct dirent *entry;
  char **arr = NULL;
  int count = 0;
  int capacity = 0;
  
  while ((entry = readdir(dir)) != NULL) {
    // Skip . and .. shortcuts
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    
    // 1. Build the full path to check its status
    // Add extra space for the slash '/' and null-terminator '\0'
    size_t path_len = strlen(dir_path) + strlen(entry->d_name) + 2;
    char *full_path = malloc(path_len);
    if (!full_path) {
      perror("Path allocation failed");
      break;
    }
    snprintf(full_path, path_len, "%s/%s", dir_path, entry->d_name);
    
    // 2. Fetch file details using stat
    struct stat path_stat;
    int is_directory = 0;
    if (stat(full_path, &path_stat) == 0) {
      if (S_ISDIR(path_stat.st_mode)) {
        is_directory = 1;
      }
    }
    
    // Free the temporary path string immediately after stat check
    free(full_path);
    
    // 3. If it is a directory, collect it
    if (is_directory) {
      // Grow array capacity if needed (doubling strategy)
      if (count >= capacity) {
        capacity = capacity == 0 ? 4 : capacity * 2;
        char **temp = realloc(arr, capacity * sizeof(char *));
        if (!temp) {
          perror("Array allocation failed");
          for (int i = 0; i < count; i++) free(arr[i]);
          free(arr);
          closedir(dir);
          return -1;
        }
        arr = temp;
      }
      
      // Duplicate the directory name string and store it
      arr[count] = strdup(entry->d_name);
      count++;
    }
  }
  
  closedir(dir);
  *out_array = arr; // Pass the allocated array back to the caller
  return count;     // Return how many items were stored
}


void print_help(const char *name) {
  printf(BOLD_WHITE "Usage: " RESET " %s <directory>\n", name);
  printf("\t Opens folder picker within passsed <directory>.\n"); 
}


void print_prompt(char ***items, int n, int sel, bool first) {
  if (!first) {
    fprintf(stderr, CUR_UP_N, n + 2);   // move cursor up n lines, + 2 for quit message at the end
  }
  
  for (int i = 0; i < n; i++) {
    fprintf(stderr, CLR_LINE "%s %s%s%s\n",
      i == sel ? BOLD_RED ">" RESET : " ",
      i == sel ? BOLD_MAGENTA : BRIGHT_BLACK,
      (*items)[i],
      RESET);
  }
  
  fprintf(stderr, "\nPress 'q' to stay in current directory.\n");
  fflush(stderr);
}


void clear_prompt(int n) {
  fprintf(stderr, CUR_UP_N, n + 2);   // move back up to the top of the drawn menu
  for (int i = 0; i < n + 2; i++) {
    fprintf(stderr, CLR_LINE "\n");
  }
  fprintf(stderr, CUR_UP_N, n + 2);   // move back up over the now-blank lines
  fflush(stderr);
}


int main(int argc, char const **argv) {
  if (argc != 2) {
    print_help(argv[0]);
    return 1;
  }

  char **items = NULL;
  int n = collect_directories(argv[1], &items), sel = 0;

  if (n < 0) { perror("opendir"); return 1; }
  if (n == 0) { // directory with no folders
    printf("No subfolders.\n");
    return 1;
  }
  
  raw_on();
  print_prompt(&items, n, sel, true);

  while (1) {
    int c = getchar();
    if (c == EOF) break;     // stdin closed / terminal gone — bail out

    if (c == '\x1b') {
      c = getchar();         // skip '['
      if (c == EOF) break;   // stdin closed / terminal gone — bail out

      int dir = getchar();
      if (dir == EOF) break; // stdin closed / terminal gone — bail out

      if (dir == 'A') sel = (sel - 1 + n) % n;
      if (dir == 'B') sel = (sel + 1) % n;
    } else if (c == '\r' || c == '\n') {
      break;
    } else if (c == 'q') {
      clear_prompt(n);
      return 0;
    }

    print_prompt(&items, n, sel, false);
  }

  clear_prompt(n);
  printf("%s/%s\n", argv[1], items[sel]);
  return 0;
}