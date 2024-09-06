//allocator.hpp: Core memory allocator

//Wrap include
#ifndef _ALLOCATOR_HPP
#define _ALLOCATOR_HPP

//Macro to enable allocator messages (set to 1 if necessary)
#define ALLOCATOR_MESSAGES 1

//Allocation types
#define MEMORY_OBJECTS 7
enum class MemObject:int{
  Array=0,
  SortedArray=1,
  Stack=2,
  Queue=3,
  Buffer=4,
  NlBuffer=5,
  String=6
};

//Allocation type descriptions
#define MEMOBJECTTEXT(obj) \
(obj==MemObject::Array?"Array": \
(obj==MemObject::SortedArray?"SortedArray": \
(obj==MemObject::Stack?"Stack": \
(obj==MemObject::Queue?"Queue": \
(obj==MemObject::Buffer?"Buffer": \
(obj==MemObject::NlBuffer?"NlBuffer": \
(obj==MemObject::String?"String": \
"Undefined")))))))

//Exception numbers
enum class AllocatorException{
  OutOfMemory=1
};

//Memory allocator control
class AllocatorControl{
  private:
    bool _DispAlloc;
    bool _DispCntrl;
    char _MsgIdAlloc;
    char _MsgIdCntrl;
    long _MemAllocated[MEMORY_OBJECTS];
    long _MaxAllocated[MEMORY_OBJECTS];
    long _TotMemAllocated;
    long _TotMaxAllocated;
  public:
    void SetDispAlloc(bool Disp,char Id);
    void SetDispCntrl(bool Disp,char Id);
    void Account(MemObject Obj,const char *TypeName,long Elements,long Size);
    AllocatorControl();
    ~AllocatorControl();

};

//Allocator control global instance
extern AllocatorControl _AloCtr;

//Allocator class
template <typename datatype> 
class Allocator{
  public:
    static datatype *Take(MemObject Obj,long Elements);
    static void Give(MemObject Obj,long Elements,datatype *Ptr);

};

// --- Implementation part (also included in the header file to avoid linking errors) ---

//Allocate space for Allocator
template <typename datatype> 
datatype *Allocator<datatype>::Take(MemObject Obj,long Elements){
  
  //Variables
  datatype *Temp;
  
  //Get memory
  try{ Temp=new datatype[Elements]; }
  catch(std::bad_alloc& Ex){
    std::string TypeDesc;
    CplusGetTypeDesc(datatype,TypeDesc);
    std::string Msg="Ouf of memory when getting "+std::to_string(Elements)+" elements of data type "+MEMOBJECTTEXT(Obj)+"<"+TypeDesc+">";
    ThrowBaseException((int)ExceptionSource::Allocator,(int)AllocatorException::OutOfMemory,Msg.c_str());
  }
  
  //Account memory
  std::string TypeDesc;
  CplusGetTypeDesc(datatype,TypeDesc);
  _AloCtr.Account(Obj,TypeDesc.c_str(),Elements,sizeof(datatype));

  //Return pointer
  return Temp;

}

//Free memory
template <typename datatype> 
void Allocator<datatype>::Give(MemObject Obj,long Elements,datatype *Ptr){

  //Deallocate memory
  delete[] Ptr;
  
  //Account memory
  std::string TypeDesc;
  CplusGetTypeDesc(datatype,TypeDesc);
  _AloCtr.Account(Obj,TypeDesc.c_str(),-Elements,sizeof(datatype));

}

#endif