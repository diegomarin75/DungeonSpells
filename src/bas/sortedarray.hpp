//SortedArray.hpp: Header file for array class
//IMPORTANT: datatype must implement a method named SortKey() otherwise this class will not compile. SortKey() returns array sorting key for each record.
//NOTE: Class templates do not have a separated implementation file.
//All needs to be defined in the header otherwise we will have linking error
//Reason is that template objects are not totally resolved by compiler so they cant be compiled separatedly on a cpp file
//See: https://bytefreaks.net/programming-2/c/c-undefined-reference-to-templated-class-function
//ChunkSize has direct impact on performance of array insertion, memory re-allocation is time consuming. Less re-allocations, more performance.

#ifndef _SORTEDARRAY_HPP
#define _SORTEDARRAY_HPP

//Exception numbers
enum class SortedArrayException{
  IndexOutOfBounds=1
};

//SortedArray class
template <typename datatype, typename sortkey> 
class SortedArray {

  //Private members
  private:
    
    //Constants
    const int CHUNK_SIZE = 16;
    const int CHUNK_PARTS = 8;

    //Internal variables
    datatype *_Arr;  //Data array
    int _ArrNr;      //Array occupied entries
    int _ArrSize;    //Array reserved space
    int _ChunkSize;  //Array increment size
    int _AddIndex;   //Last added index

    //Internal functions
    void _Allocate(int Size);
    void _Free();
    int _BinarySearch(sortkey Key,bool Approx, int Min, int Max) const;

  //Public members
  public:

    //Constructors
    SortedArray();
    SortedArray(int ChunkSize);
    
    //Destructor
    ~SortedArray();

    //Member functions
    int Length() const;
    int Size() const;
    void Add(const datatype& Element);
    int Search(sortkey Key,bool Approx=false) const;
    int Search(sortkey Key,int Min,int Max,bool Approx=false) const;
    void Delete(int Index);
    void Reset();
    int LastAdded() const;

    //Operators
    datatype& operator[](int Index) const;
    SortedArray<datatype,sortkey>& operator=(const SortedArray<datatype,sortkey>& Arr);
    template <typename datatypex,typename sortkeyx> friend std::ostream& operator<<(std::ostream& stream, const SortedArray<datatypex,sortkeyx>& Arr);
    
};

// --- Implementation part (also included in the header file to avoid linking errors) ---

//Constructor from nothing
template <typename datatype, typename sortkey> 
SortedArray<datatype,sortkey>::SortedArray(){
  _Arr=nullptr;
  _ArrNr=0;
  _ArrSize=0;
  _ChunkSize=CHUNK_SIZE;
  _AddIndex=-1;
}

//Constructor with chunk size
template <typename datatype, typename sortkey> 
SortedArray<datatype,sortkey>::SortedArray(int ChunkSize){
  _Arr=nullptr;
  _ArrNr=0;
  _ArrSize=0;
  _ChunkSize=ChunkSize;
  _Allocate(ChunkSize);
  _ArrSize=ChunkSize;
  _AddIndex=-1;
}

//Destructor
template <typename datatype, typename sortkey> 
SortedArray<datatype,sortkey>::~SortedArray(){
  _Free();
  _Arr=nullptr;
  _ArrNr=0;
  _ArrSize=0;
  _AddIndex=-1;
}

//Allocate space for array
template <typename datatype, typename sortkey> 
void SortedArray<datatype,sortkey>::_Allocate(int Size){

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
    Temp=Allocator<datatype>::Take(MemObject::SortedArray,_ArrSize);
    #endif
    
    //Reallocate elements
    for(int i=0;i<std::min(_ArrNr,Size);i++){ Temp[i]=_Arr[i]; }
    #ifdef __NOALLOC__
    delete[] _Arr;
    #else
    Allocator<datatype>::Give(MemObject::SortedArray,PrevSize,_Arr);
    #endif
    _Arr=Temp;

  }


}

//Free space for array
template <typename datatype, typename sortkey> 
void SortedArray<datatype,sortkey>::_Free(){
  #ifdef __NOALLOC__
  delete[] _Arr;
  #else
  Allocator<datatype>::Give(MemObject::SortedArray,_ArrSize,_Arr);
  #endif
}

