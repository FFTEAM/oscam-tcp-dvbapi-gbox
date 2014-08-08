#ifndef OSCAM_HPP
#define OSCAM_HPP
/* this class is implementing the oscam tcp side */

#include <thread>

#include <oscamBase.hpp>
#include <gboxBase.hpp>

class Oscam final : public OscamBase
{
	bool mIsValid;
	bool mKeepRunning;
	const uint16_t mPort;
	int mControlChannelSock;
	int mAcceptSock;
	std::thread* mControlChannelThread;
	GboxBase* gboxHandle;

	uint8_t mAdapterId;

	int createSocket() const;

	Oscam(const Oscam&) = delete;
	const Oscam& operator=(const Oscam&) = delete;

	void handleMessage(const uint8_t* const buffer, size_t len);
	int sendData(const uint8_t* const data, size_t len);

	public:
			static const uint32_t CA_SET_DESCR;
			static const uint32_t CA_SET_PID;
			static const uint32_t DMX_SET_FILTER;
			static const uint32_t DMX_STOP;

			Oscam(const uint16_t);
			virtual ~Oscam();
			bool setGboxHandle(GboxBase*);

			void controlChannelThread();

			virtual void processCW(const uint8_t* const cw0, const uint8_t* const cw1);
			virtual void setFilter(const uint16_t);

			explicit operator bool() const;
};

#endif
