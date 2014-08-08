#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <signal.h>

#include <cstdio>

#include <gbox.hpp>
#include <oscam.hpp>

static bool keepRunning = true;

void sigHandler(int signum)
{
	(void) signum;
	keepRunning = false;
}

int main()
{
	struct sigaction sig;
	sig.sa_handler = sigHandler;
	sigemptyset (&sig.sa_mask);
    sig.sa_flags = 0;
	sigaction(SIGINT, &sig, nullptr);
	sigaction(SIGTERM, &sig, nullptr);

	Gbox* gbox = new Gbox();
	if (nullptr == gbox || !(*gbox))
	{
		printf("[MAIN]: fatal error during gbox module init!\n");
		delete gbox;
		return 1;
	}

	Oscam* oscam = new Oscam(2000);
	if (nullptr == oscam || !(*oscam))
	{
		printf("[MAIN]: fatal error during gbox module init!\n");
		delete oscam;
		return 1;
	}

	oscam->setGboxHandle(gbox);
	gbox->setOscamHandle(oscam);

	while (keepRunning)
	{
		sleep (1);
	}
	printf("[MAIN]: terminating...\n");

	delete gbox;
	printf("[MAIN]: exit done!\n");
}
