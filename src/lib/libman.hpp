//libman.hpp: Library manager
#ifndef _LIBMAN_HPP
#define _LIBMAN_HPP

//Call specifications
#ifdef __WIN__
  #define CALLDECL extern "C" __stdcall __declspec(dllexport)
  #define CALLDEFN extern "C" __stdcall
#else
  #define CALLDECL extern "C"
  #define CALLDEFN extern "C"
#endif

//Definitions that depend on language (C vs C++)
#ifdef __cplusplus
  #include <cstdint>
  #include <cctype>
  #include <cstdio>
  #include <cstring>
  #include <cstdlib>
  #define __NULLPT nullptr
#else
  #define __NULLPT NULL
  #include <stdbool.h>
#endif

//Debug message macro
#define DEBUG_MSG_LEN 255
#ifdef __DEV__
#define LibDebugMsgDecl static char Message[DEBUG_MSG_LEN+1];
#define LibDebugMsg(m) DebugMsg(m)
#else
#define LibDebugMsgDecl
#define LibDebugMsg(m)
#endif

//Dispatcher commands
#define __DISPATCHERINIT      0
#define __DISPATCHERFUNSEARCH 1
#define __DISPATCHERTYPSEARCH 2
#define __DISPATCHERGETDEF    3
#define __DISPATCHERCALL      4
#define __DISPATCHERCLOSE     5

//Stringization / Concatenation macros (inner level is required to replace before actual operation)
#define INNERSTRIN(x)    #x
#define STRIN(x)         INNERSTRIN(x)
#define INNERCONCAT(x,y) x##y
#define CONCAT(x,y)      INNERCONCAT(x,y)

//Standard data types (must be used on external library functions)
typedef int16_t cshort; //16-bit signed integer
typedef int32_t cint;   //32-bit signed integer
typedef int64_t clong;  //64-bit signed integer

//Array types
#define DARRAY_TYPENAME __darray__
#define FARRAY_TYPENAME __farray__
#define CARRAY_TYPENAME __carray__
typedef struct CONCAT(__struct,DARRAY_TYPENAME) { void *ptr; clong *len; } DARRAY_TYPE;
typedef struct CONCAT(__struct,FARRAY_TYPENAME) { void *ptr; clong len; } FARRAY_TYPE;
typedef struct CONCAT(__struct,CARRAY_TYPENAME) { const void *ptr; clong len; } CARRAY_TYPE;

//Array type names
#define DARRAY(x)         CONCAT(DARRAY_TYPENAME,x)
#define FARRAY(x,n)       CONCAT(CONCAT(FARRAY_TYPENAME,x),n)
#define CDARRAY(x)        CONCAT(CARRAY_TYPENAME,x)
#define CFARRAY(x,n)      CONCAT(CONCAT(CARRAY_TYPENAME,x),n)

//Array type definitions
#define DARDEF(x)         typedef DARRAY_TYPE DARRAY(x)
#define FARDEF(x,n)       typedef FARRAY_TYPE FARRAY(x,n)
#define CDARDEF(x)        typedef CARRAY_TYPE CDARRAY(x)
#define CFARDEF(x,n)      typedef CARRAY_TYPE CFARRAY(x,n)

//Constants
#define MAXPARMS   10 //Maximun function parameters
#define BUILDNRLEN 16 //Build number length

//Function table entry
struct FunctionDef{ const void *Address; const char *Name; int ParmNr; const char *ParmType[MAXPARMS]; };

//Function pointer types to library handling functions
typedef bool (*FunPtrIsSystemLibrary)();
typedef int (*FunPtrLibArchitecture)();
typedef void (*FunPtrGetBuildNumber)(char *);
typedef bool (*FunPtrInitDispatcher)();
typedef bool (*FunPtrSearchType)(char *);
typedef int (*FunPtrSearchFunction)(char *);
typedef bool (*FunPtrGetFunctionDef)(int,FunctionDef *);
typedef bool (*FunPtrCallFunction)(int,void **);
typedef bool (*FunPtrDoProcessChange)(int);
typedef void (*FunPtrSetDbgMsgInterf)(void (*)(char *));
typedef void (*FunPtrCloseDispatcher)();

