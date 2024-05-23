#ifndef PAL_COMPUTATIONALGEOMETRY_CPP
#define PAL_COMPUTATIONALGEOMETRY_CPP 1

#include <cmath>
#include <vector>
#include <algorithm>
#include <ctime>

namespace PAL_Algorithm
{
	#define ERO (1e9)
	#define XX(x) ((x)*(x))
	#define ESP (1e-15)
	#define ET0(x) (fabs(x)<=ESP)//Equal To 0
	
	class Point2D
	{
		public:
			const static Point2D Zero;

			double x,y;

			inline static bool CompX(const Point2D &p1,const Point2D &p2)//for sort
			{
				if (p1.x==p2.x) return p1.y<p2.y;
				return p1.x<p2.x;
			}
			
			inline static bool CompY(const Point2D &p1,const Point2D &p2)
			{
				if (p1.y==p2.y) return p1.x<p2.x;
				return p1.y<p2.y;
			}
			
			inline Point2D operator + (const Point2D &p) const
			{return Point2D(x+p.x,y+p.y);}
			
			inline Point2D operator - (const Point2D &p) const
			{return Point2D(x-p.x,y-p.y);}
			
			inline Point2D operator * (double lambda) const
			{return Point2D(x*lambda,y*lambda);}

			inline Point2D operator / (double lambda) const
			{return Point2D(x/lambda,y/lambda);}
			
			inline double operator * (const Point2D &p) const
			{return x*p.x+y*p.y;}
			
			inline double operator % (const Point2D &p) const
			{return x*p.y-y*p.x;}
			
			inline bool operator == (const Point2D &p) const
			{return ET0(x-p.x)&&ET0(y-p.y);}
			
			inline bool operator != (const Point2D &p) const
			{return !operator ==(p);}
			
			inline double Mo () const
			{return sqrt(XX(x)+XX(y));}
			
			inline double Mo_2() const
			{return XX(x)+XX(y);}
			
			inline Point2D e() const
			{
				if (x==0&&y==0) return *this;
				else return operator /(Mo());
			}
			
			inline Point2D TurnLeft(double theta) const//left rotate theta(ARC)
			{return Point2D(x*cos(theta)-y*sin(theta),x*sin(theta)+y*cos(theta));}
			
			inline Point2D TurnRight(double theta) const
			{return TurnLeft(-theta);}
			
			inline Point2D TurnL90() const
			{return Point2D(-y,x);}
			
			inline Point2D TurnR90() const
			{return Point2D(y,-x);}
			
		#ifdef PAL_BASICFUNCTIONS_POSIZE_CPP
			Point ToPosizePoint() const
			{return Point(x,y);}
			
			Point2D(const Point &p):x(p.x),y(p.y) {}
		#endif
			
			Point2D(double _x,double _y):x(_x),y(_y) {}
			
			Point2D() {}
	};
	const Point2D Point2D::Zero(0,0);
	using Vector2D=Point2D;
			
	inline Point2D MidPoint(const Point2D &p1,const Point2D &p2)
	{return Point2D((p1.x+p2.x)*0.5,(p1.y+p2.y)*0.5);}
	
	inline double Dist_2(const Point2D &p1,const Point2D &p2)
	{return XX(p1.x-p2.x)+XX(p1.y-p2.y);}
	
	inline double Dist(const Point2D &p1,const Point2D &p2)
	{return sqrt(XX(p1.x-p2.x)+XX(p1.y-p2.y));}
	
	inline double Cos(const Point2D &p1,const Point2D &p2)//cos of P0p1,P0p2
	{return p1*p2/(p1.Mo()*p2.Mo());}
	
	inline double Sin(const Point2D &p1,const Point2D &p2)//sin of P0p1,P0p2
	{return p1%p2/(p1.Mo()*p2.Mo());}
	
	class Line2D
	{
		public:
			Point2D p1,p2;
			bool IsSegment;
			
			inline Line2D Line() const
			{
				if (IsSegment) return Line2D(p1,p2);
				else return *this;
			}
			
			inline bool In(const Point2D &p) const
			{
				if (p==p1||p==p2) return 1;
				if (IsSegment) return ET0(Cos(p1-p,p2-p)+1);
				else return ET0(1-fabs(Cos(p1-p,p2-p)));
			}
			
