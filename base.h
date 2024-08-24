#ifndef BASE_H_INCLUDED
#define BASE_H_INCLUDED

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

int base_io(MPI_Comm world_comm, MPI_Comm comm, int userrows, int usercols, int input);

#endif
