#pragma once
#include "BarrierMessage.h"
#include <memory>
#include "MyServerNamedPipe.h"
#include "ServerBarrierVec.h"

ServerBarrierVec serverBarrierVec;

bool processOnePipe(BarrierMessage request, BarrierMessage *response){
	if( request.getHeader()  == BarrierMessageHeader::REQ_CREATE_BARRIER){
		std::string barrierName(request.getBarrierName());
		serverBarrierVec.myEnterCriticalSection();
		int idx = serverBarrierVec.getBarrierOfName(barrierName);
		if( idx != -1 ){
			ServerSynchronizationBarrier barrier;
			serverBarrierVec.getVecElement(idx, &barrier);
			if( barrier.initialThreadCount != request.getThreadCount() ){
				response->setHeader(BarrierMessageHeader::RES_ERR_BARRIER_ARLEADY_CREATED_WRONG_INITIAL_THREAD_COUNT);
			} else{
				response->setHeader(BarrierMessageHeader::RES_BARRIER_ARLEADY_CREATED);
				response->setBarrierId(barrier.barrierId);
			}
		} else {
			ServerSynchronizationBarrier barrier;
			barrier.barrierName = std::string(request.getBarrierName());
			barrier.barrierId = serverBarrierVec.getNextBarrierId();
			barrier.initialThreadCount = request.getThreadCount();
			barrier.currentThreadCount = request.getThreadCount();
			serverBarrierVec.addBarrier(barrier);

			response->setBarrierId(barrier.barrierId);
			response->setHeader(BarrierMessageHeader::RES_NEW_BARRIER_CREATED);
		}
		serverBarrierVec.myLeaveCriticalSection();
		return true;
	} else if ( request.getHeader()  ==BarrierMessageHeader::REQ_THREAD_ACHIEVED_BARRIER){
		serverBarrierVec.myEnterCriticalSection();
		int idx = serverBarrierVec.getBarrierOfId(request.getBarrierId());
		if(idx != -1){
			ServerSynchronizationBarrier barrier;
			serverBarrierVec.getVecElement(idx, &barrier);
			barrier.currentThreadCount -= 1;
			serverBarrierVec.setElement(idx, barrier);
			response->setHeader(BarrierMessageHeader::RES_BARRIER_UP);

			if( barrier.currentThreadCount <= 0 ){	
				check( SetEvent(barrier.openBarrierWakeupEvent ), "Server: Setting event failed");
			}		   
		} else {
			response->setHeader(BarrierMessageHeader::RES_ERR_BARRIER_NOT_FOUND);
		}
		serverBarrierVec.myLeaveCriticalSection();	   
		return true;
	} else if(  request.getHeader()  == REQ_ANNOUNCE_BARRIER_DOWN_LISTENING_PIPE ){
		serverBarrierVec.myEnterCriticalSection();
		int idx = serverBarrierVec.getBarrierOfId(request.getBarrierId());
		if(idx != -1){
			ServerSynchronizationBarrier barrier;
			serverBarrierVec.getVecElement(idx, &barrier);
			if( barrier.openBarrierWakeupEvent == BAD_HANDLE){
				HANDLE gWakeupEvent = CreateEvent( 
					NULL,               // default security attributes
					FALSE,               // auto-reset event
					FALSE,              // initial state is nonsignaled
					NULL				// object name
					); 
				check( gWakeupEvent != NULL, "Server: creating event failed");
				barrier.openBarrierWakeupEvent = gWakeupEvent;
				barrier.hasSignalingThreadWaiting = true;
				serverBarrierVec.setElement(idx, barrier);
			}
			serverBarrierVec.myLeaveCriticalSection();
			WaitForSingleObject(   barrier.openBarrierWakeupEvent, INFINITE);  

			serverBarrierVec.myEnterCriticalSection();
			int idx = serverBarrierVec.getBarrierOfId(request.getBarrierId());
			serverBarrierVec.getVecElement(idx, &barrier);
					   
			if( !barrier.wasClosed ){ // was not closed, was woken becouse of barrier down
				response->setHeader(BarrierMessageHeader::RES_BARRIER_DOWN);
				barrier.hasSignalingThreadWaiting = false;
				serverBarrierVec.setElement(idx, barrier);

			} else { // we have to do cleanup
				response->setHeader(BarrierMessageHeader::RES_BARRIER_CLOSED);
				barrier.close();
				serverBarrierVec.removeElement(idx);
			}
			serverBarrierVec.myLeaveCriticalSection();
		} else {
			response->setHeader(BarrierMessageHeader::RES_ERR_BARRIER_NOT_FOUND);
			serverBarrierVec.myLeaveCriticalSection();
		} 
		return true;
	} else if(  request.getHeader()  == BarrierMessageHeader::REQ_CLOSE_BARRIER ){
		bool toReturn;
		serverBarrierVec.myEnterCriticalSection();
		int idx = serverBarrierVec.getBarrierOfId(request.getBarrierId());
		if(idx != -1){
			ServerSynchronizationBarrier barrier;
			serverBarrierVec.getVecElement(idx, &barrier);
			if( barrier.hasSignalingThreadWaiting ){ // lets tell signalling thread to wake up
				check( SetEvent(barrier.openBarrierWakeupEvent ), "Server: Setting event from closing thread failed");
				barrier.wasClosed = true;
				serverBarrierVec.setElement(idx, barrier);
			} else { //we do cleanup
				barrier.close();
				serverBarrierVec.removeElement(idx);
			}
			toReturn = false;
		} else {
			response->setHeader(BarrierMessageHeader::RES_ERR_BARRIER_NOT_FOUND);
			toReturn = true;
		}
		serverBarrierVec.myLeaveCriticalSection();
		return toReturn;
	}
	return true;
}

