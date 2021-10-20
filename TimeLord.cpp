extern "C" {

	#include <inttypes.h>
	#include <math.h>
}

#include "TimeLord.h"

TimeLord::TimeLord(){
		latitude=27.0;
		longitude=-82.0;
		timezone=-300;
		DstRules(3,2,11,1, 60); // USA
}
	
bool TimeLord::TimeZone(int z){
	if(Absolute(z)>720) return false;
	timezone=z;
	return true;
}

bool TimeLord::Position(float lat, float lon){
	if(fabs(lon)>180.0) return false;
	if(fabs(lat)>90.0) return false;
	latitude=lat;
	longitude=lon;
	return true;
}

bool TimeLord::DstRules(uint8_t sm, uint8_t sw, uint8_t em, uint8_t ew, uint8_t adv){
	if(sm==0 || sw==0 || em==0 || ew==0) return false;
	if(sm>12 || sw>4 || em>12 || ew>4) return false;
	dstm1=sm;
	dstw1=sw;
	dstm2=em;
	dstw2=ew;
	dstadv=adv;
	return true;
}

void TimeLord::GMT(uint8_t * now){
	Adjust(now,-timezone);
}

void TimeLord::DST(uint8_t *now){
	if(InDst(now)) Adjust(now, dstadv);
}

bool TimeLord::SunRise(uint8_t * when){
	return ComputeSun(when,true);
}

bool TimeLord::SunSet(uint8_t * when){
	return ComputeSun(when,false);
}

float TimeLord::MoonPhase(uint8_t * when){
// the period is 29.530588853 days
// we compute the number of days since Jan 6, 2000
// at which time the moon was 'new'
	long d;
	float p;
        float fracDay = (((when[tl_hour] * 60.0 + when[tl_minute]) * 60.0) + when[tl_second]) / 86400.0;
	
	d=DayNumber(2000+when[tl_year],when[tl_month],when[tl_day])-DayNumber(2000,1,6);
	p=(d+fracDay)/29.530588853; // total lunar cycles since 1/1/2000
	d=p;
	p-=d; // p is now the fractional cycle, 0 to (less than) 1
	return p;

}

void TimeLord::Sidereal(uint8_t * when, bool local){
	uint64_t second, d;
	long minute;

/*
	 Based on US Naval observatory GMST algorithm
	 (http://aa.usno.navy.mil/faq/docs/GAST.php)
	 Adapted for Arduino
	 -----------------------------------------------
	 
	 Since Arduino doesn't provide double precision floating point, we have 
	 modified the algorithm to use (mostly) integer math. 
	 
	 This implementation will work until the year 2100 with residual error +- 2 seconds.
	 
	 That translates to +-30 arc-seconds of angular error, which is just
	 about the field of view of a 100x telescope, and well within the field of 
	 view of a 50x scope. 
	 
*/	

	// we're working in GMT time
	GMT(when);
	
	// Get number of days since our epoch of Jan 1, 2000
	d=DayNumber(when[tl_year]+2000, when[tl_month], when[tl_day]) - DayNumber(2000,1,1);

	// compute calendar seconds since the epoch
	second=d*86400LL+when[tl_hour]*3600LL+when[tl_minute]*60LL+when[tl_second];
	
	// multiply by ratio of calendar to sidereal time
	second*=1002737909LL;
	second/=1000000000LL;
	
	// add sidereal time at the epoch
	second+=23992LL;
	
	if(local){ // convert from gmt to local
		d=240.0*longitude;
		second+=d;
	}

	// constrain to 1 calendar day
	second %= 86400LL;
	
	// update the tl_time array
	minute=second/60LL;
	d=minute*60LL;

	when[tl_second]=second-d;
	when[tl_hour]=0;
	when[tl_minute]=0;
	
	Adjust(when,minute);	
}

uint8_t TimeLord::_season(uint8_t * when){
	if(when[tl_month]<3) return 0; // winter
	if(when[tl_month]==3){
		if(when[tl_day]<22) return 0;
		return 1; // spring
	}
	
	if(when[tl_month]<6) return 1; // spring
	if(when[tl_month]==6){
		if(when[tl_day]<21) return 1;
		return 2; // summer
	}
	
	if(when[tl_month]<9) return 2; // summer
	if(when[tl_month]==9){
		if(when[tl_day]<22) return 2;
		return 3; // fall
	}
	
	if(when[tl_month]<12) return 3; // summer
	if(when[tl_day]<21) return 3;
	
	return 0; // winter
}

uint8_t TimeLord::Season(uint8_t * when){
uint8_t result;
	result=_season(when);
	if(latitude<0.0) result = (result+2) % 4;
	return result;
}

uint8_t TimeLord::DayOfWeek(uint8_t * when){
	int year;
	uint8_t  month,day;

	year=when[tl_year]+2000;
	month=when[tl_month];
	day=when[tl_day];
	
	if (month < 3) {
      month += 12;
      year--;
   }
   day= ((13*month+3)/5 + day + year + year/4 - year/100 + year/400 ) % 7;
   day=(day+1) % 7;
   return day+1;
}

