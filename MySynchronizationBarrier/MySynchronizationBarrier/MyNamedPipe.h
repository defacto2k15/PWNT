#pragma once
#include <Windows.h>
#include "Common.h"
#include "BarrierMessage.h"

class MyNamedPipe{
	DWORD lastError;
public:
	CHAR buffer[BUFSIZE];
	HANDLE pipeHandle;
	LPCWSTR pipeName;

	MyNamedPipe(LPCWSTR pipeName) : pipeHandle(BAD_HANDLE), pipeName(pipeName){
	}

	bool sendMessage(){
		if(check(assertHandleIsInitialized(), "Client: pipe handle not initialized ")){
			return false;
		}
		BOOL   fSuccess = FALSE;
	// Send a message to the pipe server.  
		DWORD cbToWrite = BUFSIZE;//(strlen(buffer)+1)*sizeof(CHAR);
		DWORD cbWritten;

		fSuccess = WriteFile( 
			pipeHandle,                  // pipe handle 
			buffer,             // message 
			cbToWrite,              // message length 
			&cbWritten,             // bytes written 
			NULL);                  // not overlapped 

		lastError = GetLastError();

		if( check(fSuccess, "Client: WriteFile to pipe failed")){
			return false;
		}

		if( check(cbWritten == cbToWrite, "Client: Sending: not all bytes were written") ){
			return false;
		}
		FlushFileBuffers(pipeHandle); 
		return true;
	}

	bool recvMessage(){
		if(check(assertHandleIsInitialized(), "Client: pipe handle not initialized ")){
			return false;
		}
		BOOL   fSuccess = FALSE;
		DWORD cbRead;
		do 
		{ 
		// Read from the pipe. 
			fSuccess = ReadFile( 
				pipeHandle,    // pipe handle 
				buffer,    // buffer to receive reply 
				BUFSIZE*sizeof(CHAR),  // size of buffer 
				&cbRead,  // number of bytes read 
				NULL);    // not overlapped 
 
			if ( ! fSuccess && GetLastError() != ERROR_MORE_DATA )
				break; 
 
		} while ( ! fSuccess);  // repeat loop if ERROR_MORE_DATA 

		lastError = GetLastError();

		return !(!fSuccess);
	}

	void close(){
		if( !(pipeHandle==BAD_HANDLE)){
			CloseHandle(pipeHandle);
		}
	}

	bool getResponse(BarrierMessage *message){
		bool res = recvMessage();
		if( !res){
			return false;
		}

		*message = *( (BarrierMessage*)buffer);
		return true;
	}

	bool pipeWasBroken(){
		return lastError == ERROR_BROKEN_PIPE;
	}

private:
	bool assertHandleIsInitialized(){
		return pipeHandle != BAD_HANDLE;
	}
};