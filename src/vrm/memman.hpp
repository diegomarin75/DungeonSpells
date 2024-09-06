//memman.hpp: CPU Memory manager

//Wrap include
#ifndef _MEMMAN_HPP
#define _MEMMAN_HPP

//Memory definitions
#define MEMMAN_FREEBITS 64
#define MEMMAN_FREELIST 128

//Primary memory manager class
class MemoryManager{
  
  //Private menbers
  private:
    
    //Internal data
    static MemoryPool _MemoryPool; 

  //Public members
  public:
    
    //Memory allocation request
    inline static char *Alloc(int ProcessId,CpuWrd Size){
      char *Ptr;
      DebugMessage(DebugLevel::VrmMemory,"Memory manager allocation (processid="+ToString(ProcessId)+" size="+ToString(Size)+")");
      if((Ptr=_MemoryPool.Allocate(Size,ProcessId))==nullptr){
        DebugMessage(DebugLevel::VrmMemory,"Memory allocation failure (processid="+ToString(ProcessId)+" size="+ToString(Size)+")");
        return nullptr;
      }
      return Ptr;
    }

    //Memory re-allocation request
    inline static bool Realloc(char **Ptr,CpuWrd Size){
      DebugMessage(DebugLevel::VrmMemory,"Memory manager re-allocation (ptr="+PTRFORMAT(*Ptr)+" size="+ToString(Size)+")");
      if(!_MemoryPool.ReAllocate(Ptr,Size)){
        DebugMessage(DebugLevel::VrmMemory,"Memory re-allocation failure (ptr="+PTRFORMAT(*Ptr)+" requested_size="+ToString(Size));
        return false;
      }
      return true;
    }

    //Free memory
    inline static void Free(char *Ptr){
      DebugMessage(DebugLevel::VrmMemory,"Memory manager release (ptr="+PTRFORMAT(Ptr)+")");
      _MemoryPool.Free(Ptr);
    }

    //Handler methods
    static bool Init(CpuWrd Units,CpuWrd ChunkUnits,CpuWrd UnitSize,bool Lock);
    static void Terminate();
    #ifdef __DEV__
      static bool Check();
    #endif

    //Constructors/Destructors
    MemoryManager(){}
    ~MemoryManager(){}

};

//Ram buffer exception numbers
enum class RamBufferException{
  InvalidCall=1,
  IndexOutOfBounds=2,
  FreedElementsOverBufferSize=3
};

//Program buffer class
template <typename datatype> 
class RamBuffer {

  //Private members
  private:
    
    //Internal variables
    datatype *_Pnt;
    char *_Name;
    int _ProcessId;
    long _Nr;
    long _Size;
    long _ChunkSize;

    //Internal functions
    bool _Allocate(long Size);
    void _FreeException(long Elements);
    void _AccessException(int Index);

    //Check exception
    inline void _CheckException(const char *Function){
      if(_ProcessId==-1){
        std::string TypeDesc;
        CplusGetTypeDesc(datatype,TypeDesc);
        std::string Msg="Exception at "+std::string(Function)+"(), requested operation on RamBuffer<"+TypeDesc+">("+std::string(_Name)+") but buffer is not initialized";
        DebugException(String(Msg.c_str()));
        ThrowBaseException((int)ExceptionSource::RamBuffer,(int)RamBufferException::InvalidCall,Msg.c_str());
      }
    }
  
  //Public members
  public:

    //Constructor / Destructor
    RamBuffer();
    ~RamBuffer();

    //Get rambuffer data pointer
    inline datatype *Pnt(){ 
      return _Pnt; 
    }

    //Get array length (inlined for performance)
    inline long Length() const {
      return _Nr;
    }
    
    //Set process id and chunk size
    inline void Init(int ProcessId,long ChunkSize,char *Name){
      _ProcessId=ProcessId;
      _ChunkSize=ChunkSize;
      _Name=Name;
    }
    
    //Push element
    inline bool Push(const datatype& Element){
      _CheckException(__FUNCTION__);
      if(!_Allocate(_Nr+1)){ return false; }
      _Nr++;
      _Pnt[_Nr-1]=Element;
      return true;
    }
    
    //Pop element
    inline bool Pop(datatype& Element){
      _CheckException(__FUNCTION__);
      if(_Nr==0){ return false; }
      Element=_Pnt[_Nr-1];
      if(!_Allocate(_Nr-1)){ return false; }
      _Nr--;
      return true;
    }

    //Reserve memory for additional elements and initialize to zero
    inline bool Reserve(long Elements){
      _CheckException(__FUNCTION__);
      if(!_Allocate(_Nr+Elements)){ return false; }
      memset(_Pnt+_Nr,0,Elements*sizeof(datatype));
      _Nr+=Elements;
      return true;
    }

    //Free memory of last n elements
    inline bool Free(long Elements){
      _CheckException(__FUNCTION__);
      if(_Nr-Elements<0){ _FreeException(Elements); }
      if(!_Allocate(_Nr-Elements)){ return false; }
      _Nr-=Elements;
      return true;
    }