//Binary search (Approx=0, returns -1 when not found. Approx=1, returns insertion position, never -1)
template <typename datatype, typename sortkey> 
int SortedArray<datatype,sortkey>::_BinarySearch(sortkey Key, bool Approx, int Min, int Max) const {
  
  //Variables
  int i = 0;
  int Low = 0;
  int High = 0;
  
  //Init loop
  if(Min>=0 && Max<=_ArrNr-1 && Min<=Max){
    Low=Min;
    High=Max;
  }
  else{
    return -1;
  }

  //Loop
  while(Low<=High){
    i=(Low+High)/2;
    if(Key==_Arr[i].SortKey()){
      return i;
    } 
    else if(Key<_Arr[i].SortKey()){
      High=i-1;
    } 
    else if(Key>_Arr[i].SortKey()){
      Low=i+1;
    }
  }

  //Return code
  if(!Approx){
    return -1;
  }
  else{
    return (Key>_Arr[i].SortKey()?i+1:i);
  }

}

//Get array length
template <typename datatype, typename sortkey> 
int SortedArray<datatype,sortkey>::Length() const{
  return _ArrNr;
}

//Get array allocated size
template <typename datatype, typename sortkey> 
int SortedArray<datatype,sortkey>::Size() const{
  return _ArrSize;
}

//Add element by sorted insertion
template <typename datatype, typename sortkey> 
void SortedArray<datatype,sortkey>::Add(const datatype& Element){
  
  //Allocate memory and increase array size for 1 more element
  _Allocate(_ArrNr+1);

  //Special case when array is empty
  if(_ArrNr==0) {
    _Arr[0]=Element;
    _ArrNr=1; 
    _AddIndex=0;
    return;
  }

  //Find insertion position
  int Pos=_BinarySearch(Element.SortKey(),true,0,_ArrNr-1);

  //Make a hole in the array
  for(int i=_ArrNr-1;i>=Pos;i+=-1){
    _Arr[i+1]=_Arr[i];
  }

  //Insert element in hole
  _Arr[Pos]=Element;
  
  //Increase array size
  _ArrNr++;

  //Keep added index
  _AddIndex=Pos;
  
}

//Search within entire array
template <typename datatype, typename sortkey> 
int SortedArray<datatype,sortkey>::Search(sortkey Key,bool Approx) const{
  return _BinarySearch(Key,Approx,0,_ArrNr-1);
}

//Search within part of array
template <typename datatype, typename sortkey> 
int SortedArray<datatype,sortkey>::Search(sortkey Key, int Min, int Max,bool Approx) const{
  return _BinarySearch(Key,Approx,Min,Max);
}

//Delete element
template <typename datatype, typename sortkey> 
void SortedArray<datatype,sortkey>::Delete(int Index){
  if(Index<0 || Index>_ArrNr-1){ return; }
  for(int i=Index;i<_ArrNr-1;i++){
    _Arr[i]=_Arr[i+1];
  }
  _Allocate(_ArrNr-1);
  _ArrNr--;
}

//Reset array
template <typename datatype, typename sortkey> 
void SortedArray<datatype,sortkey>::Reset(){
  _Free();
  _Arr=nullptr;
  _ArrNr=0;
  _ArrSize=0;
}

//Returns last added index
template <typename datatype, typename sortkey> 
int SortedArray<datatype,sortkey>::LastAdded() const {
  return _AddIndex;
}

//Access operator
template <typename datatype, typename sortkey> 
datatype& SortedArray<datatype,sortkey>::operator[](int Index) const{
  if(Index<0 || Index>_ArrNr-1){
    std::string TypeDesc;
    CplusGetTypeDesc(datatype,TypeDesc);
    std::string Msg="Invalid access for sorted array (datatype: " + TypeDesc + "). Requested index is " + std::to_string(Index) + " but array size is " + std::to_string(_ArrNr);
    ThrowBaseException((int)ExceptionSource::SortedArray,(int)SortedArrayException::IndexOutOfBounds,Msg.c_str());
  }
  return _Arr[Index];
}

//Assignment operator
template <typename datatype, typename sortkey> 
SortedArray<datatype,sortkey>& SortedArray<datatype,sortkey>::operator=(const SortedArray<datatype,sortkey>& Arr){
  _Free();
  _Arr=nullptr;
  _ArrNr=0;
  _ArrSize=0;
  _ChunkSize=Arr._ChunkSize;
  for(int i=0;i<Arr.Length();i++){ Add(Arr[i]); }
  return *this;
}

//Output to stream
template <typename datatype, typename sortkey> 
std::ostream& operator<<(std::ostream& stream, const SortedArray<datatype,sortkey>& Arr){
  for(int i=0;i<Arr._ArrNr;i++){ stream << "[" << i << "]: " << Arr._Arr[i] << std::endl; }
  return stream;
}  

#endif

