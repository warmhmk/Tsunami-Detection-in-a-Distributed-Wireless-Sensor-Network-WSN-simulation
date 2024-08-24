#ifndef SHARED_H_INCLUDED
#define SHARED_H_INCLUDED

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#define MSG_EXIT 1
#define MSG_PRINT_ORDERED 2
#define MSG_PRINT_UNORDERED 3
#define THRESHOLD_N 6000
#define DECISION_THRESHOLD 100
#define SHIFT_ROW 0
#define SHIFT_COL 1
#define DISP 1
#define G_ARRAY_SIZE 15
#define ARRAY_SIZE 50
#define ERROR 200

pthread_mutex_t stopMutex;
pthread_mutex_t input_lock;
char alti_arr[15][256];

#endif
