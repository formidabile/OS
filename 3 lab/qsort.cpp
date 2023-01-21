#define _CRT_SECURE_NO_WARNINGS

#include <cstdint>
#include <queue>
#include <windows.h>
#include <stdio.h>

using namespace std;

typedef struct border_t
{
	unsigned long lo;
	unsigned long hi;
};

queue<struct border_t> queue_border;

unsigned long thread_count = 0;
unsigned long n = 0;
unsigned long long time = 0;

long* sorted = NULL;
HANDLE threads[64];
HANDLE sem;

DWORD WINAPI thread_entry(void* processParameter);
void qsort(unsigned long left, unsigned long right);
unsigned long find_index(unsigned long left, unsigned long right);

void init(void)
{
	FILE* f = fopen("D:\\программирование\\qsort\\input.txt", "r");
	fscanf(f, "%lu%lu", &thread_count, &n);
	sorted = new long[n];
	for (unsigned long i = 0; i < n; i++)
	{
		fscanf(f, "%ld", sorted + i);
	}
	fclose(f);
	f = NULL;

	sem = CreateSemaphore(0, 1, 1, 0);

	border_t elem = { 0, n - 1 };
	queue_border.push(elem);

	memset(&threads, 0, sizeof(threads));
	for (unsigned long i = 0; i < thread_count; i++)
		threads[i] = CreateThread(0, 0, thread_entry, 0, 0, 0);
}

void deinit(void)
{
	FILE* f = fopen("D:\\программирование\\qsort\\output.txt", "w");
	fprintf(f, "%lu\n%lu\n", thread_count, n);
	for (unsigned long i = 0; i < n; i++)
	{
		fprintf(f, "%ld ", sorted[i]);
	}
	fclose(f);

	f = fopen("D:\\программирование\\qsort\\time.txt", "w");
	fprintf(f, "%llu", time);
	fclose(f);
	f = NULL;

	CloseHandle(sem);

	for (unsigned long i = 0; i < thread_count; i++)
		CloseHandle(threads[i]);
}


DWORD WINAPI thread_entry(void* thread_param)
{
	long old_val = 0;
	while (true)
	{
		WaitForSingleObject(sem, INFINITE);
		if (!queue_border.size())
		{
			ReleaseSemaphore(sem, 1, &old_val);
			return 0;
		}

		struct border_t border = queue_border.front();
		queue_border.pop();

		if (queue_border.size() != 0)
			ReleaseSemaphore(sem, 1, &old_val);

		qsort(border.lo, border.hi);

		ReleaseSemaphore(sem, 1, &old_val);
	}
}

void qsort(unsigned long left, unsigned long right)
{
	if (left >= right)
		return;

	border_t border;
	unsigned long middle = find_index(left, right);

	if (right - middle > 4096)
	{
		border.lo = middle + 1;
		border.hi = right;
		queue_border.push(border);
	}
	else
		qsort(middle + 1, right);

	if (middle - left > 4096)
	{
		border.lo = left;
		border.hi = middle;
		queue_border.push(border);
	}
	else
		qsort(left, middle);
}

unsigned long find_index(unsigned long left, unsigned long right)
{
	long mid = sorted[(left + right) / 2];
	unsigned long low = left - 1;
	unsigned long high = right + 1;

	for (;;)
	{
		for (; ++low < right && sorted[low] < mid;);
		for (; --high > left && sorted[high] > mid;);

		if (low >= high)
			return high;

		long tmp = sorted[low];
		sorted[low] = sorted[high];
		sorted[high] = tmp;
	}
}

int main(void)
{
	init();
	time = GetTickCount64();
	WaitForMultipleObjects(thread_count, threads, TRUE, INFINITE);
	time = GetTickCount64() - time;
	deinit();

	delete[] sorted;

	return 0;
}

