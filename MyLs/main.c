#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h> 
#include <errno.h>

extern char *optarg;
extern int optind, opterr, optopt;

DIR* opendir_safe(DIR* d, const char* name) {
  if((d = opendir(name)) == NULL) {
    perror("error in open dir");
    exit(1);
  }

  return d;
}

struct option long_options[] = {
    {"all", no_argument, 0, 'a'},
    {"long", no_argument, 0, 'l'},
    {"inode", no_argument, 0, 'i'},
    {"numeric", no_argument, 0, 'n'},
    {"recursive", no_argument, 0, 'R'},
    {0, 0, 0, 0}
  };

struct flags_in_line {
  bool no_flag;
  bool all_flag;
  bool long_flag;
  bool inode;
  bool numeric;
  bool recursive;
};

void printdir(struct flags_in_line* status, DIR* d, char* name, bool to_print_header);
int fill_dir_array_and_print_files(char* argv[], int argc, int* dir_array);

int main(int argc, char* argv[])
{
  int opt;
  int option_index = 0;
  struct flags_in_line status = {};

  while ((opt = getopt_long(argc, argv, "alinR", long_options, &option_index)) != -1) {
    switch (opt) {

    case 'a': status.all_flag = true;
    break;

    case 'R': status.recursive = true;
    break;

    case 'l': status.long_flag = true;
    break;

    case 'i': status.inode = true;
    break;

    case 'n': status.numeric = true;
    break;

    default:
      return 0;
    }
  }

  //printf("optind = %d\n", optind);
  if (argc-optind == 0) {
    DIR* d = opendir(".");
    if (d == NULL) perror("mistake in opendir");
  
    printdir(&status, d, ".", false);
    closedir(d);
    return 0;
  }

  int dir_array[258];
  int dir_num = fill_dir_array_and_print_files(argv, argc, dir_array);
  if (dir_num != 0)
    printf("\n\n");

  //printf("dir_num %d\n argv[dir_num] %s", dir_num, argv[dir_num]);
  for (int i = 0; i < dir_num; i++) {
    DIR* d = opendir(argv[dir_array[i]]);
    if (d == NULL) 
      perror("mistake in opendir");
    
    printdir(&status, d, argv[dir_array[i]], true);
    if (i != dir_num-1) printf("\n");
  }

  return 0;
}


//*********************************
//------------functions-----------
//*********************************

void printdir(struct flags_in_line* status, DIR* d, char* name, bool to_print_header) {
  struct dirent* directories_index[256];
  char buffer[1024] = {0};
  size_t index = 0;

  if (status->recursive || to_print_header) printf("%s:\n", name);

  struct dirent* e;
  while((e = readdir(d))!= NULL) {
    snprintf(buffer, sizeof(buffer), "%s/%s", name, e->d_name);
    struct stat st;
    stat(buffer, &st);
    
    if(e->d_name[0] != '.') {
      if (S_ISDIR(st.st_mode)) {
        directories_index[index] = e; index++;
      }
      if (status->inode) 
        printf("%lu ", e->d_ino);
      printf("%s  ", e->d_name);
    }

    else if (status->all_flag) {
      if (status->inode) 
        printf("%lu ", e->d_ino);
      printf("%s  ", e->d_name);
    }
  }
  
  printf("\n");
  
  if (status->recursive) {
    if (index != 0)
      printf("\n");
    for (size_t j = 0; j < index; j++) {
      char buffer[1024];
      snprintf(buffer, sizeof(buffer), "%s/%s", name, directories_index[j]->d_name);
      DIR* subdir = opendir(buffer);
      if(subdir != NULL) {
        printdir(status, subdir, buffer, false);
        closedir(subdir);
        if (j != index-1) printf("\n");
      }
    }
  }
}

int fill_dir_array_and_print_files(char* argv[], int argc, int* dir_array) {
  int index = 0;
  for (int i = optind; i < argc; i++) {
    //printf("%s\n", argv[i]);
    struct stat st;
    bool is_good = true;
    if(stat(argv[i], &st) == -1) {
      fprintf(stderr, "myls: cannot access '%s': %s\n", argv[i], strerror(errno));
      is_good = false;
    }
    else if (is_good) {
      if (S_ISDIR(st.st_mode)) {
        dir_array[index++] = i;
      } 
      else {
        printf("%s  ", argv[i]);
      }
    }
  }

  return index;
}