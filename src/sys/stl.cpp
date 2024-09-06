//bios.cpp: Implementation of console class
#include "bas/basedefs.hpp"
#include "bas/allocator.hpp"
#include "bas/array.hpp"
#include "bas/stack.hpp"
#include "bas/buffer.hpp"
#include "bas/string.hpp"
#include "sys/sysdefs.hpp"
#include "sys/system.hpp"
#include "sys/stl.hpp"

//Macros
#define ISDATEVALID(date)    (_Serial2Date(_Date2Serial(date))==(date)?true:false)
#define ISTIMEVALID(h,m,s,n) ((h)>=0 && (h)<=23 && (m)>=0 && (m)<=59 && (s)>=0 && (s)<=59 && (n)>=0 && (n)<=999999999LL?true:false)

//Global STL pointer
StlSubsystem* _Stl;

//Month days
const int _MonthDays[12+1]={0,31,28,31,30,31,30,31,31,30,31,30,31};

//File status
enum class StlFileStatus{
  OpenRead,
  OpenWrite,
  Closed
};
    
//File handler table
//(this table is not based on Array<type> class because cannot easily define record copy constructor, std::ifstream / std::ofstream are tricky)
struct StlFileHandler{
  String FileName;
  std::ifstream InpStream;
  std::ofstream OutStream;
  StlFileStatus Status;
  int ProcessId;
  bool Used;
  CpuLon PrevPos;
};
const int _MaxFileHandlers=255;
StlFileHandler _File[_MaxFileHandlers];

//Internal functions
CpuInt _Date2Serial(CpuInt Date);
CpuInt _Serial2Date(CpuInt Serial);

//Constructor
StlSubsystem::StlSubsystem(){
  
  //Set main ref in subclasses
  Console.SetMainPtr(this);
  FileSystem.SetMainPtr(this);
  Math.SetMainPtr(this);
  DateTime.SetMainPtr(this);

  //Init rest of variables
  _ErrorCode=StlErrorCode::Ok;
  _ErrorInfo="";

}

//Destructor
StlSubsystem::~StlSubsystem(){}

//Set bios error
void StlSubsystem::ClearError(){ _ErrorCode=StlErrorCode::Ok; _ErrorInfo=""; }
void StlSubsystem::SetError(StlErrorCode Code){ _ErrorCode=Code; _ErrorInfo=""; }
void StlSubsystem::SetError(StlErrorCode Code,const String& Info){ _ErrorCode=Code; _ErrorInfo=Info; }

//Set reference to main class
void StlSubsystem::StlConsole::SetMainPtr(StlSubsystem *Bios){
  _Main=Bios;  
}

//Constructor
StlSubsystem::StlConsole::StlConsole(){
  _LockTable.Reset();
}

//Destructor
StlSubsystem::StlConsole::~StlConsole(){}

//Internal print function
void StlSubsystem::StlConsole::_Print(const char *Pnt,bool NewLine){
  if(_LockTable.Top().ProcessId!=System::CurrentProcessId() && _LockOverride==false){
    System::Throw(SysExceptionCode::ConsoleLockedByOtherProcess,ToString(System::CurrentProcessId()));  
  }
  else{
    if(NewLine){
      std::cout << Pnt << std::endl;
    }
    else{
      std::cout << Pnt << std::flush;
    }
  }
  _LockOverride=false;
}

//Internal print error function
void StlSubsystem::StlConsole::_PrintError(const char *Pnt,bool NewLine){
  if(_LockTable.Top().ProcessId!=System::CurrentProcessId() && _LockOverride==false){
    System::Throw(SysExceptionCode::ConsoleLockedByOtherProcess,ToString(System::CurrentProcessId()));  
  }
  else{
    if(NewLine){
      std::cerr << Pnt << std::endl;
    }
    else{
      std::cerr << Pnt << std::flush;
    }
  }
  _LockOverride=false;
}

//Lock
bool StlSubsystem::StlConsole::Lock(){
  if(_LockTable.Length()>0){
    if(_LockTable.Top().ProcessId==System::CurrentProcessId()){
      return false;
    }
  }
  _LockTable.Push((StlConsoleLock){System::CurrentProcessId()});
  return true;
}

//Unlock
bool StlSubsystem::StlConsole::Unlock(){
  if(_LockTable.Length()==0){
    return false;
  }
  if(_LockTable.Top().ProcessId!=System::CurrentProcessId()){
      return false;
  }
  _LockTable.Pop();
  return true;
}

//Lock override
void StlSubsystem::StlConsole::Override(){
  _LockOverride=true;
}

//Print
void StlSubsystem::StlConsole::Print(const char *Pnt){
  _Print(Pnt,false);
}

//Print
void StlSubsystem::StlConsole::Print(const String& Str){
  _Print(Str.CharPnt(),false);
}

//PrintLine
void StlSubsystem::StlConsole::PrintLine(const char *Pnt){
  _Print(Pnt,true);
}

//PrintLine
void StlSubsystem::StlConsole::PrintLine(const String& Str){
  _Print(Str.CharPnt(),true);
}


//Print
void StlSubsystem::StlConsole::PrintError(const char *Pnt){
  _PrintError(Pnt,false);
}

//Print
void StlSubsystem::StlConsole::PrintError(const String& Str){
  _PrintError(Str.CharPnt(),false);
}

//PrintLine
void StlSubsystem::StlConsole::PrintErrorLine(const char *Pnt){
  _PrintError(Pnt,true);
}

//PrintLine
void StlSubsystem::StlConsole::PrintErrorLine(const String& Str){
  _PrintError(Str.CharPnt(),true);
}

//Print table
void StlSubsystem::StlConsole::PrintTable(const String& Headings,const Array<String>& Rows,const String& Delimiter,const String& Padding){

  //Constants
  #define HSEP '-'
  #define VSEP '|'

  //Variables
  int Columns;
  int TotalWidth;
  char ColPadding;
  String Separator;
  Array<String> Head;
  Array<String> Line;
  Array<int> Width;

  //Get number of columns from headings
  Head=Headings.Split(Delimiter);
  Columns=Head.Length();

  //Calculate maximun column widths
  Width.Resize(Columns);
  for(int i=0;i<Columns;i++){ Width[i]=Head[i].Length(); }
  for(int i=0;i<Rows.Length();i++){ 
    Line=Rows[i].Split(Delimiter); 
    for(int j=0;j<Columns;j++){ if(j<=Line.Length()-1){ if(Line[j].Length()>Width[j]){ Width[j]=Line[j].Length(); } } }
  }
  TotalWidth=Columns*3+1;
  for(int i=0;i<Columns;i++){ TotalWidth+=Width[i]; }

  //Calculate Delimiter line length
  Separator=String(TotalWidth,HSEP);

  //Print column headings
  PrintLine(Separator);
  Print(VSEP);
  for(int i=0;i<Columns;i++){ 
    Print(" "+Head[i].LJust(Width[i])+" "+String(1,VSEP));
  }
  PrintLine("");
  PrintLine(Separator);

  //Print rows
  for(int i=0;i<Rows.Length();i++){ 
    Line=Rows[i].Split(Delimiter); 
    Print(VSEP);
    for(int j=0;j<Columns;j++){
      if(Padding.Length()!=0 && j<=Padding.Length()-1){
        ColPadding=Padding[j];
      }
      else{
        if(Line[j].ContainsOnly("01234567890+-.% ") && !Line[j].ContainsOnly("+-.% ")){ ColPadding='L'; } else{ ColPadding='R'; }
      }
      if(ColPadding=='L'){
        Print(" "+Line[j].RJust(Width[j])+" "+String(1,VSEP));
      }
      else{
        Print(" "+Line[j].LJust(Width[j])+" "+String(1,VSEP));
      }
    }
    PrintLine("");
  }
  PrintLine(Separator);

}

