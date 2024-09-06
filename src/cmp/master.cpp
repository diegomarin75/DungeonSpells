//Master.cpp: Compiler master data class
#include "bas/basedefs.hpp"
#include "bas/allocator.hpp"
#include "bas/array.hpp"
#include "bas/sortedarray.hpp"
#include "bas/stack.hpp"
#include "bas/queue.hpp"
#include "bas/buffer.hpp"
#include "bas/string.hpp"
#include "sys/sysdefs.hpp"
#include "sys/system.hpp"
#include "sys/stl.hpp"
#include "sys/msgout.hpp"
#include "cmp/parser.hpp"
#include "cmp/binary.hpp"
#include "cmp/master.hpp"

//Debug switches
#define FUNCTION_SEARCH_TRACKING 1
#define GRANT_SEARCH_TRACKING 1

//Internal constants
const String _CompilerConstNamespace="__";
const String _StartFunctionName=SYSTEM_NAMESPACE "start";
const String _InitFunctionName=SYSTEM_NAMESPACE "init";
const String _DelayedInitFunctionName=SYSTEM_NAMESPACE "delayedinit";
const String _MainFunctionName=SYSTEM_NAMESPACE MAIN_FUNCTION;
const String _SystemFunctionResult=SYSTEM_NAMESPACE FUNCTION_RESULT;
const String _SystemElementTypePrefix=SYSTEM_NAMESPACE "elem_";
const String _SystemUndefinedType=SYSTEM_NAMESPACE "undef";
const String _ComplexLitValuePrefix=SYSTEM_NAMESPACE "xlv_";
const String _SystemRefToSelfName=SELF_REF_NAME;
const String _ConvertibleNumber=SYSTEM_NAMESPACE "cvnum";
const String _ConvertibleString=SYSTEM_NAMESPACE "cvstr";
const String _LocalParmName=SYSTEM_NAMESPACE "Stack";
const String _DestroyedVariablePrefix=SYSTEM_NAMESPACE "destroyed";
const String _TempVarSuffixRegl="t";
const String _TempVarSuffixProm="c";
const String _TempVarSuffixMast="m";
const String _LitVarPreffix="lit";

//Scope equality operator
bool operator==(const ScopeDef& Scope1,const ScopeDef& Scope2){
  return (Scope1.Kind==Scope2.Kind && Scope1.ModIndex==Scope2.ModIndex && Scope1.FunIndex==Scope2.FunIndex?true:false);
}

//Scope inequality operator
bool operator!=(const ScopeDef& Scope1,const ScopeDef& Scope2){
  return (Scope1.Kind!=Scope2.Kind || Scope1.ModIndex!=Scope2.ModIndex || Scope1.FunIndex!=Scope2.FunIndex?true:false);
}

//SubScope equality operator
bool operator==(const SubScopeDef& SubScope1,const SubScopeDef& SubScope2){
  return (SubScope1.Kind==SubScope2.Kind && SubScope1.TypIndex==SubScope2.TypIndex?true:false);
}

//SubScope inequality operator
bool operator!=(const SubScopeDef& SubScope1,const SubScopeDef& SubScope2){
  return (SubScope1.Kind!=SubScope2.Kind || SubScope1.TypIndex!=SubScope2.TypIndex?true:false);
}

//Search index constructor
MasterData::SearchIndex::SearchIndex(const String& IndexName,int IndexPos){
  Name=IndexName;
  Pos=IndexPos;
}

//Search index copy constructor
MasterData::SearchIndex::SearchIndex(const SearchIndex& Index){
  _Move(Index); 
}

//Search index copy
MasterData::SearchIndex& MasterData::SearchIndex::operator=(const SearchIndex& Index){
  _Move(Index);
  return *this;
}

//Search index  move
void MasterData::SearchIndex::_Move(const SearchIndex& Index){
  Name=Index.Name;
  Pos=Index.Pos;
}

//Constructor
MasterData::ScopeStkDfn::ScopeStkDfn(ScopeDef NewScope,SubScopeDef NewSubScope){
  Scope=NewScope;
  SubScope=NewSubScope;
  Mod.Reset();
  Trk.Reset();
  Typ.Reset();
  Var.Reset();
  Fun.Reset();
  Fnc.Reset();
  Gra.Reset();
}

//Search index copy constructor
MasterData::ScopeStkDfn::ScopeStkDfn(const ScopeStkDfn& ScopeStk){
  _Move(ScopeStk); 
}

//Search index copy
MasterData::ScopeStkDfn& MasterData::ScopeStkDfn::operator=(const ScopeStkDfn& ScopeStk){
  _Move(ScopeStk);
  return *this;
}

//Search index  move
void MasterData::ScopeStkDfn::_Move(const ScopeStkDfn& ScopeStk){
  Scope=ScopeStk.Scope;
  SubScope=ScopeStk.SubScope;
  Mod=ScopeStk.Mod;
  Trk=ScopeStk.Trk;
  Typ=ScopeStk.Typ;
  Var=ScopeStk.Var;
  Fun=ScopeStk.Fun;
  Fnc=ScopeStk.Fnc;
  Gra=ScopeStk.Gra;
}

//Constructor
MasterData::MasterData(void){
  _LitStrGenerator=0;
  _LitStaGenerator=0;
}

//Destructor
MasterData::~MasterData(void){

  //Close opened dynamic libraries
  for(int i=0;i<_Library.Length();i++){
    if(_Library[i].Handler!=nullptr){
      if(_Library[i].CloseDispatcher!=nullptr){ _Library[i].CloseDispatcher(); }
      #ifdef __WIN__
        FreeLibrary((HMODULE)_Library[i].Handler);
      #else
        dlclose(_Library[i].Handler);
      #endif
    }
  }

}

