

#include "Botter.h"

u32 SleepTimeMs = 16;


void signal_handler(int signal) 
{
	if (signal == SIGABRT) {
		std::cerr << "SIGABRT received\n";
	} else {
		std::cerr << "Unexpected signal " << signal << " received\n";
	}
	std::_Exit(EXIT_FAILURE);
}



int main(int argc, char** argv)
{
	// Setup handler
	auto previous_handler = std::signal(SIGABRT, signal_handler);
	if (previous_handler == SIG_ERR) {
		std::cerr << "Setup failed\n";
		return EXIT_FAILURE;
	}

	HANDLE hTread = ::GetCurrentThread();
	SetThreadAffinityMask( hTread, 1 );

	Botter* b = new Botter;
	bool quit = false;
	u64 tlast = time_now();
	u64 tdisplay = tlast; // display every 3 seconds tick rate..
	float lastDt = 0.f;
	u32 tickIdx = 0;
	while (!quit)
	{
		u64 tstart = time_now();
		lastDt = to_seconds(tstart - tlast);
		tlast  = tstart;

		// every x seconds, show tick rate of bot
		if (to_seconds(tstart - tdisplay) >= 3.f)
		{
			tdisplay = tstart;
			
			//b->printBoneArrays();
			b->log(str_format("\n---- Tick rate is: %.3f num ticks: %d-----\n", 1.f/lastDt, tickIdx));
			b->printCameraStats();
			b->log(str_format("---- BoneArrays %d | Valid %d | Inview %d -----\n", b->numBoneArrays(), b->numValidBoneArrays(), b->numInViewBoneArrays() ));
			b->printBoneArraysSimple();
			b->log(str_format("\n\n"));

			tickIdx = 0;
		}

		if (!b->valid())
		{
			delete b;
			sleep(1.f);
			printf("Restarting bot...");
			Botter* b = new Botter;
			continue;
		}

		if (!b->gameTick(tickIdx++, lastDt))
			quit = true;

		sleep_milli(SleepTimeMs);
	//	sleep_milli(300);
	//	sleep_milli(90);

	//	if (GetAsyncKeyState(VK_END) & 1) 
	//		quit = true;
	}

	delete b;
	mem_showAllocationsInOutput();
	system("pause");
	return 0;
}