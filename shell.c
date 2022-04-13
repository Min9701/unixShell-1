#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>

/*
  Compilar con: gcc shell.c -o shell
  Ejecutar con: ./shell
*/

// these are constants (macro definition)
#define MAX_LINE 80
#define MAX_ARGS 40
#define HISTORY_PATH ".history"

int p_wait;
int in_file, out_file;
int saved_in, saved_out;
int in, out;
int pipe_ind;
int save_c;


// BREAK THE COMMANDS INTO SMALLER PARTS 
void parseInput(char *command, char **args)
{
  int args_count = 0;
  int command_len = strlen(command);
  int arg_start = -1;
  for(int i = 0; i < command_len; i++)
  {
    if(command[i] == ' ' || command[i] == '\t' || command[i] == '\n')
    {
      if(arg_start != -1)
      {
        args[args_count++] = &command[arg_start];
        arg_start = -1;
      }
      command[i] = '\0';
    }
    else
    {
      if(command[i] == '&')
      {
        p_wait = 0;
        i = command_len;
      }
      if(arg_start == -1) arg_start = i;
    }                              
  }
  args[args_count] = NULL;
  //ei: osh> mkdir ddd
  // the result:
  // args[0] == mkdir
  // args[1] == ddd
  // args[2] == NULL
}
////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////// HANDLING REDIRECTION AND PIPE //////////////////////////
void checkFlags(char **args)
{
  for(int i = 0; args[i] != NULL; i++)
  {
    if(!strcmp(args[i], ">"))
    {
      if(args[i+1] == NULL)
        printf("Invalid command format\n"); //you'll receive this message when for ex: osh> history > (means there's nothing to the right of the symbol)
      else
        out_file = i + 1;
    }
    if(!strcmp(args[i], "<"))
    {
      if(args[i+1] == NULL)
        printf("Invalid command format\n");
      else
        in_file = i + 1;
    }
    if(!strcmp(args[i], "|"))
    {
      if(args[i+1] == NULL)
        printf("Invalid command after |\n");
      else
        pipe_ind = i;
    }
  }
}
////////////////////////// END OF HANDLING REDIRECTION AND PIPE //////////////////////////////

////////////////////////// READ AND EMPTY COMMAND HISTORY //////////////////////////////
void manageHistory(char **args)
{
  FILE* h = fopen(HISTORY_PATH, "r");
  if(h == NULL)
  {
    printf("The history is empty\n");  //when the history file is not found (for ei: you deleted the history file)
  }
  else
  {

    //---------------------- display the history if there's any ------------
    if(args[1] == NULL)
    {
      char c = fgetc(h);
      while(c != EOF)
      {
        printf ("%c", c);
        c = fgetc(h);
      }
    }
    //-------------------------------------------------------------------

    //------------------- type history -c to clear the history ------------
    else if(!strcmp(args[1], "-c"))
    {
      save_c = 0;
      remove(HISTORY_PATH);
    }
    // now if you type "history", the output will be "the history is empty"
    //----------------------------------------------------------------------
    
    //-------------- if the syntax is incorrect------------------
    else
    {
      printf("[!] Invalid syntax");
    }
    fclose(h); //close the history file, bc if we don't close it, it'll be a waste of memory
  }
}
/////////////////// END OF READING AND EMPTYING COMMAND HISTORY ////////////////////////

/////////////////// EXECUTE THE COMMAND AND CHECK IF IT FAILS TO EXECUTE ////////////////////////////
void execute(char **args)
{
  if(execvp(args[0], args) < 0)
  {
    printf("[!] Command not found\n"); //if it fails to execute the command, this error message will be displayed
    exit(1);
  }
}
//////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////// CREATE A .HISTORY FILE ///////////////////////////////////////
void saveCommand(char *command)
{
  FILE* h = fopen(HISTORY_PATH, "a+");
  fprintf(h, "%s", command);
  rewind(h);  //
}
//////////////////////////////////////////////////////////////////////////////////////////


