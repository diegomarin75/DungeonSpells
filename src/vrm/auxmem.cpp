//Memman.cpp: Memory manager class
#include "bas/basedefs.hpp"
#include "bas/allocator.hpp"
#include "bas/array.hpp"
#include "bas/stack.hpp"
#include "bas/buffer.hpp"
#include "bas/string.hpp"
#include "sys/sysdefs.hpp"
#include "sys/system.hpp"
#include "sys/stl.hpp"
#include "vrm/pool.hpp"
#include "vrm/memman.hpp"
#include "vrm/auxmem.hpp"

//Main memory check
#ifdef __DEV__
  #define MainMemoryCheckReturn() if(!MemoryManager::Check()){ return false; }
  #define MainMemoryCheckVoid()   if(!MemoryManager::Check()){ return; }
#else
  #define MainMemoryCheckReturn()
  #define MainMemoryCheckVoid()
#endif

//Global process id variable (used on inner allocators as we cannot pass it to them)
int _InnerProcessId;

//Internal functions
char *_AuxInnerAlloc(CpuWrd Size,int Owner);
void _AxInnerFree(char *Ptr);

//Inner memory allocator
char *_AuxInnerAlloc(CpuWrd Size,int Owner){
  return MemoryManager::Alloc(Owner,Size);
}

//Inner memory releaser
void _AuxInnerFree(char *Ptr){
  MemoryManager::Free(Ptr);
}

//Process memory manager initialization
bool AuxMemoryManager::Init(int ProcessId,CpuMbl BlockMax,CpuWrd Units,CpuWrd ChunkUnits,CpuWrd UnitSize,String& Error){

  //Variables
  long i;
  char *Ptr;
  CpuWrd BlockSize;

  //Calculate memory sizes
  BlockSize=BlockMax*sizeof(AuxBlock);

  //Get memory for block table
  if((Ptr=MemoryManager::Alloc(ProcessId,BlockSize))==nullptr){
    return false;
  }
  _Block=reinterpret_cast<AuxBlock *>(Ptr);

  //Init internal memory pool
  if(!_MemoryPool.Create(Units,ChunkUnits,UnitSize,AUXMAN_FREELIST,AUXMAN_FREEBITS,ProcessId,false,&_AuxInnerAlloc,&_AuxInnerFree)){
    Error=_MemoryPool.ErrorText(_MemoryPool.LastError());
    return false; 
  }

  //Init memory variables
  _BlockMax=BlockMax;
  _ProcessId=ProcessId;
  
  //Init block table
  for(i=0;i<_BlockMax;i++){ memset(reinterpret_cast<char *>(&_Block[i]),0,sizeof(AuxBlock)); }

  //Set last assigned pointers to -1 (means no assignment yet)
  _LastBlockAsg=-1;
  
  //Debug message
  DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory manager initialized: handlers="+ToString(_BlockMax)+" memory_unit="+ToString(UnitSize)+" memory="+ToString(Units*UnitSize));
  DebugMessage(DebugLevel::VrmAuxMemStatus,GetStatus(0,0));

  //Return code
  return true;
}

//Memory manager terminate
void AuxMemoryManager::Terminate(){
  _MemoryPool.Destroy();
  MemoryManager::Free(reinterpret_cast<char *>(_Block));     
  DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory manager terminated");
  DebugMessage(DebugLevel::VrmAuxMemStatus,GetStatus(0,0));
}

//Constructor
AuxMemoryManager::AuxMemoryManager(){
  _Init=false;
}

//Destructor
AuxMemoryManager::~AuxMemoryManager(){
  if(_Init){ 
    Terminate(); 
  }
}

