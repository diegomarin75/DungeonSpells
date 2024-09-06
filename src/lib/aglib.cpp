//agllib.cpp: Audio & Graphics library

//See symbols on library: 
//Linux: nm -D iolib.so
//Windws: objdump -t iolib.dll

//Application includes
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "bas/basedefs.hpp"
#include "bas/array.hpp"
#include "bas/stack.hpp"
#include "bas/buffer.hpp"
#include "bas/string.hpp"
#include "lib/fonts.def"
#include "lib/libman.hpp"
#include "lib/aglib.hpp"

//Library version
#define AGL_VERSION "1.0"

//Font texture width in chars
#define FONT_TEXTURE_ROW 16

//Font table
#define AGL_SYSTEM_FONTS 7
struct AglSystemFontTable{
  const char *CodeName;
  const char *FullName;
  cint Size;
  bool MonoSpace;
  cint _Width;
  cint _Height;
  cint MinChar;
  cint MaxChar;
  const FontIndex *Index;
  const unsigned char *Data;
};
#define FONT_DEF(x) (AglSystemFontTable){_FntCodeName##x,_FntFullName##x,_FntSize##x,_FntMonoSpace##x,_FntWidth##x,_FntHeight##x,_FntMinChar##x,_FntMaxChar##x,_FntIndex##x,_FntData##x}
AglSystemFontTable _SystemFont[AGL_SYSTEM_FONTS]={
  FONT_DEF(Mono10),
  FONT_DEF(Mono12),
  FONT_DEF(Mono14),
  FONT_DEF(Mono16),
  FONT_DEF(Mono18),
  FONT_DEF(Vga),
  FONT_DEF(Casio)
};

//Display handler data
struct AglDisp{
  cint ProcessId;
  bool Used;
  SDL_Window *Win;
  SDL_Renderer *Ren;     
  SDL_Surface *Sfc;
};
Array<AglDisp> _Disp;

//Texture handler data
struct AglTxtr{
  cint ProcessId;
  bool Used;
  SDL_Texture *Txtr;
  cint Width;
  cint Height;
  bool ExMode;
  double Angle;
  cint Rotx;
  cint Roty;
  SDL_RendererFlip Flip;
};
Array<AglTxtr> _Txtr;

//Font handler data
struct AglFont{
  cint ProcessId;
  AglSystemFont SysFont;
  cint ScaleX;
  cint ScaleY;
  cint Width;
  cint Height;
  cint ForeColor;
  cint BackColor;
  SDL_Texture *Tex;
};
Array<AglFont> _Font;

//Global private variables
bool _Init=false;
bool _DeviceStarted[AGL_DEVICES]={false,false,false,false};

//Internal state (depends on current process)
struct AglState{
  cint ProcessId;
  AglErrorCode ErrorCode;
  String ErrorInfo;
  String ErrorText;
  String LastError;
  String Version;
  bool Locked;
  cint CurrDispHnd;
  cint CurrFont;
  cint FontAdvance;
  cint FontOffsetX;
  cint FontOffsetY;
  cint FontForeColor;
  cint FontBackColor;
  bool FontRegeneration;
  SDL_Window *Win;
  SDL_Renderer *Ren;
  SDL_Surface *Sfc;
};
Array<AglState> _State;

//Internal state variables (all available in _State table but replicated for performance, updated on process change)
cint _ProcessId;
AglErrorCode _ErrorCode;
String _ErrorInfo;
String _ErrorText;
String _LastError;
String _Version;
bool _Locked;
cint _CurrDispHnd;
cint _CurrFont;
cint _FontAdvance;
cint _FontOffsetX;
cint _FontOffsetY;
cint _FontForeColor;
cint _FontBackColor;
bool _FontRegeneration;
SDL_Window *_Win;
SDL_Renderer *_Ren;
SDL_Surface *_Sfc;

//Module private functions
bool _InitDevice(AglDevice Dev);
void _StopDevice(AglDevice Dev);
void _CloseAll();
bool _InternalProcessChange(cint ProcessId);
void _SetError(AglErrorCode Code);
void _SetError(AglErrorCode Code,const String& Info);
String _GetSdlCompiledVersion();
String _GetSdlLinkedVersion();
cint _SdlKeyModifiersToAgl(cint _SdlKeyModifiers);
AglScanCode _SdlScanCodeToAgl(SDL_Scancode SdlScanCode);
cint _Utf8SequenceToInt(char * Utf8);
void _SetError(AglErrorCode Code);
void _SetError(AglErrorCode Code,const String& Info);
bool _NewDispHandler(cint *DispHnd);
void _FreeDispHandler(cint DispHnd);
bool _CheckDispHndAccess(cint DispHnd);
bool _NewTxtrHandler(cint *TxtrHnd);
void _FreeTxtrHandler(cint TxtrHnd);
bool _CheckTxtrHndAccess(cint TxtrHnd);
cint _GenerateFont(AglSystemFont SysFont,cint ScaleX,cint ScaleY);
void _PrintChar(cint x,cint y,cint Chr);

//Export library functions
FDECLBEGIN(1);
//Function prototypes device control & info
FDECLRS2(bool,Init,cint,char **);
FDECLRS0(char *,GetVersionString);
FDECLRS0(AglErrorCode,Error);
FDECLRS1(char *,ErrorText,AglErrorCode);
FDECLRS0(char *,LastError);
//Function prototypes for praphics
FDECLRS7(bool,DisplayOpen,const char *,cint,cint,cint,cint,cint,cint *);
FDECLRS1(bool,DisplayClose,cint);
FDECLRS2(bool,GetDisplayInfo,cint,AglDispInfo *);
FDECLRS1(bool,SetCurrentDisplay,cint);
FDECLRS0(cint,GetCurrentDisplay);
FDECLVO2(SetLogicalSize,cint,cint);
FDECLVO4(SetViewPort,cint,cint,cint,cint);
FDECLVO4(SetClipArea,cint,cint,cint,cint);
FDECLVO0(DisplayShow);
FDECLVO0(Clear);
FDECLVO4(SetColor,char,char,char,char);
FDECLVO2(SetPoint,cint,cint);
FDECLVO4(DrawLine,cint,cint,cint,cint);
FDECLVO4(DrawRectangle,cint,cint,cint,cint);
FDECLVO4(DrawFillRectangle,cint,cint,cint,cint);
FDECLVO3(DrawCircle,cint,cint,cint);
FDECLVO3(DrawFillCircle,cint,cint,cint);
FDECLVO3(SetFont,AglSystemFont,cint,cint);
FDECLVO4(SetFontForeColor,char,char,char,char);
FDECLVO4(SetFontBackColor,char,char,char,char);
FDECLRS0(AglSystemFont,GetCurrentFont);
FDECLRS0(cint,GetCurrentFontScaleX);
FDECLRS0(cint,GetCurrentFontScaleY);
FDECLRS2(bool,GetFontInfo,AglSystemFont,AglFontInfo *);
FDECLRS1(cint,GetFontLines,AglSystemFont);
FDECLRS1(cint,GetFontColumns,AglSystemFont);
FDECLVO2(SetFontOffset,cint,cint);
FDECLVO3(PrintChar,cint,cint,cint);
FDECLVO3(Print,cint,cint,const char *);
FDECLVO3(PrintCharXY,cint,cint,cint);
FDECLVO3(PrintXY,cint,cint,const char *);
FDECLRS2(bool,LoadTexture,const char *,cint *);
FDECLRS1(bool,FreeTexture,cint);
FDECLVO3(GetTextureSize,cint,cint *,cint *);
FDECLVO4(SetTextureRotation,cint,double,cint,cint);
FDECLVO2(SetTextureFlipMode,cint,AglFlipMode);
FDECLVO2(SetTextureBlendMode,cint,AglBlendMode);
FDECLVO4(SetTextureColorModulation,cint,char,char,char);
FDECLVO2(SetTextureAlphaModulation,cint,char);
FDECLVO3(RenderTexture,cint,cint,cint);
FDECLVO5(RenderTextureScaled,cint,cint,cint,cint,cint);
FDECLVO7(RenderTexturePart,cint,cint,cint,cint,cint,cint,cint);
FDECLVO9(RenderTexturePartScaled,cint,cint,cint,cint,cint,cint,cint,cint,cint);
//Function prototypes for event handling
FDECLVO1(WaitEvent,cint);
FDECLRS0(bool,HasEvents);
FDECLVO1(GetEvent,AglEventInfo *);
FDECLEND;

//Get SDL compiled version
String _GetSdlCompiledVersion(){ 
  SDL_version SdlVers; 
  SDL_version ImgVers; 
  SDL_VERSION(&SdlVers);    
  SDL_IMAGE_VERSION(&ImgVers);    
  return "SDL "+ToString(SdlVers.major)+"."+ToString(SdlVers.minor)+"."+ToString(SdlVers.patch)+
  "(img:"+ToString(ImgVers.major)+"."+ToString(ImgVers.minor)+"."+ToString(ImgVers.patch)+")"; 
}

//Get SDL library version
String _GetSdlLinkedVersion(){   
  SDL_version SdlVers; 
  SDL_version ImgVers;
  SDL_GetVersion(&SdlVers); 
  ImgVers=*(const SDL_version *)IMG_Linked_Version();
  return "SDL "+ToString(SdlVers.major)+"."+ToString(SdlVers.minor)+"."+ToString(SdlVers.patch)+
  "(img:"+ToString(ImgVers.major)+"."+ToString(ImgVers.minor)+"."+ToString(ImgVers.patch)+")"; 
}

//Set error functions
void _SetError(AglErrorCode Code){ _ErrorCode=Code; _ErrorInfo=""; }
void _SetError(AglErrorCode Code,const String& Info){ _ErrorCode=Code; _ErrorInfo=Info; }

//Process change handler (updates internal state variables)
bool _InternalProcessChange(cint ProcessId){

  //Variables
  int StateIndex=-1;
  
  //Get stored state for process
  for(int i=0;i<_State.Length();i++){
    if(_State[i].ProcessId==ProcessId){ StateIndex=i; break; }
  }
  if(StateIndex!=-1){
    _ProcessId=_State[StateIndex].ProcessId;
    _ErrorCode=_State[StateIndex].ErrorCode;
    _ErrorInfo=_State[StateIndex].ErrorInfo;
    _ErrorText=_State[StateIndex].ErrorText;
    _LastError=_State[StateIndex].LastError;
    _Version=_State[StateIndex].Version;
    _Locked=_State[StateIndex].Locked;
    _CurrDispHnd=_State[StateIndex].CurrDispHnd;
    _CurrFont=_State[StateIndex].CurrFont;
    _FontAdvance=_State[StateIndex].FontAdvance;
    _FontOffsetX=_State[StateIndex].FontOffsetX;
    _FontOffsetY=_State[StateIndex].FontOffsetY;
    _FontForeColor=_State[StateIndex].FontForeColor;
    _FontBackColor=_State[StateIndex].FontBackColor;
    _FontRegeneration=_State[StateIndex].FontRegeneration;
    _Win=_State[StateIndex].Win;
    _Ren=_State[StateIndex].Ren;
    _Sfc=_State[StateIndex].Sfc;
    return true;
  }

  //Create new state if process does not have if
  _ProcessId=ProcessId;
  _ErrorCode=AglErrorCode::Ok;
  _ErrorInfo="";
  _ErrorText="";
  _LastError="";
  _Version="";
  _Locked=false;
  _CurrDispHnd=-1;
  _CurrFont=-1;
  _FontAdvance=0;
  _FontOffsetX=0;
  _FontOffsetY=0;
  _FontForeColor=0xFFFFFFFF;
  _FontBackColor=0xFF000000;
  _FontRegeneration=false;
  _Win=nullptr;
  _Ren=nullptr;
  _Sfc=nullptr;

  //Store in table
  _State.Add((AglState){
  _ProcessId,
  _ErrorCode,
  _ErrorInfo,
  _ErrorText,
  _LastError,
  _Version,
  _Locked,
  _CurrDispHnd,
  _CurrFont,
  _FontAdvance,
  _FontOffsetX,
  _FontOffsetY,
  _FontForeColor,
  _FontBackColor,
  _FontRegeneration,
  _Win,
  _Ren,
  _Sfc
  });

  //Return code
  return true;

}