//Call dispatcher begin (syslib=1 if system library, syslib=0 if user library)
#define FDECLBEGIN(syslib) \
\
/*Function pointers for callback functions*/ \
void (*_OnLibraryClose)()=__NULLPT; /*Function pointer for on library close event (resource deallocation)*/ \
bool (*_OnProcessChange)(int)=__NULLPT; /*Function pointer for on process change event*/ \
void (*_SendDebugMessage)(char *)=__NULLPT; /*Function pointer for debug message interface*/ \
\
/*Library handling functions used by compiler and runtime*/ \
CALLDECL bool _IsSystemLibrary(); \
CALLDECL int _LibArchitecture(); \
CALLDECL void _GetBuildNumber(char *BuildNumber); \
CALLDECL bool _InitDispatcher(); \
CALLDECL bool _SearchType(char *Name); \
CALLDECL int _SearchFunction(char *Name); \
CALLDECL bool _GetFunctionDef(int Id,FunctionDef *Def); \
CALLDECL void _CallFunction(int Id,void *Arg[]); \
CALLDECL bool _DoProcessChange(int ProcessId); \
CALLDECL void _SetDbgMsgInterf(void (*_DebugMsg)(char *)); \
CALLDECL void _CloseDispatcher(); \
\
/*Library functions exposed to user code*/ \
void SetClosingFunction(void (*OnClose)()); \
void SetProcessChangeFunction(bool (*OnProcessChange)(int)); \
void DebugMsg(char *Msg); \
\
/*Internal library functions*/ \
bool _StringSearch(const char *BaseStr,const char *FindStr); \
bool _ResizeFunctionTable(int Size); \
bool _InternalDispatcher(int Command,char *SearchName,int *SearchId,int Id,FunctionDef* Def,void *Arg[]); \
\
/*Get system library flag*/ \
CALLDEFN bool _IsSystemLibrary(){ \
  return ((syslib)==1?1:0); \
} \
\
/*Get library architecture*/ \
CALLDEFN int _LibArchitecture(){ \
  return sizeof(void *)*8; \
} \
\
/*Get build number*/ \
CALLDEFN void _GetBuildNumber(char *BuildNumber){ \
  \
  /*Variables*/ \
  const char *Date=__DATE__; \
  const char *Time=__TIME__; \
  char Month[2+1]; \
  char Day[2+1]; \
  char Year[4+1]; \
  char Hour[2+1]; \
  char Minute[2+1]; \
  char Second[2+1]; \
  \
  /*Get month*/ \
  if(toupper(Date[0])=='J' && toupper(Date[1])=='A' && toupper(Date[2])=='N'){ Month[0]='0'; Month[1]='1'; Month[3]=0; } \
  if(toupper(Date[0])=='F' && toupper(Date[1])=='E' && toupper(Date[2])=='B'){ Month[0]='0'; Month[1]='2'; Month[3]=0; } \
  if(toupper(Date[0])=='M' && toupper(Date[1])=='A' && toupper(Date[2])=='R'){ Month[0]='0'; Month[1]='3'; Month[3]=0; } \
  if(toupper(Date[0])=='A' && toupper(Date[1])=='P' && toupper(Date[2])=='R'){ Month[0]='0'; Month[1]='4'; Month[3]=0; } \
  if(toupper(Date[0])=='M' && toupper(Date[1])=='A' && toupper(Date[2])=='Y'){ Month[0]='0'; Month[1]='5'; Month[3]=0; } \
  if(toupper(Date[0])=='J' && toupper(Date[1])=='U' && toupper(Date[2])=='N'){ Month[0]='0'; Month[1]='6'; Month[3]=0; } \
  if(toupper(Date[0])=='J' && toupper(Date[1])=='U' && toupper(Date[2])=='L'){ Month[0]='0'; Month[1]='7'; Month[3]=0; } \
  if(toupper(Date[0])=='A' && toupper(Date[1])=='U' && toupper(Date[2])=='G'){ Month[0]='0'; Month[1]='8'; Month[3]=0; } \
  if(toupper(Date[0])=='S' && toupper(Date[1])=='E' && toupper(Date[2])=='P'){ Month[0]='0'; Month[1]='9'; Month[3]=0; } \
  if(toupper(Date[0])=='O' && toupper(Date[1])=='C' && toupper(Date[2])=='T'){ Month[0]='1'; Month[1]='0'; Month[3]=0; } \
  if(toupper(Date[0])=='N' && toupper(Date[1])=='O' && toupper(Date[2])=='V'){ Month[0]='1'; Month[1]='1'; Month[3]=0; } \
  if(toupper(Date[0])=='D' && toupper(Date[1])=='E' && toupper(Date[2])=='C'){ Month[0]='1'; Month[1]='2'; Month[3]=0; } \
  \
  /*Get day*/ \
  Day[0]=(Date[4]==' '?'0':Date[4]); \
  Day[1]=Date[5]; \
  Day[2]=0; \
  \
  /*Get year*/ \
  Year[0]=Date[ 7]; \
  Year[1]=Date[ 8]; \
  Year[2]=Date[ 9]; \
  Year[3]=Date[10]; \
  Year[4]=0; \
  \
  /*Get hour*/ \
  Hour[0]=Time[0]; \
  Hour[1]=Time[1]; \
  Hour[2]=0; \
  \
  /*Get minute*/ \
  Minute[0]=Time[3]; \
  Minute[1]=Time[4]; \
  Minute[2]=0; \
  \
  /*Get second*/ \
  Second[0]=Time[6]; \
  Second[1]=Time[7]; \
  Second[2]=0; \
  \
  /*Get build number*/ \
  sprintf(BuildNumber,"%c%s%s%s%s%s%s%i",((syslib)==1?'S':'U'),Year,Month,Day,Hour,Minute,Second,(int)(sizeof(void *)*8)); \
  \
} \
\
/*Set closing function*/ \
void SetClosingFunction(void (*OnClose)()){ \
  _OnLibraryClose=OnClose; \
} \
\
/*Set process change function*/ \
void SetProcessChangeFunction(bool (*OnProcessChange)(int)){ \
  _OnProcessChange=OnProcessChange; \
} \
\
/*Do process change */ \
CALLDEFN bool _DoProcessChange(int ProcessId){ \
  if(_OnProcessChange!=__NULLPT){ \
    return _OnProcessChange(ProcessId); \
  } \
  return false; \
} \
\
/*Set debug message interface*/ \
CALLDEFN void _SetDbgMsgInterf(void (*DebugMsg)(char *)){ \
  _SendDebugMessage=DebugMsg; \
} \
\
/*Send debug message*/ \
void DebugMsg(char *Msg){ \
  if(_SendDebugMessage!=__NULLPT){ \
    _SendDebugMessage(Msg); \
  } \
} \
\
/*Initialize library*/ \
CALLDEFN bool _InitDispatcher(){ \
  return _InternalDispatcher(__DISPATCHERINIT,__NULLPT,__NULLPT,0,__NULLPT,__NULLPT); \
} \
\
/*Data type search*/ \
CALLDEFN bool _SearchType(char *Name){ \
  return _InternalDispatcher(__DISPATCHERTYPSEARCH,Name,__NULLPT,0,__NULLPT,__NULLPT); \
} \
\
/*Function search*/ \
CALLDEFN int _SearchFunction(char *Name){ \
  int FuncId; \
  _InternalDispatcher(__DISPATCHERFUNSEARCH,Name,&FuncId,0,__NULLPT,__NULLPT); \
  return FuncId; \
} \
\
/*Get function metadata*/ \
CALLDEFN bool _GetFunctionDef(int Id,FunctionDef *Def){ \
  if(!_InternalDispatcher(__DISPATCHERGETDEF,__NULLPT,__NULLPT,Id,Def,__NULLPT)){ \
    Def->Address=__NULLPT; \
    Def->Name=__NULLPT; \
    Def->ParmNr=-1; \
  	return false; \
  } \
  return true; \
} \
\
/*Call function*/ \
CALLDEFN void _CallFunction(int Id,void *Arg[]){ \
  _InternalDispatcher(__DISPATCHERCALL,__NULLPT,__NULLPT,Id,__NULLPT,Arg); \
} \
\
/*Close call dispatcher*/ \
CALLDEFN void _CloseDispatcher(void){ \
  _InternalDispatcher(__DISPATCHERCLOSE,__NULLPT,__NULLPT,0,__NULLPT,__NULLPT); \
} \
\
/*String search*/ \
bool _StringSearch(const char *BaseStr,const char *FindStr){ \
  char *Beg=strstr((char *)BaseStr,(char *)FindStr); \
  if(Beg==__NULLPT){ return false; } \
  char *End=Beg+strlen(FindStr)-1; \
  long BaseLen=strlen(BaseStr); \
  if(Beg==&BaseStr[0] && End==&BaseStr[BaseLen-1]){ \
    return true; \
  } \
  if(Beg>&BaseStr[0]){ \
    char PrevChr=*(Beg-1); \
    if((PrevChr>='0' && PrevChr<='9') \
    || (PrevChr>='a' && PrevChr<='z') \
    || (PrevChr>='A' && PrevChr<='Z') \
    || PrevChr=='_'){ \
      return false; \
    } \
  } \
  if(End<&BaseStr[BaseLen-1]){ \
    char NextChr=*(End+1); \
    if((NextChr>='0' && NextChr<='9') \
    || (NextChr>='a' && NextChr<='z') \
    || (NextChr>='A' && NextChr<='Z') \
    || NextChr=='_'){ \
      return false; \
    } \
  } \
  return true; \
} \
\
/*Resize function table*/ \
bool _ResizeFunctionTable(int Size,FunctionDef **Table){ \
  if(*Table==__NULLPT){ \
    if((*Table=(FunctionDef *)malloc(Size*sizeof(FunctionDef)))==__NULLPT){ return false; } \
  } \
  else{ \
    FunctionDef *Tmp; \
    if((Tmp=(FunctionDef *)realloc(*Table,Size*sizeof(FunctionDef)))==__NULLPT){ free(*Table); return false; } \
    *Table=Tmp; \
  } \
  return true; \
} \
\
/*Call dispatcher function begin*/ \
bool _InternalDispatcher(int Command,char *SearchName,int *SearchId,int Id,FunctionDef *Def,void *Arg[]){ \
  static int _FuncNr=0; \
  static FunctionDef *_FuncTable=__NULLPT; \
  LibDebugMsgDecl; \
  if(Command==__DISPATCHERCALL){ \
    LibDebugMsg((snprintf(Message,DEBUG_MSG_LEN+1,"Internal dispatcher: Called function %s()",_FuncTable[Id].Name),Message)); \
    goto *_FuncTable[Id].Address; \
  } \
  else if(Command==__DISPATCHERTYPSEARCH){ \
    LibDebugMsg((snprintf(Message,DEBUG_MSG_LEN+1,"Internal dispatcher: Search type %s",SearchName),Message)); \
    for(int i=0;i<_FuncNr;i++){ \
      if(strcmp(_FuncTable[i].ParmType[0],"void")==0){ \
        for(int j=1;j<=_FuncTable[i].ParmNr;j++){ \
          if(_StringSearch(_FuncTable[i].ParmType[j],SearchName)){ \
            return true; \
          } \
        } \
      } \
      else{ \
        for(int j=0;j<_FuncTable[i].ParmNr;j++){ \
          if(_StringSearch(_FuncTable[i].ParmType[j],SearchName)){ \
            return true; \
          } \
        } \
      } \
    } \
    return false; \
  } \
  else if(Command==__DISPATCHERFUNSEARCH){ \
    LibDebugMsg((snprintf(Message,DEBUG_MSG_LEN+1,"Internal dispatcher: Search function %s",SearchName),Message)); \
    for(int i=0;i<_FuncNr;i++){ \
      if(strcmp(SearchName,_FuncTable[i].Name)==0){ *SearchId=i; return true; } \
    } \
    *SearchId=-1; \
  } \
  else if(Command==__DISPATCHERGETDEF){ \
    LibDebugMsg((snprintf(Message,DEBUG_MSG_LEN+1,"Internal dispatcher: Get function definition for id %i",Id),Message)); \
    if(Id<0 || Id>_FuncNr-1){ return false; } \
    *Def=_FuncTable[Id]; \
    Def->Address=__NULLPT; \
  } \
  else if(Command==__DISPATCHERCLOSE){ \
    LibDebugMsg((snprintf(Message,DEBUG_MSG_LEN+1,"Internal dispatcher: Close"),Message)); \
    if(_OnLibraryClose!=__NULLPT){ _OnLibraryClose(); } \
    if(_FuncTable!=__NULLPT){ free(_FuncTable); _FuncTable=__NULLPT; _FuncNr=0; } \
  } \
  else if(Command==__DISPATCHERINIT){ \
    LibDebugMsg((snprintf(Message,DEBUG_MSG_LEN+1,"Internal dispatcher: Init"),Message)); \

//Call dispatcher end
#define FDECLEND \
/*Call dispatcher function end*/ \
  } \
  CALLEnd: \
  return true; \
} \

