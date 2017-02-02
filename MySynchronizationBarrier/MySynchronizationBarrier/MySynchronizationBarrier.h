#pragma once

#include "Common.h"
#include "AllBarierDatas.h"
#include <iostream>

static AllBarierDatas allDatas;

HANDLE CreateSynchronizationBarrier(const char *barrierName, int threadCount){
	return allDatas.createNewBarrier(barrierName, threadCount);
}

bool WaitForEvent(HANDLE barrierHandle, DWORD dwTimeoutMiliseconds = INFINITE){
	ClientBarrierData clientData;
	if( check(allDatas.getBarrier(barrierHandle, &clientData), "Barrier of given handle not found")){
		return false;
	}
	
	clientData.communicationPipe->sendAchievedBarrierMessage(clientData.barrierId);
	BarrierMessage response;
	if( check(clientData.communicationPipe->getResponse(&response)," Client:getting response from waitForEvent failed ")){
		return false;
	}

	if(response.getHeader() == BarrierMessageHeader::RES_BARRIER_DOWN ){
		return true;
	} else if( response.getHeader() == BarrierMessageHeader::RES_BARRIER_UP ){
		DWORD res = WaitForSingleObject( clientData.gWakeupEvent,	dwTimeoutMiliseconds);    
		if( check(res != WAIT_FAILED, "Client: waiting for barrier down failed")){
			return false;
		}
		return true;
	} else {
		std::cerr << "Client: bad waitForEvent response header " << std::endl;
		return false;
	}
}

void Close(HANDLE barrierHandle){
	ClientBarrierData clientData;
	if(check( allDatas.getBarrier(barrierHandle, &clientData), "Barrier of given handle not found") ){
		return;
	}
	clientData.close();
	allDatas.removeBarrier(barrierHandle);
}