//Set pointer to parent class
void StlSubsystem::StlFileSystem::SetMainPtr(StlSubsystem *Bios){
  _Main=Bios;
}
        
//Path delimiter
String StlSubsystem::StlFileSystem::Delimiter(){
  String Delim;
  switch(GetHostSystem()){
    case HostSystem::Linux:   Delim=LIN_PATH_DELIMITER; break;
    case HostSystem::Windows: Delim=WIN_PATH_DELIMITER; break;
  }
  return Delim;
}

//Normalize file path
String StlSubsystem::StlFileSystem::Normalize(const String& FilePath){
  String NormalPath;
  switch(GetHostSystem()){
    case HostSystem::Linux:   NormalPath=FilePath.Replace(WIN_PATH_DELIMITER,LIN_PATH_DELIMITER); break;
    case HostSystem::Windows: NormalPath=FilePath.Replace(LIN_PATH_DELIMITER,WIN_PATH_DELIMITER); break;
  }
  return NormalPath;
}

//Get file name
String StlSubsystem::StlFileSystem::GetFileName(const String& FilePath){
  return Normalize(FilePath).Split(Delimiter()).Last();
}

//Get file name without extension
String StlSubsystem::StlFileSystem::GetFileNameNoExt(const String& FilePath){
  String FileName = Normalize(FilePath).Split(Delimiter()).Last();
  Array<String> Parts=FileName.Split(".");
  if(Parts.Length()>1){
    FileName="";
    for(int i=0;i<Parts.Length()-1;i++){
      FileName+=((FileName.Length()==0?"":".")+Parts[i]);
    }
  }
  return FileName;
}

//Get file extension
String StlSubsystem::StlFileSystem::GetFileExtension(const String& FilePath){
  String Extension;
  String FileName = Normalize(FilePath).Split(Delimiter()).Last();
  if(FileName.Search(".")==-1){
    Extension="";
  }
  else{
    Extension="."+FileName.Split(".").Last();
  }
  return Extension;
}

//Get folder name
String StlSubsystem::StlFileSystem::GetDirName(const String& FilePath,bool EndByDelimiter){
  String Folder="";
  Array<String> Parts=Normalize(FilePath).Split(Delimiter());
  if(Parts.Length()>1){
    Folder="";
    for(int i=0;i<Parts.Length()-1;i++){
      Folder+=Parts[i]+Delimiter();
    }
  }
  if(!EndByDelimiter){ Folder=Folder.CutRight(Delimiter().Length()); }
  return Folder;

}

//Check file exists
bool StlSubsystem::StlFileSystem::FileExists(const String& Path){
  struct stat Info;
  if(stat(Path.CharPnt(),&Info)!=0){
    return false;
  }
  if(Info.st_mode & S_IFDIR){
    return false;
  }
  else{
    return true;
  }
}

//Check dir exists
bool StlSubsystem::StlFileSystem::DirExists(const String& Path){
  struct stat Info;
  if(stat(Path.CharPnt(),&Info)!=0){
    return false;
  }
  if(Info.st_mode & S_IFDIR){
    return true;
  }
  else{
    return false;
  }
}

//Constructor from char String
StlSubsystem::StlFileSystem::StlFileSystem(){
  for(int i=0;i<_MaxFileHandlers;i++){
    _File[i].FileName="";
    _File[i].Status=StlFileStatus::Closed;
    _File[i].Used=false;
    _File[i].ProcessId=-1;
    _File[i].PrevPos=-1;
  }
}

//Destructor
StlSubsystem::StlFileSystem::~StlFileSystem(){
  for(int i=0;i<_MaxFileHandlers;i++){
    if(_File[i].Status!=StlFileStatus::Closed){ _ForcedFileClose(i); }
  }
}

//Forced file close
void StlSubsystem::StlFileSystem::_ForcedFileClose(int Hnd){
  if(_File[Hnd].Status==StlFileStatus::OpenRead){
    _File[Hnd].InpStream.close();
  }
  else if(_File[Hnd].Status==StlFileStatus::OpenWrite){
    _File[Hnd].OutStream.close();
  }
  _File[Hnd].FileName="";
  _File[Hnd].Status=StlFileStatus::Closed;
}

//Get handler
bool StlSubsystem::StlFileSystem::GetHandler(int& Hnd){
  
  //Variables
  int Index=-1;
  
  //Find non used handler
  for(int i=0;i<_MaxFileHandlers;i++){
    if(!_File[i].Used){
      Index=i;
      break;
    }
  }
  if(Index!=-1){
    Hnd=Index;
    _File[Hnd].FileName="";
    _File[Hnd].Status=StlFileStatus::Closed;
    _File[Hnd].Used=true;
    _File[Hnd].ProcessId=System::CurrentProcessId();
    _File[Hnd].PrevPos=-1;
    return true;
  }

  //Return error
  return false;

}

//Free handler
bool StlSubsystem::StlFileSystem::FreeHandler(int Hnd){
  if(_File[Hnd].ProcessId!=System::CurrentProcessId()){
    _Main->SetError(StlErrorCode::HandlerForbidden);
    return false; 
  }
  if(_File[Hnd].Status!=StlFileStatus::Closed){
    _Main->SetError(StlErrorCode::FreeHandlerOpenFile);
    return false; 
  }
  _File[Hnd].Used=false;
  _File[Hnd].ProcessId=-1;
  return true;
}

//OpenForRead
bool StlSubsystem::StlFileSystem::OpenForRead(int Hnd,const String& FilePath){
  
  //Variables
  String FileName=Normalize(FilePath);

  //Check if handler belongs to current process
  if(_File[Hnd].ProcessId!=System::CurrentProcessId()){
    _Main->SetError(StlErrorCode::HandlerForbidden);
    return false; 
  }

  //Check file is in use by another process
  for(int i=0;i<_MaxFileHandlers;i++){
    if(_File[i].FileName==FileName){
      if(_File[i].ProcessId==_File[Hnd].ProcessId){ _Main->SetError(StlErrorCode::FileAlreadyOpenSelf); }
      else{ _Main->SetError(StlErrorCode::FileAlreadyOpen); }
      return false;
    }
  }
  
  //Set open mode
  std::ios_base::openmode OpenMode=std::ifstream::binary|std::ifstream::in;

  //Open file
  _File[Hnd].InpStream=std::ifstream(FileName.CharPnt(),OpenMode);
  if(!_File[Hnd].InpStream){ _Main->SetError(StlErrorCode::OpenReadError); return false; }
  
  //Save handler fields
  _File[Hnd].FileName=FileName;
  _File[Hnd].Status=StlFileStatus::OpenRead;

  //Seek position before read/write operations
  _File[Hnd].PrevPos=_File[Hnd].InpStream.tellg();
  
  //Return code
  return true;

}

