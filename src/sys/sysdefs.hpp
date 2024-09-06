//sysdefs.hpp: Common shared definitions for VM and compiler

//Include version file
#include "sys/version.hpp"

//Wrap include
#ifndef _SYSDEFS_HPP
#define _SYSDEFS_HPP

//Heading information
#define VERSION_MAXLEN 10
#define BINARY_FORMAT 0
#define MASTER_NAME "Dungeon Spells"
#define GITHUB_URL "https://github.com/lionteddy/DungeonSpells"
#define SPLASH_BANNER \
R"( ___                                ___           _ _    )" "\n" \
R"(|   \ _  _ _ _  __ _ ___ ___ _ _   / __/_ __  ___| | |___)" "\n" \
R"(| \\ | || | ' \/ _` / -_) _ \ ' \  \__ \ '_ \/ -_) | (_-<)" "\n" \
R"(|___/ \___|_|\_\__, \___\___/_|\_\ /___/ .__/\___|_|_/__/)" " (v" VERSION_NUMBER ")\n" \
R"(               |___/                   |_|               )" "\n"

//Cpu datatypes
typedef int8_t  CpuBol; //Cpu boolean type
typedef int8_t  CpuChr; //Cpu char type
typedef int16_t CpuShr; //Cpu short integer type
typedef int32_t CpuInt; //Cpu integer type
typedef int64_t CpuLon; //Cpu long integer type
typedef double  CpuFlo; //Cpu floating point type
typedef int8_t  CpuDat; //Cpu generic type (used only as pointer)
typedef int16_t CpuAgx; //Cpu array geometry index
typedef int16_t CpuIcd; //Cpu instruction code
#ifdef __DUNS32__
  typedef int32_t CpuAdr; //Cpu memory address type
  typedef int16_t CpuMbl; //Memory block number type
  typedef int32_t CpuWrd; //Memory offset type
#else
  typedef int64_t CpuAdr; //Cpu memory address type
  typedef int32_t CpuMbl; //Memory block number type
  typedef int64_t CpuWrd; //Memory offset type
#endif

//Cpu reference type
//id==0x0000: Null reference
//id==0x7FFF: Reference points to global memory
//id< 0x8000: Reference points to stack memory and id==scopeid 
//id>=0x8000: Reference points to block memory and id==block
struct CpuRef{ CpuMbl Id; CpuWrd Offset; }; 

//System name space
#define SYSTEM_NAMESPACE "$"

//File extensions
#define SOURCE_EXT     ".ds"
#define LIBRARY_EXT    ".dlb"
#define EXECUTABLE_EXT ".dex"
#define CMP_LOG_EXT    ".clog"
#define RUN_LOG_EXT    ".xlog"
#ifdef __WIN__
  #define DYNLIB_EXT ".dll"
#else
  #define DYNLIB_EXT ".so"
#endif

//Regex for file extensions
#define SOUR_REGEX R"(.*\.ds$)"
#define EXEC_REGEX R"(.*\.dex$)"
//Regex to match source or exec is: .*\.\(ds\|dex\)$

//Application Ids
#define DUNC_APPID String("C")
#define DUNR_APPID String("R")
#define DUNS_APPID String("S")

//Name of entry point, function result and reference to self
//(here because they are used when printing call stack from runtime)
#define MAIN_FUNCTION   "main"
#define FUNCTION_RESULT "result"
#define SELF_REF_NAME   "self"

//Marks inside binary files
#define FILEMARKLIBR "DSLB"
#define FILEMARKEXEC "DSXC"
#define FILEMARKGLOB "GLOB"
#define FILEMARKCODE "CODE"
#define FILEMARKFARR "FARR"
#define FILEMARKDARR "DARR"
#define FILEMARKBLCK "BLCK"
#define FILEMARKDLCA "DLCA"
#define FILEMARKDEPN "DEPN"
#define FILEMARKUREF "UREF"
#define FILEMARKRELO "RELO"
#define FILEMARKSDIM "SDIM"
#define FILEMARKSTYP "STYP"
#define FILEMARKSVAR "SVAR"
#define FILEMARKSFLD "SFLD"
#define FILEMARKSFUN "SFUN"
#define FILEMARKSPAR "SPAR"
#define FILEMARKDMOD "DMOD"
#define FILEMARKDTYP "DTYP"
#define FILEMARKDVAR "DVAR"
#define FILEMARKDFLD "DFLD"
#define FILEMARKDFUN "DFUN"
#define FILEMARKDPAR "DPAR"
#define FILEMARKDLIN "DLIN"

