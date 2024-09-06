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
#include "sys/msgout.hpp"
#include "vrm/pool.hpp"
#include "vrm/memman.hpp"

//Definition of static variables
MemoryPool MemoryManager::_MemoryPool;

//Internal functions
char *_MainInnerAlloc(CpuWrd Size,int Owner);
void _MainInnerFree(char *Ptr);

//Inner memory allocator
char *_MainInnerAlloc(CpuWrd Size,int Owner){
  char *Ptr;
  try{ Ptr=new char[Size]; } 
  catch(std::bad_alloc& Ex){ 
    return nullptr; 
  }
  return Ptr;
}

//Inner memory releaser
void _MainInnerFree(char *Ptr){
  delete[] Ptr;
}

//Memory manager initialization
bool MemoryManager::Init(CpuWrd Units,CpuWrd ChunkUnits,CpuWrd UnitSize,bool Lock){

  //Variables
  CpuWrd TotalMemory;

  //Calculate total memory
  TotalMemory=Units*UnitSize;

  //Init internal memory pool
  if(!_MemoryPool.Create(Units,ChunkUnits,UnitSize,MEMMAN_FREELIST,MEMMAN_FREEBITS,-1,Lock,&_MainInnerAlloc,&_MainInnerFree)){
    switch(_MemoryPool.LastError()){
      case MemPoolError::SmallUnitSize  : SysMessage(334).Print(ToString(UnitSize),ToString(_MemoryPool.MinUnitSize())); break;
      case MemPoolError::AllocationError: SysMessage(326).Print(ToString(TotalMemory)); break;
      case MemPoolError::PageLockFailure: SysMessage(321).Print(ToString(TotalMemory)); break;
      case MemPoolError::RequestError   : SysMessage(326).Print(ToString(TotalMemory)); break;
    }
    return false; 
  }
  
  //Debug message
  DebugMessage(DebugLevel::VrmMemory,"Memory manager initialized: memory_unit="+ToString(UnitSize)+" memory="+ToString(TotalMemory));
  
  //Return code
  return true;
 
}

//Memory manager terminate
void MemoryManager::Terminate(void){
  _MemoryPool.Destroy();     
  DebugMessage(DebugLevel::VrmMemory,"Memory manager terminated");
}

//Memory check
#ifdef __DEV__
bool MemoryManager::Check(){
  return _MemoryPool.MemoryCheck(MemOperation::OuterCheck);
}
#endif
