#ifndef PAL_TEMP_TERMINALTEXTBOX_HPP
#define PAL_TEMP_TERMINALTEXTBOX_HPP

#include "../PAL_GUI/PAL_GUI_0.cpp"

namespace PAL::Legacy::PAL_GUI
{
	WidgetType_TerminalTextBox=Widgets::GetNewWidgetTypeID("TerminalTextBox");
	
	class BetterTextEditBox:public Widgets 
	{
		protected:
			
			
		public:
			struct CharData
			{
				char32_t ch;
				Uint8 co,bg,effect;
			};
			struct LineText
			{
				vector <CharData> //Supposing a single line won't be too long.
			};
			struct CharTextCache;
			{
				
			};
			Texts;
			WarpLines;
			
			InsertText();
			RemoveText();
			SelectText();
			CopyText();
			
	};
	
	class TerminalTextBox:public Widgets
	{
		public:
			enum
			{
				TerminalEvent_None,
				TerminalEvent_Any,
				TerminalEvent_TitleSet,
			};
			
		protected:
			struct CharCache
			{
				
			};
			
			struct CharEffect
			{
				RGBA co=RGBA_NONE,
					 bg=RGBA_NONE;
				Uint32 flag=0;
			};
			
			struct CharData
			{
//				string ch;
				SharedTexturePtr tex;
				int w;
				CharEffect effect;
			};
			
			using LineText=stringUTF8_WithData <CharData>;
			
			struct LineBuffer
			{
				vector <int> NewLinePos;
				LineText str;
				
				int SumLines()
				{
					
				}
				
				void UpdateLines(/*...*/)
				{
					//...
				}
			};
			
			struct MainBuffer
			{
				SplayTree <LineBuffer*> Text;
				
				int Lines()
				{return Text.size();}
				
				void InsertLine(int p,const LineText &str)
				{
					
				}
				
				void NewLine(const LineText &str="")
				{
					InsertLine(Lines(),str);
				}
				
				void AppendText(const LineText &str)//In last line
				{
					
				}
			};
			
			struct BackBuffer
			{
//				SplayTree <LineBuffer*> Text;
				//2D array Text?
				
				int Lines()
				{return Text.size();}
				
				void AppendText(const LineText &str)
				{
					
				}
			};
			
			struct TerminalKernel//It can exist indepently, and reconstruct a new widget.
			{
				TerminalTextBox *TTB=nullptr;
				SubProcessController *SPC=nullptr;
				PAL_Thread <int,TerminalKernel*> *ReadingThread=nullptr;
				MainBuffer *MB=nullptr;
				BackBuffer *BB=nullptr;
				PUI_Font_Struct Font;//MonoFont is prefered.
				string Title;
				bool ForceWideAlign=false;
				int (*EventCB)(TerminalKernel*,int,void*)=nullptr;
				Point Cursor;
				bool BackEnabled=false;
				
				//Set this before start ReadingThread
				//EventCB will be called in MainThread by SendFunctionEvent. 
				void RegisterCallback(int (*func)(TerminalKernel*,int,void*))//This,EventType,ExtraData
				{
					EventCB=func;
				}
				
				void AppendText(const LineText &str)
				{
					if (BackEnabled)
						BB->AppendText(str);
					else MB->AppendText(str);
				}
				
				void NewLine()
				{
					
				}
				
				static int ReadingThreadFunc(TerminalKernel *&This)
				{
					string UnsolvedText;
					CharEffect CurrentEffect;
					
					auto Submit=[This,&CharEffect](string &str)
					{
						if (str!="")
						{
							LineText lt=str;
							for (int i=0;i<lt.length();++i)
								lt(i).effect=CurrentEffect;
							PUI_SendFunctionEvent<Doublet<TerminalKernel*,LineText>>([](Doublet<TerminalKernel*,LineText> &str)
							{
								This->AppendText(lt);
							},{This,});
						}
						str="";
					};
					
					SizedBuffer buf(512);
					InThreadData *da=&This->itd;
					while (true)
					{
					READ_AGAIN:
						int sz=SPC->Read(buf.data,buf.size-1);
						if (sz<0)
						{
							//Solve exception here...
						}
						else buf.data[sz]='\0';
						
						using namespace VT100CtlSeq;
						using namespace ControlSequenceName;
						string s=lastseq+buf.data;//efficient? copy lastseq to the begin of buffer?
						lastseq="";
						string txt;
						const char *s=str.c_str();
						while (*s)
						{
							switch (*s)
							{
								case '\e':
								{
									Submit(txt);
									auto code=ParseControlSequenceWithoutESC(s);
									switch (code())
									{
										case TITLESET:
											PUI_SendFunctionEvent<Doublet<TerminalKernel*,string>>([](Doublet<TerminalKernel*,string> &data)
											{
												data.a->Title=data.b;
												if (EventCB!=nullptr)
													EventCB(data.a,TerminalEvent_TitleSet,nullptr);
											},This);
											break;
										case SGR:
											if (RGBA c=SGR_ToRGBA(code.BackOr0());c!=RGBA_NONE)
												if (InRange(code.Back(),Black,LightGray)||InRange(code.Back(),DarkGray,White))
													co=c;
												else bg=c;
											else switch (code.BackOr0())
											{
												case ResetFore:	co=RGBA_NONE;		break;
												case ResetBG:	bg=RGBA_NONE;		break;
												case Reset:
													co=bg=RGBA_NONE;
													break;
												default:
													/*
														Reset			=0,
														Bold			=1,
														Dim				=2,
														Underlined		=4,
														Blink			=5,
														Reverse			=7,
														Hidden			=8,
														ResetBold		=21,
														ResetDim		=22,
														ResetUnderlined	=24,
														ResetBlink		=25,
														ResetReverse	=27,
														ResetHidden		=28,
													*/
													//...
													break;
											}
											break;
										
										case PARSE_REACHEND:
											lastseq=s;
											goto READ_AGAIN;
										case PARSE_REACHESC:
										case PARSE_REACHTERM:
										default:
											txt+=*s;
											continue;
									}
									s=code.ns;
									if (*s!='\0')
										continue;
									else break;
								}
								case '\r':
									//SetCursor to line begin.
									//...
								case '\n':
									//New line.
									//...
								default:
									//...
							}
						}
						Submit(txt);
					}
					delete[] buffer;
					return 0;
				}
			};
			TerminalKernel *Kernel=nullptr;
			
		public:
			
	};
};

#endif
