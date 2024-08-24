#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "altimeter.h"
#include "base.h"
#include "tsunameter.h"
#include "utilities.h"
#include "shared.h"

#define MSG_EXIT 1
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

// compiling instruction:
// mpicc main.c base.c altimeter.c tsunameter.c utilities.c -o main.o -lm

// to run (N: number of process (number of node + 1), y & x: grid size):
// mpirun -oversubscribe -np N main.o y x

int main(int argc, char **argv)
{

  int rank, size, userrows, usercols;
  MPI_Comm new_comm;
  
  //printf("Enter threshold: \n");
  //scanf("%d", &threshold);

  //Initialize MPI
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  
  // variable to check if user has entered grid size
  int input = 0;

  // handling user input for custom grid size
  if (argc == 3) {
      userrows = atoi (argv[1]);
      usercols = atoi (argv[2]);
      input = 1;
      if( (userrows*usercols) != size - 1) {
          printf("ERROR: nrows * ncols) = %d * %d = %d != %d\n", userrows, usercols, userrows*usercols,size);
          MPI_Finalize();
          return 0;
      }
  } 
  else {
      // set userrows and usercolumns to 0, to let MPI create best possible grid
      userrows=usercols=0;
  }

  // split commuicator into two, here base is rank 0, and the rest are used for tsunameter
  MPI_Comm_split( MPI_COMM_WORLD,rank == 0, 0, &new_comm);
  if (rank == 0)
    base_io( MPI_COMM_WORLD, new_comm, userrows, usercols, input);
  else
    tsunameter_io( MPI_COMM_WORLD, new_comm, userrows, usercols, input);
  
  MPI_Finalize();
  return 0;
}

