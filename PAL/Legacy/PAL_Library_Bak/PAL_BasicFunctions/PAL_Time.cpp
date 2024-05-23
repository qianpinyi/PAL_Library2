#ifndef PAL_TIME_CPP
#define PAL_TIME_CPP 1

#include "../PAL_BuildConfig.h"
#include "PAL_BasicFunctions_0.cpp"

#include <ctime>

#if PAL_CFG_EnableOldFunctions == 1
char *GetTime1()
{
	time_t rawtime;
	time(&rawtime);
	return ctime(&rawtime);
}
#endif

bool IsLeapYear(unsigned int y)//y>=1,It may be wrong when y is too big
{return ((y%4==0&&y%100!=0)||y%400==0)&&y%3200!=0||y%172800==0;}

unsigned int CountLeapYearTo1(unsigned int p)
{return p/4-p/100+p/400-p/3200+p/172800;}

unsigned int CountLeapYear(int l,int r)//[l,r] ,l!=0,r!=0
{
	if (l>r) return 0;
	else if (l>0&&r>0) return CountLeapYearTo1(r)-CountLeapYearTo1(l-1);
	else if (l<0&&r>0) return CountLeapYearTo1(r)+CountLeapYearTo1(-l-1)+1;//??
	else return CountLeapYearTo1(-l-1)-CountLeapYearTo1(-r-1-1)+1;
}

int GetMonthDaysCount(int m,bool isLeapYear=0)
{
	const static int mon[12]={31,28,31,30,31,30,31,31,30,31,30,31};
	if (!InRange(m,1,12)) return 0;//return 0 means error;
	else if (m==2) return mon[1]+isLeapYear;
	else return mon[m-1];
}

int GetMonthDaysCountTo1(int m,bool isLeapYear=0)
{
	const static int mon[12]={31,59,90,120,151,181,212,243,273,304,334,365};
	if (m<1) return 0;
	else if (m>12) return 365+isLeapYear;
	else if (m>=2) return mon[m-1]+isLeapYear;
	else return mon[m-1];
}

int GetDateInYearFromDays(int days,bool IsLeapYear=0)
{
	if (!InRange(days,1,365+IsLeapYear)) return 0;
	else
		for (int m=1;m<=12;++m)
			if (GetMonthDaysCountTo1(m,IsLeapYear)>=days)
				return m*10000+days-GetMonthDaysCountTo1(m-1,IsLeapYear);
	return 0;//It shouldn't run to here...;
}

int GetDayCountFromDateTo0(int year,int month,int day)
{
	if (year>0)
		return (year-1)*365+CountLeapYearTo1(year-1)+(month>=2?GetMonthDaysCountTo1(month-1,IsLeapYear(year)):0)+day;
	else if (year<0)
		return -((-year)*365+CountLeapYearTo1(-year-1)+1-(month>=2?GetMonthDaysCountTo1(month-1,IsLeapYear(-year-1)):0)-day);
	else return 0;//Invalid input return value;
}

struct PAL_TimeL
{
	int	day=0,//0~INF
		hour=0,//0~23
		minute=0,//0~59
		second=0;//0~59
		
	const static PAL_TimeL Zero;
	
	long long Seconds() const
	{return (((long long)day*24+hour)*60+minute)*60+second;}
	
	long long Minutes() const
	{return ((long long)day*24+hour)*60+minute+second/60;}
	
	long long Hours() const
	{return (long long)day*24+hour+(minute+second/60)/60;}
	
	int Days() const
	{return day+(hour+(minute+second/60)/60)/24;}
	
	inline void Regularize()
	{
		long long s=Seconds();
		second=s%60;
		minute=(s/=60)%60;
		hour=(s/=60)%24;
		day=s/24;
	}
	
	inline bool IsRegular() const
	{return InRange(second,0,59)&&InRange(minute,0,59)&&InRange(hour,0,23)&&day>=0
		||InRange(second,-59,0)&&InRange(minute,-59,0)&&InRange(hour,-23,0)&&day<=0;}
	
	inline int Signal() const
	{
		int s=Seconds();
		return s>0?1:(s==0?0:-1);
	}
	
	inline PAL_TimeL operator + (const PAL_TimeL &tar) const
	{
		PAL_TimeL re(day+tar.day,hour+tar.hour,minute+tar.minute,second+tar.second);
		re.Regularize();
		return re;
	}
	
	inline PAL_TimeL operator - (const PAL_TimeL &tar) const
	{
		PAL_TimeL re(day-tar.day,hour-tar.hour,minute-tar.minute,second-tar.second);
		re.Regularize();
		return re;
	}
	
	inline PAL_TimeL& operator += (const PAL_TimeL &tar)
	{
		day+=tar.day;
		hour+=tar.hour;
		minute+=tar.minute;
		second+=tar.second;
		Regularize();
		return *this;
	}
	
	inline PAL_TimeL& operator -= (const PAL_TimeL &tar)
	{
		day-=tar.day;
		hour-=tar.hour;
		minute-=tar.minute;
		second-=tar.second;
		Regularize();
		return *this;
	}
	
	inline PAL_TimeL operator + () const
	{return *this;}
	
