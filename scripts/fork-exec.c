#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(int argc, char **argv) {

	pid_t parent = getpid();
	pid_t pid = fork();


	if (pid == -1)
	{
		// error, failed to fork()
	}
	else if (pid > 0)
	{
      printf("Parent: %d\n", parent);
		int status;
		waitpid(pid, &status, 0);
      printf("Child Exited: %d\n", getpid());
	}
	else
	{
      printf("Child: %d\n", getpid());
		char *args[] = {"raspivid", "-w", "1280", "-h", "720", "-fps", "25", "-vf",
         "-t", "86400000", "-b", "1800000", "-o", "live.h264", NULL};
      //char *args[] = {"ls", "-l", NULL};
      // execvp( "ls", args);
      execvp( "raspivid", args);
      printf("Child Exiting: %d\n", getpid());
		exit(EXIT_FAILURE);   // exec never returns
	}

	return 0;
}
