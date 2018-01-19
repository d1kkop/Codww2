#include "App.h"

using namespace Null;


void signal_handler(int signal) 
{
	if (signal == SIGABRT) {
		std::cerr << "SIGABRT received\n";
	} else {
		std::cerr << "Unexpected signal " << signal << " received\n";
	}
	std::_Exit(EXIT_FAILURE);
}


int main(int arc, char** argv)
{
	// Setup handler
	auto previous_handler = std::signal(SIGABRT, signal_handler);
	if (previous_handler == SIG_ERR) {
		std::cerr << "Setup failed\n";
		return EXIT_FAILURE;
	}

	//Deque<MemoryEntry2> mm;

	//for (int i = 0; i < 37000000; i++)
	//{
	//	MemoryEntry2 m2;
	//	m2.offset = 2;
	//	m2.page = nullptr;
	//	m2.value = 123;
	//	mm.emplace_back(m2);
	//}

	App app;
	//app.loop();
	app.loop2();
	system("pause");
	return 0;
}