//OpenForWrite
bool StlSubsystem::StlFileSystem::OpenForWrite(int Hnd,const String& FilePath){

  //Variables
  String FileName=Normalize(FilePath);

  //Check if handler belongs to current process
  if(_File[Hnd].ProcessId!=System::CurrentProcessId()){
    _Main->SetError(StlErrorCode::HandlerForbidden);
    return false; 
  }

  //Check file is in use by another process
  for(int i=0;i<_MaxFileHandlers;i++){
    if(_File[i].FileName==FileName){
      if(_File[i].ProcessId==_File[Hnd].ProcessId){ _Main->SetError(StlErrorCode::FileAlreadyOpenSelf); }
      else{ _Main->SetError(StlErrorCode::FileAlreadyOpen); }
      return false;
    }
  }
  
  //Set open mode
  std::ios_base::openmode OpenMode=std::ifstream::binary|std::ifstream::out;

  //Open file
  _File[Hnd].OutStream=std::ofstream(FileName.CharPnt(),OpenMode);
  if(!_File[Hnd].OutStream){ _Main->SetError(StlErrorCode::OpenWriteError); return false; }
  
  //Save handler fields
  _File[Hnd].FileName=FileName;
  _File[Hnd].Status=StlFileStatus::OpenWrite;
  
  //Seek position before read/write operations
  _File[Hnd].PrevPos=_File[Hnd].OutStream.tellp();

  //Return code
  return true;

}

//OpenForAppend
bool StlSubsystem::StlFileSystem::OpenForAppend(int Hnd,const String& FilePath){

  //Variables
  String FileName=Normalize(FilePath);

  //Check if handler belongs to current process
  if(_File[Hnd].ProcessId!=System::CurrentProcessId()){
    _Main->SetError(StlErrorCode::HandlerForbidden);
    return false; 
  }

  //Check file is in use by another process
  for(int i=0;i<_MaxFileHandlers;i++){
    if(_File[i].FileName==FileName){
      if(_File[i].ProcessId==_File[Hnd].ProcessId){ _Main->SetError(StlErrorCode::FileAlreadyOpenSelf); }
      else{ _Main->SetError(StlErrorCode::FileAlreadyOpen); }
      return false;
    }
  }
  
  //Set open mode
  std::ios_base::openmode OpenMode=std::ifstream::binary|std::ifstream::out|std::ifstream::app;

  //Open file
  _File[Hnd].OutStream=std::ofstream(FileName.CharPnt(),OpenMode);
  if(!_File[Hnd].OutStream.good()){ _Main->SetError(StlErrorCode::OpenWriteError); return false; }
  
  //Save handler fields
  _File[Hnd].FileName=FileName;
  _File[Hnd].Status=StlFileStatus::OpenWrite;
  
  //Seek position before read/write operations
  _File[Hnd].PrevPos=_File[Hnd].OutStream.tellp();

  //Return code
  return true;

}

//Get file size
bool StlSubsystem::StlFileSystem::GetFileSize(int Hnd,CpuLon& Size){

  //File position
  CpuLon Pos;

  //Size of input file
  if(_File[Hnd].Status==StlFileStatus::OpenRead){
    Pos=_File[Hnd].InpStream.tellg();
    _File[Hnd].InpStream.seekg(0,std::ios::end);
    Size=_File[Hnd].InpStream.tellg();  
    _File[Hnd].InpStream.seekg(Pos,std::ios::beg);
  }

  //Error
  else{
    _Main->SetError(StlErrorCode::GetSizeError); 
    return false;
  }

  //Return code
  return true;

}

//Read line
bool StlSubsystem::StlFileSystem::Read(int Hnd,String& Line){

  //Check if handler belongs to current process
  if(_File[Hnd].ProcessId!=System::CurrentProcessId()){
    _Main->SetError(StlErrorCode::HandlerForbidden);
    return false; 
  }

  //Check handler
  if(_File[Hnd].Status!=StlFileStatus::OpenRead){ _Main->SetError(StlErrorCode::ReadWhenClosed); return false; }

  //Seek position before read/write operations
  _File[Hnd].PrevPos=_File[Hnd].InpStream.tellg();

  //Read line
  if(!(_File[Hnd].InpStream >> Line)){
    if(!_File[Hnd].InpStream.eof()){
      _Main->SetError(StlErrorCode::ReadError);
      if(!CloseFile(Hnd)){ return false; }
    }
    return false;
  }

  //Return code
  return true;

}

//Write line
bool StlSubsystem::StlFileSystem::Write(int Hnd,const String& Line){

  //Check if handler belongs to current process
  if(_File[Hnd].ProcessId!=System::CurrentProcessId()){
    _Main->SetError(StlErrorCode::HandlerForbidden);
    return false; 
  }

  //Check handler
  if(_File[Hnd].Status!=StlFileStatus::OpenWrite){ _Main->SetError(StlErrorCode::WriteWhenClosed); return false; }

  //Seek position before read/write operations
  _File[Hnd].PrevPos=_File[Hnd].OutStream.tellp();

  //Write line
  if(!(_File[Hnd].OutStream << Line)){ 
    _Main->SetError(StlErrorCode::WriteError);
    if(!CloseFile(Hnd)){ return false; }
    return false; 
  }

  //Return code
  return true;

}

//Write lines
bool StlSubsystem::StlFileSystem::WriteArray(int Hnd,const Array<String>& Lines){

  //Check if handler belongs to current process
  if(_File[Hnd].ProcessId!=System::CurrentProcessId()){
    _Main->SetError(StlErrorCode::HandlerForbidden);
    return false; 
  }

  //Check handler
  if(_File[Hnd].Status!=StlFileStatus::OpenWrite){ _Main->SetError(StlErrorCode::WriteWhenClosed); return false; }

  //Seek position before read/write operations
  _File[Hnd].PrevPos=_File[Hnd].OutStream.tellp();

  //Write lines
  for(int i=0;i<Lines.Length();i++){
    if(!(_File[Hnd].OutStream << Lines[i])){ 
      _Main->SetError(StlErrorCode::WriteError);
      if(!CloseFile(Hnd)){ return false; }
      return false; 
    }
  }

  //Return code
  return true;

}

