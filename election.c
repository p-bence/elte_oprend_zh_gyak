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

#define NUMBER_OF_PARTIES 6

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

int get_random_int(int min, int max)
{
    return min + rand() % (max - min + 1);
}

void announce_winner_party(int *votes, int length)
{
    int *results = (int *)malloc(NUMBER_OF_PARTIES * sizeof(int));
    for (int i = 0; i < NUMBER_OF_PARTIES; i++)
    {
        results[i] = 0;
    }

    for (int i = 0; i < length; i++)
    {
        results[votes[i] - 1]++;
    }

    if (length == 0)
    {
        printf("No valid votes: election is draw.\n");
        return;
    }

    int winner_index = 0;

    for (int i = 0; i < NUMBER_OF_PARTIES; i++)
    {
        if (results[i] > results[winner_index])
        {
            winner_index = i;
        }
    }

    printf("The winner is: %d \n", winner_index + 1);

    free(results);
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

    char *mqname = "/siwib4_mq";
    struct mq_attr attr;
    mqd_t mq1;
    attr.mq_maxmsg = atoi(argv[1]) + 1;
    attr.mq_msgsize = sizeof(int);
    //
    mq_unlink(mqname);
    mq1 = mq_open(mqname, O_CREAT | O_RDWR, 0600, &attr);

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

            pause();
            int eligible_to_vote;
            mq_receive(mq1, (char *)&eligible_to_vote, sizeof(int), 0);

            int *votes = (int *)malloc(eligible_to_vote * sizeof(int));
            for (int i = 0; i < eligible_to_vote; i++)
            {
                int vote_result;
                mq_receive(mq1, (char *)&vote_result, sizeof(int), 0);
                votes[i] = vote_result;
                printf("%i\n", vote_result);
            }

            announce_winner_party(votes, eligible_to_vote);

            free(votes);
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

        mq_send(mq1, (char *)&voters_result.eligible, sizeof(int), 5);
        for (int i = 0; i < voters_result.eligible; i++)
        {
            int random_party = get_random_int(1, NUMBER_OF_PARTIES);
            mq_send(mq1, (char *)&random_party, sizeof(int), 5);
        }
        sleep(1);
        kill(getppid(), SIGUSR1);

        mq_close(mq1);
        close(fd);
        free(voters);
    }

    return 0;
}