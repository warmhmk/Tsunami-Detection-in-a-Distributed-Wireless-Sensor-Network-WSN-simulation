#include <stdio.h>
#include "tsunameter.h"
#include "utilities.h"
#include "shared.h"

/* This is the tsunameter */
int tsunameter_io(MPI_Comm world_comm, MPI_Comm comm, int userrows, int usercols, int input)
{

	int size, my_rank, worldSize;

    // variable used for saving data and sending to base
	char buf[256];

    // value used for saving termination signal from base
	int exitVal = 0;
	
    // variables used for topology creation
	int ndims=2, reorder, my_cart_rank, ierr; 
	MPI_Comm comm2D;
	int dims[ndims],coord[ndims];
    // used to identify immediate neighbours (bottom, top, left, right)
	int nbr_i_lo, nbr_i_hi, nbr_j_lo, nbr_j_hi;
	int wrap_around[ndims];

    //
	int ready;

    // size of the world communicator
    MPI_Comm_size(world_comm, &worldSize); 
    // size of the tsunameter communicator
    MPI_Comm_size(comm, &size); 
    // rank of the tsunameter communicator
	MPI_Comm_rank(comm, &my_rank);  
	
	// if user has entered values, set dimensions accordingly
	if (input == 1){
        dims[0] = userrows; /* number of rows */
        dims[1] = usercols; /* number of columns */
	}
    // else set them to 0 so that MPI_Dims_create will create best possible grid
	else{
	    dims[0]=dims[1]=0;
    }
    /* function for creating dimensions based on either user specified sizes or automatically chosen sizes (if dims[0] and dims[1] is 0) */
    MPI_Dims_create(size, ndims, dims);
  
    // print message from process 0 of tsunameter to indicate user of chosen grid size
    if(my_rank==0){
        printf("Tsunameter Grid Size: %d\nGrid Dimension = [%d x %d] \n",size,dims[0],dims[1]);
        printf("System working...\n");
    }
	    
	// declaring status and request tags used for MPI send and recv functions
	MPI_Request req, req_exit;
	MPI_Status status;
	
    // status and request tags used for internode communication
    MPI_Request send_request[4];
    MPI_Request receive_request[4];
    MPI_Status send_status[4];
  
    /* create cartesian mapping */
    wrap_around[0] = 0;
    // set periodic shift is to false
    wrap_around[1] = 0; 
    reorder = 0;
    ierr =0;

    // create grid
    ierr = MPI_Cart_create(comm, ndims, dims, wrap_around, reorder, &comm2D);
    if(ierr != 0) printf("ERROR[%d] creating CART\n",ierr);
    
    // find coordinates in the cartesian communicator group
    MPI_Cart_coords(comm2D, my_rank, ndims, coord);
    // use cartesian coordinates to find rank in cartesian group
    MPI_Cart_rank(comm2D, coord, &my_cart_rank);

    // used to get the immediate neighbours(with displacement 1) of a node
    MPI_Cart_shift( comm2D, SHIFT_ROW, DISP, &nbr_i_lo, &nbr_i_hi);
    MPI_Cart_shift( comm2D, SHIFT_COL, DISP, &nbr_j_lo, &nbr_j_hi);

    // aray used to calculate SMA from random values generated
    int arrHeight[3] = {0};

    // position that resets to 0 when >= len
    int pos = 0;

    // current sum used for SMA (can substract oldest value and add newest value to getnew average)
    long sum = 0;

    // this len represents the number of values used to calculate SMA (in our case 3)
    int len = sizeof(arrHeight) / sizeof(int);

    float newAvg = 0.0;

    // seed using time and rank for random height
    unsigned int seed = time(NULL) * my_rank; 
    float height;
    
    do{
        
        // checking for end signal from base
        if(exitVal==0){
            //receive termination message froom base station
            MPI_Irecv(&exitVal, 1, MPI_INT, 0, MSG_EXIT, MPI_COMM_WORLD, &req_exit); 
        }
        
        // generate random height in range (5750 - 6500)
        height = (rand_r(&seed) % 750) + 5750;
    
        // get simple moving average using generated height value
        newAvg = movingAvg(arrHeight, &sum, pos, len, height);
        
        // sending and recieving within grid
        MPI_Isend(&newAvg, 1, MPI_INT, nbr_i_lo, 0, comm2D, &send_request[0]);
        MPI_Isend(&newAvg, 1, MPI_INT, nbr_i_hi, 0, comm2D, &send_request[1]);
        MPI_Isend(&newAvg, 1, MPI_INT, nbr_j_lo, 0, comm2D, &send_request[2]);
        MPI_Isend(&newAvg, 1, MPI_INT, nbr_j_hi, 0, comm2D, &send_request[3]);
        MPI_Waitall(4, send_request, send_status);
        
        // for performance imporovement
        usleep(500);

        // to store matches with neighbour nodes
        int matches = 0;

        // if average exceeds threshold
        if (newAvg >= THRESHOLD_N){

            // request averages of neighbours
            float recvValL = -1.0, recvValR = -1.0, recvValT = -1.0, recvValB = -1.0;
            MPI_Irecv(&recvValT, 1, MPI_INT, nbr_i_lo, 0, comm2D, &receive_request[0]);
            MPI_Irecv(&recvValB, 1, MPI_INT, nbr_i_hi, 0, comm2D, &receive_request[1]);
            MPI_Irecv(&recvValL, 1, MPI_INT, nbr_j_lo, 0, comm2D, &receive_request[2]);
            MPI_Irecv(&recvValR, 1, MPI_INT, nbr_j_hi, 0, comm2D, &receive_request[3]);
            
            // count the matches
            if (newAvg > recvValL - DECISION_THRESHOLD && newAvg < recvValL + DECISION_THRESHOLD){
                matches ++;
            }
            if (newAvg > recvValR - DECISION_THRESHOLD && newAvg < recvValR + DECISION_THRESHOLD){
                matches ++;
            }
            if (newAvg > recvValT - DECISION_THRESHOLD && newAvg < recvValT + DECISION_THRESHOLD){
                matches ++;
            }
            if (newAvg > recvValB - DECISION_THRESHOLD && newAvg < recvValB + DECISION_THRESHOLD){
                matches ++;
            }

            // if at least 2 matches
            if (matches >= 2){
                time_t now = time(NULL);
                // save data to buffer
                sprintf(buf, "rank:%d, coord:(%d, %d), height:%.2f, left:%d, %.2f, right:%d, %.2f, top:%d, %.2f, bottom:%d, %.2f, [%s]", my_cart_rank, coord[0], coord[1], newAvg, nbr_j_lo, recvValL, nbr_j_hi, recvValR, nbr_i_lo, recvValT, nbr_i_hi, recvValB ,ctime(&now));
                // sending buffer to base
                MPI_Isend(buf, strlen(buf) + 1, MPI_CHAR, 0, 0, world_comm, &req);
                MPI_Wait(&req, &status);
            }
            
        }
        
        // test for pending requests and cancel them
        MPI_Test(&req_exit, &ready, MPI_STATUS_IGNORE);
        if(!ready){
        	MPI_Cancel(&req_exit);
        }
        
        // increase pos used for SMA calculation
	      pos ++;
        if (pos >= len){
          pos = 0;
        }
        
        // sleep 2 seconds for 2 seconds cycles
        sleep(2);

    // repeat till base sends termination signal
    }while (exitVal != MSG_EXIT);
    

    fflush(stdout);
    MPI_Comm_free( &comm2D );
    return 0;
}


