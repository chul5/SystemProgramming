#include "Thread.h"
#include "Init.h"
#include "Scheduler.h"
#include "deque.h"

t_deque		readyQueue = {0};
t_deque		waitQueue = {0};

Thread*     ReadyQHead = NULL;
Thread*     ReadyQTail = NULL;

Thread*     WaitQHead = NULL;
Thread*     WaitQTail = NULL;

pthread_mutex_t readyMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t readyCond = PTHREAD_COND_INITIALIZER;
pthread_cond_t zombieCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t zombieMutex = PTHREAD_MUTEX_INITIALIZER;

void tcbBlockInit(Thread *block)
{
	block->status = THREAD_STATUS_READY;
	block->readyCond = readyCond;
	block->bRunnable = 0;
	block->readyMutex = readyMutex;
	block->parentTid = pthread_self();
	block->zombieCond = zombieCond;
	block->zombieMutex = zombieMutex;
	block->bZombie = 1;
}

void *wrapperFunc(void *arg){
	WrapperArg *pArg = (WrapperArg *)arg;
	Thread *pTh = pArg->pThread;
	pTh->tid = pthread_self();
	append_left(&readyQueue, pTh); // ready queue에 삽입.
	_thread_to_ready2(pTh); // 생성된 쓰레드를 최초 1회 ready2함수를 호출한다. 
	return pArg->funcPtr(pArg->funcArg);
}

int 	thread_create(thread_t *thread, thread_attr_t *attr, void *(*start_routine) (void *), void *arg)
{
	//tcb 생성해야함.
	Thread tcbBlock;
	WrapperArg args;

	tcbBlockInit(&tcbBlock);
	args.funcPtr = start_routine;
	args.funcArg = arg;
	args.pThread = &tcbBlock;
	pthread_create(thread, 0, wrapperFunc, &args);
	return 0;
}

t_node *findTcbBlock(thread_t tid){
	t_node *node = readyQueue.top;
	while (node){
		if (node->data->tid == tid)
			return node;
		node = node->next;
	}
	return 0;
}

int 	thread_join(thread_t thread, void **retval)
{
	
}


int 	thread_suspend(thread_t tid)
{
	t_node *node = findTcbBlock(tid);
	Thread *data;
	if (!node)
	{
		node->prev->next = node->next;
		node->next->prev = node->prev;
		data = node->data;
		data->status = THREAD_STATUS_BLOCKED; // Change status.
		free(node);
		append_left(&waitQueue, data);
		return 0;
	}
	return -1;
}


int	thread_resume(thread_t tid)
{
	t_node *node = findTcbBlock(tid);
	Thread *data;
	if (!node)
	{
		node->prev->next = node->next;
		node->next->prev = node->prev;
		data = node->data;
		data->status = THREAD_STATUS_READY;
		free(node);
		append_left(&readyQueue, data);
		return 0;
	}
	return -1;
}

int             thread_exit(void* retval)
{
	pthread_exit(retval);
	return 0;
}

thread_t	thread_self()
{
	return pthread_self();
}
