#pragma once

#include <string>
#include <vector>
#include <sstream>

class StringHelp{
public:
	static std::vector<std::string> split(std::string &input){
		std::istringstream iss(input);
		std::vector<std::string> outVec;
		std::copy(	std::istream_iterator<std::string>(iss),
					std::istream_iterator<std::string>(),
					std::back_inserter(outVec));
		return outVec;
	}
};