//String to note error at filemark when reading/writing binary files
#define INDEXHEAD "head"

//Default memory configuration for aux memory controller
#define DEFAULTMEMUNITSIZE 512
#define DEFAULTMEMUNITS  8192
#define DEFAULTCHUNKMEMUNITS  4096
#define DEFAULTBLOCKMAX  4096

//Number limits
#define MAX_CHR     +127
#define MIN_CHR     -127
#define MAX_SHR     +32767
#define MIN_SHR     -32767
#define MAX_INT     +2147483647L
#define MIN_INT     -2147483647L
#define MAX_LON     +9223372036854775807LL
#define MIN_LON     -9223372036854775807LL
#define MIN_NDOUBLE -1.797693134E+308f
#define MAX_NDOUBLE -2.225073850E-308f
#define MIN_PDOUBLE +2.225073850E-308f
#define MAX_PDOUBLE +1.797693134E+308f

//Bitmasks
#define ARRGEOMMASK80 0x8000
#define ARRGEOMMASK7F 0x7FFF
#ifdef __DUNS32__
  #define BLOCKMASK80 0x8000
  #define BLOCKMASK7F 0x7FFF
#else
  #define BLOCKMASK80 0x80000000
  #define BLOCKMASK7F 0x7FFFFFFF
#endif

//Information macros
#define HEXVALUE(x) (String(Buffer((char *)&(x),sizeof(x)).ToHex().BuffPnt()))
#define HEXFORMAT(x) (String(Buffer((char *)&(x),sizeof(x)).ToHex().BuffPnt())+String("h"))
#define NZHEXFORMAT(x) (String(Buffer((char *)&(x),sizeof(x)).ToNzHex().BuffPnt())+String("h"))
#define PTRFORMAT(p) (sizeof(p)==4?HEXFORMAT(p).Mid(0,4)+":"+HEXFORMAT(p).Mid(4,4):HEXFORMAT(p).Mid(0,4)+":"+HEXFORMAT(p).Mid(4,4)+":"+HEXFORMAT(p).Mid(8,4)+":"+HEXFORMAT(p).Mid(12,4))

//Maximun & Minimun
#define MAX(a,b) ((a)>=(b)?(a):(b))
#define MIN(a,b) ((a)<=(b)?(a):(b))

//File path length
#define FILEPATHLEN 255

//Redirection command
#define REDIRCOMMAND "2>&1"

//Instruction sizes and offsets in 32-bit mode
#ifdef __DUNS32__

#define AOFF_I     6
#define AOFF_IA    10
#define AOFF_IAA   14
#define AOFF_IAAA  18
#define AOFF_IAAG  16
#define AOFF_IAB   11
#define AOFF_IAC   11
#define AOFF_IAG   12
#define AOFF_IAGA  16
#define AOFF_IG    8
#define AOFF_IGA   12
#define AOFF_IGC   9
#define AOFF_IW    8
#define AOFF_IZ    10
#define AOFF_IZC   11
#define ISIZ_I     6
#define ISIZ_IA    10
#define ISIZ_IAA   14
#define ISIZ_IAAA  18
#define ISIZ_IAAAA 22
#define ISIZ_IAAAZ 22
#define ISIZ_IAAG  16
#define ISIZ_IAAGA 20
#define ISIZ_IAAZ  18
#define ISIZ_IAB   11
#define ISIZ_IABG  13
#define ISIZ_IAC   11
#define ISIZ_IACA  15
#define ISIZ_IACZ  15
#define ISIZ_IAF   18
#define ISIZ_IAG   12
#define ISIZ_IAGA  16
#define ISIZ_IAGAG 18
#define ISIZ_IAI   14
#define ISIZ_IAL   18
#define ISIZ_IAW   12
#define ISIZ_IAZ   14
#define ISIZ_IGA   12
#define ISIZ_IGAA  16
#define ISIZ_IGCA  13
#define ISIZ_IGCZ  13
#define ISIZ_II    10
#define ISIZ_IL    14
#define ISIZ_IWW   10
#define ISIZ_IZ    10
#define ISIZ_IZCZ  15

//Instruction sizes and offsets in 64-bit mode
#else

