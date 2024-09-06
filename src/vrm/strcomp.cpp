//strcomp.cpp: String computer class
#include "bas/basedefs.hpp"
#include "bas/allocator.hpp"
#include "bas/array.hpp"
#include "bas/stack.hpp"
#include "bas/buffer.hpp"
#include "bas/string.hpp"
#include "sys/sysdefs.hpp"
#include "sys/system.hpp"
#include "sys/stl.hpp"
#include "sys/msgout.hpp"
#include "vrm/pool.hpp"
#include "vrm/memman.hpp"
#include "vrm/auxmem.hpp"
#include "vrm/strcomp.hpp"

//String computer initialization
void StringComputer::Init(AuxMemoryManager *Aux){
  _Aux=Aux;
}

//String operation SLES
bool StringComputer::SLES(CpuBol *Res,CpuMbl Str1,CpuMbl Str2){ 
  if(!IsValid(Str1,true) || !IsValid(Str2,true)){ return false; }
  if(strcmp(_Aux->CharPtr(Str1),_Aux->CharPtr(Str2))< 0){ *Res=true; } 
  else{ *Res=false; } 
  return true; 
}

//String operation SLEQ
bool StringComputer::SLEQ(CpuBol *Res,CpuMbl Str1,CpuMbl Str2){ 
  if(!IsValid(Str1,true) || !IsValid(Str2,true)){ return false; }
  if(strcmp(_Aux->CharPtr(Str1),_Aux->CharPtr(Str2))<=0){ *Res=true; } 
  else{ *Res=false; } 
  return true; 
}

//String operation SGRE
bool StringComputer::SGRE(CpuBol *Res,CpuMbl Str1,CpuMbl Str2){ 
  if(!IsValid(Str1,true) || !IsValid(Str2,true)){ return false; }
  if(strcmp(_Aux->CharPtr(Str1),_Aux->CharPtr(Str2))> 0){ *Res=true; } 
  else{ *Res=false; } 
  return true; 
}

//String operation SGEQ
bool StringComputer::SGEQ(CpuBol *Res,CpuMbl Str1,CpuMbl Str2){ 
  if(!IsValid(Str1,true) || !IsValid(Str2,true)){ return false; }
  if(strcmp(_Aux->CharPtr(Str1),_Aux->CharPtr(Str2))>=0){ *Res=true; } 
  else{ *Res=false; } 
  return true; 
}

//String operation SEQU
bool StringComputer::SEQU(CpuBol *Res,CpuMbl Str1,CpuMbl Str2){ 
  if(!IsValid(Str1,true) || !IsValid(Str2,true)){ return false; }
  if(strcmp(_Aux->CharPtr(Str1),_Aux->CharPtr(Str2))==0){ *Res=true; } 
  else{ *Res=false; } 
  return true; 
}

//String operation SDIS
bool StringComputer::SDIS(CpuBol *Res,CpuMbl Str1,CpuMbl Str2){ 
  if(!IsValid(Str1,true) || !IsValid(Str2,true)){ return false; }
  if(strcmp(_Aux->CharPtr(Str1),_Aux->CharPtr(Str2))!=0){ *Res=true; } 
  else{ *Res=false; } 
  return true; 
}

//String operation SEMP
bool StringComputer::SEMP(CpuMbl *Des){
  
  //Variables
  CpuWrd Length=0;
  
  //Prepare destination string
  if(IsAllocated(*Des)){ if(!_Allocate(*Des,Length)){ return false; } }
  else{ if(!_NewString(Des,Length)){ return false; } }

  //Set empty string
  _Aux->CharPtr(*Des)[Length]=0;
  _Aux->SetLen(*Des,Length);

  //Get result
  return true;

}

//String operation SCOPY
bool StringComputer::SCOPY(CpuMbl *Des,char *Src){
  
  //Variables
  CpuWrd Length;

  //Prepare destination string
  Length=strlen(Src);
  if(IsAllocated(*Des)){ if(!_Allocate(*Des,Length)){ return false; } }
  else{ if(!_NewString(Des,Length)){ return false; } }

  //Copy string
  if(Length!=0){ MemCpy(_Aux->CharPtr(*Des),Src,Length); }
  _Aux->CharPtr(*Des)[Length]=0;
  _Aux->SetLen(*Des,Length);

  //Get result
  return true;
}

//String operation SCOPY
bool StringComputer::SCOPY(CpuMbl *Des,char *Src,CpuWrd Length){
  
  //Prepare destination string
  if(IsAllocated(*Des)){ if(!_Allocate(*Des,Length)){ return false; } }
  else{ if(!_NewString(Des,Length)){ return false; } }

  //Copy string
  if(Length!=0){ MemCpy(_Aux->CharPtr(*Des),Src,Length); }
  _Aux->CharPtr(*Des)[Length]=0;
  _Aux->SetLen(*Des,Length);

  //Get result
  return true;

}

//String operation SCOPY
bool StringComputer::SCOPY(CpuMbl *Des,CpuMbl Src){
  
  //Variables
  CpuWrd Length;
  CpuMbl Prv;

  //If source string is null, copy a null string
  if(Src==0){ *Des=0; return true; }

  //Check inputs
  if(!IsValid(Src,true)){ return false; }
  
  //Get source length
  Length=_Aux->GetLen(Src);

  //Prepare destination string
  Prv=-1;
  if(IsAllocated(*Des)){
    if(*Des!=Src){ if(!_Allocate(*Des,Length)){ return false; } }
    else{ Prv=*Des; if(!_NewString(Des,Length)){ return false; } }
  }
  else{ 
    if(!_NewString(Des,Length)){ return false; } 
  }

  //Copy string
  if(Length!=0){ MemCpy(_Aux->CharPtr(*Des),_Aux->CharPtr(Src),Length); }
  _Aux->CharPtr(*Des)[Length]=0;
  _Aux->SetLen(*Des,Length);

  //Free resources
  if(Prv!=-1){ _Aux->Free(Prv); }

  //Get result
  return true;
}

