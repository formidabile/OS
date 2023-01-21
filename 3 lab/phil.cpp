#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>

volatile int all_time, eat_time; 
bool phil[6]; 
pthread_t threads[5]; 
pthread_mutex_t mutex;
sem_t sem;

struct timespec start, end;

unsigned long ms(struct timespec* time)
{
	return ((unsigned long) time->tv_sec * 1000 +
		(unsigned long) time->tv_nsec / 1000000);
}

bool check(int i)
{
	if (i)
		return !phil[i - 1] && !phil[i + 1]? 1 : 0;
	return !phil[4] && !phil[1]? 1 : 0;
	
}

void* thread_entry(void* param)
{
	volatile int idx = ((char*)param - (char*)0);  
	while (1)
	{	
			clock_gettime(CLOCK_REALTIME, &end);
			if((ms(&end) - ms(&start))>=all_time)
			{
				return 0;
			}	
		
		if (!phil[idx])
		{
			if (check(idx))
			{
				if (!idx)
					phil[5] = 1;

				phil[idx] = 1;
				clock_gettime(CLOCK_REALTIME, &end);
        			printf("%lu:%d:T->E\n",(ms(&end) - ms(&start)),idx + 1);
				sem_wait(&sem);
				usleep(eat_time * 1000);
				sem_post(&sem);;
				clock_gettime(CLOCK_REALTIME, &end);
				if((ms(&end) - ms(&start)) <= all_time)
 					printf("%lu:%d:E->T\n",(ms(&end) - ms(&start)),idx + 1);
				phil[idx] = 0; 
				
				usleep(eat_time * 1000);
			}
		}

	}
	
}

int main(int argc, char const* argv[])
{
	all_time = atoi(argv[1]); 
	eat_time = atoi(argv[2]);
	pthread_mutex_init(&mutex, 0);
	sem_init(&sem, 0, 2);
	for (int i = 0; i < 5; i++)
		phil[i] = 0;
	for (int i = 0; i < 5; i++)
		 pthread_create(&threads[i], 0, thread_entry, (void* volatile)((char*)0 + i));

	clock_gettime(CLOCK_REALTIME, &start);  
	for(int i = 0; i < 5; i++)
		pthread_join(threads[i], 0); 
	sem_destroy(&sem);
	pthread_mutex_destroy(&mutex);
	return 0;
}