#define AOFF_I     10
#define AOFF_IA    18
#define AOFF_IAA   26
#define AOFF_IAAA  34
#define AOFF_IAAG  28
#define AOFF_IAB   19
#define AOFF_IAC   19
#define AOFF_IAG   20
#define AOFF_IAGA  28
#define AOFF_IG    12
#define AOFF_IGA   20
#define AOFF_IGC   13
#define AOFF_IW    12
#define AOFF_IZ    18
#define AOFF_IZC   19
#define ISIZ_I     10
#define ISIZ_IA    18
#define ISIZ_IAA   26
#define ISIZ_IAAA  34
#define ISIZ_IAAAA 42
#define ISIZ_IAAAZ 42
#define ISIZ_IAAG  28
#define ISIZ_IAAGA 36
#define ISIZ_IAAZ  34
#define ISIZ_IAB   19
#define ISIZ_IABG  21
#define ISIZ_IAC   19
#define ISIZ_IACA  27
#define ISIZ_IACZ  27
#define ISIZ_IAF   26
#define ISIZ_IAG   20
#define ISIZ_IAGA  28
#define ISIZ_IAGAG 30
#define ISIZ_IAI   22
#define ISIZ_IAL   26
#define ISIZ_IAW   20
#define ISIZ_IAZ   26
#define ISIZ_IGA   20
#define ISIZ_IGAA  28
#define ISIZ_IGCA  21
#define ISIZ_IGCZ  21
#define ISIZ_II    14
#define ISIZ_IL    18
#define ISIZ_IWW   14
#define ISIZ_IZ    18
#define ISIZ_IZCZ  27

#endif

//Host system values
enum class HostSystem{
  Linux=1,
  Windows=2
};

//CPU native data types
enum class CpuDataType:int{
  Boolean  = 0, //Boolean
  Char     = 1, //Char
  Short    = 2, //Short
  Integer  = 3, //Integer
  Long     = 4, //Long
  Float    = 5, //Float
  StrBlk   = 6, //String block
  ArrBlk   = 7, //Array block
  ArrGeom  = 8, //Array geometry
  FunAddr  = 9, //Function address
  JumpAddr =10, //Jump address
  VarAddr  =11, //Variable address (used to pass a pointer without a reference)
  Undefined=12  //Undefined data type (used when only address is relevant for instruction operation)
};
//Note: Function addresses are code addresses are same type, 
//but they are separated here because they have different formatting in assebler file

//CPU addressing modes
enum class CpuAdrMode:int{
  Address=0,    //Access by memory address
  Indirection,  //Access by indirection
  LitValue      //Access by littera value
};

//CPU decoding modes
enum class CpuDecMode:char{
  LoclVar=1, //Local variable
  GlobVar=2, //Global variable
  LoclInd=3, //Indirection through local variable reference
  GlobInd=4  //Indirection through global variable reference
};

//Constants
const int _MaxArrayDims=5;
const int _InstructionNr=361;
const int _SystemCallNr=91;
const int _MaxIdLen=64;

