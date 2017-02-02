#pragma once
#include <Windows.h>
#include <string>
#include <iostream>

HANDLE BAD_HANDLE = (HANDLE)-1;
SOCKET BAD_SOCKET = (SOCKET)-1;

#define BUFSIZE (512)

#define BARRIER_TCP_MESSAGE_SIZE (256)
#define BARRIER_MAX_NAME_SIZE (256 - 4 - 4)

#define MY_BARRIER_PIPE_NAME ( TEXT("\\\\.\\pipe\\mynamedpipe"))

bool check(bool value, std::string msgWhenFailure){
	if( !value ){
		std::cerr << msgWhenFailure<<" Last error is " <<GetLastError() << std::endl;
	}
	return !value;
}

bool check(BOOL value, std::string msgWhenFailure){
	return check(!(!value), msgWhenFailure);
}

const char MESSAGE_BARRIER_DOWN = 16;
const char MESSAGE_BARRIER_UP = 15;

class BarrierMessage;