//InitDevice
bool _InitDevice(AglDevice Dev){
  
  //Variables
  bool Error=false;
  String Info;
  cint ImgInitted=0;
  cint ImgFlags=(IMG_INIT_JPG|IMG_INIT_PNG);
  
  //Init device
  switch(Dev){
    case AglDevice::Graphics: 
      if(!_DeviceStarted[(cint)Dev]){ 
        if(SDL_WasInit(SDL_INIT_VIDEO)!=0){ 
          LibDebugMsg(String("Graphics device starting").CharPnt());
          if(SDL_InitSubSystem(SDL_INIT_VIDEO )!=0){ Error=true; Info=String(SDL_GetError()); SDL_ClearError(); } 
          else{ LibDebugMsg(String("Graphics device started").CharPnt()); }
        } 
      } 
      break;
    case AglDevice::Event: 
      if(!_DeviceStarted[(cint)Dev]){ 
        if(SDL_WasInit(SDL_INIT_EVENTS)!=0){ 
          LibDebugMsg(String("Events device starting").CharPnt());
          if(SDL_InitSubSystem(SDL_INIT_EVENTS)!=0){ Error=true; Info=String(SDL_GetError()); SDL_ClearError(); } 
          else{ LibDebugMsg(String("Events device started").CharPnt()); }
        } 
      } 
      break;
    case AglDevice::Audio: 
      if(!_DeviceStarted[(cint)Dev]){ 
        if(SDL_WasInit(SDL_INIT_AUDIO)!=0){ 
          LibDebugMsg(String("Audio device starting").CharPnt());
          if(SDL_InitSubSystem(SDL_INIT_AUDIO )!=0){ Error=true; Info=String(SDL_GetError()); SDL_ClearError(); } 
          else{ LibDebugMsg(String("Audio device started").CharPnt()); }
        } 
      } 
      break;
    case AglDevice::Image: 
      if(!_DeviceStarted[(cint)Dev]){ 
        ImgInitted=IMG_Init(ImgFlags); 
        LibDebugMsg(String("Image device starting").CharPnt());
        if((ImgInitted&ImgFlags)!=ImgFlags){ Error=true; Info="SDL image initialization failed"; } 
        else{ LibDebugMsg(String("Image device started").CharPnt()); }
      } 
      break;
  }
  if(Error){
    _SetError(AglErrorCode::InitDeviceError,Info);
    return false;
  }

  //Set device initialization flag
  _DeviceStarted[(cint)Dev]=true;

  //Return code
  return true;

}

//StopDevice
void _StopDevice(AglDevice Dev){
  
  //Variables
  cint i;

  //Destroy graphic resources
  if(Dev==AglDevice::Graphics){
  
    //Destroy font textures
    for(i=0;i<_Font.Length();i++){
      if(_Font[i].Tex!=nullptr){ SDL_DestroyTexture(_Font[i].Tex); }
    }

    //Destroy unreleased textures
    for(i=0;i<_Txtr.Length();i++){
      if(_Txtr[i].Used){ 
        SDL_DestroyTexture(_Txtr[i].Txtr); 
      }
    }
    
    //Destroy renderers and displays
    for(i=0;i<_Disp.Length();i++){
      if(_Disp[i].Used){
        SDL_DestroyRenderer(_Disp[i].Ren);
        SDL_DestroyWindow(_Disp[i].Win);
      }
    }

  }

  //Stop device
  switch(Dev){
    case AglDevice::Graphics: 
      if(_DeviceStarted[(cint)Dev]){ 
        LibDebugMsg(String("Graphics device stopping").CharPnt());
        SDL_QuitSubSystem(SDL_INIT_VIDEO); 
        LibDebugMsg(String("Graphics device stopped").CharPnt());
      } 
      break;
    case AglDevice::Event: 
      if(_DeviceStarted[(cint)Dev]){ 
        LibDebugMsg(String("Events device stopping").CharPnt());
        SDL_QuitSubSystem(SDL_INIT_EVENTS); 
        LibDebugMsg(String("Events device stopped").CharPnt());
      } 
      break;
    case AglDevice::Audio: 
      if(_DeviceStarted[(cint)Dev]){ 
        LibDebugMsg(String("Audio device stopping").CharPnt());
        SDL_QuitSubSystem(SDL_INIT_AUDIO); 
        LibDebugMsg(String("Audio device stopped").CharPnt());
      } 
      break;
    case AglDevice::Image: 
      if(_DeviceStarted[(cint)Dev]){ 
        LibDebugMsg(String("Image device stopping").CharPnt());
        IMG_Quit(); 
        LibDebugMsg(String("Image device stopped").CharPnt());
      } 
      break;
  }

}

//Close everthing
void _CloseAll(){
  _StopDevice(AglDevice::Graphics);
  _StopDevice(AglDevice::Event);
  _StopDevice(AglDevice::Audio);
  _StopDevice(AglDevice::Image);
  SDL_Quit();
}

//Initialize library
CALLDEFN bool Init(cint ProcessId,char **Error){

  //Exit if initilization already completed
  if(_Init){ return true; }
  
  //Main ready on windows
  #ifdef __WIN__
  SDL_SetMainReady();
  #endif

  //Check versions
  String Vers1=_GetSdlCompiledVersion();
  String Vers2=_GetSdlLinkedVersion();
  if(Vers1!=Vers2){
    _SetError(AglErrorCode::InitError,"Wrong version of SDL library, it is expected to be "+Vers1+" but it is "+Vers2);
    *Error=_ErrorInfo.CharPnt();
    return false;
  }

  //Set internal event handlers
  SetClosingFunction(&_CloseAll);
  SetProcessChangeFunction(&_InternalProcessChange);
  
  //Create process state
  _InternalProcessChange(ProcessId);

  //Set init flag
  _Init=true;
  
  //Return code
  return true;

}

//SubsystemVersion
CALLDEFN char *GetVersionString(){
  String Vers1=_GetSdlCompiledVersion();
  String Vers2=_GetSdlLinkedVersion();
  _Version="AGL "+String(AGL_VERSION)+" - "+Vers1;
  if(Vers1!=Vers2){
    _Version+=" [libraries "+Vers2+"]";
  }
  return _Version.CharPnt();
}

//Get new display handler
bool _NewDispHandler(cint *DispHnd){
  cint i;
  AglDisp NewDisp=(AglDisp){_ProcessId,true,nullptr,nullptr,nullptr};
  for(i=0;i<_Disp.Length();i++){ 
    if(!_Disp[i].Used){ 
      *DispHnd=i; 
      _Disp[*DispHnd]=NewDisp;
      return true; 
    }
  }
  _Disp.Add(NewDisp);
  *DispHnd=_Disp.Length()-1;
  return true;
}

//Free display handler
void _FreeDispHandler(cint DispHnd){
  
  //Variables
  cint i;
  cint Count;

  //Free handler
  _Disp[DispHnd].Used=false;
  _Disp[DispHnd].ProcessId=-1;
  _Disp[DispHnd].Win=nullptr;
  _Disp[DispHnd].Ren=nullptr;
  _Disp[DispHnd].Sfc=nullptr;

  //Count handlers to be freed at end of array
  Count=0;
  for(i=_Disp.Length()-1;i>=0;i--){
    if(!_Disp[i].Used && _Disp[i].ProcessId==_ProcessId){ Count++; }
    else{ break; }
  }

  //Free elements
  if(Count!=0){
    _Disp.Resize(_Disp.Length()-Count);
  }

}

//Check access to display handler
bool _CheckDispHndAccess(cint DispHnd){
  if(DispHnd<0 || DispHnd>_Disp.Length()-1){
    _SetError(AglErrorCode::InvalidDispHnd);
    return false;
  }
  if(!_Disp[DispHnd].Used){
    _SetError(AglErrorCode::UnusedDispHnd);
    return false;
  }
  if(_Disp[DispHnd].ProcessId!=_ProcessId){
    _SetError(AglErrorCode::ForbiddenDispHnd);
    return false;
  }
  return true;
}

//Get new display handler
bool _NewTxtrHandler(cint *TxtrHnd){
  cint i;
  AglTxtr NewTxtr=(AglTxtr){true,_ProcessId,nullptr,0,0,false,0,0,0,SDL_FLIP_NONE};
  for(i=0;i<_Txtr.Length();i++){ 
    if(!_Txtr[i].Used){ 
      *TxtrHnd=i; 
      _Txtr[*TxtrHnd]=NewTxtr;
      return true; 
    }
  }
  _Txtr.Add(NewTxtr);
  *TxtrHnd=_Txtr.Length()-1;
  return true;
}

//Free display handler
void _FreeTxtrHandler(cint TxtrHnd){

  //Variables
  cint i;
  cint Count;

  //Free handler
  _Txtr[TxtrHnd].Used=false;
  _Txtr[TxtrHnd].ProcessId=-1;
  _Txtr[TxtrHnd].Txtr=nullptr;
  _Txtr[TxtrHnd].Width=0;
  _Txtr[TxtrHnd].Height=0;
  _Txtr[TxtrHnd].ExMode=false;
  _Txtr[TxtrHnd].Angle=0;
  _Txtr[TxtrHnd].Rotx=0;
  _Txtr[TxtrHnd].Roty=0;
  _Txtr[TxtrHnd].Flip=SDL_FLIP_NONE;

  //Count handlers to be freed at end of array
  Count=0;
  for(i=_Txtr.Length()-1;i>=0;i--){
    if(!_Txtr[i].Used && _Txtr[i].ProcessId==_ProcessId){ Count++; }
    else{ break; }
  }

  //Free elements
  if(Count!=0){
    _Txtr.Resize(_Txtr.Length()-Count);
  }

}

//Check access to display handler
bool _CheckTxtrHndAccess(cint TxtrHnd){
  if(TxtrHnd<0 || TxtrHnd>_Txtr.Length()-1){
    _SetError(AglErrorCode::InvalidTxtrHnd);
    return false;
  }
  if(!_Txtr[TxtrHnd].Used){
    _SetError(AglErrorCode::UnusedTxtrHnd);
    return false;
  }
  if(_Txtr[TxtrHnd].ProcessId!=_ProcessId){
    _SetError(AglErrorCode::ForbiddenTxtrHnd);
    return false;
  }
  return true;
}

