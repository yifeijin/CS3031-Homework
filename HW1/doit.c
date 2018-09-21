#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ARG 32
#define MAX_COMMAND 128

void printStats(long start, int pid, int state)
{
	struct rusage resource_usage;
	struct timeval time;
	gettimeofday(&time, NULL);
	getrusage(RUSAGE_CHILDREN, &resource_usage);

	long CPU_time = (resource_usage.ru_utime.tv_sec * 1000) + (resource_usage.ru_utime.tv_usec / 1000);
	long wall_clock = (time.tv_sec * 1000) + (time.tv_usec / 1000) - start;

	printf("The amount of CPU time used is %ld ms.\n", CPU_time);
    printf("The elapsed “wall-clock” time for the command to execute %ld ms.\n", wall_clock);
    printf("The process preempted involuntarily %ld times.\n", resource_usage.ru_nivcsw);
    printf("The process gave up the CPU %ld times.\n", resource_usage.ru_nvcsw);
    printf("The jber Of major page faults is %ld times.\n", resource_usage.ru_majflt);
    printf("The jber Of minor page faults is %ld times.\n", resource_usage.ru_minflt);
    printf("The maximum resident set size used is %ld kb\n", resource_usage.ru_maxrss);
}

int execute(char** argv)
{
	int pid;
    
	if ((pid = fork()) < 0)
	{
		fprintf(stderr, "fork failed\n");
		exit(1);
	}
	else if (pid == 0)
	{
		if (execvp(argv[0], argv) < 0)
		{
			fprintf(stderr, "invalid command\n");
			exit(1);
		}
	}
	else 
	{
		int i;
		struct timeval start;
		gettimeofday(&start, NULL);
		long start_time = ((start.tv_sec * 1000) + (start.tv_usec/1000));
		waitpid(pid, &i, 0);
		printStats(start_time, pid, i);
	}

	return 0;
}

void re()
{
    char c;
    char buffer[200];
    int i = 0;

    do {
        fprintf(stdout, "==>");

        while((c = getchar()) != '\n' && c != EOF)
        {
            if (i == MAX_COMMAND)
                fprintf(stderr, "input too long.\n");
            else
            	buffer[i++] = c;
        }

        buffer[i] = '\0';
        
        char* read[MAX_ARG];
        int j = 0;
        char* token = strtok(buffer, " ");
        read[j] = token;
        j++;

        while (token != NULL)
        {
            token = strtok(NULL, " ");
            if (token != NULL)
            {
                read[j] = token;
                j++;
            }
        }
        read[j] = NULL; 

        if (strcmp(read[0],"cd") == 0)
        { 
            if (chdir(read[1]) == -1) 
                fprintf(stderr, "cd command failed.\n");
        }
        else if (strcmp(read[0], "exit") == 0)
        {
            printf("Exit successfully.\n");
            exit(0);
        }
        else
            execute(read);

        memset(buffer, 0, i+1);
        i = 0;
    } while (1);
}

int main(int argc, char** argv)
{
	if(argc > 1)
		execute(argv + 1);
	else
		re();

	return 0;
}