//Read buffer
bool StlSubsystem::StlFileSystem::Read(int Hnd,Buffer& Buff,long Length){

  //Check if handler belongs to current process
  if(_File[Hnd].ProcessId!=System::CurrentProcessId()){
    _Main->SetError(StlErrorCode::HandlerForbidden);
    return false; 
  }

  //Check handler
  if(_File[Hnd].Status!=StlFileStatus::OpenRead){ _Main->SetError(StlErrorCode::ReadWhenClosed); return false; }

  //Seek position before read/write operations
  _File[Hnd].PrevPos=_File[Hnd].InpStream.tellg();

  //Read buffer
  Buffer Temp=Buffer(Length,0);
  if(!_File[Hnd].InpStream.read(Temp.BuffPnt(),Length)){
    if(!_File[Hnd].InpStream.eof()){
      _Main->SetError(StlErrorCode::ReadError);
      if(!CloseFile(Hnd)){ return false; }
    }
    return false;
  }
  Buff=Temp;

  //Return code
  return true;

}

//Write buffer
bool StlSubsystem::StlFileSystem::Write(int Hnd,const Buffer& Buff){

  //Check if handler belongs to current process
  if(_File[Hnd].ProcessId!=System::CurrentProcessId()){
    _Main->SetError(StlErrorCode::HandlerForbidden);
    return false; 
  }

  //Check handler
  if(_File[Hnd].Status!=StlFileStatus::OpenWrite){ _Main->SetError(StlErrorCode::WriteWhenClosed); return false; }

  //Seek position before read/write operations
  _File[Hnd].PrevPos=_File[Hnd].OutStream.tellp();
  
  //Write buffer
  if(!_File[Hnd].OutStream.write(Buff.BuffPnt(),Buff.Length())){
    _Main->SetError(StlErrorCode::WriteError);
    if(!CloseFile(Hnd)){ return false; }
    return false; 
  }

  //Return code
  return true;

}

//Write buffers
bool StlSubsystem::StlFileSystem::WriteArray(int Hnd,const Array<Buffer>& Buffers){

  //Check if handler belongs to current process
  if(_File[Hnd].ProcessId!=System::CurrentProcessId()){
    _Main->SetError(StlErrorCode::HandlerForbidden);
    return false; 
  }

  //Check handler
  if(_File[Hnd].Status!=StlFileStatus::OpenWrite){ _Main->SetError(StlErrorCode::WriteWhenClosed); return false; }

  //Seek position before read/write operations
  _File[Hnd].PrevPos=_File[Hnd].OutStream.tellp();
  
  //Write buffers
  for(int i=0;i<Buffers.Length();i++){
    if(!_File[Hnd].OutStream.write(Buffers[i].BuffPnt(),Buffers[i].Length())){
      _Main->SetError(StlErrorCode::WriteError);
      if(!CloseFile(Hnd)){ return false; }
      return false; 
    }
  }
  
  //Retun code
  return true;

}

//Read memory
bool StlSubsystem::StlFileSystem::Read(int Hnd,char *Pnt,long Length){

  //Check if handler belongs to current process
  if(_File[Hnd].ProcessId!=System::CurrentProcessId()){
    _Main->SetError(StlErrorCode::HandlerForbidden);
    return false; 
  }

  //Check handler
  if(_File[Hnd].Status!=StlFileStatus::OpenRead){ _Main->SetError(StlErrorCode::ReadWhenClosed); return false; }

  //Seek position before read/write operations
  _File[Hnd].PrevPos=_File[Hnd].InpStream.tellg();
  
  //Read memory
  Buffer Temp=Buffer(Length,0);
  if(!_File[Hnd].InpStream.read(Temp.BuffPnt(),Length)){
    if(!_File[Hnd].InpStream.eof()){
      _Main->SetError(StlErrorCode::ReadError);
      if(!CloseFile(Hnd)){ return false; }
    }
    return false;
  }
  MemCpy(Pnt,Temp.BuffPnt(),Length);

  //Return code
  return true;

}

//Write memory
bool StlSubsystem::StlFileSystem::Write(int Hnd,const char *Pnt,long Length){

  //Check if handler belongs to current process
  if(_File[Hnd].ProcessId!=System::CurrentProcessId()){
    _Main->SetError(StlErrorCode::HandlerForbidden);
    return false; 
  }

  //Check handler
  if(_File[Hnd].Status!=StlFileStatus::OpenWrite){ _Main->SetError(StlErrorCode::WriteWhenClosed); return false; }

  //Seek position before read/write operations
  _File[Hnd].PrevPos=_File[Hnd].OutStream.tellp();

  //Write memory
  if(!_File[Hnd].OutStream.write(Pnt,Length)){
    _Main->SetError(StlErrorCode::WriteError);
    if(!CloseFile(Hnd)){ return false; }
    return false; 
  }

  //Return code
  return true;

}

//Read entire file into buffer
bool StlSubsystem::StlFileSystem::FullRead(int Hnd,Buffer& Buff){

  //Variables
  CpuLon Size;

  //Check if handler belongs to current process
  if(_File[Hnd].ProcessId!=System::CurrentProcessId()){
    _Main->SetError(StlErrorCode::HandlerForbidden);
    return false; 
  }

  //Check status
  if(_File[Hnd].Status!=StlFileStatus::OpenRead){ 
    _Main->SetError(StlErrorCode::ReadWhenClosed); 
    return false; 
  }
  
  //Seek position before read/write operations
  _File[Hnd].PrevPos=_File[Hnd].InpStream.tellg();

  //Get file size
  if(!GetFileSize(Hnd,Size)){ return false; }

  //Read buffer
  //We do not check eof() here as we are reading entire file in one call
  //If we have error it is for sure a read error
  Buffer Temp=Buffer(Size,0);
  if(!_File[Hnd].InpStream.read(Temp.BuffPnt(),Size)){
    _Main->SetError(StlErrorCode::ReadError);
    if(!CloseFile(Hnd)){ return false; }
    return false;
  }
  Buff=Temp;

  //Return code
  return true;

}

//Read full text file into string array
bool StlSubsystem::StlFileSystem::FullRead(int Hnd,Array<String>& Lines){
  String Line;
  if(_File[Hnd].ProcessId!=System::CurrentProcessId()){
    _Main->SetError(StlErrorCode::HandlerForbidden);
    return false; 
  }
  Lines.Reset();
  _Main->ClearError();
  while(Read(Hnd,Line)){ 
    Lines.Add(Line);
  }
  if(_Main->Error()!=StlErrorCode::Ok){ return false; }
  return true;
}

//Write full string array to text file
bool StlSubsystem::StlFileSystem::FullWrite(int Hnd,const Array<String>& Lines){
  if(_File[Hnd].ProcessId!=System::CurrentProcessId()){
    _Main->SetError(StlErrorCode::HandlerForbidden);
    return false; 
  }
  for(int i=0;i<Lines.Length();i++){
    if(!Write(Hnd,Lines[i])){ return false; }
  }
  return true;
}

