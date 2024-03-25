/*
 * This app starts a very simple shell and executes some simple commands.
 * The commands are stored in the hostfs_root/shellrc
 * The shell loads the file and executes the command line by line.                 
 */
#include "user_lib.h"
#include "string.h"
#include "util/types.h"

int main(int argc, char *argv[]) {
  printu("\n======== Shell Start ========\n\n");
  
  int MAXBUF = 1024;
  char buf[MAXBUF];
  char para[128];
  //gets(s);
  char pathname[MAXBUF];
  //gets(buf);
  while(1)
  {
    printu("执行程序\n");
    gets(buf); 
    if(strcmp("exit", buf) == 0)
      break;
    else if (strcmp("cd", buf) == 0)
    {
      printu("相对路径\n");
      gets(para);
      change_cwd(para);
      continue;
    }
    else
    {
      printu("\n==========Command Start============\n");
      int pid = fork();
      if (pid == 0)
      {
        int ret = exec(buf,para);
        if (ret == -1)
          printu("exec failed!\n");
      }
      else
      {
        wait(pid);
        printu("==========Command End============\n\n");
      }
    }
  }
  exit(0);
  return 0;
}
 
 

