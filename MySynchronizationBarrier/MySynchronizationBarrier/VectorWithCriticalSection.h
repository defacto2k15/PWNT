#pragma once
#include <Windows.h>

template<typename T>
class VectorWithCriticalSection{
private:
	CRITICAL_SECTION criticalSection;

public:
	std::vector< T> vec;

	VectorWithCriticalSection() {
		if(check( InitializeCriticalSectionAndSpinCount(&criticalSection, 0x00000400), "Critical section initialization failed" )){
			return;
		}
	}

	~VectorWithCriticalSection(){
		DeleteCriticalSection(&criticalSection);
	}

	bool getVecElement( int vecIndex, T *data){
		*data = vec[vecIndex];
		return true;
	}

	int addNewElement(T data){
		vec.push_back(data);
		int index = vec.size();
		return index;
	}

	bool setElement(int index, T data){
		if( vec.size() <= (unsigned)index ){
			return false;
		}
		vec[index] = data;
		return true;
	}

	void removeElement(int index){
		vec.erase(vec.begin() + index);
	}

	void myEnterCriticalSection(){
		EnterCriticalSection(&criticalSection);
	}

	void myLeaveCriticalSection(){
		LeaveCriticalSection(&criticalSection);
	}
};