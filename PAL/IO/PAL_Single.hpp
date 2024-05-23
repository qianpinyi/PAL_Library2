#ifndef PAL_IO_PAL_SINGLE_HPP
#define PAL_IO_PAL_SINGLE_HPP

#ifdef PAL_IO_PAL_XOUT_0_HPP
namespace PAL::IO
{
	int XoutType::XoutTypeIDAssign=8;
	void (*XoutType::FaultSolver)(void)=nullptr;

	XoutType Info			=PredefinedXoutType(0),
			 Warning		=PredefinedXoutType(1),
			 Error			=PredefinedXoutType(2),
			 Debug			=PredefinedXoutType(3),
			 Fault			=PredefinedXoutType(4),
			 Test			=PredefinedXoutType(5),
			 NoneXoutType	=PredefinedXoutType(-1);
};
#endif

#ifdef PAL_IO_PAL_XOUT_1_HPP
namespace PAL::IO
{
	Xout xout(XoutStdoutChannel{});
};
#endif

#endif