//String operation SSWCP
bool StringComputer::SSWCP(CpuMbl *Des,CpuMbl Src){
  
  //If source string is null, copy a null string
  if(Src==0){ *Des=0; return true; }

  //Check inputs
  if(!IsValid(Src,true)){ return false; }
  
  //Prepare destination string
  if(IsAllocated(*Des)){
    if(*Des!=Src){
      _Aux->Clear(*Des);
    }
  }
  else{
    if(!_EmptyAlloc(Des)){ return false; } 
  }

  //Set pointer and length from source string
  _Aux->SetPtr(*Des,_Aux->CharPtr(Src));
  _Aux->SetLen(*Des,_Aux->GetLen(Src));
  _Aux->SetSize(*Des,_Aux->GetSize(Src));

  //Free resources
  if(*Des!=Src){
    _Aux->SetPtr(Src,nullptr);
    _Aux->Free(Src);
  }

  //Get result
  return true;
}

//String operation SLEN
bool StringComputer::SLEN(CpuWrd *Res,CpuMbl Str){
  if(!IsValid(Str,true)){ return false; }
  *Res=(CpuWrd)_Aux->GetLen(Str);
  return true;
}

//String operation SMID
bool StringComputer::SMID(CpuMbl *Res,CpuMbl Str,CpuWrd Pos,CpuWrd Len){
  
  //Variables
  CpuWrd Length;
  CpuMbl Prv;

  //Check invalid call
  if(!IsValid(Str,true)){ 
    return false; 
  }
  if(Pos<0 || Len<0){ 
    System::Throw(SysExceptionCode::InvalidCallStringMid); 
    return false; 
  }

  //Return empty string if position is over length
  if(Pos>_Aux->GetLen(Str) || Len==0){
    Length=0;
  }
  else{
    Length=(Pos+Len<=_Aux->GetLen(Str)?Len:_Aux->GetLen(Str)-Pos);
  }

  //Prepare destination string
  Prv=-1;
  if(IsAllocated(*Res)){
    if(*Res!=Str){ if(!_Allocate(*Res,Length)){ return false; } }
    else{ Prv=*Res; if(!_NewString(Res,Length)){ return false; } }
  }
  else{ 
    if(!_NewString(Res,Length)){ return false; } 
  }

  //Copy substring String
  if(Length!=0){ MemCpy(_Aux->CharPtr(*Res),&_Aux->CharPtr(Str)[Pos],Length); }
  _Aux->CharPtr(*Res)[Length]=0;
  _Aux->SetLen(*Res,Length);

  //Free resources
  if(Prv!=-1){ _Aux->Free(Prv); }

  //Get result
  return true;

}

//String operation SINDX
bool StringComputer::SINDX(CpuRef *Res,CpuMbl Str,CpuWrd Idx){

  //Check invalid call
  if(!IsValid(Str,true)){ 
    return false; 
  }
  if(Idx<0 || Idx>_Aux->GetLen(Str)-1){ 
    System::Throw(SysExceptionCode::InvalidStringIndex,ToString(Idx),ToString(_Aux->GetLen(Str))); 
    return false; 
  }

  //Return reference to char
  (*Res)=(CpuRef){(CpuMbl)(BLOCKMASK80|(Str)),Idx};

  //Get result
  return true;

}
  
//String operation SRGHT
bool StringComputer::SRGHT(CpuMbl *Res,CpuMbl Str,CpuWrd Len){
  if(!IsValid(Str,true)){ 
    return false; 
  }
  if(Len<0){ 
    System::Throw(SysExceptionCode::InvalidCallStringRight); 
    return false; 
  }
  if(Len<=_Aux->GetLen(Str)){
    return SMID(Res,Str,_Aux->GetLen(Str)-Len,Len);
  }
  else{
    return SCOPY(Res,Str);
  }
  return true;
}

//String operation SLEFT
bool StringComputer::SLEFT(CpuMbl *Res,CpuMbl Str,CpuWrd Len){
  if(!IsValid(Str,true)){ 
    return false; 
  }
  if(Len<0){ 
    System::Throw(SysExceptionCode::InvalidCallStringLeft); 
    return false; 
  }
  if(Len<=_Aux->GetLen(Str)){
    return SMID(Res,Str,0,Len);
  }
  else{
    return SCOPY(Res,Str);
  }
  return true;
}

//String operation SCUTR
bool StringComputer::SCUTR(CpuMbl *Res,CpuMbl Str,CpuWrd Len){
  if(!IsValid(Str,true)){ 
    return false; 
  }
  if(Len<0){ 
    System::Throw(SysExceptionCode::InvalidCallStringCutRight); 
    return false; 
  }
  if(Len<_Aux->GetLen(Str)){
    return SMID(Res,Str,0,_Aux->GetLen(Str)-Len);
  }
  else{
    return SMID(Res,Str,0,0);
  }
  return true;
}

//String operation SCUTL
bool StringComputer::SCUTL(CpuMbl *Res,CpuMbl Str,CpuWrd Len){
  if(!IsValid(Str,true)){ 
    return false; 
  }
  if(Len<0){ 
    System::Throw(SysExceptionCode::InvalidCallStringCutLeft); 
    return false; 
  }
  if(Len<_Aux->GetLen(Str)){
    return SMID(Res,Str,Len,_Aux->GetLen(Str)-Len);
  }
  else{
    return SMID(Res,Str,0,0);
  }
  return true;
}

//String operation SAPPN (append to string)
bool StringComputer::SAPPN(CpuMbl *Des,char *Src){
  
  //Variables
  CpuWrd Len1;
  CpuWrd Len2;
  CpuWrd Len;
  char *PtrDes;

  //Check inputs
  if(!IsValid(*Des,true)){ 
    return false; 
  }

  //Get string lengths
  Len1=_Aux->GetLen(*Des);
  Len2=strlen(Src);
  Len=Len1+Len2;

  //Prepare destination string
  if(IsAllocated(*Des)){
    if(!_Allocate(*Des,Len)){ return false; }
  }
  else{ 
    if(!_NewString(Des,Len)){ return false; } 
  }

  //Append second string
  PtrDes=_Aux->CharPtr(*Des);
  MemCpy(PtrDes+Len1,Src,Len2);
  PtrDes[Len]=0;
  _Aux->SetLen(*Des,Len);

  //Return code
  return true;

}

