#pragma once
#include "Common.h"
#include <Windows.h>
#include <vector>
#include <iostream>
#include <algorithm>
#include "MySynchronizationBarrierServer.h"


class ClientContext 
{
public:
    OVERLAPPED	ol;
	HANDLE		hPipe;
    int			nOpCode;

	char payload[BUFSIZE]; //todo

	ClientContext()
	{
		memset(&ol, 0, sizeof(ol));
	}
};

enum
{
	OP_CONNECT,
	OP_READ,
	OP_WRITE,
	OP_DISCONNECT
};

HANDLE g_moreConnectsEvent;
HANDLE g_hShutdownEvent;
HANDLE g_hIOCompletionPort;
HANDLE g_hAcceptThread;

unsigned g_workerThreadsCount;
std::vector<HANDLE> g_workerThreadsVec;
std::vector<ClientContext*> g_vecOverlapped;

bool initializeServer(HANDLE initialPipeHandle);
DWORD WINAPI AcceptThread(LPVOID lParam);
DWORD WINAPI WorkerThread(LPVOID lpParam);

void doAfterConnectAction(ClientContext * clientContext );
void doAfterReadAction(ClientContext * clientContext );
void doAfterWriteAction(ClientContext * clientContext );

CRITICAL_SECTION g_criticalSection;

void startIocpServer(){
	g_workerThreadsCount = 4; // todo


	std::cout << "Server: Creating main pipe"<<std::endl;
	auto listenPipe = CreateNamedPipe( 
		(LPCWSTR)MY_BARRIER_PIPE_NAME,             // pipe name 
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,       // read/write access 
		PIPE_TYPE_MESSAGE |       // message type pipe 
		PIPE_READMODE_MESSAGE |   // message-read mode 
		PIPE_WAIT,                // blocking mode 
		PIPE_UNLIMITED_INSTANCES, // max. instances  
		BUFSIZE,                  // output buffer size 
		BUFSIZE,                  // input buffer size 
		0,                        // client time-out 
		NULL);                    // default security attribute 
	if( check(listenPipe != INVALID_HANDLE_VALUE,"Server: CreateNamedPipe failed")){
		return;
	}

	if(check(initializeServer(listenPipe), "Server: initialization failed ")){
		return;
	}

	DWORD nThreadID;
	g_hAcceptThread = CreateThread(0, 0, AcceptThread, (void *)listenPipe, 0, &nThreadID);
}

bool initializeServer(HANDLE initialPipeHandle){
	if(check( InitializeCriticalSectionAndSpinCount(&g_criticalSection, 0x00000400), "Server: Critical section initialization failed" )){
		return false;
	}

	g_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if( check(g_hShutdownEvent != NULL, " Server: Creating shutdown event failed ")){
		return false; 
	}

	g_moreConnectsEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if( check(g_moreConnectsEvent != NULL, " Server: Creating more connects event failed ")){
		return false; 
	}

	g_hIOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0 );
	if( check(g_hIOCompletionPort != NULL, " Server: Creating Completion port failed ")){
		return false; 
	}
	CloseHandle(initialPipeHandle);

	for( auto i = 0u; i < g_workerThreadsCount; i++ ){
		DWORD nThreadID;
		g_workerThreadsVec.push_back(CreateThread(0, 0, WorkerThread, (void *)(i+1), 0, &nThreadID));
	}
	return true;
}

void RemoveClientContext(ClientContext *pOlp)
{
	std::cout << "YYY removing " << (int)pOlp->hPipe << std::endl;
	//CXCritSec::CLocker lock(m_csOlp); todo

	DisconnectNamedPipe(pOlp->hPipe);
	CloseHandle(pOlp->hPipe);
	delete pOlp;

	EnterCriticalSection(&g_criticalSection);
	g_vecOverlapped.erase(
		std::remove_if(g_vecOverlapped.begin(), g_vecOverlapped.end(),
		[=](const ClientContext * o) { return o == pOlp; }),
		g_vecOverlapped.end());
	LeaveCriticalSection(&g_criticalSection);
}

