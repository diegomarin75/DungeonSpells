  //Buffer.cpp: Implementation of a Buffer class
#include "bas/basedefs.hpp"
#include "bas/allocator.hpp"
#include "bas/buffer.hpp"

//Constructor from nothing
Buffer::Buffer(){
  _Chr=nullptr;
  _Length=0;
  _Size=0;
  _ChunkSize=CHUNK_SIZE;
}

//Constructor from char
Buffer::Buffer(char Chr){
  _Chr=nullptr;
  _Length=0;
  _Size=0;
  _ChunkSize=CHUNK_SIZE;
  _Allocate(1);
  _Chr[0]=Chr;
  _Length=1;
}

//Constructor from char Buffer
Buffer::Buffer(const char *Pnt){
  _Chr=nullptr;
  _Length=0;
  _Size=0;
  _ChunkSize=CHUNK_SIZE;
  if(Pnt==nullptr){ return; }
  long Length=strlen(Pnt);
  _Allocate(Length);
  MemCpy(_Chr,Pnt,Length);
  _Length=Length;
}

//Constructor from char Buffer
Buffer::Buffer(const char *Pnt,long Length){
  _Chr=nullptr;
  _Length=0;
  _Size=0;
  _ChunkSize=CHUNK_SIZE;
  if(Pnt==nullptr){ return; }
  _Allocate(Length);
  MemCpy(_Chr,Pnt,Length);
  _Length=Length;
}

//Constructor from another Buffer
Buffer::Buffer(const Buffer& Buff){
  _Chr=nullptr;
  _Length=0;
  _Size=0;
  _ChunkSize=CHUNK_SIZE;
  if(Buff._Length==0){ return; }
  _Allocate(Buff._Length);
  MemCpy(_Chr,Buff._Chr,Buff._Length);
  _Length=Buff._Length;
}

//Constructor from char replication
Buffer::Buffer(long Times,char Chr){
  _Chr=nullptr;
  _Length=0;
  _Size=0;
  _ChunkSize=CHUNK_SIZE;
  if(Times<=0){ return; }
  _Allocate(Times);
  memset(_Chr,Chr,Times);
  _Length=Times;
}

//Rewind buffer (decrease length)
void Buffer::Rewind(long Bytes){
  if(Bytes>_Length){
    _Free();
    std::string Msg="Invalid buffer rewind, resulting size is negative";
    ThrowBaseException((int)ExceptionSource::Buffer,(int)BufferException::BufferRewind,Msg.c_str());
  }
  _Length-=Bytes;
}

//Reset buffer
void Buffer::Reset(){
  _Free();
}

//Buffer copy
void Buffer::Copy(const char *Pnt,long Length){
  if(Pnt==nullptr || Length<=0){ _Free(); return; }
  _Allocate(Length);
  MemCpy(_Chr,Pnt,Length);
  _Length=Length;
}

//Buffer copy
void Buffer::Append(const char *Pnt,long Length){
  if(Pnt==nullptr || Length<=0){ return; }
  _Allocate(_Length+Length);
  MemCpy(_Chr+_Length,Pnt,Length);
  _Length+=Length;
}

//Buffer format into hex
Buffer Buffer::ToHex(){
  if(_Length==0){ return Buffer(1,0); }
  long i;
  char Byte[3];
  Buffer Result;
  for(i=_Length-1;i>=0;i--){
    snprintf(Byte,3,"%02X",(unsigned char)_Chr[i]);
    Result+=Buffer((const char *)&Byte[0]);
  }
  Result+=Buffer(1,0);
  return Result;
}

