#define _CRT_SECURE_NO_WARNINGS

#include <pthread.h> 
#include <stdio.h> 
#include <errno.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h> 
#include <sys/time.h>

unsigned long thread_count;
unsigned long size;

int* sorted = NULL;
pthread_t threads[64];
sem_t sem;

unsigned char size4 = 0;
int part = 0;

void* thread_entry(void*);

void merge(int left, int mid, int right)
{
	int i, j, k; 
	int* first = (int*)malloc(4 * (mid - left + 1));
	int* last = (int*)malloc(4 * (right - mid));

	for (i = 0; i < (mid - left + 1); i++) 
  	first[i] = sorted[left + i]; 
  for (j = 0; j < (right - mid); j++) 
    last[j] = sorted[mid + 1+ j]; 
   

    for (i = 0, j = 0, k = left; i < (mid - left + 1) && j < (right - mid); k++) 
    { 
        if (first[i] <= last[j]) 
        { 
            sorted[k] = first[i]; 
            i++; 
        } 
        else
        { 
            sorted[k] = last[j]; 
            j++; 
        }  
    } 
  
    for (;i < (mid - left + 1); k++) 
    { 
        sorted[k] = first[i]; 
        i++; 
    } 
  
    for (;j < (right - mid); k++) 
    { 
        sorted[k] = last[j]; 
        j++; 
    } 

    free(first);
    free(last);
}

void msort(int left, int right)
{
	if (left >= right)
			return; 
        
    msort(left, left + (right - left) / 2); 
    msort(left + (right - left) / 2 + 1, right);
 
    merge(left, left + (right - left) / 2, right); 
}

void* thread_entry(void* arg)
{
	sem_wait(&sem);

	sem_post(&sem);

	int left = part * (size / size4);
	int right = (++part) * (size / size4) - 1;

	if (left >= right)
		return 0;

	msort(left, left + (right - left) / 2);
	msort(left + (right - left) / 2 + 1, right);
	merge(left, left + (right - left) / 2, right);

	return 0;
}

int main(void)
{
	FILE* file = fopen("input.txt", "r");
	fscanf(file, "%ld%ld", &thread_count, &size);
	sorted = (int*)malloc(4 * size);
	for (unsigned int i = 0; i < size; i ++)
		fscanf(file, "%d", &sorted[i]);
	fclose(file);

	clock_t time = clock();

	bool mod4 = size % 4;
	size4 = mod4 ? 1 : 4;

	sem_init(&sem, 1, 1);
	memset(&threads, 0, sizeof(threads));
	for (unsigned char i = 0; i < size4; i++)
		pthread_create(&threads[i], 0, thread_entry, 0);

	sem_destroy(&sem);

	for (unsigned char i = 0; i < size4; i++)
		pthread_join(threads[i], 0);

	if (!mod4)
	{
		merge(0, (size / 2 - 1) / 2, size / 2 - 1);
		merge(size / 2, size / 2 + (size - 1 - size / 2) / 2, size - 1);
		merge(0, (size - 1) / 2, size - 1);
	}

	time = 1000 * (clock() - time) / CLOCKS_PER_SEC;

	file = fopen("output.txt", "w");
	fprintf(file, "%ld\n%ld\n", thread_count, size);
	for (unsigned i = 0; i < size; i++)
		fprintf(file, "%d ", sorted[i]);

	fclose(file);
	free(sorted);

	file = fopen("time.txt", "w");
	fprintf(file, "%ld", time);
	fclose(file);

}
