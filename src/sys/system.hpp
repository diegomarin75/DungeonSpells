//system.hpp

//System wide definitions /functions
#ifndef _SYSTEM_HPP
#define _SYSTEM_HPP

//Maximun arguments per instruction
const int _MaxInstructionArgs=4; 

//Debug levels
extern int64_t _DebugLevels;

//Instruction table structure
struct InstTable{
  String Mnemonic;
  int ArgNr;
  int Length;
  CpuDataType Type[_MaxInstructionArgs];
  CpuAdrMode AdrMode[_MaxInstructionArgs];
  int Offset[_MaxInstructionArgs];
};

//Instruction table
extern const InstTable _Inst[_InstructionNr];
    
//Exception numbers
enum class SysExceptionCode:int{
  RuntimeBaseException=0,
  SystemPanic,
  ConsoleLockedByOtherProcess,
  NullReferenceIndirection,
  DivideByZero,
  InvalidInstructionCode,
  InvalidArrayBlock,
  InvalidStringBlock,
  StringAllocationError,
  NullStringAllocationError,
  InvalidCallStringMid,
  InvalidCallStringRight,
  InvalidCallStringLeft,
  InvalidCallStringCutRight,
  InvalidCallStringCutLeft,
  InvalidCallStringLJust,
  InvalidCallStringRJust,
  InvalidCallStringFind,
  InvalidCallStringReplicate,
  InvalidStringIndex,
  CharToStringConvFailure,
  ShortToStringConvFailure,
  IntegerToStringConvFailure,
  LongToStringConvFailure,
  StringToBooleanConvFailure,
  StringToCharConvFailure,
  StringToShortConvFailure,
  StringToIntegerConvFailure,
  StringToLongConvFailure,
  StringToFloatConvFailure,
  FloatToCharConvFailure,
  FloatToShortConvFailure,
  FloatToIntegerConvFailure,
  FloatToLongConvFailure,
  FloatToStringConvFailure,
  CharStringFormatFailure,
  ShortStringFormatFailure,
  IntegerStringFormatFailure,
  LongStringFormatFailure,
  FloatStringFormatFailure,
  InvalidMemoryAddress,
  MemoryAllocationFailure,
  ArrayIndexAllocationFailure,
  ArrayBlockAllocationFailure,
  CallStackUnderflow,
  StackOverflow,
  StackUnderflow,
  InvalidArrayDimension,
  InvalidDimensionSize,
  ArrayWrongDimension,
  ArrayIndexingOutOfBounds,
  ReplicationRuleNegative,
  ReplicationRuleInconsistent,
  InitializationRuleNegative,
  InitializationRuleInconsistent,
  SubroutineMaxNestingLevelReached,
  ArrayGeometryMemoryOverflow,
  ArrayGeometryInvalidIndex,
  ArrayGeometryInvalidDimension,
  ArrayGeometryIndexingOutOfBounds,
  FromCharArrayIncorrectLength,
  WriteCharArrayIncorrectLength,
  MissingDynamicLibrary,
  MissingLibraryFunc,
  DynLibArchMissmatch,
  DynLibInit1Failed,
  DynLibInit2Failed,
  UserLibraryCopyFailed,
  StaInvalidReadIndex,
  StaOperationAlreadyOpen,
  StaOperationAlreadyClosed,
  StaOperationNotOpen,
  CannotCreatePipe,
  UnableToCreateProcess,
  ErrorWaitingChild,
  UnableToCheckFdStatus,
  FdStatusError,
  ReadError,
  InvalidDate,
  InvalidTime
};

//Exception record
class SysExceptionRecord{
  private:
    void _Move(const SysExceptionRecord& Excp);
  public:
    SysExceptionCode Code;
    String Message;
    int ProcessId;
    SysExceptionRecord(){}
    ~SysExceptionRecord(){}  
    SysExceptionRecord(const SysExceptionRecord& Excp);
    SysExceptionRecord& operator=(const SysExceptionRecord& Excp);
};