//Declare void function with 0 parameters
#define FDECLVO0(fn) \
if(!_ResizeFunctionTable(_FuncNr+1,&_FuncTable)){ return false; } \
_FuncNr++; \
_FuncTable[_FuncNr-1].Name=#fn; \
_FuncTable[_FuncNr-1].Address=&&CALL##fn; \
_FuncTable[_FuncNr-1].ParmNr=0; \
_FuncTable[_FuncNr-1].ParmType[0]="void"; \
goto NEXT##fn; \
CALL##fn: fn(); goto CALLEnd; \
NEXT##fn: \

//Declare void function with 1 parameters
#define FDECLVO1(fn,t1) \
if(!_ResizeFunctionTable(_FuncNr+1,&_FuncTable)){ return false; } \
_FuncNr++; \
_FuncTable[_FuncNr-1].Name=#fn; \
_FuncTable[_FuncNr-1].Address=&&CALL##fn; \
_FuncTable[_FuncNr-1].ParmNr=1; \
_FuncTable[_FuncNr-1].ParmType[0]="void"; \
_FuncTable[_FuncNr-1].ParmType[1]=#t1; \
goto NEXT##fn; \
CALL##fn: fn(*(t1 *)Arg[0]); goto CALLEnd; \
NEXT##fn: \