//Get memory handler
bool AuxMemoryManager::_ExtendHandlers(){
  
  //Variables
  int i;
  char *Ptr;
  CpuMbl CurMax;
  CpuMbl NewMax;

  //Save current size and calculate new size
  CurMax=_BlockMax;
  NewMax=_BlockMax+(_BlockMax/4);

  //Extend handler table
  Ptr=reinterpret_cast<char *>(_Block);
  if(!MemoryManager::Realloc(&Ptr,NewMax*sizeof(AuxBlock))){ 
    return false;
  }
  
  //Update block table
  _Block=reinterpret_cast<AuxBlock *>(Ptr);
  _BlockMax=NewMax;
  
  //Init new added handlers
  for(i=CurMax;i<NewMax;i++){ memset(reinterpret_cast<char *>(&_Block[i]),0,sizeof(AuxBlock)); }

  //Debug message
  DebugMessage(DebugLevel::VrmAuxMemory,"Block table is extended to "+ToString(_BlockMax)+" blocks");
  
  //Return success
  return true;

}

//Get memory handler
bool AuxMemoryManager::_GetHandler(int ScopeId,CpuLon ScopeNr,CpuMbl *Block){
  
  //Variables
  long i;
  CpuMbl FirstFree;

  //Init result
  *Block=-1;

  //Find from last assigned block
  if(_LastBlockAsg!=-1){
    for(i=_LastBlockAsg+1;i<_BlockMax;i++){
      if(!_Block[i].Used || IsZombie(i,ScopeId,ScopeNr)){
        *Block=i;
        _LastBlockAsg=i;
        return true;
      }
    }
  }

  //Find from beginning
  for(i=1;i<_BlockMax;i++){
    if(!_Block[i].Used || IsZombie(i,ScopeId,ScopeNr)){
      *Block=i;
      _LastBlockAsg=i;
      return true;
    }
  }

  //Extend handler table
  FirstFree=_BlockMax;
  if(_ExtendHandlers()){ 
    *Block=FirstFree;
    _LastBlockAsg=FirstFree;
    return true;
  }
  
  //Return failure
  return false;

}

//Internal releaser
void AuxMemoryManager::_Free(CpuMbl Block){
  _MemoryPool.Free(_Block[Block].Ptr);
  memset(reinterpret_cast<char *>(&_Block[Block]),0,sizeof(AuxBlock));
}

//Memory empty allocation request (only handler)
bool AuxMemoryManager::EmptyAlloc(int ScopeId,CpuLon ScopeNr,CpuMbl *Block){

  //Debug message
  DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory manager empty allocation start (processid="+ToString(_ProcessId)+" scopeid="+ToString(ScopeId)+" scopenr="+ToString(ScopeNr)+")");

  //Get new memory handler (Block zero is avoided as it means no allocated block)
  if(!_GetHandler(ScopeId,ScopeNr,Block)){
    DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory empty allocation failure, no free handlers (processid="+ToString(_ProcessId)+" scopeid="+ToString(ScopeId)+" scopenr="+ToString(ScopeNr)+")");
    DebugMessage(DebugLevel::VrmAuxMemStatus,GetStatus(ScopeId,ScopeNr));
    return false; 
  }

  //Set handler fields
  _Block[*Block].ScopeId=ScopeId;
  _Block[*Block].ScopeNr=ScopeNr;
  _Block[*Block].Used=1;
  _Block[*Block].Size=0;
  _Block[*Block].Length=0;
  _Block[*Block].ArrIndex=-1;
  _Block[*Block].Ptr=nullptr;

  //Debug message
  DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory empty allocation end: processid="+ToString(_ProcessId)+" handler="+HEXFORMAT(*Block)+" scopeid="+ToString(ScopeId)+" scopenr="+ToString(ScopeNr));
  DebugMessage(DebugLevel::VrmAuxMemStatus,GetStatus(ScopeId,ScopeNr));

  //Main memory check
  MainMemoryCheckReturn();
  
  //Return code
  return true;

}

