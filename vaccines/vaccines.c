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

void signal_handler()
{
    printf("Signal megérkezett!\n");
}

int main(int argc, char **argv)
{
    int patients = atoi(argv[1]);

    pid_t child1, child2;

    signal(SIGUSR1, signal_handler);

    char *pipe1 = "siwbi_4_1_c";
    char *pipe2 = "siwbi_4_2_c";

    mkfifo(pipe1, S_IRUSR | S_IWUSR);
    mkfifo(pipe2, S_IRUSR | S_IWUSR);

    child1 = fork();
    if (child1 > 0)
    {
        child2 = fork();
        if (child2 > 0)
        { // parent => Dr. Bubó
            int p1id = open(pipe1, O_WRONLY);
            int p2id = open(pipe2, O_WRONLY);

            pause();
            pause();

            int c1_patiens = patients / 2;
            int c2_patiens = patients - c1_patiens;

            write(p1id, &c1_patiens, sizeof(int));
            write(p2id, &c2_patiens, sizeof(int));

            wait(NULL);
            wait(NULL);
            printf("Dr. Bubó befejezte.\n");

            unlink(pipe1);
            unlink(pipe2);
        }
        else
        { // child2 =>  Csőrmester
            int p2id = open(pipe2, O_RDONLY);

            sleep(1);
            kill(getppid(), SIGUSR1);

            int patientCount;
            read(p2id, (char *)&patientCount, sizeof(int));
            printf("Csőrmester: beoltandó páciensek: %i\n", patientCount);

            printf("Csőrmester befejezte.\n");
        }
    }
    else
    { // children1 => Ursula
        int p1id = open(pipe1, O_RDONLY);

        sleep(2);
        kill(getppid(), SIGUSR1);

        int patientCount;
        read(p1id, (char *)&patientCount, sizeof(int));
        printf("Ursula: beoltandó páciensek: %i\n", patientCount);

        printf("Ursula befejezte.\n");
    }
    return 0;
}