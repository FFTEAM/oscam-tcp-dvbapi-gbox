#ifndef GBOXBASE_HPP
#define GBOXBASE_HPP

class GboxBase
{
	public:
			virtual ~GboxBase() {};

			virtual void processPMT(const uint8_t* const data, size_t len) = 0;
			virtual void processCAT(const uint8_t* const data, size_t len) = 0;
			virtual void processEMM(const uint8_t* const data, size_t len) = 0;
			virtual void processECM(const uint8_t* const data, size_t len) = 0;
};

#endif