//Buffer format into hex (without leading zeros)
Buffer Buffer::ToNzHex(){
  if(_Length==0){ return Buffer(1,0); }
  int i;
  int FirstSignificant;
  char Byte[3];
  Buffer Result;
  FirstSignificant=-1;
  for(i=_Length-1;i>=0;i--){
    if(_Chr[i]!=0){ FirstSignificant=i; break; }
  }
  if(FirstSignificant==-1){ FirstSignificant=0; }
  for(i=_Length-1;i>=0;i--){
    if(i<=FirstSignificant){
      snprintf(Byte,3,"%02X",(unsigned char)_Chr[i]);
      Result+=Buffer((const char *)&Byte[0]);
    }
  }
  Result+=Buffer(1,0);
  return Result;
}

//Search for character into buffer (when Occurence==0, means find last occurence)
long Buffer::Search(char c, long StartPos, int Occurence) const{
  
  //Variables
  long i;
  int TimesFound;
  long FoundIndex;

  //Search loop
  TimesFound=0;
  for(i=StartPos;i<_Length;i++){
    if(_Chr[i]==c){
      TimesFound++;
      FoundIndex=i;
      if(TimesFound==Occurence && Occurence!=0) break;
    }
  }

  //Return code
  if(TimesFound>0){
    if(Occurence==0){
      return FoundIndex;  
    }
    else if(TimesFound==Occurence){
      return FoundIndex;  
    }
    else{
      return -1;
    }
  }
  else{
    return -1;
  }

}

//Search for buffer into buffer (when Occurence==0, means find last occurence)
long Buffer::Search(const Buffer& Buff, long StartPos, int Occurence) const{
  
  //Variables
  bool Found;
  int TimesFound;
  long i,j;
  long FoundIndex;

  //Search loop
  TimesFound=0;
  for(i=StartPos;i<_Length;i++){
    for(j=0,Found=1;j<Buff._Length;j++){
      if(i+j<_Length){ 
        if(_Chr[i+j]!=Buff._Chr[j]){ 
          Found=0; 
          break; 
        }
      }
      else{
        Found=0; 
        break; 
      }
    }
    if(Found){
      TimesFound++;
      FoundIndex=i;
      if(TimesFound==Occurence && Occurence!=0) break;
    }
  }

  //Return code
  if(TimesFound>0){
    if(Occurence==0){
      return FoundIndex;  
    }
    else if(TimesFound==Occurence){
      return FoundIndex;  
    }
    else{
      return -1;
    }
  }
  else{
    return -1;
  }

}

//Get buffer part
Buffer Buffer::Part(long Position,long Length) const{
  
  //Variables
  Buffer Buff0;
  long FinalLength;
  
  //Check call is valid
  if(Position<0 || Length<0){
    std::string Msg="Invalid call to Part() function (Requested position=" + std::to_string(Position) + ", Requested length=" + std::to_string(Length) + ", Current length=" + std::to_string(_Length) + ")";
    ThrowBaseException((int)ExceptionSource::Buffer,(int)BufferException::InvalidPartCall,Msg.c_str());
  }

  //Return empty buffer if position is over length or length is zero
  if(Position>_Length || Length==0){
    Buff0=Buffer();
    return Buff0;
  }

  //Copy buffer part
  FinalLength=(Position+Length<=_Length?Length:_Length-Position);
  Buff0._Allocate(FinalLength);
  MemCpy(Buff0._Chr,&this->_Chr[Position],FinalLength);
  Buff0._Length=FinalLength;

  //Return ressult
  return Buff0;

}

//Asignation from char
Buffer& Buffer::operator=(const char Chr){
  _Allocate(1);
  _Chr[0]=Chr;
  _Length=1;
  return *this;
}

//Asignation from char String
Buffer& Buffer::operator=(const char *Pnt){
  if(Pnt==nullptr){ _Free(); return *this; }
  long Length=strlen(Pnt);
  _Allocate(Length);
  MemCpy(_Chr,Pnt,Length);
  _Length=Length;
  return *this;
}

//Assignation from another Buffer
Buffer& Buffer::operator=(const Buffer& Buff){
  if(Buff._Length==0){ _Free(); return *this; }
  _Allocate(Buff._Length);
  MemCpy(_Chr,Buff._Chr,Buff._Length);
  _Length=Buff._Length;
  return *this;
}

