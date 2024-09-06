//coremm.cpp: Core memory manager
#include "bas/basedefs.hpp"
#include "bas/allocator.hpp"

//Allocator control global instance
AllocatorControl _AloCtr;

//Allocator control constructor
AllocatorControl::AllocatorControl(){
  _DispAlloc=false;
  _DispCntrl=false;
  _MsgIdAlloc='-';
  _MsgIdCntrl='-';
  _TotMemAllocated=0;
  _TotMaxAllocated=0;
  for(int i=0;i<MEMORY_OBJECTS;i++){ _MemAllocated[i]=0; _MaxAllocated[i]=0; }
  if(_DispAlloc){
    std::cout << "[" + std::string(1,_MsgIdAlloc) + "] System allocator initialized" << std::endl;
  }
}

//Allocator control destructor
AllocatorControl::~AllocatorControl(){
  if(_DispAlloc){
    std::cout << "[" + std::string(1,_MsgIdAlloc) + "] System allocator terminated" << std::endl;
  }
  if(_DispCntrl){
    std::cout << "[" + std::string(1,_MsgIdCntrl) + "] Memory account...: ";
    for(int i=0;i<MEMORY_OBJECTS;i++){ 
      std::cout << (i!=0?", ":"") + std::string(MEMOBJECTTEXT((MemObject)i)) + "=" + std::to_string(_MemAllocated[i]);
    }
    std::cout << std::endl;
    std::cout << "[" + std::string(1,_MsgIdCntrl) + "] Maximun allocated: ";
    for(int i=0;i<MEMORY_OBJECTS;i++){ 
      std::cout << (i!=0?", ":"") + std::string(MEMOBJECTTEXT((MemObject)i)) + "=" + std::to_string(_MaxAllocated[i]);
    }
    std::cout << std::endl;
    std::cout << "[" + std::string(1,_MsgIdCntrl) + "] Total account....: " + std::to_string(_TotMemAllocated) << std::endl;
    std::cout << "[" + std::string(1,_MsgIdCntrl) + "] Maximun total....: " + std::to_string(_TotMaxAllocated) << std::endl;
  }
}

//Set display flags
void AllocatorControl::SetDispAlloc(bool Disp,char Id){ _DispAlloc=Disp; _MsgIdAlloc=Id; }
void AllocatorControl::SetDispCntrl(bool Disp,char Id){ _DispCntrl=Disp; _MsgIdCntrl=Id; }

//Memory accounting
void AllocatorControl::Account(MemObject Obj,const char *TypeName,long Elements,long Size){
  _MemAllocated[(int)Obj]+=(Elements*Size);
  _MaxAllocated[(int)Obj]=(_MemAllocated[(int)Obj]>_MaxAllocated[(int)Obj]?_MemAllocated[(int)Obj]:_MaxAllocated[(int)Obj]);
  _TotMemAllocated+=(Elements*Size);
  _TotMaxAllocated=(_TotMemAllocated>_TotMaxAllocated?_TotMemAllocated:_TotMaxAllocated);
  if(_DispAlloc){
    std::cout << "[" + std::string(1,_MsgIdAlloc) + "] " + (Elements>0?"--> ":"<-- ") + std::string(MEMOBJECTTEXT(Obj)) + (TypeName!=nullptr?"<" + std::string(TypeName) + ">":"") + std::string((Elements>0?" take ":" give ")) + std::to_string(abs(Elements))+" elements (size = "+std::to_string(abs(Elements)*Size)+" bytes, total = "+std::to_string(_MemAllocated[(int)Obj])+" bytes, all = "+std::to_string(_TotMemAllocated)+" bytes)" << std::endl;
  }
}