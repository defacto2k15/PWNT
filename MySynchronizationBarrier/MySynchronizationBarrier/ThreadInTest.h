#pragma once
#include <functional>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <future>

template<typename T>
class ThreadInTest{
	std::function<T()> funcToRun;
	std::shared_ptr<std::thread> testThread;
public:
	ThreadInTest(std::function<T()> funcToRun ) : funcToRun(funcToRun){
	}

	std::future<T> run(){
		std::promise< T> testPromise;
		auto testFuture = testPromise.get_future();
		testThread = std::make_shared<std::thread>(std::bind([ this](std::promise<T> &testPromise){
			try {
				testPromise.set_value( funcToRun());
			} catch( ...){
				std::cerr<<" Unexpected exception caught " << std::endl;
				testPromise.set_exception(std::current_exception());
			}
		}, std::move(testPromise)));
		return testFuture;
	}

	~ThreadInTest(){
		testThread->join();
	}
};