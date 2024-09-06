//runtime.hpp: CPU runtime

//Wrap include
#ifndef _RUNTIME_HPP
#define _RUNTIME_HPP

//Include library manager
#include "lib/libman.hpp"
 
//Call stack (outside because it is used outsise
struct CallStack{
  CpuAdr OrgAddress;    //Original address (on which CALL instr. happens)
  CpuAdr RetAddress;    //Return address
  CpuAdr BasePointer;   //Current base stack pointer
  CpuLon StackSize;     //Current stack size
  CpuLon ScopeNr;       //Current scope nr
  CpuAgx AFBasePointer; //Fixed geometries base pointer
};
    
//Runtime class
class Runtime{  
  
  //Private members
  private:
  
    //Replication rule type
    enum ReplicRuleMode{ Fixed,Dynamic };

    //Inner block replication rules (for fixed and dyn arrays)
    struct ReplicRule{
      ReplicRuleMode Mode; //Replication mode (Fixed / Dynamic arrays)
      CpuWrd BaseOffset;   //BaseOffset: Offset at which array is found
      CpuAgx ArrGeom;      //Fixed array geometry
      CpuWrd CellSize;     //Array cell size
      CpuWrd Elements;     //Total array elements
      CpuWrd Index;        //Index: working element index used by replication algorithm
      CpuMbl SourceBlock;  //SourceBlock: working source block used by replication algorithm
      CpuMbl DestinBlock;  //DestinBlock: working destination block used by replication algorithm
      char *SourcePnt;     //Source pointer: uleworking source pointer used by replication algorithm
      char *DestinPnt;     //Destin pointer: uleworking destination pointer used by replication algorithm
      bool End;            //Rule processing ended
    };
    
    //Inner block initialization rules (for fixed and dyn arrays)
    struct InitRule{
      CpuWrd BaseOffset;   //BaseOffset: Offset at which fixed array is found
      CpuAgx ArrGeom;      //Fixed array geometry
      CpuWrd CellSize;     //Array cell size
      CpuWrd Elements;     //Total array elements
      CpuWrd Index;        //Index: working element index used by initialization algorithm
      CpuMbl DestinBlock;  //DestinBlock: working destination block used by initialization algorithm
      char *DestinPnt;     //Destin pointer: uleworking destination pointer used by initialization algorithm
      bool End;            //Rule processing ended
    };
    
    //Addresing mode decoder table
    struct AddrModeDecoderTable{
      CpuAdrMode AdrMode1;
      CpuAdrMode AdrMode2;
      CpuAdrMode AdrMode3;
      CpuAdrMode AdrMode4;
      CpuAdrMode AdrMode5;
    };

    //Instruction timming table
    struct InstTimmingTable{
      double CumulatedTime;
      CpuLon ExecutionCount;
      CpuLon NotTimedCount;
      int Order;
      int Rank;
      double Measured;
    };

    //Dynamic library table
    struct DynLibDef{
      CpuChr DlName[_MaxIdLen+1];
      void *Handler;
      FunPtrSearchFunction SearchFunction;
      FunPtrCallFunction CallFunction;
      FunPtrSetDbgMsgInterf SetDbgMsgInterf;
      FunPtrCloseDispatcher CloseDispatcher;
    };
    
    //Dynamic library function table
    struct DynFunDef{
      CpuChr DlFunction[_MaxIdLen+1];
      bool IsVoid;
      int LibIndex;
      int PhyFunId;
    };

    //Dyamic parameter definition
    struct DlParmDef{
      bool Update;
      bool IsString;
      bool IsDynArray;
      void *VoidPtr;
      CpuMbl *Blk;
      char *CharPtr;
      char **CharPtr2;
      clong ArrLen;
      DARRAY_TYPE DArray;
      FARRAY_TYPE FArray;
      CARRAY_TYPE CArray;
    };
    
    //Dynamic library function pointers
    struct DlFuncPtr{
      FunPtrIsSystemLibrary IsSystemLibrary;
      FunPtrLibArchitecture LibArchitecture;
      FunPtrGetBuildNumber GetBuildNumber;
      FunPtrInitDispatcher InitDispatcher;
      FunPtrSearchFunction SearchFunction;
      FunPtrCallFunction CallFunction;
      FunPtrSetDbgMsgInterf SetDbgMsgInterf;
      FunPtrCloseDispatcher CloseDispatcher;
    };