	inline PAL_TimeL operator - () const
	{return PAL_TimeL(-day,-hour,-minute,-second);}
	
	inline bool operator < (const PAL_TimeL &tar) const
	{return Seconds()<tar.Seconds();}
	
	inline bool operator > (const PAL_TimeL &tar) const
	{return Seconds()>tar.Seconds();}
	
	inline bool operator <= (const PAL_TimeL &tar) const
	{return Seconds()<=tar.Seconds();}
	
	inline bool operator >= (const PAL_TimeL &tar) const
	{return Seconds()>=tar.Seconds();}
	
	inline bool operator == (const PAL_TimeL &tar) const
	{return Seconds()==tar.Seconds();}
	
	inline bool operator != (const PAL_TimeL &tar) const
	{return Seconds()!=tar.Seconds();}
	
	PAL_TimeL(int _day,int _hour,int _minute,int _second)
	:day(_day),hour(_hour),minute(_minute),second(_second) {}

	PAL_TimeL() {}
};
const PAL_TimeL PAL_TimeL::Zero(0,0,0,0);

struct PAL_TimeP
{
	int year=1900,//-INF~INF and year!=0,when year is abnormal I can't ensure that it won't wrong (×_×;)
		month=0,//1~12
		day=0,//1~31
		hour=0,//0~23
		minute=0,//0~59
		second=0;//0~59
	
	static inline PAL_TimeP CurrentDateTime()
	{
		time_t t=time(0);
		return PAL_TimeP(*localtime(&t));//wild ptr??
	}
	
	inline bool IsRegular() const
	{return InRange(month,1,12)&&InRange(day,1,GetMonthDaysCount(month,IsLeapYear(year)))&&
			InRange(hour,0,23)&&InRange(minute,0,59)&&InRange(second,0,59);}
	
	PAL_TimeP& operator += (const PAL_TimeL &tar)//??
	{
		PAL_TimeL t=tar+PAL_TimeL(day,hour,minute,second);
		if (t.Signal()<0)
		{
			int s=24*60*60+(t.hour*60+t.minute)*60+t.second;//+-1?
			second=s%60;
			minute=(s/=60)%60;
			hour=(s/=60)/60;
			day=t.day-1;
		}
		else
		{
			day=t.day;
			hour=t.hour;
			minute=t.minute;
			second=t.second;
		}

		int daycount=GetDayCountFromDateTo0(year,month,day);
		
		if (daycount>0)
		{
			year=daycount/365+1;
			daycount%=365;
			daycount-=CountLeapYearTo1(year-1);
			while (1)
				if (daycount<=0)//it may be slow,how to iterate faster? 
					daycount+=365+IsLeapYear(--year);
				else break;
			int md=GetDateInYearFromDays(daycount,IsLeapYear(year));
			month=md/10000;
			day=md%10000;
		}
		else//I'm no sure much more...
		{
			//realize it later(after one thousand years(房房姊房房))
		}
		
		return *this;
	}
	
	inline PAL_TimeP& operator -= (const PAL_TimeL &tar)
	{return operator +=(-tar);}
	
	inline PAL_TimeP operator + (const PAL_TimeL &tar) const
	{
		PAL_TimeP re(*this);
		return re+=tar;
	}
	
	inline PAL_TimeP operator - (const PAL_TimeL &tar) const
	{
		PAL_TimeP re(*this);
		return re+=-tar;
	}
	
	inline PAL_TimeL operator - (const PAL_TimeP &tar) const
	{
		PAL_TimeL re(GetDayCountFromDateTo0(year,month,day)-GetDayCountFromDateTo0(tar.year,tar.month,tar.day)-1,
					hour+23-tar.hour,minute+59-tar.minute,second+60-tar.second);//+-1?
		re.Regularize();
		return re;
	}
	
	inline bool operator < (const PAL_TimeP &tar) const
	{
		if (year==tar.year)
			if (month==tar.month)
				if (day==tar.day)
					if (hour==tar.hour)
						if (minute==tar.minute) return second<tar.second;
						else return minute<tar.minute;
					else return hour<tar.hour;
				else return day<tar.day;
			else return month<tar.month;
		else return year<tar.year;
	}
	
	inline bool operator > (const PAL_TimeP &tar) const
	{return tar.operator < (*this);}
	
	inline bool operator <= (const PAL_TimeP &tar) const
	{return !tar.operator < (*this);}
	
	inline bool operator >= (const PAL_TimeP &tar) const
	{return !operator < (tar);}
	
	inline bool operator == (const PAL_TimeP &tar) const
	{return year==tar.year&&month==tar.month&&day==tar.day&&hour==tar.hour&&minute==tar.minute&&second==tar.second;}
	
	inline bool operator != (const PAL_TimeP &tar) const
	{return !operator == (tar);}
	
	PAL_TimeP(const tm &t):year(t.tm_year+1900),month(t.tm_mon+1),day(t.tm_mday),hour(t.tm_hour),minute(t.tm_min),second(t.tm_sec) {}
	
	PAL_TimeP(int _year,int _month,int _day,int _hour,int _minute,int _second)
	:year(_year),month(_month),day(_day),hour(_hour),minute(_minute),second(_second) {}
	
	PAL_TimeP() {}
};

#endif
