
#include "tr_local.h"

// threads
#ifdef WIN32
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
//#include <mutex.h>
#endif



 // the array to store the entries
#define MAX_CUBEMAPSAVE 4096
static thr_CubemapSave_t arrayCubemapSave[MAX_CUBEMAPSAVE];

// the linked lists:
// These lists have references to both previous & next entries (for easy entry deletion).
static thr_CubemapSave_t *entry_CubemapSave = NULL;       // the 1st of the used entries
static thr_CubemapSave_t *avail_CubemapSave = NULL;       // the 1st of the unused entries


static HANDLE mutex; // windows

// thread status
#define THREAD_STATUS_DEAD        0       ///< Thread is dead or hasn't been started
#define THREAD_STATUS_RUNNING     1       ///< Thread is running
#define THREAD_STATUS_QUITTING    2       ///< The thread is being killed


/**
 * @var R2_ThreadStatus
 * @brief Status of the thread
 */
static int R2Thread_Status = THREAD_STATUS_DEAD;

/**
 * @var R2_QuitRequested
 * @brief Quit requested?
 */
static qboolean R2Thread_QuitRequested;


/* Function that sets the thread status when the thread dies. Since that is
 * system-dependent, it can't be done in the thread's main code.
 */
static void R2Thread_SetDead();

static void R2Thread_Wait();


// mutex for lock/unlocking the thread so data can only be manipulated by one piece of code at a time
#ifdef WIN32

static void R2Thread_Lock(void)
{
	WaitForSingleObject(mutex, INFINITE);
}

static void R2Thread_Unlock(void)
{
	ReleaseMutex(mutex);
}

#else // defined __linux__ || defined __APPLE__ || defined __FreeBSD__

//mutex
static void R2Thread_Lock(void)
{
	// todo for linux..
}

static void R2Thread_Unlock(void)
{
	// todo for linux..
}

#endif


void R2Thread_Stop(void)
{
	R2Thread_QuitRequested = qtrue;
	R2Thread_Wait();
}



//-----------------------------------------------------------------------------
//  linked list stuff


/*
 *   Initialize the linked lists to manage cubemap save-to-file
 */
void THR_Init_CubemapSave(void)
{
	int i;
	entry_CubemapSave = NULL;
	avail_CubemapSave = NULL;
	for (i = 0; i < MAX_CUBEMAPSAVE; i++)
	{
		arrayCubemapSave[i].prev = (i==0)? NULL : &arrayCubemapSave[i-1];
		arrayCubemapSave[i].next = avail_CubemapSave;
		avail_CubemapSave = &arrayCubemapSave[i];
	}
}


thr_CubemapSave_t* THR_AddProbeToSave(cubemapProbe_t *probe)
{
	thr_CubemapSave_t *entry;

	if (!avail_CubemapSave)
	{
		return NULL; // none available
	}

	// exit if it already exists in the list
	for (entry = entry_CubemapSave; entry; entry = entry->next)
	{
		if (entry->probe == probe)
		{
			return entry;
		}
	}

	// lock the data,
	// so it isn't manipulated from outside this thread, while we edit the list in this thread
	R2Thread_Lock();

	// link an entry
	entry                             = avail_CubemapSave;
	if (avail_CubemapSave->next)
	{
		avail_CubemapSave->next->prev = avail_CubemapSave->prev;
	}
	avail_CubemapSave                 = avail_CubemapSave->next;
	entry->prev                       = (entry_CubemapSave)? entry_CubemapSave->prev : NULL;
	entry->next                       = entry_CubemapSave;
	if (entry_CubemapSave)
	{
		entry_CubemapSave->prev       = entry;
	}
	entry_CubemapSave                 = entry;

	// set the probe
	entry->probe = probe;

	// unlock the data
	R2Thread_Unlock();

	return entry;
}

