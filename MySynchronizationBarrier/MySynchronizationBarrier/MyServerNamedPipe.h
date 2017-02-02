#pragma once
#include "MyNamedPipe.h"

class MyServerNamedPipe  : public MyNamedPipe{
public:
	MyServerNamedPipe(LPCWSTR pipeName) : MyNamedPipe(pipeName){
	}

	bool sendStandardResponse(BarrierMessageHeader header, int barrierId){
		BarrierMessage message = BarrierMessage::createHeaderAndBarrierIdMessage(header, barrierId);
		return sendResponse(message);
	}

	bool sendResponse(BarrierMessage &message){
		memcpy(buffer,&message,BUFSIZE);
		return sendMessage();
	}

	bool initPipe(){
		pipeHandle = CreateNamedPipe( 
			(LPCWSTR)pipeName,             // pipe name 
			PIPE_ACCESS_DUPLEX,       // read/write access 
			PIPE_TYPE_MESSAGE |       // message type pipe 
			PIPE_READMODE_MESSAGE |   // message-read mode 
			PIPE_WAIT,                // blocking mode 
			PIPE_UNLIMITED_INSTANCES, // max. instances  
			BUFSIZE,                  // output buffer size 
			BUFSIZE,                  // input buffer size 
			0,                        // client time-out 
			NULL);                    // default security attribute 

		if( check(pipeHandle != INVALID_HANDLE_VALUE,"Server: CreateNamedPipe failed")){
			return false;
		}

 
		// Wait for the client to connect; if it succeeds, 
		// the function returns a nonzero value. If the function
		// returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 
 
		BOOL fConnected = ConnectNamedPipe(pipeHandle, NULL) ? 
			TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 
 
		if(check(fConnected,"Server:ConnectNamedPipe failed")){
			return false;
		}
		return true;
	}
};