//System core functions
class System{
  
  //Private members
  private:

    //Private fields
    static Stack<int> _GlobalProcessId;
    static Array<SysExceptionRecord> _ExceptionTable;
    static bool _ExceptionFlag;

    //Constructor
    System();

  //Public members
  public:
    
    //Public functions
    static void PushProcessId(int ProcessId);
    static void PopProcessId();
    static int CurrentProcessId();
    static long ExceptionCount();
    inline static bool ExceptionFlag(){ return _ExceptionFlag; }
    static void ExceptionReset();
    static bool ExceptionRead(int Index,SysExceptionRecord& Excp);
    static void Throw(SysExceptionCode Code,const String& Parm1="",const String& Parm2="",const String& Parm3="",const String& Parm4="",const String& Parm5="");
    static void ExceptionPrint();

    //Math exception check functions
    static bool inline IsChrAddSafe(CpuChr a,CpuChr b){ return (b==MIN_CHR?(a< 0?false:true):(b<0?(MIN_CHR-b>a?false:true):(MAX_CHR-b<a?false:true))); }
    static bool inline IsChrSubSafe(CpuChr a,CpuChr b){ return (b==MIN_CHR?(a>-1?false:true):(b>0?(MAX_CHR+b<a?false:true):(MIN_CHR+b>a?false:true))); }
    static bool inline IsChrMulSafe(CpuChr a,CpuChr b){ return (a==0||b==0?true:(std::abs(a)>MAX_CHR/std::abs(b)?false:true)); }
    static bool inline IsChrDivSafe(CpuChr a,CpuChr b){ return (b==0?false:true); }
    static bool inline IsShrAddSafe(CpuShr a,CpuShr b){ return (b==MIN_SHR?(a< 0?false:true):(b<0?(MIN_SHR-b>a?false:true):(MAX_SHR-b<a?false:true))); }
    static bool inline IsShrSubSafe(CpuShr a,CpuShr b){ return (b==MIN_SHR?(a>-1?false:true):(b>0?(MAX_SHR+b<a?false:true):(MIN_SHR+b>a?false:true))); }
    static bool inline IsShrMulSafe(CpuShr a,CpuShr b){ return (a==0||b==0?true:(std::abs(a)>MAX_SHR/std::abs(b)?false:true)); }
    static bool inline IsShrDivSafe(CpuShr a,CpuShr b){ return (b==0?false:true); }
    static bool inline IsIntAddSafe(CpuInt a,CpuInt b){ return (b==MIN_INT?(a< 0?false:true):(b<0?(MIN_INT-b>a?false:true):(MAX_INT-b<a?false:true))); }
    static bool inline IsIntSubSafe(CpuInt a,CpuInt b){ return (b==MIN_INT?(a>-1?false:true):(b>0?(MAX_INT+b<a?false:true):(MIN_INT+b>a?false:true))); }
    static bool inline IsIntMulSafe(CpuInt a,CpuInt b){ return (a==0||b==0?true:(std::abs(a)>MAX_INT/std::abs(b)?false:true)); }
    static bool inline IsIntDivSafe(CpuInt a,CpuInt b){ return (b==0?false:true); }
    static bool inline IsLonAddSafe(CpuLon a,CpuLon b){ return (b==MIN_LON?(a< 0?false:true):(b<0?(MIN_LON-b>a?false:true):(MAX_LON-b<a?false:true))); }
    static bool inline IsLonSubSafe(CpuLon a,CpuLon b){ return (b==MIN_LON?(a>-1?false:true):(b>0?(MAX_LON+b<a?false:true):(MIN_LON+b>a?false:true))); }
    static bool inline IsLonMulSafe(CpuLon a,CpuLon b){ return (a==0||b==0?true:(std::abs(a)>MAX_LON/std::abs(b)?false:true)); }
    static bool inline IsLonDivSafe(CpuLon a,CpuLon b){ return (b==0?false:true); }
    static void inline ClearFloException(){ std::feclearexcept(FE_ALL_EXCEPT); }
    static bool inline CheckFloException(){ return std::fetestexcept(FE_ALL_EXCEPT)==0?true:false; }
    static String GetLastFloException();
    static String ExceptionName(SysExceptionCode Code);
    static String GetDbgSymDebugStr(const DbgSymModule& Mod);
    static String GetDbgSymDebugStr(const DbgSymType& Typ);
    static String GetDbgSymDebugStr(const DbgSymVariable& Var);
    static String GetDbgSymDebugStr(const DbgSymField& Fld);
    static String GetDbgSymDebugStr(const DbgSymFunction& Fun);
    static String GetDbgSymDebugStr(const DbgSymParameter& Par);
    static String GetDbgSymDebugStr(const DbgSymLine& Lin);
};