//Memory allocation request
bool AuxMemoryManager::Alloc(int ScopeId,CpuLon ScopeNr,CpuWrd Size,int ArrIndex,CpuMbl *Block){

  //Variables
  char *Ptr;

  //Debug message
  DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory manager allocation start (processid="+ToString(_ProcessId)+" scopeid="+ToString(ScopeId)+" scopenr="+ToString(ScopeNr)+" size="+ToString(Size)+")");

  //Get new memory handler (Block zero is avoided as it means no allocated block)
  if(!_GetHandler(ScopeId,ScopeNr,Block)){
    DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory allocation failure, no free handlers (processid="+ToString(_ProcessId)+" scopeid="+ToString(ScopeId)+" scopenr="+ToString(ScopeNr)+" size="+ToString(Size)+")");
    DebugMessage(DebugLevel::VrmAuxMemStatus,GetStatus(ScopeId,ScopeNr));
    return false; 
  }

  //Try allocation
  Ptr=_MemoryPool.Allocate(Size,_ProcessId,false);

  //Traverse zombie blocks
  if(Ptr==nullptr){

    //Debug message
    DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory allocation, started search on zombie blocks");

    //Try allocation on zombie blocks (we take a block not much bigger that double size of request)
    for(CpuMbl i=1;i<_BlockMax;i++){
      if(_Block[i].Used && IsZombie(i,ScopeId,ScopeNr)){
        if(_Block[i].Size>=Size && _Block[i].Size<=2*Size){
          DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory allocation, found zombie block (scopeid="+ToString(_Block[i].ScopeId)+" scopenr="+ToString(_Block[i].ScopeNr)+" size="+ToString(_Block[i].Size)+" ptr="+PTRFORMAT(_Block[i].Ptr)+")");
          Ptr=_Block[i].Ptr;
          memset(reinterpret_cast<char *>(&_Block[i]),0,sizeof(AuxBlock));
          break;
        }
        else{
          _Free(i);
        }
      }
    }

    //Try allocation again if allocation on zombie blocks failed
    if(Ptr==nullptr){
      DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory allocation, zombie block search failed, extending memory");
      if((Ptr=_MemoryPool.Allocate(Size,_ProcessId,true))==nullptr){
        DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory allocation failure, no free blocks (processid="+ToString(_ProcessId)+" scopeid="+ToString(ScopeId)+" scopenr="+ToString(ScopeNr)+" size="+ToString(Size)+")");
        return false;
      }
    }

  }

  //Set handler fields
  _Block[*Block].ScopeId=ScopeId;
  _Block[*Block].ScopeNr=ScopeNr;
  _Block[*Block].Used=1;
  _Block[*Block].Size=Size;
  _Block[*Block].Length=0;
  _Block[*Block].ArrIndex=ArrIndex;
  _Block[*Block].Ptr=Ptr;

  //Debug message
  DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory allocation end: processid="+ToString(_ProcessId)+" size="+ToString(Size)+" handler="+HEXFORMAT(*Block)+" scopeid="+ToString(ScopeId)+" scopenr="+ToString(ScopeNr)+" ptr="+PTRFORMAT(Ptr));
  DebugMessage(DebugLevel::VrmAuxMemStatus,GetStatus(ScopeId,ScopeNr));

  //Main memory check
  MainMemoryCheckReturn();
  
  //Return code
  return true;

}

