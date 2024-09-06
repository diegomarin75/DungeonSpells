//String.cpp: Implementation of a String class
#include "bas/basedefs.hpp"
#include "bas/allocator.hpp"
#include "bas/array.hpp"
#include "bas/buffer.hpp"
#include "bas/string.hpp"

//Constructor from nothing
String::String(){
  _Chr=nullptr;
  _Length=0;
  _Size=0;
}

//Constructor from char String
String::String(const char *Pnt){
  _Chr=nullptr;
  _Length=0;
  _Size=0;
  if(Pnt==nullptr){ return; }
  _Allocate(strlen(Pnt)+1);
  strcpy(_Chr,Pnt);
  _Length=strlen(_Chr);
}

//Constructor from char String with length
String::String(const char *Pnt,long Length){
  _Chr=nullptr;
  _Length=0;
  _Size=0;
  if(Pnt==nullptr){ return; }
  _Allocate(Length+1);
  MemCpy(_Chr,Pnt,Length);
  for(int i=0;i<Length;i++){ _Chr[i]=(_Chr[i]==0?32:_Chr[i]); }
  _Chr[Length]=0;
  _Length=Length;
}

//Constructor from another String
String::String(const String& Str){
  _Chr=nullptr;
  _Length=0;
  _Size=0;
  if(Str._Length==0){ return; }
  _Allocate(Str._Length+1);
  strcpy(_Chr,Str._Chr);
  _Length=Str._Length;
}

//Constructor from single char
String::String(char Chr){
  _Chr=nullptr;
  _Length=0;
  _Size=0;
  _Allocate(2);
  _Chr[0]=Chr;
  _Chr[1]=0;
  _Length=1;
}

//Constructor from char replication
String::String(long Times,char Chr){
  _Chr=nullptr;
  _Length=0;
  _Size=0;
  if(Times<=0){ return; }
  _Allocate(Times+1);
  for(int i=0;i<Times;i++){ _Chr[i]=Chr; }
  _Chr[Times]=0;
  _Length=Times;
}

//Constructor from buffer
String::String(const Buffer& Buff){
  _Chr=nullptr;
  _Length=0;
  _Size=0;
  if(Buff.Length()==0){ return; }
  _Allocate(Buff.Length()+1);
  MemCpy(_Chr,Buff.BuffPnt(),Buff.Length());
  _Chr[Buff.Length()]=0;
  _Length=Buff.Length();
}

//Return buffer from string
Buffer String::ToBuffer() const {
  return Buffer(_Chr,_Length);
}

//Copy string into char buffer
void String::Copy(char *Dest,long MaxLen) const {
  if(_Length!=0){
    strncpy(Dest,_Chr,MaxLen);
    Dest[MaxLen]=0;
  }
  else{
    memset(Dest,0,MaxLen);
  }
}

//Safe substring
String String::Mid(long Position,long Length) const{
  
  //Variables
  String Str0;
  long FinalLength;
  
  //Check call is valid
  if(Position<0 || Length<0){
    std::string Msg="Invalid call to Mid() function (Requested position=" + std::to_string(Position) + ", Requested length=" + std::to_string(Length) + ", Current length=" + std::to_string(_Length) + ")";
    ThrowBaseException((int)ExceptionSource::String,(int)StringException::InvalidMidCall,Msg.c_str());
  }

  //Return empty string if position is over length or length is zero
  if(Position>_Length || Length==0){
    Str0="";
    return Str0;
  }

  //Copy substring String
  FinalLength=(Position+Length<=_Length?Length:_Length-Position);
  Str0._Allocate(FinalLength+1);
  strncpy(Str0._Chr,&this->_Chr[Position],FinalLength);
  Str0._Chr[FinalLength]=0;
  Str0._Length=FinalLength;

  //Return ressult
  return Str0;

}

//Rigth substring
String String::Right(long Length) const{
  if(Length<0){
    std::string Msg="Invalid call to Right() function with negative argument.";
    ThrowBaseException((int)ExceptionSource::String,(int)StringException::InvalidRightCall,Msg.c_str());
  }
  return (Length<=_Length?Mid(_Length-Length,Length):*this);
}
    
//Left substring
String String::Left(long Length) const{
  if(Length<0){
    std::string Msg="Invalid call to Left() function with negative argument.";
    ThrowBaseException((int)ExceptionSource::String,(int)StringException::InvalidLeftCall,Msg.c_str());
  }
  return (Length<=_Length?Mid(0,Length):*this);
}

//CutRigth substring
String String::CutRight(long Length) const{
  if(Length<0){
    std::string Msg="Invalid call to CutRight() function with negative argument.";
    ThrowBaseException((int)ExceptionSource::String,(int)StringException::InvalidCutRightCall,Msg.c_str());
  }
  return (Length<_Length?Mid(0,_Length-Length):String(""));
}
    
//CutLeft substring
String String::CutLeft(long Length) const{
  if(Length<0){
    std::string Msg="Invalid call to CutLeft() function with negative argument.";
    ThrowBaseException((int)ExceptionSource::String,(int)StringException::InvalidCutLeftCall,Msg.c_str());
  }
  return (Length<_Length?Mid(Length,_Length-Length):String(""));
}

//String find number of occurrences
int String::SearchCount(const String& Str, long StartPos) const{
  
  //Variables
  int Count=0;
  long Pos=StartPos;
  long NewPos;
  
  //Search count
  while((NewPos=Search(Str,Pos))!=-1){
    Count++;
    Pos=NewPos+1;
  }

  //Return code
  return Count;

}

//Search for substring at position
bool String::SearchAt(char c, long StartPos) const{
  if(StartPos>_Length-1){ return false; }
  return _Chr[StartPos]==c?true:false;
}

