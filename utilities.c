#include <stdio.h>
#include "utilities.h"
#include "shared.h"


/* To find moving average */
int movingAvg(int *ptrArrHeight, long *ptrSum, int pos, int len, int nextNum)
    {
      //Subtract the oldest height from the previus sum and add the new height
      *ptrSum = *ptrSum - ptrArrHeight[pos] + nextNum;
      
      //Assign the nextNum to the position in the array
      ptrArrHeight[pos] = nextNum;
      
      //return the average
      return *ptrSum / len;
    }

// random float generator
float RFG(int lower, int upper, int rank) {
    float num;
    //char buf[50];
    
    srand(time(NULL)*rank);
    num = ((float)rand()/(float)(RAND_MAX/(upper-lower)))+lower;

    return num;
}

// random number generator
int RNG(int lower, int upper, int mode) {
    int num;
    
    srand(mode);
 
    num = rand();
    num = num % (upper-lower);
    num = num + lower;

    return num;
}

// thread to listen to user input
void *user_input(void *input) {
    char buf[256];
    int *arr = (int *)input;
    
    sprintf(buf, "Enter 1 to stop the program!\n");
    fputs(buf,stdout);
    
    do {
        fgets(buf, 256, stdin);
    } while (strcmp(buf, "1") == 0);

    sprintf(buf, "ENDING PROGRAM......\n");
    fputs(buf,stdout);
    
    pthread_mutex_lock(&input_lock);
    arr[0] = 1;
    pthread_mutex_unlock(&input_lock);
    return 0;
}