//Memory allocation request (Forced means that block number is given, not searched)
bool AuxMemoryManager::ForcedAlloc(int ScopeId,CpuLon ScopeNr,CpuWrd Size,int ArrIndex,CpuMbl Block){

  //Variables
  char *Ptr;

  //Debug message
  DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory manager forced allocation start (size="+ToString(Size)+" handler="+HEXFORMAT(Block)+" scopeid="+ToString(ScopeId)+" scopenr="+ToString(ScopeNr)+")");

  //Check handler table extension is required
  if(Block>_BlockMax-1){
    if(!_ExtendHandlers()){ return false; }
  }

  //Check handler is valid
  if(Block<1 || Block>_BlockMax-1){
    DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory forced allocation failure, invalid handler (size="+ToString(Size)+" handler="+HEXFORMAT(Block)+" scopeid="+ToString(ScopeId)+" scopenr="+ToString(ScopeNr)+")");
    return(false);
  }

  //Force block free
  if(_Block[Block].Used){
    Free(Block);
  }

  //Try allocation
  Ptr=_MemoryPool.Allocate(Size,_ProcessId,false);

  //Traverse zombie blocks
  if(Ptr==nullptr){

    //Debug message
    DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory forced allocation, started search on zombie blocks");
    
    //Try allocation on zombie blocks (we take a block not much bigger that double size of request)
    for(CpuMbl i=1;i<_BlockMax;i++){
      if(_Block[i].Used && IsZombie(i,ScopeId,ScopeNr)){
        if(_Block[i].Size>=Size && _Block[i].Size<=2*Size){
          DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory forced allocation, found zombie block (scopeid="+ToString(_Block[i].ScopeId)+" scopenr="+ToString(_Block[i].ScopeNr)+" size="+ToString(_Block[i].Size)+" ptr="+PTRFORMAT(_Block[i].Ptr)+")");
          Ptr=_Block[i].Ptr;
          memset(reinterpret_cast<char *>(&_Block[i]),0,sizeof(AuxBlock));
          break;
        }
        else{
          _Free(i);
        }
      }
    }

    //Try allocation again if allocation on zombie blocks failed
    if(Ptr==nullptr){
      DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory forced allocation, zombie block search failed, extending memory");
      if((Ptr=_MemoryPool.Allocate(Size,_ProcessId,true))==nullptr){
        DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory forced allocation failure, no free blocks (processid="+ToString(_ProcessId)+" scopeid="+ToString(ScopeId)+" scopenr="+ToString(ScopeNr)+" size="+ToString(Size)+")");
        return false;
      }
    }

  }

  //Set handler fields
  _Block[Block].ScopeId=ScopeId;
  _Block[Block].ScopeNr=ScopeNr;
  _Block[Block].Used=1;
  _Block[Block].Size=Size;
  _Block[Block].Length=Size;
  _Block[Block].ArrIndex=ArrIndex;
  _Block[Block].Ptr=Ptr;
  
  //Debug message
  DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory forced allocation end: processid="+ToString(_ProcessId)+" size="+ToString(Size)+" handler="+HEXFORMAT(Block)+" scopeid="+ToString(ScopeId)+" scopenr="+ToString(ScopeNr)+" ptr="+PTRFORMAT(Ptr));
  DebugMessage(DebugLevel::VrmAuxMemStatus,GetStatus(ScopeId,ScopeNr));

  //Main memory check
  MainMemoryCheckReturn();

  //Return code
  return true;

}

