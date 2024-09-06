//Command line option parser

//Include files
#include "bas/basedefs.hpp"
#include "bas/allocator.hpp"
#include "bas/array.hpp"
#include "bas/sortedarray.hpp"
#include "bas/stack.hpp"
#include "bas/queue.hpp"
#include "bas/buffer.hpp"
#include "bas/string.hpp"
#include "sys/sysdefs.hpp"
#include "sys/system.hpp"
#include "sys/stl.hpp"
#include "sys/msgout.hpp"
#include "cfgparser.hpp"

//Constants
const String _CfgParserComment="#";

//Option value struct
struct OptValue {
  bool Bol;
  long Num;
  String Str;
  OptValue(){}
  ~OptValue(){}  
  OptValue(bool Bol){ this->Bol=Bol; }
  OptValue(long Num){ this->Num=Num; }
  OptValue(const String& Str){ this->Str=Str; }
  OptValue(const OptValue& Opt);
  OptValue& operator=(const OptValue& Opt);
  void _Move(const OptValue& Opt);
  String AsString(OptionType Type) const;
}; 

//Command line options (order of items must match order of entries in table _Opt[])
enum class CmdOption:int{
  OutputFile=0,    
  CompileToApp,
  EnableAsmFile,   
  StripSymbols,   
  CompilerStats,
  LinterMode,
  MaxErrorNr,
  MaxWarningNr,
  PassOnWarnings,
  LibrInfo,        
  ExecInfo,        
  MemoryUnitKB,    
  StartUnits,      
  ChunkUnits,      
  LockMemory,      
  BenchMark,       
  IncludePath,     
  LibraryPath,     
  TmpLibPath,      
  DynLibPath,      
  DisassembleFile,
  VersionInfo,
  DebugLevelIds,   
  AllDebugLevels,  
  DebugToConsole,  
  SourceFile,      
  BinaryFile       
};

//Command line option types
enum class CmdOptionKind{
  Regex,  //Regular expression option
  Coded   //Coded option
};

//Command line option table
struct CmdOptionTable{
  CmdOptionKind Kind;     //Kind of option
  OptionType DataType;    //Data of option
  String Regex;           //Argument regex
  String Code;            //Argument code
  bool ArgStart;          //Arguments passed to application from this option
  bool IsMandatory;       //Is option mandatory ?
  bool DevelVersOnly;     //Option is for development version only
  OptValue Default;       //Default value
  String EnabledAppIds;   //Option is enabled for these application ids
  int EnabledSets;        //Option is enabled for these option sets
  String ConfigName;      //Name of parameter in configuration file
  String ShortName;       //Short name
  String Description;     //Description
};

