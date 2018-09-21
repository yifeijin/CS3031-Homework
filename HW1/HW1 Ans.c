/* @File doit.c
 * @Date 09/01/15
 */
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

#define MAX_COMMAND 128
#define MAX_ARGUMENT 32

 void printStatInformation(long start_time, int pid, int status){
    struct rusage child_usage;
    struct timeval now_time;
    gettimeofday(&now_time, NULL);
    getrusage(RUSAGE_CHILDREN, &child_usage);

    printf("The CPU time used in user mode is %ld ms.\n", (child_usage.ru_utime.tv_sec * 1000) + (child_usage.ru_utime.tv_usec / 1000));
    printf("The CPU time used in system is %ld ms.\n",(child_usage.ru_stime.tv_sec * 1000) + (child_usage.ru_stime.tv_usec / 1000));
    printf("The elapsed clock time is %ld ms.\n", (now_time.tv_sec * 1000) + (now_time.tv_usec / 1000) - start_time);
    printf("The process was preempted involuntarily %ld times.\n", child_usage.ru_nivcsw);
    printf("The process gave up the CPU %ld times.\n", child_usage.ru_nvcsw);
    printf("The number Of major page faults is %ld.\n", child_usage.ru_majflt);
    printf("The number Of minor page faults is %ld.\n", child_usage.ru_minflt);
 }

int runWithCommand(char **argv){
    int child_pid;

    if ((child_pid = fork()) <0){
        fprintf(stderr,"fork() failed.\n");
        exit(1);
    }else if (child_pid == 0){
        if (execvp(argv[0], argv) < 0){
            fprintf(stderr, "exeve() failed\n");
            exit(1);
        }
    }else{
        struct timeval start;
        gettimeofday(&start, NULL);
        long start_time = ((start.tv_sec * 1000) + (start.tv_usec/1000));
        int ret;
        waitpid(child_pid, &ret, 0);//wait child process exit
        printStatInformation(start_time, child_pid, ret);

    }
    return 0;
}

void loop(){
    char c;
    char buffer[200];
    int i = 0;
    while (1){
        //read command from stdin
        fprintf(stdout, "==>");
        while((c = getchar()) != '\n' && c != EOF){
            if (i == MAX_COMMAND){
                fprintf(stderr, "command is too long! \n");
            }else{
                buffer[i++] = c;
            }
        }
        buffer[i] = '\0';//finish reading

        if (i> MAX_COMMAND) continue;

        //parse string into argument list
        char* commands[MAX_ARGUMENT];
        const char split[2] = " ";
        int num = 0;
        char* token = strtok(buffer, split);
        commands[num] = token;
        num++;
        while (token != NULL){
            token = strtok(NULL, " ");
            if (token != NULL){
                commands[num] = token;
                num++;
            }
        }
        commands[num] = NULL; 
        //finish parsing
        if (num < 1 || num >= MAX_ARGUMENT) continue;

        if (strcmp(commands[num - 1], "&") == 0){
            commands[num - 1] = NULL;
            num--;
        }

        //run the key word
        if (strcmp(commands[0], "exit") == 0){
            printf("Exit the program.\n");
            exit(0);
        }else if (strcmp(commands[0],"cd") == 0){ 
            if (chdir(commands[1]) == -1) 
                fprintf(stderr, "Change the working directory failed.\n");
        }else{
            runWithCommand(commands);
        }
        memset(buffer, 0, i+1);
        i = 0;
    }
}

int main(int argc, char **argv)
     // argc -- number of arguments 
     // argv -- an array of strings 
{
    if (argc > 1){
    runWithCommand(argv+1);
    }else{
        loop();
    }
    return 0;
}
