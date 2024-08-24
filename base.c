#include <stdio.h>
#include "base.h"
#include "utilities.h"
#include "altimeter.h"
#include "shared.h"

/* This is the master */
int base_io(MPI_Comm world_comm, MPI_Comm comm, int userrows, int usercols, int input){
	int         size, x, y, exit;
	int         counter = 0;
	int         found = 0;
	int         exist = 0;
	float       height;
	char        *date; // date from node
	char        *g_date; // date from altimeter
	char        buf[256];
	char        buf2[256];
	char        string[300];
    char        string2[300];
	char        array[ARRAY_SIZE][256]; // array of data from node
	time_t      start, now;
	int         alti_args[5];
	int         input_args[1];
	int         arr[9]; /* cartrank, coord[0](y), coord[1](x), nbr_j_lo, nbr_j_hi, nbr_i_lo, nbr_i_hi*/
    float       farr[5]; // newAvg(height), recvValL, recvValR, recvValT, recvValB

    // getting world size (number of processes)
	MPI_Comm_size(world_comm, &size );

	MPI_Status status;
	MPI_Request req;

    // variable used to send exit signal to tsunameter
	int nodeExit = 0;
	
	// to be changed ---------------------
	int interval = 1;
	int iteration = 0;
	// -----------------------------------
	
    // number of reports matched with altimeter
    int trueAlerts = 0;
    // number of false mathces
    int falseAlerts = 0;
    // total number of reports sent
    int totalAlerts = 0;

    FILE *pFile;
    // array used to format output to table in file
    char *row1[] = {"Reporting Node", "Height(m)", "Adjacent Nodes", "Coord"};
	
    // declare threads (thread for altimeter, thread2 for user input for termination)
    pthread_t thread, thread2;
    
    int ndims = 2;
    int dims[ndims];
    
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
    MPI_Dims_create(size-1, ndims, dims);

    // array to be passed to altimeter
    alti_args[0] = interval;
    alti_args[1] = iteration;
    
    // passing grid dimensions to altimeter
    alti_args[2] = dims[0];
    alti_args[3] = dims[1];
    alti_args[4] = 0;

    // create altimeter thread
    pthread_create(&thread, NULL, altimeter_io, (void *)alti_args);

    // create user_input thread
    pthread_create(&thread2, NULL, user_input, (void *)input_args);

    // opening file for writing
    pFile = fopen("log.txt", "w");

    // get current time
    time(&start);

    // check user input and update exit variable
    pthread_mutex_lock(&input_lock);
    if (input_args[0] == 1) {
    exit = 1;
    }
    pthread_mutex_unlock(&input_lock);

    // if no user input for exit
	while (exit != 1) {

        // receive data from slaves
        MPI_Irecv(buf, 256, MPI_CHAR, MPI_ANY_SOURCE, 0, world_comm, &req );
        MPI_Wait(&req, &status);
	    
	    	    
        /* save buf into array to ensure all reports are checked.
        this makes a single iteration have multiple checks, so its possible for
        multiple entries to be added to the log file in the same iteration.
        Essentially, saves all incoming reports and stores for base to compare each
        report with altimeter readings.
        */
        strcpy(array[counter], buf);
        counter++;

        // get current time
        time(&now);

        // runs for each interval
        if (now - start >= interval) {
            time(&start);
            iteration++;
          
            // for each report from tsunameter, compare with data from altimeter
            for (int k = 0; k < counter; k++) { 
              
              exist = 1; // indicate that there is at least 1 report from tsunameter
              totalAlerts ++;
                
              // extract the data from tsunameter into arrays
              sscanf(array[k], "rank:%d, coord:(%d, %d), height:%f, left:%d, %f, right:%d, %f, top:%d, %f, bottom:%d, %f", &arr[0], &arr[1], &arr[2], &farr[0], &arr[3], &farr[1], &arr[4], &farr[2], &arr[5], &farr[3], &arr[6], &farr[4]);
                
              // get values from array of string (altimeter)
              for (int j = 0; j < G_ARRAY_SIZE; j++) {
              
                // if string is not empty
                pthread_mutex_lock(&stopMutex);
                int nonEmpty = (strcmp(alti_arr[j],"") != 0);
                pthread_mutex_unlock(&stopMutex);
                  
                if (nonEmpty) {
                  // extract the int values
                  pthread_mutex_lock(&stopMutex);
                  sscanf(alti_arr[j], "%d,%d,%f", &x,&y,&height);
                  pthread_mutex_unlock(&stopMutex);
                  
                  // get tsunameter date wrapped in '[ ]'
                  strcpy(string2, array[k]);
                  date = strtok(string2, "[");
                  date = strtok(NULL, "]");

                  // get altimeter date from array wrapped in '[ ]'
                  pthread_mutex_lock(&stopMutex);
                  strcpy(string, alti_arr[j]);
                  pthread_mutex_unlock(&stopMutex);
                  g_date = strtok(string, "[");
                  g_date = strtok(NULL, "]");
                
                  // if match then log (within error range) 
                  if (arr[2] == x && arr[1] == y && (farr[0] > height-ERROR && farr[0] < height+ERROR) && (strcmp(g_date, date) == 0)) {
                    // set found to true
                    found = 1;

                    // log formatted data to file
                    fprintf(pFile, "Iteration: %d\nLogged Time: %sAlert reported time: %sMatch Type: True\n\n", iteration, ctime(&now), date);
                    
                    fprintf(pFile, "%*s | %*s   | %s\n", -3, row1[0], -7, row1[1], row1[3]);
                    fprintf(pFile, "%-14d | %10f | (%d,%d)\n", arr[0], farr[0], arr[1], arr[2]);
                    fprintf(pFile, "\n");
                    
                    fprintf(pFile, "%*s | %*s \n", -13, row1[2], -7, row1[1]);
                    fprintf(pFile, "Left : %-7d | %9f\n", arr[3], farr[1]);
                    fprintf(pFile, "Right : %-6d | %9f\n", arr[4], farr[2]);
                    fprintf(pFile, "Top : %-8d | %9f\n", arr[5], farr[3]);
                    fprintf(pFile, "Bottom : %-5d | %9f\n", arr[6], farr[4]);
                    fprintf(pFile, "\n");
                    
                    fprintf(pFile, "Satellite altimeter reporting time: %s", g_date);
                    fprintf(pFile, "Satellite altimeter reporting height(m): %f\n", height);
                    fprintf(pFile, "Satellite altimeter reporting coordinates(m): (%d,%d)\n", y,x);
                    fprintf(pFile, "\n");
                    fprintf(pFile, "================================================================================");
                    fprintf(pFile, "\n\n");

                    // update matched reports counter
                    trueAlerts++;
                    break;
                  }     
                }
              }     
              // if no match found
              if (found == 0) {
                // log to file as false-match
                fprintf(pFile, "Iteration: %d\nLogged Time: %sAlert reported time: %sMatch Type: False\n\n", iteration, ctime(&now), date);
                fprintf(pFile, "%*s | %*s   | %s\n", -3, row1[0], -7, row1[1], row1[3]);
                fprintf(pFile, "%-14d | %10f | (%d,%d)\n", arr[0], farr[0], arr[1], arr[2]);
                fprintf(pFile, "\n");
                
                fprintf(pFile, "%*s | %*s \n", -13, row1[2], -7, row1[1]);
                fprintf(pFile, "Left : %-7d | %9f\n", arr[3], farr[1]);
                fprintf(pFile, "Right : %-6d | %9f\n", arr[4], farr[2]);
                fprintf(pFile, "Top : %-8d | %9f\n", arr[5], farr[3]);
                fprintf(pFile, "Bottom : %-5d | %9f\n", arr[6], farr[4]);
                fprintf(pFile, "\n");
                
                fprintf(pFile, "Satellite altimeter reporting time: %s", g_date);
                fprintf(pFile, "Satellite altimeter reporting height(m): %f\n", height);
                fprintf(pFile, "Satellite altimeter reporting coordinates(m): (%d,%d)\n", y,x);
                fprintf(pFile, "\n");
                fprintf(pFile, "================================================================================");
                fprintf(pFile, "\n\n");
                
                falseAlerts ++;
              }
              
              // else reset found
              else {
                found = 0;
              }


            }
            // if there is at least 1 report (from tsunameter) in the array
            if (exist == 1) {
                // reset the array index
                counter = 0;
                exist = 0;
            }
        }
        
        // check if there is user input for exit and update exit val
        pthread_mutex_lock(&input_lock);
        if (input_args[0] == 1) {
          exit = 1;
        }
        pthread_mutex_unlock(&input_lock);
	}
	
    // add summary of reports at end of file
    fprintf(pFile, "Total alerts: %d\nTrue alerts: %d\nFalse alerts: %d", totalAlerts, trueAlerts, falseAlerts);

    // file closing
    fclose(pFile);

    // set altimeter termination value to 1
    pthread_mutex_lock(&stopMutex);
    alti_args[4] = 1;
    pthread_mutex_unlock(&stopMutex);

    // join threads
    pthread_join(thread, NULL);
    pthread_join(thread2, NULL);

	// send signal to end to each slave process to exit
	nodeExit = 1;
	for (int i = 1; i < size; i ++){
	    MPI_Send(&nodeExit, 1, MPI_INT, i, MSG_EXIT, MPI_COMM_WORLD);
	}

    // print message to let user know all data saved to file
    printf("Complete - All data logged in file\n");
    return 0;
}

