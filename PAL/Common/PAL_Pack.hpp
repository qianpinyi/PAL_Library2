#ifndef PAL_PACK_HPP
#define PAL_PACK_HPP

#include "PAL_Types_0.hpp"

namespace PAL
{
	class DataWithSize
	{
		public:
			void *data=nullptr;
			Uint64 size=0;
			
			Uint8* begin()
			{return (Uint8*)data;}
			
			Uint8* end()
			{
				if (data==nullptr)
					return nullptr;
				else return (Uint8*)data+size;
			}
			
			DataWithSize(void *_data,Uint64 _size):data(_data),size(_size) {}
			DataWithSize() {};
	};
	
	class DataWithSizeUnited:public DataWithSize
	{
		public:
			enum//bit 0~7:mode 8~X feature
			{
				F_Hex=0,
				F_Char=1,
				F_Mixed=2,
			};
			
			Uint64 unitSize;
			Uint64 flags;
			
			DataWithSizeUnited(void *_data,Uint64 _size,Uint64 _unitsize,Uint64 _flags=F_Hex):DataWithSize(_data,_size),unitSize(_unitsize),flags(_flags) {}
	};
	
	class SizedBuffer
	{
		public:
			char *data;
			Uint64 size;
			
			operator DataWithSize ()
			{
				return DataWithSize(data,size);
			}
			
			~SizedBuffer()
			{
				if (data!=nullptr)
					delete[] data;
			}
			
			SizedBuffer(Uint64 _size):size(_size)
			{
				if (size==0)
					data=nullptr;
				else data=new char[size];
			}
			
			SizedBuffer(const SizedBuffer &)=delete;
			SizedBuffer(const SizedBuffer &&)=delete;
			SizedBuffer& operator = (const SizedBuffer &)=delete;
			SizedBuffer& operator = (const SizedBuffer &&)=delete;
	};
};

#endif
