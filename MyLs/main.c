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
    {"long", no_argument, 0, 'l'},
    {"inode", no_argument, 0, 'i'},
    {"numeric", no_argument, 0, 'n'},
    {"recursive", no_argument, 0, 'R'},
    {0, 0, 0, 0}
  };

struct flags_in_line {
  bool no_flag;
  bool long_flag;
  bool inode;
  bool numeric;
  bool recursive;
};

void printdir(struct flags_in_line* status, DIR* d, char* name);

int main(int argc, char* argv[])
{
  int opt;
  int option_index = 0;
  struct flags_in_line status = {};

  while ((opt = getopt_long(argc, argv, "linR", long_options, &option_index)) != -1) {
    switch (opt) {

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

  printf("optind = %d\n", optind);
  if (argc-optind == 0) {
    DIR* d = opendir(".");
    if (d == NULL) perror("mistake");
  
    printdir(&status, d, ".");
    closedir(d);
    return 0;
  }

 
  for (int i = 1; i < argc-optind+1; i++) {
    printf("%s ", argv[i]);
  }
  printf("\n");


  return 0;
}



void printdir(struct flags_in_line* status, DIR* d, char* name) {
  struct dirent* directories_index[256];
  size_t index = 0;

  printf("%s:\n", name);

  struct dirent* e;
  while((e = readdir(d))!= NULL) {
    struct stat st;
    stat(e->d_name, &st);
    
    if(e->d_name[0] != '.')
    {
      if (S_ISDIR(st.st_mode)) {
        directories_index[index] = e; index++;
      }

      printf("%s ", e->d_name);
    }
  }
  
  printf("\n");
  for (size_t j = 0; j < index; j++)
    printf("%s\n", directories_index[j]->d_name);
  
  if (status->recursive) {
    for (size_t j = 0; j < index; j++) {
      DIR* d = opendir(directories_index[j]->d_name);
      printdir(status, d, directories_index[j]->d_name);
      closedir(d);
    }
  }
}