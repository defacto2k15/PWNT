#pragma once
#include "VectorWithCriticalSection.h"
#include "ServerSynchronizationBarrier.h"

class ServerBarrierVec : public VectorWithCriticalSection<ServerSynchronizationBarrier>{
	int lastBarrierId;

public:
	ServerBarrierVec() : lastBarrierId(0){
	}

	int getBarrierOfName(std::string name){
		for( auto i = 0u; i < vec.size(); i++){
			if( vec[i].barrierName == name ){
				return i;
			}
		}
		return -1;
	}

	int getBarrierOfId(int barrierId){
		for( auto i = 0u; i < vec.size(); i++){
			if( vec[i].barrierId == barrierId ){
				return i;
			}
		}
		return -1;
	}

	int getNextBarrierId(){
		return lastBarrierId + 1;
	}

	void addBarrier( ServerSynchronizationBarrier &barrier){
		lastBarrierId++;
		addNewElement(barrier);
	}

};