//Get stream state
bool StlSubsystem::StlFileSystem::GetStateFlags(int Hnd,bool& Good,bool& Eof,bool& Fail,bool& Bad){
  if(_File[Hnd].ProcessId!=System::CurrentProcessId()){
    _Main->SetError(StlErrorCode::HandlerForbidden);
    return false; 
  }
  Good=_File[Hnd].InpStream.good();
  Eof=_File[Hnd].InpStream.eof();
  Fail=_File[Hnd].InpStream.fail();
  Bad=_File[Hnd].InpStream.bad();
  return true;
}

//Close file
bool StlSubsystem::StlFileSystem::CloseFile(int Hnd){
  if(_File[Hnd].ProcessId!=System::CurrentProcessId()){
    _Main->SetError(StlErrorCode::HandlerForbidden);
    return false; 
  }
  if(_File[Hnd].Status==StlFileStatus::Closed){
    _Main->SetError(StlErrorCode::CloseAlreadyClosed);
    return false;
  }
  if(_File[Hnd].Status==StlFileStatus::OpenRead){
    _File[Hnd].InpStream.close();
  }
  else if(_File[Hnd].Status==StlFileStatus::OpenWrite){
    _File[Hnd].OutStream.close();
  }
  _File[Hnd].FileName="";
  _File[Hnd].Status=StlFileStatus::Closed;
  return true;
}

//Close all open files
void StlSubsystem::StlFileSystem::CloseAll(){
  for(int i=0;i<_MaxFileHandlers;i++){
    if(_File[i].Status==StlFileStatus::OpenRead){
      _File[i].InpStream.close();
    }
    else if(_File[i].Status==StlFileStatus::OpenWrite){
      _File[i].OutStream.close();
    }
    _File[i].FileName="";
    _File[i].Status=StlFileStatus::Closed;
  }
}

//Return stored file name in handler
String StlSubsystem::StlFileSystem::Hnd2File(int Hnd){
  return _File[Hnd].FileName;
}

//Return handler for a file name
int StlSubsystem::StlFileSystem::File2Hnd(const String& FilePath){
  int Hnd=-1;
  for(int i=0;i<_MaxFileHandlers;i++){
    if(_File[i].Used && _File[i].FileName==FilePath && _File[i].ProcessId==System::CurrentProcessId()){
      Hnd=i;
      break;
    }
  
  }
  return Hnd;
}

//Get seek position
CpuLon StlSubsystem::StlFileSystem::GetSeekPos(int Hnd){
  if(_File[Hnd].ProcessId!=System::CurrentProcessId()){
    _Main->SetError(StlErrorCode::HandlerForbidden);
    return false; 
  }
  if(_File[Hnd].Status==StlFileStatus::OpenRead){
    return _File[Hnd].InpStream.tellg();
  }
  if(_File[Hnd].Status==StlFileStatus::OpenWrite){
    return _File[Hnd].OutStream.tellp();
  }
  return -1;
}

//Get seek position
CpuLon StlSubsystem::StlFileSystem::GetPrevPos(int Hnd){
  if(_File[Hnd].ProcessId!=System::CurrentProcessId()){
    _Main->SetError(StlErrorCode::HandlerForbidden);
    return -1; 
  }
  return _File[Hnd].PrevPos;
}

//Set reference to main class
void StlSubsystem::StlMath::SetMainPtr(StlSubsystem *Bios){
  _Main=Bios;  
}

//Constructor
StlSubsystem::StlMath::StlMath(){
  unsigned long Seed = std::chrono::system_clock::now().time_since_epoch().count();
  _RandomGenerator.seed(Seed);
  _MinRand=_RandomGenerator.min();
  _MaxRand=_RandomGenerator.max()-_RandomGenerator.min();
}

//Destructor
StlSubsystem::StlMath::~StlMath(){}

//Date to serial (thanks to Howard Hinnant)
CpuInt _Date2Serial(CpuInt Date){
  CpuInt Year=YEARPART(Date);
  CpuInt Month=MONTHPART(Date);
  CpuInt Day=DAYPART(Date);
  Year-=(Month<=2);
  const CpuInt Era = (Year >= 0 ? Year : Year-399) / 400;
  const CpuInt YearOfEra = (Year - Era * 400);
  const CpuInt DayOfYear = (153*(Month > 2 ? Month-3 : Month+9) + 2)/5 + Day-1;
  const CpuInt DayOfEra = YearOfEra * 365 + YearOfEra/4 - YearOfEra/100 + DayOfYear;
  return Era * 146097 + DayOfEra - 719468;
}

//Date to serial (thanks to Howard Hinnant)
CpuInt _Serial2Date(CpuInt Serial){
  Serial += 719468;
  const CpuInt Era = (Serial >= 0 ? Serial : Serial - 146096) / 146097;
  const CpuInt DayOfEra = (Serial - Era * 146097);
  const CpuInt YearOfEra = (DayOfEra - DayOfEra/1460 + DayOfEra/36524 - DayOfEra/146096) / 365;
  const CpuInt Year = (YearOfEra) + Era * 400;
  const CpuInt DayOfYear = DayOfEra - (365*YearOfEra + YearOfEra/4 - YearOfEra/100);
  const CpuInt BaseMonth = (5*DayOfYear + 2)/153;
  const CpuInt Day = DayOfYear - (153*BaseMonth+2)/5 + 1;
  const CpuInt Month = BaseMonth < 10 ? BaseMonth+3 : BaseMonth-9;
  return DATEVALUE(Year + (Month <= 2), Month, Day);
}

//Round
CpuLon StlSubsystem::StlMath::Round(CpuFlo Number,CpuInt Decimals){
  return std::round(Number*10*Decimals)/(10*Decimals);
}

//Seed
void StlSubsystem::StlMath::Seed(CpuFlo Number){
  void *Ptr=&Number;
  CpuLon Seed=*(CpuLon *)Ptr;
  _RandomGenerator.seed(Seed);
  _MinRand=_RandomGenerator.min();
  _MaxRand=_RandomGenerator.max()-_RandomGenerator.min();
}

//Rand
CpuFlo StlSubsystem::StlMath::Rand(){
  return (CpuFlo)(_RandomGenerator()-_MinRand)/(CpuFlo)_MaxRand;
}

//Date is valid?
CpuBol StlSubsystem::StlDateTime::DateValid(CpuInt Year,CpuInt Month,CpuInt Day){
  CpuInt Date=DATEVALUE(Year,Month,Day);
  return ISDATEVALID(Date);
}

//Date value
CpuInt StlSubsystem::StlDateTime::DateValue(CpuInt Year,CpuInt Month,CpuInt Day){
  CpuInt Date=DATEVALUE(Year,Month,Day);
  if(!ISDATEVALID(Date)){
    System::Throw(SysExceptionCode::InvalidDate,ToString(Year),ToString(Month),ToString(Day));
  }
  return Date;
}

//BegOfMonth
CpuInt StlSubsystem::StlDateTime::BegOfMonth(CpuInt Date){
  return BEGOFMONTH(Date);
}

//EndOfMonth
CpuInt StlSubsystem::StlDateTime::EndOfMonth(CpuInt Date){
  return ENDOFMONTH(Date);
}

