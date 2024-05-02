#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <mqueue.h>
#include <time.h>

void handler()
{
    printf("Signal arrived!\n");
}

int main(int argc, char **argv)
{
    srand(time(NULL));

    pid_t child1, child2;

    signal(SIGUSR1, handler);

    int pipefd[2];

    if (pipe(pipefd) == -1)
    {
        perror("pipe error");
        exit(1);
    }

    child1 = fork();

    if (child1 < 0)
    {
        perror("fork error");
        exit(1);
    }

    if (child1 > 0)
    {
        child2 = fork();
        if (child2 < 0)
        {
            perror("fork error");
            exit(1);
        }
        if (child2 > 0)
        { // parent
            pause();
            pause();
            close(pipefd[0]);

            int count = atoi(argv[1]);
            int *ids = (int *)malloc(count * sizeof(int));
            write(pipefd[1], &count, sizeof(int));
            for (int i = 0; i < count; i++)
            {
                ids[i] = rand() % 100;
            }
            write(pipefd[1], ids, count * sizeof(int));
            close(pipefd[1]);
            free(ids);

            int status;
            waitpid(child1, &status, 0);
            waitpid(child2, &status, 0);
        }
        else
        { // child2 => pecsételő
            sleep(1);
            close(pipefd[1]);
            kill(getppid(), SIGUSR1);
        }
    }
    else
    { // child1 => adatellenőrző
        sleep(2);
        close(pipefd[1]);
        kill(getppid(), SIGUSR1);

        int count;

        read(pipefd[0], &count, sizeof(int));
        int *ids = (int *)malloc(count * sizeof(int));
        read(pipefd[0], ids, count * sizeof(int));
        printf("adatellenőrző: fogadtam a sorszámokat!\n");
        for (int i = 0; i < count; i++)
        {
            printf("%d\n", ids[i]);
        }
    }

    return 0;
}