//String operation SAPPN (append to string)
bool StringComputer::SAPPN(CpuMbl *Des,char *Src,CpuWrd Length){
  
  //Variables
  CpuWrd Len1;
  CpuWrd Len2;
  CpuWrd Len;
  char *PtrDes;

  //Check inputs
  if(!IsValid(*Des,true)){ 
    return false; 
  }

  //Get string lengths
  Len1=_Aux->GetLen(*Des);
  Len2=Length;
  Len=Len1+Len2;

  //Prepare destination string
  if(IsAllocated(*Des)){
    if(!_Allocate(*Des,Len)){ return false; }
  }
  else{ 
    if(!_NewString(Des,Len)){ return false; } 
  }

  //Append second string
  PtrDes=_Aux->CharPtr(*Des);
  MemCpy(PtrDes+Len1,Src,Len2);
  PtrDes[Len]=0;
  _Aux->SetLen(*Des,Len);

  //Return code
  return true;

}

//String operation SAPPN (append to string)
bool StringComputer::SAPPN(CpuMbl *Des,CpuMbl Str){
  
  //Variables
  CpuWrd Len1;
  CpuWrd Len2;
  CpuWrd Len;
  char *PtrStr;
  char *PtrDes;

  //Check inputs
  if(!IsValid(*Des,true) || !IsValid(Str,true)){ 
    return false; 
  }

  //Get string lengths
  Len1=_Aux->GetLen(*Des);
  Len2=_Aux->GetLen(Str);
  Len=Len1+Len2;

  //Prepare destination string
  if(IsAllocated(*Des)){
    if(!_Allocate(*Des,Len)){ return false; }
  }
  else{ 
    if(!_NewString(Des,Len)){ return false; } 
  }

  //Append second string
  PtrStr=_Aux->CharPtr(Str);
  PtrDes=_Aux->CharPtr(*Des);
  MemCpy(PtrDes+Len1,PtrStr,Len2);
  PtrDes[Len]=0;
  _Aux->SetLen(*Des,Len);

  //Return code
  return true;

}

//String operation SCONC
bool StringComputer::SCONC(CpuMbl *Res,CpuMbl Str1,CpuMbl Str2){
  
  //Variables
  CpuWrd Len1;
  CpuWrd Len2;
  CpuWrd Length;
  CpuMbl Prv;
  char *PtrRes;
  char *PtrStr1;
  char *PtrStr2;

  //Check inputs
  if(!IsValid(Str1,true) || !IsValid(Str2,true)){ 
    return false; 
  }

  //Get string lengths
  Len1=_Aux->GetLen(Str1);
  Len2=_Aux->GetLen(Str2);
  Length=Len1+Len2;

  //Prepare destination string
  Prv=-1;
  if(IsAllocated(*Res)){
    if(*Res!=Str1 && *Res!=Str2){ if(!_Allocate(*Res,Length)){ return false; } }
    else{ Prv=*Res; if(!_NewString(Res,Length)){ return false; } }
  }
  else{ 
    if(!_NewString(Res,Length)){ return false; } 
  }

  //Concatenate strings
  PtrStr1=_Aux->CharPtr(Str1);
  PtrStr2=_Aux->CharPtr(Str2);
  PtrRes=_Aux->CharPtr(*Res);
  MemCpy(PtrRes,PtrStr1,Len1);
  MemCpy(PtrRes+Len1,PtrStr2,Len2);
  PtrRes[Length]=0;
  _Aux->SetLen(*Res,Length);

  //Free resources
  if(Prv!=-1){ _Aux->Free(Prv); }

  //Return code
  return true;

}

//String operation SMVCO
bool StringComputer::SMVCO(CpuMbl *Des,CpuMbl Str){
  
  //Variables
  CpuWrd Len1;
  CpuWrd Len2;
  CpuWrd Length;
  CpuMbl Prv;
  char *PtrStr;
  char *PtrDes;

  //Check inputs
  if(!IsValid(*Des,true) || !IsValid(Str,true)){ 
    return false; 
  }

  //Get string lengths
  Len1=_Aux->GetLen(*Des);
  Len2=_Aux->GetLen(Str);
  Length=Len1+Len2;

  //Prepare destination string
  Prv=-1;
  if(IsAllocated(*Des)){
    if(*Des!=Str){ if(!_Allocate(*Des,Length)){ return false; } }
    else{ Prv=*Des; if(!_NewString(Des,Length)){ return false; } }
  }
  else{ 
    if(!_NewString(Des,Length)){ return false; } 
  }

  //Append second string
  PtrStr=_Aux->CharPtr(Str);
  PtrDes=_Aux->CharPtr(*Des);
  MemCpy(PtrDes+Len1,PtrStr,Len2);
  PtrDes[Length]=0;
  _Aux->SetLen(*Des,Length);

  //Free resources
  if(Prv!=-1){ _Aux->Free(Prv); }

  //Return code
  return true;

}