//CPU instruction code
enum class CpuInstCode:int{
  //Arithmethic operations
  NEGc,NEGw,NEGi,NEGl,NEGf,ADDc,ADDw,ADDi,ADDl,ADDf,SUBc,SUBw,SUBi,SUBl,SUBf,MULc,MULw,MULi,MULl,MULf,DIVc,DIVw,DIVi,DIVl,DIVf,
  MODc,MODw,MODi,MODl,INCc,INCw,INCi,INCl,INCf,DECc,DECw,DECi,DECl,DECf,PINCc,PINCw,PINCi,PINCl,PINCf,PDECc,PDECw,PDECi,PDECl,PDECf,
  //Logical operations
  LNOT,LAND,LOR,
  //Binary operations
  BNOTc,BNOTw,BNOTi,BNOTl,BANDc,BANDw,BANDi,BANDl,BORc,BORw,BORi,BORl,BXORc,BXORw,BXORi,BXORl,SHLc,SHLw,SHLi,SHLl,SHRc,SHRw,SHRi,SHRl,
  //Comparison operations
  LESb,LESc,LESw,LESi,LESl,LESf,LESs,LEQb,LEQc,LEQw,LEQi,LEQl,LEQf,LEQs,GREb,GREc,GREw,GREi,GREl,GREf,GREs,
  GEQb,GEQc,GEQw,GEQi,GEQl,GEQf,GEQs,EQUb,EQUc,EQUw,EQUi,EQUl,EQUf,EQUs,DISb,DISc,DISw,DISi,DISl,DISf,DISs,
  //Assignment operations
  MVb,MVc,MVw,MVi,MVl,MVf,MVr,LOADb,LOADc,LOADw,LOADi,LOADl,LOADf,
  MVADc,MVADw,MVADi,MVADl,MVADf,MVSUc,MVSUw,MVSUi,MVSUl,MVSUf,MVMUc,MVMUw,MVMUi,MVMUl,MVMUf,MVDIc,MVDIw,MVDIi,MVDIl,MVDIf,
  MVMOc,MVMOw,MVMOi,MVMOl,MVSLc,MVSLw,MVSLi,MVSLl,MVSRc,MVSRw,MVSRi,MVSRl,MVANc,MVANw,MVANi,MVANl,MVXOc,MVXOw,MVXOi,MVXOl,MVORc,MVORw,MVORi,MVORl,
  //Inner block replication
  RPBEG,RPSTR,RPARR,RPLOF,RPLOD,RPEND,
  //Inner block initialization
  BIBEG,BISTR,BIARR,BILOF,BIEND,
  //Memory
  REFOF,REFAD,REFER,COPY,SCOPY,SSWCP,ACOPY,TOCA,STOCA,ATOCA,FRCA,SFRCA,AFRCA,CLEAR,STACK,
  //1-dimensional fix array operations
  AF1RF,AF1RW,AF1FO,AF1NX,AF1SJ,AF1CJ,
  //Fixed array operations
  AFDEF,AFSSZ,AFGET,AFIDX,AFREF,
  //1-dimensional dyn array operations
  AD1EM,AD1DF,AD1AP,AD1IN,AD1DE,AD1RF,AD1RS,AD1RW,AD1FO,AD1NX,AD1SJ,AD1CJ,
  //Dynamic array operations
  ADEMP,ADDEF,ADSET,ADRSZ,ADGET,ADRST,ADIDX,ADREF,ADSIZ,
  //Array casting
  AF2F,AF2D,AD2F,AD2D,
  //Function calls
  PUSHb,PUSHc,PUSHw,PUSHi,PUSHl,PUSHf,PUSHr,REFPU,LPUb,LPUc,LPUw,LPUi,LPUl,LPUf,LPUr,LPUSr,LPADr,LPAFr,LRPU,LRPUS,LRPAD,LRPAF,CALL,RET,CALLN,RETN,SCALL,LCALL,SULOK,
  //Char operations
  CUPPR,CLOWR,
  //String operations
  SEMP,SLEN,SMID,SINDX,SRGHT,SLEFT,SCUTR,SCUTL,SCONC,SMVCO,SMVRC,SFIND,SSUBS,STRIM,SUPPR,SLOWR,SLJUS,SRJUS,SMATC,SLIKE,SREPL,SSPLI,SSTWI,SENWI,SISBO,SISCH,SISSH,SISIN,SISLO,SISFL,
  //Data conversions
  BO2CH,BO2SH,BO2IN,BO2LO,BO2FL,BO2ST,CH2BO,CH2SH,CH2IN,CH2LO,CH2FL,CH2ST,CHFMT,SH2BO,SH2CH,SH2IN,SH2LO,SH2FL,SH2ST,SHFMT,
  IN2BO,IN2CH,IN2SH,IN2LO,IN2FL,IN2ST,INFMT,LO2BO,LO2CH,LO2SH,LO2IN,LO2FL,LO2ST,LOFMT,FL2BO,FL2CH,FL2SH,FL2IN,FL2LO,FL2ST,FLFMT,ST2BO,ST2CH,ST2SH,ST2IN,ST2LO,ST2FL,
  //Jumps
  JMPTR,JMPFL,JMP,
  //Decoder
  DAGV1,DAGV2,DAGV3,DAGV4,DAGI1,DAGI2,DAGI3,DAGI4,DALI1,DALI2,DALI3,DALI4,
  //Other
  NOP
};