			Line2D(const Point2D &_p1,const Point2D &_p2,bool issegment=0):p1(_p1),p2(_p2),IsSegment(issegment) {}
			
			Line2D() {}
	};
			
	inline bool _Cross(const Line2D &l1,const Line2D &l2)
	{
		if (l2.In(l1.p1)||l2.In(l1.p2)||l1.In(l2.p1)||l1.In(l2.p2)) return 1;
		return ((l1.p1-l2.p1)%(l2.p2-l2.p1))*((l1.p2-l2.p1)%(l2.p2-l2.p1))<0;
	}
	
	bool Cross(const Line2D &l1,const Line2D &l2)
	{
		using namespace std;
		if (l1.IsSegment&&l2.IsSegment)
		{
			if (max(l1.p1.x,l1.p2.x)<min(l2.p1.x,l2.p2.x)||min(l1.p1.x,l1.p2.x)>max(l2.p1.x,l2.p2.x)
				||max(l1.p1.y,l1.p2.y)<min(l2.p1.y,l2.p2.y)||min(l1.p1.y,l1.p2.y)>max(l2.p1.y,l2.p2.y))
				return 0;
			return _Cross(l1,l2)&&_Cross(l2,l1);
		}
		else if (!l1.IsSegment&&!l2.IsSegment) return !ET0(1-fabs(Cos(l1.p2-l1.p1,l2.p2-l2.p1)));
		else if (!l1.IsSegment) return _Cross(l2,l1);
		else return _Cross(l1,l2);
	}
	
	inline Point2D Intersection(const Line2D &l1,const Line2D &l2)
	{
		double c1x=l1.p2.x-l1.p1.x,c2x=l2.p2.x-l2.p1.x,c1y=l1.p2.y-l1.p1.y,c2y=l2.p2.y-l2.p1.y;
		return Point2D((c1y*c2x*l1.p1.x-c2y*c1x*l2.p1.x+c1x*c2x*(l2.p1.y-l1.p1.y))/(c1y*c2x-c2y*c1x),
					   (c1x*c2y*l1.p1.y-c2x*c1y*l2.p1.y+c1y*c2y*(l2.p1.x-l1.p1.x))/(c1x*c2y-c2x*c1y));
	}
	
	Point2D RecentPointFromPointToLine(const Point2D &p,const Line2D &l)
	{
		Point2D re;
		re=Intersection(l,Line2D((l.p2-l.p1).TurnL90()+p,p,0));
		if (l.In(re)) return re;
		if (Dist_2(l.p1,p)<Dist_2(l.p2,p)) return l.p1;
		else return l.p2;
	}
	
	inline Point2D MirrorPointByLine(const Point2D &p,const Line2D &l)
	{
		Point2D re;
		re=Intersection(l.Line(),Line2D((l.p2-l.p1).TurnL90()+p,p,0));
		return re+re-p;
	}
	
	inline Line2D MirrorLineByLine(const Line2D &l,const Line2D &mirror)
	{return Line2D(MirrorPointByLine(l.p1,mirror),MirrorPointByLine(l.p2,mirror),l.IsSegment);}
	
	class HalfPlane2D
	{
		public:
			Point2D p;
			Vector2D v;
			
			inline Line2D ToLine() const
			{return Line2D(p,p+v,0);}
			
			inline bool In(const Point2D &pt) const
			{return v%(pt-p)>=0;}
			
			HalfPlane2D(const Point2D &_p,const Vector2D &_v):p(_p),v(_v) {}
			
			HalfPlane2D() {}
	};
	
	class Circle2D
	{
		public:
			const static Circle2D Unit;
			
			Point2D o;
			double r;
			
			inline double Perimeter() const
			{return 2*3.1415926535*r;}
			
			inline double Area() const
			{return 3.1415926535*r*r;}
			
			inline bool In(const Point2D &p) const
			{return Dist_2(p,o)<=XX(r);}
			
			Circle2D(const Point2D &p1,const Point2D &p2,const Point2D &p3)
			{
				Line2D l1,l2;
				l1.p1=MidPoint(p1,p2);l1.p2=(p1-p2).TurnL90()+l1.p1;l1.IsSegment=0;
				l2.p1=MidPoint(p2,p3);l2.p2=(p2-p3).TurnL90()+l2.p1;l2.IsSegment=0;
				o=Intersection(l1,l2);
				r=Dist(o,p1);
			}
			