//Command line option table (order of entries must match order of enum class CmdOption)
const int _OptNr=27;
#define DEF_INC_PATH String(GetHostSystem()==HostSystem::Windows?".\\":"./")
#define DEF_LIB_PATH String(GetHostSystem()==HostSystem::Windows?".\\src\\lib\\":"./src/lib/")
#define DEF_TMP_PATH String(GetHostSystem()==HostSystem::Windows?".\\":"./")
#define DEF_DYN_PATH String(GetHostSystem()==HostSystem::Windows?".\\":"./")
CmdOptionTable _Opt[_OptNr]={
//Option               Kind                  DataType             Regex       Code   ArgStart Mandat Devel  Default               , ApplicationIds                    Option sets           Config file name           ShortName             Description                   
/*OutputFile      */ { CmdOptionKind::Coded, OptionType::String , ""        , "-of", false,   false, false, OptValue(""          ), DUNC_APPID+DUNS_APPID           , OPSCOM|OPSCNR       , ""                       , "output file"         , "Output binary file (*"+String(EXECUTABLE_EXT)+")" },
/*CompileToApp    */ { CmdOptionKind::Coded, OptionType::Boolean, ""        , "-pk", false,   false, false, OptValue(false       ), DUNC_APPID+DUNS_APPID           , OPSCOM|OPSCNR       , ""                       , "packaged app"        , "Compile and create packaged application (up to "+ToString(ROM_BUFF_PAYLOADKB)+"KB binary file)" },
/*EnableAsmFile   */ { CmdOptionKind::Coded, OptionType::Boolean, ""        , "-as", false,   false, false, OptValue(false       ), DUNC_APPID+DUNS_APPID           , OPSCOM|OPSCNR       , "compiler.enableasmfile" , "enable asm file"     , "Generate assembler file (default:<defvalue>)" },
/*StripSymbols    */ { CmdOptionKind::Coded, OptionType::Boolean, ""        , "-ss", false,   false, false, OptValue(false       ), DUNC_APPID+DUNS_APPID           , OPSCOM|OPSCNR       , "compiler.stripsymbols"  , "strip symbols"       , "Remove debug symbols on executable file (default:<defvalue>)" },
/*CompilerStats   */ { CmdOptionKind::Coded, OptionType::Boolean, ""        , "-st", false,   false, false, OptValue(false       ), DUNC_APPID+DUNS_APPID           , OPSCOM|OPSCNR       , "compiler.statistics"    , "compiler statistics" , "Output compiler statistics (default:<defvalue>)" },
/*LinterMode      */ { CmdOptionKind::Coded, OptionType::Boolean, ""        , "-lm", false,   false, false, OptValue(false       ), DUNC_APPID+DUNS_APPID           , OPSCOM|OPSCNR       , ""                       , "linter mode"         , "Reads main module from stdin and no binaries are produced (linter mode) (default:<defvalue>)" },
/*MaxErrorNr      */ { CmdOptionKind::Coded, OptionType::Number , ""        , "-er", false,   false, false, OptValue(10L         ), DUNC_APPID+DUNS_APPID           , OPSCOM|OPSCNR       , "compiler.maxerrornr"    , "max errors"          , "Maximun number of errors to report before stopping compilation (default:<defvalue>)" },
/*MaxWarningNr    */ { CmdOptionKind::Coded, OptionType::Number , ""        , "-wr", false,   false, false, OptValue(10L         ), DUNC_APPID+DUNS_APPID           , OPSCOM|OPSCNR       , "compiler.maxwarningnr"  , "max warnings"        , "Maximun number of warnings to report before stopping compilation (default:<defvalue>)" },
/*PassOnWarnings  */ { CmdOptionKind::Coded, OptionType::Boolean, ""        , "-pw", false,   false, false, OptValue(false       ), DUNC_APPID+DUNS_APPID           , OPSCOM|OPSCNR       , "compiler.passonwarnings", "pass on warnings"    , "Ignore warnings when generating binaries (default:<defvalue>)" },
/*LibrInfo        */ { CmdOptionKind::Coded, OptionType::String , ""        , "-lf", false,   true , false, OptValue(""          ), DUNC_APPID+DUNS_APPID           , OPSLIF              , ""                       , "library file"        , "Show information on library file (*"+String(LIBRARY_EXT)+")" },
/*ExecInfo        */ { CmdOptionKind::Coded, OptionType::String , ""        , "-xf", false,   true , false, OptValue(""          ), DUNC_APPID+DUNS_APPID           , OPSXIF              , ""                       , "executable file"     , "Show information on executable file (*"+String(EXECUTABLE_EXT)+")" },
/*MemoryUnit      */ { CmdOptionKind::Coded, OptionType::Number , ""        , "-mu", false,   false, false, OptValue(64L         ), DUNR_APPID+DUNS_APPID           , OPSRUN|OPSCNR       , "runtime.memoryunitkb"   , "memory unit"         , "Memory unit size in KB (default:<defvalue>KB)" },
/*StartUnits      */ { CmdOptionKind::Coded, OptionType::Number , ""        , "-ms", false,   false, false, OptValue(512L        ), DUNR_APPID+DUNS_APPID           , OPSRUN|OPSCNR       , "runtime.startunits"     , "start units"         , "Starting memory units (default:<defvalue>)" },
/*ChunkUnits      */ { CmdOptionKind::Coded, OptionType::Number , ""        , "-mc", false,   false, false, OptValue(64L         ), DUNR_APPID+DUNS_APPID           , OPSRUN|OPSCNR       , "runtime.chunkunits"     , "chunk units"         , "Increase size of memory units (default:<defvalue>)" },
/*LockMemory      */ { CmdOptionKind::Coded, OptionType::Boolean, ""        , "-ml", false,   false, false, OptValue(false       ), DUNR_APPID+DUNS_APPID           , OPSRUN|OPSCNR       , "runtime.lockmemory"     , "lock memory pages"   , "Lock memory pages to prevent page faults and increase performance (default: <defvalue>)" },
/*BenchMark       */ { CmdOptionKind::Coded, OptionType::Number , ""        , "-bm", false,   false, false, OptValue(0L          ), DUNR_APPID+DUNS_APPID           , OPSRUN|OPSCNR       , ""                       , "benchmark"           , "Benchmark mode: 0=Disabled, 1=Execution time, 2=Plus instr. count, 3=Plus instr. timming (default:<defvalue>)" },
/*IncludePath     */ { CmdOptionKind::Coded, OptionType::String , ""        , "-in", false,   false, false, OptValue(DEF_INC_PATH), DUNC_APPID+DUNS_APPID           , OPSCOM|OPSCNR       , "compiler.includepath"   , "include path"        , "Default path for included files (*"+String(SOURCE_EXT)+") (default:<defvalue>)" },
/*LibraryPath     */ { CmdOptionKind::Coded, OptionType::String , ""        , "-li", false,   false, false, OptValue(DEF_LIB_PATH), DUNC_APPID+DUNS_APPID           , OPSCOM|OPSCNR       , "compiler.librarypath"   , "library path"        , "Default path for dynamic libraries (*"+String(LIBRARY_EXT)+") (default:<defvalue>)" },
/*TmpLibPath      */ { CmdOptionKind::Coded, OptionType::String , ""        , "-tm", false,   false, false, OptValue(DEF_TMP_PATH), DUNR_APPID+DUNS_APPID           , OPSRUN|OPSCNR       , "runtime.tmplibpath"     , "tmp lib path"        , "Default temporary path for copying user dynamic libraries (default:<defvalue>)" },
/*DynLibPath      */ { CmdOptionKind::Coded, OptionType::String , ""        , "-ld", false,   false, false, OptValue(DEF_DYN_PATH), DUNC_APPID+DUNR_APPID+DUNS_APPID, OPSCOM|OPSRUN|OPSCNR, "system.dynlibpath"      , "dyn lib path"        , "Default path for dynamic libraries (*"+String(DYNLIB_EXT)+") (default:<defvalue>)" },
/*DisassembleFile */ { CmdOptionKind::Coded, OptionType::String , ""        , "-da", false,   true , false, OptValue(""          ), DUNR_APPID+DUNS_APPID           , OPSDIS              , ""                       , "disassemble file"    , "Disassemble executable file (*"+String(EXECUTABLE_EXT)+")" },
/*VersionInfo     */ { CmdOptionKind::Coded, OptionType::Boolean, ""        , "-ve", false,   true , false, OptValue(false       ), DUNC_APPID+DUNR_APPID+DUNS_APPID, OPSVER              , ""                       , "version info"        , "Show version information" },
/*DebugLevelIds   */ { CmdOptionKind::Coded, OptionType::String , ""        , "-dl", false,   false, true , OptValue(""          ), DUNC_APPID+DUNR_APPID+DUNS_APPID, OPSCOM|OPSRUN|OPSCNR, ""                       , "debug level ids"     , "Enable debug log messages (see available levels with -dh option)" },
/*AllDebugLevels  */ { CmdOptionKind::Coded, OptionType::Boolean, ""        , "-dx", false,   false, true , OptValue(false       ), DUNC_APPID+DUNR_APPID+DUNS_APPID, OPSCOM|OPSRUN|OPSCNR, ""                       , "all debug levels"    , "Enable all debug log messages" },
/*DebugToConsole  */ { CmdOptionKind::Coded, OptionType::Boolean, ""        , "-dc", false,   false, true , OptValue(false       ), DUNC_APPID+DUNR_APPID+DUNS_APPID, OPSCOM|OPSRUN|OPSCNR, ""                       , "debug to console"    , "Output debug mesages to console" },
/*SourceFile      */ { CmdOptionKind::Regex, OptionType::String , SOUR_REGEX, ""   , true,    true , false, OptValue(""          ), DUNC_APPID+DUNS_APPID           , OPSCOM|OPSCNR       , ""                       , "source file"         , "Source file (*"+String(SOURCE_EXT)+")" },
/*BinaryFile      */ { CmdOptionKind::Regex, OptionType::String , EXEC_REGEX, ""   , true,    true , false, OptValue(""          ), DUNR_APPID                      , OPSRUN              , ""                       , "binary file"         , "Executable file (*"+String(EXECUTABLE_EXT)+")" }
};

