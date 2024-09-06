//Buffer.hpp: Header file for buffer class
//Main differences with String class is that Buffers do not have big set of manipulation fuctions and do not end in zero, thus allowing zeroes in between
#ifndef _BUFFER_HPP
#define _BUFFER_HPP
 
//Exception numbers
enum class BufferException{
  OutOfMemory=1,
  InvalidIndex=2,
  InvalidPartCall=3,
  BufferOverflow=4,
  BufferRewind=5
};

//Buffer class
class Buffer {
  
  //Private members
  private:

    //Constants
    const int CHUNK_SIZE = 128;
    const int CHUNK_PARTS = 8;

    //Private members
    char *_Chr;
    long _Length;
    long _Size;
    long _ChunkSize;
    
    //Private methods
    void _Allocate(long Size);
    void _Free();

  //Public members
  public:

    //Constructors
    Buffer();
    Buffer(char Chr);
    Buffer(const char *Pnt);
    Buffer(const char *Pnt,long Length);
    Buffer(const Buffer& Buff);
    Buffer(long Times,char Chr);
    
    //Destructor
    inline ~Buffer(){ _Free(); }
    
    //Inline functions
    inline long Length() const{ return _Length;  }
    inline char *BuffPnt() const{ return (char *)&_Chr[0]; }

    //Buffer manipulations
    void Rewind(long Bytes);
    void Reset();
    void Copy(const char *Pnt,long Length);
    void Append(const char *Pnt,long Length);
    Buffer ToHex();
    Buffer ToNzHex();
    long Search(char c,long StartPos=0,int Occurence=1) const;
    long Search(const Buffer& Buff,long StartPos=0,int Occurence=1) const;
    Buffer Part(long Position,long Length) const;

    //Operators
    Buffer& operator=(const char Chr);
    Buffer& operator=(const char *Pnt);
    Buffer& operator=(const Buffer& Buff);
    char& operator[](int Index) const;
    Buffer& operator+=(const char *Pnt);
    Buffer& operator+=(const Buffer& Buff);
    Buffer& operator+=(const char c);
    friend Buffer operator+(const Buffer& Buff1,const Buffer& Buff2);

};

#endif