//DatePart
CpuInt StlSubsystem::StlDateTime::DatePart(CpuInt Date,StlDatePart Part){
  CpuInt Result;
  switch(Part){
    case StlDatePart::Year : Result=YEARPART(Date); break;
    case StlDatePart::Month: Result=MONTHPART(Date); break;
    case StlDatePart::Day  : Result=DAYPART(Date); break;
  }
  return Result;
}

//DateAdd
CpuInt StlSubsystem::StlDateTime::DateAdd(CpuInt Date,StlDatePart Part,CpuInt Units){
  
  //Variables
  CpuInt Year;
  CpuInt Month;
  CpuInt Day;
  CpuInt NewDate;
  CpuInt Rest;
  
  //Switch on part
  switch(Part){
    
    //Year
    case StlDatePart::Year : 
      NewDate=DATEVALUE(YEARPART(Date)+Units,MONTHPART(Date),DAYPART(Date));
      break;
    
    //Month
    case StlDatePart::Month: 
      Rest=YEARPART(Date)*12+MONTHPART(Date)-1+Units;
      Year=Rest/12; Rest=Rest%12; if(Rest<0){ Year--; Rest+=12; }
      Month=Rest;
      Day=(DAYPART(Date)>DAYSINMONTH(Year,Month+1)?DAYSINMONTH(Year,Month+1):DAYPART(Date));
      NewDate=DATEVALUE(Year,Month+1,Day);
      break;
    
    //Day
    case StlDatePart::Day  : 
      NewDate=_Serial2Date(_Date2Serial(Date)+Units);
      break;
  }

  //Return result
  return NewDate;

}

//Time is valid
CpuBol StlSubsystem::StlDateTime::TimeValid(CpuInt Hour,CpuInt Min,CpuInt Sec,CpuLon NanoSec){
  return (Hour>=0 && Hour<=23 && Min>=0 && Min<=59 && Sec>=0 && Sec<=59 && NanoSec>=0 && NanoSec<=999999999LL?true:false);
}

//TimeValue
CpuLon StlSubsystem::StlDateTime::TimeValue(CpuInt Hour,CpuInt Min,CpuInt Sec,CpuLon NanoSec){
  if(!ISTIMEVALID(Hour,Min,Sec,NanoSec)){
    System::Throw(SysExceptionCode::InvalidTime,ToString(Hour),ToString(Min),ToString(Sec),ToString(NanoSec));
  }
  return TIMEVALUE(Hour,Min,Sec,NanoSec);
}

//TimePart
CpuLon StlSubsystem::StlDateTime::TimePart(CpuLon Time,StlTimePart Part){
  CpuLon Result;
  switch(Part){
    case StlTimePart::Hour   : Result=HOURPART(Time); break;
    case StlTimePart::Minute : Result=MINPART(Time); break;
    case StlTimePart::Second : Result=SECPART(Time); break;
    case StlTimePart::Nanosec: Result=NSECPART(Time); break;
  }
  return Result;
}

//TimeAdd
CpuLon StlSubsystem::StlDateTime::TimeAdd(CpuLon Time,StlTimePart Part,CpuLon Units,CpuInt *DayRest){
  
  //Variables
  CpuLon Days;
  CpuLon Hour;
  CpuLon Min;
  CpuLon Sec;
  CpuLon NSec;
  CpuLon Rest;
  CpuLon NewTime;

  //Switch depending on time part
  switch(Part){
    
    //Add hours
    case StlTimePart::Hour: 
      Rest=HOURPART(Time)+Units;
      Days=Rest/HOURS_IN_DAY; Rest=Rest%HOURS_IN_DAY; if(Rest<0){ Days--; Rest+=HOURS_IN_DAY; }
      Hour=Rest;
      Min=MINPART(Time);
      Sec=SECPART(Time);
      NSec=NSECPART(Time);
      NewTime=TIMEVALUE(Hour,Min,Sec,NSec);
      *DayRest=(int)Days;
      break;

    //Add minutes
    case StlTimePart::Minute:
      Rest=HOURPART(Time)*60+MINPART(Time)+Units;
      Days=Rest/MINS_IN_DAY ; Rest=Rest%MINS_IN_DAY;  if(Rest<0){ Days--; Rest+=MINS_IN_DAY;  }
      Hour=Rest/MINS_IN_HOUR; Rest=Rest%MINS_IN_HOUR; if(Rest<0){ Hour--; Rest+=MINS_IN_HOUR; }
      Min =Rest;
      Sec=SECPART(Time);
      NSec=NSECPART(Time);
      NewTime=TIMEVALUE(Hour,Min,Sec,NSec);
      *DayRest=(int)Days;
      break;

    //Add seconds
    case StlTimePart::Second:
      Rest=HOURPART(Time)*3600+MINPART(Time)*60+SECPART(Time)+Units;
      Days=Rest/SECS_IN_DAY   ; Rest=Rest%SECS_IN_DAY;    if(Rest<0){ Days--; Rest+=SECS_IN_DAY;    }
      Hour=Rest/SECS_IN_HOUR  ; Rest=Rest%SECS_IN_HOUR;   if(Rest<0){ Hour--; Rest+=SECS_IN_HOUR;   }
      Min =Rest/SECS_IN_MINUTE; Rest=Rest%SECS_IN_MINUTE; if(Rest<0){ Min --; Rest+=SECS_IN_MINUTE; }
      Sec =Rest;
      NSec=NSECPART(Time);; 
      NewTime=TIMEVALUE(Hour,Min,Sec,NSec);
      *DayRest=(int)Days;
      break;

    //Add nanoseconds
    case StlTimePart::Nanosec:
      Rest=HOURPART(Time)*NSECS_IN_HOUR+MINPART(Time)*NSECS_IN_MINUTE+SECPART(Time)*NSECS_IN_SECOND+NSECPART(Time)+Units;
      Days=Rest/NSECS_IN_DAY   ; Rest=Rest%NSECS_IN_DAY;    if(Rest<0){ Days--; Rest+=NSECS_IN_DAY;    }
      Hour=Rest/NSECS_IN_HOUR  ; Rest=Rest%NSECS_IN_HOUR;   if(Rest<0){ Hour--; Rest+=NSECS_IN_HOUR;   }
      Min =Rest/NSECS_IN_MINUTE; Rest=Rest%NSECS_IN_MINUTE; if(Rest<0){ Min --; Rest+=NSECS_IN_MINUTE; }
      Sec =Rest/NSECS_IN_SECOND; Rest=Rest%NSECS_IN_SECOND; if(Rest<0){ Sec --; Rest+=NSECS_IN_SECOND; }
      NSec=Rest; 
      NewTime=TIMEVALUE(Hour,Min,Sec,NSec);
      *DayRest=(int)Days;
      break;
  }

  //Return new time
  return NewTime;
}