//Option value copy constructor
OptValue::OptValue(const OptValue& Opt){
  _Move(Opt); 
}

//Option value copy
OptValue& OptValue::operator=(const OptValue& Opt){
  _Move(Opt);
  return *this;
}

//Option value  move
void OptValue::_Move(const OptValue& Opt){
  Bol=Opt.Bol;
  Num=Opt.Num;
  Str=Opt.Str;
}

//Option value as string
String OptValue::AsString(OptionType Type) const {
  String Result;
  switch(Type){
    case OptionType::Boolean: Result=(Bol?"true":"false"); break;
    case OptionType::String : Result=Str; break;
    case OptionType::Number : Result=ToString(Num); break;
  }
  return Result;
}


//Specific checks of options not implemented through option table
bool ConfigParser::_Check(int OptionSet,bool PackageMode,ConfigOptions& CfgOpt){

  //Checks for compiler
  if(OptionSet==OPSCOM || OptionSet==OPSCNR){

    //Check source file is given
    if(CfgOpt.SourceFile.Length()==0){
      SysMessage(147).Print();
      return false;
    }

    //Check source file extension
    if(_Stl->FileSystem.GetFileExtension(CfgOpt.SourceFile)!=SOURCE_EXT){
      SysMessage(144).Print(SOURCE_EXT);
      return false;
    }

    //Output file not given
    if(CfgOpt.OutputFile.Length()==0){
      CfgOpt.OutputFile=_Stl->FileSystem.GetDirName(CfgOpt.SourceFile)+_Stl->FileSystem.GetFileNameNoExt(CfgOpt.SourceFile);
    }

    //Output file must not contain extension
    if(_Stl->FileSystem.GetFileExtension(CfgOpt.OutputFile).Length()!=0){
      SysMessage(146).Print();
      return false;
    }

    //Output file name must not be larger than _MaxIdLen
    if(_Stl->FileSystem.GetFileNameNoExt(CfgOpt.OutputFile).Length()>_MaxIdLen){
      SysMessage(269).Print(ToString(_MaxIdLen));
      return false;
    }

    //Include path must exist
    if(CfgOpt.IncludePath.Length()!=0 && !_Stl->FileSystem.DirExists(CfgOpt.IncludePath)){
      SysMessage(277).Print(CfgOpt.IncludePath);
      return false;
    }

    //Library path must exist
    if(CfgOpt.LibraryPath.Length()!=0 &&!_Stl->FileSystem.DirExists(CfgOpt.LibraryPath)){
      SysMessage(278).Print(CfgOpt.LibraryPath);
      return false;
    }

    //Dyn lib path must exist
    if(CfgOpt.DynLibPath.Length()!=0 && !_Stl->FileSystem.DirExists(CfgOpt.DynLibPath)){
      SysMessage(272).Print(CfgOpt.DynLibPath);
      return false;
    }

    //Benchmark mode must be between 0 and 3
    if(CfgOpt.BenchMark<0 || CfgOpt.BenchMark>3){
      SysMessage(357).Print(CfgOpt.DynLibPath);
      return false;
    }

  }

  //Checks for runtime
  if(OptionSet==OPSRUN){

    //Check binary file name only when not in package mode
    if(!PackageMode){

      //Check binary file was provided
      if(CfgOpt.BinaryFile.Length()==0){
        SysMessage(302).Print();
        return false;
      }

      //Check binary file extension
      if(_Stl->FileSystem.GetFileExtension(CfgOpt.BinaryFile)!=EXECUTABLE_EXT){
        SysMessage(301).Print(EXECUTABLE_EXT);
        return false;
      }

      //Output file name must not be larger than _MaxIdLen
      if(CfgOpt.BinaryFile.Length()>_MaxIdLen){
        SysMessage(270).Print(ToString(_MaxIdLen));
        return false;
      }
    
      //Benchmark mode must be between 0 and 3
      if(CfgOpt.BenchMark<0 || CfgOpt.BenchMark>3){
        SysMessage(357).Print(CfgOpt.DynLibPath);
        return false;
      }

    }

    //Dyn lib path must exist
    if(CfgOpt.DynLibPath.Length()!=0 && !_Stl->FileSystem.DirExists(CfgOpt.DynLibPath)){
      SysMessage(272).Print(CfgOpt.DynLibPath);
      return false;
    }
    //Temporary dyn lib path must exist
    if(CfgOpt.TmpLibPath.Length()!=0 && !_Stl->FileSystem.DirExists(CfgOpt.TmpLibPath)){
      SysMessage(271).Print(CfgOpt.TmpLibPath);
      return false;
    }

  }

  //Checks for library info
  if(OptionSet==OPSLIF){

    //Check library info file is given
    if(CfgOpt.LibrInfo.Length()==0){
      SysMessage(280).Print();
      return false;
    }

    //Check library file extension
    if(_Stl->FileSystem.GetFileExtension(CfgOpt.LibrInfo)!=LIBRARY_EXT){
      SysMessage(145).Print(LIBRARY_EXT);
      return false;
    }

  }

  //Checks for executable info
  if(OptionSet==OPSXIF){

    //Check library info file is given
    if(CfgOpt.ExecInfo.Length()==0){
      SysMessage(281).Print();
      return false;
    }

    //Check binary file extension
    if(_Stl->FileSystem.GetFileExtension(CfgOpt.ExecInfo)!=EXECUTABLE_EXT){
      SysMessage(565).Print(EXECUTABLE_EXT);
      return false;
    }

  }

  //Checks for disassemble file
  if(OptionSet==OPSDIS){

    //Check library info file is given
    if(CfgOpt.DisassembleFile.Length()==0){
      SysMessage(566).Print();
      return false;
    }

    //Check binary file extension
    if(_Stl->FileSystem.GetFileExtension(CfgOpt.DisassembleFile)!=EXECUTABLE_EXT){
      SysMessage(567).Print(EXECUTABLE_EXT);
      return false;
    }

  }

  //Return code
  return true;

}

