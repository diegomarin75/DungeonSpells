//stl.hpp: Basic I/O functions
#ifndef _STL_HPP
#define _STL_HPP

//Null utf8 sequence (this sequence of bytes cannot happen in utf-8 as 0xFF cannot happen as start byte)
#define NULLUTF8 0xFF000000

//File path delimiter windows/linux
#define WIN_PATH_DELIMITER "\\"
#define LIN_PATH_DELIMITER "/"

//Safe modulus
//It does ((a%b)+b)%b which for a positive yields same result as a%b but for a negative yields a more consistent result
//c++ and python behave different way on this operator! The way python does yields to correct results on date/time functions!
#define MOD(a,b)                  ((((a)%(b))+(b))%(b))

//Date macros (32bit format: YYYY MMDD)
#define DATEVALUE(year,month,day) ((CpuInt)(((year)<<16)+((month)<<8)+(day)))
#define YEARPART(date)            ((CpuInt)((date)>>16))
#define MONTHPART(date)           ((CpuInt)(((date)&0x0000FF00)>>8))
#define DAYPART(date)             ((CpuInt)((date)&0x000000FF))
#define ISLEAPYEAR(year)          (((MOD((year),4) == 0) && (MOD((year),100) != 0)) || (MOD((year),400) == 0))
#define DAYSINYEAR(year)          ((CpuInt)(ISLEAPYEAR(year)?366:365))
#define DAYSINMONTH(year,month)   ((CpuInt)(_MonthDays[month]+((month)==2 && ISLEAPYEAR(year)?1:0)))
#define BEGOFMONTH(date)          ((CpuInt)(((date)&0xFFFFFF00)+1))
#define ENDOFMONTH(date)          ((CpuInt)(((date)&0xFFFFFF00)+DAYSINMONTH(YEARPART(date),MONTHPART(date))))

//Time calculation macros (64bit format: 00HH MMSS NNNN NNNN)
#define TIMEVALUE(hour,min,sec,nsec) (((CpuLon)(hour)<<48)+((CpuLon)(min)<<40)+((CpuLon)(sec)<<32)+(nsec))
#define HOURPART(time)               ((CpuLon)((time)>>48))
#define MINPART(time)                ((CpuLon)(((time)&0x0000FF0000000000)>>40))
#define SECPART(time)                ((CpuLon)(((time)&0x000000FF00000000)>>32))
#define NSECPART(time)               ((CpuLon)((time)&0x00000000FFFFFFFF))

//Time constants
const CpuLon HOURS_IN_DAY=24LL;
const CpuLon MINS_IN_DAY=24LL*60LL;
const CpuLon MINS_IN_HOUR=60LL;
const CpuLon SECS_IN_DAY=24LL*3600LL;
const CpuLon SECS_IN_HOUR=3600LL;
const CpuLon SECS_IN_MINUTE=60LL;
const CpuLon NSECS_IN_DAY=24LL*3600LL*1000000000LL;
const CpuLon NSECS_IN_HOUR=3600LL*1000000000LL;
const CpuLon NSECS_IN_MINUTE=60LL*1000000000LL;
const CpuLon NSECS_IN_SECOND=1000000000LL;

//Stl error codes
enum class StlErrorCode:int{
  Ok=0,
  OpenReadError,
  OpenWriteError,
  ReadError,
  WriteError,
  GetSizeError,
  ReadWhenClosed,
  WriteWhenClosed,
  FileAlreadyOpenSelf,
  FileAlreadyOpen,
  CloseAlreadyClosed,
  FreeHandlerOpenFile,
  HandlerForbidden,
  ExecFileNotExists,
  ReadTimeout
};

//Date parts
enum class StlDatePart:int{
  Year=1,
  Month=2,
  Day=3
};

//Time parts
enum class StlTimePart:int{
  Hour=1,
  Minute=2,
  Second=3,
  Nanosec=4
};

//Stl subsystem class
class StlSubsystem{
  
  //Private members
  private:
    
    //Console class
    class StlConsole{
      
      //Private members
      private:

        //Lock table
        struct StlConsoleLock{ int ProcessId; };

        //Private fields
        StlSubsystem *_Main;
        bool _LockOverride;
        Stack<StlConsoleLock> _LockTable;

        //Private members
        void _Print(const char *Pnt,bool NewLine);
        void _PrintError(const char *Pnt,bool NewLine);

      //Public members
      public:
        bool Lock();
        bool Unlock();
        void Override();
        void Print(const char *Pnt);
        void Print(const String& Str);
        void PrintLine(const char *Pnt);
        void PrintLine(const String& Str);
        void PrintError(const char *Pnt);
        void PrintError(const String& Str);
        void PrintErrorLine(const char *Pnt);
        void PrintErrorLine(const String& Str);
        void PrintTable(const String& Headings,const Array<String>& Rows,const String& Delimiter,const String& Padding="");
        void SetMainPtr(StlSubsystem *Bios);
        StlConsole();
        ~StlConsole();
    };
        
    //File system class
    class StlFileSystem{
      
      //Private members
      private:
        
        //Pointer to parent class
        StlSubsystem *_Main; 
        
        //Methods
        void _ForcedFileClose(int Hnd);