//This thread will look for accept event
DWORD WINAPI AcceptThread(LPVOID lParam)
{
	HANDLE arrHandles[2];

	arrHandles[0] = g_hShutdownEvent;
	arrHandles[1] = g_moreConnectsEvent;

	DWORD	dwWaitRes;
	HANDLE	hPipe;
	ClientContext* pOlp;

	std::cout << "Server : AcceptThread started " << std::endl;

	do
	{
		auto e1 = GetLastError();
		hPipe = CreateNamedPipe(
			MY_BARRIER_PIPE_NAME,
			PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_MESSAGE |
			PIPE_READMODE_MESSAGE,
			PIPE_UNLIMITED_INSTANCES,
			BUFSIZE,
			BUFSIZE,
			0,
			NULL);
		std::cout << "Server : CreateNamedPipe done hPipe " <<(int)hPipe << " error " << GetLastError() <<  std::endl;
		auto e2 = GetLastError();
		if(check(hPipe != INVALID_HANDLE_VALUE, "Server: Creating new pipe in acceptThread failed")){
			return false;
		}
		///////////////////////////////////////////////////////////
		// add the new pipe instance to the completion port
		auto res = CreateIoCompletionPort(hPipe, g_hIOCompletionPort, (ULONG_PTR) hPipe, 0);
		std::cout << "Server : CreateIoCompletionPort res " <<res << " error " << GetLastError() <<  std::endl;
		auto e3 = GetLastError();
		if( check(res != NULL, " Server: Adding new pipe to completion port failed ")){
			return false; 
		}

		pOlp = new ClientContext; //todo use make shared or sth
		pOlp->hPipe		= hPipe;
		pOlp->nOpCode	= OP_CONNECT;
		pOlp->ol.hEvent = g_moreConnectsEvent;

		EnterCriticalSection(&g_criticalSection);
		g_vecOverlapped.push_back(pOlp);
		LeaveCriticalSection(&g_criticalSection);

		BOOL connectRes = ConnectNamedPipe(hPipe, &pOlp->ol);
		std::cout << "Server : ConnectNamedPipe connectRes " <<connectRes << " error " << GetLastError() <<  std::endl;
		
		auto e4 = GetLastError();
		if( !connectRes){
			DWORD lastError = GetLastError();
			if (lastError == ERROR_PIPE_CONNECTED) // jak nie czekalismy na asynchronicznym ConnectNamedPipe
			{
				std::cout << "Server : ERROR_PIPE_CONNECTED " << " error " << GetLastError() <<  std::endl;
				PostQueuedCompletionStatus(g_hIOCompletionPort, 0, (ULONG_PTR)hPipe, &pOlp->ol);
				continue;
			} else if (lastError == ERROR_NO_DATA) //connected and disconnected before ConnectNamedPipe called
			{
				std::cout << "Server : ERROR_NO_DATA " << " error " << GetLastError() <<  std::endl;
				RemoveClientContext(pOlp);
				continue;
			} else if (lastError != ERROR_PIPE_LISTENING && lastError != ERROR_IO_PENDING)
			{
				std::cerr<<"Server: ConnectNamedPipe failed "<<lastError<<std::endl;
				RemoveClientContext(pOlp);
				continue;
			}
		}

		std::cout << "Server : Going to sleep " << " error " << GetLastError() <<  std::endl;
		dwWaitRes = WaitForMultipleObjects(2, arrHandles, FALSE, INFINITE);

		ResetEvent(g_moreConnectsEvent);
	} 
	while (dwWaitRes != WAIT_OBJECT_0);

	return 0;
}

void OnRead(ClientContext* pOlp)
{
	DWORD dwWritten;
	BarrierMessage request = *((BarrierMessage*)pOlp->payload) ;
	BarrierMessage response;
	bool shouldSend = processOnePipe(request, &response );
	if(shouldSend ){
		if (WriteFile(pOlp->hPipe,&response,BUFSIZE,&dwWritten, &pOlp->ol))
		{
			pOlp->nOpCode = OP_READ;
			PostQueuedCompletionStatus(g_hIOCompletionPort,	dwWritten, (ULONG_PTR)pOlp->hPipe, &pOlp->ol);
			std::cout << "OnRead POST 1  "<<std::endl;
		} else if (GetLastError() != ERROR_IO_PENDING)
		{
			//probably pipe is being closed, or pipe not connected
			RemoveClientContext(pOlp);
			std::cout << "OnRead POST 2  "<<std::endl;
		}
	} else {
		pOlp->nOpCode = OP_DISCONNECT;
		PostQueuedCompletionStatus(g_hIOCompletionPort,	dwWritten, (ULONG_PTR)pOlp->hPipe, &pOlp->ol);
		std::cout << "OnRead POST 3  "<<std::endl;
	}
}