//Declare void function with 2 parameters
#define FDECLVO2(fn,t1,t2) \
if(!_ResizeFunctionTable(_FuncNr+1,&_FuncTable)){ return false; } \
_FuncNr++; \
_FuncTable[_FuncNr-1].Name=#fn; \
_FuncTable[_FuncNr-1].Address=&&CALL##fn; \
_FuncTable[_FuncNr-1].ParmNr=2; \
_FuncTable[_FuncNr-1].ParmType[0]="void"; \
_FuncTable[_FuncNr-1].ParmType[1]=#t1; \
_FuncTable[_FuncNr-1].ParmType[2]=#t2; \
goto NEXT##fn; \
CALL##fn: fn(*(t1 *)Arg[0],*(t2 *)Arg[1]); goto CALLEnd; \
NEXT##fn: \

//Declare void function with 3 parameters
#define FDECLVO3(fn,t1,t2,t3) \
if(!_ResizeFunctionTable(_FuncNr+1,&_FuncTable)){ return false; } \
_FuncNr++; \
_FuncTable[_FuncNr-1].Name=#fn; \
_FuncTable[_FuncNr-1].Address=&&CALL##fn; \
_FuncTable[_FuncNr-1].ParmNr=3; \
_FuncTable[_FuncNr-1].ParmType[0]="void"; \
_FuncTable[_FuncNr-1].ParmType[1]=#t1; \
_FuncTable[_FuncNr-1].ParmType[2]=#t2; \
_FuncTable[_FuncNr-1].ParmType[3]=#t3; \
goto NEXT##fn; \
CALL##fn: fn(*(t1 *)Arg[0],*(t2 *)Arg[1],*(t3 *)Arg[2]); goto CALLEnd; \
NEXT##fn: 

//Declare void function with 4 parameters
#define FDECLVO4(fn,t1,t2,t3,t4) \
if(!_ResizeFunctionTable(_FuncNr+1,&_FuncTable)){ return false; } \
_FuncNr++; \
_FuncTable[_FuncNr-1].Name=#fn; \
_FuncTable[_FuncNr-1].Address=&&CALL##fn; \
_FuncTable[_FuncNr-1].ParmNr=4; \
_FuncTable[_FuncNr-1].ParmType[0]="void"; \
_FuncTable[_FuncNr-1].ParmType[1]=#t1; \
_FuncTable[_FuncNr-1].ParmType[2]=#t2; \
_FuncTable[_FuncNr-1].ParmType[3]=#t3; \
_FuncTable[_FuncNr-1].ParmType[4]=#t4; \
goto NEXT##fn; \
CALL##fn: fn(*(t1 *)Arg[0],*(t2 *)Arg[1],*(t3 *)Arg[2],*(t4 *)Arg[3]); goto CALLEnd; \
NEXT##fn: \