//Add time part to nano second count
CpuLon StlSubsystem::StlDateTime::NanoSecAdd(CpuLon NanoSec,StlTimePart Part,CpuLon Units,CpuInt *DayRest){
  
  //Variables
  CpuLon Count;

  //Switch depending on time part
  switch(Part){
    
    //Add hours
    case StlTimePart::Hour: 
      *DayRest=(NanoSec+Units*NSECS_IN_HOUR)/NSECS_IN_DAY+(NanoSec+Units*NSECS_IN_HOUR<0?-1:0);
      Count=MOD((NanoSec+Units*NSECS_IN_HOUR),NSECS_IN_DAY);
      break;

    //Add minutes
    case StlTimePart::Minute:
      *DayRest=(NanoSec+Units*NSECS_IN_MINUTE)/NSECS_IN_DAY+(NanoSec+Units*NSECS_IN_MINUTE<0?-1:0);
      Count=MOD((NanoSec+Units*NSECS_IN_MINUTE),NSECS_IN_DAY);
      break;

    //Add seconds
    case StlTimePart::Second:
      *DayRest=(NanoSec+Units*NSECS_IN_SECOND)/NSECS_IN_DAY+(NanoSec+Units*NSECS_IN_SECOND<0?-1:0);
      Count=MOD((NanoSec+Units*NSECS_IN_SECOND),NSECS_IN_DAY);
      break;

    //Add nanoseconds
    case StlTimePart::Nanosec:
      *DayRest=(NanoSec+Units)/NSECS_IN_DAY+(NanoSec+Units<0?-1:0);
      Count=MOD((NanoSec+Units),NSECS_IN_DAY);
      break;
  }

  //Return new nano second count
  return Count;

}

//GetDate
CpuInt StlSubsystem::StlDateTime::GetDate(CpuBol Utc){
  time_t t;
  struct tm ts;
  t=time(nullptr);
  if(Utc){ 
    ts=*gmtime(&t); 
  } 
  else{ 
    ts=*localtime(&t); 
  }
  return DATEVALUE(ts.tm_year+1900,ts.tm_mon+1,ts.tm_mday);
}

//GetTime
CpuLon StlSubsystem::StlDateTime::GetTime(CpuBol Utc){
  
  //Variables
  time_t t;
  struct tm ts1;
  struct tm ts2;
  CpuLon Result;

  //Get current system time up to nanoseconds
  std::chrono::time_point<std::chrono::system_clock> Now = std::chrono::system_clock::now();
  auto Duration = Now.time_since_epoch();
  auto Hours = std::chrono::duration_cast<std::chrono::hours>(Duration);       Duration -= Hours;
  auto Min = std::chrono::duration_cast<std::chrono::minutes>(Duration);       Duration -= Min;
  auto Sec = std::chrono::duration_cast<std::chrono::seconds>(Duration);       Duration -= Sec;
  auto MSec = std::chrono::duration_cast<std::chrono::milliseconds>(Duration); Duration -= MSec;
  auto USec = std::chrono::duration_cast<std::chrono::microseconds>(Duration); Duration -= USec;
  auto NSec = std::chrono::duration_cast<std::chrono::nanoseconds>(Duration);

  //Time in Utc
  if(Utc){
    Result=TIMEVALUE(MOD(Hours.count(),24),Min.count(),Sec.count(),MSec.count()*1000000LL+USec.count()*1000LL+NSec.count());
  }

  //Local time
  else{
    t=time(nullptr);
    ts1=*gmtime(&t);
    ts2=*localtime(&t);
    Result=TIMEVALUE(MOD(Hours.count(),24)+ts2.tm_hour-ts1.tm_hour,Min.count(),Sec.count(),MSec.count()*1000000LL+USec.count()*1000LL+NSec.count());
  }
  
  //Return result
  return Result;

}

//DateDiff
CpuInt StlSubsystem::StlDateTime::DateDiff(CpuInt Date1,CpuInt Date2){
  return _Date2Serial(Date2)-_Date2Serial(Date1);
}

//TimeDiff
CpuLon StlSubsystem::StlDateTime::TimeDiff(CpuLon Time1,CpuLon Time2){
  CpuLon NSec1=HOURPART(Time1)*NSECS_IN_HOUR+MINPART(Time1)*NSECS_IN_MINUTE+SECPART(Time1)*NSECS_IN_SECOND+NSECPART(Time1);
  CpuLon NSec2=HOURPART(Time2)*NSECS_IN_HOUR+MINPART(Time2)*NSECS_IN_MINUTE+SECPART(Time2)*NSECS_IN_SECOND+NSECPART(Time2);
  return NSec2-NSec1; 
}

//Set instance main pointer
void StlSubsystem::StlDateTime::SetMainPtr(StlSubsystem *Bios){
  _Main=Bios;  
}

//Constructor
StlSubsystem::StlDateTime::StlDateTime(){}

//Destructor
StlSubsystem::StlDateTime::~StlDateTime(){}

//Delay
void StlSubsystem::Delay(int Millisecs){
  #ifdef __WIN__
    Sleep(Millisecs);
  #else
    usleep(Millisecs*1000);
  #endif
}

//Get Error
StlErrorCode StlSubsystem::Error(){
  return _ErrorCode;
}

//Get information about last error produced
String StlSubsystem::LastError(){
  return ErrorText(_ErrorCode)+(_ErrorInfo.Length()!=0?" ("+_ErrorInfo+")":"");
}

//Error code description
String StlSubsystem::ErrorText(StlErrorCode Err){
  String Text;
  switch(Err){
    case StlErrorCode::Ok                    : Text="File Ok"; break;
    case StlErrorCode::OpenReadError         : Text="Open read error"; break;
    case StlErrorCode::OpenWriteError        : Text="Open write error"; break;
    case StlErrorCode::ReadError             : Text="Read error"; break;
    case StlErrorCode::WriteError            : Text="Write error"; break;
    case StlErrorCode::GetSizeError          : Text="Attemt to get file size when handler is not open for input"; break;
    case StlErrorCode::ReadWhenClosed        : Text="Attempt to read when handler is closed"; break;
    case StlErrorCode::WriteWhenClosed       : Text="Attempt to write when handler is closed"; break;
    case StlErrorCode::FileAlreadyOpenSelf   : Text="File is already open by this process"; break;
    case StlErrorCode::FileAlreadyOpen       : Text="File is already open by other process"; break;
    case StlErrorCode::CloseAlreadyClosed    : Text="Tried to close an already closed file"; break;
    case StlErrorCode::FreeHandlerOpenFile   : Text="Cannot free handler of an open file"; break;
    case StlErrorCode::HandlerForbidden      : Text="Tried to access handler that belongs to other process"; break;
    case StlErrorCode::ExecFileNotExists     : Text="Executable file does not exist"; break;
    case StlErrorCode::ReadTimeout           : Text="Read timeout"; break;
  }
  return Text;
}