// remove 'entry' from the list.
// return the previous entry on exit.
// return NULL if the entry is NULL.
thr_CubemapSave_t* THR_RemoveProbeToSave(thr_CubemapSave_t *entry)
{
	thr_CubemapSave_t *result;
	int i;

	if (!entry)
	{
		return NULL;
	}

	//lock
	R2Thread_Lock();

	result = entry->prev;

	// link an entry
	if (entry->prev)
	{
		entry->prev->next             = entry->next;
	}
	if (entry->next)
	{
		entry->next->prev             = entry->prev;
	}
	if (entry == entry_CubemapSave)
	{
		entry_CubemapSave             = entry->next;
	}
	entry->prev                       = (avail_CubemapSave)? avail_CubemapSave->prev : NULL;
	entry->next                       = avail_CubemapSave;
	if (avail_CubemapSave)
	{
		avail_CubemapSave->prev       = entry;
	}
	avail_CubemapSave                 = entry;

	// release the probe's memory that stored the temporary pixeldata
	for (i = 0; i < 6; i++)
	{
		if (entry->probe->cubeTemp[i]) ri.Free(entry->probe->cubeTemp[i]);
		entry->probe->cubeTemp[i] = NULL;
	}
	// unset the probe
	entry->probe = NULL;

	// unlock
	R2Thread_Unlock();

	return result;
}


void THR_ProcessProbesToSave(void)
{
	thr_CubemapSave_t *entry;
	cubemapProbe_t *probe;

	for (entry = entry_CubemapSave; entry; entry = entry->next)
	{
		if (R2Thread_Status != THREAD_STATUS_RUNNING) return;
		probe = entry->probe;
		if (!probe) continue; // better return, because when that happens, it's bad..
		R_SaveCubeProbe(probe, probe->cubeTemp, qfalse); // qfalse means: save only this one
		// this entry is processed.
//		entry = THR_RemoveProbeToSave(entry); // the previous entry is returned so the loop will do the right thing..
		(void)THR_RemoveProbeToSave(entry);
	}
}



//-----------------------------------------------------------------------------
//  thread stuff

static void R2Thread_MainLoop()
{
	while (qtrue)
	{
		if (R2Thread_QuitRequested)
		{
			R2Thread_Status = THREAD_STATUS_QUITTING;
			return; // exit the mainloop
		}
		if (R2Thread_Status == THREAD_STATUS_RUNNING)
		{
			THR_ProcessProbesToSave();
		}
	}
}


static void R2Thread()
{
	R2Thread_Status = THREAD_STATUS_RUNNING;
	R2Thread_MainLoop();
	R2Thread_SetDead();
}


#ifdef WIN32
/****** THREAD HANDLING - WINDOWS VARIANT ******/

static HANDLE R2Thread_Handle = NULL;

static DWORD WINAPI R2Thread_SystemProc(LPVOID dummy)
{
	R2Thread();
	return 0;
}

void R2Thread_Start()
{
	THR_Init_CubemapSave();
	R2Thread_QuitRequested = qfalse;
	if (R2Thread_Handle == NULL)
	{
		R2Thread_Handle = CreateThread(NULL, 0, R2Thread_SystemProc, NULL, 0, NULL);
	}
}

static void R2Thread_Wait()
{
	if (R2Thread_Handle != NULL)
	{
		if (R2Thread_Status != THREAD_STATUS_DEAD)
		{
			WaitForSingleObject(R2Thread_Handle, 10000);
			CloseHandle(R2Thread_Handle);
		}
		R2Thread_Handle = NULL;
	}
}

static void R2Thread_SetDead()
{
	R2Thread_Status = THREAD_STATUS_DEAD;
	R2Thread_Handle = NULL;
}


#else // defined __linux__ || defined __APPLE__ || defined __FreeBSD__
/****** THREAD HANDLING - UNIX VARIANT ******/


static pthread_t R2Thread_Handle = (pthread_t) NULL;

static void *R2Thread_SystemProc(void *dummy)
{
	R2Thread();
	return NULL;
}

void R2Thread_Start(void)
{
	THR_Init_CubemapSave();
	R2Thread_QuitRequested = qfalse;
	if (R2Thread_Handle == (pthread_t) NULL)
	{
		pthread_create(&R2Thread_Handle, NULL, R2Thread_SystemProc, NULL);
	}
}

static void R2Thread_SetDead()
{
	R2Thread_Status = THREAD_STATUS_DEAD;
	R2Thread_Handle = (pthread_t) NULL;
}

static void R2Thread_Wait()
{
	if (R2Thread_Handle != (pthread_t) NULL)
	{
		if (R2Thread_Status != THREAD_STATUS_DEAD)
		{
			pthread_join(R2Thread_Handle, NULL);
		}
		R2Thread_Handle = (pthread_t) NULL;
	}
}

#endif
