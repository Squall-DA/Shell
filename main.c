/* 
 * File:   main2.c
 * Author: Daniel huelsman
 * Email:
 * Created on February 8, 2011, 12:04 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_PROGS 5
#define MAX_ARGS 20

/* This is the structure that holds the information about a parsed
 * command line.  The argv array is an array of string vectors; in
 * other words, for some int i, argv[i] is an array of strings.
 * You should be able to use argv[i] in calls to one of the execv*()
 * functions.
 */
typedef struct {
  char *argv[MAX_PROGS + 1][MAX_ARGS + 1];
  int argc;       /* Number of argument vectors*/
  int progc;  /* Number of argument vectors; if > 1, piping is requested */
  char *infile;   /* Name of stdin redirect file; NULL if no redirection */
  char *outfile;  /* Name of stdout redirect file; NULL if no redirection */
  int bg;         /* Put command into background? */
  //pid_t pidkill;
} Command;

pid_t runproc(int fd[][2], int num, Command *cmd);
void executecommand(Command *cmd);

int main() {
    system("clear");
    char cmdstr[256], *user, *cmd;
    int cmdlength;
    user = (char *)getenv("USER");
    char cwd[256];
    while(1){
        cmdstr[0] = '\n';
        Command *command1 = (Command*) malloc(sizeof(Command));
        getcwd(cwd,sizeof(cwd));

        /* Loop until user gives an input */
        while(cmdstr[0]=='\n'){
            memset(cmdstr,0x00,sizeof(cmdstr));
            printf("%s~%s>$ ", user, cwd);
            fgets(cmdstr,sizeof(cmdstr), stdin);
        }

        cmdlength = strlen(cmdstr);
        if(cmdstr[cmdlength-1]== '\n'){
            cmdstr[cmdlength-1]= '\0';
        }

        /* Check string length*/
        if(strlen(cmdstr)==255){
            printf("String Length is the same as Buffersize.\n");
            printf("All of the commands may not be saved\n");
        }

        /*Get the first token (the actual command) from the input*/
       	cmd = strtok(cmdstr, " ");
        command1->argc = 0;
        command1->progc = 1;
        command1->argv[command1->progc-1][command1->argc] = cmd;

        while(1){
            //Check number of arguments
            if(command1->argc > 20 && command1->progc > 5){
                perror("\nError! Too Many Arguments!\n");
                break;
            }

            /*Extract next part of the command.*/
            cmd=strtok(NULL," ");
            /*Check that there are no more arguments*/
            if(cmd == NULL){
		break;
            }
            /*Check for input redirection*/
            if(strncmp(cmd,"<",1)==0){
                cmd = strtok(NULL," ");
                if(cmd == NULL){
                    printf("No Input File given\n");
                    break;
                }
                command1->infile = cmd;
                cmd = strtok(NULL, " ");
            }
            /*Check that there are no more arguments*/
            if(cmd == NULL){
		break;
            }
            /*Check for Piping*/
            if(strncmp(cmd,"|",1)==0){
                cmd=strtok(NULL," ");
                if(cmd == NULL){
                    printf("No second command given\n");
                    break;
                }
                command1->progc++;
                command1->argc = 0;
                command1->argv[command1->progc-1][command1->argc] = cmd;
                cmd=strtok(NULL," ");
            }
            /*Check that there are no more arguments*/
            if(cmd == NULL){
		break;
            }
            /*Check for output redirection*/
            if(strncmp(cmd,">",1)==0){
                cmd = strtok(NULL," ");
                if(cmd == NULL){
                    printf("No Output File given\n");
                    break;
                }
                command1->outfile = cmd;
                cmd = strtok(NULL, " ");
            }

            /*Check that there are no more arguments*/
            if(cmd == NULL){
		break;
            }
            /*Check if program should be run in background*/
            if(strncmp(cmd,"&",1)==0){
                command1->bg = 1;
                cmd = strtok(NULL, " ");
            }
            /*Check that there are no more arguments*/
            if(cmd == NULL){
		break;
            }
            /* Increase the argument count */
            command1->argc++;

            /*Add arguments to the Array*/
            command1->argv[command1->progc-1][command1->argc] = cmd;
            

        }
        /* Check to see if user wants to quit */
        if(strncmp(command1->argv[0][0],"exit",4)==0){
            exit(0);
        }
        /* check for change directory*/
        else if(strncmp(command1->argv[0][0],"cd",2)==0 && command1->argc>0){
            chdir(command1->argv[0][1]);
        }
        /* Check for a kill process command*/
        else if(strncmp(command1->argv[0][0],"kill",4)==0 && command1->argc == 1){
            kill(atoi(command1->argv[0][1]),SIGKILL);
            perror("kill()");
        }else{

            executecommand(command1);
        }
    }
        return (EXIT_SUCCESS);
}

