#pragma once
#include <Windows.h>
#include <memory>
#include "MyClientNamedPipe.h"
#include <thread>

class ClientBarrierData{
public:
	ClientBarrierData( HANDLE gWakeupEvent, std::shared_ptr<std::thread> messageDownReciever, std::shared_ptr<MyClientNamedPipe> communicationPipe, int inBarrierId, std::string barrierName)
		: gWakeupEvent(gWakeupEvent), messageDownReciever(messageDownReciever), communicationPipe(communicationPipe), barrierId(inBarrierId), barrierName(barrierName){
	}

	ClientBarrierData(){}

	HANDLE gWakeupEvent; 
	std::shared_ptr<std::thread> messageDownReciever; 
	std::shared_ptr<MyClientNamedPipe> communicationPipe;
	int barrierId;
	std::string barrierName;

	void close(){
		if( check(communicationPipe->sendCloseRequest(barrierId),"Client: Sending close request failed")){
			messageDownReciever->detach();
		} else {
			messageDownReciever->join();
			communicationPipe->close();
		}		
		CloseHandle(gWakeupEvent);
	}
};