//Memory re-allocation request
bool AuxMemoryManager::Realloc(int ScopeId,CpuLon ScopeNr,CpuMbl Block,CpuWrd Size){
  
  //Variables
  bool Found;

  //Debug message
  DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory manager re-allocation start (size="+ToString(Size)+" handler="+HEXFORMAT(Block)+" blockscopeid="+ToString(_Block[Block].ScopeId)+" blockscopenr="+ToString(_Block[Block].ScopeNr)+")");

  //Check handler exists, it is used and belongs to process
  if(Block<1 || Block>_BlockMax-1){
    DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory re-allocation failure, invalid handler (size="+ToString(Size)+" handler="+HEXFORMAT(Block)+" blockscopeid="+ToString(_Block[Block].ScopeId)+" blockscopenr="+ToString(_Block[Block].ScopeNr)+")");
    return(false);
  }
  else if(!_Block[Block].Used){
    DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory re-allocation failure, access to unused handler (size="+ToString(Size)+" handler="+HEXFORMAT(Block)+" blockscopeid="+ToString(_Block[Block].ScopeId)+" blockscopenr="+ToString(_Block[Block].ScopeNr)+")");
    return(false);
  }

  //Try re-allocation without pool extension
  if(!_MemoryPool.ReAllocate(&_Block[Block].Ptr,Size,false)){

    //Debug message
    DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory re-allocation, started search on zombie blocks");
    
    //Try allocation on zombie blocks (we take a block not much bigger that double size of request)
    Found=false;
    for(CpuMbl i=1;i<_BlockMax;i++){
      if(_Block[i].Used && IsZombie(i,ScopeId,ScopeNr)){
        if(_Block[i].Size>=Size && _Block[i].Size<=2*Size){
          DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory re-allocation, found zombie block (scopeid="+ToString(_Block[i].ScopeId)+" scopenr="+ToString(_Block[i].ScopeNr)+" size="+ToString(_Block[i].Size)+" ptr="+PTRFORMAT(_Block[i].Ptr)+")");
          MemCpy(_Block[i].Ptr,_Block[Block].Ptr,_Block[Block].Size); 
          _Block[Block].Ptr=_Block[i].Ptr;
          memset(reinterpret_cast<char *>(&_Block[i]),0,sizeof(AuxBlock));
          Found=true;
          break;
        }
        else{
          _Free(i);
        }
      }
    }

    //Try re-allocation again if allocation on zombie blocks failed with pool extension
    if(!Found){
      DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory re-allocation, zombie block search failed, extending memory");
      if(!_MemoryPool.ReAllocate(&_Block[Block].Ptr,Size,true)){
        DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory re-allocation failure, no free blocks (processid="+ToString(_ProcessId)+" scopeid="+ToString(ScopeId)+" scopenr="+ToString(ScopeNr)+" size="+ToString(Size)+")");
        return false;
      }
    }

  }

  //Set new size
  _Block[Block].Size=Size;
  
  //Debug message
  DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory re-allocation end: processid="+ToString(_ProcessId)+" newsize="+ToString(Size)+" handler="+HEXFORMAT(Block)+" blockscopeid="+ToString(_Block[Block].ScopeId)+" blockscopenr="+ToString(_Block[Block].ScopeNr)+" ptr="+PTRFORMAT(_Block[Block].Ptr));
  DebugMessage(DebugLevel::VrmAuxMemStatus,GetStatus(ScopeId,ScopeNr));

  //Main memory check
  MainMemoryCheckReturn();

  //Return code
  return true;

}

//Free memory
void AuxMemoryManager::Free(CpuMbl Block){
  
  //Debug message
  DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory manager release start (handler="+HEXFORMAT(Block)+" blockscopeid="+ToString(_Block[Block].ScopeId)+" blockscopenr="+ToString(_Block[Block].ScopeNr)+")");

  //Check handler exists and it is used
  if(Block<1 || Block>_BlockMax-1){
    DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory release failure, invalid handler (handler="+HEXFORMAT(Block)+" blockscopeid="+ToString(_Block[Block].ScopeId)+" blockscopenr="+ToString(_Block[Block].ScopeNr)+")");
    String Message="Tried to free invalid block handler "+ToString(Block);
    ThrowBaseException((int)ExceptionSource::AuxMemory,(int)AuxMemoryException::FreeInvalidBlock,Message.CharPnt());
    return;
  }
  else if(!_Block[Block].Used){
    DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory release failure, access to unused handler (handler="+HEXFORMAT(Block)+" blockscopeid="+ToString(_Block[Block].ScopeId)+" blockscopenr="+ToString(_Block[Block].ScopeNr)+")");
    String Message="Tried to free already released handler "+ToString(Block);
    ThrowBaseException((int)ExceptionSource::AuxMemory,(int)AuxMemoryException::FreeAlreadyReleased,Message.CharPnt());
    return;
  }

  //De-Allocate memory block
  _Free(Block);

  //Debug message
  DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory handler release end: processid="+ToString(_ProcessId)+" handler="+HEXFORMAT(Block)+" blockscopeid="+ToString(_Block[Block].ScopeId)+" blockscopenr="+ToString(_Block[Block].ScopeNr));
  DebugMessage(DebugLevel::VrmAuxMemStatus,GetStatus(_Block[Block].ScopeId,_Block[Block].ScopeNr));

  //Main memory check
  MainMemoryCheckVoid();

}

