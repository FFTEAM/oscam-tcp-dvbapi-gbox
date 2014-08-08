#ifndef GBOX_HPP
#define GBOX_HPP
/* this class is implementing the gbox udp side */

#include <thread>

#include <gboxBase.hpp>
#include <oscamBase.hpp>

class Gbox final : public GboxBase
{
	bool mIsValid;
	bool mKeepRunning;
	int mControlChannelSock;
	std::thread* mControlChannelThread;
	OscamBase* oscamHandle;

	int createListenSocket(const uint16_t port) const;
	bool sendData(const uint8_t* const data, size_t len, uint16_t port) const;
	void handleMessage(const uint8_t* const buffer, size_t len);

	Gbox(const Gbox&) = delete;
	const Gbox& operator=(const Gbox&) = delete;

	public:
			static const uint16_t GBOX_CMD_IN_PORT; // port where gbox is expecting PMT and CAT
			static const uint16_t GBOX_TABLE_OUT_PORT; // port where gbox is expecting PMT and CAT

			Gbox();
			virtual ~Gbox();
			bool setOscamHandle(OscamBase*);

			void controlChannelThread();

			virtual void processPMT(const uint8_t* const data, size_t len);
			virtual void processCAT(const uint8_t* const data, size_t len);
			virtual void processEMM(const uint8_t* const data, size_t len);
			virtual void processECM(const uint8_t* const data, size_t len);

			explicit operator bool() const;
};

#endif
