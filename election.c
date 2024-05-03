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
#include <stdbool.h>

typedef struct
{
    int id;
    bool eligible;
} Voter;

typedef struct
{
    int eligible;
    int non_eligible;
} VotersResult;

void handler()
{
    printf("Signal arrived!\n");
}

bool simulate_error()
{
    return rand() % 10 > 1;
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

    char pipename[20];
    sprintf(pipename, "/tmp/%d", getpid());
    int fid = mkfifo(pipename, S_IRUSR | S_IWUSR);

    if (fid == -1)
    {
        perror("named pipe error");
        exit(1);
    }

    int fd;

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

            fd = open(pipename, O_RDONLY);

            VotersResult voters_result;
            read(fd, &voters_result, sizeof(VotersResult));
            close(fd);
            FILE *file = fopen("result.txt", "w");
            if (file == NULL)
            {
                perror("error writing file");
                exit(1);
            }

            fprintf(file, "Eligible to elect: %d\n", voters_result.eligible);
            fprintf(file, "Not eligible to elect: %d\n", voters_result.non_eligible);
            int sum = voters_result.non_eligible + voters_result.eligible;
            fprintf(file, "Eligible to elect ratio: %2.f%%\n", voters_result.non_eligible == 0 ? 100 : (double)voters_result.eligible / sum * 100);
            fprintf(file, "Not eligible to elect ratio: %2.f%%", voters_result.eligible == 0 ? 100 : (double)voters_result.non_eligible / sum * 100);

            //...print to stdout

            fclose(file);

            int status;
            waitpid(child1, &status, 0);
            waitpid(child2, &status, 0);
        }
        else
        {
            // child1 => adatellenőrző
            sleep(2);
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

            Voter *voters = (Voter *)malloc(count * sizeof(Voter));
            for (int i = 0; i < count; i++)
            {
                voters[i].id = ids[i];
                voters[i].eligible = simulate_error();
            }

            write(pipefd[1], &count, sizeof(int));
            write(pipefd[1], voters, count * sizeof(Voter));

            kill(child2, SIGUSR1);
            free(voters);
            free(ids);
        }
    }
    else
    {
        // child2 => pecsételő
        sleep(1);
        kill(getppid(), SIGUSR1);
        pause();
        int count;
        read(pipefd[0], &count, sizeof(int));
        Voter *voters = (Voter *)malloc(count * sizeof(Voter));
        read(pipefd[0], voters, count * sizeof(Voter));

        VotersResult voters_result = {0, 0};
        for (int i = 0; i < count; i++)
        {
            printf("id: %d => %s\n", voters[i].id, voters[i].eligible ? "true" : "false");
            voters[i].eligible ? voters_result.eligible++ : voters_result.non_eligible++;
        }
        fd = open(pipename, O_WRONLY);
        write(fd, &voters_result, sizeof(VotersResult));
        close(fd);
        free(voters);
    }

    return 0;
}