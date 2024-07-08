#include "demo_risc_core.hpp"

using namespace std;
int main(int argc, char *argv[])
{
	if (argc < 2) {	
		cout<<"No binary file to execute"<<endl;
		return 0;
	}
	try {
		_demo_risc_core core0(argv[1]);
		while (1)
			core0.clock();
	}
	catch (string err) {
		cout<<"Exception: "<<err<<endl;
	}
	catch (...) {
		cout<<"Default exception"<<endl;
	}

	return 0;
}
