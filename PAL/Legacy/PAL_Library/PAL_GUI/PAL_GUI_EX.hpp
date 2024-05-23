#ifndef PAL_LEGACY_PAL_GUI_EX_HPP
#define PAL_LEGACY_PAL_GUI_EX_HPP

#include "PAL_GUI_0.cpp"
#include <cassert>

namespace PAL::Legacy::PAL_GUI
{
	Widgets::WidgetType WidgetType_BetterTextEditBox=Widgets::GetNewWidgetTypeID("BetterTextEditBox");
	class BetterTextEditBox:public Widgets
	{
		public:
			class EditListener
			{
				protected:
					BetterTextEditBox *TEB=nullptr;
					
				public:
					virtual void SetTEB(BetterTextEditBox *teb)
					{
						TEB=teb;
					}
					
					virtual void RightClick() {}
					
					virtual void BeforeClear(bool isInnerChange) {}
					virtual void AfterClear(bool isInnerChange) {}
					
					virtual void BeforeAddNewLine(int p,const stringUTF8 &strUtf8,bool isInnerChange) {}
					virtual void AfterAddNewLine(int p,const stringUTF8 &strUtf8,bool isInnerChange) {}
					
					virtual void BeforeAddText(Point pt,const stringUTF8 &strUtf8,bool isInnerChange) {}
					virtual void AfterAddText(Point pt,const stringUTF8 &strUtf8,bool isInnerChange) {}
					
					virtual void BeforeSetText(const stringUTF8 &strUtf8,bool isInnerChange) {}
					virtual void AfterSetText(const stringUTF8 &strUtf8,bool isInnerChange) {}
					
					virtual void BeforeDeleteText(Point pt1,Point pt2,bool isInnerChange) {}
					virtual void AfterDeleteText(Point pt1,Point pt2,bool isInnerChange) {}
					
					virtual void BeforeDeleteLine(int p,bool isInnerChange) {}
					virtual void AfterDeleteLine(int p,bool isInnerChange) {}
					
					virtual void Edit() {}
			};
			
			class LineListener
			{
				protected:
					BetterTextEditBox *TEB=nullptr;
				
				public:
					virtual void Update() {}
					virtual void Insert() {}
					virtual void Delete() {}
					
					virtual ~LineListener() {}
			};
			
			class EventListener
			{
				protected:
					BetterTextEditBox *TEB=nullptr;
				
				public:
					virtual void BeforeCheckEvent(const PUI_Event *event) {}
					virtual void AfterCheckEvent(const PUI_Event *event) {}
					
					virtual void BeforeCheckPos(const PUI_PosEvent *event,int mode) {}
					virtual void AfterCheckPos(const PUI_PosEvent *event,int mode) {}
					
					virtual void BeforeShow(Posize &lmt) {}
					virtual void AfterShow(Posize &lmt) {}
					
					virtual ~EventListener() {}
			};
			
			struct CharAttri
			{
				RGBA co=RGBA_NONE,
					 bg=RGBA_NONE;//if RGBA_NONE, means using DefaultFont
				bool bold=false,
					 italic=false;
			};

			class FontGroup
			{
				protected:
					int size=0;//0 means uninited
					TTF_Font* font[8]{0};
					string path[8];
					
					void Close(int i)
					{
						if (font[i]==NULL)
							return;
						TTF_CloseFont(font[i]);
						font[i]=NULL;
					}
					
					void Clear()
					{
						for (int i=0;i<8;++i)
							Close(i);
					}
					
					bool IsUnicode(int i)
					{return i&0x4;}
					
					bool IsBold(int i)
					{return i&0x1;}
					
					bool IsItalic(int i)
					{return i&0x2;}
					
				public:
				
					PUI_Font Get(int i)
					{
						if (font[i]!=NULL)
							return font[i];
						else if (font[i&0x4]!=NULL)
							return font[i&0x4];
						else return PUI_Font(size);
					}
					
					void SetFile(const string &file,int i)
					{
						Close(i);
						path[i]=file;
						if (size==0)
							return;
						font[i]=TTF_OpenFont(path[i].c_str(),size);
					}
					
					void SetSize(int siz)
					{
						if (siz<0||siz==size)
							return;
						Clear();
						size=siz;
						for (int i=0;i<8;++i)
							if (path[i]!="")
								font[i]=TTF_OpenFont(path[i].c_str(),size);
					}
					
					int Index(bool unicode,bool bold,bool italic)
					{return unicode*4+italic*2+bold;}
					
					PUI_Font Get(bool unicode,bool bold,bool italic)
					{return Get(Index(unicode,bold,italic));}
					
					void SetFile(const string &file,bool unicode,bool bold,bool italic)
					{SetFile(file,Index(unicode,bold,italic));}
					
					~FontGroup()
					{Clear();}
			};
		
		protected:
			
			struct CharData
			{
//				SharedTexturePtr cache;
				CharAttri attri;
			};
			
			struct LineData
			{
				stringUTF8_WithData <CharData> str;
//				SharedTexturePtr cache;
				
				LineData(const stringUTF8_WithData <CharData> &_str):str(_str) {}
				LineData() {}
			};
			
			struct CharTexCache
			{
				
			};
			
//			struct EachChData
//			{
//				SharedTexturePtr tex[2];
//				int timeCode[2]={0,0};
//			};
			
//			SharedTexturePtr ASCIItex[128][2];
//			int timeCodeOfASCIItex[128][2];
			
			enum
			{
				Stat_NoFocus=0,
				Stat_UpFocus=1,
				Stat_DownLeft=2,
				Stat_DownRight=3
			}stat=Stat_NoFocus;
//			SplayTree <stringUTF8_WithData <EachChData> > Text;
			stringUTF8 editingText;
			vector <int> TextLengthCount;//Using map?
			stack <int> TextLengthChange;
			int MaxTextLength=0;
			int EachChWidth=16,//chinese character is twice the english character width
				EachLineHeight=28;
//			PUI_Font font;
//			string FontPath="";
			LargeLayerWithScrollBar *fa2=NULL;
			bool Editing=0,
				 ShowBorder=1,
				 AutoNextLine=0,//implement it ten years later =_=|| 
				 ShowSpecialChar=0,
				 EnableScrollResize=0,
				 EnableEdit=1;
			int	StateInput=0,
				BorderWidth=3,
				EditingTextCursorPos; 
//				NewestCode=1;
//			void (*EachChangeFunc)(T&,BetterTextEditBox*,bool)=NULL;//bool:Is inner change//It is not so beautiful...
//			T EachChangeFuncdata;
//			void (*RightClickFunc)(T&,const Point&,BetterTextEditBox*)=NULL;
//			T RightClickFuncData;
			Point Pos=ZERO_POINT,Pos_0=ZERO_POINT,//This pos(_0) is not related to pixels
				  EditingPosL=ZERO_POINT,EditingPosR=ZERO_POINT,
				  LastCursorPos=ZERO_POINT;//this is related to pixels
			RGBA TextColor[3],
				 BackgroundColor[4],//bg,choosepart stat0,1,2/3
				 BorderColor[3],//stat0,1,2/3
				 LastTextColor[3];
			
			SplayTree <LineData> Text;
			EditListener *Listener=nullptr;//it will always be non-nullptr, if not provided, empty Listener will be here, so check nullptr is not needed
			FontGroup *DefaultFont=nullptr;
//			int LineHeight=24;
			
			void InitDefualtColor()
			{
				TextColor[0]=ThemeColorMT[0];
				TextColor[1]=ThemeColorMT[1];
				TextColor[2]=ThemeColorM[6];
//				BackgroundColor[0]=ThemeColorBG[0];
				BackgroundColor[0]=RGBA_NONE;
				for (int i=0;i<=2;++i)
					BorderColor[i]=BackgroundColor[i+1]=ThemeColorM[i*2+1],
					LastTextColor[i]=RGBA_NONE;
			}
			
			void InitDefaultFont(int defaultSize)
			{
				DefaultFont=new FontGroup();
				//...???
			}
			
			inline bool CheckWidth(const string &str)
			{return str.length()>=2;}
			
			int GetChLenOfEachWidth(const string &str)
			{
				if (str=="\t") return 4;
				return 1+CheckWidth(str);
			}
			
			int GetStrLenOfEachWidth(const stringUTF8_WithData <CharData> &strUtf8)
			{
				int re=0;
				for (int i=0;i<strUtf8.length();++i)
					re+=GetChLenOfEachWidth(strUtf8[i]);
				return re;
			}
			
			int GetChWidth(const string &str)
			{
				if (str=="\t") return 4*EachChWidth; 
				return EachChWidth+CheckWidth(str)*EachChWidth;
			}
			