//Search for substring (Occurence==0, find last occurence)
long String::Search(char c, long StartPos, int Occurence) const{
  
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

//Smart search for single string (Occurence==0, find last occurence)
long String::Search(char c, const String& TextQualifier, const String& TextEscaper, const String& RawStrBeg, 
                    const String& RawStrEnd, const String& LevelUp, const String& LevelDown, long StartPos, int Occurence) const {

  //Variables
  int i;
  int Level;
  bool TextQlf;
  bool TextEsc;
  bool LittMode;
  bool RawMode;
  bool LevUp;
  bool LevDown;
  bool RawBeg;
  bool RawEnd;
  int TimesFound;
  long FoundIndex;

  //String split loop
  i=StartPos;
  Level=0;
  LittMode=0;
  RawMode=0;
  TimesFound=0;
  do{

    //Calculate flags
    TextQlf=(TextQualifier.Length()>0 && SearchAt(TextQualifier,i)?1:0);
    TextEsc=(TextEscaper  .Length()>0 && SearchAt(TextEscaper  ,i)?1:0);
    RawBeg =(RawStrBeg    .Length()>0 && SearchAt(RawStrBeg    ,i)?1:0);
    RawEnd =(RawStrEnd    .Length()>0 && SearchAt(RawStrEnd    ,i)?1:0);
    LevUp  =(LevelUp      .Length()>0 && SearchAnyAt(LevelUp   ,i)?1:0);
    LevDown=(LevelDown    .Length()>0 && SearchAnyAt(LevelDown ,i)?1:0);
    if(TextQlf==1 && TextEsc==0 && RawMode==0){ LittMode=(LittMode==0?1:0); }
    if(RawBeg==1 && LittMode==0){ RawMode=1; }
    if(RawEnd==1 && RawMode==1){ RawMode=0; }
    
    //Follow levels (only when not inside string)
    if(LittMode==0 && RawMode==0){
      if(LevUp){ Level++; } else if(LevDown){ Level--; }
    }
    
    //Find string
    if(Level==0 && LittMode==0 && RawMode==0){
      if(SearchAt(c,i)){
        TimesFound++;
        FoundIndex=i;
        if(TimesFound==Occurence && Occurence!=0) break;
      }

    }

    //Increase counter
    if(TextEsc==1){ 
      i+=TextEscaper.Length(); 
    }
    else{
      i++;
    }

  }while(i<_Length);

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

//Search for substring at position
bool String::SearchAt(const String& Str, long StartPos) const{
  
  //Variables
  long i;
  bool Found;

  //Optimization
  if(StartPos>_Length-1 || Str._Length==0 || _Chr[StartPos]!=Str._Chr[0]){ return false; }

  //Search loop
  Found=true;
  for(i=0;i<Str._Length;i++){
    if(StartPos+i<_Length){ 
      if(_Chr[StartPos+i]!=Str._Chr[i]){ 
        Found=false; 
        break; 
      }
    }
    else{
      Found=false; 
      break; 
    }
  }
  
  //Return result
  return Found;

}

//Search for any chars of substring at position
bool String::SearchAnyAt(const String& Str, long StartPos) const{
  if(StartPos>_Length-1){ return false; }
  for(int i=0;i<Str._Length;i++){
    if(_Chr[StartPos]==Str._Chr[i]){ return true; }
  }
  return false;
}

//Search for substring (Occurence==0, find last occurence)
long String::Search(const String& Str, long StartPos, int Occurence) const{
  
  //Variables
  bool Found;
  int TimesFound;
  long i,j;
  long FoundIndex;

  //Search loop
  TimesFound=0;
  for(i=StartPos;i<_Length;i++){
    for(j=0,Found=1;j<Str._Length;j++){
      if(i+j<_Length){ 
        if(_Chr[i+j]!=Str._Chr[j]){ 
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

//Smart search for single string (Occurence==0, find last occurence)
long String::Search(const String& FindStr, const String& TextQualifier, const String& TextEscaper, const String& RawStrBeg, 
                    const String& RawStrEnd, const String& LevelUp, const String& LevelDown, long StartPos, int Occurence) const {

  //Variables
  int i;
  int Level;
  bool TextQlf;
  bool TextEsc;
  bool LittMode;
  bool RawMode;
  bool LevUp;
  bool LevDown;
  bool RawBeg;
  bool RawEnd;
  int TimesFound;
  long FoundIndex;

  //String split loop
  i=StartPos;
  Level=0;
  LittMode=0;
  RawMode=0;
  TimesFound=0;
  do{

    //Calculate flags
    TextQlf=(TextQualifier.Length()>0 && SearchAt(TextQualifier,i)?1:0);
    TextEsc=(TextEscaper  .Length()>0 && SearchAt(TextEscaper  ,i)?1:0);
    RawBeg =(RawStrBeg    .Length()>0 && SearchAt(RawStrBeg    ,i)?1:0);
    RawEnd =(RawStrEnd    .Length()>0 && SearchAt(RawStrEnd    ,i)?1:0);
    LevUp  =(LevelUp      .Length()>0 && SearchAnyAt(LevelUp   ,i)?1:0);
    LevDown=(LevelDown    .Length()>0 && SearchAnyAt(LevelDown ,i)?1:0);
    if(TextQlf==1 && TextEsc==0 && RawMode==0){ LittMode=(LittMode==0?1:0); }
    if(RawBeg==1 && LittMode==0){ RawMode=1; }
    if(RawEnd==1 && RawMode==1){ RawMode=0; }
    
    //Follow levels (only when not inside string)
    if(LittMode==0 && RawMode==0){
      if(LevUp){ Level++; } else if(LevDown){ Level--; }
    }
    
    //Find string
    if(Level==0 && LittMode==0 && RawMode==0){
      if(FindStr.Length()>0 && SearchAt(FindStr,i)){
        TimesFound++;
        FoundIndex=i;
        if(TimesFound==Occurence && Occurence!=0) break;
      }

    }

    //Increase counter
    if(TextEsc==1){ 
      i+=TextEscaper.Length(); 
    }
    else{
      i++;
    }

  }while(i<_Length);

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

//Regular expression match
bool String::Match(const String& Regex) const {
  const std::regex StdRegex(Regex.CharPnt());
  return std::regex_match(_Chr,StdRegex);
}

//Pattern match (accepting only * and ?)
bool String::Like(const String& Pattern) const {

  //Variables
  bool Found;
  long i,j,k;
  long CheckLen;
  long Index;
  long FoundIndex;

  //Special case for empty pattern
  if(Pattern._Length==0){
    return (_Length==0?true:false);
  }

  //Pattern match loop
  i=0;
  j=0;
  while(true){

    //Switch on attern char
    switch(Pattern._Chr[j]){
      
      //Any character
      case '?':
        i++;
        j++;
        break;

      //Substring
      case '*':
        
        //Eat any consecutive asterisks
        while(Pattern._Chr[j]=='*'){ j++; }

        //Return true if pattern ends in asterisk as string would match anyway
        if(j==Pattern._Length){ return true; }
        
        //Determite substring to find in string (until next asterisk or pattern end)
        for(CheckLen=0;Pattern._Chr[j+CheckLen]!=0 && Pattern._Chr[j+CheckLen]!='*';CheckLen++);

        //Find the substring in string from current position
        FoundIndex=-1;
        for(Index=0;_Chr[i+Index]!=0;Index++){
          Found=true;
          for(k=0;_Chr[i+Index+k]!=0 && k<CheckLen;k++){ 
            if(_Chr[i+Index+k]!=Pattern._Chr[j+k] && Pattern._Chr[j+k]!='?'){ Found=false; break; }
          }
          if(Found){
            FoundIndex=Index;
            break;
          }
        }
        if(FoundIndex!=-1){
          if(FoundIndex==0){ return false; }
          i+=FoundIndex+CheckLen;
          j+=CheckLen;
        }

        //Substring was not found
        else{
          return false; 
        }
        break;

      //Other characters
      default:
        if(_Chr[i]!=Pattern._Chr[j]){
          return false;
        }
        i++;
        j++;
        break;
    }

    //Exit if string and pattern are traversed till end
    if(i==_Length && j==Pattern._Length){
      return true;
    }

    //Return false if string or pattern reached end (but not the other as this is checked before)
    if(i==_Length || j==Pattern._Length){
      return false;
    }

  }

}

//This string replacement (Occurence==0 (default), All possible replacements)
void String::ReplaceWithin(const String& Old, const String& New, int Occurence){
  
  //Variables
  long Pos;
  long Length;
  long LenOld;
  long LenNew;
  long SearchPos;
  char *PtrRes;
  char *PtrNew;

  //Exit if old string is empty
  if(_Length==0 || Old._Length==0) return;

  //Get string values
  LenOld=Old._Length;
  LenNew=New._Length;
  PtrRes=_Chr;
  PtrNew=New._Chr;

  //Replace all occurences
  if(Occurence==0){
    SearchPos=0;
    while((Pos=Search(Old,SearchPos))!=-1){
      Length=_Length-LenOld+LenNew;
      if(Length>_Length){ _Allocate(Length+1); }
      PtrRes=_Chr;
      MemMove(PtrRes+Pos+LenNew,PtrRes+Pos+LenOld,Length-Pos-LenNew+1);
      MemCpy(PtrRes+Pos,PtrNew,LenNew);
      PtrRes[Length]=0;
      _Length=Length;
      SearchPos=Pos+LenNew;
    }
  }
  
  //Replace single occurence
  else{
    if((Pos=Search(Old,0,Occurence))!=-1){
      Length=_Length-LenOld+LenNew;
      if(Length>_Length){ _Allocate(Length+1); }
      PtrRes=_Chr;
      MemMove(PtrRes+Pos+LenNew,PtrRes+Pos+LenOld,Length-Pos-LenNew+1);
      MemCpy(PtrRes+Pos,PtrNew,LenNew);
      PtrRes[Length]=0;
      _Length=Length;
    }

  }

}

//String replacement (Occurence==0 (default), All possible replacements)
//String String::Replace(const String& Old, const String& New, int Occurence) const{
//  String Res=*this;
//  Res.ReplaceWithin(Old,New,Occurence);
//  return Res;
//}

//String replacement (Occurence==0 (default), All possible replacements)
void String::ReplaceWithin(const String& Old, const String& New, const String& TextQualifier, const String& TextEscaper, 
                           const String& RawStrBeg, const String& RawStrEnd, const String& LevelUp, const String& LevelDown, int Occurence){

  //Variables
  long Pos;
  long Length;
  long LenOld;
  long LenNew;
  long SearchPos;
  char *PtrRes;
  char *PtrNew;

  //Exit if old string is empty
  if(_Length==0 || Old._Length==0) return;

  //Get string values
  LenOld=Old._Length;
  LenNew=New._Length;
  PtrRes=_Chr;
  PtrNew=New._Chr;

  //Replace all occurences
  if(Occurence==0){
    SearchPos=0;
    while((Pos=Search(Old,TextQualifier,TextEscaper,RawStrBeg,RawStrEnd,LevelUp,LevelDown,SearchPos))!=-1){
      Length=_Length-LenOld+LenNew;
      if(Length>_Length){ _Allocate(Length+1); }
      PtrRes=_Chr;
      MemMove(PtrRes+Pos+LenNew,PtrRes+Pos+LenOld,Length-Pos-LenNew+1);
      MemCpy(PtrRes+Pos,PtrNew,LenNew);
      PtrRes[Length]=0;
      _Length=Length;
      SearchPos=Pos+LenNew;
    }
  }
  
  //Replace single occurence
  else{
    if((Pos=Search(Old,TextQualifier,TextEscaper,RawStrBeg,RawStrEnd,LevelUp,LevelDown,0,Occurence))!=-1){
      Length=_Length-LenOld+LenNew;
      if(Length>_Length){ _Allocate(Length+1); }
      PtrRes=_Chr;
      MemMove(PtrRes+Pos+LenNew,PtrRes+Pos+LenOld,Length-Pos-LenNew+1);
      MemCpy(PtrRes+Pos,PtrNew,LenNew);
      PtrRes[Length]=0;
      _Length=Length;
    }
  }

}

//String replacement (Occurence==0 (default), All possible replacements)
String String::Replace(const String& Old, const String& New, int Occurence) const{

  //Variables
  long Pos;
  long Length;
  long LenOld;
  long LenNew;
  long SearchPos;
  char *PtrRes;
  char *PtrNew;
  String Res;

  //Init return string
  Res=*this;
  
  //Exit if old string is empty
  if(Res._Length==0 || Old._Length==0) return Res;

  //Get string values
  LenOld=Old._Length;
  LenNew=New._Length;
  PtrRes=Res._Chr;
  PtrNew=New._Chr;

  //Replace all occurences
  if(Occurence==0){
    SearchPos=0;
    while((Pos=Res.Search(Old,SearchPos))!=-1){
      Length=Res._Length-LenOld+LenNew;
      if(Length>Res._Length){ Res._Allocate(Length+1); }
      PtrRes=Res._Chr;
      MemMove(PtrRes+Pos+LenNew,PtrRes+Pos+LenOld,Length-Pos-LenNew+1);
      MemCpy(PtrRes+Pos,PtrNew,LenNew);
      PtrRes[Length]=0;
      Res._Length=Length;
      SearchPos=Pos+LenNew;
    }
  }
  
  //Replace single occurence
  else{
    if((Pos=Res.Search(Old,0,Occurence))!=-1){
      Length=Res._Length-LenOld+LenNew;
      if(Length>Res._Length){ Res._Allocate(Length+1); }
      PtrRes=Res._Chr;
      MemMove(PtrRes+Pos+LenNew,PtrRes+Pos+LenOld,Length-Pos-LenNew+1);
      MemCpy(PtrRes+Pos,PtrNew,LenNew);
      PtrRes[Length]=0;
      Res._Length=Length;
    }

  }

  //Return String
  return Res;

}

//String replacement (Occurence==0 (default), All possible replacements)
String String::Replace(const String& Old, const String& New, const String& TextQualifier, const String& TextEscaper, 
                       const String& RawStrBeg, const String& RawStrEnd, const String& LevelUp, const String& LevelDown, int Occurence) const {

  //Variables
  long Pos;
  long Length;
  long LenOld;
  long LenNew;
  long SearchPos;
  String Res;
  char *PtrRes;
  char *PtrNew;

  //Init return string
  Res=*this;
  
  //Exit if old string is empty
  if(Res._Length==0 || Old._Length==0) return Res;

  //Get string values
  LenOld=Old._Length;
  LenNew=New._Length;
  PtrRes=Res._Chr;
  PtrNew=New._Chr;

  //Replace all occurences
  if(Occurence==0){
    SearchPos=0;
    while((Pos=Res.Search(Old,TextQualifier,TextEscaper,RawStrBeg,RawStrEnd,LevelUp,LevelDown,SearchPos))!=-1){
      Length=Res._Length-LenOld+LenNew;
      if(Length>Res._Length){ Res._Allocate(Length+1); }
      PtrRes=Res._Chr;
      MemMove(PtrRes+Pos+LenNew,PtrRes+Pos+LenOld,Length-Pos-LenNew+1);
      MemCpy(PtrRes+Pos,PtrNew,LenNew);
      PtrRes[Length]=0;
      Res._Length=Length;
      SearchPos=Pos+LenNew;
    }
  }
  
  //Replace single occurence
  else{
    if((Pos=Res.Search(Old,TextQualifier,TextEscaper,RawStrBeg,RawStrEnd,LevelUp,LevelDown,0,Occurence))!=-1){
      Length=Res._Length-LenOld+LenNew;
      if(Length>Res._Length){ Res._Allocate(Length+1); }
      PtrRes=Res._Chr;
      MemMove(PtrRes+Pos+LenNew,PtrRes+Pos+LenOld,Length-Pos-LenNew+1);
      MemCpy(PtrRes+Pos,PtrNew,LenNew);
      PtrRes[Length]=0;
      Res._Length=Length;
    }
  }

  //Return String
  return Res;

}

//Trim spaces on the right
String String::TrimRight(char TrimChar) const{

  //Variables
  long EndPos;

  //Special case for empty string
  if(_Length==0){ return *this; }

  //Find last non space position
  EndPos=-1;
  for(long i=_Length-1;i>=0;i--){ if(_Chr[i]!=TrimChar){ EndPos=i; break; }  }

  //Return result
  return (EndPos!=-1?Mid(0,EndPos+1):"");

}

//Trim spaces on the left
String String::TrimLeft(char TrimChar) const{

  //Variables
  long StartPos;
 
  //Special case for empty string
  if(_Length==0){ return *this; }

  //Find first non space position
  StartPos=-1;
  for(long i=0;i<_Length;i++){ if(_Chr[i]!=TrimChar){ StartPos=i; break; } }

  //Return result
  return (StartPos!=-1?Mid(StartPos,_Length-StartPos):"");

}

//Trim spaces
String String::Trim(char TrimChar) const{

  //Variables
  bool Found;
  long StartPos;
  long EndPos;

  //Special case for empty string
  if(_Length==0){ return *this; }

  //Find first and last non space position
  Found=false;
  for(long i=0;i<_Length;i++){ if(_Chr[i]!=TrimChar){ StartPos=i; Found=true; break; } }
  for(long i=_Length-1;i>=0;i--){ if(_Chr[i]!=TrimChar){ EndPos=i; Found=true; break; } }

  //Return result
  return (Found?Mid(StartPos,EndPos-StartPos+1):"");

}

//Convert to upper case
String String::Upper() const{
  String Str0="";
  for(int i=0;i<_Length;i++){ Str0+=toupper(_Chr[i]); }
  return Str0;
}

//Convert to lower case
String String::Lower() const{
  String Str0="";
  for(int i=0;i<_Length;i++){ Str0+=tolower(_Chr[i]); }
  return Str0;
}

//Purge all double spaces from String (leave spaces inside text qualifiers)
String String::Purge(const String& TextQualif, const String& TextEscape) const{

  //Variables
  int i,j;
  bool TextQlf;
  bool TextEsc;
  bool Space;
  bool PrevSpace;
  bool LittMode;
  bool CopyChar;
  String RetStr;

  //String copy loop
  CopyChar=0;
  LittMode=0;
  Space=0;
  PrevSpace=0;
  for(i=0;i<_Length;i++){

    //Calculate flags
    Space=0; PrevSpace=0;
    TextQlf=(TextQualif._Length>0?1:0);
    TextEsc=(TextEscape._Length>0?1:0);
    for(j=0;j<TextQualif._Length;j++){ if(_Chr[i+j]!=TextQualif._Chr[j]) TextQlf=0; break; }
    for(j=0;j<TextEscape._Length;j++){ if(_Chr[i+j]!=TextEscape._Chr[j]) TextEsc=0; break; }
    if(i>0){ if(_Chr[i-1]==' '){ PrevSpace=1; } }
    if(_Chr[i]==' '){ Space=1; }

    //Calculate litteral mode flag
    if(TextQlf==1 && TextEsc==0){ LittMode=(LittMode==0?1:0); }

    //Filter spaces
    if(Space==1 && PrevSpace==1 && LittMode==0){ CopyChar=1; } else{ CopyChar=0; }

    //Append character to result
    if(CopyChar==1){
      RetStr=RetStr+_Chr[i];
    }

  }

  //Return result
  return RetStr;
  
}

//Pad right
String String::LJust(int Length, char Chr) const{

  //Variables
  long i;
  String RetStr;

  //Do not pad if String has greater length
  if(_Length>=Length) return *this;

  //Copy characters
  RetStr=String(Length,Chr);
  for(i=0;i<Length;i++){
    if(i<=_Length-1){
      RetStr._Chr[i]=_Chr[i];
    }
  }

  //Return value
  return RetStr;    
  
}

//Pad left
String String::RJust(int Length, char Chr) const{

  //Variables
  long i;
  String RetStr;

  //Do not pad if String has greater lengtht
  if(_Length>=Length) return *this;

  //Copy characters
  RetStr=String(Length,Chr);
  for(i=0;i<Length;i++){
    if(i>=Length-_Length){
      RetStr._Chr[i]=_Chr[i+_Length-Length];
    }
  }

  //Return value
  return RetStr;    
  
}

//Replicate
String String::Replicate(const String& Str, int Times) const{
  int i;
  String RetStr;
  RetStr="";
  for(i=0;i<Times;i++){
    RetStr=RetStr+Str;
  }
  return RetStr;
}

//Concatenate with separator
String String::ConcatWithSepar(const String& Str, const String& Separ) const{
  String RetStr; 
  if(_Length==0){
    RetStr=Str;
  }
  else{
    RetStr=(*this)+Separ+Str;
  }
  return RetStr;
}

//Contains char
bool String::Contains(char c) const {
  for(long i=0;i<_Length;i++){ if(_Chr[i]==c){ return true; } }
  return false;
}

//Contains array of char
bool String::Contains(const Array<char>& c) const {
  for(long i=0;i<_Length;i++){ 
    for(int j=0;j<c.Length();j++){
      if(_Chr[i]==c[j]){ return true; } 
    }
  }
  return false;
}

//String contains substring
bool String::Contains(const String& Substring) const{
  return (Search(Substring)!=-1?true:false);
}

//String contains only (char list based)
bool String::ContainsOnly(const String& CharList) const{
  for(int i=0;i<_Length;i++){ if(CharList.Search(_Chr[i])==-1){ return false; break; } }
  return true;
}

//Find first position different from pattern
long String::FirstDifferent(const String& Pattern) const{

  //Variables
  long i;
  long RetPos;

  //Search loop
  for(i=0;i<_Length;i++){
    if(SearchAt(Pattern,i)){
      if(i+Pattern._Length<_Length){
        i+=Pattern._Length;
      }
      else{
        RetPos=i;
        break;
      }
    }
    else{
      RetPos=i;
      break;
    }

  }

  //Return value
  return RetPos;

}

//String split
Array<String> String::Split(const String& Separator, int MaxParts) const {

  //Variables
  long Pos;
  String WorkStr;
  Array<String> StrArray;

  //Return empty array when separator is empty
  if(Separator.Length()==0){ return StrArray; }

  //Split loop
  Pos=0;
  WorkStr=*this;
  while((Pos=WorkStr.Search(Separator))!=-1 && (MaxParts==0 || StrArray.Length() < MaxParts-1)){
    StrArray.Add(WorkStr.Mid(0,Pos));
    WorkStr=WorkStr.Right(WorkStr.Length()-(Pos+Separator.Length()));
  }

  //Add last part
  StrArray.Add(WorkStr);

  //Return result
  return StrArray;
  
}

//String smart split
Array<String> String::Split(const String& Separator, const String& TextQualifier, const String& TextEscaper, const String& RawStrBeg, 
                            const String& RawStrEnd, const String& LevelUp, const String& LevelDown, int MaxParts) const {

  //Variables
  long Pos;
  String WorkStr;
  Array<String> StrArray;

  //Return empty array when separator is empty
  if(Separator.Length()==0){ return StrArray; }

  //Split loop
  WorkStr=*this;
  while((Pos=WorkStr.Search(Separator,TextQualifier,TextEscaper,RawStrBeg,RawStrEnd,LevelUp,LevelDown))!=-1 && (MaxParts==0 || StrArray.Length() < MaxParts-1)){
    StrArray.Add(WorkStr.Mid(0,Pos));
    WorkStr=WorkStr.Right(WorkStr.Length()-(Pos+Separator.Length()));
  }

  //Add last part
  StrArray.Add(WorkStr);

  //Return result
  return StrArray;
  
}

//Check string contains any of the chars in given string
bool String::IsAny(const String& CharList) const {
  for(int i=0;i<CharList._Length;i++){ if(Search(CharList[i])!=-1){ return true; break; } }
  return false;
}

//String starts with
bool String::StartsWith(const String& Str) const{
  return _Length!=0 && Str._Length!=0 && _Chr[0]==Str._Chr[0] && SearchAt(Str,0)?true:false;
}

//Check string beginning matches any of the chars in string
bool String::StartsWithAny(const String& CharList) const {
  for(int i=0;i<CharList._Length;i++){ if(StartsWith(CharList[i])){ return true; break; } }
  return false;
}

//Check string beginning matches any of the strings in the array
bool String::StartsWithAny(const Array<String>& List) const {
  for(int i=0;i<List.Length();i++){ if(StartsWith(List[i])){ return true; break; } }
  return false;
}

//String ends with
bool String::EndsWith(const String& Str) const{
  if(_Length!=0 && Str._Length!=0 && _Length-Str._Length>=0){
    return _Chr[_Length-1]==Str._Chr[Str._Length-1] && SearchAt(Str,_Length-Str._Length)?true:false;
  }
  else{
    return false;
  }
}

//Check string ending matches any of the chars in string
bool String::EndsWithAny(const String& CharList) const {
  for(int i=0;i<CharList._Length;i++){ if(EndsWith(CharList[i])){ return true; break; } }
  return false;
}

//Check string ending matches any of the strings in the array
bool String::EndsWithAny(const Array<String>& List) const {
  for(int i=0;i<List.Length();i++){ if(EndsWith(List[i])){ return true; break; } }
  return false;
}

//Get string parts while beginning matches any of the strings in the list
String String::GetWhile(const String& CharList) const {
  bool Found;
  String Result="";
  for(int i=0;i<_Length;i++){
    Found=false;
    for(int j=0;j<CharList._Length;j++){ if(_Chr[i]==CharList[j]){ Found=true; break; } }
    if(Found){
      Result+=_Chr[i];
    }
    else{
      break;
    }
  }
  return Result;
}

//Get string parts while beginning matches any of the strings in the list
String String::GetWhile(const Array<String>& List) const {
  int Index;
  String Result="";
  String WorkStr=*this;
  do{
    Index=-1;
    for(int i=0;i<List.Length();i++){ if(WorkStr.StartsWith(List[i])){ Index=i; break; } }
    if(Index!=-1){
      Result+=List[Index];
      WorkStr=WorkStr.CutLeft(List[Index]._Length);
    }
  }while(Index!=-1 && WorkStr._Length!=0);
  return Result;
}

//Get string characters until beginning matches any of the strings in the list
String String::GetUntil(const String& CharList) const {
  bool Found=false;
  String Result="";
  String WorkStr=*this;
  while(!Found && WorkStr._Length!=0){
    for(int i=0;i<CharList._Length;i++){ if(WorkStr.StartsWith(CharList[i])){ Found=true; break; } }
    if(!Found){
      Result+=WorkStr[0];
      WorkStr=WorkStr.CutLeft(1);
    }
  };
  return Result;
}

//Get string characters until beginning matches any of the strings in the list
String String::GetUntil(const Array<String>& List) const {
  bool Found=false;
  String Result="";
  String WorkStr=*this;
  while(!Found && WorkStr.Length()!=0){
    for(int i=0;i<List.Length();i++){ if(WorkStr.StartsWith(List[i])){ Found=true; break; } }
    if(!Found){
      Result+=WorkStr[0];
      WorkStr=WorkStr.CutLeft(1);
    }
  };
  return Result;
}

//String to integer (safe)
int String::ToInt(bool& Error,int Base) const{
  long Result;
  char *EndPtr;
  if(_Length==0){ Error=1; return 0; }
  if(ContainsOnly(" ")){ Error=1; return 0; }
  errno=0;
  Result=strtol(_Chr,&EndPtr,Base);
  if(errno!=0 || (*EndPtr)!=0){ Error=1; return 0; }
  Error=0;
  return Result;
}

//String to integer (unsafe)
int String::ToInt(int Base) const{
  bool Error;
  return ToInt(Error,Base);
}

//Integer to String
String ToString(int Number,const String& FormatStr){
  const char *Fmt=(FormatStr.Length()==0?"%i":FormatStr._Chr);
  long Size=snprintf(nullptr,0,Fmt,Number);
  if(Size<0) return "";
  String Str=String(Size,32);
  snprintf(Str._Chr,Size+1,Fmt,Number);
  return Str;
}

//String to Long (safe)
long String::ToLong(bool& Error,int Base) const{
  long Result;
  char *EndPtr;
  if(_Length==0){ Error=1; return 0; }
  if(ContainsOnly(" ")){ Error=1; return 0; }
  errno=0;
  Result=strtol(_Chr,&EndPtr,Base);
  if(errno!=0 || (*EndPtr)!=0){ Error=1; return 0; }
  Error=0;
  return Result;
}

//String to long (unsafe)
long String::ToLong(int Base) const{
  bool Error;
  return ToLong(Error,Base);
}

//Long to String
String ToString(long Number,const String& FormatStr){
  const char *Fmt=(FormatStr.Length()==0?"%li":FormatStr._Chr);
  long Size=snprintf(nullptr,0,Fmt,Number);
  if(Size<0) return "";
  String Str=String(Size,32);
  snprintf(Str._Chr,Size+1,Fmt,Number);
  return Str;
}

//String to Long Long (safe)
long long String::ToLongLong(bool& Error,int Base) const{
  long long Result;
  char *EndPtr;
  if(_Length==0){ Error=1; return 0; }
  if(ContainsOnly(" ")){ Error=1; return 0; }
  errno=0;
  Result=strtoll(_Chr,&EndPtr,Base);
  if(errno!=0 || (*EndPtr)!=0){ Error=1; return 0; }
  Error=0;
  return Result;
}

//String to long long (unsafe)
long long String::ToLongLong(int Base) const{
  bool Error;
  return ToLongLong(Error,Base);
}

//Long Long to String
String ToString(long long Number,const String& FormatStr){
  const char *Fmt=(FormatStr.Length()==0?"%lli":FormatStr._Chr);
  long Size=snprintf(nullptr,0,Fmt,Number);
  if(Size<0) return "";
  String Str=String(Size,32);
  snprintf(Str._Chr,Size+1,Fmt,Number);
  return Str;
}

//String to Float (safe)
float String::ToFloat(bool& Error) const{
  float Result;
  char *EndPtr;
  if(_Length==0){ Error=1; return 0; }
  errno=0;
  Result=strtod(_Chr,&EndPtr);
  if(errno!=0 || (*EndPtr)!=0){ Error=1; return 0; }
  Error=0;
  return Result;
}

//String to Float (unsafe)
float String::ToFloat() const{
  bool Error;
  return ToFloat(Error);
}

//Float to String
String ToString(float Number,const String& FormatStr){
  const char *Fmt=(FormatStr.Length()==0?"%f":FormatStr._Chr);
  long Size=snprintf(nullptr,0,Fmt,Number);
  if(Size<0) return "";
  String Str=String(Size,32);
  snprintf(Str._Chr,Size+1,Fmt,Number);
  return Str;
}

//String to Double (safe)
double String::ToDouble(bool& Error) const{
  double Result;
  char *EndPtr;
  if(_Length==0){ Error=1; return 0; }
  errno=0;
  Result=strtod(_Chr,&EndPtr);
  if(errno!=0 || (*EndPtr)!=0){ Error=1; return 0; }
  Error=0;
  return Result;
}

//String to Double (unsafe)
double String::ToDouble() const{
  bool Error;
  return ToDouble(Error);
}

//Double to String
String ToString(double Number,const String& FormatStr){
  const char *Fmt=(FormatStr.Length()==0?"%f":FormatStr._Chr);
  long Size=snprintf(nullptr,0,Fmt,Number);
  if(Size<0) return "";
  String Str=String(Size,32);
  snprintf(Str._Chr,Size+1,Fmt,Number);
  return Str;
}

//Asignation from char String
String& String::operator=(const char *Pnt){
  if(Pnt==nullptr){ _Free(); return *this; }
  _Allocate(strlen(Pnt)+1);
  strcpy(_Chr,Pnt);
  _Length=strlen(_Chr);
  return *this;
}

//Assignation from another String
String& String::operator=(const String& Str){
  if(Str._Length==0){ _Free(); return *this; }
  _Allocate(Str._Length+1);
  strcpy(_Chr,Str._Chr);
  _Length=Str._Length;
  return *this;
}

//Access operator
char& String::operator[](int Index) const {
  if(Index<0 || Index>_Length-1){
    std::string Msg="Invalid string access for index " + std::to_string(Index) + " when string size is " + std::to_string(_Length);
    ThrowBaseException((int)ExceptionSource::String,(int)StringException::InvalidIndex,Msg.c_str());
  }
  return _Chr[Index];
}

//PrOperator + (concatenation of two strings)
String operator+(const String& Str1,const String& Str2){
  String Str0;
  Str0._Allocate(Str1._Length+Str2._Length+1);
  if(Str1._Length!=0){ strcpy(Str0._Chr,Str1._Chr); }
  if(Str2._Length!=0){ strcpy(Str0._Chr+Str1._Length,Str2._Chr); }
  Str0._Length=Str1._Length+Str2._Length;
  Str0._Chr[Str0._Length]=0;
  return Str0;
}

//PrOperator + (concatenation of String and char*)
String operator+(const String& Str,const char *Pnt){
  String Str0;
  long PntLength=(Pnt==nullptr?0:(long)strlen(Pnt));
  Str0._Allocate(Str._Length+PntLength+1);
  if(Str._Length!=0){ strcpy(Str0._Chr,Str._Chr); }
  if(Pnt!=nullptr){ strcpy(Str0._Chr+Str._Length,Pnt); }
  Str0._Length=Str._Length+PntLength;
  Str0._Chr[Str0._Length]=0;
  return Str0;
}

//PrOperator + (concatenation of char* and String)
String operator+(const char *Pnt,const String& Str){
  String Str0;
  long PntLength=(Pnt==nullptr?0:(long)strlen(Pnt));
  Str0._Allocate(PntLength+Str._Length+1);
  if(Pnt!=nullptr){ strcpy(Str0._Chr,Pnt); }
  if(Str._Length!=0){ strcpy(Str0._Chr+PntLength,Str._Chr); }
  Str0._Length=PntLength+Str._Length;
  Str0._Chr[Str0._Length]=0;
  return Str0;
}

//PrOperator + (concatenation of String and char)
String operator+(const String& Str,char c){
  String Str0;
  Str0._Allocate(Str._Length+1+1);
  if(Str._Length!=0){ strcpy(Str0._Chr,Str._Chr); }
  Str0._Chr[Str._Length+0]=c;
  Str0._Chr[Str._Length+1]=0;
  Str0._Length=Str._Length+1;
  Str0._Chr[Str0._Length]=0;
  return Str0;
}

//PrOperator + (concatenation of char* and String)
String operator+(char c,const String& Str){
  String Str0;
  Str0._Allocate(1+Str._Length+1);
  Str0._Chr[0]=c;
  if(Str._Length!=0){ strcpy(Str0._Chr+1,Str._Chr); }
  Str0._Length=1+Str._Length;
  Str0._Chr[Str0._Length]=0;
  return Str0;
}

//Append string
String& String::operator+=(const String& Str){
  if(Str._Length!=0){ 
    _Allocate(_Length+Str._Length+1);
    strcpy(_Chr+_Length,Str._Chr);
    _Length+=Str._Length;
    _Chr[_Length]=0;
  }
  return *this;
}

//Append char*
String& String::operator+=(const char *Pnt){
  long PntLength=(Pnt==nullptr?0:(long)strlen(Pnt));
  if(PntLength!=0){ 
    _Allocate(_Length+PntLength+1);
    strcpy(_Chr+_Length,Pnt);
    _Length+=PntLength;
    _Chr[_Length]=0;
  }
  return *this;
}

//Append char
String& String::operator+=(const char c){
  _Allocate(_Length+1+1);
  _Chr[_Length+0]=c;
  _Length++;
  _Chr[_Length]=0;
  return *this;
}

//Comparison operators
bool operator==(const String& Str1,const String& Str2 ){ if(strcmp(Str1._Chr==nullptr?"":Str1._Chr,Str2._Chr==nullptr?"":Str2._Chr)==0){ return true; } else return false; }
bool operator==(const String& Str, const char *Pnt    ){ if(strcmp(Str._Chr ==nullptr?"":Str._Chr ,Pnt      ==nullptr?"":Pnt      )==0){ return true; } else return false; }
bool operator==(const char *Pnt,   const String& Str  ){ if(strcmp(Pnt      ==nullptr?"":Pnt      ,Str._Chr ==nullptr?"":Str._Chr )==0){ return true; } else return false; }
bool operator!=(const String& Str, const char *Pnt    ){ if(strcmp(Str._Chr ==nullptr?"":Str._Chr ,Pnt      ==nullptr?"":Pnt      )!=0){ return true; } else return false; }
bool operator!=(const String& Str1,const String& Str2 ){ if(strcmp(Str1._Chr==nullptr?"":Str1._Chr,Str2._Chr==nullptr?"":Str2._Chr)!=0){ return true; } else return false; }
bool operator!=(const char *Pnt,   const String& Str  ){ if(strcmp(Pnt      ==nullptr?"":Pnt      ,Str._Chr ==nullptr?"":Str._Chr )!=0){ return true; } else return false; }
bool operator< (const String& Str, const char *Pnt    ){ if(strcmp(Str._Chr ==nullptr?"":Str._Chr ,Pnt      ==nullptr?"":Pnt      )< 0){ return true; } else return false; }
bool operator< (const String& Str1,const String& Str2 ){ if(strcmp(Str1._Chr==nullptr?"":Str1._Chr,Str2._Chr==nullptr?"":Str2._Chr)< 0){ return true; } else return false; }
bool operator< (const char *Pnt,   const String& Str  ){ if(strcmp(Pnt      ==nullptr?"":Pnt      ,Str._Chr ==nullptr?"":Str._Chr )< 0){ return true; } else return false; }
bool operator> (const String& Str, const char *Pnt    ){ if(strcmp(Str._Chr ==nullptr?"":Str._Chr ,Pnt      ==nullptr?"":Pnt      )> 0){ return true; } else return false; }
bool operator> (const String& Str1,const String& Str2 ){ if(strcmp(Str1._Chr==nullptr?"":Str1._Chr,Str2._Chr==nullptr?"":Str2._Chr)> 0){ return true; } else return false; }
bool operator> (const char *Pnt,   const String& Str  ){ if(strcmp(Pnt      ==nullptr?"":Pnt      ,Str._Chr ==nullptr?"":Str._Chr )> 0){ return true; } else return false; }
bool operator<=(const String& Str, const char *Pnt    ){ if(strcmp(Str._Chr ==nullptr?"":Str._Chr ,Pnt      ==nullptr?"":Pnt      )<=0){ return true; } else return false; }
bool operator<=(const String& Str1,const String& Str2 ){ if(strcmp(Str1._Chr==nullptr?"":Str1._Chr,Str2._Chr==nullptr?"":Str2._Chr)<=0){ return true; } else return false; }
bool operator<=(const char *Pnt,   const String& Str  ){ if(strcmp(Pnt      ==nullptr?"":Pnt      ,Str._Chr ==nullptr?"":Str._Chr )<=0){ return true; } else return false; }
bool operator>=(const String& Str, const char *Pnt    ){ if(strcmp(Str._Chr ==nullptr?"":Str._Chr ,Pnt      ==nullptr?"":Pnt      )>=0){ return true; } else return false; }
bool operator>=(const String& Str1,const String& Str2 ){ if(strcmp(Str1._Chr==nullptr?"":Str1._Chr,Str2._Chr==nullptr?"":Str2._Chr)>=0){ return true; } else return false; }
bool operator>=(const char *Pnt,   const String& Str  ){ if(strcmp(Pnt      ==nullptr?"":Pnt      ,Str._Chr ==nullptr?"":Str._Chr )>=0){ return true; } else return false; }

//String -> Stream
std::ostream& operator<<(std::ostream& stream, const String& Str){
  if(LINE_SEPAR!=LINE_END[0] || LINE_EXTRA!=0){
    String Line=Str.Replace("\n",LINE_END);
    if(Str._Length!=0){ stream << Line._Chr << std::flush; }
  }
  else{
    if(Str._Length!=0){ stream << Str._Chr << std::flush; }
  }
  return stream;
}

//Stream -> String
std::istream& operator>>(std::istream& stream, String& Str){
  
  //Exit if enf-of-file was already reached
  //We set fail it for comparisons to boolean
  if(stream.eof()){
    auto state = stream.rdstate(); 
    state |=(std::ios_base::failbit); 
    stream.clear(state);
    Str._Allocate(1);
    Str._Length=0;
    Str._Chr[0]=0;
    return stream;
  }

  //Variables
  bool DelimFound;
  long Length;
  Buffer Buff=Buffer((long)Str.DEFAULT_STREAM_INPUT,0);
  
  //Read loop
  //(read is repeated while delimiter is not found, so we never read an incomplete line)
  Str._Free();
  do{

    //Read line from stream
    stream.getline(Buff.BuffPnt(),Str.DEFAULT_STREAM_INPUT,LINE_SEPAR);
    Length=strlen(Buff.BuffPnt());
    DelimFound=true;

    //Fail bit is set by getline if line is read without delimiter, so we need to clear it
    if(stream.fail() && Buff.Search(LINE_SEPAR)==-1){
      auto state = stream.rdstate(); 
      state &=(~std::ios_base::failbit); 
      stream.clear(state);
      DelimFound=false;
    }

    //Set output string
    Str._Allocate(Str._Length+Length+1);
    if(Length!=0){ strcpy(Str._Chr+Str._Length,Buff.BuffPnt()); }
    Str._Length+=Length;
    Str._Chr[Str._Length]=0;

  }while(!DelimFound && !stream.bad() && !stream.fail() && !stream.eof());

  //Clear line extra character if present at end of the line (belongs to line ending)
  if(LINE_EXTRA!=0 && Str._Length>1 && Str._Chr[Str._Length-1]==LINE_EXTRA){
    Str._Length--;
    Str._Chr[Str._Length]=0;
  }
  
  //Return stream object
  return stream;

}

//Allocate memory for string
void String::_Allocate(long Size){
  
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
    std::string Msg="String overflow (size exceeds 2GB)";
    ThrowBaseException((int)ExceptionSource::String,(int)StringException::StringOverflow,Msg.c_str());
  }

  //Increase size
  if(Size>_Size){
    while(_Size<Size){ _Size+=DATA_CHUNK; }
    ReAllocate=true;
  }
  else if(Size<_Size-DATA_CHUNK){
    while(Size<_Size-DATA_CHUNK){ _Size-=DATA_CHUNK; }
    ReAllocate=true;
  }

  //Reallocation of elements
  if(ReAllocate){
    
    //Get memory
    #ifdef __NOALLOC__
    Chr=new char[(uint32_t)_Size];
    #else
    Chr=Allocator<char>::Take(MemObject::String,_Size);
    #endif

    //Reallocate elements
    if((MinLength=std::min(_Length,Size))!=0){ MemCpy(Chr,_Chr,MinLength); }
    #ifdef __NOALLOC__
    delete[] _Chr;
    #else
    Allocator<char>::Give(MemObject::String,PrevSize,_Chr);
    #endif
    _Chr=Chr;

  }

}

//Free string memory
void String::_Free(){
  if(_Chr==nullptr){ _Length=0; _Size=0; return; }
  #ifdef __NOALLOC__
  delete[] _Chr;
  #else
  Allocator<char>::Give(MemObject::String,_Size,_Chr);
  #endif
  _Chr=nullptr;
  _Length=0;
  _Size=0;
}