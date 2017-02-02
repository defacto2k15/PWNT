#pragma once
#include "VectorWithCriticalSection.h"
#include "ClientBarrierData.h"
#include <algorithm>
#include <iostream>

class AllBarierDatas : public VectorWithCriticalSection<ClientBarrierData>{
public:
	bool getBarrier( HANDLE barrierHandle, ClientBarrierData *data){
		myEnterCriticalSection();
		for( auto &oneElem : vec ){
			if( oneElem.barrierId == (int)barrierHandle){
				*data = oneElem;
				myLeaveCriticalSection();
				return true;
			}
		}
		myLeaveCriticalSection();
		return false;
	}

	bool getBarrierOfName( const char *name, ClientBarrierData *data){
		myEnterCriticalSection();
		for( auto &oneElem : vec ){
			if( oneElem.barrierName == std::string(name)){
				*data = oneElem;
				myLeaveCriticalSection();
				return true;
			}
		}
		myLeaveCriticalSection();
		return false;
	}

	void removeBarrier(HANDLE barrierHandle){
		myEnterCriticalSection();
		vec.erase(
			std::remove_if(vec.begin(), vec.end(), [barrierHandle](const ClientBarrierData &data){ return data.barrierId==(int)barrierHandle;}),
			vec.end());
		myLeaveCriticalSection();
	}

	HANDLE createNewBarrier(const char *barrierName, int threadCount ){
		ClientBarrierData preexistingData;
		if( getBarrierOfName(barrierName, &preexistingData) ){
			return (HANDLE)preexistingData.barrierId;
		}

		auto createdPipe = createPipeToServer(MY_BARRIER_PIPE_NAME);
		if( check( createdPipe, "Client: Pipe creation failed")){
			return BAD_HANDLE;
		}

		HANDLE gWakeupEvent = CreateEvent( 
			NULL,               // default security attributes
			TRUE,               // manual-reset event
			FALSE,              // initial state is nonsignaled
			TEXT("WriteEvent")  // object name
			); 
		if( check(gWakeupEvent != NULL, " CreateEvent failed" )){
			createdPipe->close();
			return BAD_HANDLE;
		}

		if( check(createdPipe->sendCreateBarrierRequest(barrierName, threadCount), "Clinet: CreateBarrierRequest failed ")){
			createdPipe->close();
			return BAD_HANDLE;
		}

		BarrierMessage response;
		if( check(createdPipe->getResponse(&response), "Client: get response failed") ){
			createdPipe->close();
			CloseHandle(gWakeupEvent);
			return BAD_HANDLE;
		}

		if( check( response.getHeader() == RES_NEW_BARRIER_CREATED || response.getHeader() == RES_BARRIER_ARLEADY_CREATED, "Client: bad response header")){
			createdPipe->close();
			CloseHandle(gWakeupEvent);
			return BAD_HANDLE;
		}

		auto barrierDownListeningPipe = createPipeToServer(MY_BARRIER_PIPE_NAME);
		if( check( barrierDownListeningPipe, "Client: BarrierDownListeningPipe creation failed")){
			createdPipe->close();
			CloseHandle(gWakeupEvent);
			return BAD_HANDLE;
		}

		int barrierId = response.getBarrierId();

		if( check(barrierDownListeningPipe->sendAnnounceBarrierDownListeningPipe(barrierId), "Client: announceCreateBarrierDownListeningPipe failed")){
			createdPipe->close();
			barrierDownListeningPipe->close();
			CloseHandle(gWakeupEvent);
			return BAD_HANDLE;
		}

		std::shared_ptr<std::thread> barierDownListeningPipe = std::make_shared<std::thread>( [barrierDownListeningPipe, gWakeupEvent, barrierId](){
			BarrierMessage response;
			for(;;){
				if( check(barrierDownListeningPipe->getResponse(&response), "Client: barrierDownListenerPipe getting response failed ")){
					barrierDownListeningPipe->close();
					break;
				}
				if( response.getHeader() == BarrierMessageHeader::RES_BARRIER_DOWN ){
					check( SetEvent(gWakeupEvent), "barierDownListeningPipe: SetEvent failed");
					barrierDownListeningPipe->close();
					break;
				} else if( response.getHeader() == BarrierMessageHeader::RES_BARRIER_CLOSED ){
					check( SetEvent(gWakeupEvent), "barierDownListeningPipe: SetEvent failed");
					barrierDownListeningPipe->close();
					break;
				} else{
					std::cout << "barierDownListeningPipe: got other than barier down response" << std::endl;
				}
			}
		});


		myEnterCriticalSection();
		HANDLE outHandle = (HANDLE)vec.size();
		ClientBarrierData data(gWakeupEvent, barierDownListeningPipe, createdPipe, barrierId, barrierName);
		vec.push_back(data);
		HANDLE toReturn = (HANDLE)barrierId;
		myLeaveCriticalSection();

		return toReturn;
	}

	~AllBarierDatas(){
		for( auto &elem : vec ){
			elem.messageDownReciever->join();
		}
	}

private:
	std::shared_ptr<MyClientNamedPipe> createPipeToServer(LPCWSTR pipeName){
		auto outVal = std::make_shared<MyClientNamedPipe>(pipeName);
		if(check(outVal->initPipe(),"Client: InitPipeFailed")){
			return std::shared_ptr<MyClientNamedPipe>();
		}
		return outVal;		
	}
};