//System calls
enum class SystemCall:int{
  //System
  ProgramExit=0,Panic,Delay,Execute1,Execute2,Error,ErrorText,LastError,GetArg,HostSystem,
  //Console
  ConsolePrint,ConsolePrintLine,ConsolePrintError,ConsolePrintErrorLine,
  //File system
  GetFileName,GetFileNameNoExt,GetFileExtension,GetDirName,FileExists,DirExists,GetHandler,FreeHandler,GetFileSize,
  OpenForRead,OpenForWrite,OpenForAppend,Read,Write,ReadAll,WriteAll,ReadStr,WriteStr,ReadStrAll,WriteStrAll,CloseFile,Hnd2File,File2Hnd,
  //Math
  AbsChr,AbsShr,AbsInt,AbsLon,AbsFlo,MinChr,MinShr,MinInt,MinLon,MinFlo,MaxChr,MaxShr,MaxInt,MaxLon,MaxFlo,
  Exp,Ln,Log,Logn,Pow,Sqrt,Cbrt,Sin,Cos,Tan,Asin,Acos,Atan,Sinh,Cosh,Tanh,Asinh,Acosh,Atanh,Ceil,Floor,Round,Seed,Rand,
  //Date & time
  DateValid,DateValue,BegOfMonth,EndOfMonth,DatePart,DateAdd,TimeValid,TimeValue,TimePart,TimeAdd,NanoSecAdd,GetDate,GetTime,DateDiff,TimeDiff
};

//Block definition table (for strings and arrays, same way aux mememory manager stores information)
struct BlockDef{
  CpuWrd ArrIndex;
  Buffer Buff;
};

//Array indexes
#define ZERO_INDEXES (ArrayIndexes){{0,0,0,0,0}}
struct ArrayIndexes{
  CpuWrd n[_MaxArrayDims];
};

//Fixed array definitions (used to store fixed array metadata for global arrays in binary files)
struct ArrayFixDef{
  CpuAgx GeomIndex;
  int DimNr;
  CpuWrd CellSize;
  ArrayIndexes DimSize;
};

//Dynamic array definitions (used to store dynamic array metadata for array litterals in binary files)
struct ArrayDynDef{
  int DimNr;
  CpuWrd CellSize;
  ArrayIndexes DimSize;
};

//Dyamic library call definition
struct DlCallDef{
  CpuChr DlName[_MaxIdLen+1]; //Dynamic library name
  CpuChr DlFunction[_MaxIdLen+1]; //Dynamic library function
};

//Binary file header (Same for libraries and executables)
#ifdef __DUNS32__
  #define BIN_HEADER_FILLER 8
#else
  #define BIN_HEADER_FILLER 4
#endif
struct BinaryHeader{
  char FileMark[4];                  //File mark (DSXC, DSLB)
  CpuInt BinFormat;                  //Format of the binary file
  CpuChr Arquitecture;               //32 / 64 bit
  char SysVersion[VERSION_MAXLEN+1]; //System version
  char SysBuildDate[10+1];           //System build date
  char SysBuildTime[8+1];            //System build date
  CpuBol IsBinLibrary;               //Binary file is a library?
  CpuBol DebugSymbols;               //Binary contains debug symbols
  CpuLon GlobBufferNr;               //Global buffer size
  CpuLon BlockNr;                    //Global block table size
  CpuLon ArrFixDefNr;                //Global fixed array definitions table size
  CpuLon ArrDynDefNr;                //Global dynamic array definitions table size
  CpuLon CodeBufferNr;               //Code buffer size
  CpuLon DlCallNr;                   //Number of dynamic library calls
  CpuLon MemUnitSize;                //Memory assignment unit size
  CpuLon MemUnits;                   //Starting number of memory units
  CpuLon ChunkMemUnits;              //Increase size of memory units
  CpuInt BlockMax;                   //Memory handler table size
  CpuShr LibMajorVers;               //Library major version
  CpuShr LibMinorVers;               //Library minor version
  CpuShr LibRevisionNr;              //Library revision number
  CpuInt DependencyNr;               //Dependency table size
  CpuInt UndefRefNr;                 //Undefined references table size
  CpuLon RelocTableNr;               //Relocation table size
  CpuInt LnkSymDimNr;                //Records in Dimension linker table
  CpuInt LnkSymTypNr;                //Records in Data type linker symbol table
  CpuInt LnkSymVarNr;                //Records in Variable linker symbol table
  CpuInt LnkSymFldNr;                //Records in Fields linker symbol table
  CpuInt LnkSymFunNr;                //Records in Function linker symbol table
  CpuInt LnkSymParNr;                //Records in Parameter linker symbol table
  CpuInt DbgSymModNr;                //Records in Modules debug symbol table
  CpuInt DbgSymTypNr;                //Records in Types debug symbol table
  CpuInt DbgSymVarNr;                //Records in Variables debug symbol table
  CpuInt DbgSymFldNr;                //Records in Fields debug symbol table
  CpuInt DbgSymFunNr;                //Records in Functions debug symbol table
  CpuInt DbgSymParNr;                //Records in Parameters debug symbol table
  CpuInt DbgSymLinNr;                //Records in Source code lines debug symbol table
  CpuAdr SuperInitAdr;               //Address of super initialization routine
  char Filler[BIN_HEADER_FILLER];    //Filler to have same size of header in 32 / 64 bit modes
  BinaryHeader(){};
};