//Get configuration options from configuration file
//(updates default values in _Opt[] table)
bool ConfigParser::_ParseConfigFile(const String& ModuleName,const char *ConfigFileName,bool Silent){

  //Variables
  int i,j;
  int OptIndex;
  bool Error;
  int IntValue;
  String Line;
  String Option;
  String Value;
  String ConfigFile;
  Array<String> CfgLines;
  std::ifstream CfgFile;

  //Compose configuration file nameusing path of executable module
  ConfigFile=_Stl->FileSystem.GetDirName(ModuleName)+ConfigFileName;

  //Check configuration file exists
  if(!_Stl->FileSystem.FileExists(ConfigFile)){
    if(!Silent){ SysMessage(223).Print(ConfigFileName); }
    return false;
  }

  //Open configuration file
  CfgFile=std::ifstream(ConfigFile.CharPnt());
  if(!CfgFile.good()){ 
    SysMessage(223).Print(ConfigFileName); 
    return false; 
  }

  //Read configuration file in full
  while(CfgFile >> Line){
    CfgLines.Add(Line);
  }
  if(!CfgFile.eof()){ 
    SysMessage(243).Print(ConfigFileName); 
    CfgFile.close();
    return false; 
  }
  CfgFile.close();

  //Parse configuration options
  for(i=0;i<CfgLines.Length();i++){
    
    //Skip comments and blank lines
    if(CfgLines[i].Trim().StartsWith(_CfgParserComment) || CfgLines[i].Trim().Length()==0){ continue; }

    //Check option line has equal symbol
    if(CfgLines[i].Search("=")==-1){
      SysMessage(244).Print(ConfigFileName,ToString(i+1)); 
      return false; 
    }

    //Parse option and value
    Option=CfgLines[i].Split("=")[0].Trim();
    if(Option.StartsWith("[") && Option.EndsWith("]")){ 
      Option=Option.CutLeft(1).Trim(); 
      Option=Option.CutRight(1).Trim(); 
    }
    Value=CfgLines[i].Replace("["+Option+"]","").Trim();
    if(Value.StartsWith("=")){ Value=Value.CutLeft(1).Trim(); }
    if(Value.StartsWith("\"") && Value.EndsWith("\"")){ 
      Value=Value.CutLeft(1).Trim(); 
      Value=Value.CutRight(1).Trim(); 
    }

    //Read only options for compiler, runtime or system (both)
    if(!Option.StartsWith("compiler.") && !Option.StartsWith("runtime.") && !Option.StartsWith("system.") ){ continue; }

    //Find configuration otion
    OptIndex=-1;
    for(j=0;j<_OptNr;j++){
      if(_Opt[j].ConfigName==Option){ OptIndex=j; break; }
    }
    if(OptIndex==-1){
      SysMessage(316).Print(ConfigFileName,Option); 
      return false; 
    }

    //Assign option values
    if(OptIndex==(int)CmdOption::EnableAsmFile){ 
      if(Value!="true" && Value!="false"){
        SysMessage(303).Print(ConfigFileName,Option); 
        return false; 
      }
      _Opt[OptIndex].Default.Bol=(Value=="true"?true:false);
    }
    else if(OptIndex==(int)CmdOption::StripSymbols){ 
      if(Value!="true" && Value!="false"){
        SysMessage(303).Print(ConfigFileName,Option); 
        return false; 
      }
      _Opt[OptIndex].Default.Bol=(Value=="true"?true:false);
    }
    else if(OptIndex==(int)CmdOption::CompilerStats){ 
      if(Value!="true" && Value!="false"){
        SysMessage(303).Print(ConfigFileName,Option); 
        return false; 
      }
      _Opt[OptIndex].Default.Bol=(Value=="true"?true:false);
    }
    else if(OptIndex==(int)CmdOption::LinterMode){ 
      if(Value!="true" && Value!="false"){
        SysMessage(303).Print(ConfigFileName,Option); 
        return false; 
      }
      _Opt[OptIndex].Default.Bol=(Value=="true"?true:false);
    }
    else if(OptIndex==(int)CmdOption::MaxErrorNr){ 
      IntValue=Value.ToInt(Error);
      if(Error){
        SysMessage(313).Print(ConfigFileName,Option); 
        return false; 
      }
      _Opt[OptIndex].Default.Num=IntValue;
    }
    else if(OptIndex==(int)CmdOption::MaxWarningNr){ 
      IntValue=Value.ToInt(Error);
      if(Error){
        SysMessage(313).Print(ConfigFileName,Option); 
        return false; 
      }
      _Opt[OptIndex].Default.Num=IntValue;
    }
    else if(OptIndex==(int)CmdOption::PassOnWarnings){ 
      if(Value!="true" && Value!="false"){
        SysMessage(303).Print(ConfigFileName,Option); 
        return false; 
      }
      _Opt[OptIndex].Default.Bol=(Value=="true"?true:false);
    }
    else if(OptIndex==(int)CmdOption::MemoryUnitKB){ 
      IntValue=Value.ToInt(Error);
      if(Error){
        SysMessage(313).Print(ConfigFileName,Option); 
        return false; 
      }
      _Opt[OptIndex].Default.Num=IntValue;
    }
    else if(OptIndex==(int)CmdOption::StartUnits){ 
      IntValue=Value.ToInt(Error);
      if(Error){
        SysMessage(313).Print(ConfigFileName,Option); 
        return false; 
      }
      _Opt[OptIndex].Default.Num=IntValue;
    }
    else if(OptIndex==(int)CmdOption::ChunkUnits){ 
      IntValue=Value.ToInt(Error);
      if(Error){
        SysMessage(313).Print(ConfigFileName,Option); 
        return false; 
      }
      _Opt[OptIndex].Default.Num=IntValue;
    }
    else if(OptIndex==(int)CmdOption::LockMemory){ 
      if(Value!="true" && Value!="false"){
        SysMessage(303).Print(ConfigFileName,Option); 
        return false; 
      }
      _Opt[OptIndex].Default.Bol=(Value=="true"?true:false);
    }
    else if(OptIndex==(int)CmdOption::IncludePath){ 
      _Opt[OptIndex].Default.Str=Value.Trim();
    }
    else if(OptIndex==(int)CmdOption::LibraryPath){ 
      _Opt[OptIndex].Default.Str=Value.Trim();
    }
    else if(OptIndex==(int)CmdOption::TmpLibPath){ 
      _Opt[OptIndex].Default.Str=Value.Trim();
    }
    else if(OptIndex==(int)CmdOption::DynLibPath){ 
      _Opt[OptIndex].Default.Str=Value.Trim();
    }
    else{ 
      SysMessage(316).Print(ConfigFileName,Option); 
      return false; 
    }

  }
  
  //Return code
  return true;

}