//Access operator
char& Buffer::operator[](int Index) const {
  if(Index<0 || Index>_Length-1){
    std::string Msg="Invalid buffer access for index " + std::to_string(Index) + " when buffer size is " + std::to_string(_Length);
    ThrowBaseException((int)ExceptionSource::Buffer,(int)BufferException::InvalidIndex,Msg.c_str());
  }
  return _Chr[Index];
}

//Append char*
Buffer& Buffer::operator+=(const char *Pnt){
  long Length=(Pnt==nullptr?0:strlen(Pnt));
  if(Length!=0){ 
    _Allocate(_Length+Length);
    MemCpy(_Chr+_Length,Pnt,Length); 
    _Length+=Length;
  }
  return *this;
}

//Append buffer
Buffer& Buffer::operator+=(const Buffer& Buff){
  if(Buff._Length!=0){ 
    _Allocate(_Length+Buff._Length);
    MemCpy(_Chr+_Length,Buff._Chr,Buff._Length); 
    _Length+=Buff._Length;
  }
  return *this;
}

//Append char
Buffer& Buffer::operator+=(const char c){
  _Allocate(_Length+1);
  _Chr[_Length+0]=c;
  _Length++;
  return *this;
}

//Concatenation of two buffers
Buffer operator+(const Buffer& Buff1,const Buffer& Buff2){
  Buffer Buff0;
  Buff0._Allocate(Buff1._Length+Buff2._Length+1);
  if(Buff1._Length!=0){ MemCpy(Buff0._Chr,Buff1._Chr,Buff1._Length); }
  if(Buff2._Length!=0){ MemCpy(Buff0._Chr+Buff1._Length,Buff2._Chr,Buff2._Length); }
  Buff0._Length=Buff1._Length+Buff2._Length;
  return Buff0;
}

//Allocate memory for buffer
void Buffer::_Allocate(long Size){
  
  //Variables
  char *Chr;
  bool ReAllocate=false;
  long MinLength;
  
  //Save current size
  #ifndef __NOALLOC__
  long PrevSize;
  PrevSize=_Size;
  #endif

  //Launch exception if size exceeds 2GB (32 signed integer maximun)
  if(Size>=2000000000L){
    std::string Msg="Buffer overflow (size exceeds 2GB)";
    ThrowBaseException((int)ExceptionSource::Buffer,(int)BufferException::BufferOverflow,Msg.c_str());
  }

  //Increase size
  if(Size>_Size){
    while(_Size<Size){ _Size+=_ChunkSize; }
    if(_Size/_ChunkSize>CHUNK_PARTS){ 
      while(_Size/_ChunkSize>CHUNK_PARTS){ _ChunkSize*=2; }
    }
    ReAllocate=true;
  }
  else if(Size<_Size-_ChunkSize){
    while(Size<_Size-_ChunkSize){ _Size-=_ChunkSize; }
    ReAllocate=true;
  }

  //Reallocation of elements
  if(ReAllocate){
    
    //Get memory
    #ifdef __NOALLOC__
    Chr=new char[(uint32_t)_Size];
    #else
    Chr=Allocator<char>::Take(MemObject::Buffer,_Size);
    #endif

    //Reallocate elements
    if((MinLength=std::min(_Length,Size))!=0){ MemCpy(Chr,_Chr,MinLength); }
    #ifdef __NOALLOC__
    delete[] _Chr;
    #else
    Allocator<char>::Give(MemObject::Buffer,PrevSize,_Chr);
    #endif
    _Chr=Chr;

  }

}

//Free buffer memory
void Buffer::_Free(){
  if(_Chr==nullptr){ _Length=0; _Size=0; return; }
  #ifdef __NOALLOC__
  delete[] _Chr;
  #else
  Allocator<char>::Give(MemObject::Buffer,_Size,_Chr);
  #endif
  _Chr=nullptr;
  _Length=0;
  _Size=0;
}
