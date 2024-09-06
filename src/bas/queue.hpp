//Queue.hpp: Header file for queue class
//NOTE: Class templates do not have a separated implementation file.
//All needs to be defined in the header otherwise we will have linking error
//Reason is that template objects are not totally resolved by compiler so they cant be compiled separatedly on a cpp file
//See: https://bytefreaks.net/programming-2/c/c-undefined-reference-to-templated-class-function
//ChunkSize has direct impact on performance of array insertion, memory re-allocation is time consuming. Less re-allocations, more performance.

#ifndef _QUEUE_HPP
#define _QUEUE_HPP

//Exception numbers
enum class QueueException{
  IndexOutOfBounds=1,
  QueueUnderflow=2
};

//Queue class
template <typename datatype> 
class Queue {

  //Private members
  private:
    
    //Constants
    const int CHUNK_SIZE = 16;
    const int CHUNK_PARTS = 8;

    //Internal variables
    datatype *_Queue;
    int _QueueNr;
    int _QueueSize;
    int _ChunkSize;

    //Internal functions
    void _Allocate(int Size);
    void _Free();

  //Public members
  public:

    //Constructors
    Queue();
    Queue(int ChunkSize);
    Queue(const std::initializer_list<datatype> List);

    //Destructor
    ~Queue();

    //Member functions
    int Length() const;
    int Size() const;
    void Enqueue(const datatype& Element);
    void EnqueueFirst(const datatype& Element);
    datatype Dequeue();
    datatype& Top() const;
    datatype& Top(int Index) const;
    datatype& Bottom() const;
    datatype& Bottom(int Index) const;
    void Reset();

    //Operators
    Queue<datatype>& operator=(const Queue<datatype>& Qeu);
    template <typename datatypex> friend std::ostream& operator<<(std::ostream& stream, const Queue<datatypex>& Qeu);

};

// --- Implementation part (also included in the header file to avoid linking errors) ---

//Constructor from nothing
template <typename datatype> 
Queue<datatype>::Queue(){
  _Queue=nullptr;
  _QueueNr=0;
  _QueueSize=0;
  _ChunkSize=CHUNK_SIZE;
}

//Constructor with chunk size
template <typename datatype> 
Queue<datatype>::Queue(int ChunkSize){
  _Queue=nullptr;
  _QueueNr=0;
  _QueueSize=0;
  _ChunkSize=ChunkSize;
  _Allocate(ChunkSize);
  _QueueSize=ChunkSize;
}

//Constructor with initializer list
template <typename datatype> 
Queue<datatype>::Queue(const std::initializer_list<datatype> List){
  typename std::initializer_list<datatype>::iterator Pnt;
  _Queue=nullptr;
  _QueueNr=0;
  _QueueSize=0;
  _ChunkSize=List.size();
  for(Pnt=List.begin();Pnt<List.end();Pnt++){ Enqueue(*Pnt);  }
}

//Destructor
template <typename datatype> 
Queue<datatype>::~Queue(){
  _Free();
  _Queue=nullptr;
  _QueueNr=0;
  _QueueSize=0;
}

//Allocate space for queue
template <typename datatype> 
void Queue<datatype>::_Allocate(int Size){
  
  //Variables
  datatype *Temp;
  bool ReAllocate=false;
  
  //Save current size
  #ifndef __NOALLOC__
  int PrevSize;
  PrevSize=_QueueSize;
  #endif

  //Increase size
  if(Size>_QueueSize){
    while(_QueueSize<Size){ _QueueSize+=_ChunkSize; }
    if(_QueueSize/_ChunkSize>CHUNK_PARTS){ 
      while(_QueueSize/_ChunkSize>CHUNK_PARTS){ _ChunkSize*=2; }
    }
    ReAllocate=true;
  }
  else if(Size<_QueueSize-_ChunkSize){
    while(Size<_QueueSize-_ChunkSize){ _QueueSize-=_ChunkSize; }
    ReAllocate=true;
  }

  //Reallocation of elements
  if(ReAllocate){
    
    //Get memory
    #ifdef __NOALLOC__
    Temp=new datatype[_QueueSize];
    #else
    Temp=Allocator<datatype>::Take(MemObject::Queue,_QueueSize);
    #endif

    //Reallocate elements
    for(int i=0;i<std::min(_QueueNr,Size);i++){ Temp[i]=_Queue[i]; }
    #ifdef __NOALLOC__
    delete[] _Queue;
    #else
    Allocator<datatype>::Give(MemObject::Queue,PrevSize,_Queue);
    #endif
    _Queue=Temp;

  }

}

