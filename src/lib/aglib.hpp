//aglib.hpp: Audio & Graphics library
#ifndef _AGLIB_HPP
#define _AGLIB_HPP

//Null utf8 sequence (this sequence of bytes cannot happen in utf-8 as 0xFF cannot happen as start byte)
#define NULLUTF8 0xFF000000

//Devices (listed here only he ones that need initialization and shutdown)
#define AGL_DEVICES 4
enum class AglDevice:cint{
  Graphics=0,
  Image,
  Event,
  Audio
};
 
//Display flags
enum class AglDisplayFlag:cint{
  Native=1,        //Opens display in native full screen mode
  FullScreen=2,    //Opens display in full screen mode without currnt changing resolution
  BorderLess=4,    //Display without borders
  Centered=8,      //Opens centered display
  Located=16,      //Display location is undefined and set by host system
  Accelerated=32,  //Force display HW acceleration
  Software=64,     //Use software rendering
  RetraceSync=128, //Display presentation synced with screen vertical retrace
  Resizable=256    //Window can be resizable
};

//Error codes
enum class AglErrorCode:cint{
  Ok=0,
  InitError,
  InitDeviceError,
  InvalidDispHnd,
  UnusedDispHnd,
  ForbiddenDispHnd,
  DisplayOpenError,
  SetLogSizeError,
  SetViewPortError,
  SetClipAreaError,
  ClearError,
  SetColorError,
  GetColorError,
  DrawLineError,
  DrawPointError,
  DrawRectError,
  DrawFillRectError,
  LoadSystemFontError,
  PrintCharError,
  InvalidFont,
  NoCurrentDisp,
  InvalidTxtrHnd,
  UnusedTxtrHnd,
  ForbiddenTxtrHnd,
  LoadTextureError,
  SetBlendModError,
  SetColorModError,
  SetAlphaModError,
  RenderError,
  ExRenderError
};

//Event ids
enum class AglEventId:cint{
  DispShown,        //window has been shown
  DispHidden,       //window has been hidden
  DispExposed,      //window has been exposed and should be redrawn
  DispMoved,        //window has been moved to data1, data2
  DispResized,      //window has been resized to data1xdata2; this event is always preceded by SxDL_WINDOWEVENT_SIZE_CHANGED
  DispSizeChanged,  //window size has changed, either as a result of an API call or through the system or user changing the window size; this event is followed by SxDL_WINDOWEVENT_RESIZED if the size was changed by an external event, i.e. the user or the window manager
  DispMinimized,    //window has been minimized
  DispMaximized,    //window has been maximized
  DispRestored,     //window has been restored to normal size and position
  DispEnter,        //window has gained mouse focus
  DispLeave,        //window has lost mouse focus
  DispFocusGained,  //window has gained keyboard focus
  DispFocusLost,    //window has lost keyboard focus
  DispClose,        //the window manager requests that the window be closed
  DispTakeFocus,    //window is being offered a focus (should SDL_SetWindowInputFocus() on itself or a subwindow, or ignore) (>= SDL 2.0.5) 
  KeybPressed,      //Key is pressed
  KeybReleased,     //Key is released
  KeybInput,        //Keyboard key input (translated to utf-8)
  NotImplemented    //Not implemented events fall here
};

//Keyboard modifiers
enum class AglKeyModif:cint{
  LShift=1,
  RShift=2,
  LCtrl =4,
  RCtrl =8,
  LAlt  =16,
  RAlt  =32,
  LGui  =64,
  RGui  =128,
  Num   =256,
  Caps  =1024,
  Mode  =2048  
};