//Declare void function with 5 parameters
#define FDECLVO5(fn,t1,t2,t3,t4,t5) \
if(!_ResizeFunctionTable(_FuncNr+1,&_FuncTable)){ return false; } \
_FuncNr++; \
_FuncTable[_FuncNr-1].Name=#fn; \
_FuncTable[_FuncNr-1].Address=&&CALL##fn; \
_FuncTable[_FuncNr-1].ParmNr=5; \
_FuncTable[_FuncNr-1].ParmType[0]="void"; \
_FuncTable[_FuncNr-1].ParmType[1]=#t1; \
_FuncTable[_FuncNr-1].ParmType[2]=#t2; \
_FuncTable[_FuncNr-1].ParmType[3]=#t3; \
_FuncTable[_FuncNr-1].ParmType[4]=#t4; \
_FuncTable[_FuncNr-1].ParmType[5]=#t5; \
goto NEXT##fn; \
CALL##fn: fn(*(t1 *)Arg[0],*(t2 *)Arg[1],*(t3 *)Arg[2],*(t4 *)Arg[3],*(t5 *)Arg[4]); goto CALLEnd; \
NEXT##fn: \

//Declare void function with 6 parameters
#define FDECLVO6(fn,t1,t2,t3,t4,t5,t6) \
if(!_ResizeFunctionTable(_FuncNr+1,&_FuncTable)){ return false; } \
_FuncNr++; \
_FuncTable[_FuncNr-1].Name=#fn; \
_FuncTable[_FuncNr-1].Address=&&CALL##fn; \
_FuncTable[_FuncNr-1].ParmNr=6; \
_FuncTable[_FuncNr-1].ParmType[0]="void"; \
_FuncTable[_FuncNr-1].ParmType[1]=#t1; \
_FuncTable[_FuncNr-1].ParmType[2]=#t2; \
_FuncTable[_FuncNr-1].ParmType[3]=#t3; \
_FuncTable[_FuncNr-1].ParmType[4]=#t4; \
_FuncTable[_FuncNr-1].ParmType[5]=#t5; \
_FuncTable[_FuncNr-1].ParmType[6]=#t6; \
goto NEXT##fn; \
CALL##fn: fn(*(t1 *)Arg[0],*(t2 *)Arg[1],*(t3 *)Arg[2],*(t4 *)Arg[3],*(t5 *)Arg[4],*(t6 *)Arg[5]); goto CALLEnd; \
NEXT##fn: \

//Declare void function with 7 parameters
#define FDECLVO7(fn,t1,t2,t3,t4,t5,t6,t7) \
if(!_ResizeFunctionTable(_FuncNr+1,&_FuncTable)){ return false; } \
_FuncNr++; \
_FuncTable[_FuncNr-1].Name=#fn; \
_FuncTable[_FuncNr-1].Address=&&CALL##fn; \
_FuncTable[_FuncNr-1].ParmNr=7; \
_FuncTable[_FuncNr-1].ParmType[0]="void"; \
_FuncTable[_FuncNr-1].ParmType[1]=#t1; \
_FuncTable[_FuncNr-1].ParmType[2]=#t2; \
_FuncTable[_FuncNr-1].ParmType[3]=#t3; \
_FuncTable[_FuncNr-1].ParmType[4]=#t4; \
_FuncTable[_FuncNr-1].ParmType[5]=#t5; \
_FuncTable[_FuncNr-1].ParmType[6]=#t6; \
_FuncTable[_FuncNr-1].ParmType[7]=#t7; \
goto NEXT##fn; \
CALL##fn: fn(*(t1 *)Arg[0],*(t2 *)Arg[1],*(t3 *)Arg[2],*(t4 *)Arg[3],*(t5 *)Arg[4],*(t6 *)Arg[5],*(t7 *)Arg[6]); goto CALLEnd; \
NEXT##fn: \

//Declare void function with 8 parameters
#define FDECLVO8(fn,t1,t2,t3,t4,t5,t6,t7,t8) \
if(!_ResizeFunctionTable(_FuncNr+1,&_FuncTable)){ return false; } \
_FuncNr++; \
_FuncTable[_FuncNr-1].Name=#fn; \
_FuncTable[_FuncNr-1].Address=&&CALL##fn; \
_FuncTable[_FuncNr-1].ParmNr=8; \
_FuncTable[_FuncNr-1].ParmType[0]="void"; \
_FuncTable[_FuncNr-1].ParmType[1]=#t1; \
_FuncTable[_FuncNr-1].ParmType[2]=#t2; \
_FuncTable[_FuncNr-1].ParmType[3]=#t3; \
_FuncTable[_FuncNr-1].ParmType[4]=#t4; \
_FuncTable[_FuncNr-1].ParmType[5]=#t5; \
_FuncTable[_FuncNr-1].ParmType[6]=#t6; \
_FuncTable[_FuncNr-1].ParmType[7]=#t7; \
_FuncTable[_FuncNr-1].ParmType[8]=#t8; \
goto NEXT##fn; \
CALL##fn: fn(*(t1 *)Arg[0],*(t2 *)Arg[1],*(t3 *)Arg[2],*(t4 *)Arg[3],*(t5 *)Arg[4],*(t6 *)Arg[5],*(t7 *)Arg[6],*(t8 *)Arg[7]); goto CALLEnd; \
NEXT##fn: \