void executecommand(Command *cmd) {
  int n;
  int temp_pipe[2];
  int fd[MAX_PROGS][2];
  pid_t pids[MAX_PROGS];

  /* Clears pipes (sets all values to -1*/
  for(n = 0; n < cmd->progc; n++){
    fd[n][0] = -1;
    fd[n][1] = -1;
  }

  /*Uses temp_pipe to connect write end of nth pipe to read end of (n+1)th
    pipe*/
  for(n = 0; n < cmd->progc - 1; n++){
    pipe(temp_pipe);
    fd[n][1] = temp_pipe[1];
    fd[n+1][0] = temp_pipe[0];
  }

  /*If input file redirection is occuring, redirects read end of first pipe to
    file*/
  if(cmd->infile){
    fd[0][0] = open(cmd->infile, O_RDONLY);
    if(fd[0][0] < 0){
      printf("Error executing command\n");
      exit(1);
    }
  }

  /*If output file redirection is occurring, redirects write end of last pipe to
    file.*/
  if(cmd->outfile){
    fd[cmd->progc - 1][1] =  fileno(fopen(cmd->outfile, "w"));
      if(fd[cmd->progc - 1][1] < 0){
        printf("Error outputting to file\n");
        exit(1);
      }
    }
  

  /*Runs runproc for every program in pipe, stores return values (pids of
    children) in array*/
  for(n = 0; n < cmd->progc; n++){
    pids[n] = runproc(fd, n, cmd);
  }

  /*Closes all pipes*/
  for(n = 0; n < cmd->progc-1; n++){
    if(fd[n][0] >= 0) close(fd[n][0]);
    if(fd[n][1] >= 0) close(fd[n][1]);
  }

  /*Waits for all children*/
  for(n = 0; n < cmd->progc; n++){
    wait(NULL);
  }
}

pid_t runproc(int fd[][2], int num, Command *cmd){
  pid_t pid;
  int n;
  int frk_chk;

  pid = fork();
  if(pid < 0){
    printf("Error executing command\n");
    exit(1);
  }else if (!pid){ /*Child code*/
    /*Redirects stdin/stdout of process to read/write end of corresponding
      pipe*/
    if(fd[num][0] >= 0) dup2(fd[num][0], STDIN_FILENO);
    if(fd[num][1] >= 0) dup2(fd[num][1], STDOUT_FILENO);

    /*Closes pipe ends*/
    for(n=0; n < cmd->progc - 1; n++){
      if(fd[n][0] >= 0) close(fd[n][0]);
      if(fd[n][1] >= 0) close(fd[n][1]);
    }

    /*If backgrounding: forks, parent exits, child executes program.
      If not backgrounding: program just executes*/
    if(cmd->bg){
        frk_chk = fork();
      if(frk_chk < 0){
          printf("Error executing command\n");
          exit(1);
      }else if(frk_chk){
          printf("PID %d \n",frk_chk);
          exit(0);
      }else{ 
          if(!(cmd->infile) && num == 0) close(STDIN_FILENO);
          execvp(cmd->argv[num][0], cmd->argv[num]);
      }
    }else{
      if(!num){
          dup2(fd[0][1], STDOUT_FILENO);
      }
     execvp(cmd->argv[num][0], cmd->argv[num]);
    }
    printf("Error executing command\n");
    exit(1);
  }else{ /*Parent code*/
    /*Returns pid of child, used for reaping loop*/
    return pid;
  }
}