//String operation SMVRC
bool StringComputer::SMVRC(CpuMbl *Des,CpuMbl Str){
  
  //Variables
  CpuWrd Len1;
  CpuWrd Len2;
  CpuWrd Length;
  CpuMbl Prv;
  char *PtrStr;
  char *PtrDes;

  //Check inputs
  if(!IsValid(*Des,true) || !IsValid(Str,true)){ 
    return false; 
  }

  //Get string lengths
  Len1=_Aux->GetLen(*Des);
  Len2=_Aux->GetLen(Str);
  Length=Len1+Len2;

  //Prepare destination string
  Prv=-1;
  if(IsAllocated(*Des)){
    if(*Des!=Str){ if(!_Allocate(*Des,Length)){ return false; } }
    else{ Prv=*Des; if(!_NewString(Des,Length)){ return false; } }
  }
  else{ 
    if(!_NewString(Des,Length)){ return false; } 
  }

  //Reverse concatenation
  PtrDes=_Aux->CharPtr(*Des);
  PtrStr=_Aux->CharPtr(Str);
  if(Len1>Len2){
    MemMove(PtrDes+Len2,PtrDes,Len1);
  }
  else{
    MemCpy(PtrDes+Len2,PtrDes,Len1);
  }
  MemCpy(PtrDes,PtrStr,Len2);
  PtrDes[Length]=0;
  _Aux->SetLen(*Des,Length);

  //Free resources
  if(Prv!=-1){ _Aux->Free(Prv); }

  //Return code
  return true;

}

//String operation SFIND
bool StringComputer::SFIND(CpuWrd *Res,CpuMbl Str,CpuMbl Sub,CpuWrd Beg){

  //Variables
  bool Found;
  CpuWrd i,j;
  CpuWrd FoundIndex;
  CpuWrd StrLen;
  CpuWrd SubLen;
  char *PtrStr;
  char *PtrSub;

  //Check inputs
  if(!IsValid(Str,true) || !IsValid(Sub,true)){ 
    return false; 
  }
  if(Beg<0){ 
    System::Throw(SysExceptionCode::InvalidCallStringFind); 
    return false; 
  }

  //Get pointers and lengths
  PtrStr=_Aux->CharPtr(Str);
  PtrSub=_Aux->CharPtr(Sub);
  StrLen=_Aux->GetLen(Str);
  SubLen=_Aux->GetLen(Sub);

  //If start position is over length return not found
  if(Beg>=StrLen){
    *Res=-1;
    return true;
  }

  //Search loop
  FoundIndex=-1;
  for(i=Beg;i<StrLen;i++){
    for(j=0,Found=true;j<SubLen;j++){
      if(i+j<StrLen){ 
        if(PtrStr[i+j]!=PtrSub[j]){ 
          Found=false; 
          break; 
        }
      }
      else{
        Found=false; 
        break; 
      }
    }
    if(Found){
      FoundIndex=i;
      break;
    }
  }

  //Return result
  *Res=FoundIndex;
  return true;

}

//String operation SSUBS
bool StringComputer::SSUBS(CpuMbl *Res,CpuMbl Str,CpuMbl Old,CpuMbl New){

  //Variables
  CpuWrd Position;
  CpuWrd SearchPos;
  CpuWrd Length;
  CpuWrd LenNew;
  CpuWrd LenOld;
  char *PtrRes;
  char *PtrNew;

  //Check inputs
  if(!IsValid(Str,true) || !IsValid(Old,true) || !IsValid(New,true)){ 
    return false; 
  }

  //Init return string
  if(!SCOPY(Res,Str)){ return false; }
  
  //Exit if Old string is empty
  if(_Aux->GetLen(Old)==0){ return true; }

  //Get pointers and lengths
  LenNew=_Aux->GetLen(New);
  LenOld=_Aux->GetLen(Old);
  PtrRes=_Aux->CharPtr(*Res);
  PtrNew=_Aux->CharPtr(New);
  
  //Replacement loop
  SearchPos=0;
  do{
    if(!SFIND(&Position,*Res,Old,SearchPos)){ return false; }
    if(Position==-1){ break; }
    Length=_Aux->GetLen(*Res)-LenOld+LenNew;
    if(Length>_Aux->GetLen(*Res)){ if(!_Allocate(*Res,Length)){ return false; } }
    PtrRes=_Aux->CharPtr(*Res);
    MemMove(PtrRes+Position+LenNew,PtrRes+Position+LenOld,Length-Position-LenNew+1);
    MemCpy(PtrRes+Position,PtrNew,LenNew);
    PtrRes[Length]=0;
    _Aux->SetLen(*Res,Length);
    SearchPos=Position+LenNew;
  }while(true);
  
  //Return code
  return true;

}

//String operation STRIM
bool StringComputer::STRIM(CpuMbl *Res,CpuMbl Str){

  //Variables
  CpuWrd i;
  CpuWrd Length;
  CpuWrd BegPos;
  CpuWrd EndPos;
  char *PtrStr;

  //Check inputs
  if(!IsValid(Str,true)){ 
    return false; 
  }

  //Get lengthS
  Length=_Aux->GetLen(Str);

  //Special case for empty string
  if(Length==0){ return SCOPY(Res,Str); }

  //Find first and last non space position
  PtrStr=_Aux->CharPtr(Str);
  for(i=0,BegPos=i;i<Length;i++){ if(PtrStr[i]!=' '){ BegPos=i; break; } }
  for(i=Length-1,EndPos=i;i>=0;i--){ if(PtrStr[i]!=' '){ EndPos=i; break; } }

  //Return result
  return SMID(Res,Str,BegPos,EndPos-BegPos+1);

}

//String operation SUPPR
bool StringComputer::SUPPR(CpuMbl *Res,CpuMbl Str){

  //Variables
  CpuWrd i;
  CpuWrd Length;
  CpuMbl Prv;
  char *PtrRes;
  char *PtrStr;

  //Check inputs
  if(!IsValid(Str,true)){ return false; }

  //Get string length
  Length=_Aux->GetLen(Str);
 
  //Prepare destination string
  Prv=-1;
  if(IsAllocated(*Res)){
    if(*Res!=Str){ if(!_Allocate(*Res,Length)){ return false; } }
    else{ Prv=*Res; if(!_NewString(Res,Length)){ return false; } }
  }
  else{ 
    if(!_NewString(Res,Length)){ return false; } 
  }

  //Copy string and upperize characters
  PtrRes=_Aux->CharPtr(*Res);
  PtrStr=_Aux->CharPtr(Str);
  for(i=0;i<Length;i++){ PtrRes[i]=toupper(PtrStr[i]); }
  PtrRes[Length]=0;
  _Aux->SetLen(*Res,Length);

  //Free resources
  if(Prv!=-1){ _Aux->Free(Prv); }
  
  //Return code
  return true;

}