      //Public members
      public:

        //Members
        String Delimiter();
        String Normalize(const String& FilePath);
        String GetFileName(const String& FilePath);
        String GetFileNameNoExt(const String& FilePath);
        String GetFileExtension(const String& FilePath);
        String GetDirName(const String& FilePath,bool EndByDelimiter=true);
        bool FileExists(const String& Path);
        bool DirExists(const String& Path);
        bool GetHandler(int& Hnd);
        bool FreeHandler(int Hnd);
        bool OpenForRead(int Hnd,const String& FilePath);
        bool OpenForWrite(int Hnd,const String& FilePath);
        bool OpenForAppend(int Hnd,const String& FilePath);
        bool GetFileSize(int Hnd,CpuLon& Size);
        bool Read(int Hnd,String& Line);
        bool Read(int Hnd,Buffer& Buff,long Length);
        bool Read(int Hnd,char *Pnt,long Length);
        bool Write(int Hnd,const String& Line);
        bool Write(int Hnd,const Buffer& Buff);
        bool Write(int Hnd,const char *Pnt,long Length);
        bool WriteArray(int Hnd,const Array<String>& Lines);
        bool WriteArray(int Hnd,const Array<Buffer>& Buffers);
        bool FullRead(int Hnd,Buffer& Buff);
        bool FullRead(int Hnd,Array<String>& Lines);
        bool FullWrite(int Hnd,const Array<String>& Lines);
        bool GetStateFlags(int Hnd,bool& Good,bool& Eof,bool& Fail,bool& Bad);
        bool CloseFile(int Hnd);
        void CloseAll();
        String Hnd2File(int Hnd);
        int File2Hnd(const String& FilePath);
        CpuLon GetSeekPos(int Hnd);
        CpuLon GetPrevPos(int Hnd);
        void SetMainPtr(StlSubsystem *Bios);
        StlFileSystem();
        ~StlFileSystem();

    };

    //Math class
    class StlMath{
      
      //Private members
      private:

        //Private fields
        std::minstd_rand _RandomGenerator;
        CpuLon _MinRand;
        CpuLon _MaxRand;
        StlSubsystem *_Main;

      //Public members
      public:

        //Members
        CpuLon Round(CpuFlo Number,CpuInt Decimals);
        void Seed(CpuFlo Number);
        CpuFlo Rand();
        void SetMainPtr(StlSubsystem *Bios);
        StlMath();
        ~StlMath();
    };
        
    //Datetime class
    class StlDateTime{
      
      //Private members
      private:

        //Private fields
        StlSubsystem *_Main;

      //Public members
      public:
        
        //Date format (32bit integer)
        //year mon day
        //FFFF FF  FF  -> range 32767.12.31,-32767.01.01
        
        //Time format (64 bit integer)
        //   hour min sec nanosec
        //00 FF   FF  FF  FFFFFFFFFFFF -> range 00:00:00.000000000,23:59:59.999999999

        //Members
        CpuBol DateValid(CpuInt Year,CpuInt Month,CpuInt Day);
        CpuInt DateValue(CpuInt Year,CpuInt Month,CpuInt Day);
        CpuInt BegOfMonth(CpuInt Date);
        CpuInt EndOfMonth(CpuInt Date);
        CpuInt DatePart(CpuInt Date,StlDatePart Part);
        CpuInt DateAdd(CpuInt Date,StlDatePart Part,CpuInt Units);
        CpuBol TimeValid(CpuInt Hour,CpuInt Min,CpuInt Sec,CpuLon NanoSec);
        CpuLon TimeValue(CpuInt Hour,CpuInt Min,CpuInt Sec,CpuLon NanoSec);
        CpuLon TimePart(CpuLon Time,StlTimePart Part);
        CpuLon TimeAdd(CpuLon Time,StlTimePart Part,CpuLon Units,CpuInt *DayRest);
        CpuLon NanoSecAdd(CpuLon NanoSec,StlTimePart Part,CpuLon Units,CpuInt *DayRest);
        CpuInt GetDate(CpuBol Utc);
        CpuLon GetTime(CpuBol Utc);
        CpuInt DateDiff(CpuInt Date1,CpuInt Date2);
        CpuLon TimeDiff(CpuLon Time1,CpuLon Time2);
        void SetMainPtr(StlSubsystem *Bios);
        StlDateTime();
        ~StlDateTime();
    };
        
    //Main class private properties
    StlErrorCode _ErrorCode;
    String _ErrorInfo;

  //Public members
  public:
      
    //Subclass members
    StlConsole Console;
    StlFileSystem FileSystem;
    StlMath Math;
    StlDateTime DateTime;

    //Public methods
    void Delay(int Millisecs);
    void ClearError();
    void SetError(StlErrorCode Code);
    void SetError(StlErrorCode Code,const String& Info);
    StlErrorCode Error();
    String ErrorText(StlErrorCode Code);
    String LastError();
    StlSubsystem();
    ~StlSubsystem();

};

//System callhandling
String SystemCallId(SystemCall Nr);
SystemCall FindSystemCall(const String& Id);

//Get executable path
bool GetExecutablePath(String& ExecPath);

//Global STL class pointer
extern StlSubsystem* _Stl;

#endif