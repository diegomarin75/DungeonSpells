//Dungeon spells compiler and rutime environment

//Include files
#include "bas/basedefs.hpp"
#include "bas/allocator.hpp"
#include "bas/array.hpp"
#include "bas/stack.hpp"
#include "bas/queue.hpp"
#include "bas/buffer.hpp"
#include "bas/string.hpp"
#include "sys/sysdefs.hpp"
#include "sys/system.hpp"
#include "sys/stl.hpp"
#include "sys/msgout.hpp"
#include "vrm/pool.hpp"
#include "vrm/memman.hpp"
#include "vrm/auxmem.hpp"
#include "vrm/strcomp.hpp"
#include "vrm/arrcomp.hpp"
#include "vrm/runtime.hpp"
#include "vrm/scheduler.hpp" 
#include "cfgparser.hpp"

//ROM file buffer
RomFileBuffer _RomBuffer=(RomFileBuffer){ROM_BUFF_LABEL,0,"",0,"",0x77FF};

//System process id
const int _SystemProcessId=0;

//Detect running in application package mode
bool IsPackageMode(const String& ExecName);

//Main program
int main(int argc,char *argv[]){

  //Variables
  int i;
  int ArgStart;
  String ConfigFileName;
  int OptionSet;
  ConfigOptions CmdOpt;
  StlSubsystem Stl;
  String ExecPath;

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

    //Running in application package mode
    if(IsPackageMode(ExecPath)){

      //Get configuation file name
      ConfigFileName=_Stl->FileSystem.GetDirName(ExecPath)+"_"+_Stl->FileSystem.GetFileNameNoExt(ExecPath)+APP_PKG_CONFIG_EXT;

      //Get command line arguments
      if(!ConfigParser::GetCmdLineOptions(ExecPath,argc,argv,OPSRUN,true,ConfigFileName.CharPnt(),CmdOpt,ArgStart)){ return 0; }

      //Call runtime environment main
      if(!CallRuntime(CmdOpt.BinaryFile,CmdOpt.MemoryUnitKB,CmdOpt.StartUnits,CmdOpt.ChunkUnits,CmdOpt.LockMemory,CmdOpt.BenchMark,CmdOpt.DynLibPath,CmdOpt.TmpLibPath,argc,argv,ArgStart,&_RomBuffer)){ return 0; }

    }

    //Running in normal mode
    else{

      //Show help
      if(argc==1){ ConfigParser::PrintHelp(ExecPath,DUNR_APPID); return 0; }

      //Show debug levels
      if(IsDevelopmentVersion() && argc==2 && argv[1]==String("-dh")){ ConfigParser::ShowDebugLevels(DUNR_APPID); return 0; }

      //Determine option set
      if(String(argv[1])=="-da"){
        OptionSet=OPSDIS;
      }
      else if(String(argv[1])=="-ve"){
        OptionSet=OPSVER;
      }
      else{
        OptionSet=OPSNUL;
        for(i=1;i<argc;i++){
          if(String(argv[i]).Match(EXEC_REGEX)){
            OptionSet=OPSRUN;
            break;
          }
        }
        if(OptionSet==OPSNUL){
          SysMessage(358).Print(EXECUTABLE_EXT);
          return 0;
        }
      }

      //Execute mode
      switch(OptionSet){
        
        //Disassemble
        case OPSDIS:
          if(!ConfigParser::GetCmdLineOptions(ExecPath,argc,argv,OptionSet,false,CONFIG_FILE,CmdOpt,ArgStart)){ return 0; }
          if(!ConfigParser::EnableDebugLevels(DUNR_APPID,CmdOpt.DebugLevelIds,CmdOpt.AllDebugLevels,CmdOpt.DebugToConsole)){ return 0; }
          if(!CallDisassembleFile(CmdOpt.DisassembleFile,CmdOpt.MemoryUnitKB,CmdOpt.StartUnits,CmdOpt.ChunkUnits,argc,argv,ArgStart)){ return 0; }
          break;

        //Run
        case OPSRUN:
          if(!ConfigParser::GetCmdLineOptions(ExecPath,argc,argv,OptionSet,false,CONFIG_FILE,CmdOpt,ArgStart)){ return 0; }
          if(!ConfigParser::EnableDebugLevels(DUNR_APPID,CmdOpt.DebugLevelIds,CmdOpt.AllDebugLevels,CmdOpt.DebugToConsole)){ return 0; }
          if(!CallRuntime(CmdOpt.BinaryFile,CmdOpt.MemoryUnitKB,CmdOpt.StartUnits,CmdOpt.ChunkUnits,CmdOpt.LockMemory,CmdOpt.BenchMark,CmdOpt.DynLibPath,CmdOpt.TmpLibPath,argc,argv,ArgStart,nullptr)){ return 0; }
          break;

        //Version info
        case OPSVER:
          ConfigParser::PrintVersion(DUNR_APPID);
          break;

      }

    }
  
  }

  //Exception handler
  catch(BaseException& Ex){
    _Stl->Console.PrintLine(Ex.Description());
    return 0;
  }

  //Return code
  return 0;

}

//Detect when dunr runs in application package mode
bool IsPackageMode(const String& ExecName){
  return (_Stl->FileSystem.GetFileName(ExecName).Upper()!=String(APP_PKG_CONTAINER).Upper() && _RomBuffer.Loaded?true:false);
}