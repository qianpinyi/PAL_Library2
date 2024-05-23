#ifndef PAL_GUI_PLOTS_CPP
#define PAL_GUI_PLOTS_CPP 1

/*
	21.6.9
*/

#include "../PAL_BuildConfig.h" 

#include "PAL_GUI_0.cpp"

namespace PAL::Legacy::PAL_GUI
{
	class PlotViewer2D:public Widgets
	{
		protected:
			enum
			{
				PlotType_None=0,
				PlotType_FuncX_Dot,
				PlotType_FuncX_Ployline,
				PlotType_FuncX_Smooth,
				PlotType_FuncXY_Dot,
				PlotType_FuncXY_Area,
				PlotType_Ployline,
				PlotType_PloySmooth
			};
			Surface *plot=NULL;
			int PlotType=0,
				DotSize=5,
				LineWidth=3;
			bool EnableZoom=1,
				 EnableSight=1,
				 EnableAxis=1,
				 EnableBaseline=1,
				 EnableHighlightDot=1;
			double XRangeL,XRangeR,
				   YRangeL,YRangeR;
//			RGBA ...;
			
		public:
			
			
	};
	
	class EasyPlotViewer2D
	{
		protected:
			
		public:
			
			
	};
};

#endif
