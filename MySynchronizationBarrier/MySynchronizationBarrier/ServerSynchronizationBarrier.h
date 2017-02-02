#pragma once
#include <string>
#include "Common.h"
#include <Windows.h>

class ServerSynchronizationBarrier{
public:
	int barrierId;
	std::string barrierName;
	int currentThreadCount;
	int initialThreadCount;
	HANDLE openBarrierWakeupEvent;
	bool hasSignalingThreadWaiting;
	bool wasClosed;

	ServerSynchronizationBarrier() : openBarrierWakeupEvent(BAD_HANDLE), hasSignalingThreadWaiting(false), wasClosed(false) {
	}

	void close(){
		CloseHandle(openBarrierWakeupEvent);
	}
};
