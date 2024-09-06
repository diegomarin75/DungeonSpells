//Array.hpp: Header file for array class
//NOTE: Class templates do not have a separated implementation file.
//All needs to be defined in the header otherwise we will have linking error
//Reason is that template objects are not totally resolved by compiler so they cant be compiled separatedly on a cpp file
//See: https://bytefreaks.net/programming-2/c/c-undefined-reference-to-templated-class-function
//ChunkSize has direct impact on performance of array insertion, memory re-allocation is time consuming. Less re-allocations, more performance.

#ifndef _ARRAY_HPP
#define _ARRAY_HPP

//Exception numbers
enum class ArrayException{
  IndexOutOfBounds=1,
  InvalidFromToDelete=2
};

//Array class
template <typename datatype> 
class Array {

  //Private members
  private:
    
    //Constants
    const int CHUNK_SIZE = 16;
    const int CHUNK_PARTS = 8;

    //Internal variables
    datatype *_Arr;
    int _ArrNr;
    int _ArrSize;
    int _ChunkSize;

    //Internal functions
    void _Allocate(int Size);
    void _Free();

  //Public members
  public:

    //Constructors
    Array();
    Array(int ChunkSize);
    Array(const std::initializer_list<datatype> List);
    
    //Destructor
    ~Array();

    //Member functions
    int Length() const;
    int Size() const;
    void Add(const datatype& Element);
    void Insert(int Index,const datatype& Element);
    void Delete(int Index);
    void Delete(int From,int To);
    void Clear();
    void Reset();
    void Resize(int Nr);
    datatype Last() const;
    int Search(const datatype& Element);

    //Operators
    datatype& operator[](int Index) const;
    Array<datatype>& operator=(const Array<datatype>& Arr);
    Array<datatype>& operator+=(const Array<datatype>& Arr);
    template <typename datatypex> friend std::ostream& operator<<(std::ostream& stream, const Array<datatypex>& Arr);

};

// --- Implementation part (also included in the header file to avoid linking errors) ---

//Constructor from nothing
template <typename datatype> 
Array<datatype>::Array(){
  _Arr=nullptr;
  _ArrNr=0;
  _ArrSize=0;
  _ChunkSize=CHUNK_SIZE;
}

//Constructor with chunk size
template <typename datatype> 
Array<datatype>::Array(int ChunkSize){
  _Arr=nullptr;
  _ArrNr=0;
  _ArrSize=0;
  _ChunkSize=ChunkSize;
  _Allocate(ChunkSize);
  _ArrSize=ChunkSize;
}

//Constructor with initializer list
template <typename datatype> 
Array<datatype>::Array(const std::initializer_list<datatype> List){
  typename std::initializer_list<datatype>::iterator Pnt;
  _Arr=nullptr;
  _ArrNr=0;
  _ArrSize=0;
  _ChunkSize=List.size();
  for(Pnt=List.begin();Pnt<List.end();Pnt++){ Add(*Pnt); }
}

//Destructor
template <typename datatype> 
Array<datatype>::~Array(){
  _Free();
  _Arr=nullptr;
  _ArrNr=0;
  _ArrSize=0;
}

//Allocate space for array
template <typename datatype> 
void Array<datatype>::_Allocate(int Size){
  
  //Variables
  datatype *Temp;
  bool ReAllocate=false;
  
  //Save current size
  #ifndef __NOALLOC__
  int PrevSize;
  PrevSize=_ArrSize;
  #endif

  //Increase size
  if(Size>_ArrSize){
    while(_ArrSize<Size){ _ArrSize+=_ChunkSize; }
    if(_ArrSize/_ChunkSize>CHUNK_PARTS){ 
      while(_ArrSize/_ChunkSize>CHUNK_PARTS){ _ChunkSize*=2; }
    }
    ReAllocate=true;
  }
  else if(Size<_ArrSize-_ChunkSize){
    while(Size<_ArrSize-_ChunkSize){ _ArrSize-=_ChunkSize; }
    ReAllocate=true;
  }

  //Reallocation of elements
  if(ReAllocate){
    
    //Get memory
    #ifdef __NOALLOC__
    Temp=new datatype[_ArrSize];
    #else
    Temp=Allocator<datatype>::Take(MemObject::Array,_ArrSize);
    #endif

    //Reallocate elements
    for(int i=0;i<std::min(_ArrNr,Size);i++){ Temp[i]=_Arr[i]; }
    #ifdef __NOALLOC__
    delete[] _Arr;
    #else
    Allocator<datatype>::Give(MemObject::Array,PrevSize,_Arr);
    #endif
    _Arr=Temp;

  }

}

//Free memory
template <typename datatype> 
void Array<datatype>::_Free(){
  #ifdef __NOALLOC__
  delete[] _Arr;
  #else
  Allocator<datatype>::Give(MemObject::Array,_ArrSize,_Arr);
  #endif
}

//Get array length
template <typename datatype> 
int Array<datatype>::Length() const{
  return _ArrNr;
}

//Get array allocated size
template <typename datatype> 
int Array<datatype>::Size() const{
  return _ArrSize;
}

//Append element
template <typename datatype> 
void Array<datatype>::Add(const datatype& Element){
  _Allocate(_ArrNr+1);
  _ArrNr++;
  _Arr[_ArrNr-1]=Element;
}