    //Append elements
    inline bool Append(const datatype *Ptr,long Length){
      _CheckException(__FUNCTION__);
      if(!_Allocate(_Nr+Length)){ return false; }
      MemCpy(_Pnt+_Nr,reinterpret_cast<const char *>(Ptr),Length*sizeof(datatype));
      _Nr+=Length;
      return true;
    }
    
    //Resize buffer to n elements
    inline bool Resize(long Length){
      _CheckException(__FUNCTION__);
      if(!_Allocate(Length)){ return false; }
      _Nr=Length;
      return true;
    }
    
    //Set buffer contents to zero
    inline void Clear(){
      _CheckException(__FUNCTION__);
      memset(_Pnt,0,_Nr*sizeof(datatype));
    }
    
    //Reset array
    inline void Reset(){
      if(_Pnt!=nullptr){ MemoryManager::Free(reinterpret_cast<char *>(_Pnt)); }
      _Nr=0;
      _Size=0;
      _Pnt=nullptr;
    }

    //Empty array (faster than reset as memory is not deallocated but set as unused)
    inline void Empty(){
      _Nr=0;
    }

    //Access operator (inlined for performance)
    inline datatype& operator[](int Index){
      if(Index<0 || Index>=_Nr){ _AccessException(Index); }
      return _Pnt[Index];
    }

    //Quick access (does not check buffer boundaries)
    inline datatype& Fetch(int Index){
      return _Pnt[Index];
    }

};

// Implementation of ram buffer class (because it is a template class)

//Constructor from nothing
template <typename datatype> 
RamBuffer<datatype>::RamBuffer(){
  _ProcessId=-1;
  _Nr=0;
  _Size=0;
  _ChunkSize=10;
  _Pnt=nullptr;
}

//Destructor
template <typename datatype> 
RamBuffer<datatype>::~RamBuffer(){
  if(_Pnt!=nullptr){ MemoryManager::Free(reinterpret_cast<char *>(_Pnt)); }
}

//Allocate items
template <typename datatype> 
bool RamBuffer<datatype>::_Allocate(long Size){
  
  //Variables
  char *Ptr;
  bool ResizeBuffer=false;

  //Debug message
  #ifdef __DEV__
   long Bytes=Size*sizeof(datatype);
    DebugAppend(DebugLevel::VrmMemory,"RamBuffer "+String(_Name)+" allocation for "+ToString(Bytes)+" bytes ("+ToString(Size)+" elements) (occupied="+ToString(_Nr)+" currsize="+ToString(_Size));
  #endif
  
  //Check primery memory controler pointer is valid
  _CheckException(__FUNCTION__);

  //Increase size
  if(Size>_Size){
    while(_Size<Size){ _Size+=_ChunkSize; }
    ResizeBuffer=true;
  }
  else if(Size<_Size-_ChunkSize){
    while(Size<_Size-_ChunkSize){ _Size-=_ChunkSize; }
    ResizeBuffer=true;
  }

  //Debug message
  DebugMessage(DebugLevel::VrmMemory," resize="+String((ResizeBuffer?"true":"false"))+" newsize="+ToString(_Size)+")");
  
  //Memory reallocation
  if(ResizeBuffer){
    if(_Pnt==nullptr){
      if((Ptr=MemoryManager::Alloc(_ProcessId,_Size*sizeof(datatype)))==nullptr){ return false; }
    }
    else{
      Ptr=reinterpret_cast<char *>(_Pnt);
      if(!MemoryManager::Realloc(&Ptr,_Size*sizeof(datatype))){ return false; }
    }
    _Pnt=reinterpret_cast<datatype *>(Ptr);
  }

  //Return code
  return true;

}

//Free exception (separated here as it improves performance)
template <typename datatype> 
void RamBuffer<datatype>::_FreeException(long Elements){
  std::string TypeDesc;
  CplusGetTypeDesc(datatype,TypeDesc);
  std::string Msg="RamBuffer<"+TypeDesc+">("+std::string(_Name)+") numbed of freed elements is over buffer size. Requested elements are " + std::to_string(Elements) + " but buffer size is " + std::to_string(_Nr)+" elements";
  DebugException(String(Msg.c_str()));
  ThrowBaseException((int)ExceptionSource::RamBuffer,(int)RamBufferException::FreedElementsOverBufferSize,Msg.c_str());
}

//Access operator exception (separated here as it improves performance)
template <typename datatype> 
void RamBuffer<datatype>::_AccessException(int Index){
  std::string TypeDesc;
  CplusGetTypeDesc(datatype,TypeDesc);
  std::string Msg="RamBuffer<"+TypeDesc+">("+std::string(_Name)+") access out of bounds. Requested index is " + std::to_string(Index) + " but buffer size is " + std::to_string(_Nr)+" elements";
  DebugException(String(Msg.c_str()));
  ThrowBaseException((int)ExceptionSource::RamBuffer,(int)RamBufferException::IndexOutOfBounds,Msg.c_str());
}

#endif