			inline int StrLenWithoutSlashNR(const stringUTF8_WithData <CharData> &strUtf8)
			{
				if (strUtf8.length()==0) return 0;
				if (strUtf8[strUtf8.length()-1]=="\n")
					if (strUtf8.length()>=2&&strUtf8[strUtf8.length()-2]=="\r")
						return strUtf8.length()-2;
					else return strUtf8.length()-1;
				else if (strUtf8[strUtf8.length()-1]=="\r")
					if (strUtf8.length()>=2&&strUtf8[strUtf8.length()-2]=="\n")
						return strUtf8.length()-2;
					else return strUtf8.length()-1;
				else return strUtf8.length();
			}
			
			inline Point EnsurePosValid(const Point &pt)
			{return Point(EnsureInRange(pt.x,0,StrLenWithoutSlashNR(Text[pt.y].str)),EnsureInRange(pt.y,0,Text.size()-1));}
			
			int GetTextLength(int line,int len)
			{
				stringUTF8_WithData <CharData> &strUtf8=Text[line].str;
				int w=0;
				for (int i=0;i<len;++i)
					w+=GetChWidth(strUtf8[i]);
				return w;
			}
			
			Point GetPos(const Point &pt)//get pos from pixel level to cord level
			{
				int i=0,w,LineY=EnsureInRange((pt.y-gPS.y)/EachLineHeight,0,Text.size()-1),
					x=pt.x-gPS.x-BorderWidth;
				const LineData &line=Text[LineY];
				const stringUTF8_WithData <CharData> &strUtf8=line.str;
				int strlen=StrLenWithoutSlashNR(strUtf8);
				while (i<strlen)
				{
					w=GetChWidth(strUtf8[i]);
					if (x<=w/2) return Point(i,LineY);
					++i;
					x-=w;
				}
				return Point(i,LineY);
			}
			
			void SetShowPosFromPos()//??
			{
				if (BorderWidth+EachLineHeight*Pos.y<-fa->GetrPS().y)
					fa2->SetViewPort(2,BorderWidth+EachLineHeight*Pos.y);
				else if (BorderWidth+EachLineHeight*(Pos.y+1)>-fa->GetrPS().y+fa2->GetrPS().h-fa2->GetButtomBarEnableState()*fa2->GetScrollBarWidth())
					fa2->SetViewPort(2,BorderWidth+EachLineHeight*(Pos.y+1)-fa2->GetrPS().h+fa2->GetButtomBarEnableState()*fa2->GetScrollBarWidth());
				int w=GetTextLength(Pos.y,Pos.x);
				if (BorderWidth+w<-fa->GetrPS().x+EachChWidth)
					fa2->SetViewPort(1,BorderWidth+w-EachChWidth);
				else if (BorderWidth+w>-fa->GetrPS().x+fa2->GetrPS().w-fa2->GetRightBarEnableState()*fa2->GetScrollBarWidth()-EachChWidth)
					fa2->SetViewPort(1,BorderWidth+w-fa2->GetrPS().w+fa2->GetRightBarEnableState()*fa2->GetScrollBarWidth()+EachChWidth);
			}
			
			void ResizeTextBoxHeight()
			{
				fa2->ResizeLL(0,EachLineHeight*Text.size()+2*BorderWidth);
				rPS.h=max(fa2->GetrPS().h,EachLineHeight*Text.size()+2*BorderWidth);
			}
			
			void ApplyTextLenChange()
			{
				int newMaxLen=MaxTextLength;
				while (!TextLengthChange.empty())
				{
					int len=TextLengthChange.top();
					TextLengthChange.pop();
					if (len>=0)
					{
						if (len>=TextLengthCount.size())
							TextLengthCount.resize(len+1,0);
						TextLengthCount[len]++;
						newMaxLen=max(newMaxLen,len);
					}
					else TextLengthCount[-len]--;
				}
				
				if (TextLengthCount[newMaxLen]==0)
					for (int i=newMaxLen-1;i>=0;--i)
						if (TextLengthCount[i]>0)
						{
							newMaxLen=i;
							break;
						}
				if (newMaxLen!=MaxTextLength)
				{
					MaxTextLength=newMaxLen;
					fa2->ResizeLL(MaxTextLength*EachChWidth+BorderWidth*2,0);
					rPS.w=max(fa2->GetrPS().w,MaxTextLength*EachChWidth+BorderWidth*2);
				}
			}
			
			void AddTextLenCnt(int len)
			{
				TextLengthChange.push(len);
			}
			
			void SubtractTextLenCnt(int len)
			{
				TextLengthChange.push(-len);
			}

			virtual void CalcPsEx()
			{
				if (PsEx!=NULL)
					PsEx->GetrPS(rPS);
				Posize lastPs=CoverLmt;
				fa2->ResizeLL(MaxTextLength*EachChWidth+BorderWidth*2,0);//??
				rPS.w=max(fa2->GetrPS().w,MaxTextLength*EachChWidth+BorderWidth*2);
				ResizeTextBoxHeight();
				if (fa!=NULL)
					gPS=rPS+fa->GetgPS();
				else gPS=rPS;
				Posize fa2gPS=fa2->GetgPS();//??
				CoverLmt=gPS&GetFaCoverLmt()&fa2gPS;
				if (!(lastPs==CoverLmt))
					Win->SetPresentArea(lastPs|CoverLmt);
				if (fa2->GetRightBarEnableState())
					fa2gPS.w-=fa2->GetScrollBarWidth();
				if (fa2->GetButtomBarEnableState())
					fa2gPS.h-=fa2->GetScrollBarWidth();
			}
			
//			int MaintainNearbyLines(int p,int cnt)
//			{
//				
//			}

			bool InSelectedRange(const Point &pt0,const Point &pt1,const Point &pt2)//[L,R)
			{
				if (pt1==pt2) return 0;
				if (pt1.y==pt2.y)
					return pt0.y==pt1.y&&InRange(pt0.x,min(pt1.x,pt2.x),max(pt1.x,pt2.x)-1);
				else
					if (InRange(pt0.y,min(pt1.y,pt2.y),max(pt1.y,pt2.y)))
						if (pt1.y<pt2.y)
							if (pt0.y==pt1.y)
								return pt0.x>=pt1.x;
							else if (pt0.y==pt2.y)
								return pt0.x<pt2.x;
							else return 1;
						else
							if (pt0.y==pt2.y)
								return pt0.x>=pt2.x;
							else if (pt0.y==pt1.y)
								return pt0.x<pt1.x;
							else return 1;
					else return 0;
			}
		
			void _Clear(bool isInnerChange)
			{
				if (!EnableEdit&&isInnerChange) return;
				Listener->BeforeClear(isInnerChange);
				Text.clear();
				editingText.clear();
				TextLengthCount.clear();
				MaxTextLength=0;
				Text.insert(0,LineData(stringUTF8("\r\n")));
				AddTextLenCnt(2);
				Pos=Pos_0={0,0};
				ApplyTextLenChange();
				ResizeTextBoxHeight();
				SetShowPosFromPos();
				Editing=0;
				Win->SetPresentArea(CoverLmt);
				Listener->AfterClear(isInnerChange); 
			}
			
			void _AddNewLine(int p,const stringUTF8 &strUtf8,bool isInnerChange)
			{
				if (strUtf8.length()==0||!EnableEdit&&isInnerChange) return;
				Listener->BeforeAddNewLine(p,strUtf8,isInnerChange);
				int i=0,j=0;
				while (i<strUtf8.length())
				{
					if (strUtf8[i]=="\r")
					{
						if (i<strUtf8.length()-1&&strUtf8[i+1]=="\n")
							++i;
						stringUTF8 substr=strUtf8.substrUTF8(j,i-j+1);
						Text.insert(p++,LineData(substr));
						AddTextLenCnt(GetStrLenOfEachWidth(substr));
						j=i+1;
					}
					else if (strUtf8[i]=="\n")
					{
						if (i<strUtf8.length()-1&&strUtf8[i+1]=="\r")
							++i;
						stringUTF8 substr=strUtf8.substrUTF8(j,i-j+1);
						Text.insert(p++,LineData(substr));
						AddTextLenCnt(GetStrLenOfEachWidth(substr));
						j=i+1;
					}
					++i;
				}
				if (j<strUtf8.length())
				{
					stringUTF8 substr=strUtf8.substrUTF8(j,strUtf8.length()-j)+stringUTF8("\r\n");
					Text.insert(p++,LineData(substr));
					AddTextLenCnt(GetStrLenOfEachWidth(substr));
				}
				Pos=Pos_0={StrLenWithoutSlashNR(Text[p-1].str),p-1};
				ApplyTextLenChange();
				ResizeTextBoxHeight();
				SetShowPosFromPos();
				Win->SetPresentArea(CoverLmt);
				Listener->AfterAddNewLine(p,strUtf8,isInnerChange);
			}
			
