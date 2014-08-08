#ifndef OSCAMBASE_HPP
#define OSCAMBASE_HPP

class OscamBase
{
	public:
			virtual ~OscamBase() {};

			virtual void processCW(const uint8_t* const cw0, const uint8_t* const cw1) = 0;
			virtual void setFilter(const uint16_t) = 0;
};

#endif
