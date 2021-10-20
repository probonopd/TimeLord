extern "C" {

	#include <inttypes.h>
	#include <math.h>
}

#define tl_second 0
#define tl_minute 1
#define tl_hour 2
#define tl_day 3
#define tl_month 4
#define tl_year 5

class TimeLord{
	public:
		TimeLord();
		// configuration
		bool Position(float, float);
		bool TimeZone(int);
		bool DstRules(uint8_t,uint8_t,uint8_t,uint8_t,uint8_t);
		
		// Political
		void GMT(uint8_t *);
		void DST(uint8_t *);
				
		//Solar & Astronomical
		bool SunRise(uint8_t *);
		bool SunSet(uint8_t *);
		float MoonPhase(uint8_t *);
		void Sidereal(uint8_t *, bool);
		uint8_t Season(uint8_t *);
		
		// Utility
		uint8_t DayOfWeek(uint8_t *);
		uint8_t LengthOfMonth(uint8_t *);
		bool IsLeapYear(int);
		
                // these were private
		void Adjust(uint8_t *, long);
		long DayNumber(uint16_t, uint8_t, uint8_t);
		bool InDst(uint8_t *);
	private:
		float latitude, longitude;
		int timezone;
		uint8_t dstm1, dstw1, dstm2, dstw2, dstadv;
		bool ComputeSun(uint8_t *, bool);
		char Signum(int);
		int Absolute(int);
		uint8_t _season(uint8_t *);
};