ClientContext *getClientContext(HANDLE pipe){
	EnterCriticalSection(&g_criticalSection);
	ClientContext *toReturn = nullptr;
	for( auto aOverlapped : g_vecOverlapped ){
		if( aOverlapped->hPipe == pipe){
			toReturn =  aOverlapped;
		}
	}
	LeaveCriticalSection(&g_criticalSection);
	return toReturn;
}

//Worker thread will service IOCP requests
DWORD WINAPI WorkerThread(LPVOID lpParam)
{    
	std::cout << "ServerWT: WorkerThread started " << std::endl;
	int nThreadNo = (int)lpParam;

	void *lpContext = NULL;
	OVERLAPPED       *pOverlapped = NULL;
	ClientContext   *pClientContext = NULL;
	DWORD            dwBytesTransfered = 0;
	int nBytesRecv = 0;
	int nBytesSent = 0;
	DWORD             dwBytes = 0, dwFlags = 0;

	//Worker thread will be around to process requests, until a Shutdown event is not Signaled.
	while (WAIT_OBJECT_0 != WaitForSingleObject(g_hShutdownEvent, 0))
	{
		BOOL bReturn = GetQueuedCompletionStatus(
			g_hIOCompletionPort,
			&dwBytesTransfered,
			(LPDWORD)&lpContext,
			&pOverlapped,
			INFINITE);

		if( check(bReturn, "Server: Error in WorkerThread, GetQueuedCompletionStatus returned false")){
			continue;	
		}
		//std::cout << "ServerWT:  GetQueuedCompletionStatus.  bytesTran " << dwBytesTransfered << " lpContext "<< lpContext << std::endl;

		pClientContext = getClientContext((HANDLE)lpContext);
		std::cout << "ServerWT : GETTING with opcode "<<pClientContext->nOpCode<<" handle "<<(int)lpContext<<" transfered "<<dwBytesTransfered<<" errno " << GetLastError() << std::endl;
		if(check(pClientContext != nullptr, "Server: Error in WorkerThread, cant find client context")){
			continue;
		}

		switch (pClientContext->nOpCode){
		case OP_CONNECT:
			doAfterConnectAction(pClientContext);
			break;
		case OP_READ:
			doAfterReadAction(pClientContext);
			break;
		case OP_WRITE:
			doAfterWriteAction(pClientContext);
			break;
		case OP_DISCONNECT:
			RemoveClientContext(pClientContext);
			break;
		}
	} // while

	return 0;
}


void doAfterConnectAction(ClientContext * clientContext ){
	DWORD dwBytesRead;
	clientContext->nOpCode = OP_READ;
	auto readFileRes = ReadFile(clientContext->hPipe,	clientContext->payload,	BUFSIZE, &dwBytesRead, &clientContext->ol);
	if( check((readFileRes != 0 ) || (readFileRes == 0 && GetLastError() == ERROR_IO_PENDING), "ServerWT: readFile failed") ){
		clientContext->nOpCode = OP_DISCONNECT;
		PostQueuedCompletionStatus(g_hIOCompletionPort,	0, (ULONG_PTR)clientContext->hPipe, &clientContext->ol);
	}
}

void doAfterReadAction(ClientContext * clientContext ){
	std::thread replyingThread([=](){
		BarrierMessage request = *( (BarrierMessage*)clientContext->payload);
		BarrierMessage response;
		bool res = processOnePipe(request, &response);
		if( !res ){
			clientContext->nOpCode = OP_DISCONNECT;
			PostQueuedCompletionStatus(g_hIOCompletionPort,	0, (ULONG_PTR)clientContext->hPipe, &clientContext->ol);
		} else {
			clientContext->nOpCode = OP_WRITE;
			DWORD dwWritten;
			auto writeRes = WriteFile(clientContext->hPipe,&response,BUFSIZE,&dwWritten, &clientContext->ol);
			if( check((writeRes != 0 ) || (writeRes == 0 && GetLastError() == ERROR_IO_PENDING), "ServerWT: writeFile failed") ){
				clientContext->nOpCode = OP_DISCONNECT;
				PostQueuedCompletionStatus(g_hIOCompletionPort,	0, (ULONG_PTR)clientContext->hPipe, &clientContext->ol);
			}
		}
	});
	replyingThread.detach();
}

void doAfterWriteAction(ClientContext * clientContext ){
	clientContext->nOpCode = OP_CONNECT;
	PostQueuedCompletionStatus(g_hIOCompletionPort,	0, (ULONG_PTR)clientContext->hPipe, &clientContext->ol);
}