//String operation SLOWR
bool StringComputer::SLOWR(CpuMbl *Res,CpuMbl Str){

  //Variables
  CpuWrd i;
  CpuWrd Length;
  CpuMbl Prv;
  char *PtrRes;
  char *PtrStr;

  //Check inputs
  if(!IsValid(Str,true)){ return false; }

  //Get string length
  Length=_Aux->GetLen(Str);
 
 
  //Prepare destination string
  Prv=-1;
  if(IsAllocated(*Res)){
    if(*Res!=Str){ if(!_Allocate(*Res,Length)){ return false; } }
    else{ Prv=*Res; if(!_NewString(Res,Length)){ return false; } }
  }
  else{ 
    if(!_NewString(Res,Length)){ return false; } 
  }

  //Copy string and lowerize characters
  PtrRes=_Aux->CharPtr(*Res);
  PtrStr=_Aux->CharPtr(Str);
  for(i=0;i<Length;i++){ PtrRes[i]=tolower(PtrStr[i]); }
  PtrRes[Length]=0;
  _Aux->SetLen(*Res,Length);

  //Free resources
  if(Prv!=-1){ _Aux->Free(Prv); }

  //Return code
  return true;

}

//String operation SLJUS
bool StringComputer::SLJUS(CpuMbl *Res,CpuMbl Str,CpuWrd Len,CpuChr Fill){

  //Variables
  CpuWrd Length;
  CpuMbl Prv;
  char *PtrRes;
  char *PtrStr;

  //Check inputs
  if(!IsValid(Str,true)){ return false; }
  if(Len<0){ 
    System::Throw(SysExceptionCode::InvalidCallStringLJust); 
    return false; 
  }

  //Get string length
  Length=_Aux->GetLen(Str);

  //Do not pad if String has greater length
  if(Length>=Len){ return SCOPY(Res,Str); }

  //Prepare destination string
  Prv=-1;
  if(IsAllocated(*Res)){
    if(*Res!=Str){ if(!_Allocate(*Res,Length)){ return false; } }
    else{ Prv=*Res; if(!_NewString(Res,Length)){ return false; } }
  }
  else{ 
    if(!_NewString(Res,Length)){ return false; } 
  }

  //Copy characters
  PtrRes=_Aux->CharPtr(*Res);
  PtrStr=_Aux->CharPtr(Str);
  memset(PtrRes,Fill,Len);
  MemCpy(PtrRes,PtrStr,Length);
  PtrRes[Len]=0;
  _Aux->SetLen(*Res,Len);

  //Free resources
  if(Prv!=-1){ _Aux->Free(Prv); }
  
  //Return code
  return true;

}

//String operation SRJUS
bool StringComputer::SRJUS(CpuMbl *Res,CpuMbl Str,CpuWrd Len,CpuChr Fill){

  //Variables
  CpuWrd Length;
  CpuMbl Prv;
  char *PtrRes;
  char *PtrStr;

  //Check inputs
  if(!IsValid(Str,true)){ return false; }
  if(Len<0){ 
    System::Throw(SysExceptionCode::InvalidCallStringRJust); 
    return false; 
  }

  //Get string length
  Length=_Aux->GetLen(Str);

  //Do not pad if String has greater length
  if(Length>=Len){ return SCOPY(Res,Str); }

  //Prepare destination string
  Prv=-1;
  if(IsAllocated(*Res)){
    if(*Res!=Str){ if(!_Allocate(*Res,Length)){ return false; } }
    else{ Prv=*Res; if(!_NewString(Res,Length)){ return false; } }
  }
  else{ 
    if(!_NewString(Res,Length)){ return false; } 
  }

  //Copy characters
  PtrRes=_Aux->CharPtr(*Res);
  PtrStr=_Aux->CharPtr(Str);
  memset(PtrRes,Fill,Len);
  MemCpy(PtrRes+(Len-Length),PtrStr,Length);
  PtrRes[Len]=0;
  _Aux->SetLen(*Res,Len);

  //Free resources
  if(Prv!=-1){ _Aux->Free(Prv); }

  //Return code
  return true;

}

//String regex match
bool StringComputer::SMATC(CpuBol *Res,CpuMbl Str,CpuMbl Regex){
  if(!IsValid(Str,true) || !IsValid(Regex,true)){ return false; }
  if(_Aux->GetLen(Regex)==0){
    if(_Aux->GetLen(Str)==0){ *Res=true; } else{ *Res=false; }
    return true; 
  }
  const std::regex StdRegex(_Aux->CharPtr(Regex));
  *Res=std::regex_match(_Aux->CharPtr(Str),StdRegex);
  return true; 
}

