#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
  fork() a child and print a message from the parent and 
  a message from the child
*/

int main( void )
{
  pid_t pid = fork();

  if( pid == -1 )
  {
    // When fork() returns -1, an error happened.
    perror("fork failed: ");
    exit( EXIT_FAILURE );
  }
  else if ( pid == 0 )
  {
    // When fork() returns 0, we are in the child process.
    printf("Hello from the child process\n");
    fflush(NULL);
    exit( EXIT_SUCCESS );
  }
  else 
  {
    // When fork() returns a positive number, we are in the parent
    // process and the return value is the PID of the newly created
    // child process.
    int status;
    waitpid(pid, &status, 0 );
    printf("Hello from the parent process\n");
    fflush( NULL );
  }
  return EXIT_SUCCESS;

printf("\n");

}