			Circle2D(const Point2D &_o,double _r):o(_o),r(_r) {}
			
			Circle2D() {}
	};
	const Circle2D Circle2D::Unit(Point2D::Zero,1);
	
	class Polygon2D
	{
		protected:
			std::vector <Point2D> v;//Rule:first is same with the last
		public:
			inline double Area() const
			{
				if (v.size()<=1) return 0;
				double ans=v[v.size()-1]%v[0];
				for (unsigned long long i=0;i<v.size()-1;++i)
					ans+=v[i]%v[i+1];
				return fabs(ans)/2;
			}
			
			inline bool In(const Point2D &p)
			{
				if (v.size()<=1) return 0;
				unsigned long long cnt=0;
				Line2D l(p,p,1);
				l.p2.x=ERO;
				for (unsigned long long i=0;i<v.size()-1;++i)
				{
					Line2D li(v[i],v[i+1],1);
					if (li.In(p)) return 1;//point in the edge of the polygon
					if (!ET0(li.p1.y-li.p2.y))//this edge is not horizonal
						if (l.In(li.p1))
							cnt+=li.p1.y>li.p2.y;
						else if (l.In(li.p2))
							cnt+=li.p2.y>li.p1.y;
						else cnt+=Cross(l,li);
				}
				return cnt&1;
			}
			
			inline bool In(const Line2D &l)
			{
				if (!l.IsSegment||v.size()<=1) return 0;
				if (!In(l.p1)||!In(l.p2)) return 0;
				std::vector <Point2D> PointSet;
				for (unsigned long long i=0;i<v.size();++i)
					if (l.In(v[i]))
						PointSet.push_back(v[i]);
					else
					{
						Line2D li(v[i],v[i+1],1);
						if (i!=v.size()-1&&!li.In(l.p1)&&!li.In(l.p2)&&Cross(l,li))
							return 0;
					}
				if (PointSet.empty()) return 1;
				sort(PointSet.begin(),PointSet.end(),Point2D::CompX);
				for (unsigned long long i=0;i<PointSet.size()-1;++i)
					if (!In(MidPoint(PointSet[i],PointSet[i+1])))
						return 0;
				return 1;
			}
			
			inline unsigned long long Count() const
			{
				if (v.size()>=2) return v.size()-1;
				else return 0;
			}
			
			inline unsigned long long Size() const
			{return v.size();}
			
			inline void Erase(int pos)
			{v.erase(v.begin()+pos);}
			
			inline void Insert(const Point2D &p,int pos)
			{v.insert(v.begin()+pos,p);}
			
			inline void Pushback(const Point2D &p)
			{v.push_back(p);}
			
			inline Point2D& operator [] (int pos)
			{return v[pos];}
			
			Polygon2D(const std::vector <Point2D> &_v):v(_v) {}
			
			Polygon2D() {}
	};
	
	Circle2D MinCircleCover_NoRand(const std::vector <Point2D> &pts)
	{
		if (pts.empty()) return Circle2D(Point2D::Zero,0);
		if (pts.size()==1) return Circle2D(pts[0],0);
		Circle2D ans(pts[0],0);
		for (unsigned long long i=1;i<pts.size();++i)
			if (!ans.In(pts[i]))
			{
				ans=Circle2D(pts[i],0);
				for (unsigned long long j=0;j<i;++j)
					if (!ans.In(pts[j]))
					{
						ans=Circle2D(MidPoint(pts[i],pts[j]),Dist(pts[i],pts[j])*0.5);
						for (unsigned long long k=1;k<j;++k)
							if (!ans.In(pts[k]))
								ans=Circle2D(pts[i],pts[j],pts[k]);
					}
			}
		return ans;
	}
	
	Circle2D MinCircleCover(std::vector <Point2D> pts)
	{
		if (pts.empty()) return Circle2D(Point2D::Zero,0);
		if (pts.size()==1) return Circle2D(pts[0],0);
		srand(time(NULL));
		Point2D tmp;
		for (unsigned long long i=1,pos1,pos2;i<pts.size()*10;++i)
		{
			pos1=rand()%pts.size();//is rand large enough??
			pos2=rand()%pts.size();
			tmp=pts[pos1];
			pts[pos1]=pts[pos2];
			pts[pos2]=tmp;
		}
		return MinCircleCover_NoRand(pts);
	}
	
	//...
};

#endif
