#include <stdio.h>
#include "altimeter.h"
#include "utilities.h"
#include "shared.h"

/* This is the altimeter */
void *altimeter_io(void *input)
{
    char string[256]; 
    int *arr = (int *)input;  // the pointer to the shared array (base and this thread)
    
    // extracting the values from the shared array
    int interval = arr[0];
    int iteration = arr[1];
    int y_max = arr[2];
    int x_max = arr[3];
    float height;
    int x, y;
    int threshold = 6000;
    int exit = 0;  // exit value
    time_t now;  // the time variable
    
    int i = 0; // the current index of the global array

    // seed based on time for random height generation
    int seed = (unsigned int)time(NULL);
    
    
    // check if exit or not (set exit value if base set exit value to 1)
    pthread_mutex_lock(&stopMutex);
    if (arr[4] == 1) {
        exit = 1; 
    }
    pthread_mutex_unlock(&stopMutex);
    
    // keep looping until termination signal is set
    while (!exit) {
        
        // generate random height
        height = RFG(6000, 6500, 1);
    
        // if height exceeds threshold
        if (height > threshold) {
        
            // create random coordinate 
            x = RNG(0, x_max, seed);
            y = RNG(0, y_max, iteration);
            
            // create string with coordinate, height and current time
            time(&now);
            sprintf(string, "%d,%d,%f,[%s]", x,y,height,ctime(&now));
 
            // put the data string into global array
            pthread_mutex_lock(&stopMutex);
            strcpy(alti_arr[i], string);
            pthread_mutex_unlock(&stopMutex);
            
            // FIFO (the index to put the data start over once the index reaches the array size)
            i = (i+1) % G_ARRAY_SIZE;
        }

        // sleep for specified interval
        sleep(interval);
        
        // recalculate seed
        seed = (unsigned int)time(NULL);
        iteration--;
        
        
        // check if exit or not and update exit variable accordingly
        pthread_mutex_lock(&stopMutex);
        if (arr[4] == 1) {
            exit = 1;
        }
        pthread_mutex_unlock(&stopMutex);
    }
    return 0;
}