//Modules debug symbol table
struct DbgSymModule{
  CpuChr Name[_MaxIdLen+1];   //Module name (without extension)
  CpuChr Path[FILEPATHLEN+1]; //Module path (relative as writen on import/include sentence)
};

//Datatype debug symbol table
struct DbgSymType{
  CpuChr Name[_MaxIdLen+1];   //Data type name
  CpuChr CpuType;             //Cpu data type
  CpuChr MstType;             //Master type
  CpuLon Length;              //Type size
  CpuInt FieldLow;            //Field low in member table (class variables)
  CpuInt FieldHigh;           //Field high in member table (class variables)
};

//Variable debug symbol table
struct DbgSymVariable{
  CpuChr Name[_MaxIdLen+1]; //Variable name
  CpuInt ModIndex;          //Module index
  CpuBol Glob;              //Is variable global ? 
  CpuInt FunIndex;          //Local scope function index
  CpuInt TypIndex;          //Data type table index
  CpuLon Address;           //Memory address
  CpuBol IsReference;       //Is variable a reference ? 
};

//Field debug symbol table
struct DbgSymField{
  CpuChr Name[_MaxIdLen+1]; //Field name
  CpuInt TypIndex;          //Data type
  CpuLon Offset;            //Field offset from start address
};
 
//Function debug symbol table
struct DbgSymFunction{
  CpuChr Kind;              //Function kind
  CpuChr Name[_MaxIdLen+1]; //Function name
  CpuInt ModIndex;          //Module index
  CpuLon BegAddress;        //Beginning code address
  CpuLon EndAddress;        //Ending code address
  CpuInt TypIndex;          //Return data type
  CpuBol IsVoid;            //Is function returning void ?
  CpuInt ParmNr;            //Number of function parameters
  CpuInt ParmLow;           //Parameter low in parameter table
  CpuInt ParmHigh;          //Parameter high in parameter table
};

//Parameter debug symbol table
struct DbgSymParameter{
  CpuChr Name[_MaxIdLen+1]; //Parameter name
  CpuInt TypIndex;          //Data type table index
  CpuInt FunIndex;          //Function index to which this parameter belongs to
  CpuBol IsReference;       //Is parameter passed by reference ? 
};

//Source lines
struct DbgSymLine{
  CpuInt ModIndex;    //Module index
  CpuLon BegAddress;  //Beginning code address
  CpuLon EndAddress;  //Ending code address
  CpuInt LineNr;      //Source code line number
};

//ROM file buffer definitions
#define APP_PKG_CONTAINER  "dunr"
#define APP_PKG_CONFIG_EXT ".cfg"
#define ROM_BUFF_PAYLOADKB 128 //128KB
#define ROM_BUFF_LABEL     "**DS_ROM_FILE_BUFFER**"
#define ROM_BUFF_LABEL_LEN sizeof(ROM_BUFF_LABEL)-1
struct RomFileBuffer{
  char Label[ROM_BUFF_LABEL_LEN+1];
  bool Loaded;
  char FileName[_MaxIdLen+1];
  CpuLon Length; 
  char Buff[ROM_BUFF_PAYLOADKB*1024];
  CpuShr End;
};

//Clock types
typedef std::chrono::time_point<std::chrono::steady_clock> ClockPoint;
typedef std::chrono::duration<double,std::nano> ClockSpanNSec;
typedef std::chrono::duration<double> ClockSpanSec;
#define ClockGet() std::chrono::steady_clock::now()
#define ClockIntervalNSec(a,b) (((std::chrono::duration<double,std::nano>)((a)-(b))).count())
#define ClockIntervalSec(a,b)  (((std::chrono::duration<double>)((a)-(b))).count())