//String pattern like (acepting * and ?)
bool StringComputer::SLIKE(CpuBol *Res,CpuMbl Str,CpuMbl Pattern){
  
  //Variables
  bool Found;
  char *StrPtr;
  char *PatPtr;
  CpuWrd StrLen;
  CpuWrd PatLen;
  CpuWrd i,j,k;
  CpuWrd CheckLen;
  CpuWrd Index;
  CpuWrd FoundIndex;

  //Check validity of strings
  if(!IsValid(Str,true) || !IsValid(Pattern,true)){ return false; }
  
  //Special case for empty pattern
  if(_Aux->GetLen(Pattern)==0){
    *Res=(_Aux->GetLen(Str)==0?true:false);
    return true; 
  }

  //Get string pointersand lengths
  StrPtr=_Aux->CharPtr(Str);
  StrLen=_Aux->GetLen(Str);
  PatPtr=_Aux->CharPtr(Pattern);
  PatLen=_Aux->GetLen(Pattern);

  //Pattern match loop
  i=0;
  j=0;
  while(true){

    //Switch on attern char
    switch(PatPtr[j]){
      
      //Any character
      case '?':
        i++;
        j++;
        break;

      //Substring
      case '*':
        
        //Eat any consecutive asterisks
        while(PatPtr[j]=='*'){ j++; }

        //Return true if pattern ends in asterisk as string would match anyway
        if(j==PatLen){ *Res=true; return true; }
        
        //Determite substring to find in string (until next asterisk or pattern end)
        for(CheckLen=0;PatPtr[j+CheckLen]!=0 && PatPtr[j+CheckLen]!='*';CheckLen++);

        //Find the substring in string from current position
        FoundIndex=-1;
        for(Index=0;StrPtr[i+Index]!=0;Index++){
          Found=true;
          for(k=0;StrPtr[i+Index+k]!=0 && k<CheckLen;k++){ 
            if(StrPtr[i+Index+k]!=PatPtr[j+k] && PatPtr[j+k]!='?'){ Found=false; break; }
          }
          if(Found){
            FoundIndex=Index;
            break;
          }
        }
        if(FoundIndex!=-1){
          if(FoundIndex==0){ *Res=false; return true; }
          i+=FoundIndex+CheckLen;
          j+=CheckLen;
        }

        //Substring was not found
        else{
          *Res=false; 
          return true; 
        }
        break;

      //Other characters
      default:
        if(StrPtr[i]!=PatPtr[j]){
          *Res=false; 
          return true;
        }
        i++;
        j++;
        break;
    }

    //Exit if string and pattern are traversed till end
    if(i==StrLen && j==PatLen){
      *Res=true; 
      return true;
    }

    //Return false if string or pattern reached end (but not the other as this is checked before)
    if(i==StrLen || j==PatLen){
      *Res=false; 
      return true;
    }

  }

  //Return code
  return true; 

}

//String operation SREPL
bool StringComputer::SREPL(CpuMbl *Res,CpuMbl Str,CpuWrd Num){
  
  //Variables
  CpuWrd i,k;
  CpuWrd Length;
  CpuWrd TotLen;
  CpuMbl Prv;
  char *PtrRes;
  char *PtrStr;

  //Check inputs
  if(!IsValid(Str,true)){ 
    return false; 
  }
  if(Num<0){ 
    System::Throw(SysExceptionCode::InvalidCallStringReplicate); 
    return false; 
  }

  //Get string lengths
  Length=_Aux->GetLen(Str);
  TotLen=Num*Length;

  //Prepare destination string
  Prv=-1;
  if(IsAllocated(*Res)){
    if(*Res!=Str){ if(!_Allocate(*Res,TotLen)){ return false; } }
    else{ Prv=*Res; if(!_NewString(Res,TotLen)){ return false; } }
  }
  else{ 
    if(!_NewString(Res,TotLen)){ return false; } 
  }

  //Replicate string
  k=0;
  PtrRes=_Aux->CharPtr(*Res);
  PtrStr=_Aux->CharPtr(Str);
  for(i=0;i<Num;i++){
    MemCpy(PtrRes+k,PtrStr,Length);
    k+=Length;
  }
  PtrRes[TotLen]=0;
  _Aux->SetLen(*Res,TotLen);

  //Free resources
  if(Prv!=-1){ _Aux->Free(Prv); }

  //Return code
  return true;

}

//String operation SSTWI
bool StringComputer::SSTWI(CpuBol *Res,CpuMbl Str,CpuMbl Sub){
  CpuWrd Pos;
  if(!SFIND(&Pos,Str,Sub,0)){ return false; }
  *Res=(Pos==0?true:false);
  return true;
}

//String operation SENWI
bool StringComputer::SENWI(CpuBol *Res,CpuMbl Str,CpuMbl Sub){
  CpuWrd Pos;
  CpuWrd StrLen;
  CpuWrd SubLen;
  if(!IsValid(Str,true) || !IsValid(Sub,true)){ return false; }
  StrLen=_Aux->GetLen(Str);
  SubLen=_Aux->GetLen(Sub);
  if(SubLen>StrLen){ *Res=false; return true; }
  if(!SFIND(&Pos,Str,Sub,StrLen-SubLen)){ return false; }
  *Res=(Pos==StrLen-SubLen?true:false);
  return true;
}

//String operation SISBO
bool StringComputer::SISBO(CpuBol *Res,CpuMbl Str){
  if(!IsValid(Str,true)){ return false; }
  if(strcmp(_Aux->CharPtr(Str),"0")==0 
  || strcmp(_Aux->CharPtr(Str),"1")==0 
  || strcmp(_Aux->CharPtr(Str),"true")==0 
  || strcmp(_Aux->CharPtr(Str),"false")==0){
    *Res=true;
  }
  else{
    *Res=false;
  }
  return true;
}

//String operation SISCH
bool StringComputer::SISCH(CpuBol *Res,CpuMbl Str){
  long Result;
  char *EndPtr;
  if(!IsValid(Str,true)){ return false; }
  errno=0;
  Result=strtol(_Aux->CharPtr(Str),&EndPtr,10);
  if(errno!=0 || (*EndPtr)!=0){ 
    *Res=false; 
  }
  else if(Result>=MIN_CHR && Result<=MAX_CHR){
    *Res=true;
  }
  else{
    *Res=false;
  }
  return true;
}

//String operation SISSH
bool StringComputer::SISSH(CpuBol *Res,CpuMbl Str){
  long long Result;
  char *EndPtr;
  if(!IsValid(Str,true)){ return false; }
  errno=0;
  Result=strtoll(_Aux->CharPtr(Str),&EndPtr,10);
  if(errno!=0 || (*EndPtr)!=0){ 
    *Res=false; 
  }
  else if(Result>=MIN_SHR && Result<=MAX_SHR){
    *Res=true;
  }
  else{
    *Res=false;
  }
  return true;
}

//String operation SISIN
bool StringComputer::SISIN(CpuBol *Res,CpuMbl Str){
  long long Result;
  char *EndPtr;
  if(!IsValid(Str,true)){ return false; }
  errno=0;
  Result=strtoll(_Aux->CharPtr(Str),&EndPtr,10);
  if(errno!=0 || (*EndPtr)!=0){ 
    *Res=false; 
  }
  else if(Result>=MIN_INT && Result<=MAX_INT){
    *Res=true;
  }
  else{
    *Res=false;
  }
  return true;
}