//Get system call id from number
String SystemCallId(SystemCall Nr){
  String Id="";
  switch((SystemCall)Nr){
    case SystemCall::ProgramExit              : Id="exit"; break;
    case SystemCall::Panic                    : Id="panic"; break;
    case SystemCall::Delay                    : Id="delay"; break;
    case SystemCall::Execute1                 : Id="execute1"; break;
    case SystemCall::Execute2                 : Id="execute2"; break;
    case SystemCall::Error                    : Id="error"; break;
    case SystemCall::ErrorText                : Id="errortext"; break;
    case SystemCall::LastError                : Id="lasterror"; break;
    case SystemCall::GetArg                   : Id="getarg"; break;
    case SystemCall::HostSystem               : Id="hostsystem"; break;
    case SystemCall::ConsolePrint             : Id="print"; break;
    case SystemCall::ConsolePrintLine         : Id="println"; break;
    case SystemCall::ConsolePrintError        : Id="eprint"; break;
    case SystemCall::ConsolePrintErrorLine    : Id="eprintln"; break;
    case SystemCall::GetFileName              : Id="getfnam"; break;
    case SystemCall::GetFileNameNoExt         : Id="getfnamnx"; break;
    case SystemCall::GetFileExtension         : Id="getfnamx"; break;
    case SystemCall::GetDirName               : Id="getfolder"; break;
    case SystemCall::FileExists               : Id="fileexists"; break;
    case SystemCall::DirExists                : Id="direxists"; break;
    case SystemCall::GetHandler               : Id="gethandler"; break;
    case SystemCall::FreeHandler              : Id="freehandler"; break;
    case SystemCall::GetFileSize              : Id="getfilesize"; break;
    case SystemCall::OpenForRead              : Id="openforread"; break;
    case SystemCall::OpenForWrite             : Id="openforwrite"; break;
    case SystemCall::OpenForAppend            : Id="openforappend"; break;
    case SystemCall::Read                     : Id="read"; break;
    case SystemCall::Write                    : Id="write"; break;
    case SystemCall::ReadAll                  : Id="readall"; break;
    case SystemCall::WriteAll                 : Id="writeall"; break;
    case SystemCall::ReadStr                  : Id="readstr"; break;
    case SystemCall::WriteStr                 : Id="writestr"; break;
    case SystemCall::ReadStrAll               : Id="readstrall"; break;
    case SystemCall::WriteStrAll              : Id="writestrall"; break;
    case SystemCall::CloseFile                : Id="closefile"; break;
    case SystemCall::Hnd2File                 : Id="hnd2file"; break;
    case SystemCall::File2Hnd                 : Id="file2hnd"; break;
    case SystemCall::AbsChr                   : Id="abschr"; break;
    case SystemCall::AbsShr                   : Id="absshr"; break;
    case SystemCall::AbsInt                   : Id="absint"; break;
    case SystemCall::AbsLon                   : Id="abslon"; break;
    case SystemCall::AbsFlo                   : Id="absflo"; break;
    case SystemCall::MinChr                   : Id="minchr"; break;
    case SystemCall::MinShr                   : Id="minshr"; break;
    case SystemCall::MinInt                   : Id="minint"; break;
    case SystemCall::MinLon                   : Id="minlon"; break;
    case SystemCall::MinFlo                   : Id="minflo"; break;
    case SystemCall::MaxChr                   : Id="maxchr"; break;
    case SystemCall::MaxShr                   : Id="maxshr"; break;
    case SystemCall::MaxInt                   : Id="maxint"; break;
    case SystemCall::MaxLon                   : Id="maxlon"; break;
    case SystemCall::MaxFlo                   : Id="maxflo"; break;
    case SystemCall::Exp                      : Id="exp"; break;
    case SystemCall::Ln                       : Id="ln"; break;
    case SystemCall::Log                      : Id="log"; break;
    case SystemCall::Logn                     : Id="logn"; break;
    case SystemCall::Pow                      : Id="pow"; break;
    case SystemCall::Sqrt                     : Id="sqrt"; break;
    case SystemCall::Cbrt                     : Id="cbrt"; break;
    case SystemCall::Sin                      : Id="sin"; break;
    case SystemCall::Cos                      : Id="cos"; break;
    case SystemCall::Tan                      : Id="tan"; break;
    case SystemCall::Asin                     : Id="asin"; break;
    case SystemCall::Acos                     : Id="acos"; break;
    case SystemCall::Atan                     : Id="atan"; break;
    case SystemCall::Sinh                     : Id="sinh"; break;
    case SystemCall::Cosh                     : Id="cosh"; break;
    case SystemCall::Tanh                     : Id="tanh"; break;
    case SystemCall::Asinh                    : Id="asinh"; break;
    case SystemCall::Acosh                    : Id="acosh"; break;
    case SystemCall::Atanh                    : Id="atanh"; break;
    case SystemCall::Ceil                     : Id="ceil"; break;
    case SystemCall::Floor                    : Id="floor"; break;
    case SystemCall::Round                    : Id="round"; break;
    case SystemCall::Seed                     : Id="seed"; break;
    case SystemCall::Rand                     : Id="rand"; break;
    case SystemCall::DateValid                : Id="datevalid"; break;
    case SystemCall::DateValue                : Id="datevalue"; break;
    case SystemCall::BegOfMonth               : Id="begofmonth"; break;
    case SystemCall::EndOfMonth               : Id="endofmonth"; break;
    case SystemCall::DatePart                 : Id="datepart"; break;
    case SystemCall::DateAdd                  : Id="dateadd"; break;
    case SystemCall::TimeValid                : Id="timevalid"; break;
    case SystemCall::TimeValue                : Id="timevalue"; break;
    case SystemCall::TimePart                 : Id="timepart"; break;
    case SystemCall::TimeAdd                  : Id="timeadd"; break;
    case SystemCall::NanoSecAdd               : Id="nsecadd"; break;
    case SystemCall::GetDate                  : Id="getdate"; break;
    case SystemCall::GetTime                  : Id="gettime"; break;
    case SystemCall::DateDiff                 : Id="datediff"; break;
    case SystemCall::TimeDiff                 : Id="timediff"; break;
  }
  return Id;
}

//Find system call Id
SystemCall FindSystemCall(const String& Id){
  for(int i=0;i<_SystemCallNr;i++){
    if(SystemCallId((SystemCall)i)==Id){ return (SystemCall)i; }
  }
  return (SystemCall)-1;
}

//Get system executable path
bool GetExecutablePath(String& ExecPath){

  //Maximun path size allocated
  #define MAXPATH 1024
  char Path[MAXPATH];

  //Initialize path
  memset(Path,0,sizeof(Path));

  //Get path on windows/linux
  #ifdef __WIN__
  if(GetModuleFileName(nullptr,Path,sizeof(Path))==0){ return false; }
  #else
  if(readlink("/proc/self/exe", Path, sizeof(Path))==-1){ return false; }
  #endif

  //Check truncation occured
  if(Path[MAXPATH-1]!=0){ return false; }

  //Check file exists
  if((!_Stl->FileSystem.FileExists(Path))){ return false; }

  //Return path
  ExecPath=String(Path);
  return true;

}