//Get host system
inline HostSystem GetHostSystem(){
  #ifdef __WIN__
    return HostSystem::Windows;
  #else
    return HostSystem::Linux;
  #endif
};

//Get host system name
inline String HostSystemName(HostSystem Syst){
  String Name;
  switch(Syst){
    case HostSystem::Windows: Name="windows"; break; 
    case HostSystem::Linux  : Name="linux"; break; 
  }
  return Name;
};

//Get architecture
inline int GetArchitecture(){
  #ifdef __DUNS32__
    return 32;
  #else
    return 64;
  #endif
};

//Check if we are on development version
inline bool IsDevelopmentVersion(){
  #ifdef  __DEV__
    return true;
  #else
    return false;
  #endif
}

//Check profiler mode
inline bool IsProfilerMode(){
  #ifdef __PROFILER__
  return true;
  #else
  return false;
  #endif
}

//Check sanitizer mode
inline bool IsSanitizerMode(){
  #ifdef __SANITIZER__
  return true;
  #else
  return false;
  #endif
}

//Check enabled performance tool mode
inline bool IsPerformanceToolMode(){
  #ifdef __PERFTOOL__
  return true;
  #else
  return false;
  #endif
}

//Check enabled runtime expanded mode
inline bool IsRuntimeExpandedMode(){
  #ifdef __EXPANDED__
  return true;
  #else
  return false;
  #endif
}

//Check enabled coverage tool mode
inline bool IsCoverageMode(){
  #ifdef __COVERAGE__
  return true;
  #else
  return false;
  #endif
}

//Check no-optimization mode
inline bool IsNoOptimizationMode(){
  #ifdef __NO_OPT__
  return true;
  #else
  return false;
  #endif
}

//Get version name
inline String GetVersionName(){
  return (IsDevelopmentVersion()?String("Development"):String("Release"));
}

//Get cpu datatype of cpu word
inline CpuDataType WordCpuDataType(){
  return GetArchitecture()==32?CpuDataType::Integer:CpuDataType::Long;
}

//Get build date
inline String BuildDate(){
  String Date=String(__DATE__);
  if(Date[4]==' '){ Date[4]='0'; }
  Date=Date.Upper()
  .Replace("JAN","01")
  .Replace("FEB","02")
  .Replace("MAR","03")
  .Replace("APR","04")
  .Replace("MAY","05")
  .Replace("JUN","06")
  .Replace("JUL","07")
  .Replace("AUG","08")
  .Replace("SEP","09")
  .Replace("OCT","10")
  .Replace("NOV","11")
  .Replace("DEC","12");
  Date=Date.Mid(3,2)+"."+Date.Mid(0,2)+"."+Date.Mid(6,4);
  return Date;
}

//Get build time
inline String BuildTime(){
  return String(__TIME__);
}

//Get build date and time
inline String BuildDateTime(){
  return BuildDate()+" "+BuildTime();
}

//Get build number
inline String BuildNumber(const String& AppId){
  String Version=
  (String("00")+String(VERSION_NUMBER).Split(".")[0]).Right(2)+
  (String(VERSION_NUMBER).Split(".")[1]+String("000")).Left(3);
  String DateTime=BuildDateTime().Replace(".","").Replace(":","").Replace(" ","");
  DateTime=DateTime.Mid(4,4)+DateTime.Mid(2,2)+DateTime.Mid(0,2)+DateTime.Mid(8,6);
  return HostSystemName(GetHostSystem()).Mid(0,1).Upper()+AppId+ToString(GetArchitecture())+DateTime+Version+(IsDevelopmentVersion()?"D":"R")
  +(IsProfilerMode()?"P":"")+(IsSanitizerMode()?"S":"")+(IsPerformanceToolMode()?"F":"")+(IsRuntimeExpandedMode()?"E":"")+(IsCoverageMode()?"C":"")+(IsNoOptimizationMode()?"N":"");
}

//CPU Decoder mode name
inline String CpuDecModeName(CpuDecMode Mode){
  String Result;
  switch(Mode){
    case CpuDecMode::LoclVar: Result="LoclVar"; break;
    case CpuDecMode::GlobVar: Result="GlobVar"; break;
    case CpuDecMode::LoclInd: Result="LoclInd"; break;
    case CpuDecMode::GlobInd: Result="GlobInd"; break;
  }
  return Result;
}

