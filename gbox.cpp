#include <gbox.hpp>

#include <unistd.h>
#include <cstring>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

const uint16_t Gbox::GBOX_CMD_IN_PORT = 8003;
const uint16_t Gbox::GBOX_TABLE_OUT_PORT = 8004;

Gbox::Gbox() :
	mIsValid(false),
	mKeepRunning(true),
	mControlChannelSock(createListenSocket(GBOX_CMD_IN_PORT)),
	mControlChannelThread(nullptr),
	oscamHandle(nullptr)
{
	if (mControlChannelSock > 0)
	{
		mControlChannelThread = new std::thread(&Gbox::controlChannelThread, this);
		if (mControlChannelThread)
		{
			printf("[GBOX]: everthing setup correctly!\n");
			mIsValid = true;
		}
		else
		{
			printf("[GBOX]: error during initialization of gbox control channel thread!\n");
		}
	}
	else
	{
		printf("[GBOX]: error during setup of gbox control channel socket!\n");
	}
}

Gbox::~Gbox()
{
	mKeepRunning = false;
	printf("[GBOX]: waiting for mControlChannelThread...\n");
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

bool Gbox::setOscamHandle(OscamBase* handle)
{
	if (nullptr != oscamHandle)
	{
		printf("[GBOX]: oscamHandle already set!\n");
		return false;
	}
	oscamHandle = handle;
	return true;
}

int Gbox::createListenSocket(const uint16_t port) const
{
	int fd = socket (AF_INET, SOCK_DGRAM, 0);
	if (fd==-1)
	{
		return -1;
	}

	struct sockaddr_in servAddr;

	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	servAddr.sin_port = htons(port);

	int dummy = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &dummy, sizeof(int));
	setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &dummy, sizeof(int));
	int rc = bind(fd, (struct sockaddr*)&servAddr, sizeof(servAddr));
	if (rc < 0) {
		return -1;
	}

	return fd;
}

bool Gbox::sendData(const uint8_t* const data, size_t len, uint16_t port) const
{
	struct sockaddr_in servaddr;
	int sockfd=socket(AF_INET, SOCK_DGRAM, 0);
	if (0 > sockfd)
	{
		return false;
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	servaddr.sin_port=htons(port);

	int ret = sendto(sockfd, data, len, 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
	close(sockfd);

	if (ret < len)
	{
		return false;
	}
	return true;
}

void Gbox::controlChannelThread()
{
	fd_set set;
	struct timeval timeout;
	uint8_t buffer[4096];

	while (mKeepRunning)
	{
		FD_ZERO(&set);
		FD_SET(mControlChannelSock, &set);

		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		int selret = select(mControlChannelSock  +1, &set, nullptr, nullptr, &timeout);
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
			ssize_t readret = recvfrom(mControlChannelSock, buffer, 4069, 0, nullptr, nullptr);
			if (readret < 0)
			{
				// error
			}
			else
			{
				handleMessage(buffer, readret);
			}
		}
	}
}

void Gbox::handleMessage(const uint8_t* const buffer, size_t len)
{
	if (nullptr == oscamHandle)
	{
		printf("[GBOX]: oscamHandle NOT set!\n");
		return;
	}

	if (len < 2)
	{
		printf("[GBOX]: message too short!\n");
		return;
	}

	printf("[GBOX]: from gbox:\n");
	for (size_t i = 0; i < len; i++)
	{
		printf("%02X ", buffer[i]);
	}
	printf("\n");

	switch (buffer[0])
	{
		case 0x89:	// CW
			if (len == 17)
			{
				oscamHandle->processCW(buffer+1, buffer+9);
			}
			else
			{
				printf("[GBOX]: invalid CW message length: %lu!\n", len);
			}
		break;

		case 0x8A:	// Need PID
			size_t pids = buffer[1];
			if ((pids * 2) != (len - 2))
			{
				printf("[GBOX]: invalid NeedPID message length: %lu!\n", len);
			}
			else
			{
				size_t pos = 2;
				do
				{
					oscamHandle->setFilter((buffer[pos] << 8) | buffer[pos+1]);
					pos += 2;
				} while (pos < len);
			}
		break;
	}
}

void Gbox::processPMT(const uint8_t* const data, size_t len)
{
	for (size_t i = 0; i < len; i++)
	{
		printf("%02X ", data[i]);
	}
	printf("\n");

	uint8_t* buffer = new uint8_t[len];
	if (nullptr == buffer)
	{
		printf("[GBOX]: memory allocation error!\n");
		return;
	}
	memcpy(buffer, data, len);
	buffer[0] = 0x87;
	buffer[1] = 0x02;

	bool ret = sendData(buffer, len, GBOX_TABLE_OUT_PORT);
	delete[] buffer;

	if (!ret)
	{
		printf("[GBOX]: sending PMT failed!\n");
	}
}

void Gbox::processCAT(const uint8_t* const data, size_t len)
{
	for (size_t i = 0; i < len; i++)
	{
		printf("%02X ", data[i]);
	}
	printf("\n");
}

void Gbox::processEMM(const uint8_t* const data, size_t len)
{
	for (size_t i = 0; i < len; i++)
	{
		printf("%02X ", data[i]);
	}
	printf("\n");
}

void Gbox::processECM(const uint8_t* const data, size_t len)
{
	for (size_t i = 0; i < len; i++)
	{
		printf("%02X ", data[i]);
	}
	printf("\n");
}

Gbox::operator bool() const
{
	return mIsValid;
}