//Freememory keepblock
void AuxMemoryManager::Clear(CpuMbl Block){
  
  //Debug message
  DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory manager clear start (handler="+HEXFORMAT(Block)+" blockscopeid="+ToString(_Block[Block].ScopeId)+" blockscopenr="+ToString(_Block[Block].ScopeNr)+")");

  //Check handler exists and it is used
  if(Block<1 || Block>_BlockMax-1){
    DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory clear failure, invalid handler (handler="+HEXFORMAT(Block)+" blockscopeid="+ToString(_Block[Block].ScopeId)+" blockscopenr="+ToString(_Block[Block].ScopeNr)+")");
    String Message="Tried to free invalid block handler "+ToString(Block);
    ThrowBaseException((int)ExceptionSource::AuxMemory,(int)AuxMemoryException::FreeInvalidBlock,Message.CharPnt());
    return;
  }
  else if(!_Block[Block].Used){
    DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory clear failure, access to unused handler (handler="+HEXFORMAT(Block)+" blockscopeid="+ToString(_Block[Block].ScopeId)+" blockscopenr="+ToString(_Block[Block].ScopeNr)+")");
    String Message="Tried to clear already released handler "+ToString(Block);
    ThrowBaseException((int)ExceptionSource::AuxMemory,(int)AuxMemoryException::FreeAlreadyReleased,Message.CharPnt());
    return;
  }

  //De-Allocate memory block
  _MemoryPool.Free(_Block[Block].Ptr);
  _Block[Block].Ptr=nullptr;
  _Block[Block].Size=0;
  _Block[Block].Length=0;
  _Block[Block].ArrIndex=-1;

  //Debug message
  DebugMessage(DebugLevel::VrmAuxMemory,"Aux memory handler clear end: processid="+ToString(_ProcessId)+" handler="+HEXFORMAT(Block)+" blockscopeid="+ToString(_Block[Block].ScopeId)+" blockscopenr="+ToString(_Block[Block].ScopeNr));
  DebugMessage(DebugLevel::VrmAuxMemStatus,GetStatus(_Block[Block].ScopeId,_Block[Block].ScopeNr));

  //Main memory check
  MainMemoryCheckVoid();

}

//Copy memory block contents
void AuxMemoryManager::Copy(CpuMbl Block, char *Src,CpuWrd Length){
  MemCpy(CharPtr(Block),Src,Length);
  MainMemoryCheckVoid();
}

//Memory status string
String AuxMemoryManager::GetStatus(int ScopeId,CpuLon ScopeNr){

  //Variables
  long i;
  long j;
  long BlockCount;
  CpuLon CumulSize;
  String Result;

  //Init result string
  Result="blocks{";
  
  //Count free blocks
  BlockCount=0;
  for(j=1;j<_BlockMax;j++){
    if(!_Block[j].Used || IsZombie(j,ScopeId,ScopeNr)){ BlockCount++; }
  }
  Result+=" free="+ToString(BlockCount);

  //Count used blocks per scope
  Result+=" usedflag={";
  for(i=0;i<=ScopeId+1;i++){
    BlockCount=0;
    CumulSize=0;
    for(j=1;j<_BlockMax;j++){
      if(_Block[j].Used 
      && ((i<=ScopeId && _Block[j].ScopeId==i)
      || (i>ScopeId && (_Block[j].ScopeId>i)))){ 
        BlockCount++; 
        CumulSize+=_Block[j].Size; 
      }
    }
    if(i<=ScopeId){
      Result+=" "+ToString(i)+":"+ToString(BlockCount)+"bl:"+ToString(CumulSize)+"B";
    }
    else{
      Result+=" >"+ToString(i-1)+":"+ToString(BlockCount)+"bl:"+ToString(CumulSize)+"B";
    }
  }
  Result+=" }";

  //Count zombie blocks
  Result+=" zombie=";
  BlockCount=0;
  CumulSize=0;
  for(j=1;j<_BlockMax;j++){
    if(_Block[j].Used && IsZombie(j,ScopeId,ScopeNr)){
      BlockCount++; 
      CumulSize+=_Block[j].Size; 
    }
  }
  Result+="{"+ToString(BlockCount)+"bl:"+ToString(CumulSize)+"B}";

  //Total Blocks
  Result+=" total="+ToString(_BlockMax)+" }";

  //Return result
  return Result;

}