//Free space for queue
template <typename datatype> 
void Queue<datatype>::_Free(){
  #ifdef __NOALLOC__
  delete[] _Queue;
  #else
  Allocator<datatype>::Give(MemObject::Queue,_QueueSize,_Queue);
  #endif
}

//Get queue length
template <typename datatype> 
int Queue<datatype>::Length() const{
  return _QueueNr;
}

//Get queue allocated size
template <typename datatype> 
int Queue<datatype>::Size() const{
  return _QueueSize;
}

//Queue element
template <typename datatype> 
void Queue<datatype>::Enqueue(const datatype& Element){
  _Allocate(_QueueNr+1);
  _QueueNr++;
  _Queue[_QueueNr-1]=Element;
}

//Queue element in first position
template <typename datatype> 
void Queue<datatype>::EnqueueFirst(const datatype& Element){
  
  //Special case when queue is empty
  if(_QueueNr==0){
    _Allocate(_QueueNr+1);
    _QueueNr++;
    _Queue[0]=Element;
    return;
  }

  //Allocate one more element
  _Allocate(_QueueNr+1);

  //Make a hole in the array
  for(int i=_QueueNr-1;i>=0;i+=-1){
    _Queue[i+1]=_Queue[i];
  }

  //Copy element
  _Queue[0]=Element;

  //Increase array size
  _QueueNr++;

}

//Dequeue element
template <typename datatype> 
datatype Queue<datatype>::Dequeue(){
  if(_QueueNr==0){
    std::string TypeDesc;
    CplusGetTypeDesc(datatype,TypeDesc);
    std::string Msg="Queue underflow (datatype: " + TypeDesc + ")";
    ThrowBaseException((int)ExceptionSource::Queue,(int)QueueException::QueueUnderflow,Msg.c_str());
  }
  datatype Element=_Queue[0];
  for(int i=0;i<_QueueNr-1;i++){ _Queue[i]=_Queue[i+1]; }
  _Allocate(_QueueNr-1);
  _QueueNr--;
  return Element;
}

//Access from top
template <typename datatype> 
datatype& Queue<datatype>::Top() const {
  return _Queue[_QueueNr-1];
}

//Access from top
template <typename datatype> 
datatype& Queue<datatype>::Top(int Index) const {
  if(Index>0 || _QueueNr-1-Index<0){
    std::string TypeDesc;
    CplusGetTypeDesc(datatype,TypeDesc);
    std::string Msg="Queue top access out of bounds (datatype: " + TypeDesc + "). Requested index is " + std::to_string(Index) + " but queue size is " + std::to_string(_QueueNr);
    ThrowBaseException((int)ExceptionSource::Queue,(int)QueueException::IndexOutOfBounds,Msg.c_str());
  }
  return _Queue[_QueueNr-1+Index];
}

//Access to bottom
template <typename datatype> 
datatype& Queue<datatype>::Bottom() const {
  return _Queue[0];
}

//Access from bottom
template <typename datatype> 
datatype& Queue<datatype>::Bottom(int Index) const {
  if(Index<0 || Index>_QueueNr-1){
    std::string TypeDesc;
    CplusGetTypeDesc(datatype,TypeDesc);
    std::string Msg="Queue bottom access out of bounds (datatype: " + TypeDesc + "). Requested index is " + std::to_string(Index) + " but queue size is " + std::to_string(_QueueNr);
    ThrowBaseException((int)ExceptionSource::Queue,(int)QueueException::IndexOutOfBounds,Msg.c_str());
  }
  return _Queue[Index];
}

//Reset queue
template <typename datatype> 
void Queue<datatype>::Reset(){
  _Free();
  _Queue=nullptr;
  _QueueNr=0;
  _QueueSize=0;
}

//Assignment operator
template <typename datatype> 
Queue<datatype>& Queue<datatype>::operator=(const Queue<datatype>& Qeu){
  _Free();
  _Queue=nullptr;
  _QueueNr=0;
  _QueueSize=0;
  _ChunkSize=Qeu._ChunkSize;
  for(int i=0;i<Qeu.Length();i++){ Enqueue(Qeu.Bottom(i)); }
  return *this;
}

//Output to stream
template <typename datatype> 
std::ostream& operator<<(std::ostream& stream, const Queue<datatype>& Qeu){
  for(int i=0;i<Qeu._QueueNr;i++){ stream << "[" << i << "]: " << Qeu._Queue[i] << std::flush; }
  return stream;
}  

#endif