//Declare void function with 9 parameters
#define FDECLVO9(fn,t1,t2,t3,t4,t5,t6,t7,t8,t9) \
if(!_ResizeFunctionTable(_FuncNr+1,&_FuncTable)){ return false; } \
_FuncNr++; \
_FuncTable[_FuncNr-1].Name=#fn; \
_FuncTable[_FuncNr-1].Address=&&CALL##fn; \
_FuncTable[_FuncNr-1].ParmNr=9; \
_FuncTable[_FuncNr-1].ParmType[0]="void"; \
_FuncTable[_FuncNr-1].ParmType[1]=#t1; \
_FuncTable[_FuncNr-1].ParmType[2]=#t2; \
_FuncTable[_FuncNr-1].ParmType[3]=#t3; \
_FuncTable[_FuncNr-1].ParmType[4]=#t4; \
_FuncTable[_FuncNr-1].ParmType[5]=#t5; \
_FuncTable[_FuncNr-1].ParmType[6]=#t6; \
_FuncTable[_FuncNr-1].ParmType[7]=#t7; \
_FuncTable[_FuncNr-1].ParmType[8]=#t8; \
_FuncTable[_FuncNr-1].ParmType[9]=#t9; \
goto NEXT##fn; \
CALL##fn: fn(*(t1 *)Arg[0],*(t2 *)Arg[1],*(t3 *)Arg[2],*(t4 *)Arg[3],*(t5 *)Arg[4],*(t6 *)Arg[5],*(t7 *)Arg[6],*(t8 *)Arg[7],*(t9 *)Arg[8]); goto CALLEnd; \
NEXT##fn: \

//Declare void function with 10 parameters
#define FDECLVO10(fn,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10) \
if(!_ResizeFunctionTable(_FuncNr+1,&_FuncTable)){ return false; } \
_FuncNr++; \
_FuncTable[_FuncNr-1].Name=#fn; \
_FuncTable[_FuncNr-1].Address=&&CALL##fn; \
_FuncTable[_FuncNr-1].ParmNr=1; \
_FuncTable[_FuncNr-1].ParmType[0]="void"; \
_FuncTable[_FuncNr-1].ParmType[1]=#t1; \
_FuncTable[_FuncNr-1].ParmType[2]=#t2; \
_FuncTable[_FuncNr-1].ParmType[3]=#t3; \
_FuncTable[_FuncNr-1].ParmType[4]=#t4; \
_FuncTable[_FuncNr-1].ParmType[5]=#t5; \
_FuncTable[_FuncNr-1].ParmType[6]=#t6; \
_FuncTable[_FuncNr-1].ParmType[7]=#t7; \
_FuncTable[_FuncNr-1].ParmType[8]=#t8; \
_FuncTable[_FuncNr-1].ParmType[9]=#t9; \
_FuncTable[_FuncNr-1].ParmType[10]=#t10; \
goto NEXT##fn; \
CALL##fn: fn(*(t1 *)Arg[0],*(t2 *)Arg[1],*(t3 *)Arg[2],*(t4 *)Arg[3],*(t5 *)Arg[4],*(t6 *)Arg[5],*(t7 *)Arg[6],*(t8 *)Arg[7],*(t9 *)Arg[8],*(t10 *)Arg[9]); goto CALLEnd; \
NEXT##fn: \

//Declare returning function with 0 parameters
#define FDECLRS0(rt,fn) \
if(!_ResizeFunctionTable(_FuncNr+1,&_FuncTable)){ return false; } \
_FuncNr++; \
_FuncTable[_FuncNr-1].Name=#fn; \
_FuncTable[_FuncNr-1].Address=&&CALL##fn; \
_FuncTable[_FuncNr-1].ParmNr=1; \
_FuncTable[_FuncNr-1].ParmType[0]=#rt; \
goto NEXT##fn; \
CALL##fn: *(rt *)(*(void **)Arg[0])=fn(); goto CALLEnd; \
NEXT##fn: \

//Declare returning function with 1 parameters
#define FDECLRS1(rt,fn,t1) \
if(!_ResizeFunctionTable(_FuncNr+1,&_FuncTable)){ return false; } \
_FuncNr++; \
_FuncTable[_FuncNr-1].Name=#fn; \
_FuncTable[_FuncNr-1].Address=&&CALL##fn; \
_FuncTable[_FuncNr-1].ParmNr=2; \
_FuncTable[_FuncNr-1].ParmType[0]=#rt; \
_FuncTable[_FuncNr-1].ParmType[1]=#t1; \
goto NEXT##fn; \
CALL##fn: *(rt *)(*(void **)Arg[0])=fn(*(t1 *)Arg[1]); goto CALLEnd; \
NEXT##fn: \

//Declare returning function with 2 parameters
#define FDECLRS2(rt,fn,t1,t2) \
if(!_ResizeFunctionTable(_FuncNr+1,&_FuncTable)){ return false; } \
_FuncNr++; \
_FuncTable[_FuncNr-1].Name=#fn; \
_FuncTable[_FuncNr-1].Address=&&CALL##fn; \
_FuncTable[_FuncNr-1].ParmNr=3; \
_FuncTable[_FuncNr-1].ParmType[0]=#rt; \
_FuncTable[_FuncNr-1].ParmType[1]=#t1; \
_FuncTable[_FuncNr-1].ParmType[2]=#t2; \
goto NEXT##fn; \
CALL##fn: *(rt *)(*(void **)Arg[0])=fn(*(t1 *)Arg[1],*(t2 *)Arg[2]); goto CALLEnd; \
NEXT##fn: \

