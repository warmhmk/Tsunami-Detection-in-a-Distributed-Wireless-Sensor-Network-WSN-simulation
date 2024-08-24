#ifndef UTILITIES_H_INCLUDED
#define UTILITIES_H_INCLUDED

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

int movingAvg(int *ptrArrNumbers, long *ptrSum, int pos, int len, int nextNum);
float RFG(int lower, int upper, int rank);
int RNG(int lower, int upper, int mode);
void *user_input(void *input);

#endif