//CPU data type names
inline String CpuDataTypeName(CpuDataType Type){
  String Result;
  switch(Type){
    case CpuDataType::Boolean  : Result="Boolean"; break;
    case CpuDataType::Char     : Result="Char"; break;
    case CpuDataType::Short    : Result="Short"; break;
    case CpuDataType::Integer  : Result="Integer"; break;
    case CpuDataType::Long     : Result="Long"; break;
    case CpuDataType::Float    : Result="Float"; break;
    case CpuDataType::StrBlk   : Result="StringBlock"; break;
    case CpuDataType::ArrBlk   : Result="ArrayBlock"; break;
    case CpuDataType::ArrGeom  : Result="ArrayGeometry"; break;
    case CpuDataType::FunAddr  : Result="FunctionAddress"; break;
    case CpuDataType::JumpAddr : Result="JumpAddress"; break;
    case CpuDataType::VarAddr  : Result="VarAddress"; break;
    case CpuDataType::Undefined: Result="Undefined"; break;
  }
  return Result;
}

//CPU data type short names
inline String CpuDataTypeShortName(CpuDataType Type){
  String Result;
  switch(Type){
    case CpuDataType::Boolean  : Result="Bol"; break;
    case CpuDataType::Char     : Result="Chr"; break;
    case CpuDataType::Short    : Result="Shr"; break;
    case CpuDataType::Integer  : Result="Int"; break;
    case CpuDataType::Long     : Result="Lon"; break;
    case CpuDataType::Float    : Result="Flo"; break;
    case CpuDataType::StrBlk   : Result="Str"; break;
    case CpuDataType::ArrBlk   : Result="Arr"; break;
    case CpuDataType::ArrGeom  : Result="Agx"; break;
    case CpuDataType::FunAddr  : Result="Fun"; break;
    case CpuDataType::JumpAddr : Result="Jmp"; break;
    case CpuDataType::VarAddr  : Result="Vad"; break;
    case CpuDataType::Undefined: Result="Und"; break;
  }
  return Result;
}

//CPU data type char id
inline String CpuDataTypeCharId(CpuDataType Type){
  String Result;
  switch(Type){
    case CpuDataType::Boolean  : Result="B"; break;
    case CpuDataType::Char     : Result="C"; break;
    case CpuDataType::Short    : Result="W"; break;
    case CpuDataType::Integer  : Result="I"; break;
    case CpuDataType::Long     : Result="L"; break;
    case CpuDataType::Float    : Result="F"; break;
    case CpuDataType::StrBlk   : Result="S"; break;
    case CpuDataType::ArrBlk   : Result="A"; break;
    case CpuDataType::ArrGeom  : Result="G"; break;
    case CpuDataType::FunAddr  : Result="F"; break;
    case CpuDataType::JumpAddr : Result="J"; break;
    case CpuDataType::VarAddr  : Result="V"; break;
    case CpuDataType::Undefined: Result="U"; break;
  }
  return Result;
}

//CPU addressing mode names
inline String CpuAdrModeName(CpuAdrMode Mode){
  String Result;
  switch(Mode){
    case CpuAdrMode::Address    : Result="Address"; break;
    case CpuAdrMode::Indirection: Result="Indirection"; break;
    case CpuAdrMode::LitValue   : Result="LitValue"; break;
  }
  return Result;
}

//Clean non-ascii chars on string and make them dots
inline String NonAscii2Dots(const String& Str){
  String Output;
  for(long i=0;i<Str.Length();i++){
    if(Str[i]>=32 && Str[i]<=126){
      Output+=Str[i];
    }
    else{
      Output+=".";
    }
  }
  return Output;
}

//Clean non-ascii chars on string and make them escape sequences
inline String NonAsciiEscape(const String& Str){
  String Output;
  for(long i=0;i<Str.Length();i++){
    if(Str[i]>=32 && Str[i]<=126){
      Output+=Str[i];
    }
    else{
      Output+="\\x"+HEXVALUE(Str[i]);
    }
  }
  return Output;
}

//Clean non-ascii chars on buffer and make them escape sequences
inline String NonAsciiEscape(const Buffer& Buff){
  String Output;
  for(long i=0;i<Buff.Length();i++){
    if(Buff[i]>=32 && Buff[i]<=126){
      Output+=Buff[i];
    }
    else{
      Output+="\\x"+HEXVALUE(Buff[i]);
    }
  }
  return Output;
}

#endif