//Insert element
template <typename datatype> 
void Array<datatype>::Insert(int Index,const datatype& Element){
  
  //Special case when array is empty and index is zero
  if(_ArrNr==0 && Index==0){
    _Allocate(_ArrNr+1);
    _ArrNr++;
    _Arr[0]=Element;
    return;
  }

  //Check index out of bounds
  if(Index<0 || Index>_ArrNr-1){
    std::string TypeDesc;
    CplusGetTypeDesc(datatype,TypeDesc);
    std::string Msg="Invalid access for array (datatype: " + TypeDesc + "). Requested index is " + std::to_string(Index) + " but array size is " + std::to_string(_ArrNr);
    ThrowBaseException((int)ExceptionSource::Array,(int)ArrayException::IndexOutOfBounds,Msg.c_str());
  }

  //Allocate one more element
  _Allocate(_ArrNr+1);

  //Make a hole in the array
  for(int i=_ArrNr-1;i>=Index;i+=-1){
    _Arr[i+1]=_Arr[i];
  }

  //Copy element
  _Arr[Index]=Element;

  //Increase array size
  _ArrNr++;

}

//Delete element
template <typename datatype> 
void Array<datatype>::Delete(int Index){
  if(Index<0 || Index>_ArrNr-1){
    std::string TypeDesc;
    CplusGetTypeDesc(datatype,TypeDesc);
    std::string Msg="Invalid access for array (datatype: " + TypeDesc + "). Requested index is " + std::to_string(Index) + " but array size is " + std::to_string(_ArrNr);
    ThrowBaseException((int)ExceptionSource::Array,(int)ArrayException::IndexOutOfBounds,Msg.c_str());
  }
    for(int i=Index;i<_ArrNr-1;i++){
    _Arr[i]=_Arr[i+1];
  }
  _Allocate(_ArrNr-1);
  _ArrNr--;
}

//Delete elements
template <typename datatype> 
void Array<datatype>::Delete(int From,int To){
  if(From<0){
    std::string TypeDesc;
    CplusGetTypeDesc(datatype,TypeDesc);
    std::string Msg="Invalid access for array (datatype: " + TypeDesc + "). Requested index is " + std::to_string(From) + " but array size is " + std::to_string(_ArrNr);
    ThrowBaseException((int)ExceptionSource::Array,(int)ArrayException::IndexOutOfBounds,Msg.c_str());
  }
  if(To>_ArrNr-1){
    std::string TypeDesc;
    CplusGetTypeDesc(datatype,TypeDesc);
    std::string Msg="Invalid access for array (datatype: " + TypeDesc + "). Requested index is " + std::to_string(To) + " but array size is " + std::to_string(_ArrNr);
    ThrowBaseException((int)ExceptionSource::Array,(int)ArrayException::IndexOutOfBounds,Msg.c_str());
  }
  if(To<From){
    std::string TypeDesc;
    CplusGetTypeDesc(datatype,TypeDesc);
    std::string Msg="Invalid delete operation for array (datatype: " + TypeDesc + "). To index ("+std::to_string(To) + ") is lower than from index ("+ std::to_string(From)+")";
    ThrowBaseException((int)ExceptionSource::Array,(int)ArrayException::InvalidFromToDelete,Msg.c_str());
  }
  int Elements=To-From+1;
  for(int i=From;i<_ArrNr-Elements;i++){
    _Arr[i]=_Arr[i+Elements];
  }
  _Allocate(_ArrNr-Elements);
  _ArrNr-=Elements;
}


//Resize array to nr elements
template <typename datatype> 
void Array<datatype>::Resize(int Nr){
  _Allocate(Nr);
  _ArrNr=Nr;
}

//Clear array (does not free up memory)
template <typename datatype> 
void Array<datatype>::Clear(){
  _ArrNr=0;
}

//Reset array
template <typename datatype> 
void Array<datatype>::Reset(){
  _Free();
  _Arr=nullptr;
  _ArrNr=0;
  _ArrSize=0;
}

//Return last element in array
template <typename datatype> 
datatype Array<datatype>::Last() const{
  if(_ArrNr==0){
    std::string TypeDesc;
    CplusGetTypeDesc(datatype,TypeDesc);
    std::string Msg="Invalid access for array (datatype: " + TypeDesc + "). Requested index is last element but array size is " + std::to_string(_ArrNr);
    ThrowBaseException((int)ExceptionSource::Array,(int)ArrayException::IndexOutOfBounds,Msg.c_str());
  }
  return _Arr[_ArrNr-1];
}

//Array search
template <typename datatype> 
int Array<datatype>::Search(const datatype& Element){
  for(int i=0;i<_ArrNr;i++){ if(Element==_Arr[i]){ return i; } }
  return -1;
}

//Access operator
template <typename datatype> 
datatype& Array<datatype>::operator[](int Index) const {
  if(Index<0 || Index>_ArrNr-1){
    std::string TypeDesc;
    CplusGetTypeDesc(datatype,TypeDesc);
    std::string Msg="Invalid access for array (datatype: " + TypeDesc + "). Requested index is " + std::to_string(Index) + " but array size is " + std::to_string(_ArrNr);
    ThrowBaseException((int)ExceptionSource::Array,(int)ArrayException::IndexOutOfBounds,Msg.c_str());
  }
  return _Arr[Index];
}

//Assignment operator
template <typename datatype> 
Array<datatype>& Array<datatype>::operator=(const Array<datatype>& Arr){
  _Free();
  _Arr=nullptr;
  _ArrNr=0;
  _ArrSize=0;
  _ChunkSize=Arr._ChunkSize;
  for(int i=0;i<Arr.Length();i++){ Add(Arr[i]); }
  return *this;
}

//Plus assignment
template <typename datatype> 
Array<datatype>& Array<datatype>::operator+=(const Array<datatype>& Arr){
  for(int i=0;i<Arr.Length();i++){ Add(Arr[i]); }
  return *this;
}

//Output to stream
template <typename datatype> 
std::ostream& operator<<(std::ostream& stream, const Array<datatype>& Arr){
  for(int i=0;i<Arr._ArrNr;i++){ stream << "[" << i << "]: " << Arr._Arr[i] << std::flush; }
  return stream;
}  

#endif