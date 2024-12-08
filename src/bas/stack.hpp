//Stack.hpp: Header file for strack class
//NOTE: Class templates do not have a separated implementation file.
//All needs to be defined in the header otherwise we will have linking error
//Reason is that template objects are not totally resolved by compiler so they cant be compiled separatedly on a cpp file
//See: https://bytefreaks.net/programming-2/c/c-undefined-reference-to-templated-class-function
//ChunkSize has direct impact on performance of array insertion, memory re-allocation is time consuming. Less re-allocations, more performance.

#ifndef _STACK_HPP
#define _STACK_HPP

//Exception numbers
enum class StackException{
  IndexOutOfBounds=1,
  StackUnderflow=2
};

//Stack class
template <typename datatype> 
class Stack {

  //Private members
  private:
    
    //Constants
    const int CHUNK_SIZE = 16;
    const int CHUNK_PARTS = 8;

    //Internal variables
    datatype *_Stack;
    int _StackNr;
    int _StackSize;
    int _ChunkSize;

    //Internal functions
    void _Allocate(int Size);
    void _Free();

  //Public members
  public:

    //Constructors
    Stack();
    Stack(int ChunkSize);
    Stack(const std::initializer_list<datatype> List);

    //Destructor
    ~Stack();

    //Member functions
    int Length() const;
    int Size() const;
    void Push(const datatype& Element);
    datatype Pop();
    void Pop(int Elements);
    datatype& Top() const;
    datatype& Top(int Index) const;
    datatype& Bottom() const;
    datatype& Bottom(int Index) const;
    void Reset();
    int Search(const datatype& Element);

    //Operators
    datatype& operator[](int Index) const;
    Stack<datatype>& operator=(const Stack<datatype>& Stk);
    template <typename datatypex> friend std::ostream& operator<<(std::ostream& stream, const Stack<datatypex>& Stk);

};

// --- Implementation part (also included in the header file to avoid linking errors) ---

//Constructor from nothing
template <typename datatype> 
Stack<datatype>::Stack(){
  _Stack=nullptr;
  _StackNr=0;
  _StackSize=0;
  _ChunkSize=CHUNK_SIZE;
}

//Constructor with chunk size
template <typename datatype> 
Stack<datatype>::Stack(int ChunkSize){
  _Stack=nullptr;
  _StackNr=0;
  _StackSize=0;
  _ChunkSize=ChunkSize;
  _Allocate(ChunkSize);
  _StackSize=ChunkSize;
}

//Constructor with initializer list
template <typename datatype> 
Stack<datatype>::Stack(const std::initializer_list<datatype> List){
  typename std::initializer_list<datatype>::iterator Pnt;
  _Stack=nullptr;
  _StackNr=0;
  _StackSize=0;
  _ChunkSize=List.size();
  for(Pnt=List.begin();Pnt<List.end();Pnt++){ Push(*Pnt); }
}

//Destructor
template <typename datatype> 
Stack<datatype>::~Stack(){
  _Free();
  _Stack=nullptr;
  _StackNr=0;
  _StackSize=0;
}

//Allocate space for stack
template <typename datatype> 
void Stack<datatype>::_Allocate(int Size){
  
  //Variables
  datatype *Temp;
  bool ReAllocate=false;
  
  //Save current size
  #ifndef __NOALLOC__
  int PrevSize;
  PrevSize=_StackSize;
  #endif

  //Increase size
  if(Size>_StackSize){
    while(_StackSize<Size){ _StackSize+=_ChunkSize; }
    if(_StackSize/_ChunkSize>CHUNK_PARTS){ 
      while(_StackSize/_ChunkSize>CHUNK_PARTS){ _ChunkSize*=2; }
    }
    ReAllocate=true;
  }
  else if(Size<_StackSize-_ChunkSize){
    while(Size<_StackSize-_ChunkSize){ _StackSize-=_ChunkSize; }
    ReAllocate=true;
  }

  //Reallocation of elements
  if(ReAllocate){
    
    //Get memory
    #ifdef __NOALLOC__
    Temp=new datatype[_StackSize];
    #else
    Temp=Allocator<datatype>::Take(MemObject::Stack,_StackSize);
    #endif
    
    //Reallocate elements
    for(int i=0;i<std::min(_StackNr,Size);i++){ Temp[i]=_Stack[i]; }
    #ifdef __NOALLOC__
    delete[] _Stack;
    #else
    Allocator<datatype>::Give(MemObject::Stack,PrevSize,_Stack);
    #endif
    _Stack=Temp;

  }

}

//Free space for stack
template <typename datatype> 
void Stack<datatype>::_Free(){
  #ifdef __NOALLOC__
  delete[] _Stack;
  #else
  Allocator<datatype>::Give(MemObject::Stack,_StackSize,_Stack);
  #endif
}

//Get stack length
template <typename datatype> 
int Stack<datatype>::Length() const{
  return _StackNr;
}

//Get stack allocated size
template <typename datatype> 
int Stack<datatype>::Size() const{
  return _StackSize;
}

//Push element
template <typename datatype> 
void Stack<datatype>::Push(const datatype& Element){
  _Allocate(_StackNr+1);
  _StackNr++;
  _Stack[_StackNr-1]=Element;
}