//Generate font textures
cint _GenerateFont(AglSystemFont SysFont,cint ScaleX,cint ScaleY){
  
  //Variables
  cint Hnd;
  cint c;
  cint CharNr;
  clong x,y;
  clong i,j,k,p;
  clong w,h;
  clong TexWidth;
  clong TexHeight;
  FontIndex Char;
  AglSystemFontTable Fnt;
  cint *Pixel;
  cint Pitch;
  SDL_Texture *Tex;
  unsigned char fr,fg,fb,fa;
  
  //Exit on error condition
  if(_Locked){ return -1; }

  //Check font already generated
  Hnd=-1;
  for(i=0;i<_Font.Length();i++){
    if(_Font[i].ProcessId==_ProcessId && _Font[i].SysFont==SysFont 
    && _Font[i].ScaleX==ScaleX && _Font[i].ScaleY==ScaleY
    && _Font[i].ForeColor==_FontForeColor && _Font[i].BackColor==_FontBackColor){ Hnd=i; break; }
  }
  if(Hnd!=-1){
    return Hnd;
  }

  //Debug message
  LibDebugMsg(("Generating textures for system font "+ToString((int)SysFont)+" with scale "+ToString(ScaleX)+","+ToString(ScaleY)).CharPnt());

  //Get color components of current font foreground color
  fa=(_FontForeColor&0xFF000000)>>24;
  fr=(_FontForeColor&0x00FF0000)>>16;
  fg=(_FontForeColor&0x0000FF00)>>8;
  fb=(_FontForeColor&0x000000FF)>>0;

  //Get system font
  Fnt=_SystemFont[(int)SysFont];

  //Get number of chars in font
  CharNr=Fnt.MaxChar-Fnt.MinChar+1;

  //Create texture for font
  TexWidth=ScaleX*Fnt._Width*FONT_TEXTURE_ROW;
  TexHeight=ScaleY*Fnt._Height*(1+CharNr/FONT_TEXTURE_ROW);
  if((Tex=SDL_CreateTexture(_Ren,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,TexWidth,TexHeight))==nullptr){
    _SetError(AglErrorCode::LoadSystemFontError,SDL_GetError());
    SDL_ClearError();
    return -1;
  }
  if(SDL_LockTexture(Tex,nullptr,(void **)&Pixel,&Pitch)!=0){
    _SetError(AglErrorCode::LoadSystemFontError,SDL_GetError());
    SDL_ClearError();
    return -1;
  }

  //Print character loop
  for(c=Fnt.MinChar;c<=Fnt.MaxChar;c++){

    //Get character data
    Char=Fnt.Index[c];

    //Calculate char coordinates on texture
    x=ScaleX*Fnt._Width*((c-Fnt.MinChar)%FONT_TEXTURE_ROW);
    y=ScaleY*Fnt._Height*((c-Fnt.MinChar)/FONT_TEXTURE_ROW);

    //Render char
    k=Char.Offset;
    for(j=0;j<Fnt._Height;j++){
    for(i=0;i<Fnt._Width;i++){
      if(i>=Char.Left && i<Char.Left+Char.Width
      && j>=Char.Top  && j<Char.Top +Char.Height){
        for(w=0;w<ScaleX;w++){
        for(h=0;h<ScaleY;h++){
          p=(y+j*ScaleY+h)*TexWidth+(x+i*ScaleX+w);
          if(Fnt.Data[k]!=0){
            Pixel[p]=(
              (fa<<24)|
              ((unsigned char)(((unsigned int)Fnt.Data[k]*(unsigned int)fr)/(unsigned int)255)<<16)|
              ((unsigned char)(((unsigned int)Fnt.Data[k]*(unsigned int)fg)/(unsigned int)255)<<8)|
              ((unsigned char)(((unsigned int)Fnt.Data[k]*(unsigned int)fb)/(unsigned int)255))
            );
          }
          else{
            Pixel[p]=_FontBackColor;
          }
        }}
        k++;
      }
      else{
        for(w=0;w<ScaleX;w++){
        for(h=0;h<ScaleY;h++){
          p=(y+j*ScaleY+h)*TexWidth+(x+i*ScaleX+w);
          Pixel[p]=_FontBackColor;
        }}
      }
    }}

  }

  //Update texture
  SDL_UnlockTexture(Tex);
  
  //Set texture blending mode (applies alpha transparencies when rendering)
  SDL_SetTextureBlendMode(Tex,SDL_BLENDMODE_BLEND);
  
  //Create font handler
  _Font.Add((AglFont){_ProcessId,SysFont,ScaleX,ScaleY,Fnt._Width*ScaleX,Fnt._Height*ScaleY,_FontForeColor,_FontBackColor,Tex});
  Hnd=_Font.Length()-1;

  //Clear font reneration flag
  _FontRegeneration=false;

  //Return handler
  return Hnd;
  
}

//Print char from font
void _PrintChar(cint x,cint y,cint Chr){
  
  //Variables
  AglFont Font;
  AglSystemFontTable SysFont;
  FontIndex Char;
  SDL_Rect Sour; 
  SDL_Rect Dest;

  //Renerate font if colors were changed
  //Happens when setfont*color() is called after setfont()
  if(_FontRegeneration==true){
    cint Hnd;
    if(_Locked){ return; }
    if(GetCurrentDisplay()==-1){
      _Locked=true;
      _SetError(AglErrorCode::NoCurrentDisp);
      return;
    }
    if((Hnd=_GenerateFont(_Font[_CurrFont].SysFont,_Font[_CurrFont].ScaleX,_Font[_CurrFont].ScaleY))==-1){
      _Locked=true;
      return;
    }
    _CurrFont=Hnd;
  }

  //Get font data
  Font=_Font[_CurrFont];
  SysFont=_SystemFont[(int)Font.SysFont];

  //Check character exists in font
  if(Chr>=SysFont.MinChar && Chr<=SysFont.MaxChar){
    Char=SysFont.Index[Chr];
  }
  else{
    Char=SysFont.Index[SysFont.MinChar];
  }

  //Calculate source rect (char position on texture)
  Sour.x=Font.Width*((Chr-SysFont.MinChar)%FONT_TEXTURE_ROW);
  Sour.y=Font.Height*((Chr-SysFont.MinChar)/FONT_TEXTURE_ROW);
  Sour.w=Font.Width;
  Sour.h=Font.Height;

  //Calculate destination rect (char position on screen)
  Dest.x=x+_FontOffsetX+_FontAdvance;
  Dest.y=y+_FontOffsetY;
  Dest.w=Font.Width;
  Dest.h=Font.Height;
  
  //Render char
  if(SDL_RenderCopy(_Ren,Font.Tex,&Sour,&Dest)!=0){
    _Locked=true;
    _SetError(AglErrorCode::PrintCharError,SDL_GetError());
    SDL_ClearError();
    return;
  }

  //Set font advance so next char is printed on next column
  _FontAdvance+=Char.Advance*Font.ScaleX;

}

//DisplayOpen
CALLDEFN bool DisplayOpen(const char *Title,cint x,cint y,cint width,cint height,cint Flags,cint *DispHnd){
  
  //Variables
  cint i;
  cint px=x;
  cint py=y;
  cint ax=width;
  cint ay=height;
  cint WinFlags=0;
  cint RenFlags=0;
  cint Displays;

  //Check error condition
  if(_Locked){ return false; }

  //Initialize graphics device (it is actually done only once)
  if(!_InitDevice(AglDevice::Graphics)){ return false; }
  if(!_InitDevice(AglDevice::Event)){ return false; }

  //Set centered location or host defined location
  if((Flags&(cint)AglDisplayFlag::Centered) || (Flags&(cint)AglDisplayFlag::Located)){
    if((Flags&(cint)AglDisplayFlag::Centered)){
      px=SDL_WINDOWPOS_CENTERED;
      py=SDL_WINDOWPOS_CENTERED;
    }
    else if((Flags&(cint)AglDisplayFlag::Located)){
      px=SDL_WINDOWPOS_UNDEFINED;
      py=SDL_WINDOWPOS_UNDEFINED;
    }
  }

  //Dò not pass size if display is full screen
  if((Flags&(cint)AglDisplayFlag::FullScreen)){
    ax=0;
    ay=0;
  }

  //Set SDL window flags
  if((Flags&(cint)AglDisplayFlag::Native)){ WinFlags|=SDL_WINDOW_FULLSCREEN; }
  if((Flags&(cint)AglDisplayFlag::FullScreen)){ WinFlags|=SDL_WINDOW_FULLSCREEN_DESKTOP; }
  if((Flags&(cint)AglDisplayFlag::BorderLess)){ WinFlags|=SDL_WINDOW_BORDERLESS; }
  if((Flags&(cint)AglDisplayFlag::Resizable)){ WinFlags|=SDL_WINDOW_RESIZABLE; }

  //Set SDL renderer flags
  RenFlags=SDL_RENDERER_TARGETTEXTURE;
  if((Flags&(cint)AglDisplayFlag::Accelerated)){ RenFlags|=SDL_RENDERER_ACCELERATED; }
  if((Flags&(cint)AglDisplayFlag::Software)){ RenFlags|=SDL_RENDERER_SOFTWARE; }
  if((Flags&(cint)AglDisplayFlag::RetraceSync)){ RenFlags|=SDL_RENDERER_PRESENTVSYNC; }

  //Get new display handler
  if(!_NewDispHandler(DispHnd)){ return false; }

  //Create window
  if((_Disp[*DispHnd].Win=SDL_CreateWindow(Title,px,py,ax,ay,WinFlags))==nullptr){
    _SetError(AglErrorCode::DisplayOpenError,SDL_GetError());
    SDL_ClearError();
    return false;
  }

  //Create renderer
  if((_Disp[*DispHnd].Ren=SDL_CreateRenderer(_Disp[*DispHnd].Win,-1,RenFlags))==nullptr){
    _SetError(AglErrorCode::DisplayOpenError,SDL_GetError());
    SDL_ClearError();
    return false;
  }

  //Clear screen
  if(SDL_RenderClear(_Disp[*DispHnd].Ren)!=0){
    _SetError(AglErrorCode::DisplayOpenError,SDL_GetError());
    SDL_ClearError();
    return false;
  }

  //In case we have only one display for current process make it the current one
  for(i=0,Displays=0;i<_Disp.Length();i++){ if(_Disp[i].ProcessId==_ProcessId){ Displays++; } }
  if(Displays==1){ SetCurrentDisplay(*DispHnd); }

  //Return code
  return true;
}

//DisplayClose
CALLDEFN bool DisplayClose(cint DispHnd){
  if(_Locked){ return false; }
  if(!_CheckDispHndAccess(DispHnd)){ return false; }
  SDL_DestroyRenderer(_Disp[DispHnd].Ren);
  SDL_DestroyWindow(_Disp[DispHnd].Win);
  _FreeDispHandler(DispHnd);
  return true;
}

//Get display info
CALLDEFN bool GetDisplayInfo(cint DispHnd,AglDispInfo *Info){
  if(_Locked){ return false; }
  if(!_CheckDispHndAccess(DispHnd)){ return false; }
  SDL_GetWindowPosition(_Disp[DispHnd].Win,&Info->Left,&Info->Top);
  SDL_GetWindowSize(_Disp[DispHnd].Win,&Info->Width,&Info->Height);
  strncpy(Info->Title,SDL_GetWindowTitle(_Disp[DispHnd].Win),DISPTITLELEN);
  Info->Title[DISPTITLELEN]=0;
  return true;
}

//Set current display
CALLDEFN bool SetCurrentDisplay(cint DispHnd){
  if(_Locked){ return false; }
  if(!_CheckDispHndAccess(DispHnd)){ return false; }
  _CurrDispHnd=DispHnd;
  _Win=_Disp[DispHnd].Win;
  _Ren=_Disp[DispHnd].Ren;
  _Sfc=_Disp[DispHnd].Sfc;
  return true;
}

//Get current display
CALLDEFN cint GetCurrentDisplay(){
  return _CurrDispHnd;
}

//Set screen logical size
CALLDEFN void SetLogicalSize(cint width,cint height){
  if(_Locked){ return; }
  if(SDL_RenderSetLogicalSize(_Ren,width,height)!=0){
    _Locked=true;
    _SetError(AglErrorCode::SetLogSizeError,SDL_GetError());
    SDL_ClearError();
  }
}