			void _AddText(Point pt,const stringUTF8 &strUtf8,bool isInnerChange)
			{
				if (strUtf8.length()==0||!EnableEdit&&isInnerChange) return;
				if (InRange(pt.y,0,Text.size()-1))
				{
					Listener->BeforeAddText(pt,strUtf8,isInnerChange);
					stringUTF8_WithData <CharData> &str2=Text[pt.y].str;
					stringUTF8_WithData <CharData> str3=str2.substrUTF8_WithData(pt.x,str2.length()-pt.x);
					SubtractTextLenCnt(GetStrLenOfEachWidth(str2));
					str2.erase(pt.x,str2.length()-pt.x);
					AddTextLenCnt(GetStrLenOfEachWidth(str2));
					
					int i=0,j=0;
					while (i<strUtf8.length())
					{
						if (strUtf8[i]=="\r")
						{
							if (i<strUtf8.length()-1&&strUtf8[i+1]=="\n")
								++i;
							if (j==0)
							{
								SubtractTextLenCnt(GetStrLenOfEachWidth(str2));
								str2.append(strUtf8.substrUTF8(j,i-j+1));
								AddTextLenCnt(GetStrLenOfEachWidth(str2));
								pt.y++;
								pt.x=0;
							}
							else
							{
								stringUTF8 substr=strUtf8.substrUTF8(j,i-j+1);
								Text.insert(pt.y++,LineData(substr));
								AddTextLenCnt(GetStrLenOfEachWidth(substr));
							}
							j=i+1;
						}
						else if (strUtf8[i]=="\n")
						{
							if (i<strUtf8.length()-1&&strUtf8[i+1]=="\r")
								++i;
							if (j==0)
							{
								SubtractTextLenCnt(GetStrLenOfEachWidth(str2));
								str2.append(strUtf8.substrUTF8(j,i-j+1));
								AddTextLenCnt(GetStrLenOfEachWidth(str2));
								pt.y++;
								pt.x=0;
							}
							else
							{
								stringUTF8 substr=strUtf8.substrUTF8(j,i-j+1);
								Text.insert(pt.y++,LineData(substr));
								AddTextLenCnt(GetStrLenOfEachWidth(substr));
							}
							j=i+1;
						}
						++i;
					}
					if (j<strUtf8.length())
						if (j==0)
						{
							SubtractTextLenCnt(GetStrLenOfEachWidth(str2));
							Pos=Pos_0={str2.length()+strUtf8.length(),pt.y};
							str2.append(strUtf8);
							str2.append(str3);
							AddTextLenCnt(GetStrLenOfEachWidth(str2));
						}
						else
						{
							stringUTF8_WithData <CharData>  str4=strUtf8.substrUTF8(j,strUtf8.length()-j);
							str4.append(str3);
							Text.insert(pt.y,str4);
							AddTextLenCnt(GetStrLenOfEachWidth(str4));
							Pos=Pos_0={StrLenWithoutSlashNR(Text[pt.y].str),pt.y};
							ResizeTextBoxHeight();
						}
					else
					{
						Text.insert(pt.y,str3);
						AddTextLenCnt(GetStrLenOfEachWidth(str3));
						Pos=Pos_0={StrLenWithoutSlashNR(Text[pt.y].str),pt.y};
						ResizeTextBoxHeight();
					}
					ApplyTextLenChange();
					SetShowPosFromPos();
					Win->SetPresentArea(CoverLmt);
					Listener->AfterAddText(pt,strUtf8,isInnerChange);
				}
				else _AddNewLine(EnsureInRange(pt.y,0,Text.size()),strUtf8,isInnerChange);
			}
			
			void _SetText(const stringUTF8 &strUtf8,bool isInnerChange)
			{
				if (!EnableEdit&&isInnerChange) return;
				Listener->BeforeSetText(strUtf8,isInnerChange);
				_Clear(isInnerChange);
				_AddText({0,0},strUtf8,isInnerChange);
				Listener->AfterSetText(strUtf8,isInnerChange);
			}
			
			void _AppendNewLine(const stringUTF8 &strUtf8,bool isInnerChange)
			{_AddNewLine(Text.size(),strUtf8,isInnerChange);}
			
			void _DeleteLine(int p,bool isInnerChange)
			{
				if (!EnableEdit&&isInnerChange) return;
				Listener->BeforeDeleteLine(p,isInnerChange);
				p=EnsureInRange(p,0,Text.size()-1);
				SubtractTextLenCnt(GetStrLenOfEachWidth(Text[p].str));
				Text.erase(p);
				Pos=p==0?Point(0,0):Point(p-1,StrLenWithoutSlashNR(Text[p-1].str));
				Pos=Pos_0=EnsurePosValid(Pos);
				ResizeTextBoxHeight();
				ApplyTextLenChange();
				SetShowPosFromPos();
				Listener->AfterDeleteLine(p,isInnerChange);
			}
			
			void _DeleteText(Point pt1,Point pt2,bool isInnerChange)
			{
				if (pt1==pt2||!EnableEdit&&isInnerChange) return;
				Listener->BeforeDeleteText(pt1,pt2,isInnerChange);
				if (pt1.y==pt2.y)
				{
					if (pt1.x>pt2.x)
						swap(pt1,pt2);
					Pos=pt1;
					stringUTF8_WithData <CharData>  &strUtf8=Text[pt1.y].str;
					SubtractTextLenCnt(GetStrLenOfEachWidth(strUtf8));
					strUtf8.erase(pt1.x,pt2.x-pt1.x);
					AddTextLenCnt(GetStrLenOfEachWidth(strUtf8));
				}
				else
				{
					if (pt1.y>pt2.y)
						swap(pt1,pt2);
					Pos=pt1;
					if (InRange(pt1.x,0,Text[pt1.y].str.length()-1))
					{
						stringUTF8_WithData <CharData>  &strUtf8=Text[pt1.y].str;
						SubtractTextLenCnt(GetStrLenOfEachWidth(strUtf8));
						strUtf8.erase(pt1.x,strUtf8.length()-pt1.x);
						AddTextLenCnt(GetStrLenOfEachWidth(strUtf8));
					}
					if (InRange(pt2.x,1,StrLenWithoutSlashNR(Text[pt2.y].str)))
					{
						stringUTF8_WithData <CharData> &strUtf8=Text[pt2.y].str;
						SubtractTextLenCnt(GetStrLenOfEachWidth(strUtf8));
						strUtf8.erase(0,pt2.x);
						AddTextLenCnt(GetStrLenOfEachWidth(strUtf8));
					}
					stringUTF8_WithData <CharData> &strUtf8=Text[pt1.y].str;
					SubtractTextLenCnt(GetStrLenOfEachWidth(strUtf8));
					strUtf8.append(Text[pt2.y].str);
					AddTextLenCnt(GetStrLenOfEachWidth(strUtf8));
					SubtractTextLenCnt(GetStrLenOfEachWidth(Text[pt2.y].str));
					Text.erase(pt2.y);
					for (int i=pt2.y-1;i>=pt1.y+1;--i)//It is necessary use --i that the index in SplayTree would change after erase
					{
						SubtractTextLenCnt(GetStrLenOfEachWidth(Text[i].str));
						Text.erase(i);
					}
					ResizeTextBoxHeight();
				}
				Pos=Pos_0=EnsurePosValid(Pos);
				ApplyTextLenChange();
				SetShowPosFromPos();
				Win->SetPresentArea(CoverLmt);//...
				Listener->AfterDeleteText(pt1,pt2,isInnerChange);
			}
			
			void _DeleteTextBack(int len,bool isInnerChange)
			{
				if (!EnableEdit&&isInnerChange) return;
				Point pt=Pos;
				if (len>Pos.x)
				{
					len-=Pos.x+1;
					pt.y--;
					if (pt.y<0)
						pt={0,0};
					else
					{
						while (len>StrLenWithoutSlashNR(Text[pt.y].str)&&pt.y>=1)
							len-=StrLenWithoutSlashNR(Text[pt.y--].str)+1;
						pt.x=max(0,StrLenWithoutSlashNR(Text[pt.y].str)-len);
					}
				}
				else pt.x-=len;
				_DeleteText(EnsurePosValid(pt),Pos,isInnerChange);
			}
			