//String operation SISLO
bool StringComputer::SISLO(CpuBol *Res,CpuMbl Str){
  long long Result;
  char *EndPtr;
  if(!IsValid(Str,true)){ return false; }
  errno=0;
  Result=strtoll(_Aux->CharPtr(Str),&EndPtr,10);
  if(errno!=0 || (*EndPtr)!=0){ 
    *Res=false; 
  }
  else if(Result>=MIN_LON && Result<=MAX_LON){
    *Res=true;
  }
  else{
    *Res=false;
  }
  return true;
}

//String operation SISFL
bool StringComputer::SISFL(CpuBol *Res,CpuMbl Str){
  char *EndPtr;
  if(!IsValid(Str,true)){ return false; }
  errno=0;
  strtod(_Aux->CharPtr(Str),&EndPtr);
  if(errno!=0 || (*EndPtr)!=0){ 
    *Res=false; 
  }
  else{
    *Res=true;
  }
  return true;
}

//String to boolean conversion
bool StringComputer::SST2B(CpuBol *Res,CpuMbl Str){
  if(!IsValid(Str,true)){ return false; }
  if(strcmp(_Aux->CharPtr(Str),"1")==0 
  || strcmp(_Aux->CharPtr(Str),"true")==0){
    *Res=true;
  }
  else if(strcmp(_Aux->CharPtr(Str),"0")==0 
  || strcmp(_Aux->CharPtr(Str),"false")==0){
    *Res=false;
  }
  else{
    System::Throw(SysExceptionCode::StringToBooleanConvFailure);
    return false;
  }
  return true;
}

//String to char conversion
bool StringComputer::SST2C(CpuChr *Res,CpuMbl Str){
  if(!IsValid(Str,true)){ return false; }
  *Res=_Aux->CharPtr(Str)[0];
  return true;
}

//String to integer short
bool StringComputer::SST2W(CpuShr *Res,CpuMbl Str){
  long long Result;
  char *EndPtr;
  if(!IsValid(Str,true)){ return false; }
  errno=0;
  Result=strtoll(_Aux->CharPtr(Str),&EndPtr,10);
  if(errno!=0 || (*EndPtr)!=0){
    System::Throw(SysExceptionCode::StringToShortConvFailure);
    return false;
  }
  else if(Result>=MIN_SHR && Result<=MAX_SHR){
    *Res=(CpuShr)Result;
    return true;
  }
  else{
    System::Throw(SysExceptionCode::StringToShortConvFailure);
    return false;
  }
}

//String to integer conversion
bool StringComputer::SST2I(CpuInt *Res,CpuMbl Str){
  long long Result;
  char *EndPtr;
  if(!IsValid(Str,true)){ return false; }
  errno=0;
  Result=strtoll(_Aux->CharPtr(Str),&EndPtr,10);
  if(errno!=0 || (*EndPtr)!=0){
    System::Throw(SysExceptionCode::StringToIntegerConvFailure);
    return false;
  }
  else if(Result>=MIN_INT && Result<=MAX_INT){
    *Res=(CpuInt)Result;
    return true;
  }
  else{
    System::Throw(SysExceptionCode::StringToIntegerConvFailure);
    return false;
  }
}

//String to long conversion
bool StringComputer::SST2L(CpuLon *Res,CpuMbl Str){
  long long Result;
  char *EndPtr;
  if(!IsValid(Str,true)){ return false; }
  errno=0;
  Result=strtoll(_Aux->CharPtr(Str),&EndPtr,10);
  if(errno!=0 || (*EndPtr)!=0){
    System::Throw(SysExceptionCode::StringToLongConvFailure);
    return false;
  }
  else if(Result>=MIN_LON && Result<=MAX_LON){
    *Res=(CpuLon)Result;
    return true;
  }
  else{
    System::Throw(SysExceptionCode::StringToLongConvFailure);
    return false;
  }
}

//String to float conversion
bool StringComputer::SST2F(CpuFlo *Res,CpuMbl Str){
  char *EndPtr;
  if(!IsValid(Str,true)){ return false; }
  errno=0;
  *Res=(CpuFlo)strtod(_Aux->CharPtr(Str),&EndPtr);
  if(errno!=0 || (*EndPtr)!=0){
    System::Throw(SysExceptionCode::StringToFloatConvFailure);
    return false;
  }
  return true;
}

//Boolean to string conversion
bool StringComputer::SBO2S(CpuMbl *Res,CpuBol Bol){
  if(Bol){
    return SCOPY(Res,(char *)"true");
  }
  else{
    return SCOPY(Res,(char *)"false");
  }
}

//Char to string conversion
bool StringComputer::SCH2S(CpuMbl *Res,CpuChr Chr){
  if(Chr==0){
    if(IsAllocated(*Res)){ if(!_Allocate(*Res,0)){ return false; } }
    else{ if(!_NewString(Res,0)){ return false; } }
    _Aux->CharPtr(*Res)[0]=0;
    _Aux->SetLen(*Res,0);
  }
  else{
    if(IsAllocated(*Res)){ if(!_Allocate(*Res,1)){ return false; } }
    else{ if(!_NewString(Res,1)){ return false; } }
    _Aux->CharPtr(*Res)[0]=Chr;
    _Aux->CharPtr(*Res)[1]=0;
    _Aux->SetLen(*Res,1);
  }
  return true;
}

//Short to string conversion
bool StringComputer::SSH2S(CpuMbl *Res,CpuShr Shr){
  CpuWrd Length=snprintf(nullptr,0,"%li",(long)Shr);
  if(Length<0){
    System::Throw(SysExceptionCode::ShortToStringConvFailure);
    return false;
  }
  if(IsAllocated(*Res)){ if(!_Allocate(*Res,Length)){ return false; } }
  else{ if(!_NewString(Res,Length)){ return false; } }
  snprintf(_Aux->CharPtr(*Res),Length+1,"%li",(long)Shr);
  _Aux->SetLen(*Res,Length);
  return true;
}