//Set screen view port
CALLDEFN void SetViewPort(cint x,cint y,cint width,cint height){
  if(_Locked){ return; }
  SDL_Rect Rect=(SDL_Rect){.x=x,.y=y,.w=width,.h=height};
  if(SDL_RenderSetViewport(_Ren,(const SDL_Rect *)&Rect)!=0){
    _Locked=true;
    _SetError(AglErrorCode::SetViewPortError,SDL_GetError());
    SDL_ClearError();
  }
}

//Set screen clipping area
CALLDEFN void SetClipArea(cint x,cint y,cint width,cint height){
  if(_Locked){ return; }
  SDL_Rect Rect=(SDL_Rect){.x=x,.y=y,.w=width,.h=height};
  if(SDL_RenderSetClipRect(_Ren,(const SDL_Rect *)&Rect)!=0){
    _Locked=true;
    _SetError(AglErrorCode::SetClipAreaError,SDL_GetError());
    SDL_ClearError();
  }
}

//DisplayShow
CALLDEFN void DisplayShow(){
  if(_Locked){ return; }
  SDL_RenderPresent(_Ren);
}

//Display clear
CALLDEFN void Clear(){
  if(_Locked){ return; }
  if(SDL_RenderClear(_Ren)!=0){
    _Locked=true;
    _SetError(AglErrorCode::ClearError,SDL_GetError());
    SDL_ClearError();
  };
}

//DisplaySetColor
CALLDEFN void SetColor(char r,char g,char b,char a){
  if(_Locked){ return; }
  if(SDL_SetRenderDrawColor(_Ren,r,g,b,a)!=0){
    _Locked=true;
    _SetError(AglErrorCode::SetColorError,SDL_GetError());
    SDL_ClearError();
  };
}

//DisplaySetPoint
CALLDEFN void SetPoint(cint x,cint y){
  if(_Locked){ return; }
  if(SDL_RenderDrawPoint(_Ren,x,y)!=0){
    _Locked=true;
    _SetError(AglErrorCode::DrawPointError,SDL_GetError());
    SDL_ClearError();
  };
}

//DisplayDrawLine
CALLDEFN void DrawLine(cint x1,cint y1,cint x2,cint y2){
  if(_Locked){ return; }
  if(SDL_RenderDrawLine(_Ren,x1,y1,x2,y2)!=0){;
    _Locked=true;
    _SetError(AglErrorCode::DrawLineError,SDL_GetError());
    SDL_ClearError();
  };
}

//DrawRectangle
CALLDEFN void DrawRectangle(cint x1,cint y1,cint x2,cint y2){
  if(_Locked){ return; }
  SDL_Rect Rect=(SDL_Rect){x1,y1,x2-x1,y2-y1};
  if(SDL_RenderDrawRect(_Ren,&Rect)!=0){
    _Locked=true;
    _SetError(AglErrorCode::DrawRectError,SDL_GetError());
    SDL_ClearError();
  };
}

//DrawFillRectangle
CALLDEFN void DrawFillRectangle(cint x1,cint y1,cint x2,cint y2){
  if(_Locked){ return; }
  SDL_Rect Rect=(SDL_Rect){x1,y1,x2-x1,y2-y1};
  if(SDL_RenderFillRect(_Ren,&Rect)!=0){
    _Locked=true;
    _SetError(AglErrorCode::DrawFillRectError,SDL_GetError());
    SDL_ClearError();
  };
}

//Draw circle
CALLDECL void DrawCircle(cint x,cint y,cint r){

  //Exit on error condition
  if(_Locked){ return; }

  //Variables
  cint i,j,dp;

  //Circle algorithm
  i=0;
  j=r;
  dp=1-r;
  do{
    if(SDL_RenderDrawPoint(_Ren,x+j,y-i)!=0){ _Locked=true; _SetError(AglErrorCode::DrawPointError,SDL_GetError()); SDL_ClearError(); return; };
    if(SDL_RenderDrawPoint(_Ren,x+i,y-j)!=0){ _Locked=true; _SetError(AglErrorCode::DrawPointError,SDL_GetError()); SDL_ClearError(); return; };
    if(SDL_RenderDrawPoint(_Ren,x-i,y-j)!=0){ _Locked=true; _SetError(AglErrorCode::DrawPointError,SDL_GetError()); SDL_ClearError(); return; };
    if(SDL_RenderDrawPoint(_Ren,x-j,y-i)!=0){ _Locked=true; _SetError(AglErrorCode::DrawPointError,SDL_GetError()); SDL_ClearError(); return; };
    if(SDL_RenderDrawPoint(_Ren,x-j,y+i)!=0){ _Locked=true; _SetError(AglErrorCode::DrawPointError,SDL_GetError()); SDL_ClearError(); return; };
    if(SDL_RenderDrawPoint(_Ren,x-i,y+j)!=0){ _Locked=true; _SetError(AglErrorCode::DrawPointError,SDL_GetError()); SDL_ClearError(); return; };
    if(SDL_RenderDrawPoint(_Ren,x+i,y+j)!=0){ _Locked=true; _SetError(AglErrorCode::DrawPointError,SDL_GetError()); SDL_ClearError(); return; };
    if(SDL_RenderDrawPoint(_Ren,x+j,y+i)!=0){ _Locked=true; _SetError(AglErrorCode::DrawPointError,SDL_GetError()); SDL_ClearError(); return; };
    if(dp<0){ dp=dp+2*(++i)+3; }
    else    { dp=dp+2*(++i)-2*(--j)+5; }
  }while(i<=j);

}

//Draw fill circle
CALLDECL void DrawFillCircle(cint x,cint y,cint r){

  //Exit on error condition
  if(_Locked){ return; }

  //Variables
  cint i,j,dp;

  //Circle algorithm
  i=0;
  j=r;
  dp=1-r;
  do{
    if(SDL_RenderDrawLine(_Ren,x+i,y-j,x-i,y-j)!=0){ _Locked=true; _SetError(AglErrorCode::DrawPointError,SDL_GetError()); SDL_ClearError(); return; };
    if(SDL_RenderDrawLine(_Ren,x-j,y-i,x+j,y-i)!=0){ _Locked=true; _SetError(AglErrorCode::DrawPointError,SDL_GetError()); SDL_ClearError(); return; };
    if(SDL_RenderDrawLine(_Ren,x+i,y+j,x-i,y+j)!=0){ _Locked=true; _SetError(AglErrorCode::DrawPointError,SDL_GetError()); SDL_ClearError(); return; };
    if(SDL_RenderDrawLine(_Ren,x+j,y+i,x-j,y+i)!=0){ _Locked=true; _SetError(AglErrorCode::DrawPointError,SDL_GetError()); SDL_ClearError(); return; };
    if(dp<0){ dp=dp+2*(++i)+3; }
    else    { dp=dp+2*(++i)-2*(--j)+5; }
  }while(i<=j);

}

//SetFont
CALLDEFN void SetFont(AglSystemFont Font,cint ScaleX,cint ScaleY){
  cint Hnd;
  if(_Locked){ return; }
  if((cint)Font<0 || (cint)Font>=AGL_SYSTEM_FONTS){
    _Locked=true;
    _SetError(AglErrorCode::InvalidFont);
    return;
  }
  if(GetCurrentDisplay()==-1){
    _Locked=true;
    _SetError(AglErrorCode::NoCurrentDisp);
    return;
  }
  if((Hnd=_GenerateFont(Font,ScaleX,ScaleY))==-1){
    _Locked=true;
    return;
  }
  _CurrFont=Hnd;
}

//SetFontForeColor
CALLDEFN void SetFontForeColor(char r,char g,char b,char a){
  cint FontForeColor=(((unsigned char)a<<24)|((unsigned char)r<<16)|((unsigned char)g<<8)|((unsigned char)b));
  if(FontForeColor!=_FontForeColor){
    _FontForeColor=FontForeColor;
    _FontRegeneration=true;  

  }
}

//SetFontBackColor
CALLDEFN void SetFontBackColor(char r,char g,char b,char a){
  cint FontBackColor=(((unsigned char)a<<24)|((unsigned char)r<<16)|((unsigned char)g<<8)|((unsigned char)b));
  if(FontBackColor!=_FontBackColor){
    _FontBackColor=FontBackColor;
    _FontRegeneration=true;  

  }
}

//Get current font
CALLDEFN AglSystemFont GetCurrentFont(){
  return (_CurrFont==-1?(AglSystemFont)-1:_Font[_CurrFont].SysFont);
}


//GetCurrentFontScaleX
CALLDECL cint GetCurrentFontScaleX(){
  return (_CurrFont==-1?0:_Font[_CurrFont].ScaleX);
}

//GetCurrentFontScaleY
CALLDECL cint GetCurrentFontScaleY(){
  return (_CurrFont==-1?0:_Font[_CurrFont].ScaleY);
}

//GetFontInfo
CALLDEFN bool GetFontInfo(AglSystemFont Font,AglFontInfo *Info){
  if((cint)Font<0 || (cint)Font>=AGL_SYSTEM_FONTS){ return false; }
  strncpy(Info->CodeName,_SystemFont[(cint)Font].CodeName,FONTNAMELEN); Info->CodeName[FONTNAMELEN]=0;
  strncpy(Info->FullName,_SystemFont[(cint)Font].FullName,FONTNAMELEN); Info->FullName[FONTNAMELEN]=0;
  Info->Size=_SystemFont[(cint)Font].Size;
  Info->MonoSpace=_SystemFont[(cint)Font].MonoSpace;
  Info->Width=_SystemFont[(cint)Font]._Width;
  Info->Height=_SystemFont[(cint)Font]._Height;
  Info->MinChar=_SystemFont[(cint)Font].MinChar;
  Info->MaxChar=_SystemFont[(cint)Font].MaxChar;
  return true;
}

//Get Font Lines
CALLDEFN cint GetFontLines(AglSystemFont Font){
  cint ax,ay;
  if(_Locked){ return 0; }
  if((cint)Font<0 || (cint)Font>=AGL_SYSTEM_FONTS){
    _Locked=true;
    _SetError(AglErrorCode::InvalidFont);
    return 0;
  }
  SDL_GetWindowSize(_Win,&ax,&ay);
  return ax/_SystemFont[(cint)Font]._Width;
}

//Get Font Columns
CALLDEFN cint GetFontColumns(AglSystemFont Font){
  cint ax,ay;
  if(_Locked){ return 0; }
  if((cint)Font<0 || (cint)Font>=AGL_SYSTEM_FONTS){
    _Locked=true;
    _SetError(AglErrorCode::InvalidFont);
    return 0;
  }
  if(_SystemFont[(cint)Font].MonoSpace){ return 0; }
  SDL_GetWindowSize(_Win,&ax,&ay);
  return ay/_SystemFont[(cint)Font]._Height;
}

//SetFontOffset
CALLDEFN void SetFontOffset(cint offx,cint offy){
  _FontOffsetX=offx;
  _FontOffsetY=offy;
}

//PrintChar
CALLDEFN void PrintChar(cint cx,cint cy,cint Chr){
  cint ax,ay;
  cint px,py;
  if(_Locked || _CurrFont==-1){ return; }
  _FontAdvance=0;
  SDL_GetWindowSize(_Win,&ax,&ay);
  px=cx*_Font[(cint)_CurrFont].Width;
  py=cy*_Font[(cint)_CurrFont].Height;
  _PrintChar(px,py,Chr);
}