//Keyboard scan codes
enum class AglScanCode:cint{
  Num_0=0, Num_1, Num_2, Num_3, Num_4, Num_5, Num_6, Num_7, Num_8, Num_9, A, Ac_back, Ac_bookmarks, Ac_forward, Ac_home, Ac_refresh, 
  Ac_search, Ac_stop, Again, Alterase, Apostrophe, Application, Audiomute, Audionext, Audioplay, Audioprev, Audiostop, B, Backslash, 
  Backspace, Brightnessdown, Brightnessup, C, Calculator, Cancel, Capslock, Clear, Clearagain, Comma, Computer, Copy, Crsel, 
  Currencysubunit, Currencyunit, Cut, D, Decimalseparator, Delete, Displayswitch, Down, E, Eject, End, Equals, Escape, Execute, 
  Exsel, F, F1, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F2, F20, F21, F22, F23, F24, F3, F4, F5, F6, F7, F8, F9, Find, 
  G, Grave, H, Help, Home, I, Insert, J, K, Kbdillumdown, Kbdillumtoggle, Kbdillumup, Kp_0, Kp_00, Kp_000, Kp_1, Kp_2, Kp_3, Kp_4, 
  Kp_5, Kp_6, Kp_7, Kp_8, Kp_9, Kp_a, Kp_ampersand, Kp_at, Kp_b, Kp_backspace, Kp_binary, Kp_c, Kp_clear, Kp_clearentry, Kp_colon, 
  Kp_comma, Kp_d, Kp_dblampersand, Kp_dblverticalbar, Kp_decimal, Kp_divide, Kp_e, Kp_enter, Kp_equals, Kp_equalsas400, Kp_exclam, 
  Kp_f, Kp_greater, Kp_hash, Kp_hexadecimal, Kp_leftbrace, Kp_leftparen, Kp_less, Kp_memadd, Kp_memclear, Kp_memdivide, Kp_memmultiply, 
  Kp_memrecall, Kp_memstore, Kp_memsubtract, Kp_minus, Kp_multiply, Kp_octal, Kp_percent, Kp_period, Kp_plus, Kp_plusminus, Kp_power, 
  Kp_rightbrace, Kp_rightparen, Kp_space, Kp_tab, Kp_verticalbar, Kp_xor, L, Lalt, Lctrl, Left, Leftbracket, Lgui, Lshift, M, Mail, 
  Mediaselect, Menu, Minus, Mode, Mute, N, Numlock, O, Opr, Out, P, Pagedown, Pageup, Paste, Pause, Period, Power, Printscreen, 
  Prior, Q, R, Ralt, Rctrl, Return, Return2, Rgui, Right, Rightbracket, Rshift, S, Scrolllock, Select, Semicolon, Separator, Slash, 
  Sleep, Space, Stop, Sysreq, T, Tab, Thousandsseparator, U, Undo, Unknown, Up, V, Volumedown, Volumeup, W, Www, X, Y, Z, International1, 
  International2, International3, International4, International5, International6, International7, International8, International9, Lang1, 
  Lang2, Lang3, Lang4, Lang5, Lang6, Lang7, Lang8, Lang9, Nonushash, NotImplemented
};
    
//Flip mode
enum class AglFlipMode:cint{
  None=0,     //None
  Horizontal, //Horizontal flipping
  Vertical    //Vertical flipping
};

//Blend mode
enum class AglBlendMode:cint{
  None=0, //None
  Blend,  //Alpha blending
  Add,    //Additive blending
  Mod     //Color modulation blending
};

//Fonts
enum class AglSystemFont:cint{
  Mono10=0,
  Mono12,
  Mono14,
  Mono16,
  Mono18,
  Vga,
  Casio
};

//Font info
#define FONTNAMELEN 255
struct AglFontInfo{
  char CodeName[FONTNAMELEN+1];
  char FullName[FONTNAMELEN+1];
  cint Size;
  bool MonoSpace;
  cint Width;
  cint Height;
  cint MinChar;
  cint MaxChar;
};

//Display info
#define DISPTITLELEN 255
struct AglDispInfo{
  char Title[DISPTITLELEN+1];
  cint Left;
  cint Top;
  cint Width;
  cint Height;
};

//Event data structure
struct AglEventInfo{
  AglEventId Id;
  cint DispHandler;
  cint DispX;
  cint DispY;
  cint KeyModif;
  AglScanCode ScanCode;
  cint Utf8Code;
};

