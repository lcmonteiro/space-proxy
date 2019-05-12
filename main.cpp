/* 
 * File:   main.cpp
 * Author: root
 *
 * Created on December 11, 2014, 10:53 AM
 */
#include <termios.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>

#include <cstdlib>
#include <cstring>

#include <string.h>
#include <stdbool.h>
#include <i386-linux-gnu/bits/signum.h>

#include "CTunnelI.h"
#include "CPacket.h"
#include "CTunnelRF.h"
#include "CTunnelSP.h"
#include "CTunnelTCP.h"
#include "CTunnelNull.h"

using namespace std;
/**
 * variables 
 */
static struct termios old_kbd_mode;
/*
 * functions
 */
void usage(void) {

	std::cerr << "Usage:" << std::endl;
	std::cerr << "-n <name>: interface name (mandatory)" << std::endl;
	std::cerr << "-t <type>: link type to use (mandatory)" << std::endl;
	std::cerr << "-d <device>: physical device name" << std::endl;
	std::cerr << "-l <local>: local address" << std::endl;
	std::cerr << "-r <remote>: remote address" << std::endl;
	std::cerr << "-p <port>:" << std::endl;
	std::cerr << "-s <speed>:" << std::endl;
	std::cerr << "-f <file>:" << std::endl;
	std::cerr << "EXAMPLES:" << std::endl;
	std::cerr << "$ interfacenw -n tunTCP -t tcp -l 2001:2::2 -p 8089" << std::endl;
	exit(1);
}
/**
 * 
 */
void Init() {
	struct termios t;

	tcgetattr(0, &t);

	memcpy(&old_kbd_mode, &t, sizeof (struct termios));
	t.c_lflag &= ~(ICANON | ECHO);
	t.c_cc[VTIME] = 0;
	t.c_cc[VMIN] = 1;
        
	tcsetattr(0, TCSANOW, &t);
}
/**
 * 
 */
void End() {
	tcsetattr(0, TCSANOW, &old_kbd_mode);
}
/**
 * 
 */
void Catch(int sig) {
	switch (sig) {
		default:
		{
			End();
		}
	}
        exit(0);
}

/**
 * 
 */
typedef enum {
	link_sp,
	link_tcp,
	link_rf,
	link_null
} link_t;

/**
 * 
 */
int main(int argc, char** argv) {
	/**
	 * subscribe signals
	 */
	signal(SIGBUS, Catch);
	signal(SIGABRT, Catch);
	signal(SIGSEGV, Catch);
	signal(SIGINT, Catch);
	signal(SIGTERM, Catch);
	/**
	 * options
	 */
	int opt;
	link_t type = link_null;
	string file;
	string name;
	string dev;
	string local;
	string remote;
	int port = 0;
	int speed = 9600;

	static struct option long_options[] = {
		{"name", required_argument, 0, 'n'},
		{"type", required_argument, 0, 't'},
		{"device", required_argument, 0, 'd'},
		{"local", required_argument, 0, 'l'},
		{"remote", required_argument, 0, 'r'},
		{"file", required_argument, 0, 'f'},
		{"speed", required_argument, 0, 's'},
		{"port", required_argument, 0, 'p'},
		{0, 0, 0, 0}
	};
	int option_index = 0;

	while ((opt = getopt_long(argc, argv, "n:t:d:l:r:f:s:p:", long_options, &option_index)) > 0) {
		switch (opt) {
			case 'n':
			{
				name = optarg;
				break;
			}
			case 't':
			{
				if (std::strcmp(optarg, "sp") == 0) {
					type = link_sp;
					break;
				}
				if (std::strcmp(optarg, "tcp") == 0) {
					type = link_tcp;
					break;
				}
				if (std::strcmp(optarg, "rf") == 0) {
					type = link_rf;
					break;
				}
				break;
			}
			case 'd':
			{
				dev = optarg;
				break;
			}
			case 'l':
			{
				local = optarg;
				break;
			}
			case 'r':
			{
				remote = optarg;
				break;
			}
			case 'f':
			{
				file = optarg;
				break;
			}
			case 's':
			{
				speed = std::atoi(optarg);
				break;
			}
			case 'p':
			{
				port = std::atoi(optarg);
				break;
			}
			default:
			{
				usage();
			}
		}
	}
	/**
	 * configure up
	 */
	CTunnelBase* pUp = NULL;

	if (name.empty()) {
		usage();
	}
	pUp = new CTunnelI(name.data(), local.data());
	/**
	 * configure down
	 */
	CTunnelBase* pDown = NULL;
	switch (type) {
		case link_sp:
		{
			if (dev.empty()) {
				usage();
			}
			pDown = new CTunnelSP(dev.data(), speed);
			break;
		}
		case link_tcp:
		{
			if (dev.empty() && !port) {
				usage();
			}
			pDown = new CTunnelTCP(dev.data(), port);
			break;
		}
		case link_rf:
		{
			if (dev.empty()) {
				usage();
			}
			pDown = new CTunnelRF(dev.data(), local.data(), file.data(), speed);
			break;
		}
		case link_null:
		{
			pDown = new CTunnelNull();	
			break;
		}
		default:
		{
			usage();
		}
	}
	while (1) {
		/**
		 * run
		 */
                Init();
		if (!pUp->Open()) {
			cerr << "Error: Open(up) " << endl;
                        End();
			return 1;
		}
		if (!pDown->Open()) {
			cerr << "Error: Open(down) " << endl;
			End();
                        return 1;
		}
		try {
			while (CTunnelBase::Wait(2, pUp, pDown, NULL)) {
				CPacket p;

				*pUp >> p >> *pDown >> *pUp;

				*pDown >> p >> *pUp;
				/**
				 * 
				 */
				CTunnelBase::Process();
			}

		} catch (std::string& error) {
			cerr << "Exception: " << error << endl;
		}
		End();
		pUp->Close();
		pDown->Close();
                /**
                 * wait to restart
                 */
                sleep(1);
	}

	delete pDown;
	delete pUp;

	return 0;
}

