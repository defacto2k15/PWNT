#pragma once
#include "BarrierMessageHeader.h"
#include "Common.h"

class BarrierMessage {
private:
	int barrierId;
	BarrierMessageHeader barrierHeader;
	int threadCount;
	CHAR barrierName[BUFSIZE - (sizeof(int)*2) - sizeof(BarrierMessageHeader)];
	
public:
	BarrierMessageHeader getHeader(){
		return barrierHeader;
	}

	size_t getSize(){
		return sizeof(BarrierMessage);
	}

	int getBarrierId(){
		return barrierId;
	}

	char *getBarrierName(){
		return barrierName;
	}

	int getThreadCount(){
		return threadCount;
	}

	static BarrierMessage createHeaderOnlyMessage( BarrierMessageHeader header ){
		BarrierMessage message;
		message.setHeader(header);
		return message;
	}

	static BarrierMessage createHeaderAndBarrierNameMessage( BarrierMessageHeader header, const char *barrierName ){
		BarrierMessage message;
		message.setHeader(header);
		message.setBarrierName(barrierName);
		return message;
	}

	static BarrierMessage createHeaderAndBarrierIdMessage( BarrierMessageHeader header, int barrierId){
		BarrierMessage message;
		message.setHeader(header);
		message.setBarrierId(barrierId);
		return message;
	}

	void setHeader(BarrierMessageHeader header){
		barrierHeader = header;
	}

	void setBarrierId( int inBarrierId ){
		this->barrierId = inBarrierId;
	}

	void setBarrierName(const char *inBarrierName){
		strcpy_s(this->barrierName, BARRIER_MAX_NAME_SIZE,inBarrierName); 
	}

	void setThreadCount(int threadCount){
		this->threadCount = threadCount;
	}

};