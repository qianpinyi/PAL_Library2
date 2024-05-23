#ifndef PAL_GUI_FRAGMENTS_CPP
#define PAL_GUI_FRAGMENTS_CPP 1

#include "PAL_GUI_0.cpp"
#include "../PAL_BasicFunctions/PAL_System.cpp"

namespace PAL::Legacy::PAL_GUI
{
	template <class T> int TextEditLineColorChange(TextEditLine <T> *tel,bool resetNormal)
	{
		if (resetNormal)
			for (int i=0;i<=2;++i)
				tel->SetBorderColor(i,RGBA_NONE);
		else
		{
			tel->SetBorderColor(0,{255,198,139,255});
			tel->SetBorderColor(1,{255,156,89,255});
			tel->SetBorderColor(2,{255,89,0,255});
		}
		return resetNormal;
	}
	
	template <typename T> WidgetPtr PopupMessageLayerReceiveTextInput(const string &title,const string &button,const string &initstr,int (*func)(T&,const stringUTF8 &str),const T &funcdata)
	{
		auto mbl=new MessageBoxLayer(0,title,400,80);
		mbl->SetClickOutsideReaction(1);
		mbl->EnableShowTopAreaColor(1);
		auto tel=new TextEditLine <Triplet <decltype(func),T,MessageBoxLayer*> > (0,mbl,new PosizeEX_Fa6(2,2,20,85,40,10),
			[](auto &funcdata,const stringUTF8 &str,auto *tel,bool isenter)
			{
				if (isenter)
					if (funcdata.a(funcdata.b,str)==0)
						funcdata.c->Close();
			},{func,funcdata,mbl});
		new Button <decltype(tel)> (0,mbl,new PosizeEX_Fa6(1,2,60,20,40,10),button,[](auto &tel)->void{tel->TriggerFunc();},tel);
		tel->SetText(initstr,0);
		tel->StartTextInput();
		return mbl->This();
	}
	
	void InfoMessageButton(const string &title,const string &msg,unsigned int beep=SetSystemBeep_Info,const string &button=PUIT("确定"))
	{
		SetSystemBeep(beep);
		auto msgbx=new MessageBoxButtonI(0,title,msg);
		msgbx->AddButton(button,nullptr,0);
	}
	
	void SafeMessageButton(const string &title,const string &msg,unsigned int beep=SetSystemBeep_Info,const string &button=PUIT("确定"))
	{
		PUI_SendFunctionEvent<Tetrad<string,string,unsigned,string> >([](Tetrad<string,string,unsigned,string> &data)
		{
			InfoMessageButton(data.a,data.b,data.c,data.d);
		},{title,msg,beep,button});
	}
	
	#define PUI_EasyMain(naame,auuthor,args...) 				\
		void _PUI_EasyMain(int argc,char **argv);				\
		int SDL_main(int argc,char **argv)							\
		{														\
			string name=naame,version,author=auuthor,title;		\
			Posize winps=PUI_WINPS_DEFAULT;						\
			int renderer=PUI_PreferredRenderer;					\
			unsigned int flag=PUI_Window::PUI_FLAG_RESIZEABLE;	\
			bool (*func)(const PUI_Event*,bool,int&)=nullptr;	\
			args;												\
			if (title=="")										\
				title=name+" "+version+" By:"+author;			\
			SetCmdUTF8AndPrintInfo(title,version,author);		\
			PUI_SetPreferredRenderer(renderer);					\
			PAL_GUI_Init(winps,title,flag);						\
			_PUI_EasyMain(argc,argv);							\
			if (func==nullptr)									\
				return PUI_EasyEventLoopAndQuit();				\
			else return PUI_EasyEventLoopAndQuit(func);			\
		}														\
		void _PUI_EasyMain(int argc,char **argv)
};
#endif
