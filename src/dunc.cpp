
//Dungeon spells compiler and rutime environment

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
#include "cmp/parser.hpp"
#include "cmp/binary.hpp"
#include "cmp/master.hpp"
#include "cmp/exprcomp.hpp"
#include "cmp/compiler.hpp"
#include "cfgparser.hpp"

//System process id
const int _SystemProcessId=0;

//Main program
int main(int argc,char *argv[]){

  //Variables
  int i;
  int OptionSet;
  int ArgStart;
  bool CompileToLibrary;
  ConfigOptions CmdOpt;
  StlSubsystem Stl;
  String ExecPath;
  String ContainerFile;

  //Catch exceptions
  try{

    //Set system process
    System::PushProcessId(_SystemProcessId);
    
    //Set global bios pointer
    _Stl=&Stl;
    
    //Lock console
    if(!_Stl->Console.Lock()){  std::cout << "Unable to aquire console lock!" << std::endl; return 0; }

    //Get executable path
    if(!GetExecutablePath(ExecPath)){ SysMessage(0).Print(); return 0; }

    //Show help
    if(argc==1){ ConfigParser::PrintHelp(ExecPath,DUNC_APPID); return 0; }

    //Show debug levels
    if(IsDevelopmentVersion() && argc==2 && argv[1]==String("-dh")){ ConfigParser::ShowDebugLevels(DUNC_APPID); return 0; }

    //Determine option set
    if(String(argv[1])=="-lf"){
      OptionSet=OPSLIF;
    }
    else if(String(argv[1])=="-xf"){
      OptionSet=OPSXIF;
    }
    else if(String(argv[1])=="-ve"){
      OptionSet=OPSVER;
    }
    else{
      OptionSet=OPSNUL;
      for(i=1;i<argc;i++){
        if(String(argv[i]).Match(SOUR_REGEX)){
          OptionSet=OPSCOM;
          break;
        }
      }
      if(OptionSet==OPSNUL){
        SysMessage(282).Print(SOURCE_EXT);
        return 0;
      }
    }

    //Execute mode
    switch(OptionSet){

      //Library info
      case OPSLIF:
        if(!ConfigParser::GetCmdLineOptions(ExecPath,argc,argv,OptionSet,false,CONFIG_FILE,CmdOpt,ArgStart)){ return 0; }
        if(!ConfigParser::EnableDebugLevels(DUNC_APPID,CmdOpt.DebugLevelIds,CmdOpt.AllDebugLevels,CmdOpt.DebugToConsole)){ return 0; }
        if(!CallLibraryInfo(CmdOpt.LibrInfo)){ return 0; }
        break;

      //Executable info
      case OPSXIF:
        if(!ConfigParser::GetCmdLineOptions(ExecPath,argc,argv,OptionSet,false,CONFIG_FILE,CmdOpt,ArgStart)){ return 0; }
        if(!ConfigParser::EnableDebugLevels(DUNC_APPID,CmdOpt.DebugLevelIds,CmdOpt.AllDebugLevels,CmdOpt.DebugToConsole)){ return 0; }
        if(!CallExecutableInfo(CmdOpt.ExecInfo)){ return 0; }
        break;

      //Compile
      case OPSCOM:
        if(!ConfigParser::GetCmdLineOptions(ExecPath,argc,argv,OptionSet,false,CONFIG_FILE,CmdOpt,ArgStart)){ return 0; }
        if(ArgStart<argc-1){ SysMessage(373).Print(); return 0; }
        if(!ConfigParser::EnableDebugLevels(DUNC_APPID,CmdOpt.DebugLevelIds,CmdOpt.AllDebugLevels,CmdOpt.DebugToConsole)){ return 0; }
        if(!CallCompiler(CmdOpt.SourceFile,CmdOpt.OutputFile,CmdOpt.CompileToApp,GetPkgContainer(ExecPath),CmdOpt.EnableAsmFile,CmdOpt.StripSymbols,CmdOpt.CompilerStats,CmdOpt.LinterMode,CmdOpt.MaxErrorNr,CmdOpt.MaxWarningNr,CmdOpt.PassOnWarnings,CmdOpt.IncludePath,CmdOpt.LibraryPath,CmdOpt.DynLibPath,CompileToLibrary)){ return 0; }
        break;

      //Version info
      case OPSVER:
        ConfigParser::PrintVersion(DUNC_APPID);
        break;

    }

  }
  
  //Exception handler
  catch(BaseException& Ex){
    _Stl->Console.PrintLine(Ex.Description());
    return false;
  }

  //Return code
  return 0;

}