			void _DeleteTextCursor(bool isInnerChange)
			{
				if (Pos==Pos_0)
					_DeleteTextBack(1,isInnerChange);
				else _DeleteText(Pos,Pos_0,isInnerChange);
			}
			
			void _AddTextCursor(const stringUTF8 &strUtf8,bool isInnerChange)
			{
				if (!(Pos==Pos_0))
					_DeleteTextCursor(isInnerChange);
				_AddText(Pos,strUtf8,isInnerChange);
			}

			void _SetAttributes(Point pt,Point pt2,const CharAttri &attributes)
			{
				if (pt==pt2) return;
				if (pt.y>pt2.y||pt.y==pt2.y&&pt.x>pt2.x)
					swap(pt,pt2);
				pt=EnsurePosValid(pt);
				pt2=EnsurePosValid(pt2);
				while (pt.y<pt2.y||pt.y==pt2.y&&pt.x<pt2.x)
					if (pt.y>=Text.size())
						break;
					else if (pt.x>=Text[pt.y].str.length())
						pt={0,pt.y+1};
					else
					{
						CharData &d=Text[pt.y].str(pt.x);
						d.attri=attributes;
						++pt.x;
					}
				Win->SetPresentArea(CoverLmt);//...
			}
		
		public:
			
			inline void Clear()
			{_Clear(0);}
			
			inline void AddNewLine(int p,const stringUTF8 &strUtf8)
			{_AddNewLine(p,strUtf8,0);}
			
			inline void AddText(const Point &pt,const stringUTF8 &strUtf8)
			{_AddText(pt,strUtf8,0);}
			
			inline void SetText(const stringUTF8 &strUtf8)
			{_SetText(strUtf8,0);}
			
			inline void AppendNewLine(const stringUTF8 &strUtf8)
			{_AppendNewLine(strUtf8,0);}
			
			inline void DeleteLine(int p)
			{_DeleteLine(p,0);}
			
			inline void DeleteText(const Point &pt1,const Point &pt2)
			{_DeleteText(pt1,pt2,0);}
			
			inline void DeleteTextBack(int len=1)
			{_DeleteTextBack(len,0);}
			
			inline void DeleteTextCursor()
			{_DeleteTextCursor(0);}
			
			inline void AddTextCursor(const stringUTF8 &strUtf8)
			{_AddTextCursor(strUtf8,0);}
			
			void MoveCursorPos(int delta)//?? 
			{
				if (Text.size()==0) return;
//				PUI_DD[3]<<"TextEditBox "<<IDName<<" MoveCursorPos Start "<<delta<<endl;
				if (delta>0)
					if (delta>StrLenWithoutSlashNR(Text[Pos.y].str)-Pos.x)
					{
						delta-=StrLenWithoutSlashNR(Text[Pos.y].str)-Pos.x+1;
						Pos.y++;
						if (Pos.y>=Text.size())
							Pos={StrLenWithoutSlashNR(Text[Text.size()-1].str),Text.size()-1};
						else
						{
							while (delta>StrLenWithoutSlashNR(Text[Pos.y].str)&&Pos.y<Text.size()-1)
								delta-=StrLenWithoutSlashNR(Text[Pos.y++].str)+1;
							Pos.x=min(delta,StrLenWithoutSlashNR(Text[Pos.y].str));
						}
					}
					else Pos.x+=delta;
				else
					if ((delta=-delta)>Pos.x)
					{
						delta-=Pos.x+1;
						Pos.y--;
						if (Pos.y<0)
							Pos={0,0};
						else
						{
							while (delta>StrLenWithoutSlashNR(Text[Pos.y].str)&&Pos.y>0)
								delta-=StrLenWithoutSlashNR(Text[Pos.y--].str)+1;
							Pos.x=max(0,StrLenWithoutSlashNR(Text[Pos.y].str)-delta);
						}
					}
					else Pos.x-=delta;
				Pos_0=Pos;
				SetShowPosFromPos();
				Win->SetPresentArea(CoverLmt);
//				PUI_DD[3]<<"TextEditBox "<<IDName<<" MoveCursorPos End "<<delta<<endl;
			}
			
			void MoveCursorPosUpDown(int downDelta)
			{
				downDelta=EnsureInRange(downDelta,-Pos.y,Text.size()-1-Pos.y);
				if (downDelta==0) return;
				int w=GetTextLength(Pos.y,Pos.x);
				Pos.y+=downDelta;
				Pos.x=0;
				stringUTF8_WithData <CharData> &strUtf8=Text[Pos.y].str;
				int strlen=StrLenWithoutSlashNR(strUtf8);
				for (int i=0;i<strlen&&w>0;Pos.x=++i)
					w-=GetChWidth(strUtf8[i]);
				Pos_0=Pos;
				SetShowPosFromPos();
				Win->SetPresentArea(CoverLmt);
			}
			
			void SetCursorPos(const Point &pt)
			{
				Pos=Pos_0=EnsurePosValid(pt);
				SetShowPosFromPos();
				Win->SetPresentArea(CoverLmt);//...
			}
			
			void SetCursorPos(const Point &pt1,const Point &pt2)
			{
				Pos=pt1;
				Pos_0=pt2;
				SetShowPosFromPos();
				Win->SetPresentArea(CoverLmt);
			}
			
			stringUTF8 GetSelectedTextUTF8()
			{
				stringUTF8 re;
				if (Pos==Pos_0)
					return re;
				Point pt1=Pos,pt2=Pos_0;
				if (pt1.y>pt2.y||pt1.y==pt2.y&&pt1.x>pt2.x)
					swap(pt1,pt2);
				if (pt1.y==pt2.y)
					re.append(Text[pt1.y].str.substrUTF8(pt1.x,pt2.x-pt1.x));
				else
				{
					re.append(Text[pt1.y].str.substrUTF8(pt1.x,Text[pt1.y].str.length()-pt1.x));
					for (int i=pt1.y+1;i<=pt2.y-1;++i)
						re.append(Text[i].str.StringUTF8());
					re.append(Text[pt2.y].str.substrUTF8(0,pt2.x));
				}
				return re;	
			}
			
			string GetSelectedText()
			{return GetSelectedTextUTF8().cppString();}
			
			stringUTF8 GetLineUTF8(int p)
			{return Text[EnsureInRange(p,0,Text.size()-1)].str.StringUTF8();}
			
			string GetLine(int p)
			{return Text[EnsureInRange(p,0,Text.size()-1)].str.cppString();}
			
			stringUTF8 GetAllTextUTF8()
			{
				stringUTF8 re;
				for (int i=0;i<Text.size();++i)
					re.append(Text[i].str.StringUTF8());
				return re;
			}
			
			string GetAllText()
			{return GetAllTextUTF8().cppString();}
			
			void SetEachCharSize(int w,int h)
			{
				EachChWidth=w;
				EachLineHeight=h;
				fa2->ResizeLL(MaxTextLength*EachChWidth+BorderWidth*2,EachLineHeight*Text.size()+2*BorderWidth);
				rPS.w=max(fa2->GetrPS().w,MaxTextLength*EachChWidth+BorderWidth*2);
				rPS.h=max(fa2->GetrPS().h,EachLineHeight*Text.size()+2*BorderWidth);
				SetShowPosFromPos();
			}
			
//			inline void SetFontSize(int size)
//			{
//				if (font.Size==size) return;
//				font.Size=size;
//				++NewestCode;
//				Win->SetPresentArea(CoverLmt);
//			}
//			
//			inline void SetFont(const PUI_Font &tar)
//			{
//				font=tar;
//				++NewestCode;
//				Win->SetPresentArea(CoverLmt);
//			}
			
			void SetFont(FontGroup *font)
			{
				if (font==nullptr)
					return;
				delete DefaultFont;
				DefaultFont=font;
			}
			
			inline int GetLinesCount()
			{return Text.size();}
			
			inline Point GetCursorPos()
			{return Pos;}
			
			inline void SetAttributes(const Point &pt1,const Point &pt2,const CharAttri &attributes)
			{_SetAttributes(pt1,pt2,attributes);}
			
			inline void SetLineAttributes(int p,const CharAttri &attributes)
			{
				p=EnsureInRange(p,0,Text.size()-1);
				SetAttributes({0,p},{Text[p].str.length(),p},attributes);
			}
			
			inline void SetAllAttributes(const CharAttri &attributes)
			{
				assert(Text.size()!=0);
				SetAttributes({0,0},{Text[Text.size()-1].str.length(),Text.size()-1},attributes);
			}
			