uint8_t TimeLord::LengthOfMonth(uint8_t * when){
	uint8_t odd, mnth;
	int yr;
	
	yr=when[tl_year]+2000;
	mnth=when[tl_month];
	
	if(mnth==2){
		if(IsLeapYear(yr) ) return 29;
		return 28;
	}
	odd=(mnth & 1) == 1;
	if (mnth > 7) odd  = !odd;
	if (odd) return 31;
	return 30;
}

bool TimeLord::IsLeapYear(int yr){
	return ( (yr % 4 == 0 && yr % 100 != 0) || yr % 400 == 0);
}

bool TimeLord::InDst(uint8_t * p){
	// input is assumed to be standard time
	char nSundays, prevSunday, weekday;

	if(p[tl_month]<dstm1 || p[tl_month]>dstm2) return false;
	if(p[tl_month]>dstm1 && p[tl_month]<dstm2) return true;
	
	// if we get here, we are in either the start or end month
	
	// How many sundays so far this month?
	weekday=DayOfWeek(p);
	nSundays=0;
	prevSunday=p[tl_day]-weekday+1;
	if(prevSunday>0){ 
		nSundays=prevSunday/7;
		nSundays++;
	}
	
	if(p[tl_month]==dstm1){
		if(nSundays<dstw1) return false;
		if(nSundays>dstw1) return true;
		if(weekday>1) return true;
		if(p[tl_hour]>1) return true;
		return false;
	}
	
	if(nSundays<dstw2) return true;
	if(nSundays>dstw2) return false;
	if(weekday>1) return false;
	if(p[tl_hour]>1) return false;
	return true;
}


//====Utility====================

// rather than import yet another library, we define sgn and abs ourselves
char TimeLord::Signum(int n){
	if(n<0) return -1;
	return 1;
}

int TimeLord::Absolute(int n){
	if(n<0) return 0-n;
	return n;
}

void TimeLord::Adjust(uint8_t * when, long offset){
	long tmp, mod, nxt;
	
	// offset is in minutes
	tmp=when[tl_minute]+offset; // minutes
	nxt=tmp/60;				// hours
	mod=Absolute(tmp) % 60;
	mod=mod*Signum(tmp)+60;
	mod %= 60;
	when[tl_minute]=mod;
	
	tmp=nxt+when[tl_hour];
	nxt=tmp/24;					// days
	mod=Absolute(tmp) % 24;
	mod=mod*Signum(tmp)+24;
	mod %= 24;
	when[tl_hour]=mod;
	
	tmp=nxt+when[tl_day];
	mod=LengthOfMonth(when);

        when[tl_day] = tmp;
	if(tmp>mod){
            while (tmp>mod) {
		tmp -= mod;
		when[tl_month]++;
                if (when[tl_month] > 12) {
                    when[tl_month] = 1;
                    when[tl_year]++;
                    when[tl_year] += 100;
                    when[tl_year] %= 100;
                }
                mod = LengthOfMonth(when);
            }
            when[tl_day]=tmp;
	} else if (tmp<1) {
            while (tmp<1) {
		when[tl_month]--;
                if (when[tl_month] < 1) {
                    when[tl_month] = 12;
                    when[tl_year] --;
                    when[tl_year] += 100;
                    when[tl_year] %= 100;
                }
		mod=LengthOfMonth(when);
		when[tl_day]=tmp+mod;
                tmp += mod;
            }
	}
}

bool TimeLord::ComputeSun(uint8_t * when, bool rs) {
  uint8_t  month, day;
  float y, decl, eqt, ha, lon, lat, z;
  uint8_t a;
  int doy, minutes;
  
  month=when[tl_month]-1;
  day=when[tl_day]-1;
  lon=-longitude/57.295779513082322;
  lat=latitude/57.295779513082322;
  
  
  //approximate hour;
  a=6;
  if(rs) a=18;
  
  // approximate day of year
  y= month * 30.4375 + day  + a/24.0; // 0... 365
 
  // compute fractional year
  y *= 1.718771839885e-02; // 0... 1
  
  // compute equation of time... .43068174
  eqt=229.18 * (0.000075+0.001868*cos(y)  -0.032077*sin(y) -0.014615*cos(y*2) -0.040849*sin(y* 2) );
  
  // compute solar declination... -0.398272
  decl=0.006918-0.399912*cos(y)+0.070257*sin(y)-0.006758*cos(y*2)+0.000907*sin(y*2)-0.002697*cos(y*3)+0.00148*sin(y*3);
  
  //compute hour angle
  ha=(  cos(1.585340737228125) / (cos(lat)*cos(decl)) -tan(lat) * tan(decl)  );
  
  if(fabs(ha)>1.0){// we're in the (ant)arctic and there is no rise(or set) today!
  	return false; 
  }
  
  ha=acos(ha); 
  if(rs==false) ha=-ha;
  
  // compute minutes from midnight
  minutes=720+4*(lon-ha)*57.295779513082322-eqt;
  
  // convert from UTC back to our timezone
  minutes+= timezone;
  
  // adjust the time array by minutes
  when[tl_hour]=0;
  when[tl_minute]=0;
  when[tl_second]=0;
  Adjust(when,minutes);
	return true;
}

long TimeLord::DayNumber(uint16_t y, uint8_t m, uint8_t d){

	m = (m + 9) % 12;
	y = y - m/10;
	return 365*y + y/4 - y/100 + y/400 + (m*306 + 5)/10 + d - 1 ;
}