//Debug message macro
#ifdef __DEV__
  #define DebugLevelEnabled(level) (((0x0000000000000001LL<<(int)level)&_DebugLevels)!=0)
  #define DebugOpen(file,ext)      DebugMessages::OpenLog(file,ext)
  #define DebugClose()             DebugMessages::CloseLog()
  #define DebugAppend(level,text)  if(DebugLevelEnabled(level)){ DebugMessages::Append(text); }
  #define DebugMessage(level,text) if(DebugLevelEnabled(level)){ DebugMessages::Message(level,text); }
  #define DebugOutput(text)        DebugMessages::Output(text)
  #define DebugException(text)     if(!DebugMessages::GetConsoleOutput()){ DebugMessages::Output(text); }
#else
  #define DebugLevelEnabled(level) (false)
  #define DebugOpen(file,ext) 
  #define DebugClose()
  #define DebugAppend(level,text)
  #define DebugMessage(level,text)
  #define DebugOutput(text)
  #define DebugException(text)
#endif

//Debug levels
#define DEBUG_LEVELS 35
enum class DebugLevel:int{
  SysAllocator     =0,
  SysAllocControl  =1,
  SysDynLib        =2,
  VrmMemPool       =3,
  VrmMemPoolCheck  =4,
  VrmMemory        =5,
  VrmAuxMemory     =6,
  VrmAuxMemStatus  =7,
  VrmRuntime       =8,
  VrmInnerBlockRpl =9,
  VrmInnerBlockInit=10,
  VrmExceptions    =11,
  CmpReadLine      =12,
  CmpInsLine       =13,
  CmpSplitLine     =14,
  CmpParser        =15,
  CmpComplexLit    =16,
  CmpMasterData    =17,
  CmpRelocation    =18,
  CmpBinary        =19,
  CmpLibrary       =20,
  CmpLnkSymbol     =21,
  CmpDbgSymbol     =22,
  CmpIndex         =23,
  CmpCodeBlock     =24,
  CmpJump          =25,
  CmpFwdCall       =26,
  CmpLitValRepl    =27,
  CmpMergeCode     =28,
  CmpScopeStk      =29,
  CmpInnerBlockRpl =30,
  CmpInnerBlockInit=31,
  CmpExpression    =32,
  CmpAssembler     =33,
  CmpInit          =34
};

//Debug messages
class DebugMessages{
  
  //Private members
  private:
    static std::ofstream _Log;
    static bool _ToConsole;
    static String _Append;

  //Public members
  public:
    static bool OpenLog(const String& OrigFile,const String& LogExt);
    static void CloseLog();
    static void SetConsoleOutput();
    static bool GetConsoleOutput();
    static void SetLevel(DebugLevel Level);
    static String LevelText(DebugLevel Level);
    static char Level2Id(DebugLevel Level);
    static DebugLevel Id2Level(char Id);
    static bool IsCompilerEnabled(DebugLevel Level);
    static bool IsRuntimeEnabled(DebugLevel Level);
    static bool IsGloballyEnabled(DebugLevel Level);
    static void Append(const String& Message);
    static void Message(DebugLevel Level,const String& Message);
    static void Output(const String& Message);
    static void ShowEnabledLevels();
    DebugMessages();

};

#endif