		protected:
			virtual void ReceiveKeyboardInput(const PUI_TextEvent *event)
			{
				switch (event->type)
				{
					case PUI_Event::Event_TextInputEvent:
					{
						if (!event->text.empty())
						{
							if (!(EditingPosL==EditingPosR)) 
							{
								Point oldPos=EditingPosL;//It's very ugly =_=|| 
								_DeleteText(EditingPosL,EditingPosR,1);
								SetCursorPos(oldPos);
								EditingPosL=EditingPosR=ZERO_POINT;
							}
							_AddTextCursor(stringUTF8(event->text),1);
						}
						Win->StopSolveEvent();
						break;
					}
					case PUI_Event::Event_TextEditEvent:
					{
//						PUI_DD[0]<<"TextEditBox: "<<IDName<<" Editing text: start "<<event.edit.start<<", len "<<event.edit.length<<endl;
						editingText=event->text;
						EditingTextCursorPos=event->cursor;
						Win->StopSolveEvent();
						
						Editing=editingText.length()!=0;
						if (Editing)
						{
							if (!(Pos==Pos_0))
								_DeleteTextCursor(1);
							if (!(EditingPosL==EditingPosR))
								_DeleteText(EditingPosL,EditingPosR,1);
							EditingPosL=Pos;
							_AddTextCursor(editingText,1);
							EditingPosR=Pos;
							MoveCursorPos(EditingTextCursorPos-editingText.length());
						}
						else if (!(EditingPosL==EditingPosR)) 
						{
							Point oldPos=EditingPosL;//It's very ugly =_=|| 
							_DeleteText(EditingPosL,EditingPosR,1);
							SetCursorPos(oldPos);
							EditingPosL=EditingPosR=ZERO_POINT;
						}
						SetCurrentIMEWindowPos(LastCursorPos,Posize(gPS.x,LastCursorPos.y,gPS.w,EachLineHeight),Win);
						break;
					}
				}
				Win->SetPresentArea(CoverLmt);
			}
			
			virtual void CheckEvent(const PUI_Event *event)
			{
				switch (event->type)
				{
					case PUI_Event::Event_KeyEvent:
						if (StateInput)
						{
							PUI_KeyEvent *keyevent=event->KeyEvent();
							if ((keyevent->keyType==keyevent->Key_Down||keyevent->keyType==keyevent->Key_Hold)&&!Editing)
							{
								bool flag=1;
								switch (keyevent->keyCode)
								{
									case SDLK_BACKSPACE: 	_DeleteTextCursor(1);		break;
									case SDLK_LEFT:			MoveCursorPos(-1);			break;
									case SDLK_RIGHT: 		MoveCursorPos(1);			break;
									case SDLK_UP: 			MoveCursorPosUpDown(-1);	break;
									case SDLK_DOWN: 		MoveCursorPosUpDown(1);		break;
									case SDLK_PAGEUP: 		MoveCursorPosUpDown(-fa2->GetrPS().h/EachLineHeight);	break;
									case SDLK_PAGEDOWN:		MoveCursorPosUpDown(fa2->GetrPS().h/EachLineHeight);	break;
									case SDLK_TAB: 			_AddTextCursor("\t",1);		break;
									case SDLK_RETURN:
										if (Pos.x>=StrLenWithoutSlashNR(Text[Pos.y].str))
											_AddNewLine(Pos.y+1,"\r\n",1);
										else 
										{
											Point pt=Pos;
											_AddNewLine(Pos.y+1,Text[Pos.y].str.substr(Pos.x,StrLenWithoutSlashNR(Text[Pos.y].str)-Pos.x),1);
											_DeleteText(pt,Point(StrLenWithoutSlashNR(Text[pt.y].str),pt.y),1);
											Pos_0=Pos={0,pt.y+1};
											Win->SetPresentArea(CoverLmt);
										}
										break;
									case SDLK_v:
										if (keyevent->mod&KMOD_CTRL)
										{
											char *s=SDL_GetClipboardText();
											stringUTF8 str=s;
											SDL_free(s);
											if (str.empty())
												break;
											_AddTextCursor(str,1);
										}
										else flag=0;
										break;
									case SDLK_c:
										if (keyevent->mod&KMOD_CTRL)
											if (!(Pos==Pos_0))
												SDL_SetClipboardText(GetSelectedText().c_str());
										break;
									case SDLK_x:
										if (keyevent->mod&KMOD_CTRL)
										{
											if (!(Pos==Pos_0))
											{
												SDL_SetClipboardText(GetSelectedText().c_str());
												_DeleteTextCursor(1);
											}
										}
										else flag=0;
										break;
									case SDLK_z:
										if (keyevent->mod&KMOD_CTRL)
										{
											PUI_DD[2]<<"TextEditBox: "<<IDName<<" ctrl z cannot use yet"<<endl;
											//<<...
										}
										else flag=0;
										break;
									
									case SDLK_a:
										if (keyevent->mod&KMOD_CTRL)
											SetCursorPos({StrLenWithoutSlashNR(Text[Text.size()-1].str),Text.size()-1},{0,0});
										else flag=0;
										break;
									
									case SDLK_ESCAPE:
										StateInput=0;
										TurnOnOffIMEWindow(0,Win);
										Editing=0;
										Win->SetPresentArea(CoverLmt);
										Win->SetKeyboardInputWg(NULL);
										editingText.clear();
										PUI_DD[0]<<"TextEditBox:"<<IDName<<" Stop input"<<endl;
										break;
									default:
										flag=0;
								}
								if (flag)
									Win->StopSolveEvent();
							}
							else Win->StopSolveEvent();
						}
						break;
//					case PUI_Event::Event_WheelEvent:
//						if (EnableScrollResize)
//							if (SDL_GetModState()&KMOD_CTRL)//??
//							{
//								double lambda=1+event->WheelEvent()->dy*0.1;
//								int h=EnsureInRange(EachLineHeight*lambda,10,160),w=h/2;
//								SetEachCharSize(w,h);
//								SetFontSize(h*0.75);
//								Win->StopSolveEvent();
//							}
//						break;
				}
			}
		
			virtual void CheckPos(const PUI_PosEvent *event,int mode)
			{
				if ((mode&PosMode_LoseFocus)&&Win->GetOccupyPosWg()!=this)
				{
					if (!Win->IsPosFocused()||!CoverLmt.In(event->pos)||mode&PosMode_ForceLoseFocus)
						if (StateInput)
						{
							if (event->posType==event->Pos_Down)
								if (event->button==event->Button_MainClick)
								{
									stat=Stat_NoFocus;
									StateInput=0;
									TurnOnOffIMEWindow(0,Win);
									Editing=0;
									Win->SetPresentArea(CoverLmt);
									Win->SetKeyboardInputWg(NULL);
									editingText.clear();
									RemoveNeedLoseFocus();
									PUI_DD[0]<<"TextEditBox:"<<IDName<<" Stop input"<<endl;
								}
						}
						else if (stat!=Stat_NoFocus)
						{
							stat=Stat_NoFocus;
							Win->SetPresentArea(CoverLmt);
							RemoveNeedLoseFocus();
						}
				}
				else if (mode&PosMode_Default)
					switch (event->posType)
					{
						case PUI_PosEvent::Pos_Down:
							PUI_DD[0]<<"TextEditBox "<<IDName<<" click"<<endl;
							if (!Editing)
								if (event->button==event->Button_MainClick)
								{
									stat=Stat_DownLeft;
									Pos=Pos_0=GetPos(event->pos);
									SetShowPosFromPos();
									Win->SetOccupyPosWg(this);
									SetNeedLoseFocus();
									PUI_DD[0]<<"TextEditBox "<<IDName<<": Start  input"<<endl;
								}
								else if (event->button==event->Button_SubClick)
								{
									stat=Stat_DownRight;
									SetNeedLoseFocus();
								}
							Win->StopSolvePosEvent();
							Win->SetPresentArea(CoverLmt);
							break;
						case PUI_PosEvent::Pos_Up:
							if (stat==Stat_DownLeft||stat==Stat_DownRight)
							{
								PUI_DD[0]<<"TextEditBox "<<IDName<<": up"<<endl;
								if (event->type==event->Event_TouchEvent)
									if (event->button==PUI_PosEvent::Button_MainClick)
										stat=Stat_DownLeft;
									else if (event->button==PUI_PosEvent::Button_SubClick)
										stat=Stat_DownRight;
								if (Win->GetOccupyPosWg()==this)
									Win->SetOccupyPosWg(NULL);
								if (stat==Stat_DownRight)
									Listener->RightClick();
//									if (RightClickFunc!=NULL)
//									{
//										RightClickFunc(RightClickFuncData,GetPos(event->pos),this);
//										PUI_DD[0]<<"TextEditBox "<<IDName<<": RightClickFunc"<<endl;
//									}
								stat=Stat_UpFocus;
								StateInput=1;
								TurnOnOffIMEWindow(1,Win);
								SDL_Rect rct=PosizeToSDLRect(gPS);
								SDL_SetTextInputRect(&rct);
								Win->SetKeyboardInputWg(this);
								Win->StopSolvePosEvent();
								Win->SetPresentArea(CoverLmt);
							}
							break;
						case PUI_PosEvent::Pos_Motion:
							if (stat==Stat_DownLeft||stat==Stat_DownRight)
							{
								if (!Editing)
								{
									Pos=GetPos(event->pos);
									SetShowPosFromPos();
									Win->SetPresentArea(CoverLmt);
								}
							}
							else if (stat==Stat_NoFocus)
							{
								stat=Stat_UpFocus;
								SetNeedLoseFocus();
								Win->SetPresentArea(CoverLmt);
							}
							Win->StopSolvePosEvent();
							break;
					}
			}
			
