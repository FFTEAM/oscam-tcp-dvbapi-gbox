#include <oscam.hpp>

#include <unistd.h>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <endian.h>

const uint32_t Oscam::CA_SET_DESCR		= 0x40106f86;
const uint32_t Oscam::CA_SET_PID		= 0x40086f87;
const uint32_t Oscam::DMX_SET_FILTER	= 0x403c6f2b;
const uint32_t Oscam::DMX_STOP			= 0x00006f2a;

Oscam::Oscam(const uint16_t port) :
	mIsValid(false),
	mKeepRunning(true),
	mPort(port),
	mControlChannelSock(createSocket()),
	mAcceptSock(-1),
	mControlChannelThread(nullptr),
	gboxHandle(nullptr),
	mAdapterId(0)
{
	if (mControlChannelSock > 0)
	{
		mControlChannelThread = new std::thread(&Oscam::controlChannelThread, this);
		if (mControlChannelThread)
		{
			printf("[OSCM]: everthing setup correctly!\n");
			mIsValid = true;
		}
		else
		{
			printf("[OSCM]: error during initialization of oscam control channel thread!\n");
		}
	}
	else
	{
		printf("[OSCM]: error during setup of oscam control channel socket!\n");
	}
}

Oscam::~Oscam()
{
	mKeepRunning = false;
	printf("[OSCM]: waiting for mControlChannelThread...\n");
	if (mControlChannelThread)
	{
		mControlChannelThread->join();
		delete mControlChannelThread;
	}
	if (mControlChannelSock > 0)
	{
		close(mControlChannelSock);
	}
}

bool Oscam::setGboxHandle(GboxBase* handle)
{
	if (nullptr != gboxHandle)
	{
		printf("[OSCM]: gboxHandle already set!\n");
		return false;
	}
	gboxHandle = handle;
	return true;
}

int Oscam::createSocket() const
{
	int fd = socket (AF_INET, SOCK_STREAM, 0);
	if (fd==-1)
	{
		return -1;
	}

	struct sockaddr_in servAddr;

	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(mPort);

	int dummy = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &dummy, sizeof(int));
	setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &dummy, sizeof(int));
	int rc = bind(fd, (struct sockaddr*)&servAddr, sizeof(servAddr));
	if (rc < 0) {
		return -1;
	}

	if (0 > listen(fd, 5))
	{
		return -1;
	}

	return fd;
}

int Oscam::sendData(const uint8_t* const data, size_t len)
{
	return send(mAcceptSock, data, len, 0);
}

void Oscam::controlChannelThread()
{
	fd_set set;
	struct timeval timeout;
	uint8_t buffer[4096];
	int maxfd = 0;

	while (mKeepRunning)
	{
		FD_ZERO(&set);
		FD_SET(mControlChannelSock, &set);
		maxfd = mControlChannelSock;
		if (mAcceptSock > 0)
		{
			FD_SET(mAcceptSock, &set);
			if (maxfd < mAcceptSock)
			{
				maxfd = mAcceptSock;
			}
		}

		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		int selret = select(maxfd + 1, &set, nullptr, nullptr, &timeout);
		if (selret == 0)
		{
			// select timeout
			continue;
		}
		else if (selret < 0)
		{
			// select error

			// TODO: reopen socket and renew this thread!
			break;
		}

		if (!mKeepRunning)
		{
			// double check termination condition
			break;
		}

		{
			if (FD_ISSET(mControlChannelSock, &set))
			{
				// we get a new connection, close the old one!

				if (mAcceptSock > 0)
				{
					printf("[OSCM]: got new connection, closing old!\n");
					close(mAcceptSock);
					mAcceptSock = -1;
				}

				struct sockaddr_in client;
				socklen_t client_len;
				mAcceptSock = accept(mControlChannelSock, (struct sockaddr*)&client, &client_len);
				if (mAcceptSock < 0)
				{
					// error
					printf("[OSCM]: accept() error!\n");
				}
			}
			else
			{
				// signal safe read, repeat and continue
				int readret = 0;
				do
				{
					readret = read(mAcceptSock, buffer, 4096);
				}
				while (readret < 0 && errno == EINTR);

				if (readret == 0)
				{
					// other side closed the socket
				}
				else if (readret < 0)
				{
					printf("[OSCM]: read() error!\n");
				}
				else
				{
					handleMessage(buffer, readret);
				}
				//close(mAcceptSock);
			}
		}
	}
}

void Oscam::handleMessage(const uint8_t* const buffer, size_t len)
{
	if (nullptr == gboxHandle)
	{
		printf("[OSCM]: gboxHandle NOT set!\n");
		//oscamHandle->
		return;
	}

	uint16_t head = ((buffer[0] << 8) | buffer[1]);
	switch (head)
	{
		case 0x9f80: // capmt
			mAdapterId = buffer[13];
			printf("DVB Adapter: %02X\n", mAdapterId);
			gboxHandle->processPMT(buffer, len);
		break;

		default:
			printf("[OSCM]: unknown command: %04X!\n", head);
		break;
	}
}

void Oscam::processCW(const uint8_t* const cw0, const uint8_t* const cw1)
{
	for (size_t i = 0; i < 8; i++)
	{
		printf("%02X ", cw0[i]);
	}
	printf("\n");

	for (size_t i = 0; i < 8; i++)
	{
		printf("%02X ", cw1[i]);
	}
	printf("\n");
}

/*

 typedef struct dmx_filter
{
	__u8  filter[DMX_FILTER_SIZE]; // DMX_FILTER_SIZE = 16
	__u8  mask[DMX_FILTER_SIZE];
	__u8  mode[DMX_FILTER_SIZE];
} dmx_filter_t;


struct dmx_sct_filter_params
{
	__u16          pid;
	dmx_filter_t   filter;
	__u32          timeout;
	__u32          flags;
#define DMX_CHECK_CRC       1
#define DMX_ONESHOT         2
#define DMX_IMMEDIATE_START 4
#define DMX_KERNEL_CLIENT   0x8000
};*/

void Oscam::setFilter(const uint16_t pid)
{
	uint8_t buffer[5];

/*00 <-- ADAPTER ID
2b 6f 3c 40 <-- DMX_SET_FILTER
00 00 44 18 <-- ECM PID
80 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 <-- FILTER
f0 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 <-- MASK
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 <-- MODE
00 00 b8 0b <-- TIMEOUT
00 00 04 00 <-- FLAGS
00 00*/

	buffer[0] = mAdapterId;
	buffer[1] = ((Oscam::DMX_SET_FILTER >> 24) & 0xff);
	buffer[2] = ((Oscam::DMX_SET_FILTER >> 16) & 0xff);
	buffer[3] = ((Oscam::DMX_SET_FILTER >> 8 ) & 0xff);
	buffer[4] = (Oscam::DMX_SET_FILTER & 0xff);
	buffer[5] = 0;
	buffer[6] = 0;
	buffer[7] = ((pid >> 8) & 0xff);
	buffer[8] = (pid & 0xff);
}

Oscam::operator bool() const
{
	return mIsValid;
}