//Parse command line options
bool ConfigParser::GetCmdLineOptions(const String& ExecPath,int ArgNr,char *Arg[],int OptionSet,bool PackageMode,const char *ConfigFileName,ConfigOptions& CfgOpt,int& ArgStart){

  //Variables
  int i,j;
  int OptIndex;
  bool Error;
  String OptionName;
  String ModulePath;
  Array<OptValue> Opt;
  
  //Read options in packaged mode
  if(PackageMode){

    //Update default values from configuration file (it is not impletant if config file does not exist)
    _ParseConfigFile(ExecPath,ConfigFileName,true);

    //Init options with default values
    Opt.Reset();
    for(i=0;i<_OptNr;i++){ Opt.Add(_Opt[i].Default); }

    //Set start of arguments for application (from beginning)
    ArgStart=0;

  }

  //Read options in normal mode
  else{

    //Update default values from configuration file
    if(!_ParseConfigFile(ExecPath,ConfigFileName,false)){ return false; }

    //Init options with default values
    Opt.Reset();
    for(i=0;i<_OptNr;i++){ Opt.Add(_Opt[i].Default); }

    //Init application arguments start (just to have a meanningful value, correct value is set below)
    ArgStart=0;

    //Command line argument loop
    for(i=1;i<ArgNr;i++){
    
      //Fetch option
      OptIndex=-1;
      for(j=0;j<_OptNr;j++){
        if((_Opt[j].EnabledSets&OptionSet) && (!_Opt[j].DevelVersOnly || IsDevelopmentVersion())){
          if((_Opt[j].Kind==CmdOptionKind::Regex && String(Arg[i]).Match(_Opt[j].Regex)) 
          || (_Opt[j].Kind==CmdOptionKind::Coded && _Opt[j].Code==String(Arg[i]))){
            OptIndex=j;
            break;
          }
        }
      }
      if(OptIndex==-1){
        SysMessage(245).Print(Arg[i]);
        return false;
      }

      //Get option name (used in messages)
      if(_Opt[OptIndex].Kind==CmdOptionKind::Regex){
        OptionName="<"+_Opt[OptIndex].ShortName.Lower().Replace(" ","")+">";
      }
      else{
        OptionName=_Opt[OptIndex].Code; 
      }

      //Must skip option code for non boolean named options
      if(_Opt[OptIndex].DataType!=OptionType::Boolean && _Opt[OptIndex].Kind==CmdOptionKind::Coded){
        if(i+1>=ArgNr){
          SysMessage(241).Print(_Opt[j].ShortName,OptionName);
          return false;
        }
        i++;
      }

      //Get option value
      switch(_Opt[OptIndex].DataType){
        case OptionType::Boolean: 
          Opt[OptIndex]=OptValue(true);
          break;
        case OptionType::String:
          Opt[OptIndex]=OptValue(String(Arg[i])); 
          break;
        case OptionType::Number:
          Opt[OptIndex]=OptValue(String(Arg[i]).ToLong(Error)); 
          if(Error){
            SysMessage(242).Print(OptionName);
            return false;
          }
          break;
      }

      //Set application arguments start and stop parsing options
      if(_Opt[OptIndex].ArgStart){
        ArgStart=i;
        break;
      }

    }

  }

  //Copy options to output struct
  CfgOpt.SourceFile=Opt[(int)CmdOption::SourceFile].Str;
  CfgOpt.OutputFile=Opt[(int)CmdOption::OutputFile].Str;
  CfgOpt.CompileToApp=Opt[(int)CmdOption::CompileToApp].Bol;
  CfgOpt.EnableAsmFile=Opt[(int)CmdOption::EnableAsmFile].Bol;
  CfgOpt.StripSymbols=Opt[(int)CmdOption::StripSymbols].Bol;
  CfgOpt.CompilerStats=Opt[(int)CmdOption::CompilerStats].Bol;
  CfgOpt.LinterMode=Opt[(int)CmdOption::LinterMode].Bol;
  CfgOpt.MaxErrorNr=Opt[(int)CmdOption::MaxErrorNr].Num;
  CfgOpt.MaxWarningNr=Opt[(int)CmdOption::MaxWarningNr].Num;
  CfgOpt.PassOnWarnings=Opt[(int)CmdOption::PassOnWarnings  ].Bol;
  CfgOpt.LibrInfo=Opt[(int)CmdOption::LibrInfo].Str;
  CfgOpt.ExecInfo=Opt[(int)CmdOption::ExecInfo].Str;
  CfgOpt.BinaryFile=Opt[(int)CmdOption::BinaryFile].Str;
  CfgOpt.MemoryUnitKB=Opt[(int)CmdOption::MemoryUnitKB].Num;
  CfgOpt.StartUnits=Opt[(int)CmdOption::StartUnits].Num;
  CfgOpt.ChunkUnits=Opt[(int)CmdOption::ChunkUnits].Num;
  CfgOpt.LockMemory=Opt[(int)CmdOption::LockMemory].Bol;
  CfgOpt.BenchMark=Opt[(int)CmdOption::BenchMark].Num;
  CfgOpt.IncludePath=Opt[(int)CmdOption::IncludePath].Str;
  CfgOpt.LibraryPath=Opt[(int)CmdOption::LibraryPath].Str;
  CfgOpt.TmpLibPath=Opt[(int)CmdOption::TmpLibPath].Str;
  CfgOpt.DynLibPath=Opt[(int)CmdOption::DynLibPath].Str;
  CfgOpt.DisassembleFile=Opt[(int)CmdOption::DisassembleFile].Str;
  CfgOpt.VersionInfo=Opt[(int)CmdOption::VersionInfo].Bol;
  CfgOpt.DebugLevelIds=Opt[(int)CmdOption::DebugLevelIds].Str;
  CfgOpt.AllDebugLevels=Opt[(int)CmdOption::AllDebugLevels].Bol;
  CfgOpt.DebugToConsole=Opt[(int)CmdOption::DebugToConsole].Bol;

  //System paths need to be prefixed with module path when they start by "." (current folder)
  ModulePath=_Stl->FileSystem.GetDirName(ExecPath,false);
  CfgOpt.IncludePath=(CfgOpt.IncludePath[0]=='.' && ModulePath.Length()!=0?ModulePath+CfgOpt.IncludePath.CutLeft(1):CfgOpt.IncludePath);
  CfgOpt.LibraryPath=(CfgOpt.LibraryPath[0]=='.' && ModulePath.Length()!=0?ModulePath+CfgOpt.LibraryPath.CutLeft(1):CfgOpt.LibraryPath);
  CfgOpt.TmpLibPath =(CfgOpt.TmpLibPath [0]=='.' && ModulePath.Length()!=0?ModulePath+CfgOpt.TmpLibPath .CutLeft(1):CfgOpt.TmpLibPath );
  CfgOpt.DynLibPath =(CfgOpt.DynLibPath [0]=='.' && ModulePath.Length()!=0?ModulePath+CfgOpt.DynLibPath .CutLeft(1):CfgOpt.DynLibPath );

  //Complete additional specific checks
  if(!_Check(OptionSet,PackageMode,CfgOpt)){ return false; }

  //Return code
  return true;

}

