
//basedefs.hpp

//Base definitions
#ifndef _BASEDEFS_HPP
#define _BASEDEFS_HPP

//Includes
#include <string>
#include <string.h>
#include <regex>
#include <sstream>
#include <fstream>
#include <iostream>
#include <ios>
#include <stdexcept>
#include <cxxabi.h>
#include <typeinfo>
#include <cfenv>
#include <cmath>
#include <random>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <sys/stat.h>

//Specific includes for windows/linux
#ifdef __WIN__
  #include <windows.h>
  #include <memoryapi.h>
  //#include <atlstr.h>
#else
  #include <sys/mman.h>
  #include <sys/wait.h>
  #include <unistd.h>
  #include <dlfcn.h>
  #include <poll.h>
#endif

//#pragma FENV_ACCESS ON

//Line ending definitions depending on host system
//LINE_SEPAR Character used to delimit lines
//LINE_EXTRA Extra caracters belonging to line ending apart from line separator (zero means it is missing)
//LINE_END   Full specification of line end
#ifdef __WIN__
  #define LINE_SEPAR '\n'
  #define LINE_EXTRA '\r'
  #define LINE_END   "\r\n"
#else
  #define LINE_SEPAR '\n'
  #define LINE_EXTRA 0
  #define LINE_END   "\n"
#endif

//Exception sources
enum class ExceptionSource:int{
  Allocator=0,
  Buffer=1,
  NlBuffer=2,
  String=3,
  Array=4,
  SortedArray=5,
  Stack=6,
  Queue=7,
  RamBuffer=8,
  MemoryPool=9,
  AuxMemory=10
};

//Memcpy / Memmove
#define MemCpy(dst,src,size)  memcpy((dst),(src),(size))
#define MemMove(dst,src,size) memmove((dst),(src),(size))

//Thow exception macro  
#define ThrowBaseException(source,code,arg) { const std::string _BaseExceptionMessage=arg; throw BaseException(_BaseExceptionMessage, source, code, __FILE__, __LINE__); }

//Demangle data type name to the one entered in source (useful for messages)
//Object can be anything to be demangled, output mus be string type
#define CplusGetTypeDesc(object,output_string) { char *_DemangledTypeDesc=abi::__cxa_demangle(typeid(object).name(), 0, 0, 0); \
                                                 output_string=_DemangledTypeDesc; \
                                                 if(_DemangledTypeDesc!=nullptr) free(_DemangledTypeDesc); \
                                               }
//Macro to supress unused variable warnings
#define maybeused __attribute__ ((unused))

//Class
class BaseException:public std::runtime_error{
    
  //Private members
  private:
    
    //Exception description
    std::string Desc;
  
  //Public memers
  public:
  
  //Constructor
  BaseException(const std::string& arg,int Source,int Code, const char *File,int Line):runtime_error(arg){
    std::string CodeStr=std::to_string(Source*100+Code);
    #ifdef __DEV__
    std::string FileName=std::string(File).substr(std::string(File).find_last_of("/\\") + 1);
    Desc = std::string("X") + CodeStr + std::string("[") + FileName + std::string(":") + std::to_string(Line) + std::string("] Exception: ") + arg;
    #else
    Desc = std::string("X") + CodeStr + std::string(" Exception: ") + arg;
    #endif
  }
  
  //Destructor
  ~BaseException() throw() {}

  //Get error description
  const char *Description() const throw(){
    return Desc.c_str( );
  }

};

#endif