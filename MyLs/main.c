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
#include <time.h>
#include <pwd.h>
#include <grp.h>


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
    {"dim", no_argument, 0, 'd'},
    {"all", no_argument, 0, 'a'},
    {"long", no_argument, 0, 'l'},
    {"inode", no_argument, 0, 'i'},
    {"numeric", no_argument, 0, 'n'},
    {"recursive", no_argument, 0, 'R'},
    {0, 0, 0, 0}
  };

struct flags_in_line {
  bool no_flag;
  bool dim_flag;
  bool all_flag;
  bool long_flag;
  bool inode;
  bool numeric;
  bool recursive;
};

void printdir(struct flags_in_line* status, DIR* d, char* name, bool to_print_header);
int fill_dir_array_and_print_files(struct flags_in_line* status, char* argv[], int argc, int* dir_array);
void print_long(struct stat* st, char* name, struct flags_in_line* status);
void print_access_rights(unsigned int mode); 
int count_blocks(DIR* d, char* name);

int main(int argc, char* argv[]) {
  int opt;
  int option_index = 0;
  struct flags_in_line status = {};

  while ((opt = getopt_long(argc, argv, "alindR", long_options, &option_index)) != -1) {
    switch (opt) {

    case 'd': status.dim_flag = true;
    break;

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
  
    if (!status.dim_flag) printdir(&status, d, ".", false);
    else printf(".\n");

    closedir(d);
    return 0;
  }

  int dir_array[258];
  int dir_num = fill_dir_array_and_print_files(&status, argv, argc, dir_array);

  //printf("dir_num %d\n argv[dir_num] %s", dir_num, argv[dir_num]);
  for (int i = 0; i < dir_num; i++) {
    DIR* d = opendir(argv[dir_array[i]]);
    if (d == NULL) 
      perror("mistake in opendir");
    
    if (dir_num == 1)
      printdir(&status, d, argv[dir_array[i]], false);
    else
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
  struct stat dir_stat = {};
  stat(name, &dir_stat);

  if (status->recursive || to_print_header) printf("%s:\n", name);
  if (status->long_flag || status->numeric) {
    printf("total %d\n", count_blocks(d, name));
    closedir(d);
    opendir(name);
  }

  struct dirent* e;
  while((e = readdir(d))!= NULL) {
    snprintf(buffer, sizeof(buffer), "%s/%s", name, e->d_name);
    struct stat st;
    stat(buffer, &st);
    
    if(e->d_name[0] != '.' && !status->dim_flag) {
      if (S_ISDIR(st.st_mode)) {
        directories_index[index] = e; index++;
      }
      if (status->inode) 
        printf("%lu ", e->d_ino);

      if (status->long_flag || status->numeric) {
        print_long(&st, e->d_name, status);
        //if (i < argc-1)
        printf("\n");
      }
      else {
        printf("%s  ", e->d_name);
      }
    }
    else if (status->all_flag) {
      if (status->inode) 
        printf("%lu ", e->d_ino);
      printf("%s  ", e->d_name);
    }
  }
  
  if (!(status->long_flag || status->numeric)) printf("\n");
  
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

int fill_dir_array_and_print_files(struct flags_in_line* status, char* argv[], int argc, int* dir_array) {
  int index = 0;
  int printed = 0;
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
        if (status->inode) 
          printf("%lu ", st.st_ino);
        
        if (status->long_flag || status->numeric) {
          print_long(&st, argv[i], status);
          if (i < argc-1)
            printf("\n");
        }
        else {
          printf("%s  ", argv[i]);
        }
        printed++;
      }
    }
  }

  if (printed != 0)
  {
    printf("\n");
    if (index != 0)
      printf("\n");
  }

  return index;
}

void print_long(struct stat* st, char* name, struct flags_in_line* status) {
 
  struct passwd* st_username = getpwuid(st->st_uid);
  struct group*  st_group    = getgrgid(st->st_gid);
  print_access_rights(st->st_mode); 
  //printf("%u ", st->st_mode);
  printf("%lu ", st->st_nlink);
  if (status->numeric) {
    printf("%u ", st->st_uid);
    printf("%u ", st->st_gid);
    
  }
  else {
    printf("%s ", st_username->pw_name);
    printf("%s ", st_group->gr_name);
  }
  
  printf("%lu ", st->st_size);
  char *modify_time_str = ctime(&st->st_mtime);
  modify_time_str[strlen(modify_time_str)-1] = '\0';
  printf("%s ", modify_time_str);
  printf("%s ", name);
}

void print_access_rights(unsigned int mode) {
  char str[10];
  str[0] =  (S_ISDIR(mode))  ? 'd' :
            (S_ISCHR(mode))  ? 'c' :
            (S_ISBLK(mode))  ? 'b' :
            (S_ISFIFO(mode)) ? 'p' :
            (S_ISLNK(mode))  ? 'l' : '-';

  // Right of user
    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    
    // SUID
    if (mode & S_ISUID) {
        str[3] = (mode & S_IXUSR) ? 's' : 'S';
    } else {
        str[3] = (mode & S_IXUSR) ? 'x' : '-';
    }
    
    // Rigths of group
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    
    // SGID 
    if (mode & S_ISGID) {
        str[6] = (mode & S_IXGRP) ? 's' : 'S';
    } else {
        str[6] = (mode & S_IXGRP) ? 'x' : '-';
    }
    
    // Other right
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    
  
    // Sticky bit  
    if (mode & 01000) {
        str[9] = (mode & S_IXOTH) ? 't' : 'T';
    } else {
        str[9] = (mode & S_IXOTH) ? 'x' : '-';
    }
    
    str[10] = '\0';
          
  printf("%s ", str);
}

int count_blocks(DIR* d, char* name) {
  char buffer[512] = {0};
  int blocks = 0;
  struct dirent* e;
  while((e = readdir(d))!= NULL) {
   
    if (e->d_name[0] == '.') {
      continue;
    }
    snprintf(buffer, sizeof(buffer), "%s/%s", name, e->d_name);
    struct stat st = {0};
    if (stat(buffer, &st) < 0) perror("stat: mistake");
    blocks += st.st_blocks;
  }
  
  return blocks;
}