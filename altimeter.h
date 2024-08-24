#ifndef ALTIMETER_H_INCLUDED
#define ALTIMETER_H_INCLUDED

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

void *altimeter_io(void *input);

#endif
