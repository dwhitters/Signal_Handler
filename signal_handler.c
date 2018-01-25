/******************************************************************
*   @authors David Whitters, Jonah Bukowsky
*   @date 1/25/18
*
*   CIS 452
*   Dr. Dulimarta
******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/** The pipe index for the read file descriptor. */
#define READ 0
/** The pipe index for the write file descriptor. */
#define WRITE 1
/** The max number of characters a string is allowed to contain. */
#define MAX_NUM_CHARS 1024

/** Set to 1 when the interrupt handler was called. */
int Int_Received = 0u;

/**
    Signal handler.

    @param sig_num
        The signal to handle.
*/
void Sig_Handler(int sig_num)
{
    switch(sig_num)
    {
        case SIGUSR1:
            printf(" received a SIGUSR1 signal\n");
            break;
        case SIGUSR2:
            printf(" received a SIGUSR2 signal\n");
            break;
        case SIGINT:
            Int_Received = 1u; /* Signal that this interrupt has occurred. */
            break;
    }
}

int main(int argc, char * argv[])
{
    /* The pipe used for inter-process communication. */
    int fd[2];
    /* Used to determine parent vs child process execution. */
    pid_t pid;
    /* The status of the exited child process. */
    int status;

    /* Create the pipe. */
    if(pipe(fd) < 0)
    {
        perror("PIPE INIT FAIL\n");
        exit(EXIT_FAILURE);
    }

    /* Create a child process. */
    if((pid = fork()) < 0)
    {
        perror("FORK FAILURE\n");
        exit(EXIT_FAILURE);
    }
    else if(pid == 0) /* Child processes have PIDs of zero. */
    {
        /* Child process execution. */
        int rand_num = 0; /* The random number generated. */
        int rand_sec = 0; /* Will be set to a value of 1-5. */
        pid_t parent_pid = getppid(); /* Get the parent process ID. */
        srand(time(NULL)); /* Seed the random number generator. */
        char * msg = ""; /* Used to read the pipe data. */

        /* Make stdin an alias for the pipe. */
        dup2(fd[READ], STDIN_FILENO);

        /* Close the pipe file descriptors. stdin still reads from the pipe. */
        close(fd[READ]);
        close(fd[WRITE]);

        while(1)
        {
            rand_num = rand(); /* Get a random number between 0 and RAND_MAX. */
            rand_sec = (rand_num % 5) + 1; /* Get a value from 1 to 5. */
            sleep(rand_sec);
            if((rand_num % 2) == 0)
            {
                kill(parent_pid, SIGUSR1);
            }
            else
            {
                kill(parent_pid, SIGUSR2);
            }

            /* If data was read... */
            if(read(fd[READ], msg, 1) != 0)
            {
                /* The only time this receives data is when an interrupt occurred. */
                exit(EXIT_SUCCESS); /* Exit the child process. */
            }
        }
    }

    printf("spawned child PID# %d\n", pid);

    /* Setup the signals for the parent process to handle. */
    signal(SIGINT, Sig_Handler);
    signal(SIGUSR1, Sig_Handler);
    signal(SIGUSR2, Sig_Handler);

    close(fd[READ]); /* Not used. */

    /* Continually wait for signals to occur. */
    while(1)
    {
        printf("waiting...");
        (void)pause();

        if(Int_Received)
        {
            printf("That's it, I'm shutting you down...\n");
            /* Send the terminate character to the child process via the pipe. */
            write(fd[WRITE], "Q", 2);

            /* Wait for the child process to terminate. */
            pid = wait(&status);
            break;
        }
    }

    return 0;
}