    //Struct Debug symbol tables
    struct DbgSymTables{
      RamBuffer<DbgSymModule> Mod;    //Modules
      RamBuffer<DbgSymType> Typ;      //Types
      RamBuffer<DbgSymVariable> Var;  //Variables
      RamBuffer<DbgSymField> Fld;     //Fields
      RamBuffer<DbgSymFunction> Fun;  //Functions
      RamBuffer<DbgSymParameter> Par; //Parameters
      RamBuffer<DbgSymLine> Lin;      //Source code lines
    };

    //Disassembled lines
    struct DisAsmLine{
      CpuAdr IP;
      String Hex;
      String Asm;
      String Symbols;
      String SourceInfo;
    };

    //Program data
    int _ProcessId;                   //Process Id
    int _ScopeId;                     //Current ScopeId of top function in the call stack (starts in 1, value 0 is reserved for unitialized BlockIds): Corresponds to call stack depth
    CpuLon _ScopeNr;                  //Current ScopeNr of top function in the call stack (starts in 1, value 0 is reserved for unitialized BlockIds): Corresponds a CALL counter
    CpuBol _ScopeUnlock;              //Unlocks changes in machine scope variables _ScopeId, _ScopeNr
    RamBuffer<char> _Glob;            //Global memory buffer
    RamBuffer<char> _Stack;           //Stack memory buffer
    RamBuffer<char> _Code;            //Code memory buffer
    RamBuffer<char> _ParmSt;          //Parameter stack
    RamBuffer<CallStack> _CallSt;     //Call stack
    RamBuffer<DlParmDef> _DlParm;     //Parameter definition (only for dynamic library calls)
    RamBuffer<void *> _DlVPtr;        //Parameter pointers (only for dynamic library calls)
    RamBuffer<ReplicRule> _RpRule;    //Inner block replication rules
    RamBuffer<InitRule> _BiRule;       //Inner block initialization rules
    RamBuffer<DynLibDef> _DynLib;     //Dynamic libraries (only for dynamic library calls)
    RamBuffer<DynFunDef> _DynFun;     //Dynamic functions (only for dynamic library calls)
    char *_RpSource;                  //Inner block replication rule source
    char *_RpDestin;                  //Inner block replication rule destination
    char *_BiDestin;                  //Inner block initialization rule destination
    AuxMemoryManager _Aux;            //Aux memory control record (strings and arrays stored here)
    StringComputer _StC;              //String computer instance
    ArrayComputer _ArC;               //Array computer instance
    char _ProgName[_MaxIdLen+1];      //Program name
    char _DynLibPath[FILEPATHLEN+1];  //Path for dynamic libraries
    char _TmpLibPath[FILEPATHLEN+1];  //Path for storinguser libraries
    int _ArgNr;                       //Total number of command line arguments passed
    int _ArgStart;                    //Option at which application options start
    char **_Arg;                      //Program command line arguments passed
    RomFileBuffer *_RomBuffer;        //Pointer to rombuffer
    CpuLon _RomBuffReadNr;            //Rom buffer read counter
    DbgSymTables _DebugSym;           //Debug symbol tables

    //Benchmark variables
    CpuLon _InstCount;                                             //Instruction execution counter
    Array<InstTimmingTable> _Timming;                              //Instruction timming table
    std::chrono::time_point<std::chrono::steady_clock> _ProgStart; //Program start time
    std::chrono::time_point<std::chrono::steady_clock> _ProgEnd;   //Program end time