//Print help for options
void ConfigParser::PrintHelp(const String& ModuleName,const String& AppId){

  //Constants
  const int OptMaxLen=120;
  const int OptLeftJust=17;
  const String ProgArgStr="[arg] ...";
  const String ProgArgDescription="Command line arguments";

  //Variables
  int i;
  int OptionPadding;
  int OptionSets;
  String Title;
  String OptStr;
  Array<String> Options;
  String CompilerOptions;
  String RuntimeOptions;
  String CompnRunOptions;
  String LibrInfoOptions;
  String ExecInfoOptions;
  String DisassemOptions;
  String VersInfoOptions;
  int CompilerOptLen;
  int RuntimeOptLen;
  int CompnRunOptLen;
  int LibrInfoOptLen;
  int ExecInfoOptLen;
  int DisassemOptLen;
  int VersInfoOptLen;
  String OptionList;
  String ModuleFile;

  //Parse configuration file to get updated default values in _Opt[] table
  _ParseConfigFile(ModuleName,CONFIG_FILE,true);

  //Get option strings
  for(i=0;i<_OptNr;i++){
    if(_Opt[i].Kind==CmdOptionKind::Regex){
      Options.Add("<"+_Opt[i].ShortName.Lower().Replace(" ","")+">");
    }
    else{
      Options.Add(_Opt[i].Code+(_Opt[i].DataType!=OptionType::Boolean?" <"+_Opt[i].ShortName.Lower().Replace(" ","")+">":""));
    }
  }
  for(i=0,OptionPadding=0;i<_OptNr;i++){ OptionPadding=(Options[i].Length()>OptionPadding?Options[i].Length():OptionPadding); }
  
  //Get File name in executable module
  ModuleFile=_Stl->FileSystem.GetFileNameNoExt(ModuleName);
  
  //Build option lists
  OptionList="";
  CompilerOptLen=0;
  RuntimeOptLen=0;
  CompnRunOptLen=0;
  LibrInfoOptLen=0;
  ExecInfoOptLen=0;
  DisassemOptLen=0;
  VersInfoOptLen=0;
  CompilerOptions=ModuleFile+" ";
  RuntimeOptions =ModuleFile+" ";
  CompnRunOptions=ModuleFile+" ";
  LibrInfoOptions=ModuleFile+" ";
  ExecInfoOptions=ModuleFile+" ";
  DisassemOptions=ModuleFile+" ";
  VersInfoOptions=ModuleFile+" ";
  for(i=0;i<_OptNr;i++){
    OptStr=Options[i];
    if(!_Opt[i].IsMandatory){ OptStr="["+OptStr+"]"; }
    if((_Opt[i].EnabledSets&OPSCOM) && (!_Opt[i].DevelVersOnly || IsDevelopmentVersion())){ 
      if(CompilerOptLen+OptStr.Length()+1>OptMaxLen){ CompilerOptions+="\n"+String(OptLeftJust,' ')+OptStr+" "; CompilerOptLen=0; }
      else{ CompilerOptions+=OptStr+" "; CompilerOptLen+=OptStr.Length()+1; }
    }
    if((_Opt[i].EnabledSets&OPSRUN) && (!_Opt[i].DevelVersOnly || IsDevelopmentVersion())){ 
      if(RuntimeOptLen+OptStr.Length()+1>OptMaxLen){ RuntimeOptions+="\n"+String(OptLeftJust,' ')+OptStr+" "; RuntimeOptLen =0; }
      else{ RuntimeOptions+=OptStr+" "; RuntimeOptLen+=OptStr.Length()+1; }
    }
    if((_Opt[i].EnabledSets&OPSCNR) && (!_Opt[i].DevelVersOnly || IsDevelopmentVersion())){ 
      if(CompnRunOptLen+OptStr.Length()+1>OptMaxLen){ CompnRunOptions+="\n"+String(OptLeftJust,' ')+OptStr+" "; CompnRunOptLen=0; }
      else{ CompnRunOptions+=OptStr+" "; CompnRunOptLen+=OptStr.Length()+1; }
    }
    if((_Opt[i].EnabledSets&OPSLIF) && (!_Opt[i].DevelVersOnly || IsDevelopmentVersion())){ 
      if(LibrInfoOptLen+OptStr.Length()+1>OptMaxLen){ LibrInfoOptions+="\n"+String(OptLeftJust,' ')+OptStr+" "; LibrInfoOptLen=0; }
      else{ LibrInfoOptions+=OptStr+" "; LibrInfoOptLen+=OptStr.Length()+1; }
    }
    if((_Opt[i].EnabledSets&OPSXIF) && (!_Opt[i].DevelVersOnly || IsDevelopmentVersion())){ 
      if(ExecInfoOptLen+OptStr.Length()+1>OptMaxLen){ ExecInfoOptions+="\n"+String(OptLeftJust,' ')+OptStr+" "; ExecInfoOptLen=0; }
      else{ ExecInfoOptions+=OptStr+" "; ExecInfoOptLen+=OptStr.Length()+1; }
    }
    if((_Opt[i].EnabledSets&OPSDIS) && (!_Opt[i].DevelVersOnly || IsDevelopmentVersion())){ 
      if(DisassemOptLen+OptStr.Length()+1>OptMaxLen){ DisassemOptions+="\n"+String(OptLeftJust,' ')+OptStr+" "; DisassemOptLen=0; }
      else{ DisassemOptions+=OptStr+" "; DisassemOptLen+=OptStr.Length()+1; }
    }
    if((_Opt[i].EnabledSets&OPSVER) && (!_Opt[i].DevelVersOnly || IsDevelopmentVersion())){ 
      if(VersInfoOptLen+OptStr.Length()+1>OptMaxLen){ VersInfoOptions+="\n"+String(OptLeftJust,' ')+OptStr+" "; VersInfoOptLen=0; }
      else{ VersInfoOptions+=OptStr+" "; VersInfoOptLen+=OptStr.Length()+1; }
    }
  }
  
  //Add program arguments into runtime and compile&run option sets
  OptStr=ProgArgStr;
  if(RuntimeOptLen+OptStr.Length()+1>OptMaxLen){ RuntimeOptions+="\n"+String(OptLeftJust,' ')+OptStr+" "; RuntimeOptLen =0; }
  else{ RuntimeOptions+=OptStr+" "; RuntimeOptLen+=OptStr.Length()+1; }
  if(CompnRunOptLen+OptStr.Length()+1>OptMaxLen){ CompnRunOptions+="\n"+String(OptLeftJust,' ')+OptStr+" "; CompnRunOptLen=0; }
  else{ CompnRunOptions+=OptStr+" "; CompnRunOptLen+=OptStr.Length()+1; }

  //Set selected option list and help title
  if(AppId==DUNS_APPID){
    OptionSets=OPSCNR|OPSRUN|OPSLIF|OPSXIF|OPSDIS|OPSVER;
    Title=MASTER_NAME " compiler and runtime environment";
    OptionList+="Compile & run  : "+CompnRunOptions+"\n";
    OptionList+="Execute binary : "+RuntimeOptions+"\n";
    OptionList+="Library info   : "+LibrInfoOptions+"\n";
    OptionList+="Execut. info   : "+ExecInfoOptions+"\n";
    OptionList+="Disassemble    : "+DisassemOptions+"\n";
    OptionList+="Version info   : "+VersInfoOptions;
  }
  else if(AppId==DUNC_APPID){
    OptionSets=OPSCOM|OPSLIF|OPSXIF|OPSVER;
    Title=MASTER_NAME " compiler";
    OptionList+="Compile source : "+CompilerOptions+"\n";
    OptionList+="Library info   : "+LibrInfoOptions+"\n";
    OptionList+="Execut. info   : "+ExecInfoOptions+"\n";
    OptionList+="Version info   : "+VersInfoOptions;
  }
  else if(AppId==DUNR_APPID){
    OptionSets=OPSRUN|OPSDIS|OPSVER;
    Title=MASTER_NAME " runtime environment";
    OptionList+="Execute binary : "+RuntimeOptions+"\n";
    OptionList+="Disassemble    : "+DisassemOptions+"\n";
    OptionList+="Version info   : "+VersInfoOptions;
  }

  //Print header
  _Stl->Console.PrintLine(String(SPLASH_BANNER));
  _Stl->Console.PrintLine(Title+" "+ToString(GetArchitecture())+"bit "+HostSystemName(GetHostSystem())+" - "+GetVersionName()+" version"+
  (IsProfilerMode()?" (profiler)":"")+(IsSanitizerMode()?" (sanitizer)":"")+(IsPerformanceToolMode()?" (perftool)":"")+
  (IsRuntimeExpandedMode()?" (expanded)":"")+(IsCoverageMode()?" (coverage)":"")+(IsNoOptimizationMode()?" (no-opt)":""));
  _Stl->Console.PrintLine("GitHub: "+String(GITHUB_URL));
  _Stl->Console.PrintLine("");

  //Print option strings
  _Stl->Console.PrintLine(OptionList);
  _Stl->Console.PrintLine("");

  //Print option list
  for(i=0;i<_OptNr;i++){
    if((_Opt[i].EnabledSets&OptionSets) && (!_Opt[i].DevelVersOnly || IsDevelopmentVersion())){
      _Stl->Console.PrintLine(Options[i].LJust(OptionPadding)+" - "+_Opt[i].Description.Replace("<defvalue>",_Opt[i].Default.AsString(_Opt[i].DataType)));
    }
  }
  if((OptionSets&OPSRUN) || (OptionSets&OPSCNR)){
    _Stl->Console.PrintLine(ProgArgStr.LJust(OptionPadding)+" - "+ProgArgDescription);
  }
  _Stl->Console.PrintLine("");
  
  //Print build date/time and number
  _Stl->Console.PrintLine("Build date/time: "+BuildDateTime());
  _Stl->Console.PrintLine("Build number...: "+BuildNumber(AppId));

}