//Integer to string conversion
bool StringComputer::SIN2S(CpuMbl *Res,CpuInt Int){
  CpuWrd Length=snprintf(nullptr,0,"%li",(long)Int);
  if(Length<0){
    System::Throw(SysExceptionCode::IntegerToStringConvFailure);
    return false;
  }
  if(IsAllocated(*Res)){ if(!_Allocate(*Res,Length)){ return false; } }
  else{ if(!_NewString(Res,Length)){ return false; } }
  snprintf(_Aux->CharPtr(*Res),Length+1,"%li",(long)Int);
  _Aux->SetLen(*Res,Length);
  return true;
}

//Long to string conversion
bool StringComputer::SLO2S(CpuMbl *Res,CpuLon Lon){
  CpuWrd Length=snprintf(nullptr,0,"%lli",(long long)Lon);
  if(Length<0){
    System::Throw(SysExceptionCode::LongToStringConvFailure);
    return false;
  }
  if(IsAllocated(*Res)){ if(!_Allocate(*Res,Length)){ return false; } }
  else{ if(!_NewString(Res,Length)){ return false; } }
  snprintf(_Aux->CharPtr(*Res),Length+1,"%lli",(long long)Lon);
  _Aux->SetLen(*Res,Length);
  return true;
}

//Float to string conversion
bool StringComputer::SFL2S(CpuMbl *Res,CpuFlo Flo){
  CpuWrd Length=snprintf(nullptr,0,"%f",Flo);
  if(Length<0){
    System::Throw(SysExceptionCode::FloatToStringConvFailure);
    return false;
  }
  if(IsAllocated(*Res)){ if(!_Allocate(*Res,Length)){ return false; } }
  else{ if(!_NewString(Res,Length)){ return false; } }
  snprintf(_Aux->CharPtr(*Res),Length+1,"%f",Flo);
  _Aux->SetLen(*Res,Length);
  return true;
}

//Char to string conversionwith format string
bool StringComputer::SCHFM(CpuMbl *Res,CpuChr Chr,CpuMbl Fmt){
  if(!IsValid(Fmt,true)){ return false; }
  CpuWrd Length=snprintf(nullptr,0,_Aux->CharPtr(Fmt),Chr);
  if(Length<0){
    System::Throw(SysExceptionCode::CharStringFormatFailure);
    return false;
  }
  if(IsAllocated(*Res)){ if(!_Allocate(*Res,Length)){ return false; } }
  else{ if(!_NewString(Res,Length)){ return false; } }
  snprintf(_Aux->CharPtr(*Res),Length+1,_Aux->CharPtr(Fmt),Chr);
  _Aux->SetLen(*Res,Length);
  return true;
}

//Short to string conversionwith format string
bool StringComputer::SSHFM(CpuMbl *Res,CpuShr Shr,CpuMbl Fmt){
  if(!IsValid(Fmt,true)){ return false; }
  CpuWrd Length=snprintf(nullptr,0,_Aux->CharPtr(Fmt),Shr);
  if(Length<0){
    System::Throw(SysExceptionCode::ShortStringFormatFailure);
    return false;
  }
  if(IsAllocated(*Res)){ if(!_Allocate(*Res,Length)){ return false; } }
  else{ if(!_NewString(Res,Length)){ return false; } }
  snprintf(_Aux->CharPtr(*Res),Length+1,_Aux->CharPtr(Fmt),Shr);
  _Aux->SetLen(*Res,Length);
  return true;
}

//Integer to string conversionwith format string
bool StringComputer::SINFM(CpuMbl *Res,CpuInt Int,CpuMbl Fmt){
  if(!IsValid(Fmt,true)){ return false; }
  CpuWrd Length=snprintf(nullptr,0,_Aux->CharPtr(Fmt),Int);
  if(Length<0){
    System::Throw(SysExceptionCode::IntegerStringFormatFailure);
    return false;
  }
  if(IsAllocated(*Res)){ if(!_Allocate(*Res,Length)){ return false; } }
  else{ if(!_NewString(Res,Length)){ return false; } }
  snprintf(_Aux->CharPtr(*Res),Length+1,_Aux->CharPtr(Fmt),Int);
  _Aux->SetLen(*Res,Length);
  return true;
}

//Long to string conversionwith format string
bool StringComputer::SLOFM(CpuMbl *Res,CpuLon Lon,CpuMbl Fmt){
  if(!IsValid(Fmt,true)){ return false; }
  CpuWrd Length=snprintf(nullptr,0,_Aux->CharPtr(Fmt),Lon);
  if(Length<0){
    System::Throw(SysExceptionCode::LongStringFormatFailure);
    return false;
  }
  if(IsAllocated(*Res)){ if(!_Allocate(*Res,Length)){ return false; } }
  else{ if(!_NewString(Res,Length)){ return false; } }
  snprintf(_Aux->CharPtr(*Res),Length+1,_Aux->CharPtr(Fmt),Lon);
  _Aux->SetLen(*Res,Length);
  return true;
}

//Float to string conversionwith format string
bool StringComputer::SFLFM(CpuMbl *Res,CpuFlo Flo,CpuMbl Fmt){
  if(!IsValid(Fmt,true)){ return false; }
  CpuWrd Length=snprintf(nullptr,0,_Aux->CharPtr(Fmt),Flo);
  if(Length<0){
    System::Throw(SysExceptionCode::FloatStringFormatFailure);
    return false;
  }
  if(IsAllocated(*Res)){ if(!_Allocate(*Res,Length)){ return false; } }
  else{ if(!_NewString(Res,Length)){ return false; } }
  snprintf(_Aux->CharPtr(*Res),Length+1,_Aux->CharPtr(Fmt),Flo);
  _Aux->SetLen(*Res,Length);
  return true;
}

