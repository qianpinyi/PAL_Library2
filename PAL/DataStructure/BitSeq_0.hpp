#ifndef PAL_DATASTRUCTURE_BITSEQ_0_HPP
#define PAL_DATASTRUCTURE_BITSEQ_0_HPP

#include "../Common/PAL_Types_0.hpp"
#include "../Common/PAL_TemplateTools_0.hpp"

namespace PAL::DS
{
	template <int N> class BitSeq requires (N>0)
	{
		protected:
			enum {size=(N+63)/64};
			enum class NoInit {_};
			
			Uint64 data[size];
			
			constexpr Uint64 pos0(Uint64 p)
			{return p>>6;}
			
			constexpr Uint64 pos1(Uint64 p)
			{return p&0b111111;}
			
			BitSeq(NoInit) {}
			
		public:
			
			constexpr int Size() noexcept
			{return Size;}
			
			Uint64* RawData() noexcept
			{return data;}
			
			Uint64& Raw(Uint64 p) noexcept
			{
				PAL_Assert(p<size);
				return data[p];
			}
			
			constexpr bool Get(Uint64 p) noexcept
			{
				PAL_Assert(p<N);
				return GetBitMask(data[pos0(p)],pos1(p));
			}
			
			constexpr void Set(Uint64 p,bool val) noexcept
			{
				PAL_Assert(p<N);
				if (val)
					SetBitMask1(data[pos0(p)],pos1(p));
				else SetBitMask0(data[pos0(p)],pos1(p));
			}
			
			//Bitwise operations
			static BitSeq <N> False() noexcept
			{
				return BitSeq();
			}
			
			static BitSeq <N> True() noexcept
			{
				return Not(False());
			}
			
			constexpr BitSeq <N> This()
			{
				return *this;
			}
			
			constexpr BitSeq <N> Not()
			{
				BitSeq <N> &x=*this;
				BitSeq <N> z;
				for (Uint64 i=0;i<size;++i)
					z.Raw(i)=~x.Raw(i); 
			}
			
			constexpr BitSeq <N> And(const BitSeq <N> &y)
			{
				BitSeq <N> &x=*this;
				BitSeq <N> z;
				for (Uint64 i=0;i<size;++i)
					z.Raw(i)=x.Raw(i)&y.Raw(i);
				return z;
			}
			
			constexpr BitSeq <N> Or(const BitSeq <N> &y)
			{
				BitSeq <N> &x=*this;
				BitSeq <N> z;
				for (Uint64 i=0;i<size;++i)
					z.Raw(i)=x.Raw(i)|y.Raw(i);
				return z;
			}
			
			constexpr BitSeq <N> Xor(const BitSeq <N> &y)
			{
				BitSeq <N> &x=*this;
				BitSeq <N> z;
				for (Uint64 i=0;i<size;++i)
					z.Raw(i)=x.Raw(i)^y.Raw(i);
				return z;
			}
			
			constexpr BitSeq <N> Nor(const BitSeq <N> &y)
			{
				return Or(y).Not();
			}
			
			constexpr BitSeq <N> Xnor(const BitSeq <N> &y)
			{
				return Xor(y).Not();
			}
			
			constexpr BitSeq <N> Nand(const BitSeq <N> &y)
			{
				return And(y).Not();
			}
			
			constexpr BitSeq <N> Eq(const BitSeq <N> &y)
			{
				return Xnor(y);
			}
			
			constexpr BitSeq <N> Sheffer(const BitSeq <N> &y)
			{
				return Nand(y);
			}
			
			constexpr BitSeq <N> Imply(const BitSeq <N> &y)
			{
				return Not().Or(y);
			}
			
			constexpr BitSeq <N> Rimply(const BitSeq <N> &y)
			{
				return Or(y.Not());
			}
			
			constexpr BitSeq <N> Less(const BitSeq <N> &y)
			{
				return Not().And(y);
			}
			
			constexpr BitSeq <N> Greater(const BitSeq <N> &y)
			{
				return And(y.Not());
			}
			
			//Bitsequence operations
			template <int M> constexpr BitSeq <N+M> Concat(const BitSeq <M> &y)
			{
				BitSeq <N+M> z;
				for (Uint64 i=0;)
			}
			
			template <int L,int R> constexpr BitSeq <R-L> Slice() requires(0<=L&&l<R&&R<=N) //[L,R)
			{
				
			}
			
			Trunc()
			//...
			
			Zext()
			
			Sext()
			
			//Shift operations
			Reverse()
			
			ShiftL()
			
			ShiftR()
			
			ShiftRA()
			
			//Arithmetic operations
			Add()
			
			Sub()
			
			Mul()
			
			Div()
			
			Mod()
			
			//Cast operations
			ToUint128()
			ToUint64()
			ToUint32()
			ToUint16()
			ToUint8()
			ToSint128()
			ToSint64()
			ToSint32()
			ToSint16()
			ToSint8()
			ToBool()
			
			FromUint128()
			FromUint64()
			
			BitSeq():data{0} {}
	}
	
};

#endif