//Print version
void ConfigParser::PrintVersion(const String& AppId){
  String Options
  =(IsDevelopmentVersion()?"("+GetVersionName().Lower()+" version)":"")
  +(IsProfilerMode()?"[profiler]":"")
  +(IsSanitizerMode()?"[sanitizer]":"")
  +(IsPerformanceToolMode()?"[perftool]":"")
  +(IsRuntimeExpandedMode()?"[expanded]":"")
  +(IsCoverageMode()?"[coverage]":"")
  +(IsNoOptimizationMode()?"[no-opt]":"");
  _Stl->Console.PrintLine(String(MASTER_NAME)+" v"+String(VERSION_NUMBER)+" - "+ToString(GetArchitecture())+"bit "+HostSystemName(GetHostSystem())+(Options.Length()!=0?" "+Options:""));
}

//Enable debug levels
bool ConfigParser::EnableDebugLevels(const String& AppId,String DebugLevelIds,bool AllDebugLevels,bool DebugToConsole){

  //Variables
  int i;
  DebugLevel Level;

  //Enable debug levels
  if(AllDebugLevels){
    for(i=0;i<DEBUG_LEVELS;i++){
      if(((DebugMessages::IsCompilerEnabled((DebugLevel)i) && (AppId==DUNC_APPID || AppId==DUNS_APPID)) 
      ||  (DebugMessages::IsRuntimeEnabled((DebugLevel)i)  && (AppId==DUNR_APPID || AppId==DUNS_APPID)))
      && DebugMessages::IsGloballyEnabled((DebugLevel)i)){
        DebugMessages::SetLevel((DebugLevel)i);
      }
    }
  }
  else{
    for(i=0;i<DebugLevelIds.Length();i++){
      if((Level=DebugMessages::Id2Level(DebugLevelIds[i]))==(DebugLevel)-1){
        SysMessage(219).Print(String(DebugLevelIds[i]));
        return false;
      }
      if((!DebugMessages::IsCompilerEnabled(Level) && AppId==DUNC_APPID)
      || (!DebugMessages::IsRuntimeEnabled(Level)  && AppId==DUNR_APPID)){
        SysMessage(219).Print(String(DebugLevelIds[i]));
        return false;
      }
      DebugMessages::SetLevel(Level);
    }
  }

  //Debug mesages to console
  if(DebugToConsole){
    DebugMessages::SetConsoleOutput();
  }

  //Enable system allocator debug levels
  if(DebugLevelEnabled(DebugLevel::SysAllocator)){ _AloCtr.SetDispAlloc(true,DebugMessages::Level2Id(DebugLevel::SysAllocator)); }
  if(DebugLevelEnabled(DebugLevel::SysAllocControl)){ _AloCtr.SetDispCntrl(true,DebugMessages::Level2Id(DebugLevel::SysAllocControl)); }

  //Return success
  return true;

}

//Show compiler debug levels
void ConfigParser::ShowDebugLevels(const String& AppId){
  
  //Variables
  int i;
  DebugLevel Level;

  //Print debug levels
  _Stl->Console.PrintLine("Available debug levels:");
  for(i=0;i<DEBUG_LEVELS;i++){
    Level=(DebugLevel)i;
    if((DebugMessages::IsCompilerEnabled(Level) && (AppId==DUNC_APPID || AppId==DUNS_APPID))
    || (DebugMessages::IsRuntimeEnabled(Level)  && (AppId==DUNR_APPID || AppId==DUNS_APPID))){
      _Stl->Console.PrintLine(String(DebugMessages::Level2Id(Level))+": "+DebugMessages::LevelText(Level)+(!DebugMessages::IsGloballyEnabled(Level)?" (*)":"")); 
    }
  }
  _Stl->Console.PrintLine("(*) These levels are not enabled by default through -dx option");

}