//cfgparser.hpp: Configuration and command line options parser

//Wrap include
#ifndef _CFGPARSER_HPP
#define _CFGPARSER_HPP

//Command line option sets
#define OPSNUL  0 //No option set defined
#define OPSCOM  1 //Compiler
#define OPSRUN  2 //Runtime
#define OPSCNR  4 //Compile and run
#define OPSLIF  8 //Library info
#define OPSXIF 16 //Executable info
#define OPSDIS 32 //Disasemble binary
#define OPSVER 64 //Version info

//Name of configuration file
#define CONFIG_FILE "ds.config"

//Configuration option data types
enum class OptionType{
  Boolean,
  String,
  Number
};

//Configuration options
struct ConfigOptions{
  String SourceFile;
  String OutputFile;
  bool CompileToApp;
  bool EnableAsmFile;
  bool StripSymbols;
  bool CompilerStats;
  bool LinterMode;
  long MaxErrorNr;
  long MaxWarningNr;
  bool PassOnWarnings;
  String LibrInfo;
  String ExecInfo;
  String BinaryFile;
  long MemoryUnitKB;
  long StartUnits;
  long ChunkUnits;
  bool LockMemory;
  long BenchMark;
  String IncludePath;
  String LibraryPath;
  String TmpLibPath;
  String DynLibPath;
  String DisassembleFile;
  bool VersionInfo;
  String DebugLevelIds;
  bool AllDebugLevels;
  bool DebugToConsole;
};

//Configuration parser class
class ConfigParser {
  
  //Private members
  private:
    static bool _Check(int OptionSet,bool PackageMode,ConfigOptions& CfgOpt);
    static bool _ParseConfigFile(const String& ModuleName,const char *ConfigFileName,bool Silent);

  //Public members
  public:
    static bool GetCmdLineOptions(const String& ExecPath,int ArgNr,char *Arg[],int OptionSet,bool PackageMode,const char *ConfigFileName,ConfigOptions& CfgOpt,int& ArgStart);
    static void PrintHelp(const String& ModuleName,const String& AppId);
    static void PrintVersion(const String& AppId);
    static bool EnableDebugLevels(const String& AppId,String DebugLevelIds,bool AllDebugLevels,bool DebugToConsole);
    static void ShowDebugLevels(const String& AppId);
};

//Calculate package container file name using system executable path
inline String GetPkgContainer(const String& ExecPath){
  String Container=_Stl->FileSystem.GetDirName(ExecPath)+APP_PKG_CONTAINER;
  if(GetHostSystem()==HostSystem::Windows){
    Container+=".exe";
  }
  return Container;
}

#endif
