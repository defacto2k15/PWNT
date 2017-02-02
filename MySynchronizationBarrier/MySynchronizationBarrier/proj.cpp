#include "stdafx.h"
#include <Windows.h>
#include <string>
#include <iostream>
#include <thread>
#include <memory>
#include <functional>
#include <vector>
#include <strsafe.h>
#include <algorithm>
#include "MyNamedPipe.h"
#include "Common.h"
#include "BarrierMessage.h"
#include "MyClientNamedPipe.h"
#include "MyServerNamedPipe.h"
#include "ClientBarrierData.h"
#include "VectorWithCriticalSection.h"
#include "AllBarierDatas.h"
#include "ServerSynchronizationBarrier.h"
#include "ServerBarrierVec.h"
#include "MySynchronizationBarrier.h"
#include "MyWinService.h"

///////SERVER THINGS
#include "MySynchronizationBarrierServer.h"
#include "IoCompletitionPortServer.h"


void Listen(){
	startIocpServer();
}

#include "ThreadInTest.h"
#include "OneTest.h"

bool test1(){
	const char *barrierName = "myBarrier1";
	ThreadInTest<bool> client( [=](){
		HANDLE barrier = CreateSynchronizationBarrier(barrierName, 1);
		if( barrier == BAD_HANDLE ){
			std::cerr << "Creating synchronization barrier failed"<<std::endl;
			return false;
		}
		Sleep(2000); 

		bool res = WaitForEvent(barrier);
		if( !res ){
			std::cerr << "Waiting for event failed"<<std::endl;
			return false;
		}
		Close(barrier);
		return true;
	});
	
	auto future = client.run();
	return future.get();
}

template<typename T>
bool isFutureReady(std::future<T> &future) { 
	return future.wait_for(std::chrono::seconds(0)) == std::future_status::ready; 
}

bool test2(){
	const char *barrierName = "myBarrier2";
	HANDLE barrier = CreateSynchronizationBarrier(barrierName, 2);
	if( barrier == BAD_HANDLE ){
		std::cerr << "Creating synchronization barrier failed"<<std::endl;
		return false;
	}
	Sleep(1000);
	ThreadInTest<bool> client1( [=](){
		bool res = WaitForEvent(barrier);
		if( !res ){
			std::cerr << "Waiting for event failed"<<std::endl;
			return false;
		}
		return true;
	});

	Sleep(1000);
	auto future1 = client1.run();
	if( isFutureReady(future1) ){
		std::cerr << " First client ended, but not all clients achieved barrier " << std::endl;
		return false;
	}

	ThreadInTest<bool> client2( [=](){
		bool res = WaitForEvent(barrier);
		if( !res ){
			std::cerr << "Waiting for event failed"<<std::endl;
			return false;
		}
		return true;
	});
	Sleep(300);
	auto future2 = client2.run();
	if( isFutureReady(future2) ){
		std::cerr << " Second client ended, but not all clients achieved barrier " << std::endl;
		return false;
	}
	bool res = future1.get() && future2.get() ;
	Close(barrier);
	return res;
}

bool test3(){
	const char *barrierName = "myBarrier3";
	HANDLE barrier = CreateSynchronizationBarrier(barrierName, 1);
	bool res = WaitForEvent(barrier);
	if( !res ){
		std::cerr << "First Waiting for event failed"<<std::endl;
		return false;
	}
	res = WaitForEvent(barrier);
	if( !res ){
		std::cerr << "Second Waiting for event failed"<<std::endl;
		return false;
	}
	return true;
}


bool test4(){
	const char *barrierName = "myBarrier4";
	HANDLE barrier1 = CreateSynchronizationBarrier(barrierName, 1);
	if( check(barrier1 != BAD_HANDLE, " First creating barrier failed" )){
		return false;
	}
	HANDLE barrier2 = CreateSynchronizationBarrier(barrierName, 1);
	if( check(barrier2 != BAD_HANDLE, " Second creating barrier failed" )){
		return false;
	}
	if( check(barrier2 == barrier1, " First and second barrier handles are diffrent " )){
		return false;
	}
	bool res = WaitForEvent(barrier2);
	if( !res ){
		std::cerr << " Waiting for event failed"<<std::endl;
		return false;
	}
	return true;
}

bool test5(){
	const char *barrierName = "myBarrier5";
	HANDLE barrier = CreateSynchronizationBarrier(barrierName, 1);
	Sleep(400);
	Close(barrier);
	return true;
}


#include "InteractiveTestingActions.h"
#include "InteractiveTestActor.h"
#include "StringHelp.h"
#include <map>

int interactiveMain(){
	//std::thread server( [=](){ Listen();});
	//server.detach();

	std::string line;
	std::map<std::string, HANDLE> barriersMap;
	std::map<std::string, InteractiveTestActor> actorsMap;

	while(true){
		std::cout << "?>";
		std::getline (std::cin,line);

		auto tokens = StringHelp::split(line);
		if( tokens.size() < 1 ){
			std::cout << "?" << std::endl;
			continue;
		}

		if( tokens[0] == "help" ){
			InteractiveTestingActions::writeHelp();
		} else if( tokens[0] == "create_barrier" ){
			InteractiveTestingActions::createBarrier(tokens, barriersMap);
		} else if ( tokens[0] == "close_barrier" ){
			InteractiveTestingActions::closeBarrier(tokens, barriersMap);
		} else if ( tokens[0] == "wait_on_barrier" ){
			InteractiveTestingActions::waitOnBarrier(tokens, barriersMap, actorsMap);
		} else if ( tokens[0] == "new_actor" ){
			InteractiveTestingActions::newActor(tokens, actorsMap );
		} else if ( tokens[0] == "list_barriers" ){
			InteractiveTestingActions::listBarriers( barriersMap);
		} else if ( tokens[0] == "list_actors" ){
			InteractiveTestingActions::listActors( actorsMap);
		} else if ( tokens[0] == "quit" ){
			break;
		}  else {
			InteractiveTestingActions::writeBadOption();
		}
		std::cout << std::endl;
	}
	std::exit(0);
	return 0;
}

int testMain(){
	std::thread server( [=](){ Listen();});
	server.detach();
	Sleep(1000);

	std::vector<OneTest> tests;
	tests.push_back(OneTest("test1: Single thread barrier ", test1));
	tests.push_back(OneTest("test2: Double thread barrier ", test2));
	tests.push_back(OneTest("test3: Double entering single barrier ", test3));
	tests.push_back(OneTest("test4: Double creating barrier ", test4));
	tests.push_back(OneTest("test5: Can create and remove without entering ", test5));
	
	for( auto test : tests ){
		bool res = test.run();
		if(!res ){
			break;
		}
	}
	return 0;
}

int main(int argc, CHAR *argv[]){
	if( argc == 2 && ( strcmp(argv[1], "-interactive") == 0 )){
		interactiveMain();
		return 0;
	}
	if( argc == 2 && ( strcmp(argv[1], "-test") == 0 )){
		testMain();
		return 0;
	}

	serviceAppMain(argc, argv);
	//testMain();
	return 0;
}