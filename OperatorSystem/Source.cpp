#include <Windows.h>
#include <iostream>
#include <cstdio>
#include <queue>
using namespace std;
const int MAX = 5;
int ThreadID = 0;
struct node//�����̺߳����Ĳ���
{
	int number;//��͵ķ�Χ
	int ID;//�̵߳ı��
};
class ThreadInfo//�̵߳������Ϣ
{
public:
	HANDLE hThread;//���
	int time;//�߳��������Ҫ��ʱ��
	node *InfoToCal;//nodeָ��
	ThreadInfo()
	{
		hThread = nullptr;
		time = 0;
		InfoToCal = nullptr;
	}
	ThreadInfo(HANDLE hThread, int time, node *InfoToCal)
	{
		this->hThread = hThread;
		this->time = time;
		this->InfoToCal = InfoToCal;
	}
};
//ѭ������
template<typename T>
class Queue
{
	int head;
	int rear;
	int capcity;
	int size = 0;
public:
	Queue(int capcity = 0)
	{
		this->capcity = capcity;
		this->head = 0;
		this->rear = 0;
		this->queue = (T*)new T[capcity];
	}
	bool push(T node)
	{
		if (size >= capcity)
			return false;
		queue[rear++] = node;
		rear %= capcity;
		size++;
		return true;
	}
	bool pop()
	{
		if (this->size <= 0) return false;
		head++;
		head %= capcity;
		this->size--;
	}
	T front()
	{
		return queue[head];
	}
	bool empty()
	{
		return this->size == 0;
	}
	bool full()
	{
		return this->capcity == this->size;
	}
	T *queue;
};
Queue<ThreadInfo> que(10);
//����͵��߳�
DWORD WINAPI Calculate(LPVOID lpParam)
{
	node t = *(node*)lpParam;
	int ans = 0;
	for (int i = 1;i <= t.number;i++)
	{
		ans += i;
	}
	cout << "Thread ID :" << t.ID << endl;
	cout << "sum is " << ans << endl;
	return 0;
}
//���ݿ���̨�������������߳�
DWORD WINAPI Read(LPVOID lpParam)
{
	while (true)
	{
		HANDLE hThread = NULL;
		DWORD dwThread;
		int time;
		node *InfoToCal;
		if (que.full())
		{
			Sleep(200);
			continue;
		}
		InfoToCal = (node*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(node));
		cout << "enter TIME NUMBER to create a new thread" << endl;
		cin >> time >> InfoToCal->number;
		InfoToCal->ID = ThreadID++;
		time *= 1000;
		hThread = CreateThread(
			nullptr,
			0,
			Calculate,
			InfoToCal,
			CREATE_SUSPENDED,
			&dwThread
		);
		ThreadInfo tmp(hThread, time, InfoToCal);
		que.push(tmp);
	}
	return 0;
}
int main()
{
	cout << "Time Slice" << endl;
	HANDLE hRead = nullptr;
	DWORD dwRead;
	hRead = CreateThread(
		nullptr,
		0,
		Read,
		nullptr,
		0,
		&dwRead
	);
	//�����е�ÿ���̷ֱ߳�����40ms
	while (true)
	{
		ThreadInfo tmp = que.front();
		que.pop();
		Sleep(40);
		tmp.time -= 40;
		if (tmp.time <= 0) //�������
		{
			ResumeThread(tmp.hThread);
			WaitForSingleObject(tmp.hThread, INFINITE);
			HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, tmp.InfoToCal);
			CloseHandle(tmp.hThread);
		}
		else
			que.push(tmp);
	}
	return 0;
}
