#pragma once

class InteractiveTestActor{
	std::string actorName;
	bool busy;
public:
	InteractiveTestActor(std::string actorName) : actorName(actorName), busy(false){
	}

	InteractiveTestActor() : actorName("!!!UNSET!!!"), busy(false){
	}

	bool isBusy(){
		return busy;
	}

	void waitOnBarrier(HANDLE barrier){
		busy = true;
		std::thread thread( [=](){
			std::cout << "##  Actor "<<actorName<<" starts wait on barrier of handle "<<(int)barrier<<std::endl;
			bool res = WaitForEvent(barrier);
			std::string status = "POSITIVE";
			if( !res ){
				status = "NEGATIVE";
			}
			std::cout << "##  Actor "<<actorName<<" ended wait on barrier of handle "<<(int)barrier<<" and wait result was "<<status<<std::endl;
			busy = false;
		});
		thread.detach();
	}
};