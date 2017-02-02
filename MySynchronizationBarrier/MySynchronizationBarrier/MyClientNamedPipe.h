#pragma once
#include "MyNamedPipe.h"
#include <Windows.h>

class MyClientNamedPipe  : public MyNamedPipe{
public:
	MyClientNamedPipe(LPCWSTR pipeName) : MyNamedPipe(pipeName){
	}

	bool sendCreateBarrierRequest(const char *name, int threadCount){
		BarrierMessage message;
		message.setBarrierName(name);
		message.setHeader(BarrierMessageHeader::REQ_CREATE_BARRIER);
		message.setThreadCount(threadCount);
		memcpy(buffer,&message,BUFSIZE);
		return sendMessage();
	}

	bool sendAchievedBarrierMessage(int barrierId){
		BarrierMessage message;
		message.setBarrierId(barrierId);
		message.setHeader(BarrierMessageHeader::REQ_THREAD_ACHIEVED_BARRIER);
		memcpy(buffer,&message,BUFSIZE);
		return sendMessage();
	}

	bool sendAnnounceBarrierDownListeningPipe(int barrierId){
		BarrierMessage message;
		message.setBarrierId(barrierId);
		message.setHeader(BarrierMessageHeader::REQ_ANNOUNCE_BARRIER_DOWN_LISTENING_PIPE);
		memcpy(buffer,&message,BUFSIZE);
		return sendMessage();
	}

	bool sendCloseRequest(int barrierId){
		BarrierMessage message;
		message.setBarrierId(barrierId);
		message.setHeader(BarrierMessageHeader::REQ_CLOSE_BARRIER);
		memcpy(buffer,&message,BUFSIZE);
		return sendMessage();
	}

	bool initPipe(){
		while(1){
			pipeHandle = CreateFile( 
				(LPCWSTR)pipeName,   // pipe name 
				GENERIC_READ |  // read and write access 
				GENERIC_WRITE, 
				0,              // no sharing 
				NULL,           // default security attributes
				OPEN_EXISTING,  // opens existing pipe 
				0,              // default attributes 
				NULL);          // no template file 
			if (pipeHandle != INVALID_HANDLE_VALUE) 
				break; 
 
			// Exit if an error other than ERROR_PIPE_BUSY occurs. 
			
			if( check(GetLastError() == ERROR_PIPE_BUSY, "Could not open pipe")){
				return false;
			}
			if( check(WaitNamedPipe((LPCWSTR)pipeName, 20000), "Could not open pipe: 20 second wait timed out.")){
				return false;
			}
		}
		BOOL   fSuccess = FALSE; 
		DWORD dwMode = PIPE_READMODE_MESSAGE; 

		fSuccess = SetNamedPipeHandleState( 
			pipeHandle,    // pipe handle 
			&dwMode,  // new pipe mode 
			NULL,     // don't set maximum bytes 
			NULL);    // don't set maximum time 

		if( check(fSuccess, "SetNamedPipeHandleState failed.")){
			return false;
		} 
		return true;
	}

	void close(){
		DisconnectNamedPipe(pipeHandle);
		MyNamedPipe::close();
	}
};