    //Private functions
    int _GetLibraryId(char *DlName);
    bool _OpenExecutable(const String& FileName,int& Hnd);
    bool _CloseExecutable(const String& FileName,int Hnd);
    bool _ReadExecutable(const String& FileName,int Hnd,const char *Part,int Index,int SubIndex,Buffer& Buff,long Length);
    bool _ReadExecutable(const String& FileName,int Hnd,const char *Part,int Index,int SubIndex,char *Pnt,long Length);
    String _DlCallStr(CpuInt DlCallId);
    bool _OpenDynLibrary(const String& LibFile,void **Handler,DlFuncPtr *FuncPtr);
    bool _ScanDynLibrary(void *Handler,const String& LibFile,DlFuncPtr *FuncPtr);
    void _CloseDynLibrary(void *Handler);
    void _DlCallHandler(CpuInt DlCallId);
    String _RpRulePrint(int Rule);
    void _RpGotoFirstElement(int Rule);
    void _RpGotoNextElement(int Rule);
    bool _InnerBlockReplication(CpuWrd Offset,bool ForString,bool ForArray);
    String _BiRulePrint(int Rule);
    void _BiGotoFirstElement(int Rule);
    void _BiGotoNextElement(int Rule);
    bool _InnerBlockInitialization(CpuWrd Offset,bool ForString,bool ForArray,int DimNr,CpuWrd CellSize);
    bool _ExecuteExternal(CpuMbl ExecutableFile,CpuMbl Arguments,CpuMbl *SdOut,CpuMbl *StdErr,bool Redirect,bool ArgIsArray);
    void _RunProgram(int BenchMark,CpuAdr& LastIP);
    double _MinimunClockTick();
    String _GetScaledTime(double NanoSecs);
    void _PrintBenchMark(int BenchMark);
    String _ToStringCpuBol(CpuBol Arg);
    String _ToStringCpuChr(CpuChr Arg);
    String _ToStringCpuShr(CpuShr Arg);
    String _ToStringCpuInt(CpuInt Arg);
    String _ToStringCpuLon(CpuLon Arg);
    String _ToStringCpuFlo(CpuFlo Arg);
    String _ToStringCpuMbl(CpuMbl Arg);
    String _ToStringCpuAgx(CpuAgx Arg);
    String _ToStringCpuAdr(CpuAdr Arg);
    String _ToStringCpuWrd(CpuWrd Arg);
    String _ToStringCpuRef(CpuRef Arg);
    String _ToStringCpuDat(CpuDat Arg);
    void _InnerRefIndirection(char *GlobPnt,char *StackPnt,CpuRef Ref,char **Ptr,CpuMbl &Scope);
    bool _RefIndirection(char *GlobPnt,char *StackPnt,CpuRef Ref,char **Ptr,CpuMbl &Scope);
    bool _DecodeInstructionCodes(int BenchMark,const void **InstAddress,const void *FakeInstHandler,char *CodePtr);
    bool _DecodeLocalVariables(bool FirstTime,char *CodePtr,const char *OldStackPtr,const char *NewStackPtr);
    String _GetFunctionDebugName(int FunIndex);
    bool _DumpDisassembledLines(CpuAdr FuncAddress,const String& FuncDebugName,const Array<DisAsmLine>& Lines);
    bool _DisassembleLine(char *CodePtr,char *StackPtr,CpuAdr IP,CpuDecMode DecMode1,CpuDecMode DecMode2,CpuDecMode DecMode3,CpuDecMode DecMode4,CpuAdr Adv1,CpuAdr Adv2,CpuAdr Adv3,CpuAdr Adv4,int CurrFunIndex,bool RuntimeMode,String& Hex, String& Asm,String& Symbols);
    bool _GetArguments(CpuAdr IP,CpuAdr BP,char *GlobPnt,char *StackPnt,char *CodePtr,CpuDecMode DecMode1,CpuDecMode DecMode2,CpuDecMode DecMode3,CpuDecMode DecMode4,int Stage,String *Arg);
    String _AddressInfoLine(CpuAdr CodeAddress);
    String _AddressInfo(CpuAdr CodeAddress,bool& MainFrame);
    void _PrintCallStack(CpuAdr IP);

    //System call pointer table
    const void *_SysCallFunPtr[_SystemCallNr];

  //Public members
  public:
  
    //Binary header of loaded program
    BinaryHeader BinHdr;

    //Members
    int GetCurrentProcess();
    bool LoadProgram(const String& FileName,int ProcessId,int ArgNr,char *Arg[],int ArgStart);
    bool ExecProgram(int ProcessId,int BenchMark);
    bool Disassemble();
    void SetLibPaths(const String& DynLibPath,const String& TmpLibPath);
    void CloseAllFiles();
    void UnloadLibraries();
    void SetRomBuffer(RomFileBuffer *Ptr);

    //Constructor / Destructor
    Runtime(){};
    ~Runtime(){};

};

//File disassembling entry point
bool CallDisassembleFile(const String& BinaryFile,CpuWrd MemoryUnitKB,CpuWrd StartUnits,CpuWrd ChunkUnits,int ArgNr,char *Arg[],int ArgStart);

//Runtime entry point
bool CallRuntime(const String& BinaryFile,CpuWrd MemoryUnitKB,CpuWrd StartUnits,CpuWrd ChunkUnits,bool LockMemory,int BenchMark,
                 const String& DynLibPath,const String& TmpLibPath,int ArgNr,char *Arg[],int ArgStart,RomFileBuffer *RomBuffer);

//Debug message interface
void LibDebugMessage(char *Msg);

#endif