//Print
CALLDEFN void Print(cint cx,cint cy,const char *Text){
  cint ax,ay;
  cint px,py;
  cint Length=(cint)strlen(Text);
  if(_Locked || _CurrFont==-1){ return; }
  _FontAdvance=0;
  SDL_GetWindowSize(_Win,&ax,&ay);
  px=cx*_Font[(cint)_CurrFont].Width;
  py=cy*_Font[(cint)_CurrFont].Height;
  for(cint i=0;i<Length;i++){
    _PrintChar(px,py,Text[i]);
  }
}

//PrintCharXY
CALLDEFN void PrintCharXY(cint x,cint y,cint Chr){
  if(_Locked || _CurrFont==-1){ return; }
  _FontAdvance=0;
  _PrintChar(x,y,Chr);
}

//PrintXY
CALLDEFN void PrintXY(cint x,cint y,const char *Text){
  cint Length=(cint)strlen(Text);
  if(_Locked || _CurrFont==-1){ return; }
  _FontAdvance=0;
  for(cint i=0;i<Length;i++){
    _PrintChar(x,y,Text[i]);
  }
}

//Load texture
CALLDEFN bool LoadTexture(const char *FileName,cint *TxtrHnd){

  //Variables
  SDL_Texture *Texture;
  SDL_Surface *Surface;

  //Exit on error condition
  if(_Locked){ return false; }

  //Initialize image device (it is actually done only once)
  if(!_InitDevice(AglDevice::Image)){ return false; }

  //Load texture image file
  if((Surface=IMG_Load(FileName))==nullptr){
    _SetError(AglErrorCode::LoadTextureError,IMG_GetError());
    return false;
  }

  //Create new texture from surface
  if((Texture=SDL_CreateTextureFromSurface(_Ren,Surface))==nullptr){
    SDL_FreeSurface(Surface);
    _SetError(AglErrorCode::LoadTextureError,SDL_GetError());
    return false;
  }

  //Get new texture handler
  if(!_NewTxtrHandler(TxtrHnd)){ 
    SDL_FreeSurface(Surface);
    SDL_DestroyTexture(Texture);
    return false; 
  }

  //Set texture in handler
  _Txtr[*TxtrHnd].Txtr=Texture;
  _Txtr[*TxtrHnd].Width=Surface->w;
  _Txtr[*TxtrHnd].Height=Surface->h;

  //Free surface
  SDL_FreeSurface(Surface);

  //Return code
  return true;

}

//Free texture
CALLDEFN bool FreeTexture(cint TxtrHnd){
  if(_Locked){ return false; }
  if(_CheckTxtrHndAccess(TxtrHnd)){ return false; }
  SDL_DestroyTexture(_Txtr[TxtrHnd].Txtr);
  _FreeTxtrHandler(TxtrHnd);
  return true;
}

//Get texture size
CALLDEFN void GetTextureSize(cint TxtrHnd,cint *Width,cint *Height){
  if(_Locked){ return; }
  if(_CheckTxtrHndAccess(TxtrHnd)){ return; }
  *Width=_Txtr[TxtrHnd].Width;
  *Height=_Txtr[TxtrHnd].Height;
}

//Set texture rotation
CALLDEFN void SetTextureRotation(cint TxtrHnd,double Angle,cint Rotx,cint Roty){
  if(_Locked){ return; }
  if(_CheckTxtrHndAccess(TxtrHnd)){ return; }
  _Txtr[TxtrHnd].ExMode=(Angle!=0 || _Txtr[TxtrHnd].Flip!=SDL_FLIP_NONE?true:false);
  _Txtr[TxtrHnd].Angle=Angle;
  _Txtr[TxtrHnd].Rotx=Rotx;
  _Txtr[TxtrHnd].Roty=Roty;
}

//SetTextureFlipMode
CALLDEFN void SetTextureFlipMode(cint TxtrHnd,AglFlipMode Flip){
  if(_Locked){ return; }
  if(_CheckTxtrHndAccess(TxtrHnd)){ return; }
  _Txtr[TxtrHnd].ExMode=(Flip!=AglFlipMode::None ||_Txtr[TxtrHnd].Angle!=0?true:false);
  switch(Flip){
    case AglFlipMode::None      : _Txtr[TxtrHnd].Flip=SDL_FLIP_NONE; break;
    case AglFlipMode::Horizontal: _Txtr[TxtrHnd].Flip=SDL_FLIP_HORIZONTAL; break;
    case AglFlipMode::Vertical  : _Txtr[TxtrHnd].Flip=SDL_FLIP_VERTICAL; break;
  }
}

//SetTextureBlendMode
CALLDEFN void SetTextureBlendMode(cint TxtrHnd,AglBlendMode Blend){
  if(_Locked){ return; }
  if(_CheckTxtrHndAccess(TxtrHnd)){ return; }
  bool Error=false;
  switch(Blend){
    case AglBlendMode::None : if(SDL_SetTextureBlendMode(_Txtr[TxtrHnd].Txtr,SDL_BLENDMODE_NONE)!=0){ Error=true; } break;  //None
    case AglBlendMode::Blend: if(SDL_SetTextureBlendMode(_Txtr[TxtrHnd].Txtr,SDL_BLENDMODE_NONE)!=0){ Error=true; } break;  //Alpha blending
    case AglBlendMode::Add  : if(SDL_SetTextureBlendMode(_Txtr[TxtrHnd].Txtr,SDL_BLENDMODE_NONE)!=0){ Error=true; } break;  //Additive blending
    case AglBlendMode::Mod  : if(SDL_SetTextureBlendMode(_Txtr[TxtrHnd].Txtr,SDL_BLENDMODE_NONE)!=0){ Error=true; } break;  //Color modulation blending
  }
  if(Error){
    _Locked=true;
    _SetError(AglErrorCode::SetBlendModError,SDL_GetError());
  }
}

//SetTextureColorModulation
CALLDEFN void SetTextureColorModulation(cint TxtrHnd,char r,char g,char b){
  if(_Locked){ return; }
  if(_CheckTxtrHndAccess(TxtrHnd)){ return; }
  if(SDL_SetTextureColorMod(_Txtr[TxtrHnd].Txtr,r,g,b)!=0){
    _Locked=true;
    _SetError(AglErrorCode::SetColorModError,SDL_GetError());
  }
}

//SetTextureAlphaModulation
CALLDEFN void SetTextureAlphaModulation(cint TxtrHnd,char a){
  if(_Locked){ return; }
  if(_CheckTxtrHndAccess(TxtrHnd)){ return; }
  if(SDL_SetTextureAlphaMod(_Txtr[TxtrHnd].Txtr,a)!=0){
    _Locked=true;
    _SetError(AglErrorCode::SetAlphaModError,SDL_GetError());
  }
}

//RenderTexture
CALLDEFN void RenderTexture(cint TxtrHnd,cint x,cint y){
  if(_Locked){ return; }
  if(_CheckTxtrHndAccess(TxtrHnd)){ return; }
  SDL_Rect Dest=(SDL_Rect){x,y,_Txtr[TxtrHnd].Width,_Txtr[TxtrHnd].Height};
  if(_Txtr[TxtrHnd].ExMode){
    SDL_Point Point=(SDL_Point){_Txtr[TxtrHnd].Rotx,_Txtr[TxtrHnd].Roty};
    if(SDL_RenderCopyEx(_Ren,_Txtr[TxtrHnd].Txtr,nullptr,&Dest,_Txtr[TxtrHnd].Angle,&Point,_Txtr[TxtrHnd].Flip)!=0){
      _Locked=true;
      _SetError(AglErrorCode::ExRenderError,SDL_GetError());
    }
  }
  else{
    if(SDL_RenderCopy(_Ren,_Txtr[TxtrHnd].Txtr,nullptr,&Dest)!=0){
      _Locked=true;
      _SetError(AglErrorCode::RenderError,SDL_GetError());
    }
  }
}

//RenderTextureScaled
CALLDEFN void RenderTextureScaled(cint TxtrHnd,cint x,cint y,cint ax,cint ay){
  if(_Locked){ return; }
  if(_CheckTxtrHndAccess(TxtrHnd)){ return; }
  SDL_Rect Dest=(SDL_Rect){x,y,ax,ay};
  if(_Txtr[TxtrHnd].ExMode){
    SDL_Point Point=(SDL_Point){_Txtr[TxtrHnd].Rotx,_Txtr[TxtrHnd].Roty};
    if(SDL_RenderCopyEx(_Ren,_Txtr[TxtrHnd].Txtr,nullptr,&Dest,_Txtr[TxtrHnd].Angle,&Point,_Txtr[TxtrHnd].Flip)!=0){
      _Locked=true;
      _SetError(AglErrorCode::ExRenderError,SDL_GetError());
    }
  }
  else{
    if(SDL_RenderCopy(_Ren,_Txtr[TxtrHnd].Txtr,nullptr,&Dest)!=0){
      _Locked=true;
      _SetError(AglErrorCode::RenderError,SDL_GetError());
    }
  }
}

//RenderTexturePart
CALLDEFN void RenderTexturePart(cint TxtrHnd,cint x,cint y,cint tx,cint ty,cint tax,cint tay){
  if(_Locked){ return; }
  if(_CheckTxtrHndAccess(TxtrHnd)){ return; }
  SDL_Rect Sour=(SDL_Rect){tx,ty,tax,tay};
  SDL_Rect Dest=(SDL_Rect){x,y,_Txtr[TxtrHnd].Width,_Txtr[TxtrHnd].Height};
  if(_Txtr[TxtrHnd].ExMode){
    SDL_Point Point=(SDL_Point){_Txtr[TxtrHnd].Rotx,_Txtr[TxtrHnd].Roty};
    if(SDL_RenderCopyEx(_Ren,_Txtr[TxtrHnd].Txtr,&Sour,&Dest,_Txtr[TxtrHnd].Angle,&Point,_Txtr[TxtrHnd].Flip)!=0){
      _Locked=true;
      _SetError(AglErrorCode::ExRenderError,SDL_GetError());
    }
  }
  else{
    if(SDL_RenderCopy(_Ren,_Txtr[TxtrHnd].Txtr,&Sour,&Dest)!=0){
      _Locked=true;
      _SetError(AglErrorCode::RenderError,SDL_GetError());
    }
  }
}


//RenderTexturePartScaled
CALLDEFN void RenderTexturePartScaled(cint TxtrHnd,cint x,cint y,cint ax,cint ay,cint tx,cint ty,cint tax,cint tay){
  if(_Locked){ return; }
  if(_CheckTxtrHndAccess(TxtrHnd)){ return; }
  SDL_Rect Sour=(SDL_Rect){tx,ty,tax,tay};
  SDL_Rect Dest=(SDL_Rect){x,y,ax,ay};
  if(_Txtr[TxtrHnd].ExMode){
    SDL_Point Point=(SDL_Point){_Txtr[TxtrHnd].Rotx,_Txtr[TxtrHnd].Roty};
    if(SDL_RenderCopyEx(_Ren,_Txtr[TxtrHnd].Txtr,&Sour,&Dest,_Txtr[TxtrHnd].Angle,&Point,_Txtr[TxtrHnd].Flip)!=0){
      _Locked=true;
      _SetError(AglErrorCode::ExRenderError,SDL_GetError());
    }
  }
  else{
    if(SDL_RenderCopy(_Ren,_Txtr[TxtrHnd].Txtr,&Sour,&Dest)!=0){
      _Locked=true;
      _SetError(AglErrorCode::RenderError,SDL_GetError());
    }
  }
}

//Unlock (after error)
CALLDEFN void Unlock(){
  _Locked=false;
}

