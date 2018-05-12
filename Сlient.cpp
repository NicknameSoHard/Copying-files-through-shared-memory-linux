#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <fcntl.h>

struct shared_buffer
{
	int size_buff[8];
	char block_data[8][124];
};

int main(int argc, char** argv)
{
	// Check arguments
	char* dir;
	int file_id;
	if (argv[1] == NULL)
	{
		printf("The field of the file is empty, there is nothing to transfer.\n");
		exit(EXIT_SUCCESS);
	}
	else
	{
		if( strncmp(argv[1], "-h", 2) == 0)
		{
			printf("Client take file as argument and send it on server.\n");
			exit(EXIT_SUCCESS);
		}
		dir = new char[strlen(argv[1]) + 1];
		strcpy(dir,argv[1]);
		dir[strlen(argv[1])]='\0';
		
		file_id = open(dir,O_RDONLY);
		if(file_id == -1)
	  	{
			printf("File doesn't exist or is not available\n");
			close(file_id);
			exit(EXIT_SUCCESS);	
		}
		close(file_id);
		printf("File: %s\n", dir);
	}
	// Get shared memory id
 	void *shared_memory = (void *)0;
	struct shared_buffer* shared_data = new struct shared_buffer;
 	int shmid;
	shmid = shmget((key_t)7501, 1024, 0666 | IPC_CREAT);
 	if (shmid == -1)
	{
		fprintf(stderr, "shmget failed\n");
		exit(EXIT_FAILURE);
 	}
  	// Attach shared memory
 	shared_memory = shmat(shmid, (void *)0, 0);
 	if (shared_memory == (void *)-1)
	{
		fprintf(stderr, "shmat failed\n");
		exit(EXIT_FAILURE);
 	}
	printf("Getting shared memory: %p\n", shared_memory);
	// Associate our struct and shared memory
	shared_data = (struct shared_buffer*)shared_memory;
	// Create semaphores
	int sem_full, sem_empty;
	sem_full = semget((key_t)1057, 1, 0666 | IPC_CREAT);
	sem_empty = semget((key_t)2057, 1, 0666 | IPC_CREAT);
	if(sem_full == -1 || sem_empty == -1)
	{
		printf("semget failed\n");	
		exit(EXIT_FAILURE);
	}
	// Create struct for control semaphores
	struct sembuf control_sem;
	control_sem.sem_flg = 0;
	control_sem.sem_num = 0;

	// Set semaphores on default state
	semctl(sem_full, 0, SETVAL, 0); 
	semctl(sem_empty, 0, SETVAL, 8);

	// Open file
	file_id = open(dir,O_RDONLY);
	delete[] dir;
	int read_size, mod;
	int cycle_counter = 0;
	while(true)
	{		
		mod = cycle_counter%8;
		control_sem.sem_op = -1;
		semop(sem_empty, &control_sem, 1);
		read_size = read(file_id, shared_data->block_data[mod], 124);
		shared_data->size_buff[mod] = read_size;
		control_sem.sem_op = 1;
		semop(sem_full,&control_sem, 1);
		if(read_size <= 0)
		{
			control_sem.sem_op = 1;
			semop(sem_empty,&control_sem, 1);
			break;
		}
		else
			printf("Package %i is accepted. Size: %i.\n", cycle_counter, read_size);
		cycle_counter++;
 	}
	close(file_id);
	// Detaches the shared memory segment
 	if (shmdt(shared_memory) == -1)
 	{
		fprintf(stderr, "shmdt failed\n");
		exit(EXIT_FAILURE);
 	}	
  exit(EXIT_SUCCESS);
}