//Create master data main definitions
void MasterData::SetMainDefinitions(){
  
  //Scope for system defined types and constants
  ScopeDef Scope=_ScopeStk.Top().Scope;
  SubScopeDef SubScope=_ScopeStk.Top().SubScope;

  //Define basic types
  StoreType(Scope,SubScope,"bool"    ,MasterType::Boolean ,false,-1,true,0,0,-1         ,-1,-1,-1,-1,-1,true,SourceInfo()); BolTypIndex=Types.Length()-1;
  StoreType(Scope,SubScope,"char"    ,MasterType::Char    ,false,-1,true,0,0,-1         ,-1,-1,-1,-1,-1,true,SourceInfo()); ChrTypIndex=Types.Length()-1;
  StoreType(Scope,SubScope,"short"   ,MasterType::Short   ,false,-1,true,0,0,-1         ,-1,-1,-1,-1,-1,true,SourceInfo()); ShrTypIndex=Types.Length()-1;
  StoreType(Scope,SubScope,"int"     ,MasterType::Integer ,false,-1,true,0,0,-1         ,-1,-1,-1,-1,-1,true,SourceInfo()); IntTypIndex=Types.Length()-1;
  StoreType(Scope,SubScope,"long"    ,MasterType::Long    ,false,-1,true,0,0,-1         ,-1,-1,-1,-1,-1,true,SourceInfo()); LonTypIndex=Types.Length()-1;
  StoreType(Scope,SubScope,"float"   ,MasterType::Float   ,false,-1,true,0,0,-1         ,-1,-1,-1,-1,-1,true,SourceInfo()); FloTypIndex=Types.Length()-1;
  StoreType(Scope,SubScope,"string"  ,MasterType::String  ,false,-1,true,0,0,-1         ,-1,-1,-1,-1,-1,true,SourceInfo()); StrTypIndex=Types.Length()-1;
  StoreType(Scope,SubScope,"char[]"  ,MasterType::DynArray,false,-1,true,0,1,ChrTypIndex,-1,-1,-1,-1,-1,true,SourceInfo()); ChaTypIndex=Types.Length()-1;
  StoreType(Scope,SubScope,"string[]",MasterType::DynArray,false,-1,true,0,1,StrTypIndex,-1,-1,-1,-1,-1,true,SourceInfo()); StaTypIndex=Types.Length()-1;
  StoreType(Scope,SubScope,"word"    ,WordMasterType()    ,false,-1,true,0,0,-1         ,-1,-1,-1,-1,-1,true,SourceInfo()); WrdTypIndex=Types.Length()-1;

  //Define system constants
  //Important: These constants are stored as global variables, if used inside libraries, 
  //programs that use that libraries will the values when library was compiled, not when program is compiled
  //Hosystem is redundant, as there is a system call for it
  StoreSystemString(Scope,0,_CompilerConstNamespace+"hostsystem"+_CompilerConstNamespace,HostSystemName(GetHostSystem()));
  StoreSystemInteger(Scope,0,_CompilerConstNamespace+"architecture"+_CompilerConstNamespace,GetArchitecture());

  //Fixed array master type method: word .dsize(char dimension)
  StoreMasterMethod("dsize",WrdTypIndex,false,false,true,MasterType::FixArray,MasterMethod::ArfDsize);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),WrdTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"dimension",ChrTypIndex,false,false,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Fixed1-dim array master type method: word .len()
  StoreMasterMethod("len",WrdTypIndex,false,false,true,MasterType::FixArray,MasterMethod::ArfLen);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),WrdTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Fixed 1-dim array master type method: string .join(string separator)
  StoreMasterMethod("join",StrTypIndex,false,false,false,MasterType::FixArray,MasterMethod::ArfJoin);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"separator",StrTypIndex,true,true,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Dynamic array master type method: void .rsize(word size1)
  StoreMasterMethod("rsize",-1,true,false,false,MasterType::DynArray,MasterMethod::ArdRsize1);
  StoreParameter(Functions.Length()-1,"size1",WrdTypIndex,false,false,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Dynamic array master type method: void .rsize(word size1,word size2)
  StoreMasterMethod("rsize",-1,true,false,false,MasterType::DynArray,MasterMethod::ArdRsize2);
  StoreParameter(Functions.Length()-1,"size1",WrdTypIndex,false,false,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"size2",WrdTypIndex,false,false,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Dynamic array master type method: void .rsize(word size1,word size2,word size3)
  StoreMasterMethod("rsize",-1,true,false,false,MasterType::DynArray,MasterMethod::ArdRsize3);
  StoreParameter(Functions.Length()-1,"size1",WrdTypIndex,false,false,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"size2",WrdTypIndex,false,false,1,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"size3",WrdTypIndex,false,false,2,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Dynamic array master type method: void .rsize(word size1,word size2,word size3,word size4)
  StoreMasterMethod("rsize",-1,true,false,false,MasterType::DynArray,MasterMethod::ArdRsize4);
  StoreParameter(Functions.Length()-1,"size1",WrdTypIndex,false,false,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"size2",WrdTypIndex,false,false,1,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"size3",WrdTypIndex,false,false,2,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"size4",WrdTypIndex,false,false,3,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Dynamic array master type method: void .rsize(word size1,word size2,word size3,word size4,word size5)
  StoreMasterMethod("rsize",-1,true,false,false,MasterType::DynArray,MasterMethod::ArdRsize5);
  StoreParameter(Functions.Length()-1,"size1",WrdTypIndex,false,false,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"size2",WrdTypIndex,false,false,1,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"size3",WrdTypIndex,false,false,2,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"size4",WrdTypIndex,false,false,3,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"size5",WrdTypIndex,false,false,4,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Dynamic array master type method: word .dsize(char dimension)
  StoreMasterMethod("dsize",WrdTypIndex,false,false,true,MasterType::DynArray,MasterMethod::ArdDsize);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),WrdTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"dimension",ChrTypIndex,false,false,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Dynamic array master type method: void .reset()
  StoreMasterMethod("reset",-1,true,false,false,MasterType::DynArray,MasterMethod::ArdReset);
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Dynamic 1-dim array master type method: word .len()
  StoreMasterMethod("len",WrdTypIndex,false,false,false,MasterType::DynArray,MasterMethod::ArdLen);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),WrdTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Dynamic 1-dim array master type method: void .add($element elem)
  StoreMasterMethod("add",-1,true,false,false,MasterType::DynArray,MasterMethod::ArdAdd);
  StoreParameter(Functions.Length()-1,GetElementTypePrefix()+"data",-1,false,false,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Dynamic 1-dim array master type method: void .ins(word index,$element elem)
  StoreMasterMethod("ins",-1,true,false,false,MasterType::DynArray,MasterMethod::ArdIns);
  StoreParameter(Functions.Length()-1,"index",WrdTypIndex,false,false,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,GetElementTypePrefix()+"data",-1,false,false,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Dynamic 1-dim array master type method: void .del(word index)
  StoreMasterMethod("del",-1,true,false,false,MasterType::DynArray,MasterMethod::ArdDel);
  StoreParameter(Functions.Length()-1,"index",WrdTypIndex,false,false,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Dynamic 1-dim array master type method: string .join(string separator)
  StoreMasterMethod("join",StrTypIndex,false,false,false,MasterType::DynArray,MasterMethod::ArdJoin);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"separator",StrTypIndex,true,true,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: char .upper()
  StoreMasterMethod("upper",ChrTypIndex,false,false,false,MasterType::Char,MasterMethod::ChrUpper);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ChrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: char .lower()
  StoreMasterMethod("lower",ChrTypIndex,false,false,false,MasterType::Char,MasterMethod::ChrLower);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ChrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: word .len()
  StoreMasterMethod("len",WrdTypIndex,false,false,false,MasterType::String,MasterMethod::StrLen);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),WrdTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: string .sub(word position,word length)
  StoreMasterMethod("sub",StrTypIndex,false,false,false,MasterType::String,MasterMethod::StrSub);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"position",WrdTypIndex,false,false,1,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"length",WrdTypIndex,false,false,2,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: string .right(word length)
  StoreMasterMethod("right",StrTypIndex,false,false,false,MasterType::String,MasterMethod::StrRight);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"length",WrdTypIndex,false,false,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: string .left(word length)
  StoreMasterMethod("left",StrTypIndex,false,false,false,MasterType::String,MasterMethod::StrLeft);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"length",WrdTypIndex,false,false,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: string .cutr(word length)
  StoreMasterMethod("cutr",StrTypIndex,false,false,false,MasterType::String,MasterMethod::StrCutRight);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"length",WrdTypIndex,false,false,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: string .cutl(word length)
  StoreMasterMethod("cutl",StrTypIndex,false,false,false,MasterType::String,MasterMethod::StrCutLeft);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"length",WrdTypIndex,false,false,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: word .search(string substring,word start)
  StoreMasterMethod("search",WrdTypIndex,false,false,false,MasterType::String,MasterMethod::StrSearch);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),WrdTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"substring",StrTypIndex,true,true,1,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"start",WrdTypIndex,false,false,2,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: string .replace(string old, string new)
  StoreMasterMethod("replace",StrTypIndex,false,false,false,MasterType::String,MasterMethod::StrReplace);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"old",StrTypIndex,true,true,1,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"new",StrTypIndex,true,true,2,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: string .trim()
  StoreMasterMethod("trim",StrTypIndex,false,false,false,MasterType::String,MasterMethod::StrTrim);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: string .upper()
  StoreMasterMethod("upper",StrTypIndex,false,false,false,MasterType::String,MasterMethod::StrUpper);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: string .lower()
  StoreMasterMethod("lower",StrTypIndex,false,false,false,MasterType::String,MasterMethod::StrLower);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: string .ljust(int width)
  StoreMasterMethod("ljust",StrTypIndex,false,false,false,MasterType::String,MasterMethod::StrLJust1);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"width",IntTypIndex,false,false,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: string .rjust(int width)
  StoreMasterMethod("rjust",StrTypIndex,false,false,false,MasterType::String,MasterMethod::StrRJust1);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"width",IntTypIndex,false,false,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: string .ljust(int width,char fillchar)
  StoreMasterMethod("ljust",StrTypIndex,false,false,false,MasterType::String,MasterMethod::StrLJust2);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"width",IntTypIndex,false,false,1,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"fillchar",ChrTypIndex,false,false,2,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: string .rjust(int width,char fillchar)
  StoreMasterMethod("rjust",StrTypIndex,false,false,false,MasterType::String,MasterMethod::StrRJust2);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"width",IntTypIndex,false,false,1,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"fillchar",ChrTypIndex,false,false,2,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: bool .match(string regex)
  StoreMasterMethod("match",BolTypIndex,false,false,false,MasterType::String,MasterMethod::StrMatch);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),BolTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"regex",StrTypIndex,true,true,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: bool .like(string pattern)
  StoreMasterMethod("like",BolTypIndex,false,false,false,MasterType::String,MasterMethod::StrLike);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),BolTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"pattern",StrTypIndex,true,true,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: string .replicate(int times)
  StoreMasterMethod("replicate",StrTypIndex,false,false,false,MasterType::String,MasterMethod::StrReplicate);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"times",IntTypIndex,false,false,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: string[] .split(string separator)
  StoreMasterMethod("split",StaTypIndex,false,false,false,MasterType::String,MasterMethod::StrSplit);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StaTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"separator",StrTypIndex,true,true,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: bool .startswith(string substring)
  StoreMasterMethod("startswith",BolTypIndex,false,false,false,MasterType::String,MasterMethod::StrStartswith);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),BolTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"substring",StrTypIndex,true,true,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: bool .endswith(string substring)
  StoreMasterMethod("endswith",BolTypIndex,false,false,false,MasterType::String,MasterMethod::StrEndswith);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),BolTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"substring",StrTypIndex,true,true,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: bool .isbol()
  StoreMasterMethod("isbol",BolTypIndex,false,false,false,MasterType::String,MasterMethod::StrIsbool);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),BolTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: bool .ischr()
  StoreMasterMethod("ischr",BolTypIndex,false,false,false,MasterType::String,MasterMethod::StrIschar);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),BolTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: bool .isshr()
  StoreMasterMethod("isshr",BolTypIndex,false,false,false,MasterType::String,MasterMethod::StrIsshort);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),BolTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: bool .isint()
  StoreMasterMethod("isint",BolTypIndex,false,false,false,MasterType::String,MasterMethod::StrIsint);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),BolTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: bool .islon()
  StoreMasterMethod("islon",BolTypIndex,false,false,false,MasterType::String,MasterMethod::StrIslong);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),BolTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //String Master type method: bool .isflo()
  StoreMasterMethod("isflo",BolTypIndex,false,false,false,MasterType::String,MasterMethod::StrIsfloat);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),BolTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from boolean to char
  StoreMasterMethod("tochr"  ,ChrTypIndex,false,false,false,MasterType::Boolean,MasterMethod::BolTochar);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ChrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from boolean to short
  StoreMasterMethod("toshr" ,ShrTypIndex,false,false,false,MasterType::Boolean,MasterMethod::BolToshort);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ShrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from boolean to int
  StoreMasterMethod("toint"   ,IntTypIndex,false,false,false,MasterType::Boolean,MasterMethod::BolToint);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),IntTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from boolean to long
  StoreMasterMethod("tolon"  ,LonTypIndex,false,false,false,MasterType::Boolean,MasterMethod::BolTolong);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),LonTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from boolean to float
  StoreMasterMethod("toflo" ,FloTypIndex,false,false,false,MasterType::Boolean,MasterMethod::BolTofloat);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),FloTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from boolean to string
  StoreMasterMethod("tostr",StrTypIndex,false,false,false,MasterType::Boolean,MasterMethod::BolTostring);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);
  
  //Data conversion from char to boolean
  StoreMasterMethod("tobol"  ,BolTypIndex,false,false,false,MasterType::Char,MasterMethod::ChrTobool);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ChrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from char to short
  StoreMasterMethod("toshr" ,ShrTypIndex,false,false,false,MasterType::Char,MasterMethod::ChrToshort);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ShrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from char to int
  StoreMasterMethod("toint"   ,IntTypIndex,false,false,false,MasterType::Char,MasterMethod::ChrToint);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),IntTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from char to long
  StoreMasterMethod("tolon"  ,LonTypIndex,false,false,false,MasterType::Char,MasterMethod::ChrTolong);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),LonTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from char to float
  StoreMasterMethod("toflo" ,FloTypIndex,false,false,false,MasterType::Char,MasterMethod::ChrTofloat);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),FloTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from char to string
  StoreMasterMethod("tostr",StrTypIndex,false,false,false,MasterType::Char,MasterMethod::ChrTostring);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);
  
  //Data conversion from char to string with format
  StoreMasterMethod("format",StrTypIndex,false,false,false,MasterType::Char,MasterMethod::ChrFormat);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"fmtspec",StrTypIndex,true,true,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);
  
  //Data conversion from short to bool 
  StoreMasterMethod("tobol"  ,BolTypIndex,false,false,false,MasterType::Short,  MasterMethod::ShrTobool);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),BolTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from short to char 
  StoreMasterMethod("tochr"  ,ChrTypIndex,false,false,false,MasterType::Short,  MasterMethod::ShrTochar);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ChrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from short to int 
  StoreMasterMethod("toint"   ,IntTypIndex,false,false,false,MasterType::Short,  MasterMethod::ShrToint);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),IntTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from short to long 
  StoreMasterMethod("tolon"  ,LonTypIndex,false,false,false,MasterType::Short,  MasterMethod::ShrTolong);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),LonTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from short to float 
  StoreMasterMethod("toflo" ,FloTypIndex,false,false,false,MasterType::Short,  MasterMethod::ShrTofloat);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),FloTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from short to string 
  StoreMasterMethod("tostr",StrTypIndex,false,false,false,MasterType::Short,  MasterMethod::ShrTostring);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from short to string with format
  StoreMasterMethod("format",StrTypIndex,false,false,false,MasterType::Short,MasterMethod::ShrFormat);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"fmtspec",StrTypIndex,true,true,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);
  
  //Data conversion from integer to bool 
  StoreMasterMethod("tobol"  ,BolTypIndex,false,false,false,MasterType::Integer,MasterMethod::IntTobool);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),BolTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from integer to char 
  StoreMasterMethod("tochr"  ,ChrTypIndex,false,false,false,MasterType::Integer,MasterMethod::IntTochar);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ChrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from integer to short 
  StoreMasterMethod("toshr" ,ShrTypIndex,false,false,false,MasterType::Integer,MasterMethod::IntToshort);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ShrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from integer to long 
  StoreMasterMethod("tolon"  ,LonTypIndex,false,false,false,MasterType::Integer,MasterMethod::IntTolong);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),LonTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from integer to float 
  StoreMasterMethod("toflo" ,FloTypIndex,false,false,false,MasterType::Integer,MasterMethod::IntTofloat);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),FloTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from integer to string 
  StoreMasterMethod("tostr",StrTypIndex,false,false,false,MasterType::Integer,MasterMethod::IntTostring);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);
  
  //Data conversion from integer to string with format
  StoreMasterMethod("format",StrTypIndex,false,false,false,MasterType::Integer,MasterMethod::IntFormat);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"fmtspec",StrTypIndex,true,true,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);
  
  //Data conversion from long to bool 
  StoreMasterMethod("tobol"  ,BolTypIndex,false,false,false,MasterType::Long   ,MasterMethod::LonTobool);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),BolTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from long to char 
  StoreMasterMethod("tochr"  ,ChrTypIndex,false,false,false,MasterType::Long   ,MasterMethod::LonTochar);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ChrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from long to short 
  StoreMasterMethod("toshr" ,ShrTypIndex,false,false,false,MasterType::Long   ,MasterMethod::LonToshort);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ShrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from long to int 
  StoreMasterMethod("toint"   ,IntTypIndex,false,false,false,MasterType::Long   ,MasterMethod::LonToint);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),IntTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from long to float 
  StoreMasterMethod("toflo" ,FloTypIndex,false,false,false,MasterType::Long   ,MasterMethod::LonTofloat);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),FloTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from long to string 
  StoreMasterMethod("tostr",StrTypIndex,false,false,false,MasterType::Long   ,MasterMethod::LonTostring);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);
  
  //Data conversion from long to string with format
  StoreMasterMethod("format",StrTypIndex,false,false,false,MasterType::Long   ,MasterMethod::LonFormat);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"fmtspec",StrTypIndex,true,true,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);
  
  //Data conversion from float to bool 
  StoreMasterMethod("tobol"  ,BolTypIndex,false,false,false,MasterType::Float  ,MasterMethod::FloTobool);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),BolTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from float to char 
  StoreMasterMethod("tochr"  ,ChrTypIndex,false,false,false,MasterType::Float  ,MasterMethod::FloTochar);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ChrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from float to short 
  StoreMasterMethod("toshr" ,ShrTypIndex,false,false,false,MasterType::Float  ,MasterMethod::FloToshort);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ShrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from float to int 
  StoreMasterMethod("toint"   ,IntTypIndex,false,false,false,MasterType::Float  ,MasterMethod::FloToint);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),IntTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from float to long 
  StoreMasterMethod("tolon"  ,LonTypIndex,false,false,false,MasterType::Float  ,MasterMethod::FloTolong);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),LonTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from float to string 
  StoreMasterMethod("tostr",StrTypIndex,false,false,false,MasterType::Float  ,MasterMethod::FloTostring);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);
  
  //Data conversion from float to string with format
  StoreMasterMethod("format",StrTypIndex,false,false,false,MasterType::Float,MasterMethod::FloFormat);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"fmtspec",StrTypIndex,true,true,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);
  
  //Data conversion from string to bool 
  StoreMasterMethod("tobol"  ,BolTypIndex,false,false,false,MasterType::String ,MasterMethod::StrTobool);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),BolTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from string to char 
  StoreMasterMethod("tochr"  ,ChrTypIndex,false,false,false,MasterType::String ,MasterMethod::StrTochar);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ChrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from string to short 
  StoreMasterMethod("toshr" ,ShrTypIndex,false,false,false,MasterType::String ,MasterMethod::StrToshort);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ShrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from string to int 
  StoreMasterMethod("toint"   ,IntTypIndex,false,false,false,MasterType::String ,MasterMethod::StrToint);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),IntTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from string to long 
  StoreMasterMethod("tolon"  ,LonTypIndex,false,false,false,MasterType::String ,MasterMethod::StrTolong);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),LonTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from string to float 
  StoreMasterMethod("toflo" ,FloTypIndex,false,false,false,MasterType::String ,MasterMethod::StrTofloat);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),FloTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Class master type method: string .fieldname(int field index)
  StoreMasterMethod("fieldcount",IntTypIndex,false,false,true,MasterType::Class,MasterMethod::ClaFieldCount);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Class master type method: string .fieldname(int field index)
  StoreMasterMethod("fieldnames",StaTypIndex,false,false,true,MasterType::Class,MasterMethod::ClaFieldNames);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StaTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Class master type method: string .fieldtype(int field index)
  StoreMasterMethod("fieldtypes",StaTypIndex,false,false,true,MasterType::Class,MasterMethod::ClaFieldTypes);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StaTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from enum to char
  StoreMasterMethod("tochr"   ,ChrTypIndex,false,false,false,MasterType::Enum ,MasterMethod::EnuTochar);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ChrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from enum to short
  StoreMasterMethod("toshr"   ,ShrTypIndex,false,false,false,MasterType::Enum ,MasterMethod::EnuToshort);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ChrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from enum to int 
  StoreMasterMethod("toint"   ,IntTypIndex,false,false,false,MasterType::Enum ,MasterMethod::EnuToint);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),IntTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from enum to long
  StoreMasterMethod("tolon"   ,LonTypIndex,false,false,false,MasterType::Enum ,MasterMethod::EnuTolong);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),LonTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from enum to float
  StoreMasterMethod("toflo"   ,FloTypIndex,false,false,false,MasterType::Enum ,MasterMethod::EnuTofloat);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),FloTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Data conversion from enum to string
  StoreMasterMethod("tostr"   ,StrTypIndex,false,false,false,MasterType::Enum ,MasterMethod::EnuTostring);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Enum master type method: string .fieldname(int field index)
  StoreMasterMethod("fieldcount",StrTypIndex,false,false,true,MasterType::Enum,MasterMethod::EnuFieldCount);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Enum master type method: string .fieldname(int field index)
  StoreMasterMethod("fieldnames",StaTypIndex,false,false,true,MasterType::Enum,MasterMethod::EnuFieldNames);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StaTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Enum master type method: string .fieldtype(int field index)
  StoreMasterMethod("fieldtypes",StaTypIndex,false,false,true,MasterType::Enum,MasterMethod::EnuFieldTypes);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StaTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Name method for boolean
  StoreMasterMethod("name",StrTypIndex,false,false,true,MasterType::Boolean,MasterMethod::BolName);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Name method for char
  StoreMasterMethod("name",StrTypIndex,false,false,true,MasterType::Char,MasterMethod::ChrName);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Name method for short
  StoreMasterMethod("name",StrTypIndex,false,false,true,MasterType::Short,MasterMethod::ShrName);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Name method for integer
  StoreMasterMethod("name",StrTypIndex,false,false,true,MasterType::Integer,MasterMethod::IntName);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Name method for long
  StoreMasterMethod("name",StrTypIndex,false,false,true,MasterType::Long,MasterMethod::LonName);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Name method for float
  StoreMasterMethod("name",StrTypIndex,false,false,true,MasterType::Float,MasterMethod::FloName);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Name method for string
  StoreMasterMethod("name",StrTypIndex,false,false,true,MasterType::String,MasterMethod::StrName);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Name method for class
  StoreMasterMethod("name",StrTypIndex,false,false,true,MasterType::Class,MasterMethod::ClaName);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Name method for enum
  StoreMasterMethod("name",StrTypIndex,false,false,true,MasterType::Enum,MasterMethod::EnuName);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Name method for fixarray
  StoreMasterMethod("name",StrTypIndex,false,false,true,MasterType::FixArray,MasterMethod::ArfName);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Name method for dynarray
  StoreMasterMethod("name",StrTypIndex,false,false,true,MasterType::DynArray,MasterMethod::ArdName);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Type method for boolean
  StoreMasterMethod("dtype",StrTypIndex,false,false,true,MasterType::Boolean,MasterMethod::BolType);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Type method for char
  StoreMasterMethod("dtype",StrTypIndex,false,false,true,MasterType::Char,MasterMethod::ChrType);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Type method for short
  StoreMasterMethod("dtype",StrTypIndex,false,false,true,MasterType::Short,MasterMethod::ShrType);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Type method for integer
  StoreMasterMethod("dtype",StrTypIndex,false,false,true,MasterType::Integer,MasterMethod::IntType);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Type method for long
  StoreMasterMethod("dtype",StrTypIndex,false,false,true,MasterType::Long,MasterMethod::LonType);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Type method for float
  StoreMasterMethod("dtype",StrTypIndex,false,false,true,MasterType::Float,MasterMethod::FloType);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Type method for string
  StoreMasterMethod("dtype",StrTypIndex,false,false,true,MasterType::String,MasterMethod::StrType);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Type method for class
  StoreMasterMethod("dtype",StrTypIndex,false,false,true,MasterType::Class,MasterMethod::ClaType);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Type method for enum
  StoreMasterMethod("dtype",StrTypIndex,false,false,true,MasterType::Enum,MasterMethod::EnuType);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Type method for fixarray
  StoreMasterMethod("dtype",StrTypIndex,false,false,true,MasterType::FixArray,MasterMethod::ArfType);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Type method for dynarray
  StoreMasterMethod("dtype",StrTypIndex,false,false,true,MasterType::DynArray,MasterMethod::ArdType);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Sizeof method for boolean
  StoreMasterMethod("sizeof",WrdTypIndex,false,false,true,MasterType::Boolean,MasterMethod::BolSizeof);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),WrdTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Sizeof method for char
  StoreMasterMethod("sizeof",WrdTypIndex,false,false,true,MasterType::Char,MasterMethod::ChrSizeof);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),WrdTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Sizeof method for short
  StoreMasterMethod("sizeof",WrdTypIndex,false,false,true,MasterType::Short,MasterMethod::ShrSizeof);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),WrdTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Sizeof method for integer
  StoreMasterMethod("sizeof",WrdTypIndex,false,false,true,MasterType::Integer,MasterMethod::IntSizeof);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),WrdTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Sizeof method for long
  StoreMasterMethod("sizeof",WrdTypIndex,false,false,true,MasterType::Long,MasterMethod::LonSizeof);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),WrdTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Sizeof method for float
  StoreMasterMethod("sizeof",WrdTypIndex,false,false,true,MasterType::Float,MasterMethod::FloSizeof);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),WrdTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Sizeof method for string
  StoreMasterMethod("sizeof",WrdTypIndex,false,false,false,MasterType::String,MasterMethod::StrSizeof);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),WrdTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Sizeof method for enum
  StoreMasterMethod("sizeof",WrdTypIndex,false,false,true,MasterType::Enum,MasterMethod::EnuSizeof);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),WrdTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Sizeof method for fixarray
  StoreMasterMethod("sizeof",WrdTypIndex,false,false,true,MasterType::FixArray,MasterMethod::ArfSizeof);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),WrdTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Sizeof method for dynarray
  StoreMasterMethod("sizeof",WrdTypIndex,false,false,false,MasterType::DynArray,MasterMethod::ArdSizeof);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),WrdTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Tobytes method for boolean
  StoreMasterMethod("tobytes",ChaTypIndex,false,false,false,MasterType::Boolean,MasterMethod::BolToBytes);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ChaTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Tobytes method for char
  StoreMasterMethod("tobytes",ChaTypIndex,false,false,false,MasterType::Char,MasterMethod::ChrToBytes);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ChaTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Tobytes method for short
  StoreMasterMethod("tobytes",ChaTypIndex,false,false,false,MasterType::Short,MasterMethod::ShrToBytes);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ChaTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Tobytes method for integer
  StoreMasterMethod("tobytes",ChaTypIndex,false,false,false,MasterType::Integer,MasterMethod::IntToBytes);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ChaTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Tobytes method for long
  StoreMasterMethod("tobytes",ChaTypIndex,false,false,false,MasterType::Long,MasterMethod::LonToBytes);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ChaTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Tobytes method for float
  StoreMasterMethod("tobytes",ChaTypIndex,false,false,false,MasterType::Float,MasterMethod::FloToBytes);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ChaTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Tobytes method for string
  StoreMasterMethod("tobytes",ChaTypIndex,false,false,false,MasterType::String,MasterMethod::StrToBytes);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ChaTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Tobytes method for class
  StoreMasterMethod("tobytes",ChaTypIndex,false,false,false,MasterType::Class,MasterMethod::ClaToBytes);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ChaTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Tobytes method for enum
  StoreMasterMethod("tobytes",ChaTypIndex,false,false,false,MasterType::Enum,MasterMethod::EnuToBytes);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ChaTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Tobytes method for fixarray
  StoreMasterMethod("tobytes",ChaTypIndex,false,false,false,MasterType::FixArray,MasterMethod::ArfToBytes);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ChaTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Tobytes method for dynarray
  StoreMasterMethod("tobytes",ChaTypIndex,false,false,false,MasterType::DynArray,MasterMethod::ArdToBytes);
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ChaTypIndex,false,true,0,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Frombytes method for Boolean
  StoreMasterMethod("frombytes",BolTypIndex,false,true,false,MasterType::Boolean,MasterMethod::BolFromBytes);   
  StoreParameter(Functions.Length()-1,GetFuncResultName(),BolTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"bytes",ChaTypIndex,true,true,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Frombytes method for Char
  StoreMasterMethod("frombytes",ChrTypIndex,false,true,false,MasterType::Char,MasterMethod::ChrFromBytes);      
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ChrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"bytes",ChaTypIndex,true,true,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Frombytes method for Short
  StoreMasterMethod("frombytes",ShrTypIndex,false,true,false,MasterType::Short,MasterMethod::ShrFromBytes);     
  StoreParameter(Functions.Length()-1,GetFuncResultName(),ShrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"bytes",ChaTypIndex,true,true,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Frombytes method for Integer
  StoreMasterMethod("frombytes",IntTypIndex,false,true,false,MasterType::Integer,MasterMethod::IntFromBytes);   
  StoreParameter(Functions.Length()-1,GetFuncResultName(),IntTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"bytes",ChaTypIndex,true,true,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Frombytes method for Long
  StoreMasterMethod("frombytes",LonTypIndex,false,true,false,MasterType::Long,MasterMethod::LonFromBytes);      
  StoreParameter(Functions.Length()-1,GetFuncResultName(),LonTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"bytes",ChaTypIndex,true,true,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Frombytes method for Float
  StoreMasterMethod("frombytes",FloTypIndex,false,true,false,MasterType::Float,MasterMethod::FloFromBytes);     
  StoreParameter(Functions.Length()-1,GetFuncResultName(),FloTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"bytes",ChaTypIndex,true,true,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Frombytes method for String
  StoreMasterMethod("frombytes",StrTypIndex,false,true,false,MasterType::String,MasterMethod::StrFromBytes);    
  StoreParameter(Functions.Length()-1,GetFuncResultName(),StrTypIndex,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"bytes",ChaTypIndex,true,true,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Frombytes method for Class
  StoreMasterMethod("frombytes",-1,false,true,false,MasterType::Class,MasterMethod::ClaFromBytes);             
  StoreParameter(Functions.Length()-1,GetFuncResultName(),-1,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"bytes",ChaTypIndex,true,true,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Frombytes method for Enum
  StoreMasterMethod("frombytes",-1,false,true,false,MasterType::Enum,MasterMethod::EnuFromBytes);             
  StoreParameter(Functions.Length()-1,GetFuncResultName(),-1,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"bytes",ChaTypIndex,true,true,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Frombytes method for FixArray
  StoreMasterMethod("frombytes",-1,false,true,false,MasterType::FixArray,MasterMethod::ArfFromBytes);           
  StoreParameter(Functions.Length()-1,GetFuncResultName(),-1,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"bytes",ChaTypIndex,true,true,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

  //Frombytes method for DynArray
  StoreMasterMethod("frombytes",-1,false,true,false,MasterType::DynArray,MasterMethod::ArdFromBytes);           
  StoreParameter(Functions.Length()-1,GetFuncResultName(),-1,false,true,0,true,SourceInfo(),"");
  StoreParameter(Functions.Length()-1,"bytes",ChaTypIndex,true,true,1,true,SourceInfo(),"");
  StoreFunctionSearchIndex(Functions.Length()-1);
  StoreFunctionId(Functions.Length()-1);

}

//Return start function name
String MasterData::GetStartFunctionName() const { 
  return _StartFunctionName; 
}

//Return init function name
String MasterData::GetInitFunctionName(const String& Module) const { 
  return _InitFunctionName+Module; 
}

//Return function is a delayed init routine
bool MasterData::IsDelayedInitFunction(const String& FunctionName) const {
  Array<String> Parts=FunctionName.Split(".");
  return Parts.Length()>=1&&Parts[1].StartsWith(_DelayedInitFunctionName)?true:false; 
}

//Return delayed init function name
String MasterData::GetDelayedInitFunctionName(const String& Module) const { 
  return _DelayedInitFunctionName+Module; 
}

//Return super init function name
String MasterData::GetSuperInitFunctionName() const { 
  return _InitFunctionName+"super"+Modules[MainModIndex()].Name; 
}

//Return main function name
String MasterData::GetMainFunctionName() const { 
  return _MainFunctionName; 
}

//Return function result name
String MasterData::GetFuncResultName() const { 
  return _SystemFunctionResult; 
}

//Return element type name
String MasterData::GetElementTypePrefix() const {
  return _SystemElementTypePrefix;
}

//Get self reference name
String MasterData::GetSelfRefName() const {
  return _SystemRefToSelfName;
}

//Composeoperator function name
String MasterData::GetOperatorFuncName(const String& Opr) const {
  return "["+Opr.Trim()+"]";
}

//Get static variable name
String MasterData::GetStaticVarName(int FunIndex,const String& VarName) const {
  return GetSystemNameSpace()+Functions[FunIndex].Fid+"_"+VarName;
}

//Get static field variable name
String MasterData::GetStaticFldName(int FldIndex) const {
  return 
  GetSystemNameSpace()
  +Modules[Types[Fields[FldIndex].SubScope.TypIndex].Scope.ModIndex].Name
  +"_"
  +Types[Fields[FldIndex].SubScope.TypIndex].Name.Replace(".","_")
  +"_"
  +Fields[FldIndex].Name;
}

//Get complex litteral value name begin
String MasterData::GetComplexLitValuePrefix() const {
  return _ComplexLitValuePrefix;
}

//Checks if variable is created for complex lit value replacements
bool MasterData::IsComplexLitVariable(int VarIndex) const {
  if(Variables[VarIndex].Name.StartsWith(GetComplexLitValuePrefix())){
    return true;
  }
  else{
    return false;
  }
}

//Main module functions
int MasterData::MainModIndex() const { return 0; }

//Return init routine was started already
bool MasterData::IsInitRoutineStarted(){
  return Bin.HasInitAddress();
}

//Reset init routine for next module
void MasterData::InitRoutineReset(){
  Bin.ClearInitAddress();
}

//Set start of initialization routine for current module
void MasterData::InitRoutineStart(){
  CpuAdr Address=Bin.CurrentCodeAddress();
  Bin.SetInitAddress(Address,Modules[CurrentScope().ModIndex].Name);
  AsmInitHeader(CurrentScope().ModIndex);
}

//Set end of initialization routine fot current module
bool MasterData::InitRoutineEnd(){

  //Variables
  int DbgSymFunIndex;

  //If there is not initialization routine or no code was emitted and we are at main module, exit clearing assembler buffer
  if((Bin.HasInitAddress()==false || Bin.GetInitAddress()==Bin.CurrentCodeAddress()) && CurrentScope().ModIndex!=MainModIndex()){
    Bin.ClearInitAddress();
    Bin.AsmResetBuffer(AsmSection::Init);
    Bin.AsmResetBuffer(AsmSection::Body);
    return true;
  }

  //Insert NOP instruction in case no init code is generated (because having a routine with nothing inside is not beaufiful)
  Bin.AsmOutNewLine(AsmSection::Body);
  if(Bin.GetInitAddress()==Bin.CurrentCodeAddress()){
    if(!Bin.AsmWriteCode(CpuInstCode::NOP)){ return false; }
    Bin.AsmOutNewLine(AsmSection::Body);
  }

  //Set end of initialization routine
  if(!Bin.AsmWriteCode(CpuInstCode::RET)){ return false; }

  //Set debug symbol for init routine
  if(DebugSymbols){
    DbgSymFunIndex=Bin.StoreDbgSymFunction((CpuChr)FunctionKind::Function,GetInitFunctionName(Modules[CurrentScope().ModIndex].Name),(CpuInt)Modules[CurrentScope().ModIndex].DbgSymIndex,
    (CpuLon)Bin.GetInitAddress(),(CpuLon)(Bin.CurrentCodeAddress()-1),(CpuInt)-1,(CpuBol)true,(CpuInt)0,(CpuInt)-1,(CpuInt)-1,SourceInfo());
  }
  else{
    DbgSymFunIndex=-1;
  }

  //Solve litteral value variables for the init routine
  if(!Bin.SolveLitValueVars(DebugSymbols,Modules[CurrentScope().ModIndex].DbgSymIndex,DbgSymFunIndex)){ return false; }
  
  //Merge code buffers
  Bin.MergeCodeBuffers(Bin.GetInitAddress());

  //Register init routine
  Bin.AppendInitRoutine(Bin.GetInitAddress(),Modules[CurrentScope().ModIndex].Name,GetInitFunctionName(Modules[CurrentScope().ModIndex].Name),CurrentScope().Depth);
  if(DebugSymbols){
    Bin.UpdateDbgSymFunctionEndAddress(DbgSymFunIndex,(CpuLon)(Bin.CurrentCodeAddress()-1));
  }

  //Return code
  return true;
  
}

//Init scope stack
void MasterData::InitScopeStack(){
  ScopeDef Scope=(ScopeDef){ScopeKind::Public,MainModIndex(),-1,0};
  SubScopeDef SubScope=(SubScopeDef){SubScopeKind::None,-1};
  _ScopeStk.Push(ScopeStkDfn(Scope,SubScope));
  DebugMessage(DebugLevel::CmpScopeStk,"Scope init: "+ScopeName(Scope)+", StackLength="+ToString(_ScopeStk.Length()));
}

//Open scope
bool MasterData::OpenScope(ScopeKind Kind,int ModIndex,int FunIndex,CpuLon CodeBlockId){
  
  //Debug message
  DebugMessage(DebugLevel::CmpScopeStk,"Scope open: "+ScopeName((ScopeDef){Kind,ModIndex,FunIndex}));
  
  //Check subscope is closed
  if(_ScopeStk.Top().SubScope.Kind!=SubScopeKind::None){
    SysMessage(192).Print();
    return false;
  }
  
  //Push to scope stack
  ScopeDef Scope=(ScopeDef){Kind,ModIndex,FunIndex,_ScopeStk.Length()};
  SubScopeDef SubScope=(SubScopeDef){SubScopeKind::None,-1};
  _ScopeStk.Push(ScopeStkDfn(Scope,SubScope));
  
  //Preparations for local scopes
  if(Kind==ScopeKind::Local){
    if(_ScopeStk.Length()-2>=0 && _ScopeStk.Top(-1).Scope.Kind!=ScopeKind::Local){ Bin.ResetStackSize(); }
    Bin.ResetLoclGeometryIndex();
    ResetLabelGenerator();
    ResetFlowLabelGenerator();
    CreateParmVariables(Scope,CodeBlockId);
  }

  //Debug message
  DebugMessage(DebugLevel::CmpScopeStk,CurrentScopeStack());
  
  //Return code
  return true;

}

//Close scope
bool MasterData::CloseScope(ScopeKind Kind,bool CompileToLibrary,const Sentence& Stn){

  //Variables
  bool Error=false;
  SourceInfo SrcInfo;
  ScopeDef Scope=_ScopeStk.Top().Scope;

  //Debug message
  DebugMessage(DebugLevel::CmpScopeStk,"Scope close: "+ScopeName(Scope));

  //Get closing token details
  if(Stn.Id!=SentenceId::Empty && Stn.Tokens.Length()!=0){
    SrcInfo=Stn.Tokens[0].SrcInfo();
  }
  else{
    SrcInfo.FileName=Stn.FileName();
    SrcInfo.LineNr=Stn.LineNr();
    SrcInfo.ColNr=-1;
  }
  
  //Check scope stack length
  if(_ScopeStk.Length()==0){
    SrcInfo.Msg(196).Print();
    return false;
  }

  //Check sub scope is closed as well
  if(_ScopeStk.Top().SubScope.Kind!=SubScopeKind::None){
    SrcInfo.Msg(193).Print();
    return false;
  }

  //Check scope kind matches
  if(_ScopeStk.Top().Scope.Kind!=Kind){
    SrcInfo.Msg(195).Print();
    return false;
  }

  //Check defined grants on scope
  if(!CheckGrants(Scope)){ Error=true; }

  //Solve jump addresses 
  if(!Bin.SolveJumpAddresses(_ScopeStk.Length()-1,ScopeName(_ScopeStk.Top().Scope))){ Error=true; }

  //Set function stack size (only for local scopes and not done for local functions as parent function reserves all stack size)
  //Solve litteral value variables (only for local scopes and not local functions since all variables belong to parent function)
  if(_ScopeStk.Top().Scope.Kind==ScopeKind::Local && _ScopeStk.Top(-1).Scope.Kind!=ScopeKind::Local){
    if(!Bin.SetFunctionStackInst()){ Error=true; }
    if(!Bin.SolveLitValueVars(DebugSymbols,Modules[_ScopeStk.Top().Scope.ModIndex].DbgSymIndex,Functions[_ScopeStk.Top().Scope.FunIndex].DbgSymIndex)){ Error=true; }
    Bin.SetFunctionStackSize(Functions[_ScopeStk.Top().Scope.FunIndex].Address);
    ModifyNestedFunctionAddresses(_ScopeStk.Top().Scope.FunIndex);
    Bin.MergeCodeBuffers(Functions[_ScopeStk.Top().Scope.FunIndex].Address);
  }

  //Solve forward calls (skipping nested functions, since this is done on parent function)
  if((_ScopeStk.Top().Scope.Kind==ScopeKind::Local && _ScopeStk.Top(-1).Scope.Kind!=ScopeKind::Local)
  || _ScopeStk.Top().Scope.Kind!=ScopeKind::Local){
    if(!Bin.SolveForwardCalls(_ScopeStk.Length()-1,ScopeName(_ScopeStk.Top().Scope))){ Error=true; }
  }

  //Switch on scope kind
  switch(Scope.Kind){
    
    //Close module public scope
    case ScopeKind::Public:
      if(CompileToLibrary){ AsmExports(); }
      CheckUnusedScopeObjects(Scope,SrcInfo,0,-1);
      CheckUndefinedFunctions(Scope,SrcInfo);
      if(_ScopeStk.Length()>1){
        if(!CopyPublicIndexes(_ScopeStk.Length()-1)){ Error=true; }
      }
      if(_ScopeStk.Length()==1){ AsmDlCallIds(); }
      break;
    
    //Close module private scope
    case ScopeKind::Private:
      CheckUnusedScopeObjects(Scope,SrcInfo,0,-1);
      CheckUndefinedFunctions(Scope,SrcInfo);
      PurgeScopeObjects(Scope);
      break;
    
    //Close local scope
    case ScopeKind::Local:  
      CheckUnusedScopeObjects(Scope,SrcInfo,0,-1);
      CheckUndefinedFunctions(Scope,SrcInfo);
      PurgeScopeObjects(Scope);
      InitLabelGenerator();
      InitFlowLabelGenerator();
      break;
  }
  
  //Remove scope from stack
  _ScopeStk.Pop();

  //Debug message
  DebugMessage(DebugLevel::CmpScopeStk,CurrentScopeStack());
  
  //Return code
  if(Error){
    return false;
  }
  else{
    return true;
  }

}

//Delete top scope (no checks done at all)
void MasterData::DeleteScope(){

  //Debug message
  DebugMessage(DebugLevel::CmpScopeStk,"Scope delete: "+ScopeName(_ScopeStk.Top().Scope));

  //Remove scope from stack
  _ScopeStk.Pop();

  //Debug message
  DebugMessage(DebugLevel::CmpScopeStk,CurrentScopeStack());
  
}

//Print current scope stack
String MasterData::CurrentScopeStack() const {
  String Output="";
  if(_ScopeStk.Length()!=0){
    Output="Current scope: "+ScopeName(_ScopeStk.Top().Scope);
  }
  else{
    Output="Current scope: {}";
  }
  Output+=", StackLength="+ToString(_ScopeStk.Length());
  if(_ScopeStk.Length()!=0){
    Output+=" --> [";
    for(int i=0;i<_ScopeStk.Length();i++){
      Output+=(i==0?"":",")+ScopeName(_ScopeStk[i].Scope);
    }
    Output+="]";
  }
  return Output;
}

//Current module
String MasterData::CurrentModule() const {
  return (_ScopeStk.Top().Scope.ModIndex!=-1?Modules[_ScopeStk.Top().Scope.ModIndex].Name:String("(none)"));
}

//Current scope
const ScopeDef& MasterData::CurrentScope() const{
  return _ScopeStk.Top().Scope;
}

//Parent scope (only to be called when it is sure there is a parent scope)
const ScopeDef& MasterData::ParentScope() const{
  return _ScopeStk.Top(-1).Scope;
}

//Scope description
String MasterData::ScopeDescription(const ScopeDef& Scope) const {
  String Name;
  switch(Scope.Kind){
    case ScopeKind::Public:  Name="public:"+(Scope.ModIndex==-1?"null":Modules[Scope.ModIndex].Name); break;
    case ScopeKind::Private: Name="private:"+(Scope.ModIndex==-1?"null":Modules[Scope.ModIndex].Name); break;
    case ScopeKind::Local:   Name="local:"+Functions[Scope.FunIndex].Name+"()"; break;
  }
  return Name;
}

//Scope technical name
String MasterData::ScopeName(const ScopeDef& Scope) const {
  String Name;
  switch(Scope.Kind){
    case ScopeKind::Public:  Name="{Pub:"+ToString(Scope.Depth)+":"+(Scope.ModIndex==-1?"null":(Scope.ModIndex==0?MAIN_FUNCTION:Modules[Scope.ModIndex].Name))+"}"; break;
    case ScopeKind::Private: Name="{Pri:"+ToString(Scope.Depth)+":"+(Scope.ModIndex==-1?"null":(Scope.ModIndex==0?MAIN_FUNCTION:Modules[Scope.ModIndex].Name))+"}"; break;
    case ScopeKind::Local:   Name="{Loc:"+ToString(Scope.Depth)+":"+(Scope.FunIndex==-1?"null":Functions[Scope.FunIndex].Name)+"}"; break;
  }
  return Name;
}

//Find scope on scope stack
int MasterData::ScopeFindIndex(const ScopeDef& Scope) const {
  for(int i=_ScopeStk.Length()-1;i>=0;i--){
    if(_ScopeStk[i].Scope==Scope){
      return i;
    }
  }
  return -1;
}

//Find module private parent scope (returns index)
int MasterData::_InnerPrivScope() const {
  for(int i=_ScopeStk.Length()-1;i>=0;i--){
    if(_ScopeStk[i].Scope.Kind==ScopeKind::Private){
      return i;
    }
  }
  return -1;
}

//Find module private parent scope (returns scope)
ScopeDef MasterData::InnerPrivScope() const {
  int ScopeIndex=_InnerPrivScope();
  if(ScopeIndex!=-1){ return _ScopeStk[ScopeIndex].Scope; }
  return (ScopeDef){(ScopeKind)-1,-1,-1,-1};
}

//Sub scope open
bool MasterData::SubScopeBegin(SubScopeKind Kind,int TypIndex){
  DebugMessage(DebugLevel::CmpScopeStk,"Subscope begin: "+SubScopeName((SubScopeDef){Kind,TypIndex}));
  _ScopeStk.Top().SubScope=(SubScopeDef){Kind,TypIndex};
  return true;
}

//Sub scope exit
bool MasterData::SubScopeEnd(){
  DebugMessage(DebugLevel::CmpScopeStk,"Subscope end: "+SubScopeName(_ScopeStk.Top().SubScope));
  if(_ScopeStk.Top().SubScope.Kind==SubScopeKind::None){
    SysMessage(194).Print();
    return false;
  }
  if(_ScopeStk.Top().SubScope.TypIndex!=-1){
    if(Types[_ScopeStk.Top().SubScope.TypIndex].SubScope.Kind!=SubScopeKind::None){
      _ScopeStk.Top().SubScope=(SubScopeDef){Types[_ScopeStk.Top().SubScope.TypIndex].SubScope.Kind,Types[_ScopeStk.Top().SubScope.TypIndex].SubScope.TypIndex};
    }
    else{
      _ScopeStk.Top().SubScope=(SubScopeDef){SubScopeKind::None,-1};
    }
  }
  else{
    _ScopeStk.Top().SubScope=(SubScopeDef){SubScopeKind::None,-1};
  }
  return true;
}

//Current sub scope
const SubScopeDef& MasterData::CurrentSubScope() const{
  return _ScopeStk.Top().SubScope;
}

//Subscope name
String MasterData::SubScopeName(const SubScopeDef& SubScope) const {
  String Name;
  switch(SubScope.Kind){
    case SubScopeKind::Public:  Name="{Pub:"+CannonicalTypeName(SubScope.TypIndex)+"}"; break;
    case SubScopeKind::Private: Name="{Pri:"+CannonicalTypeName(SubScope.TypIndex)+"}"; break;
    case SubScopeKind::None:    Name="{}"; break;
  }
  return Name;
}

//Function kind name
String MasterData::FunctionKindName(FunctionKind Kind) const {
  String Name;
  switch(Kind){
    case FunctionKind::Function   : Name="Function";    break;
    case FunctionKind::MasterMth  : Name="MasterMth";   break;
    case FunctionKind::Member     : Name="Member";      break;
    case FunctionKind::SysCall    : Name="SysCall";     break;
    case FunctionKind::SysFunc    : Name="SysFunc";     break;
    case FunctionKind::DlFunc     : Name="DlFunc";      break;
    case FunctionKind::Operator   : Name="Operator";    break;
  }
  return Name;
}
  
//Increase Label Generator
void MasterData::IncreaseLabelGenerator(){
  _LabelGenerator2++;
}

//Get Label Generator
long MasterData::GetLabelGenerator(){
  return 10000*_LabelGenerator1+_LabelGenerator2;
}

//Reset label generator (to be called at initialization of each scope)
void MasterData::ResetLabelGenerator() {
  _LabelGenerator1++;
  _LabelGenerator2=0;
}

//Init label generator (to be called once at beginning)
void MasterData::InitLabelGenerator() {
  _LabelGenerator1=0;
  _LabelGenerator2=0;
}

//Increase flow label Generator
void MasterData::IncreaseFlowLabelGenerator(){
  _FlowLabelGenerator2++;
}

//Get flow label Generator
CpuLon MasterData::GetFlowLabelGenerator(){
  return 10000*_FlowLabelGenerator1+_FlowLabelGenerator2;
}

//Reset flow label generator (to be called at initialization of each scope)
void MasterData::ResetFlowLabelGenerator() {
  _FlowLabelGenerator1++;
  _FlowLabelGenerator2=0;
}

//Init flow label generator (to be called once at beginning)
void MasterData::InitFlowLabelGenerator() {
  _FlowLabelGenerator1=0;
  _FlowLabelGenerator2=0;
}

//Check class has all fields visible from scope
bool MasterData::AreAllFieldsVisible(const ScopeDef& Scope,int ClassTypIndex) const {
  
  //Variables
  int i;
  bool AllVisible;
  
  //Init result
  AllVisible=true;
  
  //Check visibility of all fields
  i=-1;
  while((i=FieldLoop(ClassTypIndex,i))!=-1){
    if(!IsMemberVisible(Scope,ClassTypIndex,i,-1)){ AllVisible=false; break; }
  }

  //Return result
  return AllVisible;

}

//Check member is visible from scope
bool MasterData::IsMemberVisible(const ScopeDef& Scope,int ClassTypIndex,int FldIndex,int FunIndex) const {

  //Check grant for field
  if(FldIndex!=-1){

    //Public fields are always visible
    if(Fields[FldIndex].SubScope.Kind==SubScopeKind::Public){ return true; }
    
    //Check private fields
    switch(Scope.Kind){
      
      //Public / Private scopes cannot access class private fields
      case ScopeKind::Public: 
      case ScopeKind::Private: 
        return false; 
        break;

      //Checks for local scopes
      case ScopeKind::Local:   
        switch(Functions[Scope.FunIndex].Kind){
          case FunctionKind::Function : 
            if(GraSearch(GrantKind::FunctToClass,ClassTypIndex,"",Functions[Scope.FunIndex].Name,-1,-1)!=-1
            || GraSearch(GrantKind::FunctToField,ClassTypIndex,"",Functions[Scope.FunIndex].Name,FldIndex,-1)!=-1){ 
              return true; 
            }
            else{
              return false;
            }
            break;
          case FunctionKind::Operator : 
            if(GraSearch(GrantKind::OpertToClass,ClassTypIndex,"",Functions[Scope.FunIndex].Name,-1,-1)!=-1
            || GraSearch(GrantKind::OpertToField,ClassTypIndex,"",Functions[Scope.FunIndex].Name,FldIndex,-1)!=-1){ 
              return true; 
            }
            else{
              return false;
            }
            break;
          case FunctionKind::Member     : 
            if(Functions[Scope.FunIndex].SubScope.TypIndex==ClassTypIndex 
            || GraSearch(GrantKind::ClassToClass,ClassTypIndex,Types[Functions[Scope.FunIndex].SubScope.TypIndex].Name,"",-1,-1)!=-1
            || GraSearch(GrantKind::ClassToField,ClassTypIndex,Types[Functions[Scope.FunIndex].SubScope.TypIndex].Name,"",FldIndex,-1)!=-1
            || GraSearch(GrantKind::FunMbToClass,ClassTypIndex,Types[Functions[Scope.FunIndex].SubScope.TypIndex].Name,Functions[Scope.FunIndex].Name,-1,-1)!=-1
            || GraSearch(GrantKind::FunMbToField,ClassTypIndex,Types[Functions[Scope.FunIndex].SubScope.TypIndex].Name,Functions[Scope.FunIndex].Name,FldIndex,-1)!=-1){
              return true; 
            }
            else{
              return false;
            }
            break;
          case FunctionKind::MasterMth:
          case FunctionKind::SysCall  :
          case FunctionKind::SysFunc  :
          case FunctionKind::DlFunc   :
            return false;
            break;
        }
        break;
    }

  }

  //Check grant for methods
  else if(FunIndex!=-1){

    //Public function methods are always visible
    if(Functions[FunIndex].SubScope.Kind==SubScopeKind::Public){ return true; }
    
    //Check private fields
    switch(Scope.Kind){
      
      //Public / Private scopes cannot access class private fields
      case ScopeKind::Public: 
      case ScopeKind::Private: 
        return false; 
        break;

      //Checks for local scopes
      case ScopeKind::Local:   
        switch(Functions[Scope.FunIndex].Kind){
          case FunctionKind::Function : 
            if(GraSearch(GrantKind::FunctToClass,ClassTypIndex,"",Functions[Scope.FunIndex].Name,-1,-1)!=-1
            || GraSearch(GrantKind::FunctToFunMb,ClassTypIndex,"",Functions[Scope.FunIndex].Name,-1,FunIndex)!=-1){ 
              return true; 
            }
            else{
              return false;
            }
            break;
          case FunctionKind::Operator : 
            if(GraSearch(GrantKind::OpertToClass,ClassTypIndex,"",Functions[Scope.FunIndex].Name,-1,-1)!=-1
            || GraSearch(GrantKind::OpertToFunMb,ClassTypIndex,"",Functions[Scope.FunIndex].Name,-1,FunIndex)!=-1){ 
              return true; 
            }
            else{
              return false;
            }
            break;
          case FunctionKind::Member     : 
            if(Functions[Scope.FunIndex].SubScope.TypIndex==ClassTypIndex 
            || GraSearch(GrantKind::ClassToClass,ClassTypIndex,Types[Functions[Scope.FunIndex].SubScope.TypIndex].Name,"",-1,-1)!=-1
            || GraSearch(GrantKind::ClassToFunMb,ClassTypIndex,Types[Functions[Scope.FunIndex].SubScope.TypIndex].Name,"",-1,FunIndex)!=-1
            || GraSearch(GrantKind::FunMbToClass,ClassTypIndex,Types[Functions[Scope.FunIndex].SubScope.TypIndex].Name,Functions[Scope.FunIndex].Name,-1,-1)!=-1
            || GraSearch(GrantKind::FunMbToFunMb,ClassTypIndex,Types[Functions[Scope.FunIndex].SubScope.TypIndex].Name,Functions[Scope.FunIndex].Name,-1,FunIndex)!=-1){
              return true; 
            }
            else{
              return false;
            }
            break;
          case FunctionKind::MasterMth:
          case FunctionKind::SysCall  :
          case FunctionKind::SysFunc  :
          case FunctionKind::DlFunc   :
            return false;
            break;
        }
        break;
    }

  }

  //Return no grant found
  return false;

}

//Remove associated search index entry for the data type
bool MasterData::TypeDetach(int TypIndex){
  int Index;
  if(_ScopeStk.Length()==0){ return false; }
  for(int i=_ScopeStk.Length()-1;i>=0;i--){
    Index=_ScopeStk[i].Typ.Search(Modules[Types[TypIndex].Scope.ModIndex].Name+"."+Types[TypIndex].Name);
    if(Index!=-1){ _ScopeStk[i].Typ.Delete(Index); return true; }
    if(_ScopeStk[i].Scope.Kind==ScopeKind::Public){ return false; }
  }
  return false;
}

//Dot collission check (Checks identifiers that can happen before member operator)
bool MasterData::DotCollissionCheck(const String& Idn,String& FoundObject) const {

  //Variables
  int TrkIndex;
  int TypIndex;
  int VarIndex;

  //Find identifier as module tracker
  if((TrkIndex=TrkSearch(Idn))!=-1){
    FoundObject="module tracker "+Trackers[TrkIndex].Name;
    return false;
  }

  //Find identifier as structured type
  if((TypIndex=TypSearch(Idn,CurrentScope().ModIndex))!=-1){
    if(Types[TypIndex].MstType==MasterType::Class && Types[TypIndex].Scope.ModIndex==CurrentScope().ModIndex){
      FoundObject="class name "+CannonicalTypeName(TypIndex);
      return false;
    }
    if(Types[TypIndex].MstType==MasterType::Enum && Types[TypIndex].Scope.ModIndex==CurrentScope().ModIndex){
      FoundObject="enumerated type "+CannonicalTypeName(TypIndex);
      return false;
    }
  }

  //Find identifier as structured variable
  if((VarIndex=VarSearch(Idn,CurrentScope().ModIndex))!=-1){
    if(Types[Variables[VarIndex].TypIndex].MstType==MasterType::Class && Variables[VarIndex].Scope.ModIndex==CurrentScope().ModIndex){
      FoundObject="class instance "+CannonicalTypeName(TypIndex);
      return false;
    }
  }

  //Return no error
  return true;

}

//Module search
int MasterData::ModSearch(const String& Name) const {
  int Index;
  if(_ScopeStk.Length()==0){ return -1; }
  for(int i=_ScopeStk.Length()-1;i>=0;i--){
    if(_ScopeStk[i].Scope.Kind==ScopeKind::Public){
      Index=_ScopeStk[i].Mod.Search(Name);
      if(Index!=-1){ return _ScopeStk[i].Mod[Index].Pos; }
    }
  }
  return -1;
}

//Tracker search
int MasterData::TrkSearch(const String& Name) const {
  int Index;
  if(_ScopeStk.Length()==0){ return -1; }
  for(int i=_ScopeStk.Length()-1;i>=0;i--){
    if(_ScopeStk[i].Scope.Kind==ScopeKind::Public){
      Index=_ScopeStk[i].Trk.Search(Name);
      if(Index!=-1){ return _ScopeStk[i].Trk[Index].Pos; }
      return -1;
    }
  }
  return -1;
}

//Datatype search
int MasterData::TypSearch(const String& Name,int ModIndex) const {
  
  //Variables
  int Index;

  //Skip search if scope stack is not defined yet
  if(_ScopeStk.Length()==0){ return -1; }
  
  //Search type in scope stack
  for(int i=_ScopeStk.Length()-1;i>=0;i--){
    
    //Normal search
    Index=_ScopeStk[i].Typ.Search(Modules[ModIndex].Name+"."+Name);
    if(Index!=-1){ return _ScopeStk[i].Typ[Index].Pos; }

    //Search abreviated name when inside subscope (current class name can be omitted from type name)
    if(_ScopeStk[i].SubScope.Kind!=SubScopeKind::None){
      Index=_ScopeStk[i].Typ.Search(Modules[ModIndex].Name+"."+Types[_ScopeStk[i].SubScope.TypIndex].Name+"."+Name);
      if(Index!=-1){ return _ScopeStk[i].Typ[Index].Pos; }
    }

    //Stop if we reach first public scope
    if(_ScopeStk[i].Scope.Kind==ScopeKind::Public){ break; }
  }

  //When last public scope is searched and type is not found we do a last search on main public scope for system types
  //but only if type is searched as belonging to current module
  if(CurrentScope().ModIndex==ModIndex){
    Index=_ScopeStk[0].Typ.Search(Modules[MainModIndex()].Name+"."+Name);
    if(Index!=-1){ 
      if(Types[_ScopeStk[0].Typ[Index].Pos].IsSystemDef){ return _ScopeStk[0].Typ[Index].Pos; }
    }
  }

  //Return not found
  return -1;

}

//Variable search
int MasterData::VarSearch(const String& Name,int ModIndex,bool FindHidden,int *SearchIndex) const {
  
  //Variables
  int Index;
  int VarIndex;

  //Skip search if scope stack is not defined yet
  if(_ScopeStk.Length()==0){ return -1; }

  //Search type in scope stack
  Index=-1;
  VarIndex=-1;
  for(int i=_ScopeStk.Length()-1;i>=0;i--){
    Index=_ScopeStk[i].Var.Search(Modules[ModIndex].Name+"."+Name);
    if(Index!=-1){ VarIndex=_ScopeStk[i].Var[Index].Pos; break; }
    if(_ScopeStk[i].Scope.Kind==ScopeKind::Local){
      Index=_ScopeStk[_InnerPrivScope()].Var.Search(Modules[ModIndex].Name+"."+GetStaticVarName(_ScopeStk[i].Scope.FunIndex,Name));
      if(Index!=-1){ VarIndex=_ScopeStk[_InnerPrivScope()].Var[Index].Pos; break; }
    }
    if(_ScopeStk[i].Scope.Kind==ScopeKind::Public){ break; }
  }

  //When last public scope is searched and variable is not found we do a last search on main public scope for system variables
  //but only if type is searched as belonging to current module
  if(VarIndex==-1){
    if(CurrentScope().ModIndex==ModIndex){
      Index=_ScopeStk[0].Var.Search(Modules[MainModIndex()].Name+"."+Name);
      if(Index!=-1){ 
        if(Variables[_ScopeStk[0].Var[Index].Pos].IsSystemDef){ VarIndex=_ScopeStk[0].Var[Index].Pos; }
      }
    }
  }

  //Return search index
  if(Index!=-1 && SearchIndex!=nullptr){ *SearchIndex=Index; }

  //Return result
  if(FindHidden){
    return VarIndex;
  }
  else{
    return VarIndex!=-1 && !Variables[VarIndex].IsHidden?VarIndex:-1;
  }

}

//Field search
int MasterData::FldSearch(const String& Name,int TypIndex) const {
  int TrueTypIndex=TypIndex;
  while(Types[TrueTypIndex].OrigTypIndex!=-1){ TrueTypIndex=Types[TrueTypIndex].OrigTypIndex; }
  if(Types[TrueTypIndex].FieldLow!=-1 && Types[TrueTypIndex].FieldHigh!=-1){
    for(int i=Types[TrueTypIndex].FieldLow;i<=Types[TrueTypIndex].FieldHigh;i++){
      if(Fields[i].SubScope.TypIndex==TrueTypIndex && Fields[i].Name==Name){ return i; }
    }
  }
  return -1;
}

//Parameter search
int MasterData::ParSearch(const String& Name,int FunIndex) const {
  if(Functions[FunIndex].ParmLow!=-1 && Functions[FunIndex].ParmHigh!=-1){
    for(int i=Functions[FunIndex].ParmLow;i<=Functions[FunIndex].ParmHigh;i++){
      if(Parameters[i].Name==Name){ return i; }
    }
  }
  return -1;
}

//Internal function search
int MasterData::_FunSearch(const String& SearchName,const String& Name,const String& Parms,const String& ConvParms,String *Matches,bool ByName) const {
  
  //Variables
  int i;
  int Index;
  int FoundIndex;
  int MatchCount;
  String NameMatches;

  //Function search tracking messages
  #ifdef __DEV__
  String SearchTry;
  String FoundName;
  ScopeDef FoundScope;
  if(FUNCTION_SEARCH_TRACKING){
    DebugAppend(DebugLevel::CmpIndex,"FunSearch(sname="+SearchName+" name="+Name+" parms=("+Parms+") cparms=("+ConvParms+") byname="+(ByName?"yes":"no")+") --> ");
  }
  #endif
  
  //Init matched functions
  if(Matches!=nullptr){ *Matches=""; };

  //Return not found if scope stack is empty
  if(_ScopeStk.Length()==0){ 
    #ifdef __DEV__
    if(FUNCTION_SEARCH_TRACKING){
      DebugMessage(DebugLevel::CmpIndex,"empty scope");
    }
    #endif
    return -1; 
  }

  //Search first on normal index
  FoundIndex=-1;
  for(i=_ScopeStk.Length()-1;i>=0;i--){
    if(ByName){
      Index=_ScopeStk[i].Fun.Search(SearchName+"(",ByName);
      if(Index!=-1 && Index<_ScopeStk[i].Fun.Length()){ Index=(Functions[_ScopeStk[i].Fun[Index].Pos].Name==Name?Index:-1); } else{ Index=-1; }
    }
    else{
      Index=_ScopeStk[i].Fun.Search(SearchName+"("+Parms+")",ByName);
    }
    if(Index!=-1){ 
      FoundIndex=_ScopeStk[i].Fun[Index].Pos; 
      #ifdef __DEV__
      SearchTry="Index";
      FoundName=_ScopeStk[i].Fun[Index].Name;
      FoundScope=_ScopeStk[i].Scope;
      #endif
      break; 
    }
    if(_ScopeStk[i].Scope.Kind==ScopeKind::Public){ break; }
  }

  //When last public scope is searched and function is not found we do a last search on main public scope for master methods
  if(FoundIndex==-1){
    if(ByName){
      Index=_ScopeStk[0].Fun.Search(SearchName+"(",ByName);
      if(Index!=-1 && Index<_ScopeStk[0].Fun.Length()){ Index=(Functions[_ScopeStk[0].Fun[Index].Pos].Name==Name?Index:-1); } else{ Index=-1; }
    }
    else{
      Index=_ScopeStk[0].Fun.Search(SearchName+"("+Parms+")",ByName);
    }
    if(Index!=-1 && Functions[_ScopeStk[0].Fun[Index].Pos].Kind==FunctionKind::MasterMth){ 
      FoundIndex=_ScopeStk[0].Fun[Index].Pos; 
      #ifdef __DEV__
      SearchTry="Master";
      FoundName=_ScopeStk[0].Fun[Index].Name; 
      FoundScope=_ScopeStk[0].Scope;
      #endif
    }
  }

  //Return found index
  if(FoundIndex!=-1){ 
    #ifdef __DEV__
    if(FUNCTION_SEARCH_TRACKING){
      DebugMessage(DebugLevel::CmpIndex,"["+SearchTry+":"+ScopeName(FoundScope)+":"+ToString(FoundIndex)+":"+FoundName+"] name="+Functions[FoundIndex].Name+" kind="+FunctionKindName(Functions[FoundIndex].Kind)+" scope="+ScopeName(Functions[FoundIndex].Scope));
    }
    #endif
    return FoundIndex; 
  }
  
  //If we search by name or without converted parameter list we end here
  if(ByName || ConvParms.Length()==0){ 
    #ifdef __DEV__
    if(FUNCTION_SEARCH_TRACKING){
      DebugMessage(DebugLevel::CmpIndex,"null");
    }
    #endif
    return -1; 
  }
  
  //Search again on convertible index
  FoundIndex=-1;
  for(i=_ScopeStk.Length()-1;i>=0;i--){
    Index=_ScopeStk[i].Fnc.Search(SearchName+"("+ConvParms+")");
    if(Index!=-1){ 
      FoundIndex=_ScopeStk[i].Fnc[Index].Pos; 
      _FunNameMatches(i,Index,MatchCount,NameMatches); 
      #ifdef __DEV__
      SearchTry="cIndex";
      FoundName=_ScopeStk[i].Fnc[Index].Name; 
      FoundScope=_ScopeStk[i].Scope;
      #endif
      break; 
    }
    if(_ScopeStk[i].Scope.Kind==ScopeKind::Public){ break; }
  }

  //When last public scope is searched and function is not found we do a last search on main public scope for master methods
  if(FoundIndex==-1){ 
    Index=_ScopeStk[0].Fnc.Search(SearchName+"("+ConvParms+")");
    if(Index!=-1 && Functions[_ScopeStk[0].Fun[Index].Pos].Kind==FunctionKind::MasterMth){ 
      FoundIndex=_ScopeStk[0].Fnc[Index].Pos; 
      _FunNameMatches(i,Index,MatchCount,NameMatches); 
      #ifdef __DEV__
      SearchTry="cMaster";
      FoundName=_ScopeStk[0].Fnc[Index].Name; 
      FoundScope=_ScopeStk[0].Scope;
      #endif
    }
  }

  //Return success only if single entry is found
  if(FoundIndex!=-1 && MatchCount==1){ 
    #ifdef __DEV__
    if(FUNCTION_SEARCH_TRACKING){
      DebugMessage(DebugLevel::CmpIndex,"["+SearchTry+":"+ScopeName(FoundScope)+":"+ToString(FoundIndex)+":"+FoundName+"] name="+Functions[FoundIndex].Name+" kind="+FunctionKindName(Functions[FoundIndex].Kind)+" scope="+ScopeName(Functions[FoundIndex].Scope));
    }
    #endif
    return FoundIndex; 
  }

  //Return not found
  if(Matches!=nullptr){ *Matches=NameMatches; }
  #ifdef __DEV__
  if(FUNCTION_SEARCH_TRACKING){
    DebugMessage(DebugLevel::CmpIndex,"null");
  }
  #endif
  return -1;

}

//Calculate function matches on convertible index 
void MasterData::_FunNameMatches(int ScopeIndex,int FncIndex,int& MatchCount,String& NameMatches) const {
  
  //Variables
  int i;
  int UpperMatch;

  //Init outputs
  MatchCount=0;
  NameMatches="";

  //Calculate name maches for given convertible index number
  for(i=FncIndex;i>=0;i--){ if(_ScopeStk[ScopeIndex].Fnc[i].Name==_ScopeStk[ScopeIndex].Fnc[FncIndex].Name){ UpperMatch=i; } else{ break; } }
  for(i=UpperMatch;i<_ScopeStk[ScopeIndex].Fnc.Length();i++){
    if(_ScopeStk[ScopeIndex].Fnc[i].Name==_ScopeStk[ScopeIndex].Fnc[FncIndex].Name){
      if(NameMatches.Search(Functions[_ScopeStk[ScopeIndex].Fnc[FncIndex].Pos].FullName)==-1){ 
        NameMatches+=(NameMatches.Length()!=0?", ":"")+Functions[_ScopeStk[ScopeIndex].Fnc[FncIndex].Pos].FullName;
        MatchCount++;
      }
    }
    else{
      break;
    }
  }

}

//Generic search
int MasterData::GfnSearch(const String& SearchName,const String& Parms) const {
  String Matches;
  return _FunSearch(SearchName,"",Parms,"",&Matches,false);
}

//Function search
int MasterData::FunSearch(const String& Name,int ModIndex,const String& Parms,const String& ConvParms,String *Matches,bool ByName) const {
  return _FunSearch(Modules[ModIndex].Name+"."+Name,Name,Parms,ConvParms,Matches,ByName);
}

//Master method search
int MasterData::MmtSearch(const String& Name,MasterType MstType,const String& Parms,const String& ConvParms,String *Matches,bool ByName) const {
  return _FunSearch(MasterTypeName(MstType)+"."+Name,Name,Parms,ConvParms,Matches,ByName);
}

//Function member search
int MasterData::FmbSearch(const String& Name,int ModIndex,int ClassTypIndex,const String& Parms,const String& ConvParms,String *Matches,bool ByName) const {
  int TrueTypIndex=ClassTypIndex;
  while(Types[TrueTypIndex].OrigTypIndex!=-1){ TrueTypIndex=Types[TrueTypIndex].OrigTypIndex; }
  return _FunSearch(Modules[ModIndex].Name+"."+Types[TrueTypIndex].Name+"."+Name,Name,Parms,ConvParms,Matches,ByName);
}

//Operator search
int MasterData::OprSearch(const String& Name,const String& Parms,const String& ConvParms,String *Matches,bool ByName) const {
  return _FunSearch(Name,Name,Parms,ConvParms,Matches,ByName);
}

//Grant search
int MasterData::GraSearch(GrantKind Kind,int TypIndex,const String& FrTypName,const String& FrFunName,int ToFldIndex,int ToFunIndex) const {
  
  //Variables
  int Index;
  int FoundIndex;
  
  //Function search tracking messages
  #ifdef __DEV__
  String FoundName;
  ScopeDef FoundScope;
  if(GRANT_SEARCH_TRACKING){
    DebugAppend(DebugLevel::CmpIndex,"GraSearch(name="+GrantName(Kind,TypIndex,FrTypName,FrFunName,ToFldIndex,ToFunIndex)+") --> ");
  }
  #endif

  //Exit if no scopes are defined
  if(_ScopeStk.Length()==0){ 
    #ifdef __DEV__
    if(GRANT_SEARCH_TRACKING){
      DebugMessage(DebugLevel::CmpIndex,"empty scope");
    }
    #endif
    return -1; 
  }
  
  //Scope stack search
  FoundIndex=-1;
  for(int i=_ScopeStk.Length()-1;i>=0;i--){
    Index=_ScopeStk[i].Gra.Search(GrantName(Kind,TypIndex,FrTypName,FrFunName,ToFldIndex,ToFunIndex));
    if(Index!=-1){ 
      FoundIndex=_ScopeStk[i].Gra[Index].Pos; 
      #ifdef __DEV__
      FoundName=_ScopeStk[i].Gra[Index].Name; 
      FoundScope=_ScopeStk[i].Scope;
      #endif
      break; 
    }
    if(_ScopeStk[i].Scope.Kind==ScopeKind::Public){ break; }
  }

  //Debug message for result
  #ifdef __DEV__
  if(GRANT_SEARCH_TRACKING){
    if(FoundIndex!=-1){
      DebugMessage(DebugLevel::CmpIndex,"["+ScopeName(FoundScope)+":"+ToString(FoundIndex)+"] name="+FoundName);
    }
    else{
      DebugMessage(DebugLevel::CmpIndex,"null");
    }
  }
  #endif

  //Return index
  return FoundIndex;

}

//Get type identifiers
//A sorted array is used in the algorithm to return types with longest names first
//It happens that there are types that can start by the same chars and are not correctly identified in parser if short types happen first in the returned list
String MasterData::GetTypeIdentifiers(const ScopeDef& Scope) const {
  
  //Seach index
  struct TypeId{
    String Name;
    String SortKey() const { return ToString(_MaxIdLen-Name.Length(),"%02i")+Name; }
    TypeId(){}
    TypeId(const String& TypeName){ Name=TypeName; }
  };
    
  //Variables
  int i,j,k;
  int Index;
  int MinTraversedScope;
  String TypeName;
  SortedArray<TypeId,String> TypeIds;
  String TypeList;
  
  //Get list of defined types in scope
  MinTraversedScope=-1;
  for(i=_ScopeStk.Length()-1;i>=0;i--){
    for(j=0;j<_ScopeStk[i].Typ.Length();j++){
      Index=_ScopeStk[i].Typ[j].Pos;
      if(Types[Index].Scope.ModIndex==Scope.ModIndex
      && _ScopeStk[i].SubScope.Kind!=SubScopeKind::None
      && Types[Index].Name.StartsWith(Types[_ScopeStk[i].SubScope.TypIndex].Name+".")
      && Types[Index].Name.Length()>Types[_ScopeStk[i].SubScope.TypIndex].Name.Length()+1){
        TypeName=Types[Index].Name.Right(Types[Index].Name.Length()-Types[_ScopeStk[i].SubScope.TypIndex].Name.Length()-1);
      }
      else if(_ScopeStk[i].Scope.Kind==ScopeKind::Public){ 
        if(Types[Index].Scope.ModIndex==Scope.ModIndex){
          TypeName=Types[Index].Name;
        }
        else{
          for(k=0;k<_ScopeStk[i].Trk.Length();k++){
            if(Trackers[k].ModIndex==Types[Index].Scope.ModIndex){
              TypeName=Trackers[k].Name+"."+Types[Index].Name;
            }
          }
        }
      }
      else{
        TypeName=Types[Index].Name;
      }
      if(!TypeName.Contains("[")){
        TypeIds.Add(TypeName);
      }
    }
    if(MinTraversedScope==-1 || i<MinTraversedScope){ MinTraversedScope=i; }
    if(_ScopeStk[i].Scope.Kind==ScopeKind::Public){ break; }
  }

  //Add also system types which are visible from all scopes (only if not added already, since they are in scope 0)
  if(MinTraversedScope!=0){
    for(j=0;j<_ScopeStk[0].Typ.Length();j++){
      Index=_ScopeStk[0].Typ[j].Pos;
      if(Types[Index].IsSystemDef && !Types[Index].Name.Contains("[")){
        TypeIds.Add(Types[Index].Name);
      }
    }
  }
  
  //Convert sorted array into string
  TypeList="";
  for(i=0;i<TypeIds.Length();i++){
    TypeList+=(TypeList.Length()!=0?String(_TypeIdSeparator):"")+TypeIds[i].Name;
  }

  //Return type list (array types are ignored - since it is ReadTypeSpec() the function that parses array specification)
  return TypeList;

}

//Copy public indexes to parent scope
bool MasterData::CopyPublicIndexes(int ScopeIndex){
  
  //Variables
  int i;
  int ParentScope;

  //Copy only if there is an upper module scope
  if(_ScopeStk.Length()<=1){
    SysMessage(197).Print();
    return false;
  }

  //Check Scope index is valid
  if(ScopeIndex-1<0 || ScopeIndex-1>_ScopeStk.Length()-1){
    SysMessage(198).Print(ToString(ScopeIndex),ToString(_ScopeStk.Length()));
    return false;
  }

  //Copy only if there is an upper module public scope
  ParentScope=-1;
  for(i=ScopeIndex-1;i>=0;i--){
    if(_ScopeStk[i].Scope.Kind==ScopeKind::Public){ ParentScope=i; break; }
  } 
  if(ParentScope==-1){ 
    SysMessage(199).Print(ToString(ScopeIndex),ToString(_ScopeStk.Length()));
    return false; 
  }

  //Copy module names
  for(i=0;i<_ScopeStk[ScopeIndex].Mod.Length();i++){
    _ScopeStk[ParentScope].Mod.Add(_ScopeStk[ScopeIndex].Mod[i]);
  }

  //Copy data types
  for(i=0;i<_ScopeStk[ScopeIndex].Typ.Length();i++){
    _ScopeStk[ParentScope].Typ.Add(_ScopeStk[ScopeIndex].Typ[i]);
  }

  //Copy variables
  for(i=0;i<_ScopeStk[ScopeIndex].Var.Length();i++){
    _ScopeStk[ParentScope].Var.Add(_ScopeStk[ScopeIndex].Var[i]);
  }

  //Copy functions
  for(i=0;i<_ScopeStk[ScopeIndex].Fun.Length();i++){
    _ScopeStk[ParentScope].Fun.Add(_ScopeStk[ScopeIndex].Fun[i]);
  }

  //Return code
  return true;

}

//Purge scope objects in master data tables
void MasterData::PurgeScopeObjects(const ScopeDef& Scope){
  
  //Variables
  int i;

  //Purge grants defined on types
  do{
    i=Grants.Length()-1;
    if(i>=0 && Types[Grants[i].TypIndex].Scope==Scope){
      DebugMessage(DebugLevel::CmpMasterData,"Purge GRA["+ToString(i)+"]: scope="+ScopeName(Scope)+", name="+
      GrantName(Grants[i].Kind,Grants[i].TypIndex,Grants[i].FrTypName,Grants[i].FrFunName,Grants[i].ToFldIndex,Grants[i].ToFunIndex));
      Grants.Resize(i);
    }
    else{ 
      break; 
    }
  }while(true);
  
  //Purge fields of types
  do{
    i=Fields.Length()-1;
    if(i>=0 && Types[Fields[i].SubScope.TypIndex].Scope==Scope){
      DebugMessage(DebugLevel::CmpMasterData,"Purge FLD["+ToString(i)+"]: scope="+ScopeName(Scope)+", name="+Fields[i].Name+", type="+CannonicalTypeName(Fields[i].TypIndex));
      Fields.Resize(i);
    }
    else{
      break; 
    }
  }while(true);
  
  //Purge dimensions of types
  do{
    i=Dimensions.Length()-1;
    if(i>=0 && Types[Dimensions[i].TypIndex].Scope==Scope){
      String DimString="";
      for(int j=0;j<_MaxArrayDims;j++){ DimString+=(DimString.Length()!=0?",":"")+ToString(Dimensions[i].DimSize.n[j]); }
      DebugMessage(DebugLevel::CmpMasterData,"Purge DIM["+ToString(i)+"]: scope="+ScopeName(Scope)+", geomindex="+ToString(Dimensions[i].GeomIndex)+" dimsizes=("+DimString+")");
      Dimensions.Resize(i);
    }
    else{
      break;
    }
  }while(true);
  
  //Purge parameters of functions
  do{
    i=Parameters.Length()-1;
    if(i>=0
    && ((Functions[Parameters[i].FunIndex].Kind!=FunctionKind::Member && Functions[Parameters[i].FunIndex].Scope==Scope)
    ||  (Functions[Parameters[i].FunIndex].Kind==FunctionKind::Member && Types[Functions[Parameters[i].FunIndex].SubScope.TypIndex].Scope==Scope))){
      DebugMessage(DebugLevel::CmpMasterData,"Purge PAR["+ToString(i)+"]: scope="+ScopeName(Scope)+", function="+Functions[Parameters[i].FunIndex].Name+" name="+Parameters[i].Name);
      Parameters.Resize(i);
    }
    else{
      break;
    }
  }while(true);
  
  //Purge functions
  do{
    i=Functions.Length()-1;
    if(i>=0 
    && ((Functions[i].Kind!=FunctionKind::Member && Functions[i].Scope==Scope)
    ||  (Functions[i].Kind==FunctionKind::Member && Types[Functions[i].SubScope.TypIndex].Scope==Scope))){
      DebugMessage(DebugLevel::CmpMasterData,"Purge FUN["+ToString(i)+"]: scope="+ScopeName(Scope)+", function="+Functions[i].Name+" kind="+FunctionKindName(Functions[i].Kind));
      Functions.Resize(i);
    }
    else{
      break;
    }
  }while(true);
  
  //Purge variables
  do{
    i=Variables.Length()-1;
    if(i>=0 && Variables[i].Scope==Scope && ((Scope.Kind==ScopeKind::Local && (!Variables[i].IsConst || (Variables[i].IsConst && Variables[i].IsReference)) && !Variables[i].IsStatic) || Scope.Kind!=ScopeKind::Local)){
      DebugMessage(DebugLevel::CmpMasterData,"Purge VAR["+ToString(i)+"]: scope="+ScopeName(Scope)+", name="+Variables[i].Name+" type="+(Variables[i].TypIndex!=-1?CannonicalTypeName(Variables[i].TypIndex):"null"));
      Variables.Resize(i);
    }
    else{
      break;
    }
  }while(true);

  //Purge types
  do{
    i=Types.Length()-1;
    if(i>=0 && Types[i].Scope==Scope){
      DebugMessage(DebugLevel::CmpMasterData,"Purge TYP["+ToString(i)+"]: scope="+ScopeName(Scope)+", name="+CannonicalTypeName(i));
      Types.Resize(i);
    }
    else{
      break;
    }
  }while(true);
  
  //Purge module trackers
  if(Scope.Kind==ScopeKind::Public){
    do{
      i=Trackers.Length()-1;
      if(i>=0 && Trackers[i].ModIndex==Scope.ModIndex){
        DebugMessage(DebugLevel::CmpMasterData,"Purge TRK["+ToString(i)+"]: scope="+ScopeName(Scope)+", name="+Trackers[i].Name+" module="+Modules[Trackers[i].ModIndex].Name);
        Trackers.Resize(i);
      }
      else{
        break;
      }
    }while(true);
  }

}

//Unused variable warnings for scope
void MasterData::CheckUnusedScopeObjects(const ScopeDef& Scope,const SourceInfo& SrcInfo,CpuLon CodeBlockId,CpuLon FlowLabel){
  
  //Variables
  int i;
  bool Report;
  String VarType;
  String ScopeMsgName;

  //Warnings for unused wariables
  for(i=Variables.Length()-1;i>=0;i--){
    
    //Filter variables by scope
    if(Variables[i].Scope==Scope){

      //Check unused variables
      if(!Variables[i].IsSourceUsed 
      && !Variables[i].Name.StartsWith(SYSTEM_NAMESPACE) 
      && Variables[i].Name!=GetSelfRefName()
      && Variables[i].IsHidden==false
      && (CodeBlockId==0 || (CodeBlockId!=0 && Variables[i].CodeBlockId==CodeBlockId))
      && (FlowLabel==-1 || (FlowLabel!=-1 && Variables[i].FlowLabel==FlowLabel))){
        
        //Init report flag
        Report=false;

        //Switch on scope
        switch(Scope.Kind){
          case ScopeKind::Local:   
            Report=true;
            ScopeMsgName=Modules[Scope.ModIndex].Name+"."+Functions[Scope.FunIndex].Name+"()";
            VarType=(Variables[i].IsParameter?"Parameter":"Variable");
            break;
          case ScopeKind::Private:
            Report=true;
            ScopeMsgName="module "+Modules[Scope.ModIndex].Name;
            VarType="Variable";
            break;
          case ScopeKind::Public:
            Report=(Scope.ModIndex==MainModIndex() && !Modules[Variables[i].Scope.ModIndex].IsModLibrary?true:false);
            ScopeMsgName="module "+Modules[Scope.ModIndex].Name;
            VarType="Variable";
            break;
        }

        //Add code block name to scope name
        if(CodeBlockId!=0 && Variables[i].CodeBlockId==CodeBlockId){
          ScopeMsgName=" block {"+CodeBlockIdName(CodeBlockId).Lower()+"} inside "+ScopeMsgName;
        }
        
        //Add flow label reference to scope name
        if(FlowLabel!=-1 && Variables[i].FlowLabel==FlowLabel){
          ScopeMsgName=" for() / array() operator inside "+ScopeMsgName;
        }
        
        //Report message
        if(Report){
          if(Variables[i].LineNr!=-1 && Variables[i].ColNr!=-1 && Variables[i].SourceLine.Length()!=0){
            SysMessage(132,SrcInfo.FileName,Variables[i].LineNr,Variables[i].ColNr,Variables[i].SourceLine).Print(VarType,Variables[i].Name,ScopeMsgName);
          }
          else{
            SrcInfo.Msg(132).Print(VarType,Variables[i].Name,ScopeMsgName);
          }
        }

      }

    }

    //Exit if another scope is reached (must ignore destroyed variables)
    else{
      break;
    }

  }

}

//Check declared functions but not defined
void MasterData::CheckUndefinedFunctions(const ScopeDef& Scope,const SourceInfo& SrcInfo){
  
  //Variables
  int i;
  bool Report;
  String FuncName;
  String ScopeMsgName;

  //Warnings for undefined functions
  for(i=0;i<Functions.Length();i++){
    
    //Filter variables by scope
    if(Functions[i].Scope==Scope){

      //Check unused variables
      if(!Functions[i].IsDefined){
        
        //Init report flag
        Report=false;

        //Switch on scope
        switch(Scope.Kind){
          case ScopeKind::Local:   
            Report=true;
            ScopeMsgName=Modules[Scope.ModIndex].Name+"."+Functions[Scope.FunIndex].Name+"()";
            break;
          case ScopeKind::Private:
            Report=true;
            ScopeMsgName="module "+Modules[Scope.ModIndex].Name;
            break;
          case ScopeKind::Public:
            Report=(Scope.ModIndex==MainModIndex() && !Modules[Functions[i].Scope.ModIndex].IsModLibrary?true:false);
            ScopeMsgName="module "+Modules[Scope.ModIndex].Name;
            break;
        }

        //Get function name and skip report for functions that are never defined
        switch(Functions[i].Kind){
          case FunctionKind::Function:  FuncName="Function "+FunctionSearchName(i); break;
          case FunctionKind::MasterMth: FuncName="Master method "+FunctionSearchName(i); break;
          case FunctionKind::Member:    FuncName="Member function "+FunctionSearchName(i); break;
          case FunctionKind::Operator:  FuncName="Operator "+FunctionSearchName(i); break;
          case FunctionKind::SysCall:   FuncName=""; Report=false; break;
          case FunctionKind::SysFunc:   FuncName=""; Report=false; break;
          case FunctionKind::DlFunc:    FuncName=""; Report=false; break;
        }

        //Report message
        if(Report){
          if(Functions[i].LineNr!=-1 && Functions[i].ColNr!=-1 && Functions[i].SourceLine.Length()!=0){
            SysMessage(320,SrcInfo.FileName,Functions[i].LineNr,Functions[i].ColNr,Functions[i].SourceLine).Print(FuncName,ScopeMsgName);
          }
          else{
            SrcInfo.Msg(320).Print(FuncName,ScopeMsgName);
          }
        }

      }

    }

  }

}

//Grant from name
String MasterData::GrantFrName(GrantKind Kind,int TypIndex,const String& FrTypName,const String& FrFunName) const {
  
  //Variables
  String FrName;

  //From name
  switch(Kind){
    case GrantKind::ClassToClass: 
    case GrantKind::ClassToField: 
    case GrantKind::ClassToFunMb: 
      FrName=Modules[Types[TypIndex].Scope.ModIndex].Name+"."+FrTypName; 
      break;
    case GrantKind::FunctToClass: 
    case GrantKind::FunctToField: 
    case GrantKind::FunctToFunMb: 
      FrName=Modules[Types[TypIndex].Scope.ModIndex].Name+"."+FrFunName+"()"; 
      break;
    case GrantKind::FunMbToClass: 
    case GrantKind::FunMbToField: 
    case GrantKind::FunMbToFunMb: 
      FrName=Modules[Types[TypIndex].Scope.ModIndex].Name+"."+FrTypName+"."+FrFunName+"()"; 
      break;
    case GrantKind::OpertToClass: 
    case GrantKind::OpertToField: 
    case GrantKind::OpertToFunMb: 
      FrName=FrFunName+"()"; 
      break;
  }

  //Return name
  return FrName;

}

//Grant to name
String MasterData::GrantToName(GrantKind Kind,int TypIndex,int ToFldIndex,int ToFunIndex) const {
  
  //Variables
  String ToName;

  //To name
  switch(Kind){
    case GrantKind::ClassToClass: 
    case GrantKind::FunctToClass: 
    case GrantKind::FunMbToClass: 
    case GrantKind::OpertToClass: 
      ToName=Modules[Types[TypIndex].Scope.ModIndex].Name+"."+Types[TypIndex].Name; 
      break;
    case GrantKind::ClassToField: 
    case GrantKind::FunctToField: 
    case GrantKind::FunMbToField: 
    case GrantKind::OpertToField: 
      ToName=Modules[Types[TypIndex].Scope.ModIndex].Name+"."+Types[TypIndex].Name+"."+Fields[ToFldIndex].Name; 
      break;
    case GrantKind::ClassToFunMb: 
    case GrantKind::FunctToFunMb: 
    case GrantKind::FunMbToFunMb: 
    case GrantKind::OpertToFunMb: 
      ToName=Modules[Types[TypIndex].Scope.ModIndex].Name+"."+Types[TypIndex].Name+"."+Functions[ToFunIndex].Name+"()"; 
      break;
  }

  //Return name
  return ToName;

}

//Grant name
String MasterData::GrantName(GrantKind Kind,int TypIndex,const String& FrTypName,const String& FrFunName,int ToFldIndex,int ToFunIndex) const {
  String FrName=GrantFrName(Kind,TypIndex,FrTypName,FrFunName);
  String ToName=GrantToName(Kind,TypIndex,ToFldIndex,ToFunIndex);
  return GrantKindShortName(Kind)+":"+FrName+"->"+ToName;
}

//Grant name
String MasterData::GrantName(int GraIndex) const {
  String FrName=GrantFrName(Grants[GraIndex].Kind,Grants[GraIndex].TypIndex,Grants[GraIndex].FrTypName,Grants[GraIndex].FrFunName);
  String ToName=GrantToName(Grants[GraIndex].Kind,Grants[GraIndex].TypIndex,Grants[GraIndex].ToFldIndex,Grants[GraIndex].ToFunIndex);
  return GrantKindShortName(Grants[GraIndex].Kind)+":"+FrName+"->"+ToName;
}

//Check grant definitions on scope close (only from name, since to name is checked on definition)
bool MasterData::CheckGrants(const ScopeDef& Scope){

  //Variables
  int i;
  int TypIndex;

  //Loop on grants of given scope
  for(i=Grants.Length()-1;i>=0;i--){
    
    //Take only grants on given scope
    if(Types[Grants[i].TypIndex].Scope==Scope){
      
      //Switch on grant kind
      switch(Grants[i].Kind){
        
        //Check class
        case GrantKind::ClassToClass:
        case GrantKind::ClassToField:
        case GrantKind::ClassToFunMb:
          if(TypSearch(Grants[i].FrTypName,Types[Grants[i].TypIndex].Scope.ModIndex)==-1){
            SysMessage(209,Modules[Scope.ModIndex].Name,Grants[i].FrLineNr,Grants[i].FrColNr).Print(Grants[i].FrTypName);
            return false;
          }
          break;

        //Check function
        case GrantKind::FunctToClass:
        case GrantKind::FunctToField:
        case GrantKind::FunctToFunMb:
          if(FunSearch(Grants[i].FrFunName,Types[Grants[i].TypIndex].Scope.ModIndex,"","",nullptr,true)==-1){
            SysMessage(210,Modules[Scope.ModIndex].Name,Grants[i].FrLineNr,Grants[i].FrColNr).Print(Grants[i].FrFunName);
            return false;
          }
          break;
        
        //Check function member
        case GrantKind::FunMbToClass:
        case GrantKind::FunMbToField:
        case GrantKind::FunMbToFunMb:
          if((TypIndex=TypSearch(Grants[i].FrTypName,Types[Grants[i].TypIndex].Scope.ModIndex))==-1){
            SysMessage(209,Modules[Scope.ModIndex].Name,Grants[i].FrLineNr,Grants[i].FrColNr).Print(Grants[i].FrTypName);
            return false;
          }
          if(FmbSearch(Grants[i].FrFunName,Types[Grants[i].TypIndex].Scope.ModIndex,TypIndex,"","",nullptr,true)==-1){
            SysMessage(211,Modules[Scope.ModIndex].Name,Grants[i].FrLineNr,Grants[i].FrColNr).Print(Grants[i].FrFunName,Grants[i].FrTypName);
            return false;
          }
          break;
        
        //Check operator function
        case GrantKind::OpertToClass:
        case GrantKind::OpertToField:
        case GrantKind::OpertToFunMb:
          if(OprSearch(Grants[i].FrFunName,"","",nullptr,true)==-1){
            SysMessage(214,Modules[Scope.ModIndex].Name,Grants[i].FrLineNr,Grants[i].FrColNr).Print(Grants[i].FrFunName);
            return false;
          }
          break;

      }
    }

    //Exit when another scope is reached
    else{
      break;
    }
  
  }
  
  //Return code
  return true;

}

//Create parameter variables for function scope
void MasterData::CreateParmVariables(const ScopeDef& Scope,CpuLon CodeBlockId){
  
  //Check function has parameters
  if(Functions[Scope.FunIndex].ParmNr==0){ return; }
  
  //Get parameer table indexes
  int ParmLow=Functions[Scope.FunIndex].ParmLow;
  int ParmHigh=Functions[Scope.FunIndex].ParmHigh;
  
  //Assembler comment
  if(!Functions[Scope.FunIndex].IsVoid || Functions[Scope.FunIndex].Kind==FunctionKind::Member || Functions[Scope.FunIndex].ParmNr!=0){
    Bin.AsmOutCommentLine(AsmSection::Decl,"Parameters",true);
  }
  
  //Function result
  if(!Functions[Scope.FunIndex].IsVoid){
    StoreVariable(Scope,CodeBlockId,-1,GetFuncResultName(),Functions[Scope.FunIndex].TypIndex,false,false,true,true,false,false,false,(TypLength(Functions[Scope.FunIndex].TypIndex)!=0?true:false),false,SourceInfo(),"");
    Variables[Variables.Length()-1].IsInitialized=true;
    DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+Variables[Variables.Length()-1].Name+" in scope "+ScopeName(Variables[Variables.Length()-1].Scope));
    Parameters[ParmLow].Address=Variables[Variables.Length()-1].Address;
    DebugMessage(DebugLevel::CmpMasterData,"PAR["+ToString(ParmLow)+"]: name="+Parameters[ParmLow].Name+" type="+CannonicalTypeName(Parameters[ParmLow].TypIndex)+" address="+HEXFORMAT(Parameters[ParmLow].Address));
    ParmLow++;
    Bin.AsmOutVarDecl(AsmSection::Decl,false,true,true,false,false,GetFuncResultName(),CpuDataTypeFromMstType(Types[Functions[Scope.FunIndex].TypIndex].MstType),
    VarLength(Variables.Length()-1),Variables[Variables.Length()-1].Address,"",false,false,"","");
  }
  
  //Reference to self
  if(Functions[Scope.FunIndex].Kind==FunctionKind::Member){
    StoreVariable(Scope,CodeBlockId,-1,GetSelfRefName(),Functions[Scope.FunIndex].SubScope.TypIndex,false,false,true,true,false,false,false,(TypLength(Functions[Scope.FunIndex].SubScope.TypIndex)!=0?true:false),false,SourceInfo(),"");
    Variables[Variables.Length()-1].IsInitialized=true;
    DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+Variables[Variables.Length()-1].Name+" in scope "+ScopeName(Variables[Variables.Length()-1].Scope));
    Parameters[ParmLow].Address=Variables[Variables.Length()-1].Address;
    DebugMessage(DebugLevel::CmpMasterData,"PAR["+ToString(ParmLow)+"]: name="+Parameters[ParmLow].Name+" type="+CannonicalTypeName(Parameters[ParmLow].TypIndex)+" address="+HEXFORMAT(Parameters[ParmLow].Address));
    ParmLow++;
    Bin.AsmOutVarDecl(AsmSection::Decl,false,true,true,false,false,GetSelfRefName(),CpuDataTypeFromMstType(Types[Functions[Scope.FunIndex].SubScope.TypIndex].MstType),
    VarLength(Variables.Length()-1),Variables[Variables.Length()-1].Address,"",false,false,"","");
  }

  //Rest of parameters
  for(int i=ParmLow;i<=ParmHigh;i++){
    SourceInfo SrcInfo=SourceInfo(Modules[Functions[Parameters[i].FunIndex].Scope.ModIndex].Path,Parameters[i].LineNr,Parameters[i].ColNr);
    StoreVariable(Scope,CodeBlockId,-1,Parameters[i].Name,Parameters[i].TypIndex,false,Parameters[i].IsConst,true,Parameters[i].IsReference,false,false,false,(TypLength(Parameters[i].TypIndex)!=0?true:false),false,SrcInfo,Parameters[i].SourceLine);
    Variables[Variables.Length()-1].IsInitialized=true;
    DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+Variables[Variables.Length()-1].Name+" in scope "+ScopeName(Variables[Variables.Length()-1].Scope));
    Parameters[i].Address=Variables[Variables.Length()-1].Address;
    DebugMessage(DebugLevel::CmpMasterData,"PAR["+ToString(i)+"]: name="+Parameters[i].Name+" type="+CannonicalTypeName(Parameters[i].TypIndex)+" address="+HEXFORMAT(Parameters[i].Address));
    Bin.AsmOutVarDecl(AsmSection::Decl,false,true,Parameters[i].IsReference,Parameters[i].IsConst,false,Parameters[i].Name,CpuDataTypeFromMstType(Types[Parameters[i].TypIndex].MstType),
    VarLength(Variables.Length()-1),Variables[Variables.Length()-1].Address,"",false,false,"","");
  }

  //Assembler blank line
  if(!Functions[Scope.FunIndex].IsVoid || Functions[Scope.FunIndex].Kind==FunctionKind::Member || Functions[Scope.FunIndex].ParmNr!=0){
    Bin.AsmOutNewLine(AsmSection::Decl);
  }

}

//Calculate number of array elements for fixed arrays
CpuWrd MasterData::ArrayElements(int DimNr,ArrayIndexes& DimSize) const {

  //Variables
  CpuWrd Elements;

  //Switch on array number of dimensions
  switch(DimNr){
    case 1: Elements=DimSize.n[0]; break;
    case 2: Elements=DimSize.n[0]*DimSize.n[1]; break;
    case 3: Elements=DimSize.n[0]*DimSize.n[1]*DimSize.n[2]; break;
    case 4: Elements=DimSize.n[0]*DimSize.n[1]*DimSize.n[2]*DimSize.n[3]; break;
    case 5: Elements=DimSize.n[0]*DimSize.n[1]*DimSize.n[2]*DimSize.n[3]*DimSize.n[4]; break;
  }  

  //Return result
  return Elements;
 
}

//Determines if two arrays are equivalent
//(when master type is the same, dimension sizes are the same and element type is the same or different but master data is the same and element types are master atomic)
bool MasterData::EquivalentArrays(int TypIndex1,int TypIndex2) const {
  if(Types[TypIndex1].MstType==MasterType::FixArray && Types[TypIndex2].MstType==MasterType::FixArray){
    if(Types[TypIndex1].DimNr==Types[TypIndex2].DimNr){
      bool SameDimSizes=true;
      for(int i=0;i<Types[TypIndex1].DimNr;i++){
        if(Dimensions[Types[TypIndex1].DimIndex].DimSize.n[i]!=Dimensions[Types[TypIndex2].DimIndex].DimSize.n[i]){
          SameDimSizes=false;
          break;
        }
      }
      if(SameDimSizes){
        if(Types[TypIndex1].ElemTypIndex==Types[TypIndex1].ElemTypIndex){
          return true;
        }
        else if(Types[Types[TypIndex1].ElemTypIndex].MstType==Types[Types[TypIndex1].ElemTypIndex].MstType
        && IsMasterAtomic(Types[TypIndex1].ElemTypIndex) && IsMasterAtomic(Types[TypIndex2].ElemTypIndex)){
          return true;
        }
      }
    }
  }
  else if(Types[TypIndex1].MstType==MasterType::DynArray && Types[TypIndex2].MstType==MasterType::DynArray){
    if(Types[TypIndex1].DimNr==Types[TypIndex2].DimNr){
      if(Types[TypIndex1].ElemTypIndex==Types[TypIndex1].ElemTypIndex){
        return true;
      }
      else if(Types[Types[TypIndex1].ElemTypIndex].MstType==Types[Types[TypIndex1].ElemTypIndex].MstType
      && IsMasterAtomic(Types[TypIndex1].ElemTypIndex) && IsMasterAtomic(Types[TypIndex2].ElemTypIndex)){
        return true;
      }
    }
  }
  return false;
}

//Filters word type converting it to int/long
int MasterData::WordTypeFilter(int TypIndex,bool CurrentArchitecture) const {
  int Result;
  if(TypIndex==WrdTypIndex){
    if     (CurrentArchitecture  && GetArchitecture()==32){ Result=IntTypIndex; }
    else if(CurrentArchitecture  && GetArchitecture()==64){ Result=LonTypIndex; }
    else if(!CurrentArchitecture && GetArchitecture()==32){ Result=LonTypIndex; }
    else if(!CurrentArchitecture && GetArchitecture()==64){ Result=IntTypIndex; }
    else{ Result=TypIndex; }
  }
  else{
    Result=TypIndex;
  }
  return Result;
}

//Replaces system type defined in symbol tables by system types in current one
int MasterData::SystemTypeFilter(int TypIndex) const {
  if(Types[TypIndex].IsSystemDef){
    return TypSearch(Types[TypIndex].Name,MainModIndex());
  }
  else{
    return TypIndex;
  }
}

//Calculate cannonical type name (unique name)
String MasterData::CannonicalTypeName(int TypIndex) const {
  if(MainModIndex()==Types[TypIndex].Scope.ModIndex){
    return Types[TypIndex].Name;
  }
  else{
    return Modules[Types[TypIndex].Scope.ModIndex].Name+"."+Types[TypIndex].Name;
  }
}

//Converts type name comming from parser into equivalent type name as appearing on index
//Parser identifies data types as they appear in source code (with tracker), we replace tracker by module name
void MasterData::DecoupleTypeName(const String& ParserTypeName,String& TypeName,int& ModIndex) const {
  if(ParserTypeName.Search(".")!=-1){
    int TrkIndex;
    Array<String> TypePart=ParserTypeName.Split(".",2);
    if((TrkIndex=TrkSearch(TypePart[0]))==-1){
      TypeName=ParserTypeName;
      ModIndex=CurrentScope().ModIndex;
    }
    else{
      TypeName=TypePart[1];
      ModIndex=Trackers[TrkIndex].ModIndex;
    }
  }
  else{
    TypeName=ParserTypeName;
    ModIndex=CurrentScope().ModIndex;
  }
}

//Determine if master type is atomic
bool MasterData::IsMasterAtomic(MasterType MstType) const{
  bool IsAtomic;
  switch(MstType){
    case MasterType::Boolean : IsAtomic=true;  break;
    case MasterType::Char    : IsAtomic=true;  break;
    case MasterType::Short   : IsAtomic=true;  break;
    case MasterType::Integer : IsAtomic=true;  break;
    case MasterType::Long    : IsAtomic=true;  break;
    case MasterType::Float   : IsAtomic=true;  break;
    case MasterType::String  : IsAtomic=true;  break;
    case MasterType::Enum    : IsAtomic=false; break;
    case MasterType::Class   : IsAtomic=false; break;
    case MasterType::FixArray: IsAtomic=false; break;
    case MasterType::DynArray: IsAtomic=false; break;
  }
  return IsAtomic;
}

//Determine if data type is atomic
bool MasterData::IsMasterAtomic(int TypIndex) const{
  return IsMasterAtomic(Types[TypIndex].MstType);
}

//Determine if a class is empty (does not have any fields)
bool MasterData::IsEmptyClass(int TypIndex) const{
  if(Types[TypIndex].MstType!=MasterType::Class){
    return false;
  }
  if(Types[TypIndex].FieldLow==-1 && Types[TypIndex].FieldHigh==-1){
    return true;
  }
  else{
    return false;
  }
}

//Determine if a class is static (all fields defined as static)
bool MasterData::IsStaticClass(int TypIndex) const{
  if(Types[TypIndex].MstType!=MasterType::Class){
    return false;
  }
  if(Types[TypIndex].FieldLow!=-1 && Types[TypIndex].FieldHigh!=-1){
    int i=-1;
    while((i=FieldLoop(TypIndex,i))!=-1){
      if(!Fields[i].IsStatic){ return false; }
    }
    return true;
  }
  else{
    return false;
  }
}

//Calculate data type length on main memory
CpuWrd MasterData::TypLength(int TypIndex) const {
  
  //Variables
  int i;
  CpuWrd Size;

  //Switch on master type
  switch(Types[TypIndex].MstType){
    case MasterType::Boolean: 
      Size=sizeof(CpuBol);                
      break;
    case MasterType::Char: 
      Size=sizeof(CpuChr);   
      break;
    case MasterType::Short: 
      Size=sizeof(CpuShr);   
      break;
    case MasterType::Integer: 
      Size=sizeof(CpuInt);   
      break;
    case MasterType::Long: 
      Size=sizeof(CpuLon);   
      break;
    case MasterType::Float:   
      Size=sizeof(CpuFlo); 
      break;
    case MasterType::String:  
      Size=sizeof(CpuMbl); 
      break;
    case MasterType::Class:
      Size=0;
      if(Types[TypIndex].FieldLow!=-1 && Types[TypIndex].FieldHigh!=-1){
        i=-1;
        while((i=FieldLoop(TypIndex,i))!=-1){ 
          if(!Fields[i].IsStatic){
            Size+=TypLength(Fields[i].TypIndex); 
          }
        }
      }
      break;
    case MasterType::Enum: 
      Size=sizeof(CpuInt);   
      break;
    case MasterType::FixArray:
      Size=TypLength(Types[TypIndex].ElemTypIndex)*ArrayElements(Types[TypIndex].DimNr,Dimensions[Types[TypIndex].DimIndex].DimSize);
      break;
    case MasterType::DynArray:
      Size=sizeof(CpuMbl);
      break;
  }

  //Return result
  return Size;

}

//Calculate variable length on stack memory
CpuWrd MasterData::VarLength(int VarIndex) const {
  
  //Is a reference ?
  if(Variables[VarIndex].IsReference){
    return sizeof(CpuRef);
  }

  //Rest of cases
  else{
    return TypLength(Variables[VarIndex].TypIndex);
  }

}

//StoreModule
void MasterData::StoreModule(const String& Name,const String& Path,bool IsModLibrary,const SourceInfo& SrcInfo){

  //Variables
  int ModIndex;
  ModuleTable Module;
  
  //Fill in module fields
  Module.Name=Name;
  Module.Path=Path;
  Module.IsModLibrary=IsModLibrary;
  Module.DbgSymIndex=-1;

  //Add to table
  Modules.Add(Module);
  ModIndex=Modules.Length()-1;

  //Add to index
  _ScopeStk.Top().Mod.Add(SearchIndex(Name,ModIndex));

  //Store debug symbols
  if(DebugSymbols){
    Modules[ModIndex].DbgSymIndex=
    Bin.StoreDbgSymModule(
    Modules[ModIndex].Name,
    Modules[ModIndex].Path,
    SrcInfo);
  }

  //Debug messages
  DebugMessage(DebugLevel::CmpMasterData,"MOD["+ToString(ModIndex)+"]: name="+Name+" path="+Path+" library="+(IsModLibrary?"yes":"no"));
  DebugMessage(DebugLevel::CmpIndex,"MOD: name="+Name+" index="+ToString(ModIndex)+" searchindex="+ToString(_ScopeStk.Top().Mod.LastAdded())+", scope="+ScopeName(_ScopeStk.Top().Scope));

}

//Store Module tracker
void MasterData::StoreTracker(const String& Name,int ModIndex){

  //Variables
  int TrkIndex;
  TrackerTable Tracker;
  
  //Fill in module fields
  Tracker.Name=Name;
  Tracker.ModIndex=ModIndex;

  //Add to table
  Trackers.Add(Tracker);
  TrkIndex=Trackers.Length()-1;

  //Add to index
  _ScopeStk.Top().Trk.Add(SearchIndex(Name,TrkIndex));

  //Debug message
  DebugMessage(DebugLevel::CmpMasterData,"TRK["+ToString(TrkIndex)+"]: name="+Name+" module="+Modules[ModIndex].Name);
  DebugMessage(DebugLevel::CmpIndex,"TRK: name="+Name+" index="+ToString(TrkIndex)+" searchindex="+ToString(_ScopeStk.Top().Trk.LastAdded())+", scope="+ScopeName(_ScopeStk.Top().Scope));

}

//Store dimension
void MasterData::StoreDimension(ArrayIndexes DimSize,CpuAgx GeomIndex){
  
  //Variables
  DimensionTable Dimension;
  
  //Fill in dimension fields
  Dimension.DimSize=DimSize;
  Dimension.GeomIndex=GeomIndex;
  Dimension.TypIndex=-1;
  Dimension.LnkSymIndex=-1;

  //Add to table
  Dimensions.Add(Dimension);

  //Debug message
  #ifdef __DEV__
  String DimString="";
  int DimIndex=Dimensions.Length()-1;
  for(int i=0;i<_MaxArrayDims;i++){ DimString+=(DimString.Length()!=0?",":"")+ToString(DimSize.n[i]); }
  DebugMessage(DebugLevel::CmpMasterData,"DIM["+ToString(DimIndex)+"]: geomindex="+ToString(GeomIndex)+" dimsizes=("+DimString+")");
  #endif

}

//StoreType
//Length==0 means calculate automatically
void MasterData::StoreType(const ScopeDef& Scope,const SubScopeDef& SubScope,const String& Name,MasterType MstType,bool IsTypedef,int OrigTypIndex,bool IsSystemDef,
                           CpuWrd Length,int DimNr,int ElemTypIndex,int DimIndex,int FieldLow,int FieldHigh,int MemberLow,int MemberHigh,bool CreateMetaInfo,const SourceInfo& SrcInfo){

  //Variables
  int TypIndex;
  String IdxName;
  CpuAdr MetaName;
  TypeTable Type;
  
  //Meta variable
  if(CreateMetaInfo){
    MetaName=(CreateMetaInfo?StoreLitString(Name):0);
  }

  //Fill in data type fields
  Type.Name=Name;
  Type.MstType=MstType;
  Type.Scope=Scope;
  Type.SubScope=SubScope;
  Type.IsTypedef=IsTypedef;
  Type.OrigTypIndex=OrigTypIndex;
  Type.IsSystemDef=IsSystemDef;
  Type.DimNr=DimNr;
  Type.ElemTypIndex=(DimNr!=0?ElemTypIndex:-1);
  Type.DimIndex=DimIndex;
  Type.FieldLow=FieldLow;
  Type.FieldHigh=FieldHigh;
  Type.MemberLow=MemberLow;
  Type.MemberHigh=MemberHigh;
  Type.Length=0;
  Type.EnumCount=0;
  Type.AuxFieldOffset=0;
  Type.MetaName=(CreateMetaInfo?MetaName:-1);
  Type.MetaStNames=-1;
  Type.MetaStTypes=-1;
  Type.DlName="";
  Type.DlType="";
  Type.LnkSymIndex=-1;
  Type.DbgSymIndex=-1;

  //Add to table
  Types.Add(Type);
  TypIndex=Types.Length()-1;

  //Update length
  Types[TypIndex].Length=(Length==0?TypLength(TypIndex):Length);

  //Add type to search index
  IdxName=Modules[Scope.ModIndex].Name+"."+Name;
  _ScopeStk.Top().Typ.Add(SearchIndex(IdxName,TypIndex));

  //Store dimension symbol (all of them so Bin._DimensionNr(), _Md->Dimensions() and the dimension indexes on instructions are all in sync)
  if(CompileToLibrary && Scope.Kind==ScopeKind::Public && Types[TypIndex].DimIndex!=-1 && Scope.ModIndex==MainModIndex()){
    Dimensions[DimIndex].LnkSymIndex=
    Bin.StoreLnkSymDimension(Dimensions[Types[TypIndex].DimIndex].DimSize,(CpuAgx)Dimensions[Types[TypIndex].DimIndex].GeomIndex);
  }

  //Store coresponding symbol
  if(CompileToLibrary && Scope.Kind==ScopeKind::Public && Scope.ModIndex==MainModIndex()){
    Types[TypIndex].LnkSymIndex=
    Bin.StoreLnkSymType(
    Types[TypIndex].Name,
    (CpuChr)Types[TypIndex].MstType,
    (CpuInt)(Types[TypIndex].Scope.FunIndex!=-1?Functions[Types[TypIndex].Scope.FunIndex].LnkSymIndex:-1),
    (CpuInt)(Types[TypIndex].SubScope.TypIndex!=-1?Types[Types[TypIndex].SubScope.TypIndex].LnkSymIndex:-1),
    (CpuBol)Types[TypIndex].IsTypedef,
    (CpuInt)(Types[TypIndex].OrigTypIndex!=-1?Types[Types[TypIndex].OrigTypIndex].LnkSymIndex:-1),
    (CpuBol)Types[TypIndex].IsSystemDef,
    (CpuLon)Types[TypIndex].Length,
    (CpuChr)Types[TypIndex].DimNr,
    (CpuInt)(Types[TypIndex].ElemTypIndex!=-1?Types[Types[TypIndex].ElemTypIndex].LnkSymIndex:-1),
    (CpuInt)(Types[TypIndex].DimIndex!=-1?Dimensions[Types[TypIndex].DimIndex].LnkSymIndex:-1),
    (CpuInt)(Types[TypIndex].FieldLow!=-1?Fields[Types[TypIndex].FieldLow].LnkSymIndex:-1),
    (CpuInt)(Types[TypIndex].FieldHigh!=-1?Fields[Types[TypIndex].FieldHigh].LnkSymIndex:-1),
    (CpuInt)(Types[TypIndex].MemberLow!=-1?Functions[Types[TypIndex].MemberLow].LnkSymIndex:-1),
    (CpuInt)(Types[TypIndex].MemberHigh!=-1?Functions[Types[TypIndex].MemberHigh].LnkSymIndex:-1),
    (CpuAdr)Types[TypIndex].MetaName,
    (CpuAdr)Types[TypIndex].MetaStNames,
    (CpuAdr)Types[TypIndex].MetaStTypes,
    SrcInfo);
  }

  //Store debug symbols
  if(DebugSymbols){
    Types[TypIndex].DbgSymIndex=
    Bin.StoreDbgSymType(
    Types[TypIndex].Name,
    (CpuChr)CpuDataTypeFromMstType(Types[TypIndex].MstType),
    (CpuChr)Types[TypIndex].MstType,
    (CpuLon)Types[TypIndex].Length,
    (CpuInt)(Types[TypIndex].FieldLow!=-1?Fields[Types[TypIndex].FieldLow].DbgSymIndex:-1),
    (CpuInt)(Types[TypIndex].FieldHigh!=-1?Fields[Types[TypIndex].FieldHigh].DbgSymIndex:-1),
    SrcInfo);
  }

  //Debug message
  DebugMessage(DebugLevel::CmpMasterData,"TYP["+ToString(TypIndex)+"]: name="+Name+" msttype="+MasterTypeName(MstType)+" scope="+ScopeName(Scope)
  +(SubScope.Kind!=SubScopeKind::None?" subscope="+SubScopeName(SubScope):"")+" typedef="+(IsTypedef?"yes":"no")+" origtypindex="+ToString(OrigTypIndex)
  +" systemdef="+(IsSystemDef?"yes":"no")+" length="+ToString(Types[TypIndex].Length)+(DimNr!=0?" dimnr="+ToString(DimNr)
  +" elemtype="+CannonicalTypeName(ElemTypIndex):"")+" lnksymindex="+ToString(Types[TypIndex].LnkSymIndex));
  DebugMessage(DebugLevel::CmpIndex,"TYP: name="+IdxName+" index="+ToString(TypIndex)+" searchindex="+ToString(_ScopeStk.Top().Typ.LastAdded())+", scope="+ScopeName(_ScopeStk.Top().Scope));

}

//Reuse variable
bool MasterData::ReuseVariable(const ScopeDef& Scope,CpuLon CodeBlockId,CpuLon FlowLabel,const String& Name,int TypIndex,bool IsStatic,
                               bool IsConst,bool IsReference,const SourceInfo& SrcInfo,const String& SourceLine,int& VarIndex){

  //Only check for hidden variables if have local storage
  if(Scope.Kind==ScopeKind::Local && (!IsConst || (IsConst && IsReference)) && !IsStatic){
      
    //Check variable is defined already as hidden
    if((VarIndex=VarSearch(Name,Scope.ModIndex,true))!=-1){

      //Internal error: If variable is found here it can only be because it is hidden and it is in the same scope
      if(!Variables[VarIndex].IsHidden){
        SrcInfo.Msg(443).Print(Name);
        VarIndex=-1;
        return false;
      }

      //Internal error: If variable is found here it must be in the same scope
      if(Variables[VarIndex].Scope!=Scope){
        SrcInfo.Msg(444).Print(Name);
        VarIndex=-1;
        return false;
      }

      //Only variables with local storage can be reused
      if(Variables[VarIndex].Scope.Kind==ScopeKind::Local 
      && (!Variables[VarIndex].IsConst || (Variables[VarIndex].IsConst && Variables[VarIndex].IsReference)) && !Variables[VarIndex].IsStatic){

        //Reuse only if data type is the same
        if(Variables[VarIndex].TypIndex==TypIndex && Variables[VarIndex].IsReference==IsReference){
          DebugMessage(DebugLevel::CmpMasterData,"Variable "+Name+" with datatype "+CannonicalTypeName(TypIndex)+" in scope "+ScopeName(Scope)
          +" with code block "+CodeBlockDefText(GetCodeBlockDef(Variables[VarIndex].CodeBlockId))+" and flow label "+ToString(Variables[VarIndex].FlowLabel)+" is reused");
          Variables[VarIndex].CodeBlockId=CodeBlockId;
          Variables[VarIndex].FlowLabel=FlowLabel;
          Variables[VarIndex].IsSourceUsed=false;
          Variables[VarIndex].IsInitialized=false;
          Variables[VarIndex].IsHidden=false;
          Variables[VarIndex].LnkSymIndex=-1;
          Variables[VarIndex].LineNr=SrcInfo.LineNr;
          Variables[VarIndex].ColNr=SrcInfo.ColNr;
          Variables[VarIndex].SourceLine=SourceLine;
          DebugMessage(DebugLevel::CmpExpression,"Source used flag cleared for variable "+Variables[VarIndex].Name+" in scope "+ScopeName(Variables[VarIndex].Scope));
          DebugMessage(DebugLevel::CmpExpression,"Initialized flag cleared for variable "+Variables[VarIndex].Name+" in scope "+ScopeName(Variables[VarIndex].Scope));
          return true;
        }
        
      }

      //Found hidden variable without local storage
      else{
        SrcInfo.Msg(445).Print(Name);
        VarIndex=-1;
        return false;
      }
  
    }
  
  }

  //No variable reuse but no error
  VarIndex=-1;
  return true;

}

//Clean hidden variable
void MasterData::CleanHidden(const ScopeDef& Scope,const String& Name){
  
  //Variables
  int VarIndex;
  int SearchIndex;
  
  //Check if we have another variable with same name but hidden
  if((VarIndex=VarSearch(Name,Scope.ModIndex,true,&SearchIndex))!=-1){

    //Debug message
    DebugMessage(DebugLevel::CmpMasterData,"Variable "+Name+" with index "+ToString(VarIndex)+" in scope "+ScopeName(Scope)+" is destroyed");
    DebugMessage(DebugLevel::CmpExpression,"Source used flag cleared for variable "+Variables[VarIndex].Name+" in scope "+ScopeName(Variables[VarIndex].Scope));
    DebugMessage(DebugLevel::CmpExpression,"Initialized flag cleared for variable "+Variables[VarIndex].Name+" in scope "+ScopeName(Variables[VarIndex].Scope));
    
    //Destroy search index
    _ScopeStk[Variables[VarIndex].Scope.Depth].Var.Delete(SearchIndex);
    
    //Destroy variable data (scope is kept on purpose, as there are several procedures that expect variables in same scope to be consecutive in variables table)
    Variables[VarIndex].Name=_DestroyedVariablePrefix+"("+Variables[VarIndex].Name+")";
    Variables[VarIndex].CodeBlockId=0;
    Variables[VarIndex].FlowLabel=0;
    Variables[VarIndex].TypIndex=-1;
    Variables[VarIndex].Address=0;
    Variables[VarIndex].IsConst=false;
    Variables[VarIndex].IsStatic=false;
    Variables[VarIndex].IsParameter=false;
    Variables[VarIndex].IsReference=false;
    Variables[VarIndex].IsTempVar=false;
    Variables[VarIndex].IsSystemDef=false;
    Variables[VarIndex].IsTempLocked=false;
    Variables[VarIndex].IsSourceUsed=false;
    Variables[VarIndex].IsInitialized=false;
    Variables[VarIndex].IsHidden=false;
    Variables[VarIndex].MetaName=-1;
    Variables[VarIndex].LnkSymIndex=-1;
    Variables[VarIndex].DbgSymIndex=-1;
    Variables[VarIndex].LineNr=0;
    Variables[VarIndex].ColNr=-1;
    Variables[VarIndex].SourceLine="";

  }

}

//Store variable
void MasterData::StoreVariable(const ScopeDef& Scope,CpuLon CodeBlockId,CpuLon FlowLabel,const String& Name,int TypIndex,bool IsStatic,bool IsConst,bool IsParameter,
                               bool IsReference,bool IsTempVar,bool IsLitVar,bool IsSystemDef,bool BufferStore,bool CreateMetaInfo,const SourceInfo& SrcInfo,const String& SourceLine){

  //Variables
  int VarIndex;
  String IdxName;
  CpuAdr MetaName;
  CpuWrd Length;
  CpuAdr Address;
  VariableTable Variable;

  //Meta variable
  if(CreateMetaInfo){
    MetaName=(CreateMetaInfo?StoreLitString(Name):0);
  }

  //Calculate variable address
  if(BufferStore){
    Address=(Scope.Kind==ScopeKind::Local && (!IsConst || (IsConst && IsReference)) && !IsStatic?Bin.CurrentStackAddress():Bin.CurrentGlobAddress());
  }

  //Fill in variable fields
  Variable.Name=Name;
  Variable.Scope=Scope;
  Variable.CodeBlockId=CodeBlockId;
  Variable.FlowLabel=FlowLabel;
  Variable.TypIndex=TypIndex;
  Variable.Address=(BufferStore?Address:0);
  Variable.IsConst=IsConst;
  Variable.IsComputed=false;
  Variable.IsStatic=IsStatic;
  Variable.IsParameter=IsParameter;
  Variable.IsReference=IsReference;
  Variable.IsTempVar=IsTempVar;
  Variable.IsLitVar=IsLitVar;
  Variable.IsSystemDef=IsSystemDef;
  Variable.IsTempLocked=false;
  Variable.IsSourceUsed=false;
  Variable.IsInitialized=false;
  Variable.IsHidden=false;
  Variable.MetaName=(CreateMetaInfo?MetaName:-1);
  Variable.LnkSymIndex=-1;
  Variable.DbgSymIndex=-1;
  Variable.LineNr=SrcInfo.LineNr;
  Variable.ColNr=SrcInfo.ColNr;
  Variable.SourceLine=SourceLine;

  //Add to table
  Variables.Add(Variable);
  VarIndex=Variables.Length()-1;

  //Increase buffers
  if(BufferStore){
    Length=VarLength(VarIndex);
    if(Variables[VarIndex].Scope.Kind==ScopeKind::Local && (!IsConst || (IsConst && IsReference)) && !IsStatic){ 
      Bin.CumulStackSize(Length); 
    } 
    else{
      Bin.StoreGlobValue(Buffer(Length,0)); 
    }
  }
  else{
    Length=0;
  }

  //Modify search indexes
  if(Scope.Kind==ScopeKind::Local && ((IsConst && !IsReference) || IsStatic)){
    IdxName=Modules[Scope.ModIndex].Name+"."+GetStaticVarName(Scope.FunIndex,Name);
    _ScopeStk[_InnerPrivScope()].Var.Add(SearchIndex(IdxName,VarIndex));
  }
  else{
    IdxName=Modules[Scope.ModIndex].Name+"."+Name;
    _ScopeStk.Top().Var.Add(SearchIndex(IdxName,VarIndex));
  }

  //Store corresponding symbol
  if(CompileToLibrary && !IsSystemDef && !IsTempVar && !IsLitVar && Scope.Kind==ScopeKind::Public && Scope.ModIndex==MainModIndex()){
    Variables[VarIndex].LnkSymIndex=
    Bin.StoreLnkSymVariable(
    Variables[VarIndex].Name,
    (CpuInt)(Variables[VarIndex].Scope.FunIndex!=-1?Functions[Variables[VarIndex].Scope.FunIndex].LnkSymIndex:-1),
    (CpuInt)(Variables[VarIndex].TypIndex!=-1?Types[Variables[VarIndex].TypIndex].LnkSymIndex:-1),
    (CpuLon)Variables[VarIndex].Address,
    (CpuBol)Variables[VarIndex].IsConst,
    (CpuAdr)Variables[VarIndex].MetaName,
    SrcInfo);
  }

  //Store debug symbols
  if(DebugSymbols){
    Variables[VarIndex].DbgSymIndex=
    Bin.StoreDbgSymVariable(
    Variables[VarIndex].Name,
    (CpuInt)(Variables[VarIndex].Scope.ModIndex!=-1?Modules[Variables[VarIndex].Scope.ModIndex].DbgSymIndex:-1),
    (Variables[VarIndex].Scope.Kind==ScopeKind::Local?0:1),
    (CpuInt)(Variables[VarIndex].Scope.FunIndex!=-1?Functions[Variables[VarIndex].Scope.FunIndex].DbgSymIndex:-1),
    (CpuInt)(Variables[VarIndex].TypIndex!=-1?Types[Variables[VarIndex].TypIndex].DbgSymIndex:-1),
    (CpuLon)Variables[VarIndex].Address,
    (CpuBol)Variables[VarIndex].IsReference,
    SrcInfo);
  }

  //Debug message
  DebugMessage(DebugLevel::CmpMasterData,"VAR["+ToString(VarIndex)+"]: name="+Name+" type="+(TypIndex!=-1?CannonicalTypeName(TypIndex):"-1")+" scope="+ScopeName(Scope)
  +(CodeBlockId!=0?" codeblock="+CodeBlockDefText(GetCodeBlockDef(CodeBlockId)):"")+(FlowLabel!=-1?" flowlabel="+ToString(FlowLabel):"")
  +" address="+ToString(Variable.Address)+" const="+(IsConst?"yes":"no")+" static="+(IsStatic?"yes":"no")+" parameter="+(IsParameter?"yes":"no")
  +" ref="+(IsReference?"yes":"no")+" temp="+(IsTempVar?"yes":"no")+" litvar="+(IsLitVar?"yes":"no")+" buffstore="+(BufferStore?"yes":"no")+" length="+ToString(Length)
  +" lnksymindex="+ToString(Variables[VarIndex].LnkSymIndex));
  if(Scope.Kind==ScopeKind::Local && ((IsConst && !IsReference) || IsStatic)){
    DebugMessage(DebugLevel::CmpIndex,"VAR: name="+IdxName+" index="+ToString(VarIndex)+" searchindex="+ToString(_ScopeStk[_InnerPrivScope()].Var.LastAdded())+", scope="+ScopeName(_ScopeStk[_InnerPrivScope()].Scope));
  }
  else{
    DebugMessage(DebugLevel::CmpIndex,"VAR: name="+IdxName+" index="+ToString(VarIndex)+" searchindex="+ToString(_ScopeStk.Top().Var.LastAdded())+", scope="+ScopeName(_ScopeStk.Top().Scope));
  }

}

//Store field
void MasterData::StoreField(const SubScopeDef& SubScope,const String& Name,int TypIndex,bool IsStatic,CpuInt EnumValue,const SourceInfo& SrcInfo){

  //Variables
  int FldIndex;
  FieldTable Field;

  //Fill member variables
  Field.Name=Name;
  Field.TypIndex=TypIndex;
  Field.Offset=Types[SubScope.TypIndex].AuxFieldOffset;
  Field.IsStatic=IsStatic;
  Field.SubScope=SubScope;
  Field.EnumValue=EnumValue;
  Field.LnkSymIndex=-1;
  Field.DbgSymIndex=-1;

  //Add field
  Fields.Add(Field);
  FldIndex=Fields.Length()-1;

  //Increase field offset (static fields do not count as they are not stored inside the class)
  if(!IsStatic){
    Types[SubScope.TypIndex].AuxFieldOffset+=Types[TypIndex].Length;
  }
  
  //Store corresponding symbol
  if(CompileToLibrary && Types[SubScope.TypIndex].Scope.Kind==ScopeKind::Public && Types[SubScope.TypIndex].Scope.ModIndex==MainModIndex()){
    Fields[FldIndex].LnkSymIndex=
    Bin.StoreLnkSymField(
    Fields[FldIndex].Name,
    (CpuInt)(Fields[FldIndex].SubScope.TypIndex!=-1?Types[Fields[FldIndex].SubScope.TypIndex].LnkSymIndex:-1),
    (CpuChr)(Fields[FldIndex].SubScope.Kind),
    (CpuInt)(Fields[FldIndex].TypIndex!=-1?Types[Fields[FldIndex].TypIndex].LnkSymIndex:-1),
    (CpuLon)Fields[FldIndex].Offset,
    (CpuChr)Fields[FldIndex].IsStatic,
    EnumValue,SrcInfo);  
  }

  //Store debug symbols
  if(DebugSymbols){
    Fields[FldIndex].DbgSymIndex=
    Bin.StoreDbgSymField(
    Fields[FldIndex].Name,
    (CpuInt)(Fields[FldIndex].TypIndex!=-1?Types[Fields[FldIndex].TypIndex].DbgSymIndex:-1),
    (CpuLon)Fields[FldIndex].Offset,
    SrcInfo);
  }

  //Debug message
  DebugMessage(DebugLevel::CmpMasterData,"FLD["+ToString(FldIndex)+"]: type="+CannonicalTypeName(TypIndex)+" name="+Name+" subscope="+SubScopeName(SubScope)
  +" offset="+ToString(Field.Offset)+" static="+(IsStatic?"yes":"no")+" enumvalue="+ToString(EnumValue)+" lnksymindex="+ToString(Fields[FldIndex].LnkSymIndex));

}

//Store function depending on its kind
void MasterData::StoreRegularFunction(const ScopeDef& Scope,const String& Name,int TypIndex,bool IsVoid,bool IsNested,const SourceInfo& SrcInfo,const String& SourceLine){
  SubScopeDef SubScope=(SubScopeDef){SubScopeKind::None,-1};
  _InnerStoreFunction(Scope,SubScope,FunctionKind::Function,Name,TypIndex,IsVoid,IsNested,false,false,(MasterType)-1,(MasterMethod)-1,-1,(CpuInstCode)-1,"","",SrcInfo,SourceLine);
}
void MasterData::StoreSystemCall(const ScopeDef& Scope,const String& Name,int TypIndex,bool IsVoid,int SysCallNr,const SourceInfo& SrcInfo){
  SubScopeDef SubScope=(SubScopeDef){SubScopeKind::None,-1};
  _InnerStoreFunction(Scope,SubScope,FunctionKind::SysCall,Name,TypIndex,IsVoid,false,false,false,(MasterType)-1,(MasterMethod)-1,SysCallNr,(CpuInstCode)-1,"","",SrcInfo,"");
}
void MasterData::StoreSystemFunction(const ScopeDef& Scope,const String& Name,int TypIndex,bool IsVoid,CpuInstCode InstCode,const SourceInfo& SrcInfo){
  SubScopeDef SubScope=(SubScopeDef){SubScopeKind::None,-1};
  _InnerStoreFunction(Scope,SubScope,FunctionKind::SysFunc,Name,TypIndex,IsVoid,false,false,false,(MasterType)-1,(MasterMethod)-1,-1,InstCode,"","",SrcInfo,"");
}
void MasterData::StoreDlFunction(const ScopeDef& Scope,const String& Name,int TypIndex,bool IsVoid,const String& DlName,const String& DlFunction,const SourceInfo& SrcInfo){
  SubScopeDef SubScope=(SubScopeDef){SubScopeKind::None,-1};
  _InnerStoreFunction(Scope,SubScope,FunctionKind::DlFunc,Name,TypIndex,IsVoid,false,false,false,(MasterType)-1,(MasterMethod)-1,-1,(CpuInstCode)-1,DlName,DlFunction,SrcInfo,"");
}
void MasterData::StoreMasterMethod(const String& Name,int TypIndex,bool IsVoid,bool IsInitializer,bool IsMetaMethod,MasterType MstType,MasterMethod MstMethod){
  ScopeDef Scope=(const ScopeDef&){ScopeKind::Public,-1,-1};
  SubScopeDef SubScope=(SubScopeDef){SubScopeKind::None,-1};
  _InnerStoreFunction(Scope,SubScope,FunctionKind::MasterMth,Name,TypIndex,IsVoid,false,IsInitializer,IsMetaMethod,MstType,MstMethod,-1,(CpuInstCode)-1,"","",SourceInfo(),"");
}
void MasterData::StoreMemberFunction(const SubScopeDef& SubScope,const String& Name,int TypIndex,bool IsVoid,bool IsInitializer,const SourceInfo& SrcInfo,const String& SourceLine){
  ScopeDef Scope=Types[SubScope.TypIndex].Scope;
  _InnerStoreFunction(Scope,SubScope,FunctionKind::Member,Name,TypIndex,IsVoid,false,IsInitializer,false,(MasterType)-1,(MasterMethod)-1,-1,(CpuInstCode)-1,"","",SrcInfo,SourceLine);
}
void MasterData::StoreOperatorFunction(const ScopeDef& Scope,const String& Name,int TypIndex,bool IsVoid,bool IsNested,const SourceInfo& SrcInfo,const String& SourceLine){
  SubScopeDef SubScope=(SubScopeDef){SubScopeKind::None,-1};
  _InnerStoreFunction(Scope,SubScope,FunctionKind::Operator,Name,TypIndex,IsVoid,IsNested,false,false,(MasterType)-1,(MasterMethod)-1,-1,(CpuInstCode)-1,"","",SrcInfo,SourceLine);
}

//Store function
void MasterData::_InnerStoreFunction(const ScopeDef& Scope,const SubScopeDef& SubScope,FunctionKind Kind,const String& Name,int TypIndex,bool IsVoid,bool IsNested,
                                     bool IsInitializer,bool IsMetaMethod,MasterType MstType,MasterMethod MstMethod,int SysCallNr,CpuInstCode InstCode,
                                     const String& DlName,const String& DlFunction,const SourceInfo& SrcInfo,const String& SourceLine){

  //Variables
  int FunIndex;
  bool CreateLnkSymbol;
  bool CreateDbgSymbol;
  FunctionTable Function;

  //Fill function variables
  Function.Kind=Kind;
  Function.Name=Name;
  Function.FullName="";
  Function.Fid="";
  Function.Scope=Scope;
  Function.SubScope=SubScope;
  Function.Address=0;
  Function.TypIndex=TypIndex;
  Function.IsVoid=IsVoid;
  Function.IsNested=IsNested;
  Function.IsDefined=false;
  Function.IsInitializer=IsInitializer;
  Function.IsMetaMethod=IsMetaMethod;
  Function.ParmNr=0;
  Function.ParmLow=-1;
  Function.ParmHigh=-1;
  Function.MstType=MstType;
  Function.MstMethod=MstMethod;
  Function.SysCallNr=SysCallNr;
  Function.InstCode=InstCode;
  Function.DlName=DlName;
  Function.DlFunction=DlFunction;
  Function.LnkSymIndex=-1;
  Function.DbgSymIndex=-1;
  Function.LineNr=SrcInfo.LineNr;
  Function.ColNr=SrcInfo.ColNr;
  Function.SourceLine=SourceLine;

  //Add function
  Functions.Add(Function);
  FunIndex=Functions.Length()-1;

  //Determine if symbols are to be created
  switch(Kind){
    case FunctionKind::Function: 
      CreateLnkSymbol=(Scope.Kind==ScopeKind::Public?true:false); 
      CreateDbgSymbol=true; 
      break;
    case FunctionKind::SysCall: 
      CreateLnkSymbol=(Scope.Kind==ScopeKind::Public?true:false); 
      CreateDbgSymbol=false; 
      break;
    case FunctionKind::SysFunc: 
      CreateLnkSymbol=(Scope.Kind==ScopeKind::Public?true:false); 
      CreateDbgSymbol=false; 
      break;
    case FunctionKind::DlFunc: 
      CreateLnkSymbol=(Scope.Kind==ScopeKind::Public?true:false); 
      CreateDbgSymbol=false; 
      break;
    case FunctionKind::MasterMth: 
      CreateLnkSymbol=false; 
      CreateDbgSymbol=false; 
      break;
    case FunctionKind::Member: 
      CreateLnkSymbol=(SubScope.Kind==SubScopeKind::Public && Types[SubScope.TypIndex].Scope.Kind==ScopeKind::Public?true:false); 
      CreateDbgSymbol=true; 
      break;
    case FunctionKind::Operator: 
      CreateLnkSymbol=(Scope.Kind==ScopeKind::Public?true:false); 
      CreateDbgSymbol=true; 
      break;
  }

  //Store corresponding symbol
  if(CompileToLibrary && CreateLnkSymbol && Scope.ModIndex==MainModIndex()){
    Functions[FunIndex].LnkSymIndex=
    Bin.StoreLnkSymFunction(
    (CpuChr)Functions[FunIndex].Kind,
    Functions[FunIndex].Name,
    (CpuInt)(Functions[FunIndex].SubScope.TypIndex!=-1?Types[Functions[FunIndex].SubScope.TypIndex].LnkSymIndex:-1),
    (CpuChr)Functions[FunIndex].SubScope.Kind,
    (CpuLon)Functions[FunIndex].Address,
    (CpuInt)(Functions[FunIndex].TypIndex!=-1?Types[Functions[FunIndex].TypIndex].LnkSymIndex:-1),
    (CpuBol)Functions[FunIndex].IsVoid,
    (CpuBol)Functions[FunIndex].IsInitializer,
    (CpuBol)Functions[FunIndex].IsMetaMethod,
    (CpuInt)Functions[FunIndex].ParmNr,
    (CpuInt)(Functions[FunIndex].ParmLow!=-1?Parameters[Functions[FunIndex].ParmLow].LnkSymIndex:-1),
    (CpuInt)(Functions[FunIndex].ParmHigh!=-1?Parameters[Functions[FunIndex].ParmHigh].LnkSymIndex:-1),
    (CpuChr)Function.MstType,
    (CpuShr)Function.MstMethod,
    (CpuInt)Functions[FunIndex].SysCallNr,
    (CpuShr)Functions[FunIndex].InstCode,
    Functions[FunIndex].DlName,
    Functions[FunIndex].DlFunction,
    SrcInfo);
  }

  //Store debug symbols
  if(DebugSymbols && CreateDbgSymbol){
    Functions[FunIndex].DbgSymIndex=
    Bin.StoreDbgSymFunction(
    (CpuChr)Functions[FunIndex].Kind,
    Functions[FunIndex].Name,
    (CpuInt)(Functions[FunIndex].Scope.ModIndex!=-1?Modules[Functions[FunIndex].Scope.ModIndex].DbgSymIndex:-1),
    (CpuLon)Functions[FunIndex].Address,
    (CpuLon)Functions[FunIndex].Address,
    (CpuInt)(Functions[FunIndex].TypIndex!=-1?Types[Functions[FunIndex].TypIndex].DbgSymIndex:-1),
    (CpuBol)Functions[FunIndex].IsVoid,
    (CpuInt)Functions[FunIndex].ParmNr,
    (CpuInt)(Functions[FunIndex].ParmLow!=-1?Parameters[Functions[FunIndex].ParmLow].DbgSymIndex:-1),
    (CpuInt)(Functions[FunIndex].ParmHigh!=-1?Parameters[Functions[FunIndex].ParmHigh].DbgSymIndex:-1),
    SrcInfo);
  }

  //Debug message
  DebugMessage(DebugLevel::CmpMasterData,"FUN["+ToString(FunIndex)+"]: name="+Name+" kind="+FunctionKindName(Kind)+" returns="+(TypIndex==-1?"undefined":CannonicalTypeName(TypIndex))
  +" void="+(IsVoid?"yes":"no")+" nested="+(IsNested?"yes":"no")+" init"+(IsInitializer?"yes":"no")+" meta="+(IsMetaMethod?"yes":"no")
  +(Kind==FunctionKind::MasterMth?" msttype="+MasterTypeName(MstType):"")
  +(Kind==FunctionKind::Member?" subscope="+SubScopeName(SubScope):"")
  +(Kind==FunctionKind::SysCall?+" scope="+ScopeName(Scope)+" syscallnr="+ToString(SysCallNr):"")
  +(Kind==FunctionKind::SysFunc?+" scope="+ScopeName(Scope)+" instcode="+ToString((int)InstCode):"")
  +(Kind==FunctionKind::DlFunc?+" scope="+ScopeName(Scope)+" dlname="+DlName+" dlfunction="+DlFunction:"")
  +(Kind==FunctionKind::Function?+" scope="+ScopeName(Scope):"")
  +" lnksymindex="+ToString(Functions[FunIndex].LnkSymIndex));

}

//Store parameter
void MasterData::StoreParameter(int FunIndex,const String& Name,int TypIndex,bool IsConst,bool IsReference,int ParmOrder,bool ModifyFunParmIndexes,const SourceInfo& SrcInfo,const String& SourceLine){

  //Variables
  int ParIndex;
  String ParmStr;
  String FunctionName;
  bool CreateLnkSymbol;
  bool CreateDbgSymbol;
  ScopeDef Scope;
  SubScopeDef SubScope;
  ParameterTable Parameter;

  //Fill in parameter fields
  Parameter.Name=Name;
  Parameter.TypIndex=TypIndex;
  Parameter.IsConst=IsConst;
  Parameter.IsReference=IsReference;
  Parameter.ParmOrder=ParmOrder;
  Parameter.FunIndex=FunIndex;
  Parameter.Address=0;
  Parameter.LnkSymIndex=-1;
  Parameter.DbgSymIndex=-1;
  Parameter.LineNr=SrcInfo.LineNr;
  Parameter.ColNr=SrcInfo.ColNr;
  Parameter.SourceLine=SourceLine;

  //Add to table
  Parameters.Add(Parameter);
  ParIndex=Parameters.Length()-1;

  //Add parameter to function (Passed as false when loading symbols, because function table is fully loaded before)
  if(ModifyFunParmIndexes){
    if(Functions[FunIndex].ParmNr==0){ Functions[FunIndex].ParmLow=ParIndex; }
    Functions[FunIndex].ParmHigh=ParIndex;
    Functions[FunIndex].ParmNr++;
  }

  //Determine if symbol is to be created
  Scope=Functions[FunIndex].Scope;
  SubScope=Functions[FunIndex].SubScope;
  switch(Functions[FunIndex].Kind){
    case FunctionKind::Function: 
      CreateLnkSymbol=(Scope.Kind==ScopeKind::Public?true:false); 
      CreateDbgSymbol=true; 
      break;
    case FunctionKind::SysCall: 
      CreateLnkSymbol=(Scope.Kind==ScopeKind::Public?true:false); 
      CreateDbgSymbol=false; 
      break;
    case FunctionKind::SysFunc: 
      CreateLnkSymbol=(Scope.Kind==ScopeKind::Public?true:false); 
      CreateDbgSymbol=false; 
      break;
    case FunctionKind::DlFunc: 
      CreateLnkSymbol=(Scope.Kind==ScopeKind::Public?true:false); 
      CreateDbgSymbol=false; 
      break;
    case FunctionKind::MasterMth: 
      CreateLnkSymbol=false; 
      CreateDbgSymbol=false; 
      break;
    case FunctionKind::Member: 
      CreateLnkSymbol=(SubScope.Kind==SubScopeKind::Public && Types[SubScope.TypIndex].Scope.Kind==ScopeKind::Public?true:false); 
      CreateDbgSymbol=true; 
      break;
    case FunctionKind::Operator: 
      CreateLnkSymbol=(Scope.Kind==ScopeKind::Public?true:false); 
      CreateDbgSymbol=true; 
      break;
  }

  //Store corresponding symbol
  if(CompileToLibrary && CreateLnkSymbol && Scope.ModIndex==MainModIndex()){
    Parameters[ParIndex].LnkSymIndex=
    Bin.StoreLnkSymParameter(
    Parameters[ParIndex].Name,
    (CpuInt)(Parameters[ParIndex].TypIndex!=-1?Types[Parameters[ParIndex].TypIndex].LnkSymIndex:-1),
    (CpuBol)Parameters[ParIndex].IsConst,
    (CpuBol)Parameters[ParIndex].IsReference,
    (CpuInt)Parameters[ParIndex].ParmOrder,
    (CpuInt)Functions[Parameters[ParIndex].FunIndex].LnkSymIndex,
    SrcInfo);
  }

  //Store debug symbols
  if(DebugSymbols && CreateDbgSymbol){
    Parameters[ParIndex].DbgSymIndex=
    Bin.StoreDbgSymParameter(
    Parameters[ParIndex].Name,
    (CpuInt)(Parameters[ParIndex].TypIndex!=-1?Types[Parameters[ParIndex].TypIndex].DbgSymIndex:-1),
    (CpuInt)(Parameters[ParIndex].FunIndex!=-1?Functions[Parameters[ParIndex].FunIndex].DbgSymIndex:-1),
    (CpuBol)Parameters[ParIndex].IsReference,
    SrcInfo);
  }

  //Debug message
  DebugMessage(DebugLevel::CmpMasterData,"PAR["+ToString(ParIndex)+"]: function="+Functions[FunIndex].Name+" name="+Name+" type="+(TypIndex==-1?"undefined":CannonicalTypeName(TypIndex))
  +" order="+ToString(ParmOrder)+" const="+(IsConst?"yes":"no")+" ref="+(IsReference?"yes":"no")+" lnksymindex="+ToString(Parameters[ParIndex].LnkSymIndex));

}

//StoreGrant
void MasterData::StoreGrant(GrantKind Kind,int TypIndex,const String& FrTypName,const String& FrFunName,int ToFldIndex,int ToFunIndex,int FrLineNr,int FrColNr){

  //Variables
  int GraIndex;
  String IdxName;
  GrantTable Grant;
  
  //Fill in data type fields
  Grant.Kind=Kind;
  Grant.TypIndex=TypIndex;
  Grant.FrTypName=FrTypName;
  Grant.FrFunName=FrFunName;
  Grant.ToFldIndex=ToFldIndex;
  Grant.ToFunIndex=ToFunIndex;
  Grant.FrLineNr=FrLineNr;
  Grant.FrColNr=FrColNr;

  //Add to table
  Grants.Add(Grant);
  GraIndex=Grants.Length()-1;

  //Modify search indexes
  IdxName=GrantName(Kind,TypIndex,FrTypName,FrFunName,ToFldIndex,ToFunIndex);
  _ScopeStk.Top().Gra.Add(SearchIndex(IdxName,GraIndex));

  //Debug message
  DebugMessage(DebugLevel::CmpMasterData,"GRA["+ToString(GraIndex)+"]: name="+IdxName);
  DebugMessage(DebugLevel::CmpIndex,"GRA: name="+IdxName+" index="+ToString(GraIndex)+" searchindex="+ToString(_ScopeStk.Top().Gra.LastAdded())+", scope="+ScopeName(_ScopeStk.Top().Scope));

}

//Hide local variables
void MasterData::HideLocalVariables(const ScopeDef& Scope,const Array<CpuLon>& ClosedBlocks,CpuLon FlowLabel,const SourceInfo& SrcInfo){
  
  //Variables
  int i,j;

  //Hide local variables from closed blocks
  if(ClosedBlocks.Length()!=0){
    for(j=0;j<ClosedBlocks.Length();j++){
      CheckUnusedScopeObjects(Scope,SrcInfo,ClosedBlocks[j],-1);
    }
    for(i=Variables.Length()-1;i>=0;i--){
      if(Variables[i].Scope==Scope){
        if(!Variables[i].IsTempVar && !Variables[i].IsSystemDef){
          for(j=0;j<ClosedBlocks.Length();j++){
            if(Variables[i].CodeBlockId==ClosedBlocks[j]){
              DebugMessage(DebugLevel::CmpMasterData,"Variable "+Variables[i].Name
              +" with datatype "+CannonicalTypeName(Variables[i].TypIndex)+" in scope "+ScopeName(Variables[i].Scope)
              +" and code block "+CodeBlockDefText(GetCodeBlockDef(Variables[i].CodeBlockId))+" is hidden");
              Variables[i].IsHidden=true;
            }
          }
        }
      }
      else{
        break;
      }
    }
  }

  //Hide local variables from closed for() / array() operator
  else if(FlowLabel!=-1){
    CheckUnusedScopeObjects(Scope,SrcInfo,0,FlowLabel);
    for(i=Variables.Length()-1;i>=0;i--){
      if(Variables[i].Scope==Scope){
        if(!Variables[i].IsTempVar && !Variables[i].IsSystemDef){
          if(Variables[i].FlowLabel==FlowLabel){
            DebugMessage(DebugLevel::CmpMasterData,"Variable "+Variables[i].Name
            +" with datatype "+CannonicalTypeName(Variables[i].TypIndex)+" in scope "+ScopeName(Variables[i].Scope)
            +" and flow label "+ToString(FlowLabel)+" is hidden");
            Variables[i].IsHidden=true;
          }
        }
      }
      else{
        break;
      }
    }
  }


}

//Get convertible type name (system numeric types are all output with same name,to ease automatic numeric conversions)
//Also strings are returned with convertible names so that sames types from different modules or coming from type definitions, are seen as same type)
String MasterData::ConvertibleTypeName(int TypIndex) const {
  if(Types[TypIndex].MstType==MasterType::Char 
  || Types[TypIndex].MstType==MasterType::Short 
  || Types[TypIndex].MstType==MasterType::Integer 
  || Types[TypIndex].MstType==MasterType::Long 
  || Types[TypIndex].MstType==MasterType::Float){
    return _ConvertibleNumber;
  }
  else if(Types[TypIndex].MstType==MasterType::String){
    return _ConvertibleString;
  }
  else{
    return Types[TypIndex].Name;
  }
}

//Get function search name
String MasterData::FunctionSearchName(int FunIndex) const {
  String FunName;
  switch(Functions[FunIndex].Kind){
    case FunctionKind::Function:  FunName=Modules[Functions[FunIndex].Scope.ModIndex].Name+"."+Functions[FunIndex].Name; break;
    case FunctionKind::MasterMth: FunName=MasterTypeName(Functions[FunIndex].MstType)+"."+Functions[FunIndex].Name; break;
    case FunctionKind::Member:    FunName=Modules[Functions[FunIndex].Scope.ModIndex].Name+"."+Types[Functions[FunIndex].SubScope.TypIndex].Name+"."+Functions[FunIndex].Name; break;
    case FunctionKind::SysCall:   FunName=Modules[Functions[FunIndex].Scope.ModIndex].Name+"."+Functions[FunIndex].Name; break;
    case FunctionKind::SysFunc:   FunName=Modules[Functions[FunIndex].Scope.ModIndex].Name+"."+Functions[FunIndex].Name; break;  
    case FunctionKind::DlFunc:    FunName=Modules[Functions[FunIndex].Scope.ModIndex].Name+"."+Functions[FunIndex].Name; break;
    case FunctionKind::Operator:  FunName=Functions[FunIndex].Name; break;
  }
  return FunName;
}

//Calculate function parameter list
String MasterData::GetFunctionParameterList(int FunIndex,bool UseConvertibleTypes) const {

  //Variables
  int i;
  String TypName;
  String ParmStr;
  int ParmLow;
  int ParmHigh;

  //Compose function parameter string
  ParmStr="";
  if(Functions[FunIndex].ParmNr!=0){
    ParmLow=Functions[FunIndex].ParmLow;
    ParmHigh=Functions[FunIndex].ParmHigh;
    if(!Functions[FunIndex].IsVoid){ ParmLow++; }
    if(Functions[FunIndex].Kind==FunctionKind::Member){ ParmLow++; }
    for(i=ParmLow;i<=ParmHigh;i++){
      if(Parameters[i].Name.StartsWith(GetElementTypePrefix())){
        TypName=GetElementTypePrefix().Replace("_","");  
      }
      else if(Parameters[i].TypIndex==-1){
        TypName=_SystemUndefinedType;  
      }
      else{
        if(!UseConvertibleTypes){
          TypName=Types[WordTypeFilter(Parameters[i].TypIndex,true)].Name;
        }
        else{
          TypName=ConvertibleTypeName(WordTypeFilter(Parameters[i].TypIndex,true));
        }
      }
      ParmStr+=(ParmStr.Length()!=0?",":"")+TypName;
    }
  }

  //Return result
  return ParmStr;

}

//Store function search index according to argument list
void MasterData::StoreFunctionSearchIndex(int FunIndex,int ScopeStkIndex){

  //Variables
  int StkIndex;
  String FunName;
  String ParmStr1;
  String ParmStr2;

  //Calculate symbol storage and index name
  FunName=FunctionSearchName(FunIndex);

  //Get function parameter string
  ParmStr1=GetFunctionParameterList(FunIndex,false);
  ParmStr2=GetFunctionParameterList(FunIndex,true);

  //Update function full name
  if(Functions[FunIndex].Scope.Kind==ScopeKind::Local){
    Functions[FunIndex].FullName=Modules[Functions[FunIndex].Scope.ModIndex].Name+"."+GetParentFunctionName(FunIndex)+"."+Functions[FunIndex].Name+"("+ParmStr1+")";
  }
  else{
    Functions[FunIndex].FullName=FunName+"("+ParmStr1+")";
  }

  //Calculate scope stack index
  if(ScopeStkIndex!=-1){
    StkIndex=ScopeStkIndex;
  }
  else{
    StkIndex=_ScopeStk.Length()-1;
  }

  //Add function to index
  _ScopeStk[StkIndex].Fun.Add(SearchIndex(FunName+"("+ParmStr1+")",FunIndex));
  DebugMessage(DebugLevel::CmpIndex,"FUN: name="+FunName+"("+ParmStr1+")"+" index="+ToString(FunIndex)+" searchindex="+ToString(_ScopeStk[StkIndex].Fun.LastAdded())+", scope="+ScopeName(Functions[FunIndex].Scope));

  //Add function to convertible index (only if parameter string has converted parameters and therefore it is diferent)
  if(ParmStr1!=ParmStr2){
    _ScopeStk[StkIndex].Fnc.Add(SearchIndex(FunName+"("+ParmStr2+")",FunIndex));
    DebugMessage(DebugLevel::CmpIndex,"FNC: name="+FunName+"("+ParmStr2+")"+" index="+ToString(FunIndex)+" searchindex="+ToString(_ScopeStk[StkIndex].Fnc.LastAdded())+", scope="+ScopeName(Functions[FunIndex].Scope));
  }

}

//Delete function and its parameters
void MasterData::DeleteFunction(int FunIndex){

  //Variables
  int i;
  int ParIndex;
  int LnkSymIndex;
  int DbgSymIndex;

  //Delete parameters (note that after deletion index of upper parms changes, it is correct to delete always at lowest index)
  ParIndex=Functions[FunIndex].ParmLow;
  LnkSymIndex=Parameters[ParIndex].LnkSymIndex;
  DbgSymIndex=Parameters[ParIndex].DbgSymIndex;
  for(i=Functions[FunIndex].ParmLow;i<=Functions[FunIndex].ParmHigh;i++){
    if(LnkSymIndex!=-1){ Bin.DeleteLnkSymParameter(LnkSymIndex); }
    if(DbgSymIndex!=-1){ Bin.DeleteDbgSymParameter(DbgSymIndex); }
    Parameters.Delete(ParIndex);
  }

  //Delete function
  if(Functions[FunIndex].LnkSymIndex!=-1){ Bin.DeleteLnkSymFunction(Functions[FunIndex].LnkSymIndex); }
  if(Functions[FunIndex].DbgSymIndex!=-1){ Bin.DeleteDbgSymFunction(Functions[FunIndex].DbgSymIndex); }
  Functions.Delete(FunIndex);

}

//Get parent function name
String MasterData::GetParentFunctionName(int FunIndex) const {
  String Parent;
  String GrandParent;
  if(Functions[FunIndex].Scope.Kind!=ScopeKind::Local || Functions[FunIndex].Scope.FunIndex==-1){
    Parent="";
  }
  else{
    GrandParent=GetParentFunctionName(Functions[FunIndex].Scope.FunIndex);
    if(GrandParent!=""){
      Parent=GrandParent+"."+Functions[Functions[FunIndex].Scope.FunIndex].Name;
    }
    else{
      Parent=Functions[Functions[FunIndex].Scope.FunIndex].Name;
    }
  }
  return Parent;
}

//Modify nested function addresses
void MasterData::ModifyNestedFunctionAddresses(int FunIndex){
  int i;
  CpuAdr ParentAddress=Functions[FunIndex].Address;
  CpuAdr OldAddress;
  CpuAdr NewAddress;
  for(i=Functions.Length()-1;i>=0;i--){
    if(Functions[i].IsNested && Functions[i].Address>ParentAddress){
      OldAddress=Functions[i].Address;
      NewAddress=OldAddress+Bin.GetInitBufferSize();
      Functions[i].Address=NewAddress;
      DebugMessage(DebugLevel::CmpMergeCode,"Nested function "+Functions[i].Name+"() is recalculated from address "+HEXFORMAT(OldAddress)+" to new address "+HEXFORMAT(NewAddress));
    }
    else{
      break;
    }
  }
}

//Calculate function id (Function mangled name)
void MasterData::StoreFunctionId(int FunIndex){
  
  //Variables
  int Index;
  int Count=0;

  //Get function search name
  String Name=FunctionSearchName(FunIndex);
  
  //Special case when there is not scope open
  if(_ScopeStk.Length()==0){ Functions[FunIndex].Fid=Name.Replace(".","_"); return; }

  //Get count of function name matches in all scopes
  for(int i=_ScopeStk.Length()-1;i>=0;i--){
    
    //For every match in scopes: get all matches downwards and upwards (since index is sorted)
    Index=_ScopeStk[i].Fun.Search(Name,true);
    if(Index!=-1 && Index<_ScopeStk[i].Fun.Length() && Functions[_ScopeStk[i].Fun[Index].Pos].Name==Functions[FunIndex].Name){ 
      
      //Count scope match
      Count++;

      //Search downwards
      for(int j=Index+1;j<_ScopeStk[i].Fun.Length();j++){
        if(FunctionSearchName(_ScopeStk[i].Fun[j].Pos)==Name){ 
          Count++; 
        }
        else{ break; }
      }
      
      //Search upwards
      for(int j=Index-1;j>=0;j--){
        if(FunctionSearchName(_ScopeStk[i].Fun[j].Pos)==Name){ 
          Count++; 
        }
        else{ break; }
      }

    }

    //Finish scope loop
    if(_ScopeStk[i].Scope.Kind==ScopeKind::Public){ break; }

  }

  //For local functions and operators we must include in name parent functions
  if(Functions[FunIndex].Scope.Kind==ScopeKind::Local){
    Name=Modules[Functions[FunIndex].Scope.ModIndex].Name+"."+GetParentFunctionName(FunIndex)+"."+Functions[FunIndex].Name;
  }
  
  //Update function id
  Functions[FunIndex].Fid=Name.Replace(".","_")+(Count==1?String(""):ToString(Count));
  DebugMessage(DebugLevel::CmpMasterData,"FID["+ToString(FunIndex)+"]: name="+Functions[FunIndex].Name+" kind="+ToString((int)Functions[FunIndex].Kind)+" fullname="+Functions[FunIndex].FullName+" fid="+Functions[FunIndex].Fid);

}

//Load dynamic library
int MasterData::_LoadDynLibrary(const String& DlName,const SourceInfo& SrcInfo){

  //Variables
  int LibIndex;
  String LibFile;
  void *Handler;
  FunPtrLibArchitecture LibArchitecture;
  FunPtrInitDispatcher InitDispatcher;
  FunPtrSearchType SearchType;
  FunPtrSearchFunction SearchFunction;
  FunPtrGetFunctionDef GetFunctionDef;
  FunPtrCloseDispatcher CloseDispatcher;

  //Check library is loaded already
  LibIndex=-1;
  for(int i=0;i<_Library.Length();i++){ 
    if(_Library[i].Name==DlName){ LibIndex=i; break; } 
  }
  if(LibIndex!=-1){ return LibIndex; }

  //Open library
  LibFile=DynLibPath+DlName+DYNLIB_EXT;
  #ifdef __WIN__
    Handler=LoadLibrary(LibFile.CharPnt());
  #else
    Handler=dlopen(LibFile.CharPnt(),RTLD_LAZY);
  #endif
  if(Handler==nullptr){
    SrcInfo.Msg(248).Print(LibFile);
    return -1;
  }
  DebugMessage(DebugLevel::SysDynLib,"Opened dynamic library "+LibFile);

  //Get procedure addresses
  #ifdef __WIN__
    LibArchitecture=(FunPtrLibArchitecture)GetProcAddress((HMODULE)Handler,"_LibArchitecture");
    InitDispatcher=(FunPtrInitDispatcher)GetProcAddress((HMODULE)Handler,"_InitDispatcher");
    SearchType=(FunPtrSearchType)GetProcAddress((HMODULE)Handler,"_SearchType");
    SearchFunction=(FunPtrSearchFunction)GetProcAddress((HMODULE)Handler,"_SearchFunction");
    GetFunctionDef=(FunPtrGetFunctionDef)GetProcAddress((HMODULE)Handler,"_GetFunctionDef");
    CloseDispatcher=(FunPtrCloseDispatcher)GetProcAddress((HMODULE)Handler,"_CloseDispatcher");
  #else
    LibArchitecture=(FunPtrLibArchitecture)dlsym(Handler,"_LibArchitecture");
    InitDispatcher=(FunPtrInitDispatcher)dlsym(Handler,"_InitDispatcher");
    SearchType=(FunPtrSearchType)dlsym(Handler,"_SearchType");
    SearchFunction=(FunPtrSearchFunction)dlsym(Handler,"_SearchFunction");
    GetFunctionDef=(FunPtrGetFunctionDef)dlsym(Handler,"_GetFunctionDef");
    CloseDispatcher=(FunPtrCloseDispatcher)dlsym(Handler,"_CloseDispatcher");
  #endif
  if(LibArchitecture==nullptr || InitDispatcher==nullptr || SearchType==nullptr || SearchFunction==nullptr || GetFunctionDef==nullptr || CloseDispatcher==nullptr){
    if(LibArchitecture==nullptr){ SrcInfo.Msg(249).Print(DlName,MASTER_NAME,"_LibArchitecture"); }
    if(InitDispatcher==nullptr){ SrcInfo.Msg(249).Print(DlName,MASTER_NAME,"_InitDispatcher"); }
    if(SearchType==nullptr){ SrcInfo.Msg(249).Print(DlName,MASTER_NAME,"_SearchType"); }
    if(SearchFunction==nullptr){ SrcInfo.Msg(249).Print(DlName,MASTER_NAME,"_SearchFunction"); }
    if(GetFunctionDef==nullptr){ SrcInfo.Msg(249).Print(DlName,MASTER_NAME,"_GetFunctionDef"); }
    if(CloseDispatcher==nullptr){ SrcInfo.Msg(249).Print(DlName,MASTER_NAME,"_CloseDispatcher"); }
    #ifdef __WIN__
      FreeLibrary((HMODULE)Handler);
    #else
      dlclose(Handler);
    #endif
    DebugMessage(DebugLevel::SysDynLib,"Closed dynamic library "+LibFile);
    return -1;
  }

  //Check library architecture matches
  if(GetArchitecture()!=LibArchitecture()){
    SrcInfo.Msg(260).Print(GetArchitecture(),LibFile,LibArchitecture()); 
    #ifdef __WIN__
      FreeLibrary((HMODULE)Handler);
    #else
      dlclose(Handler);
    #endif
    DebugMessage(DebugLevel::SysDynLib,"Closed dynamic library "+LibFile);
    return -1;
  }

  //Call init dispatcher function
  DebugMessage(DebugLevel::SysDynLib,"Initializing call dispatcher initialized on dynamic library "+LibFile);
  if(!InitDispatcher()){
    SrcInfo.Msg(250).Print(LibFile); 
    #ifdef __WIN__
      FreeLibrary((HMODULE)Handler);
    #else
      dlclose(Handler);
    #endif
    DebugMessage(DebugLevel::SysDynLib,"Closed dynamic library "+LibFile);
    return -1;
  }
  DebugMessage(DebugLevel::SysDynLib,"Call dispatcher initialized on dynamic library "+LibFile);

  //Save library data into table
  _Library.Add((LibraryDef){
    .Name=DlName,
    .Handler=Handler,
    .InitDispatcher=InitDispatcher,
    .SearchType=SearchType,
    .SearchFunction=SearchFunction,
    .GetFunctionDef=GetFunctionDef,
    .CloseDispatcher=CloseDispatcher
  });
  LibIndex=_Library.Length()-1;
  DebugMessage(DebugLevel::SysDynLib,"Dynamic library "+LibFile+" registered with index "+ToString(LibIndex));

  //Return library index
  return LibIndex;

}

//Check dynamic function data type against source type in declaration
bool MasterData::DlParameterMatch(int FunIndex,const String& DlName,bool Result,int ParmIndex,const String& DlType,const SourceInfo& SrcInfo){
  
  //Variables
  bool IsVoid;
  bool IsConst;
  bool IsReference;
  int TypIndex;
  String LibType;
  String SourceType;
  String SourceName;
  String ExpectedType;
  String AltExpectedType;
  String ArraySize;

  //Get checking variables
  if(Result){
    if(Functions[FunIndex].IsVoid){
      IsVoid=true;
      IsConst=false;
      IsReference=false;
      TypIndex=Functions[FunIndex].TypIndex;
      SourceName="function result";
    }
    else{
      IsVoid=false;
      IsConst=false;
      IsReference=false;
      TypIndex=Functions[FunIndex].TypIndex;
      SourceName="function result";
    }
  }
  else{
    IsVoid=false;
    IsConst=Parameters[Functions[FunIndex].ParmLow+ParmIndex].IsConst;
    IsReference=Parameters[Functions[FunIndex].ParmLow+ParmIndex].IsReference;
    TypIndex=Parameters[Functions[FunIndex].ParmLow+ParmIndex].TypIndex;
    SourceName="parameter "+ToString(ParmIndex+(Functions[FunIndex].IsVoid?1:0));
  }
  LibType=DlType;

  //Check it is void (only for result)
  if(Result){
    if(!IsVoid && LibType=="void"){
      SrcInfo.Msg(254).Print(Functions[FunIndex].DlFunction,DlName); 
      return false;
    }
    else if(IsVoid && LibType!="void"){
      SrcInfo.Msg(255).Print(Functions[FunIndex].DlFunction,DlName); 
      return false;
    }
    if(IsVoid){ return true; }
  }

  //Switch on master type
  switch(Types[TypIndex].MstType){
    
    //Boolean
    case MasterType::Boolean:
      SourceType=(IsReference?"ref ":"")+Types[BolTypIndex].Name;
      ExpectedType=String("bool")+(IsReference?" *":"");
      AltExpectedType="";
      break;
    
    //Char
    case MasterType::Char:
      SourceType=(IsReference?"ref ":"")+Types[ChrTypIndex].Name;
      ExpectedType=String("char")+(IsReference?" *":"");
      AltExpectedType="";
      break;
    
    //Short
    case MasterType::Short:
      SourceType=(IsReference?"ref ":"")+Types[ShrTypIndex].Name;
      ExpectedType=String("cshort")+(IsReference?" *":"");
      AltExpectedType="";
      break;
    
    //Integer
    case MasterType::Integer:
      SourceType=(IsReference?"ref ":"")+Types[IntTypIndex].Name;
      ExpectedType=String("cint")+(IsReference?" *":"");
      AltExpectedType="";
      break;
    
    //Long
    case MasterType::Long:
      SourceType=(IsReference?"ref ":"")+Types[LonTypIndex].Name;
      ExpectedType=String("clong")+(IsReference?" *":"");
      AltExpectedType="";
      break;
    
    //Float
    case MasterType::Float:
      SourceType=(IsReference?"ref ":"")+Types[FloTypIndex].Name;
      ExpectedType=String("double")+(IsReference?" *":"");
      AltExpectedType="";
      break;
    
    //String
    case MasterType::String:
      if(!IsReference && !Result){ SrcInfo.Msg(257).Print(SourceName,Functions[FunIndex].DlFunction,DlName,MasterTypeName(Types[TypIndex].MstType)); return false; }
      SourceType=(IsConst?"const ref ":"ref ")+Types[StrTypIndex].Name;
      ExpectedType=(IsConst?"const char *":(Result?"char *":"char **"));
      AltExpectedType="";
      break;
    
    //Enum
    case MasterType::Enum:
      SourceType=(IsReference?"ref ":"")+CannonicalTypeName(TypIndex);
      ExpectedType=Types[TypIndex].Name+(IsReference?" *":"");
      if(Types[TypIndex].DlName==Functions[FunIndex].DlName){
        AltExpectedType=Types[TypIndex].DlType+(IsReference?" *":"");
      }
      else{
        AltExpectedType="";
      }
      break;
    
    //Class
    case MasterType::Class:
      if(!IsReference && !Result){ SrcInfo.Msg(257).Print(SourceName,Functions[FunIndex].DlFunction,DlName,MasterTypeName(Types[TypIndex].MstType)); return false; }
      SourceType=(IsConst?"const ref ":"ref ")+CannonicalTypeName(TypIndex);
      ExpectedType=(IsConst?"const "+Types[TypIndex].Name+" *":Types[TypIndex].Name+" *");
      if(Types[TypIndex].DlName==Functions[FunIndex].DlName){
        AltExpectedType=(IsConst?"const "+Types[TypIndex].DlType+" *":Types[TypIndex].DlType+" *");
      }
      else{
        AltExpectedType="";
      }
      break;
    
    //FixArray
    case MasterType::FixArray:
      if(!IsReference && !Result){ SrcInfo.Msg(257).Print(SourceName,Functions[FunIndex].DlFunction,DlName,MasterTypeName(Types[TypIndex].MstType)); return false; }
      if(Types[TypIndex].DimNr!=1){ SrcInfo.Msg(258).Print(SourceName,Functions[FunIndex].DlFunction,DlName); return false; }
      ArraySize=ToString(Dimensions[Types[TypIndex].DimIndex].DimSize.n[0]);
      SourceType=(IsConst?"const ref ":"ref ")+CannonicalTypeName(TypIndex)+"["+ArraySize+"]";
      ExpectedType=(IsConst?STRIN(CARRAY_TYPENAME)+Types[TypIndex].Name+ArraySize+" *":STRIN(FARRAY_TYPENAME)+Types[TypIndex].Name+ArraySize+" *");
      if(Types[TypIndex].DlName==Functions[FunIndex].DlName){
        AltExpectedType=(IsConst?STRIN(CARRAY_TYPENAME)+Types[TypIndex].DlType+ArraySize+" *":STRIN(FARRAY_TYPENAME)+Types[TypIndex].DlType+ArraySize+" *");
      }
      else{
        AltExpectedType="";
      }
      break;
    
    //DynArray
    case MasterType::DynArray:
      if(!IsReference && !Result){ SrcInfo.Msg(257).Print(SourceName,Functions[FunIndex].DlFunction,DlName,MasterTypeName(Types[TypIndex].MstType)); return false; }
      if(Types[TypIndex].DimNr!=1){ SrcInfo.Msg(258).Print(SourceName,Functions[FunIndex].DlFunction,DlName); return false; }
      SourceType=(IsConst?"const ref ":"ref ")+CannonicalTypeName(TypIndex)+"[]";
      ExpectedType=(IsConst?STRIN(CARRAY_TYPENAME)+Types[TypIndex].Name+" *":STRIN(DARRAY_TYPENAME)+Types[TypIndex].Name+" *");
      if(Types[TypIndex].DlName==Functions[FunIndex].DlName){
        AltExpectedType=(IsConst?STRIN(CARRAY_TYPENAME)+Types[TypIndex].DlType+" *":STRIN(DARRAY_TYPENAME)+Types[TypIndex].DlType+" *");
      }
      else{
        AltExpectedType="";
      }
      break;
  
  }

  //Check data type matches
  if(LibType!=ExpectedType && (AltExpectedType.Length()==0 || LibType!=AltExpectedType)){
    SrcInfo.Msg(256).Print(SourceName,Functions[FunIndex].DlFunction,DlName,SourceType,ExpectedType+(AltExpectedType.Length()!=0?" or "+AltExpectedType:""),LibType); 
    return false;
  }

  //Types with inner blocks are not supported
  if(HasInnerBlocks(TypIndex)){
    SrcInfo.Msg(259).Print(SourceName,Functions[FunIndex].DlFunction,DlName,SourceType);     
    return false;
  }

  //Return code
  return true;

}

//Check dynamic function against library
bool MasterData::DlFunctionCheck(int FunIndex,const SourceInfo& SrcInfo){

  //Variables
  int i;
  int LibIndex;
  int FuncId;
  FunctionDef Def;

  //Load library
  if((LibIndex=_LoadDynLibrary(Functions[FunIndex].DlName,SrcInfo))==-1){ return false; }

  //Search dl function name
  if((FuncId=_Library[LibIndex].SearchFunction((char *)Functions[FunIndex].DlFunction.CharPnt()))==-1){
    SrcInfo.Msg(251).Print(Functions[FunIndex].DlFunction,Functions[FunIndex].DlName);
    return false;
  }
  DebugMessage(DebugLevel::SysDynLib,"Function "+String((char *)Functions[FunIndex].DlFunction.CharPnt())+"() found with id "+ToString(FuncId)+" on dynamic library "+Functions[FunIndex].DlName);

  //Get dl function definition
  if(!_Library[LibIndex].GetFunctionDef(FuncId,&Def)){
    SrcInfo.Msg(252).Print(Functions[FunIndex].DlFunction,Functions[FunIndex].DlName);
    return false;
  }
  DebugMessage(DebugLevel::SysDynLib,"Read metadata for function "+String((char *)Functions[FunIndex].DlFunction.CharPnt())+"() found on dynamic library "+Functions[FunIndex].DlName);
  
  //Check number of parameters
  if(Functions[FunIndex].ParmNr!=Def.ParmNr){
    SrcInfo.Msg(253).Print(Functions[FunIndex].DlFunction,Functions[FunIndex].DlName,ToString(Def.ParmNr),ToString(Functions[FunIndex].ParmNr));
    return false;
  }

  //Check returning type
  if(!DlParameterMatch(FunIndex,Functions[FunIndex].DlName,true,-1,Def.ParmType[0],SrcInfo)){ return false; }

  //Check parameter types
  for(i=(Functions[FunIndex].IsVoid?0:1);i<Functions[FunIndex].ParmNr;i++){
    if(!DlParameterMatch(FunIndex,Functions[FunIndex].DlName,false,i,Def.ParmType[i+(Functions[FunIndex].IsVoid?1:0)],SrcInfo)){ return false; }
  }
  DebugMessage(DebugLevel::SysDynLib,"Metadata check passed for function "+String((char *)Functions[FunIndex].DlFunction.CharPnt())+"() on dynamic library "+Functions[FunIndex].DlName);

  //Return code
  return true;

}

//Check type exists on dynamic library function parameters
bool MasterData::DlTypeCheck(const String& TypeName,const String& DlName,const SourceInfo& SrcInfo){

  //Variables
  int LibIndex;

  //Load library
  if((LibIndex=_LoadDynLibrary(DlName,SrcInfo))==-1){ return false; }

  //Search type on library
  if(!_Library[LibIndex].SearchType((char *)TypeName.CharPnt())){
    SrcInfo.Msg(267).Print(TypeName,DlName);
    return false;
  }
  DebugMessage(DebugLevel::SysDynLib,"Type name "+TypeName+" is found on function parameters of library "+DlName);

  //Return code
  return true;

}

//Blocknumber bytes
Buffer MasterData::_BlockNumberBytes(int Block) const {
  Buffer Bytes;
  for(int i=sizeof(CpuMbl)-1,Shift=0;i>=0;i--){ Bytes+=(char)((Block >> Shift) & 0xFF); Shift+=8; }
  return Bytes;
}

//Store system constant integer as global variable
int MasterData::StoreSystemInteger(const ScopeDef& Scope,CpuLon CodeBlockId,const String& VarName,CpuInt Value){
  
  //Variables
  int VarIndex;

  //Store constant variable
  StoreVariable(Scope,CodeBlockId,-1,VarName,IntTypIndex,false,true,false,false,false,false,true,true,false,SourceInfo(),"");  
  VarIndex=Variables.Length()-1;

  //Add litteral value to glob buffer (rewind is necessary because store variable moves pointer)
  Bin.GlobBufferRewind(sizeof(CpuInt));
  Bin.StoreGlobValue(Buffer((char *)&Value,sizeof(CpuInt)));

  //Set as initialized and source used
  Variables[VarIndex].IsInitialized=true;
  Variables[VarIndex].IsSourceUsed=true;
  DebugMessage(DebugLevel::CmpExpression,"Source used flag set for variable "+Variables[VarIndex].Name+" in scope "+ScopeName(Variables[VarIndex].Scope));
  DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+Variables[VarIndex].Name+" in scope "+ScopeName(Variables[VarIndex].Scope));

  //Assembler output
  Bin.AsmOutLine(AsmSection::Data,"","STORE "+VarName+" ("+CpuDataTypeCharId(CpuDataType::Integer)+")"+ToString(Value)+"",";Address=["+HEXFORMAT(Variables[VarIndex].Address)+"]");

  //Return variable index
  return VarIndex;

}

//Store system constant string as global variable
int MasterData::StoreSystemString(const ScopeDef& Scope,CpuLon CodeBlockId,const String& VarName,const String& Value){

  //Variables
  int VarIndex;
  CpuMbl Block;

  //Store constant variable
  StoreVariable(Scope,CodeBlockId,-1,VarName,StrTypIndex,false,true,false,false,false,false,true,true,false,SourceInfo(),"");  
  VarIndex=Variables.Length()-1;

  //Add litteral value to block buffer and get block number
  Block=Bin.CurrentBlockAddress();
  Bin.StoreStrBlockValue(Value.ToBuffer());
  
  //Store block number in global buffer (rewind is necessary because store variable moves pointer)
  Bin.GlobBufferRewind(sizeof(CpuMbl));
  Bin.AddBlkRelocation(RelocType::GloBlock,Bin.CurrentGlobAddress(),CurrentModule(),"BLK:"+HEXFORMAT(Block));
  Bin.StoreGlobValue(_BlockNumberBytes(Block));

  //Set as initialized and source used
  Variables[VarIndex].IsInitialized=true;
  Variables[VarIndex].IsSourceUsed=true;
  DebugMessage(DebugLevel::CmpExpression,"Source used flag set for variable "+Variables[VarIndex].Name+" in scope "+ScopeName(Variables[VarIndex].Scope));
  DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+Variables[VarIndex].Name+" in scope "+ScopeName(Variables[VarIndex].Scope));

  //Assembler output
  Bin.AsmOutLine(AsmSection::Data,"","STORE "+VarName+" \""+NonAsciiEscape(Value)+"\"",";Address=["+HEXFORMAT(Variables[VarIndex].Address)+"]");

  //Return variable index
  return VarIndex;

}

//Store litteral string as variable
CpuAdr MasterData::StoreLitString(const String& Value){

  //Variables
  CpuAdr Address;
  CpuMbl Block;
  
  //Add litteral value to block buffer and get block number
  Block=Bin.CurrentBlockAddress();
  Bin.StoreStrBlockValue(Value.ToBuffer());
  
  //Store block number in global buffer
  Address=Bin.CurrentGlobAddress();
  Bin.AddBlkRelocation(RelocType::GloBlock,Address,CurrentModule(),"BLK:"+HEXFORMAT(Block));
  Bin.StoreGlobValue(_BlockNumberBytes(Block));

  //Assembler output
  Bin.AsmOutLine(AsmSection::Data,"","STORE \""+NonAsciiEscape(Value)+"\"",";Address=["+HEXFORMAT(Address)+"]");

  //Return address
  return Address;

}

//Store constant string array as litteral value
CpuAdr MasterData::StoreLitStringArray(Array<String>& Values){
  
  //Variables
  int i;
  CpuAdr Address;
  CpuMbl ArrBlock;
  CpuMbl StrBlock;
  Buffer ArrContentBytes;

  //Get new array block number (next block after all string blocks)
  ArrBlock=Bin.CurrentBlockAddress()+Values.Length();

  //Store strings in array as blocks and get block numbers as buffer
  for(i=0;i<Values.Length();i++){
    StrBlock=Bin.CurrentBlockAddress();
    Bin.StoreStrBlockValue(Values[i].ToBuffer());
    Bin.AddBlkBlkRelocation(RelocType::BlkBlock,ArrBlock,ArrContentBytes.Length(),CurrentModule(),"IBL:"+HEXFORMAT(StrBlock));
    ArrContentBytes+=_BlockNumberBytes(StrBlock);
  }

  //Add array content to array block buffer
  Bin.StoreArrBlockValue(ArrContentBytes,(ArrayDynDef){1,sizeof(CpuMbl),{Values.Length()}});

  //Store array block number in global buffer
  Address=Bin.CurrentGlobAddress();
  Bin.AddBlkRelocation(RelocType::GloBlock,Address,CurrentModule(),"BLK:"+HEXFORMAT(ArrBlock));
  Bin.StoreGlobValue(_BlockNumberBytes(ArrBlock));

  //Assembler output
  for(i=0;i<Values.Length();i++){
    if(i==0){
      Bin.AsmOutLine(AsmSection::Data,"","STORE {\""+NonAsciiEscape(Values[i])+"\"",";Address=["+HEXFORMAT(Address)+"]");
    }
    else if(i==Values.Length()-1){
      Bin.AsmOutLine(AsmSection::Data,"","STORE  \""+NonAsciiEscape(Values[i])+"\"}");
    }
    else{
      Bin.AsmOutLine(AsmSection::Data,"","STORE  \""+NonAsciiEscape(Values[i])+"\",");
    }
  }

  //Return variable index
  return Address;

}

//Get temporary variable and lock it for use
bool MasterData::TempVarNew(const ScopeDef& Scope,CpuLon CodeBlockId,int TypIndex,bool IsReference,const SourceInfo& SrcInfo,TempVarKind TempKind,int& VarIndex){

  //Static variables
  static int TmpBol=0;
  static int TmpChr=0;
  static int TmpShr=0;
  static int TmpInt=0;
  static int TmpLon=0;
  static int TmpFlo=0;
  static int TmpStr=0;
  static int TmpCla=0;
  static int TmpEnu=0;
  static int TmpArf=0;
  static int TmpArd=0;
  static int TmpRef=0;
  static int PrevFunIndex=-1;

  //Variables
  int i;
  bool Found;
  String Suffix;
  String TempVarName;
  String KindName;

  //Message
  DebugMessage(DebugLevel::CmpExpression,"Requested new temp variable for type="+CannonicalTypeName(TypIndex)+
  " reference="+(IsReference?"true":"false")+" kind="+(TempKind==TempVarKind::Promotion?"promotion":"regular"));
  
  //Select temp variable suffix
  switch(TempKind){
    case TempVarKind::Regular  : Suffix=_TempVarSuffixRegl; KindName="regular";   break;
    case TempVarKind::Promotion: Suffix=_TempVarSuffixProm; KindName="promotion"; break;
    case TempVarKind::Master   : Suffix=_TempVarSuffixMast; KindName="master";    break;
  }

  //Initialize counters on scope change
  if(Scope.FunIndex!=PrevFunIndex){
    TmpBol=0; TmpChr=0; TmpShr=0; TmpInt=0; TmpLon=0; TmpFlo=0; TmpStr=0; TmpCla=0; TmpEnu=0; TmpArf=0; TmpArd=0; TmpRef=0; PrevFunIndex=Scope.FunIndex;
  }

  //Reuse existing temporary variable in the current scope
  for(i=0;i<_ScopeStk.Top().Var.Length();i++){
    if(Variables[_ScopeStk.Top().Var[i].Pos].Scope.Kind==Scope.Kind
    && Variables[_ScopeStk.Top().Var[i].Pos].Scope.FunIndex==Scope.FunIndex
    && Variables[_ScopeStk.Top().Var[i].Pos].IsConst==false
    && Variables[_ScopeStk.Top().Var[i].Pos].IsTempVar==true
    && Variables[_ScopeStk.Top().Var[i].Pos].IsTempLocked==false
    && Variables[_ScopeStk.Top().Var[i].Pos].Name.EndsWith(Suffix)){
      Found=false;
      if(IsReference && Variables[_ScopeStk.Top().Var[i].Pos].IsReference){ Found=true; }
      else if(!IsReference && Types[TypIndex].MstType==MasterType::Class && TypIndex==Variables[_ScopeStk.Top().Var[i].Pos].TypIndex && !Variables[_ScopeStk.Top().Var[i].Pos].IsReference){ Found=true; }
      else if(!IsReference && Types[TypIndex].MstType==MasterType::Enum && TypIndex==Variables[_ScopeStk.Top().Var[i].Pos].TypIndex && !Variables[_ScopeStk.Top().Var[i].Pos].IsReference){ Found=true; }
      else if(!IsReference && Types[TypIndex].MstType==MasterType::FixArray && TypIndex==Variables[_ScopeStk.Top().Var[i].Pos].TypIndex && !Variables[_ScopeStk.Top().Var[i].Pos].IsReference){ Found=true; }
      else if(!IsReference && Types[TypIndex].MstType==MasterType::DynArray && TypIndex==Variables[_ScopeStk.Top().Var[i].Pos].TypIndex && !Variables[_ScopeStk.Top().Var[i].Pos].IsReference){ Found=true; }
      else if(!IsReference && Types[Variables[_ScopeStk.Top().Var[i].Pos].TypIndex].MstType==Types[TypIndex].MstType && !Variables[_ScopeStk.Top().Var[i].Pos].IsReference
      && Types[TypIndex].MstType!=MasterType::Class && Types[TypIndex].MstType!=MasterType::Enum && Types[TypIndex].MstType!=MasterType::FixArray && Types[TypIndex].MstType!=MasterType::DynArray){ Found=true; }
      if(Found){
        VarIndex=_ScopeStk.Top().Var[i].Pos;
        Variables[VarIndex].IsTempLocked=true;
        if(IsReference){ Variables[VarIndex].TypIndex=TypIndex; }
        DebugMessage(DebugLevel::CmpExpression,"Temp variable "+Variables[VarIndex].Name+" reused "+
        "(type="+CannonicalTypeName(Variables[VarIndex].TypIndex)+" reference="+(Variables[VarIndex].IsReference?"true":"false")+" kind="+KindName+")");
        return true;
      }
    }
  }

  //Define new name of temporary variable
  if(IsReference){
    TempVarName=GetSystemNameSpace()+"Ref"+Format("%03i",TmpRef++)+Suffix;
  }
  else{
    switch(Types[TypIndex].MstType){
      case MasterType::Boolean : TempVarName=GetSystemNameSpace()+"Bol"+Format("%03i",TmpBol++)+Suffix; break;
      case MasterType::Char    : TempVarName=GetSystemNameSpace()+"Chr"+Format("%03i",TmpChr++)+Suffix; break;
      case MasterType::Short   : TempVarName=GetSystemNameSpace()+"Shr"+Format("%03i",TmpShr++)+Suffix; break;
      case MasterType::Integer : TempVarName=GetSystemNameSpace()+"Int"+Format("%03i",TmpInt++)+Suffix; break;
      case MasterType::Long    : TempVarName=GetSystemNameSpace()+"Lon"+Format("%03i",TmpLon++)+Suffix; break;
      case MasterType::Float   : TempVarName=GetSystemNameSpace()+"Flo"+Format("%03i",TmpFlo++)+Suffix; break;
      case MasterType::String  : TempVarName=GetSystemNameSpace()+"Str"+Format("%03i",TmpStr++)+Suffix; break;
      case MasterType::Class   : TempVarName=GetSystemNameSpace()+"Cla"+Format("%03i",TmpCla++)+Suffix; break;
      case MasterType::Enum    : TempVarName=GetSystemNameSpace()+"Enu"+Format("%03i",TmpEnu++)+Suffix; break;
      case MasterType::FixArray: TempVarName=GetSystemNameSpace()+"Arf"+Format("%03i",TmpArf++)+Suffix; break;
      case MasterType::DynArray: TempVarName=GetSystemNameSpace()+"Ard"+Format("%03i",TmpArd++)+Suffix; break;
    }
  }

  //Store variable
  StoreVariable(Scope,CodeBlockId,-1,TempVarName,TypIndex,false,false,false,IsReference,true,false,false,true,false,SourceInfo(),"");
  VarIndex=Variables.Length()-1;
  DebugMessage(DebugLevel::CmpExpression,"New temp variable "+Variables[VarIndex].Name+" created "+
  "(type="+CannonicalTypeName(Variables[VarIndex].TypIndex)+" reference="+(Variables[VarIndex].IsReference?"true":"false")+" kind="+KindName+")");
  
  //Emit assembler variable declaration
  Bin.AsmOutVarDecl(AsmSection::Temp,(Scope.Kind!=ScopeKind::Local?true:false),false,IsReference,false,false,TempVarName,CpuDataTypeFromMstType(Types[TypIndex].MstType),
  VarLength(VarIndex),Variables[VarIndex].Address,"",false,false,"","");

  //Lock variable
  Variables[VarIndex].IsTempLocked=true;
  
  //Return variable index
  return true;

}

//Lock temporary variable
void MasterData::TempVarLock(int VarIndex){
  Variables[VarIndex].IsTempLocked=true;
  DebugMessage(DebugLevel::CmpExpression,"Temp variable "+Variables[VarIndex].Name+" locked");
 }

//Unlock temporary variable
void MasterData::TempVarUnlock(int VarIndex){
  Variables[VarIndex].IsTempLocked=false;
  DebugMessage(DebugLevel::CmpExpression,"Temp variable "+Variables[VarIndex].Name+" released");
 }

//Unlock temporary variable
void MasterData::TempVarUnlockAll(){
  DebugMessage(DebugLevel::CmpExpression,"Release all temp variables");
  for(int i=0;i<Variables.Length();i++){
    if(Variables[i].IsTempVar){
      Variables[i].IsTempLocked=false;
      DebugMessage(DebugLevel::CmpExpression,"Temp variable "+Variables[i].Name+" released");
    }
  }
}

//Has inner blocks
bool MasterData::HasInnerBlocks(int TypIndex){

  //Variables
  int i;

  //Arrays
  if(Types[TypIndex].MstType==MasterType::FixArray || Types[TypIndex].MstType==MasterType::DynArray){
    if(Types[Types[TypIndex].ElemTypIndex].MstType==MasterType::String){
      return true;
    }
    else if(Types[Types[TypIndex].ElemTypIndex].MstType==MasterType::DynArray){
      return true;
    }
    else if(Types[Types[TypIndex].ElemTypIndex].MstType==MasterType::FixArray){
      return HasInnerBlocks(Types[TypIndex].ElemTypIndex);
    }
    else if(Types[Types[TypIndex].ElemTypIndex].MstType==MasterType::Class){
      return HasInnerBlocks(Types[TypIndex].ElemTypIndex);
    }
  }

  //class types
  else if(Types[TypIndex].MstType==MasterType::Class && Types[TypIndex].FieldLow!=-1 && Types[TypIndex].FieldHigh!=-1){
    i=-1;
    while((i=FieldLoop(TypIndex,i))!=-1){
      if(Types[Fields[i].TypIndex].MstType==MasterType::String){
        return true;
      }
      else if(Types[Fields[i].TypIndex].MstType==MasterType::DynArray){
        return true;
      }
      else if(Types[Fields[i].TypIndex].MstType==MasterType::FixArray){
        return HasInnerBlocks(Fields[i].TypIndex);
      }
      else if(Types[Fields[i].TypIndex].MstType==MasterType::Class){
        return HasInnerBlocks(Fields[i].TypIndex);
      }
    }
  }

  //If nothing above happens type has not inner blocks
  return false;

}

//Calculate CpuDataType from Master type
CpuDataType MasterData::CpuDataTypeFromMstType(MasterType MstType) const {
  CpuDataType Type;
  switch(MstType){
    case MasterType::Boolean : Type=CpuDataType::Boolean;   break;
    case MasterType::Char    : Type=CpuDataType::Char;      break;
    case MasterType::Short   : Type=CpuDataType::Short;     break;
    case MasterType::Integer : Type=CpuDataType::Integer;   break;
    case MasterType::Long    : Type=CpuDataType::Long;      break;
    case MasterType::Float   : Type=CpuDataType::Float;     break;
    case MasterType::String  : Type=CpuDataType::StrBlk;    break;
    case MasterType::Class   : Type=CpuDataType::Undefined; break;
    case MasterType::Enum    : Type=CpuDataType::Integer;   break;
    case MasterType::FixArray: Type=CpuDataType::Undefined; break;
    case MasterType::DynArray: Type=CpuDataType::ArrBlk;    break;
  }
  return Type;
}

//Check object is potentially undefined 
bool MasterData::UndefinedObject(int ModIndex) const {
  return (CompileToLibrary && ModIndex!=MainModIndex() && Modules[ModIndex].IsModLibrary?true:false);
}

//Returns source type (goes to original data type through all possible definitions)
int MasterData::GetSourceType(int TypIndex) const {
  int SourceTypIndex=TypIndex;
  while(Types[SourceTypIndex].IsTypedef){
    SourceTypIndex=Types[SourceTypIndex].OrigTypIndex;
  }
  return SourceTypIndex;
}

//Return parameter variable as assembler operand (only for local functions)
AsmArg MasterData::AsmPar(int ParmIndex) const {
  AsmArg Arg;
  Arg.IsNull=false;
  Arg.IsError=false;
  Arg.IsUndefined=false;
  Arg.AdrMode=CpuAdrMode::Address;
  Arg.Value.Adr=(Arg.IsUndefined?0:Parameters[ParmIndex].Address); 
  Arg.Glob=false;
  Arg.Name=_LocalParmName+HEXFORMAT(Parameters[ParmIndex].Address).TrimLeft('0');
  Arg.ObjectId="";
  Arg.ScopeDepth=-1;
  Arg.Type=CpuDataTypeFromMstType(Types[Parameters[ParmIndex].TypIndex].MstType);
  return Arg;
}

//Return variable as assembler operand
AsmArg MasterData::_AsmVarIndex(int VarIndex,bool Indirection) const {
  
  //Variables
  AsmArg Arg;
  
  //Set not null argument and no error
  Arg.IsNull=false;
  Arg.IsError=false;

  //Set addressing mode same as expression token
  Arg.AdrMode=(Indirection?CpuAdrMode::Indirection:CpuAdrMode::Address);

  //Set undefined flag
  Arg.IsUndefined=(Variables[VarIndex].Scope.Kind!=ScopeKind::Local && UndefinedObject(Variables[VarIndex].Scope.ModIndex)?true:false);

  //Set common attributes
  Arg.Value.Adr=(Arg.IsUndefined?0:Variables[VarIndex].Address); 
  Arg.Glob=(Variables[VarIndex].Scope.Kind==ScopeKind::Local && (!Variables[VarIndex].IsConst || (Variables[VarIndex].IsConst && Variables[VarIndex].IsReference)) && !Variables[VarIndex].IsStatic?false:true);
  Arg.Name=Variables[VarIndex].Name;
  Arg.ObjectId=Variables[VarIndex].Name+ObjIdSep+Modules[Variables[VarIndex].Scope.ModIndex].Name;
  Arg.ScopeDepth=-1;
  
  //Set data type
  Arg.Type=CpuDataTypeFromMstType(Types[Variables[VarIndex].TypIndex].MstType);

  //Return value
  return Arg;

}

//Return variable as assembler operand
AsmArg MasterData::AsmVar(int VarIndex) const {
  return _AsmVarIndex(VarIndex,false);
}

//Return variable address as assembler operand
AsmArg MasterData::AsmVad(int VarIndex) const {
  AsmArg Arg;
  Arg.IsNull=false;
  Arg.IsError=false;
  Arg.IsUndefined=false;
  Arg.AdrMode=CpuAdrMode::LitValue;
  Arg.ObjectId="";
  Arg.Type=CpuDataType::VarAddr; 
  Arg.ScopeDepth=-1;
  Arg.Name=Variables[VarIndex].Name;
  Arg.Value.Adr=Variables[VarIndex].Address;
  Arg.Glob=(Variables[VarIndex].Scope.Kind==ScopeKind::Local && (!Variables[VarIndex].IsConst || (Variables[VarIndex].IsConst && Variables[VarIndex].IsReference)) && !Variables[VarIndex].IsStatic?false:true);
  return Arg;
}

//Return null variable address as assembler operand
AsmArg MasterData::AsmNva() const {
  AsmArg Arg;
  Arg.IsNull=false;
  Arg.IsError=false;
  Arg.IsUndefined=false;
  Arg.AdrMode=CpuAdrMode::LitValue;
  Arg.ObjectId="";
  Arg.Type=CpuDataType::VarAddr; 
  Arg.ScopeDepth=-1;
  Arg.Name="$null";
  Arg.Value.Adr=0;
  Arg.Glob=true;
  return Arg;
}

//Return variable as assembler operand (indirection)
AsmArg MasterData::AsmInd(int VarIndex) const {
  return _AsmVarIndex(VarIndex,true);
}

//Return meta constant assembler operand
AsmArg MasterData::AsmMta(MetaConstCase Case,int Index) const {
  AsmArg Arg;
  Arg.IsNull=false;
  Arg.IsError=false;
  Arg.IsUndefined=false;
  Arg.AdrMode=CpuAdrMode::Address;
  switch(Case){
    case MetaConstCase::VarName:  
      Arg.Value.Adr=Variables[Index].MetaName;
      Arg.Name="STR("+ToString(Variables[Index].MetaName,"%0"+ToString((int)(sizeof(CpuAdr)*2))+"X")+"h)";
      Arg.ObjectId=OBJECTID_MVN":"+Variables[Index].Name+ObjIdSep+Modules[Variables[Index].Scope.ModIndex].Name;
      Arg.Type=CpuDataType::StrBlk; 
      break;
    case MetaConstCase::TypName:  
      Arg.Value.Adr=Types[Index].MetaName;
      Arg.Name="STR("+ToString(Types[Index].MetaName,"%0"+ToString((int)(sizeof(CpuAdr)*2))+"X")+"h)";
      Arg.ObjectId=OBJECTID_MTN":"+Types[Index].Name+ObjIdSep+Modules[Types[Index].Scope.ModIndex].Name;
      Arg.Type=CpuDataType::StrBlk; 
      break;
    case MetaConstCase::FldNames: 
      Arg.Value.Adr=Types[Index].MetaStNames;
      Arg.Name="ARR("+ToString(Types[Index].MetaStNames,"%0"+ToString((int)(sizeof(CpuAdr)*2))+"X")+"h)";
      Arg.ObjectId=OBJECTID_MFN":"+Types[Index].Name+ObjIdSep+Modules[Types[Index].Scope.ModIndex].Name;
      Arg.Type=CpuDataType::ArrBlk; 
      break;
    case MetaConstCase::FldTypes: 
      Arg.Value.Adr=Types[Index].MetaStTypes;
      Arg.Name="ARR("+ToString(Types[Index].MetaStTypes,"%0"+ToString((int)(sizeof(CpuAdr)*2))+"X")+"h)";
      Arg.ObjectId=OBJECTID_MFT":"+Types[Index].Name+ObjIdSep+Modules[Types[Index].Scope.ModIndex].Name;
      Arg.Type=CpuDataType::ArrBlk; 
      break;
  }
  Arg.Glob=true;
  Arg.ScopeDepth=-1;
  return Arg;
}

//Return array geometry as assembler operand
AsmArg MasterData::AsmAgx(int TypIndex) const {
  AsmArg Arg;
  int SourceTypIndex=GetSourceType(TypIndex);
  Arg.IsNull=false;
  Arg.IsError=false;
  Arg.IsUndefined=UndefinedObject(Types[SourceTypIndex].Scope.ModIndex);
  Arg.AdrMode=CpuAdrMode::LitValue;
  Arg.Name="";
  Arg.ObjectId=Types[SourceTypIndex].Name+ObjIdSep+Modules[Types[SourceTypIndex].Scope.ModIndex].Name;
  Arg.Type=CpuDataType::ArrGeom; 
  Arg.Value.Agx=Dimensions[Types[SourceTypIndex].DimIndex].GeomIndex;
  Arg.Glob=(Types[SourceTypIndex].Scope.Kind==ScopeKind::Local?false:true);
  Arg.ScopeDepth=-1;
  return Arg;
}

//Return function code address as assembler operand
//(nested functions are treated as undefined to force they are solved by forward call resolution
//because merging of code buffers modifies its address)
AsmArg MasterData::AsmFun(int FunIndex) const {
  AsmArg Arg;
  Arg.IsNull=false;
  Arg.IsError=false;
  Arg.IsUndefined=UndefinedObject(Functions[FunIndex].Scope.ModIndex);
  Arg.AdrMode=CpuAdrMode::LitValue;
  Arg.Name=Functions[FunIndex].Fid;
  Arg.ObjectId=FunctionSearchName(FunIndex)+"("+GetFunctionParameterList(FunIndex,false)+")"+ObjIdSep+Modules[Functions[FunIndex].Scope.ModIndex].Name;
  Arg.Type=CpuDataType::FunAddr; 
  Arg.Value.Adr=(Arg.IsUndefined || Functions[FunIndex].IsNested?0:Functions[FunIndex].Address);
  Arg.ScopeDepth=Functions[FunIndex].Scope.Depth;
  return Arg;
}

//Return subroutine address as assembler operand
AsmArg MasterData::AsmSub(CpuAdr Address,const String& Module,const String& Name,int ScopeDepth) const {
  return Bin.SubroutineAsmArg(Address,Module,Name,ScopeDepth);
}

//Return jump code address as assembler operand
//(Jump addresses are not solved at this point)
AsmArg MasterData::AsmJmp(const String& Label) const {
  AsmArg Arg;
  Arg.IsNull=false;
  Arg.IsError=false;
  Arg.IsUndefined=false;
  Arg.AdrMode=CpuAdrMode::LitValue;
  Arg.Name=Label;
  Arg.ObjectId="";
  Arg.Type=CpuDataType::JumpAddr; 
  Arg.Value.Adr=0;
  Arg.ScopeDepth=_ScopeStk.Top().Scope.Depth;
  return Arg;
}

//Return error assembler operand
AsmArg MasterData::AsmErr() const {
  AsmArg Arg;
  Arg.IsNull=false;
  Arg.IsError=true;
  return Arg;
}

//Write function to assembler file
void MasterData::AsmWriteFunction(int FunIndex){
  
  //Emit function header
  Bin.AsmOutLine(AsmSection::Head,"("+Functions[FunIndex].Fid+")","FUNCTION");

  //Emit assembler body footer
  Bin.AsmOutNewLine(AsmSection::Body);
  Bin.AsmOutCommentLine(AsmSection::Body,"End function",true);
  Bin.AsmOutLine(AsmSection::Body,"","END");

}

//Emit function declaration in assembler
void MasterData::AsmFunDeclaration(int FunIndex,bool Import,bool Export,const String& Library){
  
  //Variables
  int i;
  bool Local;
  AsmSection Section;
  String AsmParms="";

  //Section is DECL for public/private functions and HEAD for local functions
  if(CurrentScope().Kind==ScopeKind::Local){
    Local=true;
    Section=AsmSection::Head;
  }
  else{
    Local=false;
    Section=AsmSection::Decl;
  }

  //Get parameter list
  if(Functions[FunIndex].ParmNr!=0){
    for(i=Functions[FunIndex].ParmLow;i<=Functions[FunIndex].ParmHigh;i++){
      AsmParms+=(AsmParms.Length()!=0?",":"")+Bin.GetAsmParameter(Parameters[i].IsReference,Parameters[i].IsConst,CpuDataTypeFromMstType(Types[Parameters[i].TypIndex].MstType),Parameters[i].Name);
    }
  }

  //Emit declaration
  if(Functions[FunIndex].Kind==FunctionKind::Function || Functions[FunIndex].Kind==FunctionKind::Member || Functions[FunIndex].Kind==FunctionKind::Operator){
    Bin.AsmOutFunDecl(Section,Functions[FunIndex].Fid,AsmParms,Import,Export,Local,Library,Functions[FunIndex].Address,"");
  }
  else if(Functions[FunIndex].Kind==FunctionKind::SysCall){
    Bin.AsmOutFunDecl(Section,Functions[FunIndex].Fid,AsmParms,Import,Export,Local,Library,0,"SCALL("+ToString(Functions[FunIndex].SysCallNr)+")");
  }
  else if(Functions[FunIndex].Kind==FunctionKind::SysFunc){
    Bin.AsmOutFunDecl(Section,Functions[FunIndex].Fid,AsmParms,Import,Export,Local,Library,0,"INST("+Bin.GetMnemonic(Functions[FunIndex].InstCode)+")");
  }
  else if(Functions[FunIndex].Kind==FunctionKind::DlFunc){
    Bin.AsmOutFunDecl(Section,Functions[FunIndex].Fid,AsmParms,Import,Export,Local,Library,0,"DCALL("+Functions[FunIndex].DlName+":"+Functions[FunIndex].DlFunction+")");
  }

}

//Emit module initialization routine header
void MasterData::AsmInitHeader(int ModIndex){
  Bin.AsmOutNewLine(AsmSection::Star);
  Bin.AsmOutLine(AsmSection::Star,"("+GetInitFunctionName(Modules[ModIndex].Name)+")","FUNCTION");
}

//Emit assembler exports
void MasterData::AsmExports(void){
  
  //Variables
  int i;
  
  //Exports header
  Bin.AsmOutSeparator(AsmSection::Decl);
  Bin.AsmOutCommentLine(AsmSection::Decl,"Exports",false);
  Bin.AsmOutSeparator(AsmSection::Decl);

  //Emit variable exports
  Bin.AsmOutNewLine(AsmSection::Decl);
  Bin.AsmOutCommentLine(AsmSection::Decl,"Variable exports",true);
  for(i=0;i<Variables.Length();i++){
    if(Variables[i].LnkSymIndex!=-1 && !Variables[i].IsParameter){
      Bin.AsmOutVarDecl(AsmSection::Decl,(Variables[i].Scope.Kind!=ScopeKind::Local?true:false),false,Variables[i].IsReference,Variables[i].IsConst,Variables[i].IsStatic,
      Variables[i].Name,CpuDataTypeFromMstType(Types[Variables[i].TypIndex].MstType),VarLength(i),Variables[i].Address,"",false,true,"","");
    }
  }

  //Emit function exports
  Bin.AsmOutNewLine(AsmSection::Decl);
  Bin.AsmOutCommentLine(AsmSection::Decl,"Function exports",true);
  for(i=0;i<Functions.Length();i++){
    if(Functions[i].LnkSymIndex!=-1){
      AsmFunDeclaration(i,false,true,"");
    }
  }

  //Super initialization routine
  Bin.AsmOutFunDecl(AsmSection::Decl,GetSuperInitFunctionName(),"",false,true,false,"",Bin.GetSuperInitAddress(),"");
  
}

//Emit assembler exports
void MasterData::AsmDlCallIds(void){
  
  //No Dl Calls to print
  if(Bin.CurrentDlCallId()==0){ return; }

  //Header
  Bin.AsmOutNewLine(AsmSection::Foot);
  Bin.AsmOutSeparator(AsmSection::Foot);
  Bin.AsmOutCommentLine(AsmSection::Foot,"Dynamic library call ids on executable file",false);
  Bin.AsmOutSeparator(AsmSection::Foot);
  
  //Output DlCall Ids
  Bin.AsmOutDlCallTable(AsmSection::Foot);

}

//Emit super initialization routine
bool MasterData::GenerateSuperInitRoutine(){
  
  //Only generated for main module
  if(CurrentScope().ModIndex!=MainModIndex()){ return true; }
  
  //Assembler header
  Bin.AsmOutNewLine(AsmSection::Body);
  Bin.AsmOutLine(AsmSection::Body,"("+GetSuperInitFunctionName()+")","FUNCTION");

  //Save super init routine address
  CpuAdr Address=Bin.CurrentCodeAddress();
  Bin.SetSuperInitAddress(Address,Modules[MainModIndex()].Name,GetSuperInitFunctionName());

  //Emit calls to initialization routines
  if(!Bin.CallInitRoutines()){ return false; }

  //Emit ending return
  if(!Bin.AsmWriteCode(CpuInstCode::RET)){ return false; }

  //Save debug symbol
  if(DebugSymbols){
    Bin.StoreDbgSymFunction((CpuChr)FunctionKind::Function,GetSuperInitFunctionName(),(CpuInt)Modules[MainModIndex()].DbgSymIndex,
    (CpuLon)Address,(CpuLon)(Bin.CurrentCodeAddress()-1),(CpuInt)-1,(CpuBol)true,(CpuInt)0,(CpuInt)-1,(CpuInt)-1,SourceInfo());
  }

  //Return code
  return true;

}

//Generate start code
bool MasterData::GenerateStartCode(){

  //Header
  Bin.AsmOutNewLine(AsmSection::Star);
  Bin.StoreJumpDestination(GetStartFunctionName(),CurrentScope().Depth,Bin.CurrentCodeAddress());
  
  //Set call to super init routine
  if(!Bin.AsmWriteCode(CpuInstCode::CALL,AsmSection::Star,AsmSub(0,Modules[MainModIndex()].Name,GetSuperInitFunctionName(),0))){ return false; } 

  //Unlock machine scopes (before this instruction machine scope does not change to avoid release of blocks, since they may belong to global variables)
  if(!Bin.AsmWriteCode(CpuInstCode::SULOK,AsmSection::Star)){ return false; }    
  
  //Set call to main program
  if(!Bin.AsmWriteCode(CpuInstCode::CALL,AsmSection::Star,AsmSub(0,Modules[MainModIndex()].Name,GetMainFunctionName(),0))){ return false; } 
  
  //Set system call program exit
  if(!Bin.AsmWriteCode(CpuInstCode::SCALL,AsmSection::Star,Bin.AsmLitInt((int)SystemCall::ProgramExit))){ return false; }

  //Return code
  return true;

}

//Get master type name
String MasterTypeName(MasterType MstType){
  String Name;
  switch(MstType){
    case MasterType::Boolean : Name="boolean";  break;
    case MasterType::Char    : Name="char";     break;
    case MasterType::Short   : Name="short";    break;
    case MasterType::Integer : Name="integer";  break;
    case MasterType::Long    : Name="long";     break;
    case MasterType::Float   : Name="float";    break;
    case MasterType::String  : Name="string";   break;
    case MasterType::Class   : Name="class";    break;
    case MasterType::Enum    : Name="enum";     break;
    case MasterType::FixArray: Name="fixarray"; break;
    case MasterType::DynArray: Name="dynarray"; break;
  }
  return Name;
}

//Grant kind short name
String GrantKindShortName(GrantKind Kind){
  
  //Variables
  String Name;

  //From name
  switch(Kind){
    case GrantKind::ClassToClass: Name="CLCL"; break;
    case GrantKind::ClassToField: Name="CLFI"; break;
    case GrantKind::ClassToFunMb: Name="CLFM"; break;
    case GrantKind::FunctToClass: Name="FUCL"; break;
    case GrantKind::FunctToField: Name="FUFI"; break;
    case GrantKind::FunctToFunMb: Name="FUFM"; break;
    case GrantKind::FunMbToClass: Name="FMCL"; break;
    case GrantKind::FunMbToField: Name="FMFI"; break;
    case GrantKind::FunMbToFunMb: Name="FMFM"; break;
    case GrantKind::OpertToClass: Name="OPCL"; break;
    case GrantKind::OpertToField: Name="OPFI"; break;
    case GrantKind::OpertToFunMb: Name="OPFM"; break;
  }

  //Return name
  return Name;

}

//Field loop next field
//(First iteration must pass FldIndex=-1)
int MasterData::FieldLoop(int TypIndex,int FldIndex) const {
  
  //Variables
  int TrueTypIndex;
  int NewFldIndex;

  //Get true type index
  TrueTypIndex=TypIndex;
  while(Types[TrueTypIndex].OrigTypIndex!=-1){ TrueTypIndex=Types[TrueTypIndex].OrigTypIndex; }

  //Exit on invalid field indexes
  if(Types[TrueTypIndex].FieldLow==-1 || Types[TrueTypIndex].FieldHigh==-1){ return -1; }

  //Find first field
  if(FldIndex==-1){
    NewFldIndex=-1;
    for(int i=Types[TrueTypIndex].FieldLow;i<=Types[TrueTypIndex].FieldHigh;i++){
      if(Fields[i].SubScope.TypIndex==TrueTypIndex){ NewFldIndex=i; break; }
    }
  }

  //Find next field
  else{
    NewFldIndex=-1;
    for(int i=FldIndex+1;i<=Types[TrueTypIndex].FieldHigh;i++){
      if(Fields[i].SubScope.TypIndex==TrueTypIndex){ NewFldIndex=i; break; }
    }
  }

  //Return field
  return NewFldIndex;

}

//Field cound
int MasterData::FieldCount(int TypIndex) const {
  
  //Variables
  int TrueTypIndex;
  int FieldCount;

  //Get true type index
  TrueTypIndex=TypIndex;
  while(Types[TrueTypIndex].OrigTypIndex!=-1){ TrueTypIndex=Types[TrueTypIndex].OrigTypIndex; }

  //Exit on invalid field indexes
  if(Types[TrueTypIndex].FieldLow==-1 || Types[TrueTypIndex].FieldHigh==-1){ return 0; }

  //Count fields
  FieldCount=0;
  for(int i=Types[TrueTypIndex].FieldLow;i<=Types[TrueTypIndex].FieldHigh;i++){
    if(Fields[i].SubScope.TypIndex==TrueTypIndex){ FieldCount++; }
  }

  //Return field count
  return FieldCount;

}

//Function member loop next
//(First iteration must pass FunIndex=-1)
int MasterData::MemberLoop(int TypIndex,int FunIndex) const {
  
  //Variables
  int TrueTypIndex;
  int NewFunIndex;

  //Get true type index
  TrueTypIndex=TypIndex;
  while(Types[TrueTypIndex].OrigTypIndex!=-1){ TrueTypIndex=Types[TrueTypIndex].OrigTypIndex; }

  //Exit on invalid member indexes
  if(Types[TrueTypIndex].MemberLow==-1 || Types[TrueTypIndex].MemberHigh==-1){ return -1; }

  //Find first member
  if(FunIndex==-1){
    NewFunIndex=-1;
    for(int i=Types[TrueTypIndex].MemberLow;i<=Types[TrueTypIndex].MemberHigh;i++){
      if(Functions[i].SubScope.TypIndex==TrueTypIndex){ NewFunIndex=i; break; }
    }
  }

  //Find next member
  else{
    NewFunIndex=-1;
    for(int i=FunIndex+1;i<=Types[TrueTypIndex].MemberHigh;i++){
      if(Functions[i].SubScope.TypIndex==TrueTypIndex){ NewFunIndex=i; break; }
    }
  }

  //Return function member
  return NewFunIndex;

}

//Replace static variable names in code according to current scope
Sentence MasterData::ReplaceStaticVarNames(const ScopeDef& Scope,const Sentence& Stn) const {

  //Variables
  bool Replace;
  Sentence WorkStn=Stn;

  //Loop over sentence 
  for(int i=0;i<WorkStn.Tokens.Length();i++){

    //Filter identifiers
    if(WorkStn.Tokens[i].Id()==PrTokenId::Identifier){

      //Loop over current scope variables
      for(int j=Variables.Length()-1;j>=0;j--){
        
        //Variables of current scope
        if(Variables[j].Scope==Scope){

          //Found static variable that matches
          if(Variables[j].IsStatic && WorkStn.Tokens[i].Value.Idn==Variables[j].Name){

            //Check identifier is not a function name
            if(i+1<=WorkStn.Tokens.Length()-1){
              if(Stn.Tokens[i+1].Id()==PrTokenId::Punctuator && Stn.Tokens[i+1].Value.Pnc==PrPunctuator::BegParen){ Replace=false; }
              else{ Replace=true; }
            }
            else if(i==WorkStn.Tokens.Length()-1){
              Replace=true; 
            }

            //Change static variables and set used flag variable
            if(Replace){
              WorkStn.Tokens[i].Value.Idn=GetStaticVarName(Scope.FunIndex,Variables[j].Name);
              Variables[j].IsSourceUsed=true;
              DebugMessage(DebugLevel::CmpExpression,"Source used flag set on variable "+Variables[j].Name+" in scope "+ScopeName(Scope));
            }

          }

        }

        //Exit if reached another scope
        else{
          break;
        }

      }

    }

  }

  //Return changed sentence
  return WorkStn;

}