//Wait for event
CALLDEFN void WaitEvent(cint Timeout){
  if(Timeout==0){
    SDL_WaitEvent(nullptr);  
  }
  else{
    SDL_WaitEventTimeout(nullptr,Timeout);
  }
}

//HasEvents
CALLDEFN bool HasEvents(){
  return SDL_PollEvent(nullptr)==0?false:true;
}

//GetEvent
CALLDEFN void GetEvent(AglEventInfo *Event){
  
  //Variables
  SDL_Event SdlEvent;

  //init result
  Event->Id=(AglEventId)-1;
  Event->DispHandler=_CurrDispHnd;
  Event->DispX=-1;
  Event->DispY=-1;
  Event->KeyModif=0;
  Event->ScanCode=(AglScanCode)-1;
  Event->Utf8Code=NULLUTF8;

  //Poll SDL event queue
  if(!SDL_PollEvent(&SdlEvent)){ return; }

  //Translate SDL event to Agl event
  switch(SdlEvent.type){
    
    //Key pressed
    case SDL_KEYDOWN: 
      Event->Id=AglEventId::KeybPressed;
      Event->KeyModif=_SdlKeyModifiersToAgl(SdlEvent.key.keysym.mod);
      Event->ScanCode=_SdlScanCodeToAgl(SdlEvent.key.keysym.scancode);
      break;

    //Key released
    case SDL_KEYUP: 
      Event->Id=AglEventId::KeybReleased;
      Event->KeyModif=_SdlKeyModifiersToAgl(SdlEvent.key.keysym.mod);
      Event->ScanCode=_SdlScanCodeToAgl(SdlEvent.key.keysym.scancode);
      break;
    
    //Text input
    case SDL_TEXTINPUT: 
      Event->Id=AglEventId::KeybInput;
      Event->Utf8Code=_Utf8SequenceToInt(SdlEvent.text.text);
      break;
    
    //Window events
    case SDL_WINDOWEVENT: 
      switch(SdlEvent.window.event){
        case SDL_WINDOWEVENT_SHOWN       : Event->Id=AglEventId::DispShown; break;       
        case SDL_WINDOWEVENT_HIDDEN      : Event->Id=AglEventId::DispHidden; break;      
        case SDL_WINDOWEVENT_EXPOSED     : Event->Id=AglEventId::DispExposed; break;     
        case SDL_WINDOWEVENT_MOVED       : Event->Id=AglEventId::DispMoved; Event->DispX=SdlEvent.window.data1; Event->DispY=SdlEvent.window.data2; break;
        case SDL_WINDOWEVENT_RESIZED     : Event->Id=AglEventId::DispResized; Event->DispX=SdlEvent.window.data1; Event->DispY=SdlEvent.window.data2; break;    
        case SDL_WINDOWEVENT_SIZE_CHANGED: Event->Id=AglEventId::DispSizeChanged; break; 
        case SDL_WINDOWEVENT_MINIMIZED   : Event->Id=AglEventId::DispMinimized; break;   
        case SDL_WINDOWEVENT_MAXIMIZED   : Event->Id=AglEventId::DispMaximized; break;   
        case SDL_WINDOWEVENT_RESTORED    : Event->Id=AglEventId::DispRestored; break;    
        case SDL_WINDOWEVENT_ENTER       : Event->Id=AglEventId::DispEnter; break;       
        case SDL_WINDOWEVENT_LEAVE       : Event->Id=AglEventId::DispLeave; break;       
        case SDL_WINDOWEVENT_FOCUS_GAINED: Event->Id=AglEventId::DispFocusGained; break; 
        case SDL_WINDOWEVENT_FOCUS_LOST  : Event->Id=AglEventId::DispFocusLost; break;   
        case SDL_WINDOWEVENT_CLOSE       : Event->Id=AglEventId::DispClose; break;       
        case SDL_WINDOWEVENT_TAKE_FOCUS  : Event->Id=AglEventId::DispTakeFocus; break;   
        default                          : Event->Id=AglEventId::NotImplemented; break;
      }
      break;
    
    //Not implemented events
    default:
      break;
  }

}

//Get Error
CALLDEFN AglErrorCode Error(){
  _Locked=false;
  return _ErrorCode;
}

//Get information about last error produced
CALLDEFN char *LastError(){
  _LastError=String(ErrorText(_ErrorCode))+(_ErrorInfo.Length()!=0?" ("+_ErrorInfo+")":"");
  return _LastError.CharPnt();
}

//Error code description
CALLDEFN char *ErrorText(AglErrorCode Err){
  switch(Err){
    case AglErrorCode::Ok                   : _ErrorText="File Ok"; break;
    case AglErrorCode::InitError            : _ErrorText="AGL initialization error"; break; 
    case AglErrorCode::InitDeviceError      : _ErrorText="Device initialization error"; break; 
    case AglErrorCode::DisplayOpenError     : _ErrorText="Display open error"; break; 
    case AglErrorCode::InvalidDispHnd       : _ErrorText="Access to invalid display handler"; break;
    case AglErrorCode::UnusedDispHnd        : _ErrorText="Access to unused display handler"; break;
    case AglErrorCode::ForbiddenDispHnd     : _ErrorText="Access to other process display handler"; break;
    case AglErrorCode::SetLogSizeError      : _ErrorText="Set logical size error"; break;
    case AglErrorCode::SetViewPortError     : _ErrorText="Set view port error"; break;
    case AglErrorCode::SetClipAreaError     : _ErrorText="Set clipping area error"; break;
    case AglErrorCode::ClearError           : _ErrorText="Screen clear error"; break;
    case AglErrorCode::SetColorError        : _ErrorText="Set color error"; break;
    case AglErrorCode::GetColorError        : _ErrorText="Get color error"; break;
    case AglErrorCode::DrawLineError        : _ErrorText="Draw line error"; break;
    case AglErrorCode::DrawPointError       : _ErrorText="Draw point error"; break;
    case AglErrorCode::DrawRectError        : _ErrorText="Draw rectangle error"; break;
    case AglErrorCode::DrawFillRectError    : _ErrorText="Draw fill rectangle error"; break;
    case AglErrorCode::LoadSystemFontError  : _ErrorText="Load system font error"; break;
    case AglErrorCode::PrintCharError       : _ErrorText="Print char error"; break;
    case AglErrorCode::InvalidFont          : _ErrorText="Invalid font"; break;
    case AglErrorCode::NoCurrentDisp        : _ErrorText="No current display set"; break;
    case AglErrorCode::InvalidTxtrHnd       : _ErrorText="Access invalid texture handler"; break;
    case AglErrorCode::UnusedTxtrHnd        : _ErrorText="Access unused texture handler"; break;
    case AglErrorCode::ForbiddenTxtrHnd     : _ErrorText="Access other process texture handler"; break;
    case AglErrorCode::LoadTextureError     : _ErrorText="Error while loading texture"; break;
    case AglErrorCode::SetBlendModError     : _ErrorText="Set texture blend modulation eerror"; break;
    case AglErrorCode::SetColorModError     : _ErrorText="Set texture color modulation error"; break;
    case AglErrorCode::SetAlphaModError     : _ErrorText="Set texture alpha modulation error"; break;
    case AglErrorCode::RenderError          : _ErrorText="Texture render error"; break;
    case AglErrorCode::ExRenderError        : _ErrorText="Texture render error"; break;

  }
  return _ErrorText.CharPnt();
}

//Translate SDL key modifiers to Agl key modifier
cint _SdlKeyModifiersToAgl(cint _SdlKeyModifiers){
  cint KeyModif=0;
  if(_SdlKeyModifiers&KMOD_LSHIFT){ KeyModif|=(cint)AglKeyModif::LShift; }
  if(_SdlKeyModifiers&KMOD_RSHIFT){ KeyModif|=(cint)AglKeyModif::RShift; }
  if(_SdlKeyModifiers&KMOD_LCTRL ){ KeyModif|=(cint)AglKeyModif::LCtrl;  }
  if(_SdlKeyModifiers&KMOD_RCTRL ){ KeyModif|=(cint)AglKeyModif::RCtrl;  }
  if(_SdlKeyModifiers&KMOD_LALT  ){ KeyModif|=(cint)AglKeyModif::LAlt;   }
  if(_SdlKeyModifiers&KMOD_RALT  ){ KeyModif|=(cint)AglKeyModif::RAlt;   }
  if(_SdlKeyModifiers&KMOD_LGUI  ){ KeyModif|=(cint)AglKeyModif::LGui;   }
  if(_SdlKeyModifiers&KMOD_RGUI  ){ KeyModif|=(cint)AglKeyModif::RGui;   }
  if(_SdlKeyModifiers&KMOD_NUM   ){ KeyModif|=(cint)AglKeyModif::Num;    }
  if(_SdlKeyModifiers&KMOD_CAPS  ){ KeyModif|=(cint)AglKeyModif::Caps;   }
  if(_SdlKeyModifiers&KMOD_MODE  ){ KeyModif|=(cint)AglKeyModif::Mode;   }
  return KeyModif;
}

