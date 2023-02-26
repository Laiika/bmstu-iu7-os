/*
 * This is sample code generated by rpcgen.
 * These are only templates and you can use them
 * as a guideline for developing your own functions.
 */

#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include "bakery.h"

typedef struct
{
    int num;
    char letter;
    int id;
} target_t;

int  choosing[MAX_CLIENTS] = {0};
int  number[MAX_CLIENTS]   = {0};
char letter                = 'a';
int  num                   = 0;


void *get_number(void *arg)
{
    target_t *target = arg;
    int i = num;
    num++;
    target->id = i;
    choosing[i] = 1;
    int max = 0;
    for (int j = 0; j < MAX_CLIENTS; j++)
        if (number[j] > max)
            max = number[j];
    number[i] = max + 1;
    target->num = number[i];
    choosing[i] = 0;
}

void *bakery(void *arg)
{
    target_t *target = arg;
    int i = target->id;
    for (int j = 0; j < MAX_CLIENTS; j++) 
    {
        while (choosing[j]);
        while ((number[j] > 0) && (number[j] < number[i] || (number[j] == number[i] && j < i)));
    }
    target->letter = letter;
    letter++;
    number[i] = 0;
}

struct BAKERY *bakery_proc_1_svc(struct BAKERY *argp, struct svc_req *rqstp)
{
	static struct BAKERY result;
    switch (argp->op)
    {
        case GET_NUMBER:
        {
            pthread_t thread;
            target_t target_res;
            pthread_create(&thread, NULL, get_number, &target_res);
            pthread_join(thread, NULL);
            result.pid = argp->pid;
            result.num = target_res.num;
            result.op = target_res.id;
            break;
        }
        case GET_LETTER:    
        {
            pthread_t thread;
            target_t target_res;
            target_res.id = argp->num;
            pthread_create(&thread, NULL, bakery, &target_res);
            pthread_join(thread, NULL);
            result.pid = argp->pid;
            result.letter = target_res.letter;
            result.op = target_res.id;
            break;
        }
        default:
            break;
    }
    return &result;
}