//Pop element
template <typename datatype> 
datatype Stack<datatype>::Pop(){
  if(_StackNr==0){
    std::string TypeDesc;
    CplusGetTypeDesc(datatype,TypeDesc);
    std::string Msg="Stack underflow (datatype: " + TypeDesc + ") at pop 1 element operation";
    ThrowBaseException((int)ExceptionSource::Stack,(int)StackException::StackUnderflow,Msg.c_str());
  }
  datatype Element=_Stack[_StackNr-1];
  _Allocate(_StackNr-1);
  _StackNr--;
  return Element;
}

//Pop elements
template <typename datatype> 
void Stack<datatype>::Pop(int Elements){
  if(_StackNr<Elements){
    std::string TypeDesc;
    CplusGetTypeDesc(datatype,TypeDesc);
    std::string Msg="Stack underflow (datatype: " + TypeDesc + ") at pop "+std::to_string(Elements)+" elements operation";
    ThrowBaseException((int)ExceptionSource::Stack,(int)StackException::StackUnderflow,Msg.c_str());
  }
  _Allocate(_StackNr-Elements);
  _StackNr-=Elements;
  return;
}

//Access to top
template <typename datatype> 
datatype& Stack<datatype>::Top() const {
  if(_StackNr==0){
    std::string TypeDesc;
    CplusGetTypeDesc(datatype,TypeDesc);
    std::string Msg="Stack top access out of bounds (datatype: " + TypeDesc + "). Requested index is " + std::to_string(0) + " but stack size is " + std::to_string(_StackNr);
    ThrowBaseException((int)ExceptionSource::Stack,(int)StackException::IndexOutOfBounds,Msg.c_str());
  }
  return _Stack[_StackNr-1];
}

//Access from top
template <typename datatype> 
datatype& Stack<datatype>::Top(int Index) const {
  if(Index>0 || _StackNr-1+Index<0){
    std::string TypeDesc;
    CplusGetTypeDesc(datatype,TypeDesc);
    std::string Msg="Stack top access out of bounds (datatype: " + TypeDesc + "). Requested index is " + std::to_string(Index) + " but stack size is " + std::to_string(_StackNr);
    ThrowBaseException((int)ExceptionSource::Stack,(int)StackException::IndexOutOfBounds,Msg.c_str());
  }
  return _Stack[_StackNr-1+Index];
}

//Access to bottom
template <typename datatype> 
datatype& Stack<datatype>::Bottom() const {
  if(_StackNr==0){
    std::string TypeDesc;
    CplusGetTypeDesc(datatype,TypeDesc);
    std::string Msg="Stack bottom access out of bounds (datatype: " + TypeDesc + "). Requested index is " + std::to_string(0) + " but stack size is " + std::to_string(_StackNr);
    ThrowBaseException((int)ExceptionSource::Stack,(int)StackException::IndexOutOfBounds,Msg.c_str());
  }
  return _Stack[0];
}

//Access from bottom
template <typename datatype> 
datatype& Stack<datatype>::Bottom(int Index) const {
  if(Index<0 || Index>_StackNr-1){
    std::string TypeDesc;
    CplusGetTypeDesc(datatype,TypeDesc);
    std::string Msg="Stack bottom access out of bounds (datatype: " + TypeDesc + "). Requested index is " + std::to_string(Index) + " but stack size is " + std::to_string(_StackNr);
    ThrowBaseException((int)ExceptionSource::Stack,(int)StackException::IndexOutOfBounds,Msg.c_str());
  }
  return _Stack[Index];
}

//Reset array
template <typename datatype> 
void Stack<datatype>::Reset(){
  _Free();
  _Stack=nullptr;
  _StackNr=0;
  _StackSize=0;
}

//Array search
template <typename datatype> 
int Stack<datatype>::Search(const datatype& Element){
  for(int i=0;i<_StackNr;i++){ if(Element==_Stack[i]){ return i; } }
  return -1;
}

//Access operator
template <typename datatype> 
datatype& Stack<datatype>::operator[](int Index) const{
  if(Index<0 || Index>_StackNr-1){
    std::string TypeDesc;
    CplusGetTypeDesc(datatype,TypeDesc);
    std::string Msg="Stack access out of bounds (datatype: " + TypeDesc + "). Requested index is " + std::to_string(Index) + " but stack size is " + std::to_string(_StackNr);
    ThrowBaseException((int)ExceptionSource::Stack,(int)StackException::IndexOutOfBounds,Msg.c_str());
  }
  return _Stack[Index];
}

//Assignment operator
template <typename datatype> 
Stack<datatype>& Stack<datatype>::operator=(const Stack<datatype>& Stk){
  _Free();
  _Stack=nullptr;
  _StackNr=0;
  _StackSize=0;
  _ChunkSize=Stk._ChunkSize;
  for(int i=0;i<Stk.Length();i++){ Push(Stk.Bottom(i)); }
  return *this;
}

//Output to stream
template <typename datatype> 
std::ostream& operator<<(std::ostream& stream, const Stack<datatype>& Stk){
  for(int i=0;i<Stk._StackNr;i++){ stream << "[" << i << "]: " << Stk._Stack[i] << std::flush; }
  return stream;
}  

#endif