int main(void)
{
  char command[MAX_LINE];
  char last_command[MAX_LINE];
  char parse_command[MAX_LINE];
  char *args[MAX_ARGS];
  char *argsp1[MAX_ARGS], *argsp2[MAX_ARGS];
  int should_run = 1, history = 0;
  int alert;
  int pipech[2];
  //these below are defined up there. these are just reference  
  //#define MAX_LINE 80
  //#define MAX_ARGS 40
  //#define HISTORY_PATH ".history"
  
  // the program 
  while(should_run)
  {
    printf("osh> ");                   // DISPLAY THE PROMPT
    fflush(stdout);                    // CLEAR THE OUTPUT BUFFER
    /*
    it's a pre-defined function
    fflush is used to clear the output buffer. 
    read this article to understand why we need to clear the buffer: https://www.geeksforgeeks.org/use-fflushstdin-c/
    */
    
    fgets(command, MAX_LINE, stdin);      // READ THE COMMAND THE USER TYPES
    /*
    pre-defined function

    fgets is used to READ THE COMMAND THE USER TYPES
    When you type something after osh> it will read that input 
    ex : osh> abc -> abc will be read
    
    command is the array we defined in the beginning of the code
    for example: when we executes the command: osh> ls , "ls" will be passed
    in the command array.
    here, max_line is 80, means the command only has 80 characters maximum, including the null terminator
    
    read more about this : https://www.tutorialspoint.com/c_standard_library/c_function_fgets.htm
    */

    p_wait = 1;
    alert = 0;
    out_file = in_file = -1;
    pipe_ind = -1;
    save_c = 1;

    strcpy(parse_command, command);
    /*
    After we use fgets to read the command the user typed, 
    "command" array now contains that command, and strcpy copies 
    the command in "command" array to "parse_command" array
    */

    parseInput(parse_command, args);


    ////////////////////////////// HITTING ENTER WITHOUT ANY COMMANDS ////////////////////////////////////
    if(args[0] == NULL || !strcmp(args[0], "\0") || !strcmp(args[0], "\n")) continue;
    // when you hit enter without any command, the flow will be back to the 
    // beginning of the while loop
    //////////////////////////////////////////////////////////////////////////////////////////////////////


    //////////////////////////////// THE EXIT COMMAND ////////////////////////////////////////////////////
    if(!strcmp(args[0], "exit")) //if both strings are equal, 0 is returned, so we have to use ! to turn 0 into a truthy value
    {
      should_run = 0;
      continue;
    }
    /////////////////////////// END OF THE EXIT COMMAND //////////////////////////////////////////////////


    //////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////// HISTORY FEATURE //////////////////////////////////////////////////////
    
    /*
    type "!!" to display and execute the most recent command
    */
    if(!strcmp(args[0], "!!"))
    {
      if(history)
      {
        printf("%s", last_command);
        strcpy(command, last_command);
        strcpy(parse_command, command);
        parseInput(parse_command, args);
      }
      else  //if there's no command executed before
      {
        printf("No commands in history \n");
        continue;
      }
    }
    //////////////////////////// END OF HISTORY FEATURE //////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////// REDIRECTION ///////////////////////////////////////////////////////
    checkFlags(args);

    //----------------INPUT REDIRECTION---------------------
    if(in_file != -1)
    {
      in = open(args[in_file], O_RDONLY);
      if(in < 0)
      {
        printf("Failed to open file \'%s\'\n", args[in_file]);
        alert = 1;
      }
      else
      {
        saved_in = dup(0);
        dup2(in, 0);
        close(in);
        args[in_file - 1] = NULL;
      }
    }

    //----------------OUTPUT REDIRECTION-------------------
    if(out_file != -1)
    {
      out = open(args[out_file], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
      if(out < 0)
      {
        printf("Failed to open file \'%s\'\n", args[out_file]);
        alert = 1;
      }
      else
      {
        saved_out = dup(1);
        dup2(out, 1);
        close(out);
        args[out_file - 1] = NULL;
      }
    }
    ///////////////////////////// END OF REDIRECTION //////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////


    ////////////////////////////////// MANAGE PIPE ////////////////////////////////////////////////////////////////////
    // EI: ls -1
    /*
    argsp1[0] = ls
    argsp1[1] = -1
    argsp1[2] = NULL

    args[3] = |

    argsp2[0] == args[4] == wc
    argsp2[1] = NULL;
    */
    if(pipe_ind != -1)
    {
      int i = 0;
      for(; i < pipe_ind; i++) argsp1[i] = args[i];
      argsp1[i] = NULL;
      i++;
      for(; args[i] != NULL; i++) argsp2[i-pipe_ind-1] = args[i];
      argsp2[i] = NULL;
    }
    
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////// HISTORY MANAGEMENT ///////////////////////////////////////////////////
    
    if(!alert && should_run)
    {
      if(!strcmp(args[0], "history")) manageHistory(args);
      else
      {

        //STOP or CONTINUE A PROCESS
        /* type: ps -a to show the process ids, and then run stop "a certain process id" to stop that process*/
        if(!strcmp(args[0], "stop") || !strcmp(args[0], "continue"))
        {
          args[2] = args[1];
          args[1] = strcmp(args[0], "stop") ? "-SIGCONT" /*if the user types continue*/: "-SIGSTOP" /*if you type stop*/;
          args[0] = "kill";
          args[3] = NULL;
        }


        //---------------PIPE -----------------
        if(fork() == 0)
        {

          if(pipe_ind != -1)
          {
            pipe(pipech);
            if(fork() == 0)
            {

              //FIRST COMMAND ( TO THE RIGHTT OF THE PIPE SYMBOL)
              saved_out = dup(1);
              dup2(pipech[1], 1);
              close(pipech[0]);
              execute(argsp1);
            }
            else
            {
              //SECOND 
              wait(NULL);
              saved_in = dup(0);
              dup2(pipech[0], 0);
              close(pipech[1]);
              execute(argsp2);
            }
          }
        ////////////// end of pipe ///////////////////////
          else
            execute(args);
            //WHERE EVERY OTHER COMMAND GETS EXECUTED (FOR EX: ls) except pipes

        }
        else
        {
          if(p_wait) wait(NULL); //wait for the child process to finish
        }
      }

      /// history management
      strcpy(last_command, command);
      if(save_c) saveCommand(command); //save the command in the history file
      history = 1;
      // end of history management
    }

    //set everything's back to normal after the pipe: 
    dup2(saved_out, 1); //set the stdout back to 1
    dup2(saved_in, 0);  //set the stdin back to 0
  }
  return 0;
}
  