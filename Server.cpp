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
	// Get directory and help
  char* dir;
	if(argv[1] == NULL)
	{
		char* buff = new char[256];
		getcwd(buff, 256);
		int size = strlen(buff);
		dir = new char[size + 16 + 1];
		dir[0]='\0';
    strcat(dir,buff);
		dir[size]= '/';
		dir[size + 1]='\0';
    strcat(dir,"transmission.txt\0");
		delete[] buff;
		printf("No arguments. Creating default file: %s\n", dir);
	}
	else
	{
		dir = new char[strlen(argv[1])];
		strcpy(dir,argv[1]);
    // help
    if( strlen(dir) == 2 && strncmp(dir, "-h", 2) == 0)
  	{
      printf("Server take file name as argument and save all what client send.\n");
	    exit(EXIT_SUCCESS);
    }
		printf("Creating file wirh name %s\n", dir);
	}
  // Get shared memory id
  void *shared_memory = (void*)0;
  struct shared_buffer* shared_data = new struct shared_buffer;
  int shmid;
  srand((unsigned int)getpid());
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
  // Associate our struct and shared memory
  shared_data = (struct shared_buffer*)shared_memory;
  printf("Getting shared memory: %p\n", shared_memory);
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
  // Waiting if it need
  while(semctl(sem_full, 0, GETVAL) != 0 && semctl(sem_empty, 0, GETVAL) !=0)
  {
    printf("Waiting for client\n");
    sleep(1);
  };
  // Create file
  int file_id;
  file_id = creat(dir, 0644);
  delete[] dir;

  int write_size, mod;
  int cycle_counter = 0;
  while(true)
  {
    mod = cycle_counter%8;
    control_sem.sem_op = -1;
		semop(sem_full,&control_sem, 1);
    write_size = write(file_id, shared_data->block_data[mod], shared_data->size_buff[mod]);
    control_sem.sem_op = 1;
    semop(sem_empty,&control_sem, 1);
    if(write_size <= 0)
    {
      printf("File saved.\n");
      break;
    }
    else
      printf("Package %i is accepted. Size: %i.\n", cycle_counter, write_size);
    cycle_counter++;
  }
  close(file_id);
  // Detaches the shared memory segment
  if (shmdt(shared_memory) == -1)
  {
    fprintf(stderr, "shmdt failed\n");
    exit(EXIT_FAILURE);
  }
  // Mark the segment to be destroyed.
  if (shmctl(shmid, IPC_RMID, 0) == -1)
  {
    fprintf(stderr, "shmctl(IPC_RMID) failed\n");
    exit(EXIT_FAILURE);
  }
    exit(EXIT_SUCCESS);
}
