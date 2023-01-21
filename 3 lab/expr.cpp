#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h> 
#include <math.h>

HANDLE mutex;
HANDLE threads[64];

int* numbers;
int count_threads, count, summary;
time_t all_time;

volatile int result = 0, task = 0;
int array_[64];

FILE* file;

DWORD WINAPI thread_entry(void* param)
{
	int sum, res = 0, index, j;

	WaitForSingleObject(mutex, INFINITE);
	index = task++;
	ReleaseMutex(mutex);

	j = array_[index];
	while (j < array_[index + 1])
	{
		int deg2 = pow((double)2, count - 2);
		int comb = j;
		sum = numbers[0];

		for (int i = 1; i < count; ++i, deg2 >>= 1)
		{
			if (comb < deg2)
				sum = sum - numbers[i];
			else
			{
				sum = sum + numbers[i];
				comb = comb - deg2;
			}

		}

		if (sum == summary) 
			res = res + 1;

		j += 1;
	}

	WaitForSingleObject(mutex, INFINITE);
	result += res;
	ReleaseMutex(mutex);

	return 0;
}

void expression(int index, int sign, int sum)
{
	if (sign) 
		sum = sum + numbers[index];
	else 
		sum = sum - numbers[index];

	result = (index + 1 == count && sum == summary) ? result + 1 : result;
	
	if (index + 1 != count)
	{
		expression(index + 1, sign, sum);
		expression(index + 1, 1 - sign, sum);
	}
}

void input()
{
	file = fopen("D:\\программирование\\expr\\input.txt", "r");
	fscanf(file, "%d", &count_threads);
	fscanf(file, "%d", &count);

	numbers = (int*)malloc((count - 1) * sizeof(int));

	for (int i = 0; i < count; i++)
		fscanf(file, "%d", &numbers[i]);

	fscanf(file, "%d", &summary);
	fclose(file);

}

void output()
{
	for (int i = 0; i < count_threads; i++)
		CloseHandle(threads[i]);
	CloseHandle(mutex);

	file = fopen("D:\\программирование\\expr\\output.txt", "w");
	fprintf(file, "%d\n%d\n%d", count_threads, count, result);
	fclose(file);

	file = fopen("D:\\программирование\\expr\\time.txt", "w");
	fprintf(file, "%lu", all_time);
	fclose(file);

}

void init_threads()
{
	mutex = CreateMutex(NULL, FALSE, NULL);
	
	for (int i = 0; i < count_threads; i++)
		threads[i] = CreateThread(0, 0, thread_entry, (void*)i, 0, 0);
}

void one_thread_case()
{
	all_time = GetTickCount64();

	expression(1, 1, numbers[0]);
	expression(1, 0, numbers[0]);

	all_time = GetTickCount64() - all_time;

	file = fopen("D:\\программирование\\expr\\output.txt", "w");
	fprintf(file, "%d\n%d\n%d", count_threads, count, result);
	fclose(file);

	file = fopen("D:\\программирование\\expr\\time.txt", "w");
	fprintf(file, "%lu", all_time);
	fclose(file);
}

int main()
{
	input();

	if (count_threads == 1)
		one_thread_case();

	else
	{
		int inter = (int)(pow((double)2, count - 1)) / count_threads;

		for (int i = 1; i < count_threads; i++)
			array_[i] = array_[i - 1] + inter;

		array_[count_threads] = pow((double)2, count - 1);

		init_threads();

		all_time = GetTickCount64();
		WaitForMultipleObjects(count_threads - 1, threads, TRUE, INFINITE);
		all_time = GetTickCount64() - all_time;

		output();
	}
	return 0;
}