//Declare returning function with 3 parameters
#define FDECLRS3(rt,fn,t1,t2,t3) \
if(!_ResizeFunctionTable(_FuncNr+1,&_FuncTable)){ return false; } \
_FuncNr++; \
_FuncTable[_FuncNr-1].Name=#fn; \
_FuncTable[_FuncNr-1].Address=&&CALL##fn; \
_FuncTable[_FuncNr-1].ParmNr=4; \
_FuncTable[_FuncNr-1].ParmType[0]=#rt; \
_FuncTable[_FuncNr-1].ParmType[1]=#t1; \
_FuncTable[_FuncNr-1].ParmType[2]=#t2; \
_FuncTable[_FuncNr-1].ParmType[3]=#t3; \
goto NEXT##fn; \
CALL##fn: *(rt *)(*(void **)Arg[0])=fn(*(t1 *)Arg[1],*(t2 *)Arg[2],*(t3 *)Arg[3]); goto CALLEnd; \
NEXT##fn: \

//Declare returning function with 4 parameters
#define FDECLRS4(rt,fn,t1,t2,t3,t4) \
if(!_ResizeFunctionTable(_FuncNr+1,&_FuncTable)){ return false; } \
_FuncNr++; \
_FuncTable[_FuncNr-1].Name=#fn; \
_FuncTable[_FuncNr-1].Address=&&CALL##fn; \
_FuncTable[_FuncNr-1].ParmNr=5; \
_FuncTable[_FuncNr-1].ParmType[0]=#rt; \
_FuncTable[_FuncNr-1].ParmType[1]=#t1; \
_FuncTable[_FuncNr-1].ParmType[2]=#t2; \
_FuncTable[_FuncNr-1].ParmType[3]=#t3; \
_FuncTable[_FuncNr-1].ParmType[4]=#t4; \
goto NEXT##fn; \
CALL##fn: *(rt *)(*(void **)Arg[0])=fn(*(t1 *)Arg[1],*(t2 *)Arg[2],*(t3 *)Arg[3],*(t4 *)Arg[4]); goto CALLEnd; \
NEXT##fn: \

//Declare returning function with 5 parameters
#define FDECLRS5(rt,fn,t1,t2,t3,t4,t5) \
if(!_ResizeFunctionTable(_FuncNr+1,&_FuncTable)){ return false; } \
_FuncNr++; \
_FuncTable[_FuncNr-1].Name=#fn; \
_FuncTable[_FuncNr-1].Address=&&CALL##fn; \
_FuncTable[_FuncNr-1].ParmNr=6; \
_FuncTable[_FuncNr-1].ParmType[0]=#rt; \
_FuncTable[_FuncNr-1].ParmType[1]=#t1; \
_FuncTable[_FuncNr-1].ParmType[2]=#t2; \
_FuncTable[_FuncNr-1].ParmType[3]=#t3; \
_FuncTable[_FuncNr-1].ParmType[4]=#t4; \
_FuncTable[_FuncNr-1].ParmType[5]=#t5; \
goto NEXT##fn; \
CALL##fn: *(rt *)(*(void **)Arg[0])=fn(*(t1 *)Arg[1],*(t2 *)Arg[2],*(t3 *)Arg[3],*(t4 *)Arg[4],*(t5 *)Arg[5]); goto CALLEnd; \
NEXT##fn: \

//Declare returning function with 6 parameters
#define FDECLRS6(rt,fn,t1,t2,t3,t4,t5,t6) \
if(!_ResizeFunctionTable(_FuncNr+1,&_FuncTable)){ return false; } \
_FuncNr++; \
_FuncTable[_FuncNr-1].Name=#fn; \
_FuncTable[_FuncNr-1].Address=&&CALL##fn; \
_FuncTable[_FuncNr-1].ParmNr=7; \
_FuncTable[_FuncNr-1].ParmType[0]=#rt; \
_FuncTable[_FuncNr-1].ParmType[1]=#t1; \
_FuncTable[_FuncNr-1].ParmType[2]=#t2; \
_FuncTable[_FuncNr-1].ParmType[3]=#t3; \
_FuncTable[_FuncNr-1].ParmType[4]=#t4; \
_FuncTable[_FuncNr-1].ParmType[5]=#t5; \
_FuncTable[_FuncNr-1].ParmType[6]=#t6; \
goto NEXT##fn; \
CALL##fn: *(rt *)(*(void **)Arg[0])=fn(*(t1 *)Arg[1],*(t2 *)Arg[2],*(t3 *)Arg[3],*(t4 *)Arg[4],*(t5 *)Arg[5],*(t6 *)Arg[6]); goto CALLEnd; \
NEXT##fn: \

//Declare returning function with 7 parameters
#define FDECLRS7(rt,fn,t1,t2,t3,t4,t5,t6,t7) \
if(!_ResizeFunctionTable(_FuncNr+1,&_FuncTable)){ return false; } \
_FuncNr++; \
_FuncTable[_FuncNr-1].Name=#fn; \
_FuncTable[_FuncNr-1].Address=&&CALL##fn; \
_FuncTable[_FuncNr-1].ParmNr=8; \
_FuncTable[_FuncNr-1].ParmType[0]=#rt; \
_FuncTable[_FuncNr-1].ParmType[1]=#t1; \
_FuncTable[_FuncNr-1].ParmType[2]=#t2; \
_FuncTable[_FuncNr-1].ParmType[3]=#t3; \
_FuncTable[_FuncNr-1].ParmType[4]=#t4; \
_FuncTable[_FuncNr-1].ParmType[5]=#t5; \
_FuncTable[_FuncNr-1].ParmType[6]=#t6; \
_FuncTable[_FuncNr-1].ParmType[7]=#t7; \
goto NEXT##fn; \
CALL##fn: *(rt *)(*(void **)Arg[0])=fn(*(t1 *)Arg[1],*(t2 *)Arg[2],*(t3 *)Arg[3],*(t4 *)Arg[4],*(t5 *)Arg[5],*(t6 *)Arg[6],*(t7 *)Arg[7]); goto CALLEnd; \
NEXT##fn: \

