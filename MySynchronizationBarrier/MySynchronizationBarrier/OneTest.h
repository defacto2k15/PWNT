#pragma once
#include <string>
#include <functional>
#include <iostream>

class OneTest{
	std::string description;
	std::function<bool()> testFunc;
public:
	OneTest( std::string description, std::function<bool()> testFunc ) : description(description), testFunc(testFunc){
	}

	bool run(){
		std::cout<<"====================================="<<std::endl;
		std::cout << "Testing: "<<description<<std::endl;
		bool res = testFunc();
		if( res ){
			std::cout << "	OK" << std::endl;
		} else {
			std::cout << "	FAILED" << std::endl;
		}
		return res;
	}
};