//Function prototypes device control & info
CALLDECL bool Init(cint ProcessId,char **Error);
CALLDECL char *GetVersionString();
CALLDECL AglErrorCode Error();
CALLDECL char *ErrorText(AglErrorCode Code);
CALLDECL char *LastError();

//Function prototypes for praphics
CALLDECL bool DisplayOpen(const char *Title,cint x,cint y,cint width,cint height,cint Flags,cint *DispHnd);
CALLDECL bool DisplayClose(cint DispHnd);
CALLDECL bool GetDisplayInfo(cint DispHnd,AglDispInfo *Info);
CALLDECL bool SetCurrentDisplay(cint DispHnd);
CALLDECL cint GetCurrentDisplay();
CALLDECL void SetLogicalSize(cint width,cint height);
CALLDECL void SetViewPort(cint x,cint y,cint width,cint height);
CALLDECL void SetClipArea(cint x,cint y,cint width,cint height);
CALLDECL void DisplayShow();
CALLDECL void Clear();
CALLDECL void SetColor(char r,char g,char b,char a);
CALLDECL void SetPoint(cint x,cint y);
CALLDECL void DrawLine(cint x1,cint y1,cint x2,cint y2);
CALLDECL void DrawRectangle(cint x1,cint y1,cint x2,cint y2);
CALLDECL void DrawFillRectangle(cint x1,cint y1,cint x2,cint y2);
CALLDECL void DrawCircle(cint x,cint y,cint r);
CALLDECL void DrawFillCircle(cint x,cint y,cint r);
CALLDECL void SetFont(AglSystemFont Font,cint ScaleX,cint ScaleY);
CALLDECL void SetFontForeColor(char r,char g,char b,char a);
CALLDECL void SetFontBackColor(char r,char g,char b,char a);
CALLDECL AglSystemFont GetCurrentFont();
CALLDECL cint GetCurrentFontScaleX();
CALLDECL cint GetCurrentFontScaleY();
CALLDECL bool GetFontInfo(AglSystemFont Font,AglFontInfo *Info);
CALLDECL cint GetFontLines(AglSystemFont Font);
CALLDECL cint GetFontColumns(AglSystemFont Font);
CALLDECL void SetFontOffset(cint offx,cint offy);
CALLDECL void PrintChar(cint cx,cint cy,cint Chr);
CALLDECL void Print(cint cx,cint cy,const char *Text);
CALLDECL void PrintCharXY(cint x,cint y,cint Chr);
CALLDECL void PrintXY(cint x,cint y,const char *Text);
CALLDECL bool LoadTexture(const char *FileName,cint *TxtrHnd);
CALLDECL bool FreeTexture(cint TxtrHnd);
CALLDECL void GetTextureSize(cint TxtrHnd,cint *Width,cint *Height);
CALLDECL void SetTextureRotation(cint TxtrHnd,double Angle,cint Rotx,cint Roty);
CALLDECL void SetTextureFlipMode(cint TxtrHnd,AglFlipMode Flip);
CALLDECL void SetTextureBlendMode(cint TxtrHnd,AglBlendMode Blend);
CALLDECL void SetTextureColorModulation(cint TxtrHnd,char r,char g,char b);
CALLDECL void SetTextureAlphaModulation(cint TxtrHnd,char a);
CALLDECL void RenderTexture(cint TxtrHnd,cint x,cint y);
CALLDECL void RenderTextureScaled(cint TxtrHnd,cint x,cint y,cint ax,cint ay);
CALLDECL void RenderTexturePart(cint TxtrHnd,cint x,cint y,cint tx,cint ty,cint tax,cint tay);
CALLDECL void RenderTexturePartScaled(cint TxtrHnd,cint x,cint y,cint ax,cint ay,cint tx,cint ty,cint tax,cint tay);

//Function prototypes events
CALLDECL void WaitEvent(cint Timeout);
CALLDECL bool HasEvents();
CALLDECL void GetEvent(AglEventInfo *Event);

#endif