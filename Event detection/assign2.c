#include <stdio.h>
#include "mpi.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>
#define EXIT 0
#define EVENT 1
#define MAXN 256


int sum; //total messages of system
void baseStation(MPI_Comm master_comm); 
void node(MPI_Comm master_comm, MPI_Comm comm);
int eventChecker(int array[4]);

int main(int argc, char** argv){
    int rank;
    int size;
    MPI_Init(&argc, &argv); //Initialize the MPI execution environment 
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); //Determines the rank of the calling process in the communicator 
    MPI_Comm_size(MPI_COMM_WORLD, &size);//Determines the size of the group associated with a communicator 

    if (size != 21){ //if the number of nodes is not eqaul to 21, abort.
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int ndims = 2; //number of dimensions of cartesian grid 
    int dimension[2]; //size of the the dimension is 2, to make a 4x5 grid
    int periods[2]; //logical array of size ndims specifying whether the grid is periodic (true) or not (false) in each dimension 
    int reorder = 0;//ranking may be reordered (true) or not (false) (logical) 
    MPI_Comm comm_cart; //communicator with new cartesian topology (handle) 
    MPI_Comm comm_old; //input communicator (handle) 

    dimension[0]=5;
    dimension[1]=4;
    periods[0]=0; //stored as False
    periods[1]=0; //stored as False

    
    MPI_Comm_split(MPI_COMM_WORLD, rank==0, 0, &comm_old); //split into masters(base station) & slaves(nodes)

    if (rank > 0){ //4x5 grid for the slaves
        MPI_Cart_create(comm_old, ndims, dimension, periods, reorder, &comm_cart);// Makes a new communicator to which topology information has been attached 
    }
    
    if (rank==0){
	baseStation(MPI_COMM_WORLD); //rank 0 executes base station 
    }else if (rank>0){
	 node(MPI_COMM_WORLD, comm_cart); //other ranks nodes
    }

    MPI_Finalize(); //Terminates MPI execution environment 
    return 0;
}


int eventChecker(int array[4]){
    int simulation1=0;
    int simulation2=0;
 
    for (int i=0; i<=3;i++){
         if (array[i] == array[0]){ //compare each nodes values with the first value
		simulation1++; //increment the simualation counter
    	 }
    }
    
    if (simulation1 >=3){
	 return 1; //return true if there are 3 or more events
    }    
   
    for (int i=0; i<=3;i++){
         if (array[i] == array[1]){ //compare each nodes values with the second value
		simulation2++; //increment the simulation counter
	 }
    }
    
    if (simulation2 >=3){
	return 1; //return true if there are 3 or more events
    }else{
	return 0; //return false if not
    }
}


void node(MPI_Comm master_comm, MPI_Comm comm){
    int numRequests;
    int rank;
    int size;
    MPI_Request array_of_requests[8];
    MPI_Status array_of_statuses[8];
    int adjNodes[4];
    int recieve_buffer[4];
    char buf[MAXN];
    MPI_Request request;
    int numMsg=0;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    //generate a random number for 1000 iterations
    for  (int j = 0; j < 1000; j++){
        srandom(time(NULL) + rank + j);
        int min = 1; 
	int max=10;
        int ranNum = (random()%(max-min+1))+min;
        numRequests = 0;
        MPI_Cart_shift(comm, 0, 1, &adjNodes[0], &adjNodes[1]); //get adjacent nodes
        MPI_Cart_shift(comm, 1, 1, &adjNodes[2], &adjNodes[3]); //get adjacent nodes
        
	//sending messages the adjacent nodes
        for (int i=0; i<4; i++){
                if(adjNodes[i] >= 0){
                    MPI_Isend(&ranNum, 1, MPI_INT, adjNodes[i], 0, comm, &array_of_requests[numRequests++]);
                    numMsg++;
                    MPI_Irecv(&recieve_buffer[i], 1, MPI_INT, adjNodes[i], 0, comm, &array_of_requests[numRequests++]);
                }
            }

        MPI_Waitall(numRequests, array_of_requests, array_of_statuses);//Waits for all given MPI Requests to complete 


        if (eventChecker(recieve_buffer)==1){ //if theres an event there
            sprintf(buf, "Node Event: %d   Adjacent Nodes:[%d, %d, %d, %d]\n", rank, adjNodes[0], adjNodes[1], adjNodes[2], adjNodes[3]);
            MPI_Isend(buf, MAXN, MPI_CHAR, 0, EVENT, master_comm, &request); //non-blocking send to base station
            numMsg++; //increment the total messages
        }
    }

    //to end simulation, a finish request is sent
    //the total messages(num messages) are collected in MPI_reduce
    MPI_Isend(buf, MAXN, MPI_CHAR, 0, EXIT, master_comm, &request);
    numMsg++; 
    MPI_Reduce(&numMsg, &sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD); 
}



void baseStation(MPI_Comm master_comm){
    char buffer[MAXN]="NULL"; //initialize the buffer
    char measure[MAXN]; //measure array of size 256
    int requests = 0;
    int events=0;
    int numMsg=0;
    MPI_Status status;
    FILE *output;

    output = fopen("output.txt", "w+");
    double start = MPI_Wtime();//start time
    //iteration for the nodes 
    while (requests < 20){
        MPI_Recv(buffer, MAXN, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, master_comm, &status);
        switch(status.MPI_TAG){  
	    case EXIT: //case for message exit
		requests++; 
		break;
   	    case EVENT: //case for message event
		fputs(buffer, output); //write
		events++;
		break;
        }
    }
	//to calculate the total messages
    MPI_Reduce(&numMsg, &sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD); //Reduces values on all processes to a single value 
    double end = MPI_Wtime(); //end time
    double total = end - start; //get the total time
    sprintf(measure, "Time Taken: %f\nEvents:%d\nMessages: %d\n", total, events, sum);
    fputs(measure , output); //log performance metrics
    fclose(output);
}