			PUI_Font GetCharFont(const string &c,const CharData &d)
			{
				return DefaultFont->Get(c.length()>=2,d.attri.bold,d.attri.italic);
			}
			
			SharedTexturePtr GetCharTex(const string &c,const CharData &d,int flags)
			{
//				RGBA &co=NowTextColor[chCol];
				SharedTexturePtr tex;
				if (c=="\r")
					if (ShowSpecialChar)
					{
//						if (timeCodeOfASCIItex['\r'][chCol]!=NewestCode||!ASCIItex['\r'][chCol])
//						{
//							ASCIItex['\r'][chCol]=SharedTexturePtr(Win->CreateRGBATextTexture("\\r",co,font));
//							timeCodeOfASCIItex['\r'][chCol]=NewestCode;
//						}
//						tex=ASCIItex['\r'][chCol]();
					}
					else DoNothing;
				else if (c=="\n") ;
				else if (c=="\t") ;
				else
				{
//					if (strUtf8(j).timeCode[chCol]!=NewestCode||!strUtf8(j).tex[chCol])
//						if (str.length()==1&&str[0]>0)
//						{
//							if (timeCodeOfASCIItex[str[0]][chCol]!=NewestCode||!ASCIItex[str[0]][chCol])
//							{
//								ASCIItex[str[0]][chCol]=SharedTexturePtr(Win->CreateRGBATextTexture(str.c_str(),co,font));
//								timeCodeOfASCIItex[str[0]][chCol]=NewestCode;
//							}
//							strUtf8(j).tex[chCol]=ASCIItex[str[0]][chCol];
//							strUtf8(j).timeCode[chCol]=NewestCode;
//						}
//						else
//						{
//							strUtf8(j).tex[chCol]=SharedTexturePtr(Win->CreateRGBATextTexture(str.c_str(),co,font));
//							strUtf8(j).timeCode[chCol]=NewestCode;
//						}
//					tex=strUtf8(j).tex[chCol]();
					RGBA co=d.attri.co;
					if (co==RGBA_NONE)
						switch (flags)
						{
							case 0:	co=TextColor[0];	break;
							case 1:	co=TextColor[1];	break;
							case 2:	co=TextColor[2];	break;
							default:co=RGBA_BLACK;		break;
						}
					tex=SharedTexturePtr(Win->CreateRGBATextTexture(c.c_str(),ThemeColor(co),GetCharFont(c,d)));
				}
				return tex;
			}
			
			virtual void Show(Posize &lmt)
			{
//				Point EditingPosL,EditingPosR;
//				
//				if (StateInput&&Editing)
//				{
//					EditingPosL=Pos;
//					AddText(Pos,editingText);
//					EditingPosR=Pos;
//					MoveCursorPos(EditingTextCursorPos-editingText.length());
//				}
				
				Win->RenderFillRect(lmt&gPS,ThemeColor(BackgroundColor[0]));
				if (ShowBorder)
					Win->RenderDrawRectWithLimit(fa2->GetgPS(),ThemeColor(BorderColor[EnsureInRange((int)stat,0,2)]),lmt);
				
				int ForL=-fa->GetrPS().y/EachLineHeight,
					ForR=ForL+fa2->GetrPS().h/EachLineHeight+1;
				ForL=EnsureInRange(ForL,0,Text.size()-1);
				ForR=EnsureInRange(ForR,0,Text.size()-1);
				
				Posize CharPs=Posize(gPS.x+BorderWidth,gPS.y+BorderWidth+ForL*EachLineHeight,EachChWidth,EachLineHeight);
				for (int i=ForL,flag=0;i<=ForR;++i)
				{
					stringUTF8_WithData <CharData> &strUtf8=Text[i].str;
					for (int j=0;j<strUtf8.length();++j)
					{
						const string &str=strUtf8[j];
						CharPs.w=GetChWidth(str);
						if ((CharPs&lmt).Size()>0)
						{
							if (Editing)
							{
								if (Pos==Point(j,i)&&StateInput)
									Win->RenderFillRect(Posize(CharPs.x-1,CharPs.y,2,CharPs.h)&lmt,ThemeColor(BackgroundColor[1+EnsureInRange((int)stat,0,2)]));
								flag=InSelectedRange({j,i},EditingPosL,EditingPosR);
							}
							else if (Pos==Pos_0)
							{
								if (Pos==Point(j,i)&&StateInput)
									Win->RenderFillRect(Posize(LastCursorPos.x=CharPs.x-1,LastCursorPos.y=CharPs.y,2,CharPs.h)&lmt,ThemeColor(BackgroundColor[1+EnsureInRange((int)stat,0,2)]));
							}
							else flag=InSelectedRange({j,i},Pos,Pos_0);
							
							if (!Editing&&flag)
								Win->RenderFillRect(CharPs&lmt,ThemeColor(BackgroundColor[1+EnsureInRange((int)stat,0,2)]));
							
							int chCol=flag?(Editing?2:1):0;
//							if (chCol==2)
//								Win->RenderDrawText(str,CharPs,lmt,0,ThemeColor(TextColor[2]),font);
//							else
//							{
//							}
							
							SharedTexturePtr tex=GetCharTex(str,strUtf8(j),chCol);
							if (!!tex)
								Win->RenderCopyWithLmt_Centre(tex(),CharPs,lmt);
						}
						CharPs.x+=CharPs.w;
					}
					
					if (Editing)
					{
						if (Pos==Point(strUtf8.length(),i))
							Win->RenderFillRect(Posize(CharPs.x-1,CharPs.y,2,CharPs.h)&lmt,ThemeColor(BackgroundColor[1+EnsureInRange((int)stat,0,2)]));
					}
					else if (Pos==Pos_0)
						if (Pos==Point(strUtf8.length(),i))
							Win->RenderFillRect(Posize(CharPs.x-1,CharPs.y,2,CharPs.h)&lmt,ThemeColor(BackgroundColor[1+EnsureInRange((int)stat,0,2)]));
					CharPs.x=gPS.x+BorderWidth;
					CharPs.y+=EachLineHeight;
				}
				
//				if (StateInput&&Editing)
//				{
//					MoveCursorPos(editingText.length()-EditingTextCursorPos+editingText.length());
//					DeleteTextBack(editingText.length());
//				}

				Win->Debug_DisplayBorder(gPS);
			}
		
		public:
			inline void SetTextColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					TextColor[p]=!co?(p==2?ThemeColorM[6]:ThemeColorMT[p]):co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"TextEditBox "<<IDName<<": SetTextColor p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
			inline void SetBackgroundColor(int p,const RGBA &co)
			{
				if (InRange(p,0,3))
				{
					BackgroundColor[p]=!co?(p==0?ThemeColorBG[0]:ThemeColorM[p*2-1]):co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"TextEditBox "<<IDName<<": SetBackgroundColor p "<<p<<" is not in Range[0,3]"<<endl;
			}
			
			inline void SetBorderColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					BorderColor[p]=!co?ThemeColorM[p*2+1]:co;
					Win->SetPresentArea(CoverLmt);
				}
				else PUI_DD[1]<<"TextEditBox "<<IDName<<": SetBorderColor p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
			inline void EnableShowBorder(bool en)
			{
				if (ShowBorder==en) return;
				ShowBorder=en;
				Win->SetPresentArea(CoverLmt);
			}
			