//Translate SDL scan code into Agl scan code
AglScanCode _SdlScanCodeToAgl(SDL_Scancode SdlScanCode){
  AglScanCode ScanCode;
  switch(SdlScanCode){
    case SDL_SCANCODE_0:                  ScanCode=AglScanCode::Num_0; break;
    case SDL_SCANCODE_1:                  ScanCode=AglScanCode::Num_1; break;
    case SDL_SCANCODE_2:                  ScanCode=AglScanCode::Num_2; break;
    case SDL_SCANCODE_3:                  ScanCode=AglScanCode::Num_3; break;
    case SDL_SCANCODE_4:                  ScanCode=AglScanCode::Num_4; break;
    case SDL_SCANCODE_5:                  ScanCode=AglScanCode::Num_5; break;
    case SDL_SCANCODE_6:                  ScanCode=AglScanCode::Num_6; break;
    case SDL_SCANCODE_7:                  ScanCode=AglScanCode::Num_7; break;
    case SDL_SCANCODE_8:                  ScanCode=AglScanCode::Num_8; break;
    case SDL_SCANCODE_9:                  ScanCode=AglScanCode::Num_9; break;
    case SDL_SCANCODE_A:                  ScanCode=AglScanCode::A; break;
    case SDL_SCANCODE_AC_BACK:            ScanCode=AglScanCode::Ac_back; break;
    case SDL_SCANCODE_AC_BOOKMARKS:       ScanCode=AglScanCode::Ac_bookmarks; break;
    case SDL_SCANCODE_AC_FORWARD:         ScanCode=AglScanCode::Ac_forward; break;
    case SDL_SCANCODE_AC_HOME:            ScanCode=AglScanCode::Ac_home; break;
    case SDL_SCANCODE_AC_REFRESH:         ScanCode=AglScanCode::Ac_refresh; break;
    case SDL_SCANCODE_AC_SEARCH:          ScanCode=AglScanCode::Ac_search; break;
    case SDL_SCANCODE_AC_STOP:            ScanCode=AglScanCode::Ac_stop; break;
    case SDL_SCANCODE_AGAIN:              ScanCode=AglScanCode::Again; break;
    case SDL_SCANCODE_ALTERASE:           ScanCode=AglScanCode::Alterase; break;
    case SDL_SCANCODE_APOSTROPHE:         ScanCode=AglScanCode::Apostrophe; break;
    case SDL_SCANCODE_APPLICATION:        ScanCode=AglScanCode::Application; break;
    case SDL_SCANCODE_AUDIOMUTE:          ScanCode=AglScanCode::Audiomute; break;
    case SDL_SCANCODE_AUDIONEXT:          ScanCode=AglScanCode::Audionext; break;
    case SDL_SCANCODE_AUDIOPLAY:          ScanCode=AglScanCode::Audioplay; break;
    case SDL_SCANCODE_AUDIOPREV:          ScanCode=AglScanCode::Audioprev; break;
    case SDL_SCANCODE_AUDIOSTOP:          ScanCode=AglScanCode::Audiostop; break;
    case SDL_SCANCODE_B:                  ScanCode=AglScanCode::B; break;
    case SDL_SCANCODE_BACKSLASH:          ScanCode=AglScanCode::Backslash; break;
    case SDL_SCANCODE_BACKSPACE:          ScanCode=AglScanCode::Backspace; break;
    case SDL_SCANCODE_BRIGHTNESSDOWN:     ScanCode=AglScanCode::Brightnessdown; break;
    case SDL_SCANCODE_BRIGHTNESSUP:       ScanCode=AglScanCode::Brightnessup; break;
    case SDL_SCANCODE_C:                  ScanCode=AglScanCode::C; break;
    case SDL_SCANCODE_CALCULATOR:         ScanCode=AglScanCode::Calculator; break;
    case SDL_SCANCODE_CANCEL:             ScanCode=AglScanCode::Cancel; break;
    case SDL_SCANCODE_CAPSLOCK:           ScanCode=AglScanCode::Capslock; break;
    case SDL_SCANCODE_CLEAR:              ScanCode=AglScanCode::Clear; break;
    case SDL_SCANCODE_CLEARAGAIN:         ScanCode=AglScanCode::Clearagain; break;
    case SDL_SCANCODE_COMMA:              ScanCode=AglScanCode::Comma; break;
    case SDL_SCANCODE_COMPUTER:           ScanCode=AglScanCode::Computer; break;
    case SDL_SCANCODE_COPY:               ScanCode=AglScanCode::Copy; break;
    case SDL_SCANCODE_CRSEL:              ScanCode=AglScanCode::Crsel; break;
    case SDL_SCANCODE_CURRENCYSUBUNIT:    ScanCode=AglScanCode::Currencysubunit; break;
    case SDL_SCANCODE_CURRENCYUNIT:       ScanCode=AglScanCode::Currencyunit; break;
    case SDL_SCANCODE_CUT:                ScanCode=AglScanCode::Cut; break;
    case SDL_SCANCODE_D:                  ScanCode=AglScanCode::D; break;
    case SDL_SCANCODE_DECIMALSEPARATOR:   ScanCode=AglScanCode::Decimalseparator; break;
    case SDL_SCANCODE_DELETE:             ScanCode=AglScanCode::Delete; break;
    case SDL_SCANCODE_DISPLAYSWITCH:      ScanCode=AglScanCode::Displayswitch; break;
    case SDL_SCANCODE_DOWN:               ScanCode=AglScanCode::Down; break;
    case SDL_SCANCODE_E:                  ScanCode=AglScanCode::E; break;
    case SDL_SCANCODE_EJECT:              ScanCode=AglScanCode::Eject; break;
    case SDL_SCANCODE_END:                ScanCode=AglScanCode::End; break;
    case SDL_SCANCODE_EQUALS:             ScanCode=AglScanCode::Equals; break;
    case SDL_SCANCODE_ESCAPE:             ScanCode=AglScanCode::Escape; break;
    case SDL_SCANCODE_EXECUTE:            ScanCode=AglScanCode::Execute; break;
    case SDL_SCANCODE_EXSEL:              ScanCode=AglScanCode::Exsel; break;
    case SDL_SCANCODE_F:                  ScanCode=AglScanCode::F; break;
    case SDL_SCANCODE_F1:                 ScanCode=AglScanCode::F1; break;
    case SDL_SCANCODE_F10:                ScanCode=AglScanCode::F10; break;
    case SDL_SCANCODE_F11:                ScanCode=AglScanCode::F11; break;
    case SDL_SCANCODE_F12:                ScanCode=AglScanCode::F12; break;
    case SDL_SCANCODE_F13:                ScanCode=AglScanCode::F13; break;
    case SDL_SCANCODE_F14:                ScanCode=AglScanCode::F14; break;
    case SDL_SCANCODE_F15:                ScanCode=AglScanCode::F15; break;
    case SDL_SCANCODE_F16:                ScanCode=AglScanCode::F16; break;
    case SDL_SCANCODE_F17:                ScanCode=AglScanCode::F17; break;
    case SDL_SCANCODE_F18:                ScanCode=AglScanCode::F18; break;
    case SDL_SCANCODE_F19:                ScanCode=AglScanCode::F19; break;
    case SDL_SCANCODE_F2:                 ScanCode=AglScanCode::F2; break;
    case SDL_SCANCODE_F20:                ScanCode=AglScanCode::F20; break;
    case SDL_SCANCODE_F21:                ScanCode=AglScanCode::F21; break;
    case SDL_SCANCODE_F22:                ScanCode=AglScanCode::F22; break;
    case SDL_SCANCODE_F23:                ScanCode=AglScanCode::F23; break;
    case SDL_SCANCODE_F24:                ScanCode=AglScanCode::F24; break;
    case SDL_SCANCODE_F3:                 ScanCode=AglScanCode::F3; break;
    case SDL_SCANCODE_F4:                 ScanCode=AglScanCode::F4; break;
    case SDL_SCANCODE_F5:                 ScanCode=AglScanCode::F5; break;
    case SDL_SCANCODE_F6:                 ScanCode=AglScanCode::F6; break;
    case SDL_SCANCODE_F7:                 ScanCode=AglScanCode::F7; break;
    case SDL_SCANCODE_F8:                 ScanCode=AglScanCode::F8; break;
    case SDL_SCANCODE_F9:                 ScanCode=AglScanCode::F9; break;
    case SDL_SCANCODE_FIND:               ScanCode=AglScanCode::Find; break;
    case SDL_SCANCODE_G:                  ScanCode=AglScanCode::G; break;
    case SDL_SCANCODE_GRAVE:              ScanCode=AglScanCode::Grave; break;
    case SDL_SCANCODE_H:                  ScanCode=AglScanCode::H; break;
    case SDL_SCANCODE_HELP:               ScanCode=AglScanCode::Help; break;
    case SDL_SCANCODE_HOME:               ScanCode=AglScanCode::Home; break;
    case SDL_SCANCODE_I:                  ScanCode=AglScanCode::I; break;
    case SDL_SCANCODE_INSERT:             ScanCode=AglScanCode::Insert; break;
    case SDL_SCANCODE_J:                  ScanCode=AglScanCode::J; break;
    case SDL_SCANCODE_K:                  ScanCode=AglScanCode::K; break;
    case SDL_SCANCODE_KBDILLUMDOWN:       ScanCode=AglScanCode::Kbdillumdown; break;
    case SDL_SCANCODE_KBDILLUMTOGGLE:     ScanCode=AglScanCode::Kbdillumtoggle; break;
    case SDL_SCANCODE_KBDILLUMUP:         ScanCode=AglScanCode::Kbdillumup; break;
    case SDL_SCANCODE_KP_0:               ScanCode=AglScanCode::Kp_0; break;
    case SDL_SCANCODE_KP_00:              ScanCode=AglScanCode::Kp_00; break;
    case SDL_SCANCODE_KP_000:             ScanCode=AglScanCode::Kp_000; break;
    case SDL_SCANCODE_KP_1:               ScanCode=AglScanCode::Kp_1; break;
    case SDL_SCANCODE_KP_2:               ScanCode=AglScanCode::Kp_2; break;
    case SDL_SCANCODE_KP_3:               ScanCode=AglScanCode::Kp_3; break;
    case SDL_SCANCODE_KP_4:               ScanCode=AglScanCode::Kp_4; break;
    case SDL_SCANCODE_KP_5:               ScanCode=AglScanCode::Kp_5; break;
    case SDL_SCANCODE_KP_6:               ScanCode=AglScanCode::Kp_6; break;
    case SDL_SCANCODE_KP_7:               ScanCode=AglScanCode::Kp_7; break;
    case SDL_SCANCODE_KP_8:               ScanCode=AglScanCode::Kp_8; break;
    case SDL_SCANCODE_KP_9:               ScanCode=AglScanCode::Kp_9; break;
    case SDL_SCANCODE_KP_A:               ScanCode=AglScanCode::Kp_a; break;
    case SDL_SCANCODE_KP_AMPERSAND:       ScanCode=AglScanCode::Kp_ampersand; break;
    case SDL_SCANCODE_KP_AT:              ScanCode=AglScanCode::Kp_at; break;
    case SDL_SCANCODE_KP_B:               ScanCode=AglScanCode::Kp_b; break;
    case SDL_SCANCODE_KP_BACKSPACE:       ScanCode=AglScanCode::Kp_backspace; break;
    case SDL_SCANCODE_KP_BINARY:          ScanCode=AglScanCode::Kp_binary; break;
    case SDL_SCANCODE_KP_C:               ScanCode=AglScanCode::Kp_c; break;
    case SDL_SCANCODE_KP_CLEAR:           ScanCode=AglScanCode::Kp_clear; break;
    case SDL_SCANCODE_KP_CLEARENTRY:      ScanCode=AglScanCode::Kp_clearentry; break;
    case SDL_SCANCODE_KP_COLON:           ScanCode=AglScanCode::Kp_colon; break;
    case SDL_SCANCODE_KP_COMMA:           ScanCode=AglScanCode::Kp_comma; break;
    case SDL_SCANCODE_KP_D:               ScanCode=AglScanCode::Kp_d; break;
    case SDL_SCANCODE_KP_DBLAMPERSAND:    ScanCode=AglScanCode::Kp_dblampersand; break;
    case SDL_SCANCODE_KP_DBLVERTICALBAR:  ScanCode=AglScanCode::Kp_dblverticalbar; break;
    case SDL_SCANCODE_KP_DECIMAL:         ScanCode=AglScanCode::Kp_decimal; break;
    case SDL_SCANCODE_KP_DIVIDE:          ScanCode=AglScanCode::Kp_divide; break;
    case SDL_SCANCODE_KP_E:               ScanCode=AglScanCode::Kp_e; break;
    case SDL_SCANCODE_KP_ENTER:           ScanCode=AglScanCode::Kp_enter; break;
    case SDL_SCANCODE_KP_EQUALS:          ScanCode=AglScanCode::Kp_equals; break;
    case SDL_SCANCODE_KP_EQUALSAS400:     ScanCode=AglScanCode::Kp_equalsas400; break;
    case SDL_SCANCODE_KP_EXCLAM:          ScanCode=AglScanCode::Kp_exclam; break;
    case SDL_SCANCODE_KP_F:               ScanCode=AglScanCode::Kp_f; break;
    case SDL_SCANCODE_KP_GREATER:         ScanCode=AglScanCode::Kp_greater; break;
    case SDL_SCANCODE_KP_HASH:            ScanCode=AglScanCode::Kp_hash; break;
    case SDL_SCANCODE_KP_HEXADECIMAL:     ScanCode=AglScanCode::Kp_hexadecimal; break;
    case SDL_SCANCODE_KP_LEFTBRACE:       ScanCode=AglScanCode::Kp_leftbrace; break;
    case SDL_SCANCODE_KP_LEFTPAREN:       ScanCode=AglScanCode::Kp_leftparen; break;
    case SDL_SCANCODE_KP_LESS:            ScanCode=AglScanCode::Kp_less; break;
    case SDL_SCANCODE_KP_MEMADD:          ScanCode=AglScanCode::Kp_memadd; break;
    case SDL_SCANCODE_KP_MEMCLEAR:        ScanCode=AglScanCode::Kp_memclear; break;
    case SDL_SCANCODE_KP_MEMDIVIDE:       ScanCode=AglScanCode::Kp_memdivide; break;
    case SDL_SCANCODE_KP_MEMMULTIPLY:     ScanCode=AglScanCode::Kp_memmultiply; break;
    case SDL_SCANCODE_KP_MEMRECALL:       ScanCode=AglScanCode::Kp_memrecall; break;
    case SDL_SCANCODE_KP_MEMSTORE:        ScanCode=AglScanCode::Kp_memstore; break;
    case SDL_SCANCODE_KP_MEMSUBTRACT:     ScanCode=AglScanCode::Kp_memsubtract; break;
    case SDL_SCANCODE_KP_MINUS:           ScanCode=AglScanCode::Kp_minus; break;
    case SDL_SCANCODE_KP_MULTIPLY:        ScanCode=AglScanCode::Kp_multiply; break;
    case SDL_SCANCODE_KP_OCTAL:           ScanCode=AglScanCode::Kp_octal; break;
    case SDL_SCANCODE_KP_PERCENT:         ScanCode=AglScanCode::Kp_percent; break;
    case SDL_SCANCODE_KP_PERIOD:          ScanCode=AglScanCode::Kp_period; break;
    case SDL_SCANCODE_KP_PLUS:            ScanCode=AglScanCode::Kp_plus; break;
    case SDL_SCANCODE_KP_PLUSMINUS:       ScanCode=AglScanCode::Kp_plusminus; break;
    case SDL_SCANCODE_KP_POWER:           ScanCode=AglScanCode::Kp_power; break;
    case SDL_SCANCODE_KP_RIGHTBRACE:      ScanCode=AglScanCode::Kp_rightbrace; break;
    case SDL_SCANCODE_KP_RIGHTPAREN:      ScanCode=AglScanCode::Kp_rightparen; break;
    case SDL_SCANCODE_KP_SPACE:           ScanCode=AglScanCode::Kp_space; break;
    case SDL_SCANCODE_KP_TAB:             ScanCode=AglScanCode::Kp_tab; break;
    case SDL_SCANCODE_KP_VERTICALBAR:     ScanCode=AglScanCode::Kp_verticalbar; break;
    case SDL_SCANCODE_KP_XOR:             ScanCode=AglScanCode::Kp_xor; break;
    case SDL_SCANCODE_L:                  ScanCode=AglScanCode::L; break;
    case SDL_SCANCODE_LALT:               ScanCode=AglScanCode::Lalt; break;
    case SDL_SCANCODE_LCTRL:              ScanCode=AglScanCode::Lctrl; break;
    case SDL_SCANCODE_LEFT:               ScanCode=AglScanCode::Left; break;
    case SDL_SCANCODE_LEFTBRACKET:        ScanCode=AglScanCode::Leftbracket; break;
    case SDL_SCANCODE_LGUI:               ScanCode=AglScanCode::Lgui; break;
    case SDL_SCANCODE_LSHIFT:             ScanCode=AglScanCode::Lshift; break;
    case SDL_SCANCODE_M:                  ScanCode=AglScanCode::M; break;
    case SDL_SCANCODE_MAIL:               ScanCode=AglScanCode::Mail; break;
    case SDL_SCANCODE_MEDIASELECT:        ScanCode=AglScanCode::Mediaselect; break;
    case SDL_SCANCODE_MENU:               ScanCode=AglScanCode::Menu; break;
    case SDL_SCANCODE_MINUS:              ScanCode=AglScanCode::Minus; break;
    case SDL_SCANCODE_MODE:               ScanCode=AglScanCode::Mode; break;
    case SDL_SCANCODE_MUTE:               ScanCode=AglScanCode::Mute; break;
    case SDL_SCANCODE_N:                  ScanCode=AglScanCode::N; break;
    case SDL_SCANCODE_NUMLOCKCLEAR:       ScanCode=AglScanCode::Numlock; break;
    case SDL_SCANCODE_O:                  ScanCode=AglScanCode::O; break;
    case SDL_SCANCODE_OPER:               ScanCode=AglScanCode::Opr; break;
    case SDL_SCANCODE_OUT:                ScanCode=AglScanCode::Out; break;
    case SDL_SCANCODE_P:                  ScanCode=AglScanCode::P; break;
    case SDL_SCANCODE_PAGEDOWN:           ScanCode=AglScanCode::Pagedown; break;
    case SDL_SCANCODE_PAGEUP:             ScanCode=AglScanCode::Pageup; break;
    case SDL_SCANCODE_PASTE:              ScanCode=AglScanCode::Paste; break;
    case SDL_SCANCODE_PAUSE:              ScanCode=AglScanCode::Pause; break;
    case SDL_SCANCODE_PERIOD:             ScanCode=AglScanCode::Period; break;
    case SDL_SCANCODE_POWER:              ScanCode=AglScanCode::Power; break;
    case SDL_SCANCODE_PRINTSCREEN:        ScanCode=AglScanCode::Printscreen; break;
    case SDL_SCANCODE_PRIOR:              ScanCode=AglScanCode::Prior; break;
    case SDL_SCANCODE_Q:                  ScanCode=AglScanCode::Q; break;
    case SDL_SCANCODE_R:                  ScanCode=AglScanCode::R; break;
    case SDL_SCANCODE_RALT:               ScanCode=AglScanCode::Ralt; break;
    case SDL_SCANCODE_RCTRL:              ScanCode=AglScanCode::Rctrl; break;
    case SDL_SCANCODE_RETURN:             ScanCode=AglScanCode::Return; break;
    case SDL_SCANCODE_RETURN2:            ScanCode=AglScanCode::Return2; break;
    case SDL_SCANCODE_RGUI:               ScanCode=AglScanCode::Rgui; break;
    case SDL_SCANCODE_RIGHT:              ScanCode=AglScanCode::Right; break;
    case SDL_SCANCODE_RIGHTBRACKET:       ScanCode=AglScanCode::Rightbracket; break;
    case SDL_SCANCODE_RSHIFT:             ScanCode=AglScanCode::Rshift; break;
    case SDL_SCANCODE_S:                  ScanCode=AglScanCode::S; break;
    case SDL_SCANCODE_SCROLLLOCK:         ScanCode=AglScanCode::Scrolllock; break;
    case SDL_SCANCODE_SELECT:             ScanCode=AglScanCode::Select; break;
    case SDL_SCANCODE_SEMICOLON:          ScanCode=AglScanCode::Semicolon; break;
    case SDL_SCANCODE_SEPARATOR:          ScanCode=AglScanCode::Separator; break;
    case SDL_SCANCODE_SLASH:              ScanCode=AglScanCode::Slash; break;
    case SDL_SCANCODE_SLEEP:              ScanCode=AglScanCode::Sleep; break;
    case SDL_SCANCODE_SPACE:              ScanCode=AglScanCode::Space; break;
    case SDL_SCANCODE_STOP:               ScanCode=AglScanCode::Stop; break;
    case SDL_SCANCODE_SYSREQ:             ScanCode=AglScanCode::Sysreq; break;
    case SDL_SCANCODE_T:                  ScanCode=AglScanCode::T; break;
    case SDL_SCANCODE_TAB:                ScanCode=AglScanCode::Tab; break;
    case SDL_SCANCODE_THOUSANDSSEPARATOR: ScanCode=AglScanCode::Thousandsseparator; break;
    case SDL_SCANCODE_U:                  ScanCode=AglScanCode::U; break;
    case SDL_SCANCODE_UNDO:               ScanCode=AglScanCode::Undo; break;
    case SDL_SCANCODE_UNKNOWN:            ScanCode=AglScanCode::Unknown; break;
    case SDL_SCANCODE_UP:                 ScanCode=AglScanCode::Up; break;
    case SDL_SCANCODE_V:                  ScanCode=AglScanCode::V; break;
    case SDL_SCANCODE_VOLUMEDOWN:         ScanCode=AglScanCode::Volumedown; break;
    case SDL_SCANCODE_VOLUMEUP:           ScanCode=AglScanCode::Volumeup; break;
    case SDL_SCANCODE_W:                  ScanCode=AglScanCode::W; break;
    case SDL_SCANCODE_WWW:                ScanCode=AglScanCode::Www; break;
    case SDL_SCANCODE_X:                  ScanCode=AglScanCode::X; break;
    case SDL_SCANCODE_Y:                  ScanCode=AglScanCode::Y; break;
    case SDL_SCANCODE_Z:                  ScanCode=AglScanCode::Z; break;
    case SDL_SCANCODE_INTERNATIONAL1:     ScanCode=AglScanCode::International1; break;
    case SDL_SCANCODE_INTERNATIONAL2:     ScanCode=AglScanCode::International2; break;
    case SDL_SCANCODE_INTERNATIONAL3:     ScanCode=AglScanCode::International3; break;
    case SDL_SCANCODE_INTERNATIONAL4:     ScanCode=AglScanCode::International4; break;
    case SDL_SCANCODE_INTERNATIONAL5:     ScanCode=AglScanCode::International5; break;
    case SDL_SCANCODE_INTERNATIONAL6:     ScanCode=AglScanCode::International6; break;
    case SDL_SCANCODE_INTERNATIONAL7:     ScanCode=AglScanCode::International7; break;
    case SDL_SCANCODE_INTERNATIONAL8:     ScanCode=AglScanCode::International8; break;
    case SDL_SCANCODE_INTERNATIONAL9:     ScanCode=AglScanCode::International9; break;
    case SDL_SCANCODE_LANG1:              ScanCode=AglScanCode::Lang1; break;
    case SDL_SCANCODE_LANG2:              ScanCode=AglScanCode::Lang2; break;
    case SDL_SCANCODE_LANG3:              ScanCode=AglScanCode::Lang3; break;
    case SDL_SCANCODE_LANG4:              ScanCode=AglScanCode::Lang4; break;
    case SDL_SCANCODE_LANG5:              ScanCode=AglScanCode::Lang5; break;
    case SDL_SCANCODE_LANG6:              ScanCode=AglScanCode::Lang6; break;
    case SDL_SCANCODE_LANG7:              ScanCode=AglScanCode::Lang7; break;
    case SDL_SCANCODE_LANG8:              ScanCode=AglScanCode::Lang8; break;
    case SDL_SCANCODE_LANG9:              ScanCode=AglScanCode::Lang9; break;
    case SDL_SCANCODE_NONUSHASH:          ScanCode=AglScanCode::Nonushash; break;
    default:                              ScanCode=AglScanCode::NotImplemented; break;
  }
  return ScanCode;
}

//Convert Utf-8 sequence of bytes to integer
cint _Utf8SequenceToInt(char * Utf8){
  cint Code;
  switch(String(Utf8).Length()){
    case 1:  Code=(unsigned char)Utf8[0]; break;
    case 2:  Code=256*(unsigned char)Utf8[0]+(unsigned char)Utf8[1]; break;
    case 3:  Code=256*256*(unsigned char)Utf8[0]+256*(unsigned char)Utf8[1]+(unsigned char)Utf8[2]; break;
    case 4:  Code=256*256*256*(unsigned char)Utf8[0]+256*256*(unsigned char)Utf8[1]+256*(unsigned char)Utf8[2]+(unsigned char)Utf8[3]; break;
    default: Code=0xFF000000; break; //This should not occur as utf-8 has 4 bytes maximun, therefore we output 0xFF000000 as 0xFF is invalid in utf-8 as start byte
  }
 return Code;
}