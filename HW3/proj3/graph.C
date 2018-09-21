// Yifei Jin
// yjin2
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <vector>
#include <map>

using namespace std;

// use mutex when enter a crital region
void BeginRegion(), EndRegion();
// the things that the thread will do
void *Thread(void *);

// store the result
int result;
// mutex when add to result
pthread_mutex_t mutex;

// the start time of the 
time_t start;

// the node that present the pthread
class Node
{

public:

    Node(char i, int v, int t)
    {
        id = i;
        value = v;
        timeSleep = t;
    }

    ~Node()
    {
        for (size_t i = 0; i < parent.size(); i++) {

            sem_destroy(&sems[parent[i]]);
        }

    }

    // create the thread that the node will do
    void CreateThread()
    {
        if (pthread_create(&pid, NULL, Thread, (void *)this) != 0) {
            perror("pthread_create");
            exit(1);
        }

    }

    // add parent node
    void AddParentNode(char id)
    {
        parent.push_back(id);
    }

    // add child node
    void AddChildNode(char id)
    {
        child.push_back(id);
    }

    // init the sem of the thread
    void InitSem()
    {
        for (size_t i = 0; i < parent.size(); i++) {
            sem_t sem;
            sem_init(&sem, 0, 0);
            sems[parent[i]] = sem;
        }
        // init the sem
        // sem_init(&sem, 0, 1-parent.size());
    }


    // the id of the node
    char id;
    // the pthread id of the thread
    pthread_t pid;
    // the value that the thread shoud add to total
    int value;
    // the time that the thread should sleep
    int timeSleep;

    // the parent of the node
    vector<char> parent;
    // the child of the node
    vector<char> child;
    // the sem use to do some work
    map<char, sem_t> sems;
    // sem_t sem;


};

// convert the char to the node
map<char, Node*> char2node;



int main(int argc, char *argv[])
{
    
    if (argc != 2) {
        perror("command error");
        exit(1);
    }

    // init the mutex
    if (pthread_mutex_init(&mutex, NULL) < 0) {
        perror("pthread_mutex_init");
        exit(1);
    }

    // read the config from the file

    FILE *fp = fopen(argv[1], "r");
    char * line = NULL;
    size_t len = 0;
    int read;

    if (fp == NULL) {
        perror("Fail to open the config file");
        exit(1);
    }
    // read line from file
    while ((read = getline(&line, &len, fp)) != -1) {
        char * pch = NULL;
        // read the char that represent by the node
        pch = strtok (line," ");
        char id = *pch;
        // read the value that the node will get
        pch = strtok (NULL, " ");
        int value = atoi(pch);
        // read the time that the thread will sleep
        pch = strtok (NULL, " ");
        int timeSleep = atoi(pch);
        // create a new node
        Node *node = new Node(id, value, timeSleep);
        // record the relation of id and node 
        char2node[id] = node;

        // get the parent node of the current node
        pch = strtok (NULL, " ");
        while (pch != NULL)
        {
            char parentId = *pch;
            // add parent node 
            char2node[id]->AddParentNode(parentId);
            // add child node
            char2node[parentId]->AddChildNode(id);
            pch = strtok (NULL, " ");
        }

    }
    // free the memory
    if (line)  
        free(line); 
    fclose(fp);


    // init the sem
    for (map<char, Node*>::iterator it = char2node.begin(); it != char2node.end(); ++it) {
        it->second->InitSem();
    }

    // record the time that start
    time(&start);
    // create the thread
    for (map<char, Node*>::iterator it = char2node.begin(); it != char2node.end(); ++it) {
        it->second->CreateThread();
    }


    // free all the nodes
    for (map<char, Node*>::iterator it = char2node.begin(); it != char2node.end(); ++it) {
        (void)pthread_join(it->second->pid, NULL);
        delete it->second;
    }
    time_t end;
    time(&end);
    // the diff time
    double cost = difftime(end, start);

    cout << "Total computation resulted in a value of " << result << " after " << cost << " seconds." << endl;


    (void)pthread_mutex_destroy(&mutex);
    return 0;
}


void BeginRegion()
{
    pthread_mutex_lock(&mutex);
}

void EndRegion()
{
    pthread_mutex_unlock(&mutex);
}

void *Thread(void *arg)
{
    Node *node = (Node *)arg;
    // try to do the work
    for (size_t i = 0; i < node->parent.size(); i++) {
        sem_wait(&(node->sems[node->parent[i]]));
    }

    // sleep
    sleep(node->timeSleep);

    time_t end;
    time(&end);
    double cost = difftime(end, start); 
    cout << "Node " << node->id << " computed a value of " << node->value << " after " << cost <<" second." << endl;
    
    // critical region
    BeginRegion();
    result += node->value;
    EndRegion();

    // signal the child's sem
    for (int i = 0; i < node->child.size(); i++) {

        Node *childNode = char2node[node->child[i]];
        sem_post(&(childNode->sems[node->id]));
    }


}



