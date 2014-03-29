#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "msg.h"    /* For the message struct */

/* The size of the shared memory chunk */
#define SHARED_MEMORY_CHUNK_SIZE 1000

/* The ids for the shared memory segment and the message queue */
int shmid, msqid;

/* The pointer to the shared memory */
void *sharedMemPtr;

/* The name of the received file */
const char recvFileName[] = "recvfile";

/**
 * Sets up the shared memory segment and message queue
 * @param shmid - the id of the allocated shared memory 
 * @param msqid - the id of the shared memory
 * @param sharedMemPtr - the pointer to the shared memory
 */

void init(int& shmid, int& msqid, void*& sharedMemPtr)
{
	
	/* TODO: 
		1. Create a file called keyfile.txt containing string "Hello world" (you may do
			so manually or from the code).
		2. Use ftok("keyfile.txt", 'a') in order to generate the key.
		3. Use the key in the TODO's below. Use the same key for the queue
			and the shared memory segment. This also serves to illustrate the difference
			between the key and the id used in message queues and shared memory. The id
			for any System V object (i.e. message queues, shared memory, and sempahores) 
			is unique system-wide among all System V objects. Two objects, on the other hand,
			may have the same key.
	 */
	printf("Initalizing everything in reciever...\n");
	key_t key = ftok("keyfile.txt", 'a');
	if (key == -1)
	{
		perror("ftok");
		exit(1);
	}

	/* TODO: Allocate a piece of shared memory. The size of the segment must be SHARED_MEMORY_CHUNK_SIZE. */
	shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, 0644);
	if (shmid == -1)
	{
		perror("shmget");
		exit(1);
	}

	/* TODO: Attach to the shared memory */
	sharedMemPtr = shmat(shmid, (void *)0, 0);
	if (sharedMemPtr == (void *) -1)
	{
		perror("shmat");
		exit(1);
	}

	/* TODO: Create a message queue */
	msqid = msgget(key, 0666);
	if (msqid == -1)
	{
		perror("msgget");
		exit(1);
	}
	printf("DEBUG: msqid(%d)\n", msqid);

	/* Store the IDs and the pointer to the shared memory region in the corresponding parameters */
	printf("...everything initialized in correctly!\n\n");
}
 

/**
 * The main loop
 */
void mainLoop()
{
	/* The size of the mesage */
	int msgSize = 0;
	
	/* Open the file for writing */
	FILE* fp = fopen(recvFileName, "w");
		
	/* Error checks */
	if(!fp)
	{
		perror("fopen");    
		exit(-1);
	}
		
	/* TODO: Receive the message and get the message size. The message will 
	 * contain regular information. The message will be of SENDER_DATA_TYPE
	 * (the macro SENDER_DATA_TYPE is defined in msg.h).  If the size field
	 * of the message is not 0, then we copy that many bytes from the shared
	 * memory region to the file. Otherwise, if 0, then we close the file and
	 * exit.
	 *
	 * NOTE: the received file will always be saved into the file called
	 * "recvfile"
	 */

	// Create messages to send and recieve!
	message sndMsg;
	message rcvMsg;
	printf("...created message to store info...\n");

	// Important: Add +1 to keep the null terminator \0 at the end!
	printf("DEBUG: msqid(%d)\n", msqid);
	printf("Reading in message...\n");
	if(msgrcv(msqid, &rcvMsg, SHARED_MEMORY_CHUNK_SIZE+1, SENDER_DATA_TYPE, 0) == -1)
	{
		perror("msgrcv");
		exit(1);
	}
	
	msgSize = rcvMsg.size + 1;
	printf("...recieved!\n");
	printf("DEBUG: %d %ld\n", rcvMsg.size, rcvMsg.mtype);

	/* Keep receiving until the sender set the size to 0, indicating that
	 * there is no more data to send
	 */ 
	while(msgSize != 0)
	{   
		printf("...looping...\n\n");
		/* If the sender is not telling us that we are done, then get to work */
		if(msgSize != 0)
		{
			printf("Going to write to file...\n");
			//This below fails!
			/* Save the shared memory to file */
			if(fwrite(sharedMemPtr, sizeof(char), msgSize, fp) == 0)
			{
				perror("fwrite");
			}
			printf("...wrote to file!\n");
			
			/* TODO: Tell the sender that we are ready for the next file chunk. 
			 * I.e. send a message of type RECV_DONE_TYPE (the value of size field
			 * does not matter in this case). 
			 */
			printf("ending empty message back...\n");
			msgsnd(msqid, &sndMsg, 0, 0);
			printf("...sent!\n");
		}
		/* We are done */
		else
		{
			/* Close the file */
			fclose(fp);
		}
	}
}


/**
 * Perfoms the cleanup functions
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 * @param msqid - the id of the message queue
 */
void cleanUp(const int& shmid, const int& msqid, void* sharedMemPtr)
{
	//printf("Oops! trying to clean up. Not implemented yet!\n");
	/* TODO: Detach from shared memory */
	shmdt(sharedMemPtr);
	printf("Detached from shared memory...\n");
	
	/* TODO: Deallocate the shared memory chunk */
	//This isn't working...
	shmctl(shmid, IPC_RMID, NULL);
	printf("Deallocated shared memory chunk...\n");
	
	/* TODO: Deallocate the message queue */
	msgctl(msqid, IPC_RMID, NULL);
	printf("Deallocated the message queue...\n");
}


/**
 * Handles the exit signal
 * @param signal - the signal type
 */
void ctrlCSignal(int signal)
{
	printf("\nCtrl-C has been pressed, going to clean up...\n");
	/* Free system V resources */
	cleanUp(shmid, msqid, sharedMemPtr);
}

int main(int argc, char** argv)
{
	
	/* TODO: Install a singnal handler (see signaldemo.cpp sample file).
	 * In a case user presses Ctrl-c your program should delete message
	 * queues and shared memory before exiting. You may add the cleaning functionality
	 * in ctrlCSignal().
	 */
	signal(SIGINT, ctrlCSignal);
				
	/* Initialize */
	init(shmid, msqid, sharedMemPtr);
	
	printf("Starting reciever program...\n");
	/* Go to the main loop */
	mainLoop();

	/** TODO: Detach from shared memory segment, and deallocate shared memory and message queue (i.e. call cleanup) **/
	cleanUp(shmid, msqid, sharedMemPtr);
		
	return 0;
}
