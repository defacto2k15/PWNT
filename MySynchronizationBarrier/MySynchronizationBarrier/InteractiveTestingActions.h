#pragma once
#include <vector>
#include <string>
#include <map>
#include <Windows.h>
#include "InteractiveTestActor.h"


class InteractiveTestingActions{
public:
	static void writeHelp(){
		std::cout << ">>Help: possible commands" << std::endl;
		std::cout << ">>  " << "create_barrier [Barrier name] [thread count]" <<std::endl;
		std::cout << ">>  " << "close_barrier [Barrier name]" <<std::endl;
		std::cout << ">>  " << "wait_on_barrier [Actor name] [Barrier name]" <<std::endl;
		std::cout << ">>  " << "new_actor [Actor name]" <<std::endl;
		std::cout << ">>  " << "list_barriers" <<std::endl;
		std::cout << ">>  " << "list_actors" <<std::endl;
		std::cout << ">>  " << "quit" <<std::endl;
	}

	static void createBarrier(std::vector<std::string> &tokens, std::map<std::string, HANDLE> &barriersMap){
		if( tokens.size() != 3 ){
			std::cout << "!!Bad argument count. Should be like 'create_barrier [Barrier name] [thread count]'"<<std::endl;
			return;
		}
		std::string barrierName = tokens[1];
		int threadCount;
		try{
			threadCount = std::stoi(tokens[2]);
		} catch( ... ){
			std::cout <<"!!Second argument must be int" << std::endl;
			return;
		}
		
		if( barriersMap.count(barrierName) != 0 ){
			std::cout << "!!There arleady exists barrier of this name"<<std::endl;
			return;
		}

		std::cout << ">>Creating barrier of name "<<barrierName<<std::endl;
		HANDLE result = CreateSynchronizationBarrier(barrierName.c_str(), threadCount);
		if( result == BAD_HANDLE ){
			std::cout << "!!Creating barrier failed"<<std::endl;
			return;
		}
		barriersMap[barrierName] = result;
		std::cout <<">>Barrier creating OK"<<std::endl;
	}

	static void closeBarrier(std::vector<std::string> &tokens, std::map<std::string, HANDLE> &barriersMap){
		if( tokens.size() != 2 ){
			std::cout << "!!Bad argument count. Should be like 'close_barrier [Barrier name]'"<<std::endl;
			return;
		}

		std::string barrierName = tokens[1];
		if( barriersMap.count(barrierName) == 0 ){
			std::cout << "!!There are no barriers of this name"<<std::endl;
			return;
		}
		Close(barriersMap[barrierName]);
		barriersMap.erase(barrierName);
	}

	static void waitOnBarrier(std::vector<std::string> &tokens, std::map<std::string, HANDLE> &barriersMap,  std::map<std::string, InteractiveTestActor> &actorsMap){
		if( tokens.size() != 3 ){
			std::cout << "!!Bad argument count. Should be like 'wait_on_barrier [Actor name] [Barrier name]'"<<std::endl;
			return;
		}

		std::string actorName = tokens[1];
		std::string barrierName = tokens[2];
		if( barriersMap.count(barrierName) == 0 ){
			std::cout << "!!There are no barriers of this name"<<std::endl;
			return;
		}
		if( actorsMap.count(actorName) == 0 ){
			std::cout << "!!There are no actors of this name"<<std::endl;
			return;
		}		
		
		if( actorsMap[actorName].isBusy() ){
			std::cout << "!!Actor is busy"<<std::endl;
			return;
		}

		actorsMap[actorName].waitOnBarrier(barriersMap[barrierName]);
		std::cout << ">>Told actor to wait"<<std::endl;
		return;
	}

	static void newActor(std::vector<std::string> &tokens, std::map<std::string, InteractiveTestActor> &actorsMap){
		if( tokens.size() != 2 ){
			std::cout << "!!Bad argument count. Should be like 'new_actor [Actor name]'"<<std::endl;
			return;
		}

		std::string actorName = tokens[1];
		if( actorsMap.count(actorName) != 0 ){
			std::cout << "!!There arleady if actor of this name"<<std::endl;
			return;
		}
		actorsMap[actorName] = InteractiveTestActor(actorName);
		std::cout << ">>Created new actor"<<std::endl;
	}

	static void listBarriers(std::map<std::string, HANDLE> &barriersMap){
		std::cout << ">> Current Barriers:"<<std::endl;
		int i = 0;
		for( auto &pair : barriersMap ){
			std::cout<<">>["<<i<<"] = "<<pair.first<<" handle value: "<<(int)pair.second<<std::endl;
			i++;
		}
	}

	static void listActors(std::map<std::string, InteractiveTestActor> &actorsMap){
		std::cout << ">> Current Actors:"<<std::endl;
		int i = 0;
		for( auto &pair : actorsMap ){
			std::string isBusy = "false";
			if(pair.second.isBusy()){
				isBusy = "true";
			}

			std::cout<<">>["<<i<<"] = "<<pair.first<<" isBusy: "<<isBusy<<std::endl;
			i++;
		}
	}

	static void writeBadOption(){
		std::cout << "!!I dont understand the command"<<std::endl;
		std::cout << "!!type 'help' for help" << std::endl;
	}
};

