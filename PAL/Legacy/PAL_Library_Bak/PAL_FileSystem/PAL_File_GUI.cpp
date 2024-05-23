#ifndef PAL_FILE_GUI
#define PAL_FILE_GUI 1

#include "../PAL_BasicFunctions/PAL_BasicFunctions_File.cpp"
#include "../PAL_BasicFunctions/PAL_BasicFunctions_Charset.cpp"
#include "../PAL_GUI/PAL_GUI_0.cpp"

struct CoMoFileGUIData
{
	using namespace std;
	SDL_Mutex *mu=NULL;
	queue <string> que;
	string tar;
	bool IsCopy;
//	void (*GetSizeFuncCallback)()=
	atomic_bool CoMoFileRunning;
	
	static int CoMoFileInQueue(void *userdata)
	{
		while (CoMoFileRunning)
		{
			
		}
	}
	
	void GetAllFileSize(const string &str)
	{
		
	}
	
	void CoMoFile(const vector <string> &src)
	{
		for (auto &vp:src)
			GetAllFileSize(vp);
		
	}
};


int CoMoFileWithGUI(const std::vector <std::string> &src,const std::string &tar,bool IsCopy)
{
	using namespace PAL_GUI;
	using namespace Charset;
	
	
	
	auto win=new PUI_Window({400,400,400,240});
	new TinyText(0,win->BackgroundLayer,new PosizeEX_Fa6(2,3,20,20,20,30),AnsiToUtf8(""));
	
}

#endif