//Declare returning function with 8 parameters
#define FDECLRS8(rt,fn,t1,t2,t3,t4,t5,t6,t7,t8) \
if(!_ResizeFunctionTable(_FuncNr+1,&_FuncTable)){ return false; } \
_FuncNr++; \
_FuncTable[_FuncNr-1].Name=#fn; \
_FuncTable[_FuncNr-1].Address=&&CALL##fn; \
_FuncTable[_FuncNr-1].ParmNr=9; \
_FuncTable[_FuncNr-1].ParmType[0]=#rt; \
_FuncTable[_FuncNr-1].ParmType[1]=#t1; \
_FuncTable[_FuncNr-1].ParmType[2]=#t2; \
_FuncTable[_FuncNr-1].ParmType[3]=#t3; \
_FuncTable[_FuncNr-1].ParmType[4]=#t4; \
_FuncTable[_FuncNr-1].ParmType[5]=#t5; \
_FuncTable[_FuncNr-1].ParmType[6]=#t6; \
_FuncTable[_FuncNr-1].ParmType[7]=#t7; \
_FuncTable[_FuncNr-1].ParmType[8]=#t8; \
goto NEXT##fn; \
CALL##fn: *(rt *)(*(void **)Arg[0])=fn(*(t1 *)Arg[1],*(t2 *)Arg[2],*(t3 *)Arg[3],*(t4 *)Arg[4],*(t5 *)Arg[5],*(t6 *)Arg[6],*(t7 *)Arg[7],*(t8 *)Arg[8]); goto CALLEnd; \
NEXT##fn: \

//Declare returning function with 9 parameters
#define FDECLRS9(rt,fn,t1,t2,t3,t4,t5,t6,t7,t8,t9) \
if(!_ResizeFunctionTable(_FuncNr+1,&_FuncTable)){ return false; } \
_FuncNr++; \
_FuncTable[_FuncNr-1].Name=#fn; \
_FuncTable[_FuncNr-1].Address=&&CALL##fn; \
_FuncTable[_FuncNr-1].ParmNr=10; \
_FuncTable[_FuncNr-1].ParmType[0]=#rt; \
_FuncTable[_FuncNr-1].ParmType[1]=#t1; \
_FuncTable[_FuncNr-1].ParmType[2]=#t2; \
_FuncTable[_FuncNr-1].ParmType[3]=#t3; \
_FuncTable[_FuncNr-1].ParmType[4]=#t4; \
_FuncTable[_FuncNr-1].ParmType[5]=#t5; \
_FuncTable[_FuncNr-1].ParmType[6]=#t6; \
_FuncTable[_FuncNr-1].ParmType[7]=#t7; \
_FuncTable[_FuncNr-1].ParmType[8]=#t8; \
_FuncTable[_FuncNr-1].ParmType[9]=#t9; \
goto NEXT##fn; \
CALL##fn: *(rt *)(*(void **)Arg[0])=fn(*(t1 *)Arg[1],*(t2 *)Arg[2],*(t3 *)Arg[3],*(t4 *)Arg[4],*(t5 *)Arg[5],*(t6 *)Arg[6],*(t7 *)Arg[7],*(t8 *)Arg[8],*(t9 *)Arg[9]); goto CALLEnd; \
NEXT##fn: \

//Declare returning function with 10 parameters
#define FDECLRS10(rt,fn,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10) \
if(!_ResizeFunctionTable(_FuncNr+1,&_FuncTable)){ return false; } \
_FuncNr++; \
_FuncTable[_FuncNr-1].Name=#fn; \
_FuncTable[_FuncNr-1].Address=&&CALL##fn; \
_FuncTable[_FuncNr-1].ParmNr=11; \
_FuncTable[_FuncNr-1].ParmType[0]=#rt; \
_FuncTable[_FuncNr-1].ParmType[1]=#t1; \
_FuncTable[_FuncNr-1].ParmType[2]=#t2; \
_FuncTable[_FuncNr-1].ParmType[3]=#t3; \
_FuncTable[_FuncNr-1].ParmType[4]=#t4; \
_FuncTable[_FuncNr-1].ParmType[5]=#t5; \
_FuncTable[_FuncNr-1].ParmType[6]=#t6; \
_FuncTable[_FuncNr-1].ParmType[7]=#t7; \
_FuncTable[_FuncNr-1].ParmType[8]=#t8; \
_FuncTable[_FuncNr-1].ParmType[9]=#t9; \
_FuncTable[_FuncNr-1].ParmType[10]=#t10; \
goto NEXT##fn; \
CALL##fn: *(rt *)(*(void **)Arg[0])=fn(*(t1 *)Arg[1],*(t2 *)Arg[2],*(t3 *)Arg[3],*(t4 *)Arg[4],*(t5 *)Arg[5],*(t6 *)Arg[6],*(t7 *)Arg[7],*(t8 *)Arg[8],*(t9 *)Arg[9],*(t10 *)Arg[10]); goto CALLEnd; \
NEXT##fn: \

#endif