			void SetBorderWidth(int w)
			{
				BorderWidth=w;
				fa2->ResizeLL(MaxTextLength*EachChWidth+BorderWidth*2,EachLineHeight*Text.size()+2*BorderWidth);
				rPS.w=max(fa2->GetrPS().w,MaxTextLength*EachChWidth+BorderWidth*2);
				rPS.h=max(fa2->GetrPS().h,EachLineHeight*Text.size()+2*BorderWidth);
				SetShowPosFromPos();
			}
			
			inline bool GetShowSpecialChar() const
			{return ShowSpecialChar;}
			
			inline void SetShowSpecialChar(bool en)
			{
				if (en==ShowSpecialChar) return;
				ShowSpecialChar=en;
				Win->SetPresentArea(CoverLmt);
			}
			
//			inline void SetEachChangeFunc(void (*_eachchangefunc)(T&,BetterTextEditBox*,bool))
//			{EachChangeFunc=_eachchangefunc;}
//			
//			inline void SetEachChangeFuncData(const T &eachchangefuncdata)
//			{EachChangeFuncdata=eachchangefuncdata;}
//			
//			inline void SetRightClickFunc(void (*_rightClickFunc)(T&,const Point&,BetterTextEditBox*))
//			{RightClickFunc=_rightClickFunc;}
//			
//			inline void SetRightClickFuncData(const T &rightclickfuncdata)
//			{RightClickFuncData=rightclickfuncdata;}

			inline void SetEditListener(EditListener *lis)
			{
				if (lis==nullptr)
					return;
				delete Listener;
				Listener=lis;
				Listener->SetTEB(this);
			}
			
			inline void SetEnableScrollResize(bool enable)
			{EnableScrollResize=enable;}
			
			inline void SetEnableEdit(bool enable)
			{EnableEdit=enable;}
			
			void AddPsEx(PosizeEX *psex)
			{fa2->AddPsEx(psex);}
			
			virtual void SetrPS(const Posize &ps)
			{fa2->SetrPS(ps);}
			
			inline LargeLayerWithScrollBar* GetFa2()
			{return fa2;}
			
			virtual ~BetterTextEditBox()
			{
				DeleteToNULL(Listener);
				DeleteToNULL(DefaultFont);
			}
			
			BetterTextEditBox(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex)
			:Widgets(_ID,WidgetType_BetterTextEditBox)
			{
				Listener=new EditListener();
				Listener->SetTEB(this);
				fa2=new LargeLayerWithScrollBar(0,_fa,psex,ZERO_POSIZE);
				SetFa(fa2->LargeArea());
				rPS=ZERO_POSIZE;
				TextLengthCount.resize(10,0);
				_Clear(1);
				InitDefualtColor();
				InitDefaultFont(16);
			}
			
			BetterTextEditBox(const WidgetID &_ID,Widgets *_fa,const Posize &_rps)
			:Widgets(_ID,WidgetType_BetterTextEditBox)
			{
				Listener=new EditListener();
				Listener->SetTEB(this);
				fa2=new LargeLayerWithScrollBar(0,_fa,_rps,ZERO_POSIZE);
				SetFa(fa2->LargeArea());
				rPS={0,0,_rps.w,_rps.h};
				TextLengthCount.resize(10,0);
				_Clear(1);
				InitDefualtColor();
				InitDefaultFont(16);
			}
	};
	
	Widgets::WidgetType WidgetType_ColorSelectBox=Widgets::GetNewWidgetTypeID("ColorSelectBox");
	class ColorSelectBox:public Widgets//24.4.27
	{
		public:
			enum//Do not support alpha blend
			{
				Mode_Hue=0
			};
			
			class UpdateListener
			{
				public:
					virtual void Update(ColorSelectBox *This,const RGBA &co)=0;
			};
			
			class UpdateListenerV:public UpdateListener
			{
				protected:
					void (*func)(ColorSelectBox*,const RGBA&)=nullptr;
					
				public:
					
					virtual void Update(ColorSelectBox *This,const RGBA &co)
					{
						if (func!=nullptr)
							func(This,co);
					}
					
					UpdateListenerV(void (*_func)(ColorSelectBox*,const RGBA&)):func(_func) {}
			};
			
			template <class T> class UpdateListenerT:public UpdateListener
			{
				protected:
					void (*func)(T&,ColorSelectBox*,const RGBA&)=nullptr;
					T data;
				
				public:
					
					virtual void Update(ColorSelectBox *This,const RGBA &co)
					{
						if (func!=nullptr)
							func(data,This,co);
					}
					
					UpdateListenerT(void (*_func)(T&,ColorSelectBox*,const RGBA&),const T &_data)
					:func(_func),data(_data) {}
			};
		
		protected:
			enum
			{
				Stat_None=0,
				Stat_SlidingD1=1,
				Stat_SlidingD2=2
			}stat=Stat_None;
			int Mode=0;
			Triplet <double,double,double> Value{0,0,0};
			int D1Width=30,
				BorderWidth=3;
			RGBA BorderColor{ThemeColorM[0]},
				 CoordColor{ThemeColorM[2]};
			SharedTexturePtr D1Tex,
							 D2Tex;
			Triplet <double,double,double> LastValue{0,0,0};
			Posize LastD1Rps=ZERO_POSIZE,LastD2Rps=ZERO_POSIZE;
			UpdateListener *Listener=nullptr;
			
			bool D1Vertical()
			{return rPS.w>=rPS.h;}
			
			int D2Width()
			{return min(rPS.w,rPS.h);}
			
			Posize D1BorderRPS()
			{
				if (D1Vertical())
					return Posize(rPS.w-D1Width,0,D1Width,rPS.h);
				else return Posize(0,rPS.h-D1Width,rPS.w,D1Width);
			}
			
			Posize D2BorderRPS()
			{return Posize(0,0,D2Width(),D2Width());}
			
			Posize D1RPS()//Not include border
			{return D1BorderRPS().Shrink(BorderWidth);}
			
			Posize D2RPS()
			{return D2BorderRPS().Shrink(BorderWidth);}
			
			Triplet <double,double,double> ColorToCoord(const RGBA &co)
			{
				if (Mode==Mode_Hue)
				{
					HSVA hsv(co);
					return {hsv.h,hsv.s,hsv.v};
				}
				else return {0,0,0};
			}
			
			RGBA CoordToColor(const Triplet <double,double,double> &coord)
			{
				if (Mode==Mode_Hue)
					return RGBA(HSVA(coord.a,coord.b,coord.c));
				else return RGBA_NONE;
			}
			
			void UdpateD2Point(const Posize &gps,const Point &pt)
			{
				double x=(pt.x-gps.x)*1.0/gps.w,
					   y=(pt.y-gps.y)*1.0/gps.h;
				Value.b=EnsureInRange(x,0,1);
				Value.c=1-EnsureInRange(y,0,1);
				if (Listener)
					Listener->Update(this,CoordToColor(Value));
			}
			
			void UpdateD1Pos(const Posize &gps,const Point &pt)
			{
				if (D1Vertical())
					Value.a=1-EnsureInRange((pt.y-gps.y)*1.0/gps.h,0,1);
				else Value.a=EnsureInRange((pt.x-gps.x)*1.0/gps.w,0,1);
				if (Listener)
					Listener->Update(this,CoordToColor(Value));
			}
			
			void UpdateTexture(bool forceUdpate)
			{
				Posize d1rps=D1RPS(),
					   d2rps=D2RPS();
				if (Value.a==LastValue.a&&LastD1Rps==d1rps&&LastD2Rps==d2rps&&!forceUdpate)
					return;
				if (d1rps.w<=3||d1rps.h<=3||d2rps.w<=3||d2rps.h<=3)
					return;
				SDL_Surface *D1Sur=SDL_CreateRGBSurfaceWithFormat(0,d1rps.w,d1rps.h,32,SDL_PIXELFORMAT_RGBA32),
							*D2Sur=SDL_CreateRGBSurfaceWithFormat(0,d2rps.w,d2rps.h,32,SDL_PIXELFORMAT_RGBA32);
				SDL_SetSurfaceBlendMode(D1Sur,SDL_BLENDMODE_BLEND);
				SDL_SetSurfaceBlendMode(D2Sur,SDL_BLENDMODE_BLEND);
				
				if (D1Vertical())
					for (int i=0;i<D1Sur->h;++i)
					{
						RGBA co=CoordToColor({1.0-(D1Sur->h==0?0:i*1.0/(D1Sur->h-1)),1,1});
						for (int j=0;j<D1Sur->w;++j)
							*((Uint32*)D1Sur->pixels+i*D1Sur->pitch/4+j)=SDL_MapRGBA(D1Sur->format,co.r,co.g,co.b,co.a);
					}
				else
					for (int j=0;j<D1Sur->w;++j)
					{
						RGBA co=CoordToColor({D1Sur->w==0?0:j*1.0/(D1Sur->w-1),1,1});
						for (int i=0;i<D1Sur->h;++i)
							*((Uint32*)D1Sur->pixels+i*D1Sur->pitch/4+j)=SDL_MapRGBA(D1Sur->format,co.r,co.g,co.b,co.a);
					}
				for (int i=0;i<D2Sur->h;++i)
					for (int j=0;j<D2Sur->w;++j)
					{
						RGBA co=CoordToColor({Value.a,(D2Sur->w==0?0:j*1.0/(D2Sur->w-1)),1.0-(D2Sur->h==0?0:i*1.0/(D2Sur->h-1))});
						*((Uint32*)D2Sur->pixels+i*D2Sur->pitch/4+j)=SDL_MapRGBA(D2Sur->format,co.r,co.g,co.b,co.a);
					}
				
				D1Tex=SharedTexturePtr(Win->CreateTextureFromSurfaceAndDelete(D1Sur));
				D2Tex=SharedTexturePtr(Win->CreateTextureFromSurfaceAndDelete(D2Sur));
				LastValue=Value;
				LastD1Rps=d1rps;
				LastD2Rps=d2rps;
			}
			
			virtual void CheckPos(const PUI_PosEvent *event,int mode)
			{
				if (mode&(PosMode_Default|PosMode_Occupy))
					switch (event->posType)
					{
						case PUI_PosEvent::Pos_Down:
							if (event->button==PUI_PosEvent::Button_MainClick)
							{
								if (auto d1gps=D1RPS()+gPS;d1gps.In(event->pos))
								{
									stat=Stat_SlidingD1;
									UpdateD1Pos(d1gps,event->pos);
								}
								else if (auto d2gps=D2RPS()+gPS;d2gps.In(event->pos))
								{
									stat=Stat_SlidingD2;
									UdpateD2Point(d2gps,event->pos);
								}
								Win->SetOccupyPosWg(this);
								Win->StopSolvePosEvent();
								Win->SetPresentArea(CoverLmt);
							}
							break;
						case PUI_PosEvent::Pos_Up:
							if (stat!=Stat_None)
							{
								if (stat==Stat_SlidingD1)
									UpdateD1Pos(D1RPS()+gPS,event->pos);
								else if (stat==Stat_SlidingD2)
									UdpateD2Point(D2RPS()+gPS,event->pos);
								stat=Stat_None;
								Win->SetOccupyPosWg(NULL);
								Win->StopSolvePosEvent();
								Win->SetPresentArea(CoverLmt);
							}
							break;
						case PUI_PosEvent::Pos_Motion:
							if (stat!=Stat_None)
							{
								if (stat==Stat_SlidingD1)
									UpdateD1Pos(D1RPS()+gPS,event->pos);
								else if (stat==Stat_SlidingD2)
									UdpateD2Point(D2RPS()+gPS,event->pos);
								Win->SetPresentArea(CoverLmt);
							}
							Win->StopSolvePosEvent();
							break;
					}
			}
			
			virtual void Show(Posize &lmt)
			{
				UpdateTexture(false);
				
				Posize d2gps=D2RPS()+gPS,
					   d1gps=D1RPS()+gPS;
				Win->RenderCopyWithLmt(D2Tex(),d2gps,lmt);
				Win->RenderCopyWithLmt(D1Tex(),d1gps,lmt);
				for (int i=0;i<BorderWidth;++i)
				{
					Win->RenderDrawRectWithLimit((D2BorderRPS()+gPS).Shrink(i),ThemeColor(BorderColor),lmt);
					Win->RenderDrawRectWithLimit((D1BorderRPS()+gPS).Shrink(i),ThemeColor(BorderColor),lmt);
				}
				
				Point pt=d2gps.GetLU();
				pt.x+=Value.b*d2gps.w;
				pt.y+=(1.0-Value.c)*d2gps.h;
				Win->RenderFillRect(Posize(d2gps.x,pt.y-1,d2gps.w,3)&lmt,RGBA_WHITE);
				Win->RenderFillRect(Posize(pt.x-1,d2gps.y,3,d2gps.h)&lmt,RGBA_WHITE);
				Win->RenderFillRect(Posize(d2gps.x,pt.y,d2gps.w,1)&lmt,ThemeColor(CoordColor));
				Win->RenderFillRect(Posize(pt.x,d2gps.y,1,d2gps.h)&lmt,ThemeColor(CoordColor));
				if (D1Vertical())
				{
					int pos=d1gps.y+d1gps.h*(1-Value.a);
					Win->RenderFillRect(Posize(d1gps.x,pos-1,d1gps.w,3)&lmt,RGBA_WHITE);
					Win->RenderFillRect(Posize(d1gps.x,pos,d1gps.w,1)&lmt,ThemeColor(CoordColor));
				}
				else
				{
					int pos=d1gps.x+d1gps.w*Value.a;
					Win->RenderFillRect(Posize(pos-1,d1gps.y,3,d1gps.h)&lmt,RGBA_WHITE);
					Win->RenderFillRect(Posize(pos,d1gps.y,1,d1gps.h)&lmt,ThemeColor(CoordColor));
				}
				
				Win->Debug_DisplayBorder(gPS);
			}
			
		public:
			
			RGBA GetColor()
			{return CoordToColor(Value);}
			
			void SetColor(RGBA co,bool triggerfunc=true)
			{
				Value=ColorToCoord(co);
				Win->SetPresentArea(CoverLmt);
				if (triggerfunc&&Listener)
					Listener->Update(this,co);
			}
			
			void SetListener(UpdateListener *lis)
			{
				if (Listener)
					delete Listener;
				Listener=lis;
			}
			
			~ColorSelectBox()
			{
				if (Listener)
					delete Listener;
			}
			
			ColorSelectBox(const WidgetID &_ID,Widgets *_fa,PosizeEX *psex,int _mode=Mode_Hue)
			:Widgets(_ID,WidgetType_ColorSelectBox,_fa,psex),Mode(_mode) {}
			
			ColorSelectBox(const WidgetID &_ID,Widgets *_fa,const Posize &_rps,int _mode=Mode_Hue)
			:Widgets(_ID,WidgetType_ColorSelectBox,_fa,_rps),Mode(_mode) {}
	};
	
	Widgets::WidgetType WidgetType_DowningButton=Widgets::GetNewWidgetTypeID("DowningButton");
	template <class T> class DowningButton:public Button <T>
	{
		protected:
			Uint32 TimerInterval=50;
			SDL_TimerID IntervalTimerID=0;
			
			void CallFunc()
			{
				Point mousePt;
				if (IntervalTimerID!=0&&!(SDL_GetGlobalMouseState(&mousePt.x,&mousePt.y)&SDL_BUTTON(SDL_BUTTON_LEFT)))
					SetRepeatDownEvent(false);
				if (IntervalTimerID!=0)//??
					if (this->func!=nullptr)
						this->func(this->funcData);
			}

			static Uint32 TimerFunc(Uint32 itv,void *_this)
			{
				auto *This=(DowningButton <T>*)_this;
				PUI_SendFunctionEvent<WidgetPtr>([](WidgetPtr &wg)
				{
					if (wg.Valid())
					{
						auto *This=(DowningButton <T>*)wg.Target();
						This->CallFunc();
					}
				},This->This());
				return itv;
			}
			
			void SetRepeatDownEvent(bool on)
			{
				if ((IntervalTimerID!=0)==on)
					return;
				if (on)
				{
					PUI_DD[0]<<"DowningButton "<<this->IDName<<" timer on."<<endl;
					IntervalTimerID=SDL_AddTimer(TimerInterval,TimerFunc,this);
				}
				else
				{
					PUI_DD[0]<<"DowningButton "<<this->IDName<<" timer off."<<endl;
					SDL_RemoveTimer(IntervalTimerID);
					IntervalTimerID=0;
				}
			}
			
			virtual void TriggerButtonFunction(int buttonType)
			{
				CallFunc();
				SetRepeatDownEvent(buttonType==this->Button_Down);
			}
			
		public:
			
			void SetTimerInterval(Uint32 itv)
			{
				TimerInterval=itv;
			}
			
			virtual ~DowningButton()
			{
				SetRepeatDownEvent(0);
			}
			
			template <typename ...Ts> DowningButton(const Widgets::WidgetID &_ID,const Ts &...args)
			:Button<T>::Button(_ID,WidgetType_DowningButton,args...)
			{
				this->ThisButtonSolvePressedClick=true;
			}
	};
	
};

#endif
