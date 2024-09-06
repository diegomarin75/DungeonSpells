//arrcomp.cpp: Array computer class
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
#include "vrm/arrcomp.hpp"

//string[] array operation
enum class StaOperation{
  Read=1,
  Write=2
};

//Internal global variables
CpuMbl _StaArrBlock=-1;
StaOperation _StaOpr=(StaOperation)0;
int _StaArrIndex=-1;
CpuWrd _StaLines=0;
CpuWrd _StaWrittenLines=0;
CpuWrd _StaReservedLines=0;
const CpuWrd _StaChunkLines=512;

//Internal funtion prototypes
void _CalculateDimensionFactors(int DimNr,CpuWrd CellSize,ArrayIndexes DimSize,CpuWrd *Factor);
CpuWrd _CalculateArrayOffset(int DimNr,CpuWrd *Factor,ArrayIndexes DimValue);

//Calculate array dimension factors
void _CalculateDimensionFactors(int DimNr,CpuWrd CellSize,ArrayIndexes DimSize,CpuWrd *Factor){

  //Switch on array number of dimensions
  switch(DimNr){
    
    //1-Dimension array
    case 1:
      Factor[0]=CellSize*(1);
      break;
  
    //2-Dimension array
    case 2:
      Factor[0]=CellSize*(DimSize.n[1]);
      Factor[1]=CellSize*(1);
      break;
  
    //3-Dimension array
    case 3:
      Factor[0]=CellSize*(DimSize.n[2]*DimSize.n[1]);
      Factor[1]=CellSize*(DimSize.n[2]);
      Factor[2]=CellSize*(1);
      break;
  
    //4-Dimension array
    case 4:
      Factor[0]=CellSize*(DimSize.n[3]*DimSize.n[2]*DimSize.n[1]);
      Factor[1]=CellSize*(DimSize.n[3]*DimSize.n[2]);
      Factor[2]=CellSize*(DimSize.n[3]);
      Factor[3]=CellSize*(1);
      break;
  
    //5-Dimension array
    case 5:
      Factor[0]=CellSize*(DimSize.n[4]*DimSize.n[3]*DimSize.n[2]*DimSize.n[1]);
      Factor[1]=CellSize*(DimSize.n[4]*DimSize.n[3]*DimSize.n[2]);
      Factor[2]=CellSize*(DimSize.n[4]*DimSize.n[3]);
      Factor[3]=CellSize*(DimSize.n[4]);
      Factor[4]=CellSize*(1);
      break;
  
  }

}

//Calculate array element offset
CpuWrd _CalculateArrayOffset(int DimNr,CpuWrd *Factor,ArrayIndexes DimValue){

  //Variables
  CpuWrd Offset;

  //Switch on array number of dimensions
  switch(DimNr){
    case 1: Offset=DimValue.n[0]*Factor[0]; break;
    case 2: Offset=DimValue.n[1]*Factor[1]+DimValue.n[0]*Factor[0]; break;
    case 3: Offset=DimValue.n[2]*Factor[2]+DimValue.n[1]*Factor[1]+DimValue.n[0]*Factor[0]; break;
    case 4: Offset=DimValue.n[3]*Factor[3]+DimValue.n[2]*Factor[2]+DimValue.n[1]*Factor[1]+DimValue.n[0]*Factor[0]; break;
    case 5: Offset=DimValue.n[4]*Factor[4]+DimValue.n[3]*Factor[3]+DimValue.n[2]*Factor[2]+DimValue.n[1]*Factor[1]+DimValue.n[0]*Factor[0]; break;
  }

  //Return value
  return Offset;

}

//Move to next element
void ArrayComputer::_FixMoveNext(CpuAgx AbsGeom,ArrayIndexes& DimValue,bool& LastReached){

  //Variables
  int i;

  //Next element loop over dimensions starting with lowest one
  LastReached=false;
  for(i=_ArrGeom[AbsGeom].DimNr-1;i>=0;i--){
    DimValue.n[i]++;
    if(DimValue.n[i]==_ArrGeom[AbsGeom].DimSize.n[i]-1){
      if(i==0){
        LastReached=true;
        break;
      }
    }
    else if(DimValue.n[i]>_ArrGeom[AbsGeom].DimSize.n[i]-1){
      DimValue.n[i]=0;
    }
    else{
      break;
    }
  }

}

//Check array has element
bool ArrayComputer::_FixHasElement(CpuAgx AbsGeom,ArrayIndexes& DimValue){

  //Variables
  int i;
  bool HasElement;

  //Next element loop over dimensions starting with lowest one
  HasElement=true;
  for(i=0;i<_ArrGeom[AbsGeom].DimNr;i++){
    if(DimValue.n[i]>_ArrGeom[AbsGeom].DimSize.n[i]-1){
      HasElement=false;
      break;
    }
  }

  //Return result
  return HasElement;

}

//Initialize fixed array computer
void ArrayComputer::FixInit(int ProcessId,long ArrGeomChunkSize){
  _ArrGeom.Init(ProcessId,ArrGeomChunkSize,(char *)"_ArrGeom");
}

//Calculate array number of elements
CpuWrd ArrayComputer::FixGetElements(CpuAgx ArrGeom){

  //Variables
  CpuWrd Elements;
  CpuAgx AbsGeom=_FixAgxDecoder(ArrGeom);

  //Switch on array number of dimensions
  switch(_ArrGeom[AbsGeom].DimNr){
    case 1: Elements=_ArrGeom[AbsGeom].DimSize.n[0]; break;
    case 2: Elements=_ArrGeom[AbsGeom].DimSize.n[0]*_ArrGeom[AbsGeom].DimSize.n[1]; break;
    case 3: Elements=_ArrGeom[AbsGeom].DimSize.n[0]*_ArrGeom[AbsGeom].DimSize.n[1]*_ArrGeom[AbsGeom].DimSize.n[2]; break;
    case 4: Elements=_ArrGeom[AbsGeom].DimSize.n[0]*_ArrGeom[AbsGeom].DimSize.n[1]*_ArrGeom[AbsGeom].DimSize.n[2]*_ArrGeom[AbsGeom].DimSize.n[3]; break;
    case 5: Elements=_ArrGeom[AbsGeom].DimSize.n[0]*_ArrGeom[AbsGeom].DimSize.n[1]*_ArrGeom[AbsGeom].DimSize.n[2]*_ArrGeom[AbsGeom].DimSize.n[3]*_ArrGeom[AbsGeom].DimSize.n[4]; break;
  }  

  //Return result
  return Elements;

}

//Store array metadata
bool ArrayComputer::FixStoreGeom(int DimNr,CpuWrd CellSize,ArrayIndexes DimSize){
  ArrayGeometry Geom;
  Geom=(ArrayGeometry){DimNr,CellSize,DimSize,{0},0,0,CpuDecMode::LoclVar,0};
  if(!_ArrGeom.Push(Geom)){ return false; }
  return true;
}

//Get string print of array (for debug messages)
String ArrayComputer::FixPrint(CpuAgx ArrGeom){
  String Result;
  CpuAgx AbsGeom=_FixAgxDecoder(ArrGeom);
  if(AbsGeom<=_ArrGeom.Length()-1){
    Result="fixarray(absgeom="+ToString(AbsGeom)+
    ",cellsz="+ToString(_ArrGeom[AbsGeom].CellSize)+
    ",loopix="+ToString(_ArrGeom[AbsGeom].LoopIndex)+
    ",ixvar="+NZHEXFORMAT(_ArrGeom[AbsGeom].IndexVarAddr)+(_ArrGeom[AbsGeom].IndexVarMode==CpuDecMode::GlobVar?"(glob)":"(locl)")+
    ",exitadr="+NZHEXFORMAT(_ArrGeom[AbsGeom].ExitAdr)+
    ")[";
    for(int i=1;i<=_ArrGeom[AbsGeom].DimNr;i++){
      Result+=ToString(_ArrGeom[AbsGeom].DimSize.n[i-1]);
      if(i<_ArrGeom[AbsGeom].DimNr){ Result+=","; }
    }
    Result+="]";
  }
  else{
    Result="fixarray(absgeom="+ToString(AbsGeom)+")";
  }
  return Result;
}

//Create array geometry index in table
bool ArrayComputer::_FixCreateIndex(CpuAgx AbsGeom){
  if(AbsGeom>_ArrGeom.Length()-1){
    while(AbsGeom>_ArrGeom.Length()-1){
      if(!_ArrGeom.Push((ArrayGeometry){-1,-1,{0},{0},0,0,CpuDecMode::LoclVar,0})){ return false; }
    }
    return true;
  }
  else{
    return true;
  }
}

//Calculate offset for 1-dimensional array
bool ArrayComputer::AF1OF(CpuAgx ArrGeom,CpuWrd DimValue,CpuWrd *Offset){

  //Variables
  CpuAgx AbsGeom=_FixAgxDecoder(ArrGeom);

  //Check index is defined
  if(AbsGeom>_ArrGeom.Length()-1){
    System::Throw(SysExceptionCode::ArrayGeometryInvalidIndex,ToString(AbsGeom),ToString(_ArrGeom.Length()-1)); 
    return false;
  }

  //Check validity of input parameters
  if(DimValue<0 || DimValue>_ArrGeom[AbsGeom].DimSize.n[0]-1){
    System::Throw(SysExceptionCode::ArrayGeometryIndexingOutOfBounds,"1",ToString(_ArrGeom[AbsGeom].DimSize.n[0]),ToString(DimValue)); 
    return false;
  }

  //Calculate offset
  *Offset=_ArrGeom[AbsGeom].CellSize*DimValue;

  //Return code
  return true;

}

//Rewind array to prepare loop
bool ArrayComputer::AF1RW(CpuAgx ArrGeom,CpuAdr IndexVarAddr,CpuDecMode IndexVarMode,CpuAdr ExitAdr){

  //Variables
  CpuAgx AbsGeom=_FixAgxDecoder(ArrGeom);

  //Check index is defined
  if(AbsGeom>_ArrGeom.Length()-1){
    System::Throw(SysExceptionCode::ArrayGeometryInvalidIndex,ToString(AbsGeom),ToString(_ArrGeom.Length()-1)); 
    return false;
  }

  //Reset loop variables
  _ArrGeom[AbsGeom].LoopIndex=0;
  _ArrGeom[AbsGeom].IndexVarAddr=IndexVarAddr;
  _ArrGeom[AbsGeom].IndexVarMode=IndexVarMode;
  _ArrGeom[AbsGeom].ExitAdr=ExitAdr;

  //Return code
  return true;
  
}

//Array for loop begin
bool ArrayComputer::AF1FO(CpuAgx ArrGeom,CpuWrd *Offset,CpuAdr *JumpAdr){

  //Variables
  CpuAgx AbsGeom=_FixAgxDecoder(ArrGeom);

  //Check index is defined
  if(AbsGeom>_ArrGeom.Length()-1){
    System::Throw(SysExceptionCode::ArrayGeometryInvalidIndex,ToString(AbsGeom),ToString(_ArrGeom.Length()-1)); 
    return false;
  }

  //Check loop end condition
  if(_ArrGeom[AbsGeom].LoopIndex>=_ArrGeom[AbsGeom].DimSize.n[0]){
    
    //Return exit adress
    *JumpAdr=_ArrGeom[AbsGeom].ExitAdr;

  }

  //Return offset and move to next element
  else{

    //Calculate offset
    *Offset=_ArrGeom[AbsGeom].CellSize*_ArrGeom[AbsGeom].LoopIndex;

    //Set exit address to zero
    *JumpAdr=0;

  }

  //Return code
  return true;

}

//Array for loop next element
bool ArrayComputer::AF1NX(CpuAgx ArrGeom,CpuAdr *IndexVarAddr,CpuDecMode *IndexVarMode){

  //Variables
  CpuAgx AbsGeom=_FixAgxDecoder(ArrGeom);

  //Check index is defined
  if(AbsGeom>_ArrGeom.Length()-1){
    System::Throw(SysExceptionCode::ArrayGeometryInvalidIndex,ToString(AbsGeom),ToString(_ArrGeom.Length()-1)); 
    return false;
  }

  //Move to next element
  _ArrGeom[AbsGeom].LoopIndex++;

  //Return index variable
  *IndexVarAddr=_ArrGeom[AbsGeom].IndexVarAddr;
  *IndexVarMode=_ArrGeom[AbsGeom].IndexVarMode;

  //Return code
  return true;

}

//Define array cellsize and dimensions
bool ArrayComputer::AFDEF(CpuAgx ArrGeom,int DimNr,CpuWrd CellSize){

  //Variables
  CpuAgx AbsGeom=_FixAgxDecoder(ArrGeom);

  //Make index available in table
  if(!_FixCreateIndex(AbsGeom)){
    System::Throw(SysExceptionCode::ArrayGeometryMemoryOverflow,ToString(AbsGeom)); 
    return false;
  }

  //Save array metadata
  _ArrGeom[AbsGeom].DimNr=DimNr;
  _ArrGeom[AbsGeom].CellSize=CellSize;

  //Return code
  return true;
  
}

//Define array dimension size
bool ArrayComputer::AFSSZ(CpuAgx ArrGeom,int DimIndex,CpuWrd Size){

  //Variables
  CpuAgx AbsGeom=_FixAgxDecoder(ArrGeom);

  //Check index is defined
  if(AbsGeom>_ArrGeom.Length()-1){
    System::Throw(SysExceptionCode::ArrayGeometryInvalidIndex,ToString(AbsGeom),ToString(_ArrGeom.Length()-1)); 
    return false;
  }

  //Check array has defined requested dimension
  if(DimIndex<1 || DimIndex>_ArrGeom[AbsGeom].DimNr){
    System::Throw(SysExceptionCode::ArrayGeometryInvalidDimension,ToString(AbsGeom),ToString(DimIndex),ToString(_ArrGeom[AbsGeom].DimNr)); 
    return false;
  }

  //Save array metadata
  _ArrGeom[AbsGeom].DimSize.n[DimIndex-1]=Size;

  //Return code
  return true;
  
}

//Get array dimension size
bool ArrayComputer::AFGET(CpuAgx ArrGeom,int DimIndex,CpuWrd *Size){

  //Variables
  CpuAgx AbsGeom=_FixAgxDecoder(ArrGeom);

  //Check index is defined
  if(AbsGeom>_ArrGeom.Length()-1){
    System::Throw(SysExceptionCode::ArrayGeometryInvalidIndex,ToString(AbsGeom),ToString(_ArrGeom.Length()-1)); 
    return false;
  }

  //Check array has defined requested dimension
  if(DimIndex<1 || DimIndex>_ArrGeom[AbsGeom].DimNr){
    System::Throw(SysExceptionCode::ArrayGeometryInvalidDimension,ToString(AbsGeom),ToString(DimIndex),ToString(_ArrGeom[AbsGeom].DimNr)); 
    return false;
  }

  //Save array metadata
  *Size=_ArrGeom[AbsGeom].DimSize.n[DimIndex-1];

  //Return code
  return true;
  
}

//Set dimension value
bool ArrayComputer::AFIDX(CpuAgx ArrGeom,int DimIndex,CpuWrd DimValue){

  //Variables
  CpuAgx AbsGeom=_FixAgxDecoder(ArrGeom);

  //Check index is defined
  if(AbsGeom>_ArrGeom.Length()-1){
    System::Throw(SysExceptionCode::ArrayGeometryInvalidIndex,ToString(AbsGeom),ToString(_ArrGeom.Length()-1)); 
    return false;
  }

  //Check array has defined requested dimension
  if(DimIndex<1 || DimIndex>_ArrGeom[AbsGeom].DimNr){
    System::Throw(SysExceptionCode::ArrayGeometryInvalidDimension,ToString(AbsGeom),ToString(DimIndex),ToString(_ArrGeom[AbsGeom].DimNr)); 
    return false;
  }

  //Check validity of input parameters
  if(DimValue<0 || DimValue>_ArrGeom[AbsGeom].DimSize.n[DimIndex-1]-1){
    System::Throw(SysExceptionCode::ArrayGeometryIndexingOutOfBounds,ToString(DimIndex),ToString(_ArrGeom[AbsGeom].DimSize.n[DimIndex-1]),ToString(DimValue)); 
    return false;
  }

  //Save array metadata
  _ArrGeom[AbsGeom].DimValue.n[DimIndex-1]=DimValue;

  //Return code
  return true;
  
}

//Calculate offset for n-dimensional array
bool ArrayComputer::AFOFN(CpuAgx ArrGeom,CpuWrd *Offset){

  //Variables
  CpuAgx AbsGeom=_FixAgxDecoder(ArrGeom);
  CpuWrd Factor[_MaxArrayDims];

  //Check index is defined
  if(AbsGeom>_ArrGeom.Length()-1){
    System::Throw(SysExceptionCode::ArrayGeometryInvalidIndex,ToString(AbsGeom),ToString(_ArrGeom.Length()-1)); 
    return false;
  }

  //Calculate array element offset
  _CalculateDimensionFactors(_ArrGeom[AbsGeom].DimNr,_ArrGeom[AbsGeom].CellSize,_ArrGeom[AbsGeom].DimSize,Factor);
  *Offset=_CalculateArrayOffset(_ArrGeom[AbsGeom].DimNr,Factor,_ArrGeom[AbsGeom].DimValue);

  //Return code
  return true;
  
}

//1-dim string fix array join
bool ArrayComputer::AF1SJ(CpuMbl *Str,char *Data,CpuAgx ArrGeom,CpuMbl Sep){

  //Variables
  CpuWrd i;
  CpuWrd SepLen;
  CpuMbl Elem;
  CpuAgx AbsGeom=_FixAgxDecoder(ArrGeom);

  //Check index is defined
  if(AbsGeom>_ArrGeom.Length()-1){
    System::Throw(SysExceptionCode::ArrayGeometryInvalidIndex,ToString(AbsGeom),ToString(_ArrGeom.Length()-1)); 
    return false;
  }

  //Get separator length
  SepLen=_Aux->GetLen(Sep);

  //Join first array element
  if(_ArrGeom[AbsGeom].DimSize.n[0]>0){
    Elem=*(CpuMbl *)&Data[0];
    if(Elem!=0){ if(!_StC->SCOPY(Str,Elem)){ return false; } }
  }

  //Join rest of array elements
  for(i=1;i<_ArrGeom[AbsGeom].DimSize.n[0];i++){
    if(SepLen!=0){ if(!_StC->SAPPN(Str,Sep)){ return false; } }
    Elem=*(CpuMbl *)&Data[i*sizeof(CpuMbl)];
    if(Elem!=0){ if(!_StC->SAPPN(Str,Elem)){ return false; } }
  }

  //Return code
  return true;

}

//1-dim char fix array join
bool ArrayComputer::AF1CJ(CpuMbl *Str,char *Data,CpuAgx ArrGeom,CpuMbl Sep){

  //Variables
  CpuWrd i;
  CpuWrd SepLen;
  CpuAgx AbsGeom=_FixAgxDecoder(ArrGeom);

  //Check index is defined
  if(AbsGeom>_ArrGeom.Length()-1){
    System::Throw(SysExceptionCode::ArrayGeometryInvalidIndex,ToString(AbsGeom),ToString(_ArrGeom.Length()-1)); 
    return false;
  }

  //Get separator length
  SepLen=_Aux->GetLen(Sep);

  //Join first array element
  if(_ArrGeom[AbsGeom].DimSize.n[0]>0){
    if(!_StC->SCOPY(Str,&Data[0],1)){ return false; }
  }

  //Join rest of array elements
  for(i=1;i<_ArrGeom[AbsGeom].DimSize.n[0];i++){
    if(SepLen!=0){ if(!_StC->SAPPN(Str,Sep)){ return false; } }
    if(!_StC->SAPPN(Str,&Data[i],1)){ return false; }
  }

  //Return code
  return true;

}

//Find new ArrIndex
int ArrayComputer::_DynNewArrIndex(){
  int i;
  int ArrIndex=-1;
  for(i=0;i<_ArrMeta.Length();i++){
    if(!_ArrMeta[i].Used || _ArrMeta[i].ScopeId>_ScopeId || (_ArrMeta[i].ScopeId==_ScopeId && _ArrMeta[i].ScopeNr!=_ScopeNr)){ ArrIndex=i; break; }
  }
  if(ArrIndex==-1){
    if(!_ArrMeta.Push((ArrayMeta){_ScopeId,_ScopeNr,true,0,0,{0},{0},{0}})){ return -1; }
    ArrIndex=_ArrMeta.Length()-1;
    for(i=0;i<_MaxArrayDims;i++){ _ArrMeta[ArrIndex].DimSize.n[i]=0; _ArrMeta[ArrIndex].DimValue.n[i]=0;}
  }
  _ArrMeta[ArrIndex].ScopeId=_ScopeId;
  _ArrMeta[ArrIndex].ScopeNr=_ScopeNr;
  _ArrMeta[ArrIndex].Used=true;
  _ArrMeta[ArrIndex].DimNr=0;
  _ArrMeta[ArrIndex].CellSize=0;
  _ArrMeta[ArrIndex].LoopIndex=0;
  _ArrMeta[ArrIndex].IndexVarAddr=0;
  _ArrMeta[ArrIndex].IndexVarMode=CpuDecMode::LoclVar;
  _ArrMeta[ArrIndex].ExitAdr=0;
  return ArrIndex;
} 

//Free ArrIndex
void ArrayComputer::_DynFreeArrIndex(int ArrIndex){
  int i;
  _ArrMeta[ArrIndex].ScopeId=0;
  _ArrMeta[ArrIndex].ScopeNr=0;
  _ArrMeta[ArrIndex].Used=false;
  _ArrMeta[ArrIndex].DimNr=0;
  _ArrMeta[ArrIndex].CellSize=0;
  _ArrMeta[ArrIndex].LoopIndex=0;
  _ArrMeta[ArrIndex].IndexVarAddr=0;
  _ArrMeta[ArrIndex].IndexVarMode=CpuDecMode::LoclVar;
  _ArrMeta[ArrIndex].ExitAdr=0;
  for(i=0;i<_MaxArrayDims;i++){ 
    _ArrMeta[ArrIndex].PrevSize.n[i]=0; 
    _ArrMeta[ArrIndex].DimSize.n[i]=0; 
    _ArrMeta[ArrIndex].DimValue.n[i]=0;
  }
}

//Calculate array size
CpuWrd ArrayComputer::_DynGetElements(int ArrIndex){

  //Variables
  CpuWrd Elements;

  //Switch on array number of dimensions
  switch(_ArrMeta[ArrIndex].DimNr){
    case 1: Elements=_ArrMeta[ArrIndex].DimSize.n[0]; break;
    case 2: Elements=_ArrMeta[ArrIndex].DimSize.n[0]*_ArrMeta[ArrIndex].DimSize.n[1]; break;
    case 3: Elements=_ArrMeta[ArrIndex].DimSize.n[0]*_ArrMeta[ArrIndex].DimSize.n[1]*_ArrMeta[ArrIndex].DimSize.n[2]; break;
    case 4: Elements=_ArrMeta[ArrIndex].DimSize.n[0]*_ArrMeta[ArrIndex].DimSize.n[1]*_ArrMeta[ArrIndex].DimSize.n[2]*_ArrMeta[ArrIndex].DimSize.n[3]; break;
    case 5: Elements=_ArrMeta[ArrIndex].DimSize.n[0]*_ArrMeta[ArrIndex].DimSize.n[1]*_ArrMeta[ArrIndex].DimSize.n[2]*_ArrMeta[ArrIndex].DimSize.n[3]*_ArrMeta[ArrIndex].DimSize.n[4]; break;
  }  

  //Return result
  return Elements;

}

//Calculate fixed array size
CpuWrd ArrayComputer::_DynCalculateArraySize(int ArrIndex){
  return _ArrMeta[ArrIndex].CellSize*_DynGetElements(ArrIndex);
}

//Copy array contents when geometry changes
void ArrayComputer::_DynCopyArrayElements(int DimNr,CpuWrd CellSize,CpuMbl SrcBlock, ArrayIndexes SrcDims,CpuMbl DstBlock, ArrayIndexes DstDims){

  //Variables
  ArrayIndexes Index;
  CpuWrd SrcOffset,DstOffset;
  CpuWrd SrcFactor[_MaxArrayDims];
  CpuWrd DstFactor[_MaxArrayDims];

  //Calculate dimension offset factors
  _CalculateDimensionFactors(DimNr,CellSize,SrcDims,SrcFactor);
  _CalculateDimensionFactors(DimNr,CellSize,DstDims,DstFactor);
  
  //Switch on array number of dimensions
  switch(DimNr){
    
    //1-Dimension array
    case 1:
      for(Index.n[0]=0;Index.n[0]<DstDims.n[0];Index.n[0]++){
        SrcOffset=_CalculateArrayOffset(DimNr,SrcFactor,Index);
        DstOffset=_CalculateArrayOffset(DimNr,DstFactor,Index);
        if(Index.n[0]<SrcDims.n[0]){
          MemCpy(&_Aux->CharPtr(DstBlock)[DstOffset],&_Aux->CharPtr(SrcBlock)[SrcOffset],CellSize);
        }
        else{
          memset(&_Aux->CharPtr(DstBlock)[DstOffset],0,CellSize);
        }
      }
      break;
  
    //2-Dimension array
    case 2:
      for(Index.n[0]=0;Index.n[0]<DstDims.n[0];Index.n[0]++){
      for(Index.n[1]=0;Index.n[1]<DstDims.n[1];Index.n[1]++){
        SrcOffset=_CalculateArrayOffset(DimNr,SrcFactor,Index);
        DstOffset=_CalculateArrayOffset(DimNr,DstFactor,Index);
        if(Index.n[0]<SrcDims.n[0] && Index.n[1]<SrcDims.n[1]){
          MemCpy(&_Aux->CharPtr(DstBlock)[DstOffset],&_Aux->CharPtr(SrcBlock)[SrcOffset],CellSize);
        }
        else{
          memset(&_Aux->CharPtr(DstBlock)[DstOffset],0,CellSize);
        }
      }}
      break;
  
    //3-Dimension array
    case 3:
      for(Index.n[0]=0;Index.n[0]<DstDims.n[0];Index.n[0]++){
      for(Index.n[1]=0;Index.n[1]<DstDims.n[1];Index.n[1]++){
      for(Index.n[2]=0;Index.n[2]<DstDims.n[2];Index.n[2]++){
        SrcOffset=_CalculateArrayOffset(DimNr,SrcFactor,Index);
        DstOffset=_CalculateArrayOffset(DimNr,DstFactor,Index);
        if(Index.n[0]<SrcDims.n[0] && Index.n[1]<SrcDims.n[1] && Index.n[2]<SrcDims.n[2]){
          MemCpy(&_Aux->CharPtr(DstBlock)[DstOffset],&_Aux->CharPtr(SrcBlock)[SrcOffset],CellSize);
        }
        else{
          memset(&_Aux->CharPtr(DstBlock)[DstOffset],0,CellSize);
        }
      }}}
      break;
  
    //4-Dimension array
    case 4:
      for(Index.n[0]=0;Index.n[0]<DstDims.n[0];Index.n[0]++){
      for(Index.n[1]=0;Index.n[1]<DstDims.n[1];Index.n[1]++){
      for(Index.n[2]=0;Index.n[2]<DstDims.n[2];Index.n[2]++){
      for(Index.n[3]=0;Index.n[3]<DstDims.n[3];Index.n[3]++){
        SrcOffset=_CalculateArrayOffset(DimNr,SrcFactor,Index);
        DstOffset=_CalculateArrayOffset(DimNr,DstFactor,Index);
        if(Index.n[0]<SrcDims.n[0] && Index.n[1]<SrcDims.n[1] && Index.n[2]<SrcDims.n[2] && Index.n[3]<SrcDims.n[3]){
          MemCpy(&_Aux->CharPtr(DstBlock)[DstOffset],&_Aux->CharPtr(SrcBlock)[SrcOffset],CellSize);
        }
        else{
          memset(&_Aux->CharPtr(DstBlock)[DstOffset],0,CellSize);
        }
      }}}}
      break;
  
    //5-Dimension array
    case 5:
      for(Index.n[0]=0;Index.n[0]<DstDims.n[0];Index.n[0]++){
      for(Index.n[1]=0;Index.n[1]<DstDims.n[1];Index.n[1]++){
      for(Index.n[2]=0;Index.n[2]<DstDims.n[2];Index.n[2]++){
      for(Index.n[3]=0;Index.n[3]<DstDims.n[3];Index.n[3]++){
      for(Index.n[4]=0;Index.n[4]<DstDims.n[4];Index.n[4]++){
        SrcOffset=_CalculateArrayOffset(DimNr,SrcFactor,Index);
        DstOffset=_CalculateArrayOffset(DimNr,DstFactor,Index);
        if(Index.n[0]<SrcDims.n[0] && Index.n[1]<SrcDims.n[1] && Index.n[2]<SrcDims.n[2] && Index.n[3]<SrcDims.n[3] && Index.n[4]<SrcDims.n[4]){
          MemCpy(&_Aux->CharPtr(DstBlock)[DstOffset],&_Aux->CharPtr(SrcBlock)[SrcOffset],CellSize);
        }
        else{
          memset(&_Aux->CharPtr(DstBlock)[DstOffset],0,CellSize);
        }
      }}}}}
      break;
  
  }

}

//Initialize array computer
void ArrayComputer::DynInit(int ProcessId,long ArrMetaChunkSize,AuxMemoryManager *Aux,StringComputer *StC){
  _Aux=Aux;
  _StC=StC;
  _ArrMeta.Init(ProcessId,ArrMetaChunkSize,(char *)"_ArrMeta");
}

//Set scopeid
void ArrayComputer::DynSetScope(int ScopeId,CpuLon ScopeNr){
  _ScopeId=ScopeId;
  _ScopeNr=ScopeNr;
}

//Check array block as destination
bool ArrayComputer::_DynCheckAsDestin(CpuMbl *ArrBlock,CpuWrd InitSize,int *ArrIndex){

  //When array block is zero: get new ArrIndex and allocate new array block
  if(*ArrBlock==0){ 
    if((*ArrIndex=_DynNewArrIndex())==-1){
      System::Throw(SysExceptionCode::ArrayIndexAllocationFailure);
      return false;
    }
    if(!_Aux->Alloc(_ScopeId,_ScopeNr,InitSize,*ArrIndex,ArrBlock)){
      _DynFreeArrIndex(*ArrIndex);
      System::Throw(SysExceptionCode::ArrayBlockAllocationFailure);
      return false;
    }
  }

  //When array block is not zero: Check validity and get ArrIndex
  else{
    if(!_Aux->IsValid(*ArrBlock)){
      System::Throw(SysExceptionCode::InvalidArrayBlock,ToString(*ArrBlock)); 
      return false; 
    }
    *ArrIndex=_Aux->GetArrIndex(*ArrBlock);
  }

  //Return code
  return true;

}

//Check array block as source
bool ArrayComputer::_DynCheckAsSource(CpuMbl ArrBlock,int *ArrIndex){

  //Check validity and get ArrIndex
  if(!_Aux->IsValid(ArrBlock) || ArrBlock==0){
    System::Throw(SysExceptionCode::InvalidArrayBlock,ToString(ArrBlock)); 
    return false; 
  }
  *ArrIndex=_Aux->GetArrIndex(ArrBlock);

  //Return code
  return true;

}

//Move to next element
void ArrayComputer::_DynMoveNext(int ArrIndex,ArrayIndexes& DimValue,bool& LastReached){

  //Variables
  int i;

  //Next element loop over dimensions starting with lowest one
  LastReached=false;
  for(i=_ArrMeta[ArrIndex].DimNr-1;i>=0;i--){
    DimValue.n[i]++;
    if(DimValue.n[i]==_ArrMeta[ArrIndex].DimSize.n[i]-1){
      if(i==0){
        LastReached=true;
        break;
      }
    }
    else if(DimValue.n[i]>_ArrMeta[ArrIndex].DimSize.n[i]-1){
      DimValue.n[i]=0;
    }
    else{
      break;
    }
  }

}

//Check array has element
bool ArrayComputer::_DynHasElement(int ArrIndex,ArrayIndexes& DimValue){

  //Variables
  int i;
  bool HasElement;

  //Next element loop over dimensions starting with lowest one
  HasElement=true;
  for(i=0;i<_ArrMeta[ArrIndex].DimNr;i++){
    if(DimValue.n[i]>_ArrMeta[ArrIndex].DimSize.n[i]-1){
      HasElement=false;
      break;
    }
  }

  //Return result
  return HasElement;

}

//Store array metadata
bool ArrayComputer::DynStoreMeta(int ScopeId,CpuLon ScopeNr,int DimNr,CpuWrd CellSize,ArrayIndexes DimSize){
  ArrayMeta Meta;
  Meta=(ArrayMeta){ScopeId,ScopeNr,true,DimNr,CellSize,{0},DimSize,{0},0,0,CpuDecMode::LoclVar,0};
  if(!_ArrMeta.Push(Meta)){ return false; }
  return true;
}

//Get string print of array (for debug messages)
String ArrayComputer::DynPrint(CpuMbl ArrBlock){
  String Result;
  int ArrIndex=_Aux->GetArrIndex(ArrBlock);
  Result="dynarray(index="+ToString(ArrIndex)+
  ",cellsz="+ToString(_ArrMeta[ArrIndex].CellSize)+
  ",loopix="+ToString(_ArrMeta[ArrIndex].LoopIndex)+
  ",ixvar="+NZHEXFORMAT(_ArrMeta[ArrIndex].IndexVarAddr)+(_ArrMeta[ArrIndex].IndexVarMode==CpuDecMode::GlobVar?"(glob)":"(locl)")+
  ",exitadr="+NZHEXFORMAT(_ArrMeta[ArrIndex].ExitAdr)+
  ")[";
  for(int i=1;i<=_ArrMeta[ArrIndex].DimNr;i++){
    Result+=ToString(_ArrMeta[ArrIndex].DimSize.n[i-1]);
    if(i<_ArrMeta[ArrIndex].DimNr){ Result+=","; }
  }
  Result+="]";
  return Result;
}

//Return empty 1-dim array
bool ArrayComputer::AD1EM(CpuMbl *Arr,CpuWrd CellSize){
  int ArrIndex;
  if(!_DynCheckAsDestin(Arr,0,&ArrIndex)){ return false; }
  _ArrMeta[ArrIndex].DimNr=1;
  _ArrMeta[ArrIndex].CellSize=CellSize;
  _ArrMeta[ArrIndex].DimSize.n[0]=0;
  return true;
}

//Define 1-dim array and set to zero elements if it does not exist before
bool ArrayComputer::AD1DF(CpuMbl *ArrBlock){

  //Variables
  bool WasExisting;
  int ArrIndex;

  //Save if array was existing before
  WasExisting=(*ArrBlock==0?false:true);

  //Check array as destination
  if(!_DynCheckAsDestin(ArrBlock,0,&ArrIndex)){ return false; }

  //Save array metadata
  _ArrMeta[ArrIndex].DimNr=1;

  //Init indexes to zero if array was not existing
  if(!WasExisting){
    for(int i=1;i<=_ArrMeta[ArrIndex].DimNr;i++){
      _ArrMeta[ArrIndex].DimSize.n[i-1]=0;
    }
  }  

  //Return code
  return true;
  
}

//Append to 1-dimensional array and get offset to last element
bool ArrayComputer::AD1AP(CpuMbl ArrBlock,CpuWrd *Offset,CpuWrd CellSize){
  
  //Variables
  int ArrIndex;
  CpuWrd BlockSize;
  
  //Check array as source
  if(!_DynCheckAsSource(ArrBlock,&ArrIndex)){ return false; }

  //Increase dimension size by 1 and set cell size
  _ArrMeta[ArrIndex].DimSize.n[0]++;
  _ArrMeta[ArrIndex].CellSize=CellSize;

  //Resize 1-dimensional array
  BlockSize=_DynCalculateArraySize(ArrIndex);
  if(!_Aux->Realloc(_ScopeId,_ScopeNr,ArrBlock,BlockSize)){
    System::Throw(SysExceptionCode::ArrayBlockAllocationFailure);
    return false;
  }

  //Calculate offset
  *Offset=_ArrMeta[ArrIndex].CellSize*(_ArrMeta[ArrIndex].DimSize.n[0]-1);

  //Set element to zero (as it can be intepreted as invalid memory block if element is a block)
  memset(_Aux->CharPtr(ArrBlock)+(*Offset),0,_ArrMeta[ArrIndex].CellSize);

  //Return code
  return true;

}

//Insert to 1-dimensional array and get offset to inserted element
bool ArrayComputer::AD1IN(CpuMbl ArrBlock,CpuWrd DimValue,CpuWrd *Offset,CpuWrd CellSize){
  
  //Variables
  char *Ptr;
  int ArrIndex;
  CpuWrd PrevSize;
  CpuWrd BlockSize;
  
  //Check array as source
  if(!_DynCheckAsSource(ArrBlock,&ArrIndex)){ return false; }
  
  //Check validity of input parameters
  if(DimValue<0 || DimValue>_ArrMeta[ArrIndex].DimSize.n[0]){
    System::Throw(SysExceptionCode::ArrayIndexingOutOfBounds,"1",ToString(_ArrMeta[ArrIndex].DimSize.n[0]),ToString(DimValue)); 
    return false;
  }

  //Save previous size
  PrevSize=_ArrMeta[ArrIndex].DimSize.n[0];

  //Increase dimension size and set cellsize
  _ArrMeta[ArrIndex].DimSize.n[0]++;
  _ArrMeta[ArrIndex].CellSize=CellSize;

  //Resize array block
  BlockSize=_DynCalculateArraySize(ArrIndex);
  if(!_Aux->Realloc(_ScopeId,_ScopeNr,ArrBlock,BlockSize)){
    System::Throw(SysExceptionCode::ArrayBlockAllocationFailure);
    return false;
  }

  //Special case when array was empty and index is zero
  if(PrevSize==0 && DimValue==0){
    *Offset=0;
    return true;
  }

  //Make a hole in the array
  if(PrevSize-DimValue>0){
    Ptr=_Aux->CharPtr(ArrBlock);
    MemMove(Ptr+((DimValue+1)*CellSize),Ptr+(DimValue*CellSize),(PrevSize-DimValue)*CellSize);
  }

  //Calculate offset
  *Offset=DimValue*CellSize;

  //Set element to zero (as it can be intepreted as invalid memory block if element is a block)
  memset(_Aux->CharPtr(ArrBlock)+(*Offset),0,CellSize);

  //Return code
  return true;

}

//Delete position from 1-dimensional array
bool ArrayComputer::AD1DE(CpuMbl ArrBlock,CpuWrd DimValue){
  
  //Variables
  char *Ptr;
  int ArrIndex;
  CpuWrd PrevSize;
  CpuWrd BlockSize;
  CpuWrd CellSize;
  
  //Check array as source
  if(!_DynCheckAsSource(ArrBlock,&ArrIndex)){ return false; }
  
  //Check validity of input parameters
  if(DimValue<0 || DimValue>_ArrMeta[ArrIndex].DimSize.n[0]-1){
    System::Throw(SysExceptionCode::ArrayIndexingOutOfBounds,"1",ToString(_ArrMeta[ArrIndex].DimSize.n[0]),ToString(DimValue)); 
    return false;
  }

  //Delete element in array
  PrevSize=_ArrMeta[ArrIndex].DimSize.n[0];
  if(PrevSize-DimValue-1>0){
    Ptr=_Aux->CharPtr(ArrBlock);
    CellSize=_ArrMeta[ArrIndex].CellSize;
    MemMove(Ptr+((DimValue)*CellSize),Ptr+((DimValue+1)*CellSize),(PrevSize-DimValue-1)*CellSize);
  }

  //Decrease dimension size
  _ArrMeta[ArrIndex].DimSize.n[0]--;

  //Resize array block
  BlockSize=_DynCalculateArraySize(ArrIndex);
  if(!_Aux->Realloc(_ScopeId,_ScopeNr,ArrBlock,BlockSize)){
    System::Throw(SysExceptionCode::ArrayBlockAllocationFailure);
    return false;
  }

  //Return code
  return true;

}

//Offset calculation for 1-dimensional array
bool ArrayComputer::AD1OF(CpuMbl ArrBlock,CpuWrd DimValue,CpuWrd *Offset){
  
  //Variables
  int ArrIndex;
  
  //Check array as source
  if(!_DynCheckAsSource(ArrBlock,&ArrIndex)){ return false; }

  //Check validity of input parameters
  if(DimValue<0 || DimValue>_ArrMeta[ArrIndex].DimSize.n[0]-1){
    System::Throw(SysExceptionCode::ArrayIndexingOutOfBounds,"1",ToString(_ArrMeta[ArrIndex].DimSize.n[0]),ToString(DimValue)); 
    return false;
  }

  //Calculate offset
  *Offset=_ArrMeta[ArrIndex].CellSize*DimValue;

  //Return code
  return true;

}

//Reset array
bool ArrayComputer::AD1RS(CpuMbl *ArrBlock){

  //Variables
  int ArrIndex;

  //Check array as destination
  if(!_DynCheckAsDestin(ArrBlock,0,&ArrIndex)){ return false; }

  //Save array metadata
  _ArrMeta[ArrIndex].DimNr=1;

  //Init indexes to zero
  for(int i=1;i<=_ArrMeta[ArrIndex].DimNr;i++){
    _ArrMeta[ArrIndex].DimSize.n[i-1]=0;
  }

  //Return code
  return true;
  
}

//Rewind array and prepare for loop
bool ArrayComputer::AD1RW(CpuMbl ArrBlock,CpuAdr IndexVarAddr,CpuDecMode IndexVarMode,CpuAdr ExitAdr){
  
  //Variables
  int ArrIndex;

  //Check array as source
  if(!_DynCheckAsSource(ArrBlock,&ArrIndex)){ return false; }

  //Reset loop variables
  _ArrMeta[ArrIndex].LoopIndex=0;
  _ArrMeta[ArrIndex].IndexVarAddr=IndexVarAddr;
  _ArrMeta[ArrIndex].IndexVarMode=IndexVarMode;
  _ArrMeta[ArrIndex].ExitAdr=ExitAdr;

  //Return code
  return true;

}

//Array for loop begin
bool ArrayComputer::AD1FO(CpuMbl ArrBlock,CpuWrd *Offset,CpuAdr *JumpAdr){
  
  //Variables
  int ArrIndex;
  
  //Check array as source
  if(!_DynCheckAsSource(ArrBlock,&ArrIndex)){ return false; }

  //Check loop end condition
  if(_ArrMeta[ArrIndex].LoopIndex>=_ArrMeta[ArrIndex].DimSize.n[0]){
    
    //Return exit adress
    *JumpAdr=_ArrMeta[ArrIndex].ExitAdr;

  }

  //Return offset and move to next element
  else{

    //Calculate offset
    *Offset=_ArrMeta[ArrIndex].CellSize*_ArrMeta[ArrIndex].LoopIndex;

    //Set exit address to zero
    *JumpAdr=0;

  }

  //Return code
  return true;

}

//Array for loop next element
bool ArrayComputer::AD1NX(CpuMbl ArrBlock,CpuAdr *IndexVarAddr,CpuDecMode *IndexVarMode){
  
  //Variables
  int ArrIndex;
  
  //Check array as source
  if(!_DynCheckAsSource(ArrBlock,&ArrIndex)){ return false; }

  //Move to next element
  _ArrMeta[ArrIndex].LoopIndex++;

  //Return index variable
  *IndexVarAddr=_ArrMeta[ArrIndex].IndexVarAddr;
  *IndexVarMode=_ArrMeta[ArrIndex].IndexVarMode;

  //Return code
  return true;

}

//1-dim string dyn array join
bool ArrayComputer::AD1SJ(CpuMbl *Str,CpuMbl ArrBlock,CpuMbl Sep){

  //Variables
  int ArrIndex;
  CpuWrd i;
  CpuMbl Elem;
  CpuWrd SepLen;

  //Check array as source
  if(!_DynCheckAsSource(ArrBlock,&ArrIndex)){ return false; }

  //Return empty string if array is empty
  if(_ArrMeta[ArrIndex].DimSize.n[0]==0){
    if(!_StC->SEMP(Str)){ return false; }
    return true;
  }

  //Join first array element
  if(_ArrMeta[ArrIndex].DimSize.n[0]>0){
    Elem=*(CpuMbl *)&_Aux->CharPtr(ArrBlock)[0];
    if(Elem!=0){ if(!_StC->SCOPY(Str,Elem)){ return false; } }
  }

  //Get separator length
  SepLen=_Aux->GetLen(Sep);

  //Join rest of array elements
  for(i=1;i<_ArrMeta[ArrIndex].DimSize.n[0];i++){
    if(SepLen!=0){ if(!_StC->SAPPN(Str,Sep)){ return false; } }
    Elem=*(CpuMbl *)&_Aux->CharPtr(ArrBlock)[i*sizeof(CpuMbl)];
    if(Elem!=0){ if(!_StC->SAPPN(Str,Elem)){ return false; } }
  }

  //Return code
  return true;

}

//1-dim char dyn array join
bool ArrayComputer::AD1CJ(CpuMbl *Str,CpuMbl ArrBlock,CpuMbl Sep){

  //Variables
  CpuWrd i;
  CpuWrd SepLen;
  int ArrIndex;

  //Check array as source
  if(!_DynCheckAsSource(ArrBlock,&ArrIndex)){ return false; }

  //Return empty string if array is empty
  if(_ArrMeta[ArrIndex].DimSize.n[0]==0){
    if(!_StC->SEMP(Str)){ return false; }
    return true;
  }

  //Join first array element
  if(_ArrMeta[ArrIndex].DimSize.n[0]>0){
    if(!_StC->SCOPY(Str,&_Aux->CharPtr(ArrBlock)[0],1)){ return false; }
  }

  //Get separator length
  SepLen=_Aux->GetLen(Sep);

  //Join rest of array elements
  for(i=1;i<_ArrMeta[ArrIndex].DimSize.n[0];i++){
    if(SepLen!=0){ if(!_StC->SAPPN(Str,Sep)){ return false; } }
    if(!_StC->SAPPN(Str,&_Aux->CharPtr(ArrBlock)[i],1)){ return false; }
  }

  //Return code
  return true;

}

//Copy array
bool ArrayComputer::ACOPY(CpuMbl *Destin,CpuMbl Source){ 
  
  //Variables
  int i;
  int ArrIndex1;
  int ArrIndex2;
  CpuWrd ArrSize;
  
  //When source array is initial, clear as well destination array
  if(Source==0){
    if(*Destin!=0){
      _DynFreeArrIndex(_Aux->GetArrIndex(*Destin));
      _Aux->Free(*Destin);
      (*Destin)=0;
    }
    return true;
  }

  //Check source array
  if(!_DynCheckAsSource(Source,&ArrIndex2)){ return false; }
  ArrSize=_DynCalculateArraySize(ArrIndex2);
  
  //Check destination array, get dimension index and resize
  if(!_DynCheckAsDestin(Destin,ArrSize,&ArrIndex1)){ return false; }
  
  //Reallocate array space
  if(!_Aux->Realloc(_ScopeId,_ScopeNr,*Destin,ArrSize)){
    _DynFreeArrIndex(ArrIndex1);
    _Aux->Free(*Destin);
    (*Destin)=0;
    System::Throw(SysExceptionCode::ArrayBlockAllocationFailure);
    return false;
  }

  //Copy array contents
  MemCpy(_Aux->CharPtr(*Destin),_Aux->CharPtr(Source),ArrSize);

  //Copy array metadata
  _ArrMeta[ArrIndex1].DimNr=_ArrMeta[ArrIndex2].DimNr;
  _ArrMeta[ArrIndex1].CellSize=_ArrMeta[ArrIndex2].CellSize;
  for(i=0;i<_MaxArrayDims;i++){ 
    _ArrMeta[ArrIndex1].DimSize.n[i]=_ArrMeta[ArrIndex2].DimSize.n[i]; 
    _ArrMeta[ArrIndex1].DimValue.n[i]=0;
  }
  
  //Return code
  return true;

}

//Create empty array
bool ArrayComputer::ADEMP(CpuMbl *Destin,int DimNr,CpuWrd CellSize){ 
  
  //Variables
  int i;
  int ArrIndex;
  
  //Check destination array, get dimension index and resize
  if(!_DynCheckAsDestin(Destin,0,&ArrIndex)){ return false; }
  
  //Reallocate array space
  if(!_Aux->Realloc(_ScopeId,_ScopeNr,*Destin,0)){
    _DynFreeArrIndex(ArrIndex);
    _Aux->Free(*Destin);
    (*Destin)=0;
    System::Throw(SysExceptionCode::ArrayBlockAllocationFailure);
    return false;
  }

  //Copy array metadata
  _ArrMeta[ArrIndex].DimNr=DimNr;
  _ArrMeta[ArrIndex].CellSize=CellSize;
  for(i=0;i<_MaxArrayDims;i++){ 
    _ArrMeta[ArrIndex].DimSize.n[i]=0; 
    _ArrMeta[ArrIndex].DimValue.n[i]=0;
  }
  
  //Return code
  return true;

}

//Define array cellsize and dimensions
bool ArrayComputer::ADDEF(CpuMbl *ArrBlock,int DimNr,CpuWrd CellSize){

  //Variables
  int ArrIndex;

  //Check array as destination
  if(!_DynCheckAsDestin(ArrBlock,0,&ArrIndex)){ return false; }

  //Save array metadata
  _ArrMeta[ArrIndex].DimNr=DimNr;
  _ArrMeta[ArrIndex].CellSize=CellSize;

  //Return code
  return true;
  
}

//Set array dimension
bool ArrayComputer::ADSET(CpuMbl *ArrBlock,int DimIndex,CpuWrd Size){
  
  //Variables
  int ArrIndex;
  
  //Checkk array as source
  if(!_DynCheckAsSource(*ArrBlock,&ArrIndex)){ return false; }
  
  //Check validity of input parameters
  if(DimIndex<1 || DimIndex>_ArrMeta[ArrIndex].DimNr){
    System::Throw(SysExceptionCode::InvalidArrayDimension,ToString(*ArrBlock),ToString(DimIndex)); 
    return false; 
  }
  if(Size<0){
    System::Throw(SysExceptionCode::InvalidDimensionSize); 
    return false; 
  }
  
  //Set dimension size and save previous value
  _ArrMeta[ArrIndex].PrevSize.n[DimIndex-1]=_ArrMeta[ArrIndex].DimSize.n[DimIndex-1];
  _ArrMeta[ArrIndex].DimSize.n[DimIndex-1]=Size;

  //Return code
  return true;

}

//Resize array
bool ArrayComputer::ADRSZ(CpuMbl *ArrBlock){
  
  //Variables
  int ArrIndex;
  CpuWrd BlockSize;
  CpuMbl AuxBlock;
  
  //Check array as source
  if(!_DynCheckAsSource(*ArrBlock,&ArrIndex)){ return false; }
  
  //Resizing 1-dimensional array: Reallocation is enough
  if(_ArrMeta[ArrIndex].DimNr==1){
    BlockSize=_DynCalculateArraySize(ArrIndex);
    if(!_Aux->Realloc(_ScopeId,_ScopeNr,*ArrBlock,BlockSize)){
      System::Throw(SysExceptionCode::ArrayBlockAllocationFailure);
      return false;
    }
  }

  //Resizing n-dimensional arrays: Places of array elements change and we need to copy old array into new one
  else{
    BlockSize=_DynCalculateArraySize(ArrIndex);
    if(!_Aux->Alloc(_ScopeId,_ScopeNr,BlockSize,ArrIndex,&AuxBlock)){
      System::Throw(SysExceptionCode::ArrayBlockAllocationFailure);
      return false;
    }
    _DynCopyArrayElements(_ArrMeta[ArrIndex].DimNr,_ArrMeta[ArrIndex].CellSize,*ArrBlock,_ArrMeta[ArrIndex].PrevSize,AuxBlock,_ArrMeta[ArrIndex].DimSize);
    _Aux->Free(*ArrBlock);
    *ArrBlock=AuxBlock;
  }

  //Return code
  return true;

}

//Array dimension size get
bool ArrayComputer::ADGET(CpuMbl ArrBlock,int DimIndex,CpuWrd *Size){
  
  //Variables
  int ArrIndex;
  
  //Check array as source
  if(!_DynCheckAsSource(ArrBlock,&ArrIndex)){ return false; }
  
  //Check validity of input parameters
  if(DimIndex<1 || DimIndex>_ArrMeta[ArrIndex].DimNr){
    System::Throw(SysExceptionCode::InvalidArrayDimension,ToString(ArrBlock),ToString(DimIndex)); 
    return false; 
  }

  //Return result
  *Size=_ArrMeta[ArrIndex].DimSize.n[DimIndex-1];

  //Return code
  return true;

}

//Reset array
bool ArrayComputer::ADRST(CpuMbl *ArrBlock){
  
  //Variables
  int ArrIndex;
  
  //Check array as source
  if(!_DynCheckAsSource(*ArrBlock,&ArrIndex)){ return false; }
  
  //Init all array indexes to zero
  for(int i=1;i<=_ArrMeta[ArrIndex].DimNr;i++){
    _ArrMeta[ArrIndex].DimSize.n[i-1]=0;
  }

  //Resize array memory to zero
  if(!_Aux->Realloc(_ScopeId,_ScopeNr,*ArrBlock,0)){
    System::Throw(SysExceptionCode::ArrayBlockAllocationFailure);
    return false;
  }

  //Return code
  return true;

}

//Set dimension index value
bool ArrayComputer::ADIDX(CpuMbl ArrBlock,int DimIndex,CpuWrd DimValue){
  
  //Variables
  int ArrIndex;
  
  //Check array as source
  if(!_DynCheckAsSource(ArrBlock,&ArrIndex)){ return false; }

  //Check validity of input parameters
  if(DimIndex<1 || DimIndex>_ArrMeta[ArrIndex].DimNr){
    System::Throw(SysExceptionCode::ArrayWrongDimension,ToString(_ArrMeta[ArrIndex].DimNr),ToString(DimIndex)); 
    return false; 
  }
  if(DimValue<0 || DimValue>_ArrMeta[ArrIndex].DimSize.n[DimIndex-1]-1){
    System::Throw(SysExceptionCode::ArrayIndexingOutOfBounds,ToString(DimIndex),ToString(_ArrMeta[ArrIndex].DimSize.n[DimIndex-1]),ToString(DimValue)); 
    return false; 
  }

  //Store index
  _ArrMeta[ArrIndex].DimValue.n[DimIndex-1]=DimValue;

  //Return code
  return true;

}

//Offset calculation for n-dimensional array
bool ArrayComputer::ADOFN(CpuMbl ArrBlock,CpuWrd *Offset){
  
  //Variables
  int ArrIndex;
  CpuWrd Factor[_MaxArrayDims];
  
  //Check array as source
  if(!_DynCheckAsSource(ArrBlock,&ArrIndex)){ return false; }

  //Calculate array element offset
  _CalculateDimensionFactors(_ArrMeta[ArrIndex].DimNr,_ArrMeta[ArrIndex].CellSize,_ArrMeta[ArrIndex].DimSize,Factor);
  *Offset=_CalculateArrayOffset(_ArrMeta[ArrIndex].DimNr,Factor,_ArrMeta[ArrIndex].DimValue);

  //Return code
  return true;

}

//Calculate array size
bool ArrayComputer::ADSIZ(CpuMbl ArrBlock,CpuWrd *Size){

  //Variables
  int ArrIndex;

  //Check array as source
  if(!_DynCheckAsSource(ArrBlock,&ArrIndex)){ return false; }

  //Return result
  *Size=_DynCalculateArraySize(ArrIndex);

  //Return code
  return true;

}

//String split
bool ArrayComputer::SSPL(CpuMbl *Arr,CpuMbl Str,CpuMbl Sep){

  //Split part;
  struct SplitPart{
    CpuWrd Pos;
    CpuWrd Len;
  };

  //Variables
  int i;
  int ArrIndex;
  CpuWrd FoundPos;
  CpuWrd SearchPos;
  CpuWrd StrLen;
  CpuWrd SepLen;
  CpuWrd ArrSize;
  SplitPart Split;
  Array<SplitPart> Splits;

  //Check strings and get lengths
  if(!_StC->IsValid(Str,true) || !_StC->IsValid(Sep,true)){ return false; }
  StrLen=_Aux->GetLen(Str);
  SepLen=_Aux->GetLen(Sep);

  //Return empty array if input string is empty
  if(StrLen==0){
    if(!_DynCheckAsDestin(Arr,0,&ArrIndex)){ return false; }
    _ArrMeta[ArrIndex].DimNr=1;
    _ArrMeta[ArrIndex].CellSize=sizeof(CpuMbl);
    _ArrMeta[ArrIndex].DimSize.n[0]=0;
    return true;
  }

  //Calculate string splits
  SearchPos=0;
  do{
    Split.Pos=SearchPos;
    if(!_StC->SFIND(&FoundPos,Str,Sep,SearchPos)){ return false; }
    if(FoundPos==SearchPos){ 
      Split.Len=0;
      Splits.Add(Split);
    }
    else if(FoundPos!=-1){ 
      Split.Len=FoundPos-SearchPos;
      Splits.Add(Split);
    }
    else{
      Split.Len=StrLen-SearchPos;
      Splits.Add(Split);
      break;
    }
    if(SearchPos==StrLen-1){ 
      Split.Pos=StrLen;
      Split.Len=0;
      Splits.Add(Split);
      break;
    }
    SearchPos=FoundPos+SepLen;
  }while(true);

  //Case when separator is not found (do not split)
  if(Splits.Length()==0){
    Split.Pos=0;
    Split.Len=StrLen;
    Splits.Add(Split);
  }
  
  //Check array as destination
  ArrSize=sizeof(CpuMbl)*Splits.Length();
  if(!_DynCheckAsDestin(Arr,ArrSize,&ArrIndex)){ return false; }

  //Reallocate size and set metadata
  if(!_Aux->Realloc(_ScopeId,_ScopeNr,*Arr,ArrSize)){
    _DynFreeArrIndex(ArrIndex);
    _Aux->Free(*Arr);
    *Arr=0;
    System::Throw(SysExceptionCode::ArrayBlockAllocationFailure);
    return false;
  }
  _ArrMeta[ArrIndex].DimNr=1;
  _ArrMeta[ArrIndex].CellSize=sizeof(CpuMbl);
  _ArrMeta[ArrIndex].DimSize.n[0]=Splits.Length();

  //Copy string splits
  for(i=0;i<Splits.Length();i++){
    *(CpuMbl *)&_Aux->CharPtr(*Arr)[i*sizeof(CpuMbl)]=0;
    if(!_StC->SCOPY((CpuMbl *)&_Aux->CharPtr(*Arr)[i*sizeof(CpuMbl)],(char *)&_Aux->CharPtr(Str)[Splits[i].Pos],Splits[i].Len)){ 
      _Aux->Free(*Arr);
      _DynFreeArrIndex(ArrIndex);
      *Arr=0;
      return false; 
    }
  }

  //Return code
  return true;

}

//Fill 1-dimensional dynamic array from void pointer and number of elements
bool ArrayComputer::ADVCP(CpuMbl *ArrBlock,void *Data,CpuLon Elements){

  //Variables
  int ArrIndex;
  Buffer Buff;

  //Check provided array as destination
  if(!_DynCheckAsDestin(ArrBlock,0,&ArrIndex)){ return false; }
  
  //Set array elements
  _ArrMeta[ArrIndex].DimSize.n[0]=Elements;
  
  //Allocate array memory
  if(!_Aux->Realloc(_ScopeId,_ScopeNr,*ArrBlock,Elements*DynGetCellSize(*ArrBlock))){
    _DynFreeArrIndex(ArrIndex);
    _Aux->Free(*ArrBlock);
    (*ArrBlock)=0;
    System::Throw(SysExceptionCode::ArrayBlockAllocationFailure);
    return false;
  }
  
  //Copy filedata
  MemCpy(_Aux->CharPtr(*ArrBlock),(char *)Data,Elements*DynGetCellSize(*ArrBlock));

  //Return code
  return true;

}

//Read char[] from file
bool ArrayComputer::RDCH(CpuBol *RetCode,CpuInt Handler,CpuMbl *ArrBlock,CpuLon Length){

  //Variables
  int ArrIndex;
  Buffer Buff;

  //Check provided array as destination
  if(!_DynCheckAsDestin(ArrBlock,0,&ArrIndex)){ return false; }
  
  //Initialize array metadata
  _ArrMeta[ArrIndex].DimNr=1;
  _ArrMeta[ArrIndex].CellSize=sizeof(CpuChr);
  _ArrMeta[ArrIndex].DimSize.n[0]=0;
  
  //Read buffer
  if(!_Stl->FileSystem.Read(Handler,Buff,Length)){ *RetCode=false; return true; }

  //Allocate array memory
  if(!_Aux->Realloc(_ScopeId,_ScopeNr,*ArrBlock,Length)){
    _DynFreeArrIndex(ArrIndex);
    _Aux->Free(*ArrBlock);
    (*ArrBlock)=0;
    System::Throw(SysExceptionCode::ArrayBlockAllocationFailure);
    return false;
  }
  
  //Copy filedata
  MemCpy(_Aux->CharPtr(*ArrBlock),Buff.BuffPnt(),Length);
  _ArrMeta[ArrIndex].DimSize.n[0]=Length;

  //Return code
  *RetCode=true;
  return true;

}

//Write char[] to file
bool ArrayComputer::WRCH(CpuBol *RetCode,CpuInt Handler,CpuMbl ArrBlock,CpuLon Length){
  
  //Variables
  int ArrIndex;

  //Check array as source
  if(!_DynCheckAsSource(ArrBlock,&ArrIndex)){ return false; }
  
  //Check length of provided array is not overflowed
  if(Length>_ArrMeta[ArrIndex].DimSize.n[0]){
    System::Throw(SysExceptionCode::WriteCharArrayIncorrectLength,ToString(_ArrMeta[ArrIndex].DimSize.n[0]),ToString(Length));
    return false;
  }

  //Write buffer to file
  if(!_Stl->FileSystem.Write(Handler,_Aux->CharPtr(ArrBlock),Length)){
    *RetCode=false;
    return true;
  }

  //Return code
  *RetCode=true;
  return true;

}

//Read all file into char[]
bool ArrayComputer::RDALCH(CpuBol *RetCode,CpuInt Handler,CpuMbl *ArrBlock){

  //Variables
  int ArrIndex;
  Buffer Buff;
  CpuLon Size;

  //Check provided array as destination
  if(!_DynCheckAsDestin(ArrBlock,0,&ArrIndex)){ return false; }
  
  //Get file size
  if(!_Stl->FileSystem.GetFileSize(Handler,Size)){ *RetCode=false; return true; }

  //Initialize array metadata
  _ArrMeta[ArrIndex].DimNr=1;
  _ArrMeta[ArrIndex].CellSize=sizeof(CpuChr);
  _ArrMeta[ArrIndex].DimSize.n[0]=Size;
  
  //Read buffer
  if(!_Stl->FileSystem.Read(Handler,Buff,Size)){ *RetCode=false; return true; }

  //Allocate array memory
  if(!_Aux->Realloc(_ScopeId,_ScopeNr,*ArrBlock,Size)){
    _DynFreeArrIndex(ArrIndex);
    _Aux->Free(*ArrBlock);
    (*ArrBlock)=0;
    System::Throw(SysExceptionCode::ArrayBlockAllocationFailure);
    return false;
  }
  
  //Copy filedata
  MemCpy(_Aux->CharPtr(*ArrBlock),Buff.BuffPnt(),Size);

  //Return code
  *RetCode=true;
  return true;

}

//Write all file from char[]
bool ArrayComputer::WRALCH(CpuBol *RetCode,CpuInt Handler,CpuMbl ArrBlock){

  //Variables
  int ArrIndex;

  //Check array as source
  if(!_DynCheckAsSource(ArrBlock,&ArrIndex)){ return false; }
  
  //Write buffer to file
  if(!_Stl->FileSystem.Write(Handler,_Aux->CharPtr(ArrBlock),_ArrMeta[ArrIndex].DimSize.n[0])){
    *RetCode=false;
    return true;
  }

  //Return code
  *RetCode=true;
  return true;

}

//Read text file into array
bool ArrayComputer::RDALST(CpuBol *RetCode,CpuInt Handler,CpuMbl *ArrBlock){
  
  //Variables
  CpuBol ReadSomething;
  String Line;

  //Write loop
  if(!STAOPW(ArrBlock)){ return false; }
  _Stl->ClearError();
  ReadSomething=false;
  while(_Stl->FileSystem.Read(Handler,Line)){
    ReadSomething=true;
    if(!STAWRL(Line)){ return false; }
  }
  if(_Stl->Error()!=StlErrorCode::Ok){
    System::Throw(SysExceptionCode::ReadError,ToString(Handler)); 
    return false; 
  }
  if(!STACLO()){ return false; }

  //Return code
  *RetCode=ReadSomething;
  return true;

}

//Write string array into text file
bool ArrayComputer::WRALST(CpuBol *RetCode,CpuInt Handler,CpuMbl ArrBlock){
  
  //Variables
  int i;
  int ArrIndex;
  CpuWrd Length;
  CpuMbl LineBlock;
  String Line;

  //Check array as source
  if(!_DynCheckAsSource(ArrBlock,&ArrIndex)){ return false; }
  
  //Line write loop
  Length=_ArrMeta[ArrIndex].DimSize.n[0];
  for(i=0;i<Length;i++){
    LineBlock=*(CpuMbl *)&_Aux->CharPtr(ArrBlock)[i*sizeof(CpuMbl)];
    if(!_Aux->IsValid(LineBlock)){
      System::Throw(SysExceptionCode::InvalidStringBlock,ToString(LineBlock)); 
      return false; 
    }
    if(LineBlock!=0){
      Line=String(_Aux->CharPtr(LineBlock));
      if(!_Stl->FileSystem.Write(Handler,Line)){
        *RetCode=false;
        return true;
      }
    }
  }

  //Return code
  *RetCode=true;
  return true;

}

//To char array
bool ArrayComputer::TOCA(CpuMbl *ArrBlock,CpuDat *Data,CpuWrd Length){
  
  //Variables
  int ArrIndex;

  //Check provided array as destination
  if(!_DynCheckAsDestin(ArrBlock,0,&ArrIndex)){ return false; }
  
  //Initialize array metadata
  _ArrMeta[ArrIndex].DimNr=1;
  _ArrMeta[ArrIndex].CellSize=sizeof(CpuChr);
  _ArrMeta[ArrIndex].DimSize.n[0]=Length;
  
  //Reallocate arraysize
  if(!_Aux->Realloc(_ScopeId,_ScopeNr,*ArrBlock,Length*sizeof(CpuChr))){
    _DynFreeArrIndex(ArrIndex);
    _Aux->Free(*ArrBlock);
    (*ArrBlock)=0;
    System::Throw(SysExceptionCode::ArrayBlockAllocationFailure);
    return false;
  }
  
  //Copy data
  MemCpy(_Aux->CharPtr(*ArrBlock),Data,Length);

  //Return code
  return true;

}

//String to char array
bool ArrayComputer::STOCA(CpuMbl *ArrBlock,CpuMbl Str){

  //Variables
  int ArrIndex;
  CpuWrd Length;

  //Get string lwngth
  if(!_StC->SLEN(&Length,Str)){ return false; }
  
  //Check provided array as destination
  if(!_DynCheckAsDestin(ArrBlock,0,&ArrIndex)){ return false; }
  
  //Initialize array metadata
  _ArrMeta[ArrIndex].DimNr=1;
  _ArrMeta[ArrIndex].CellSize=sizeof(CpuChr);
  _ArrMeta[ArrIndex].DimSize.n[0]=Length+1;
  
  //Reallocate array size
  if(!_Aux->Realloc(_ScopeId,_ScopeNr,*ArrBlock,(Length+1)*sizeof(CpuChr))){
    _DynFreeArrIndex(ArrIndex);
    _Aux->Free(*ArrBlock);
    (*ArrBlock)=0;
    System::Throw(SysExceptionCode::ArrayBlockAllocationFailure);
    return false;
  }
  
  //Copy data
  MemCpy(_Aux->CharPtr(*ArrBlock),_StC->CharPtr(Str),Length+1);

  //Return code
  return true;

}

//Array to char array
bool ArrayComputer::ATOCA(CpuMbl *Destin,CpuMbl Source){

  //Variables
  int ArrIndex1;
  int ArrIndex2;
  CpuWrd ArrSize;
  
  //Check source array
  if(!_DynCheckAsSource(Source,&ArrIndex2)){ return false; }
  ArrSize=_DynCalculateArraySize(ArrIndex2);
  
  //Check destination array, get dimension index and resize
  if(!_DynCheckAsDestin(Destin,ArrSize,&ArrIndex1)){ return false; }

  //Initialize array metadata
  _ArrMeta[ArrIndex1].DimNr=1;
  _ArrMeta[ArrIndex1].CellSize=sizeof(CpuChr);
  _ArrMeta[ArrIndex1].DimSize.n[0]=ArrSize;
  
  //Reallocate array size
  if(!_Aux->Realloc(_ScopeId,_ScopeNr,*Destin,ArrSize*sizeof(CpuChr))){
    _DynFreeArrIndex(ArrIndex2);
    _Aux->Free(*Destin);
    (*Destin)=0;
    System::Throw(SysExceptionCode::ArrayBlockAllocationFailure);
    return false;
  }
  
  //Copy data
  MemCpy(_Aux->CharPtr(*Destin),_Aux->CharPtr(Source),ArrSize);

  //Return code
  return true;

}

//From char array
bool ArrayComputer::FRCA(CpuDat *Data,CpuMbl ArrBlock,CpuWrd Length){

  //Variables
  int ArrIndex;
  CpuWrd ArrSize;

  //Check provided array as source
  if(!_DynCheckAsSource(ArrBlock,&ArrIndex)){ return false; }
  ArrSize=_DynCalculateArraySize(ArrIndex);
  
  //Char array must have expected length
  if(Length!=ArrSize){
    System::Throw(SysExceptionCode::FromCharArrayIncorrectLength,ToString(ArrSize),ToString(Length));
    return false;
  }

  //Copy data
  MemCpy(Data,_Aux->CharPtr(ArrBlock),Length);

  //Return code
  return true;

}

//String from char array
bool ArrayComputer::SFRCA(CpuMbl *Str,CpuMbl ArrBlock){

  //Variables
  int ArrIndex;
  CpuWrd Length;

  //Check provided array as source
  if(!_DynCheckAsSource(ArrBlock,&ArrIndex)){ return false; }

  //Get length
  Length=_ArrMeta[ArrIndex].DimSize.n[0];

  //Copy string  
  if(!_StC->SCOPY(Str,_Aux->CharPtr(ArrBlock),Length)){ return false; }

  //Return code
  return true;

}

//Array from char array
bool ArrayComputer::AFRCA(CpuMbl Destin,CpuMbl Source){

  //Variables
  int ArrIndex1;
  int ArrIndex2;
  CpuWrd ArrSize1;
  CpuWrd ArrSize2;
  
  //Check source array
  if(!_DynCheckAsSource(Source,&ArrIndex2)){ return false; }
  ArrSize2=_DynCalculateArraySize(ArrIndex2);
  
  //Check destination array (as source because it must exist)
  if(!_DynCheckAsSource(Destin,&ArrIndex1)){ return false; }
  ArrSize1=_DynCalculateArraySize(ArrIndex1);

  //Char array must have expected length
  if(ArrSize1!=ArrSize2){
    System::Throw(SysExceptionCode::FromCharArrayIncorrectLength,ToString(ArrSize2),ToString(ArrSize1));
    return false;
  }

  //Copy data
  MemCpy(_Aux->CharPtr(Destin),_Aux->CharPtr(Source),ArrSize1);

  //Return code
  return true;

}

//Fixed to fixed array casting
bool ArrayComputer::AF2F(CpuDat *DestDat,CpuAgx DestGeom,CpuDat *SourDat,CpuAgx SourGeom){

  //Variables
  int i;
  bool LastReached;
  bool Exit;
  ArrayIndexes DimValue;
  CpuWrd DestOffset;
  CpuWrd SourOffset;
  CpuAgx DestAbsGeom=_FixAgxDecoder(DestGeom);
  CpuWrd DestFactor[_MaxArrayDims];
  CpuAgx SourAbsGeom=_FixAgxDecoder(SourGeom);
  CpuWrd SourFactor[_MaxArrayDims];

  //Check indexes are defined
  if(DestAbsGeom>_ArrGeom.Length()-1){
    System::Throw(SysExceptionCode::ArrayGeometryInvalidIndex,ToString(DestAbsGeom),ToString(_ArrGeom.Length()-1)); 
    return false;
  }
  if(SourAbsGeom>_ArrGeom.Length()-1){
    System::Throw(SysExceptionCode::ArrayGeometryInvalidIndex,ToString(SourAbsGeom),ToString(_ArrGeom.Length()-1)); 
    return false;
  }

  //Calculate array dimension factors
  _CalculateDimensionFactors(_ArrGeom[DestAbsGeom].DimNr,_ArrGeom[DestAbsGeom].CellSize,_ArrGeom[DestAbsGeom].DimSize,DestFactor);
  _CalculateDimensionFactors(_ArrGeom[SourAbsGeom].DimNr,_ArrGeom[SourAbsGeom].CellSize,_ArrGeom[SourAbsGeom].DimSize,SourFactor);
  
  //Array element loop
  Exit=false;
  LastReached=false;
  for(i=0;i<_ArrGeom[DestAbsGeom].DimNr;i++){ DimValue.n[i]=0; }
  do{
    if(_FixHasElement(SourAbsGeom,DimValue)){
      DestOffset=_CalculateArrayOffset(_ArrGeom[DestAbsGeom].DimNr,DestFactor,DimValue);
      SourOffset=_CalculateArrayOffset(_ArrGeom[SourAbsGeom].DimNr,SourFactor,DimValue);
      MemCpy(&DestDat[DestOffset],&SourDat[SourOffset],_ArrGeom[DestAbsGeom].CellSize);
    }
    else{
      memset(&DestDat[DestOffset],0,_ArrGeom[DestAbsGeom].CellSize);
    }
    if(LastReached){ Exit=true; }
    _FixMoveNext(DestAbsGeom,DimValue,LastReached);
  }while(!Exit);
  
  //Return code
  return true;

}

//Fixed to dynamic array casting
bool ArrayComputer::AF2D(CpuMbl *DestArrBlock,CpuDat *SourDat,CpuAgx SourGeom){

  //Variables
  int i;
  bool LastReached;
  bool Exit;
  ArrayIndexes DimValue;
  int DestArrIndex;
  CpuWrd DestArrSize;
  CpuWrd DestOffset;
  CpuWrd SourOffset;
  CpuWrd DestFactor[_MaxArrayDims];
  CpuAgx SourAbsGeom=_FixAgxDecoder(SourGeom);
  CpuWrd SourFactor[_MaxArrayDims];

  //Check source array
  if(SourAbsGeom>_ArrGeom.Length()-1){
    System::Throw(SysExceptionCode::ArrayGeometryInvalidIndex,ToString(SourAbsGeom),ToString(_ArrGeom.Length()-1)); 
    return false;
  }
  
  //Check destination array (as source because it must be defined)
  if(!_DynCheckAsSource(*DestArrBlock,&DestArrIndex)){ return false; }

  //Set dimension sizes on destination array
  for(i=0;i<_ArrMeta[DestArrIndex].DimNr;i++){ 
    _ArrMeta[DestArrIndex].DimSize.n[i]=(i<=_ArrGeom[SourAbsGeom].DimNr-1?_ArrGeom[SourAbsGeom].DimSize.n[i]:0); 
    _ArrMeta[DestArrIndex].DimValue.n[i]=0;
  }

  //Reallocate destination array memory buffer
  DestArrSize=_DynCalculateArraySize(DestArrIndex);
  if(!_Aux->Realloc(_ScopeId,_ScopeNr,*DestArrBlock,DestArrSize)){
    _DynFreeArrIndex(DestArrIndex);
    _Aux->Free(*DestArrBlock);
    (*DestArrBlock)=0;
    System::Throw(SysExceptionCode::ArrayBlockAllocationFailure);
    return false;
  }

  //Calculate array dimension factors
  _CalculateDimensionFactors(_ArrMeta[DestArrIndex].DimNr,_ArrMeta[DestArrIndex].CellSize,_ArrMeta[DestArrIndex].DimSize,DestFactor);
  _CalculateDimensionFactors(_ArrGeom[SourAbsGeom].DimNr,_ArrGeom[SourAbsGeom].CellSize,_ArrGeom[SourAbsGeom].DimSize,SourFactor);
  
  //Array element loop
  Exit=false;
  LastReached=false;
  for(i=0;i<_ArrMeta[DestArrIndex].DimNr;i++){ DimValue.n[i]=0; }
  do{
    if(_FixHasElement(SourAbsGeom,DimValue)){
      DestOffset=_CalculateArrayOffset(_ArrMeta[DestArrIndex].DimNr,DestFactor,DimValue);
      SourOffset=_CalculateArrayOffset(_ArrGeom[SourAbsGeom].DimNr,SourFactor,DimValue);
      MemCpy(&_Aux->CharPtr(*DestArrBlock)[DestOffset],&SourDat[SourOffset],_ArrMeta[DestArrIndex].CellSize);
    }
    else{
      memset(&_Aux->CharPtr(*DestArrBlock)[DestOffset],0,_ArrMeta[DestArrIndex].CellSize);
    }
    if(LastReached){ Exit=true; }
    _DynMoveNext(DestArrIndex,DimValue,LastReached);
  }while(!Exit);
  
  //Return code
  return true;

}

//Dynamic to fixed array casting
bool ArrayComputer::AD2F(CpuDat *DestDat,CpuAgx DestGeom,CpuMbl SourArrBlock){

  //Variables
  int i;
  bool LastReached;
  bool Exit;
  ArrayIndexes DimValue;
  CpuWrd DestOffset;
  CpuWrd SourOffset;
  CpuAgx DestAbsGeom=_FixAgxDecoder(DestGeom);
  CpuWrd DestFactor[_MaxArrayDims];
  int SourArrIndex;
  CpuWrd SourFactor[_MaxArrayDims];

  //Check destination array
  if(DestAbsGeom>_ArrGeom.Length()-1){
    System::Throw(SysExceptionCode::ArrayGeometryInvalidIndex,ToString(DestAbsGeom),ToString(_ArrGeom.Length()-1)); 
    return false;
  }
  
  //Check source array
  if(!_DynCheckAsSource(SourArrBlock,&SourArrIndex)){ return false; }

  //Calculate array dimension factors
  _CalculateDimensionFactors(_ArrGeom[DestAbsGeom].DimNr,_ArrGeom[DestAbsGeom].CellSize,_ArrGeom[DestAbsGeom].DimSize,DestFactor);
  _CalculateDimensionFactors(_ArrMeta[SourArrIndex].DimNr,_ArrMeta[SourArrIndex].CellSize,_ArrMeta[SourArrIndex].DimSize,SourFactor);
  
  //Array element loop
  Exit=false;
  LastReached=false;
  for(i=0;i<_ArrGeom[DestAbsGeom].DimNr;i++){ DimValue.n[i]=0; }
  do{
    if(_DynHasElement(SourArrIndex,DimValue)){
      DestOffset=_CalculateArrayOffset(_ArrGeom[DestAbsGeom].DimNr,DestFactor,DimValue);
      SourOffset=_CalculateArrayOffset(_ArrMeta[SourArrIndex].DimNr,SourFactor,DimValue);
      MemCpy(&DestDat[DestOffset],&_Aux->CharPtr(SourArrBlock)[SourOffset],_ArrGeom[DestAbsGeom].CellSize);
    }
    else{
      memset(&DestDat[DestOffset],0,_ArrGeom[DestAbsGeom].CellSize);
    }
    if(LastReached){ Exit=true; }
    _FixMoveNext(DestAbsGeom,DimValue,LastReached);
  }while(!Exit);
  
  //Return code
  return true;

}

//Dynamic to dynamic array casting
bool ArrayComputer::AD2D(CpuMbl *DestArrBlock,CpuMbl SourArrBlock){

  //Variables
  int i;
  bool LastReached;
  bool Exit;
  ArrayIndexes DimValue;
  int DestArrIndex;
  CpuWrd DestArrSize;
  CpuWrd DestOffset;
  CpuWrd SourOffset;
  CpuWrd DestFactor[_MaxArrayDims];
  int SourArrIndex;
  CpuWrd SourFactor[_MaxArrayDims];

  //Check source array
  if(!_DynCheckAsSource(SourArrBlock,&SourArrIndex)){ return false; }
  
  //Check destination array (as source because it must be defined)
  if(!_DynCheckAsSource(*DestArrBlock,&DestArrIndex)){ return false; }

  //Set dimension sizes on destination array
  for(i=0;i<_ArrMeta[DestArrIndex].DimNr;i++){ 
    _ArrMeta[DestArrIndex].DimSize.n[i]=(i<=_ArrMeta[SourArrIndex].DimNr-1?_ArrMeta[SourArrIndex].DimSize.n[i]:0); 
    _ArrMeta[DestArrIndex].DimValue.n[i]=0;
  }

  //Reallocate destination array memory buffer
  DestArrSize=_DynCalculateArraySize(DestArrIndex);
  if(!_Aux->Realloc(_ScopeId,_ScopeNr,*DestArrBlock,DestArrSize)){
    _DynFreeArrIndex(DestArrIndex);
    _Aux->Free(*DestArrBlock);
    (*DestArrBlock)=0;
    System::Throw(SysExceptionCode::ArrayBlockAllocationFailure);
    return false;
  }

  //Calculate array dimension factors
  _CalculateDimensionFactors(_ArrMeta[DestArrIndex].DimNr,_ArrMeta[DestArrIndex].CellSize,_ArrMeta[DestArrIndex].DimSize,DestFactor);
  _CalculateDimensionFactors(_ArrMeta[SourArrIndex].DimNr,_ArrMeta[SourArrIndex].CellSize,_ArrMeta[SourArrIndex].DimSize,SourFactor);
  
  //Array element loop
  Exit=false;
  LastReached=false;
  for(i=0;i<_ArrMeta[DestArrIndex].DimNr;i++){ DimValue.n[i]=0; }
  do{
    if(_DynHasElement(SourArrIndex,DimValue)){
      DestOffset=_CalculateArrayOffset(_ArrMeta[DestArrIndex].DimNr,DestFactor,DimValue);
      SourOffset=_CalculateArrayOffset(_ArrMeta[SourArrIndex].DimNr,SourFactor,DimValue);
      MemCpy(&_Aux->CharPtr(*DestArrBlock)[DestOffset],&_Aux->CharPtr(SourArrBlock)[SourOffset],_ArrMeta[DestArrIndex].CellSize);
    }
    else{
      memset(&_Aux->CharPtr(*DestArrBlock)[DestOffset],0,_ArrMeta[DestArrIndex].CellSize);
    }
    if(LastReached){ Exit=true; }
    _DynMoveNext(DestArrIndex,DimValue,LastReached);
  }while(!Exit);
  
  //Return code
  return true;

}

//string[] array interface: STAOPR - Open for read
bool ArrayComputer::STAOPR(CpuMbl ArrBlock,CpuWrd *Lines){

  //Variables
  int ArrIndex;
  
  //Check operation not open already
  if(_StaOpr!=(StaOperation)0){
    System::Throw(SysExceptionCode::StaOperationAlreadyOpen); 
    return false;
  }

  //Check array as source
  if(!_DynCheckAsSource(ArrBlock,&ArrIndex)){ return false; }
  
  //Init string[] interface variables
  _StaArrBlock=ArrBlock;
  _StaArrIndex=ArrIndex;
  _StaOpr=StaOperation::Read;
  _StaLines=_ArrMeta[ArrIndex].DimSize.n[0];
  _StaWrittenLines=0;
  _StaReservedLines=0;
  
  //Return number of lines
  *Lines=_StaLines;

  //Debug message
  DebugMessage(DebugLevel::VrmRuntime,"STA: Open for read (arrblock="+HEXFORMAT(ArrBlock)+")");
  
  //Return code
  return true;

}

//string[] array interface: STARDL - Read line
bool ArrayComputer::STARDL(CpuWrd Index,CpuMbl *StrBlock){
  if(_StaOpr==(StaOperation)0){
    System::Throw(SysExceptionCode::StaOperationNotOpen); 
    return false;
  }
  if(Index<0 || Index>_StaLines-1){
    System::Throw(SysExceptionCode::StaInvalidReadIndex,ToString(_StaArrBlock),ToString(Index),ToString(_StaLines-1)); 
    return false;
  }
  else{
    *StrBlock=*(CpuMbl *)&_Aux->CharPtr(_StaArrBlock)[Index*sizeof(CpuMbl)];
  }
  DebugMessage(DebugLevel::VrmRuntime,"STA: Read line (arrblock="+HEXFORMAT(_StaArrBlock)+")");
  return true;
}

//string[] array interface: STAOPW - Open for write
bool ArrayComputer::STAOPW(CpuMbl *ArrBlock){
  
  //Variables
  int ArrIndex;
  
  //Check operation not open already
  if(_StaOpr!=(StaOperation)0){
    System::Throw(SysExceptionCode::StaOperationAlreadyOpen); 
    return false;
  }

  //Check array as destination
  if(!_DynCheckAsDestin(ArrBlock,sizeof(CpuMbl)*_StaChunkLines,&ArrIndex)){ return false; }
  
  //Initialize array metadata
  _ArrMeta[ArrIndex].DimNr=1;
  _ArrMeta[ArrIndex].CellSize=sizeof(CpuMbl);
  _ArrMeta[ArrIndex].DimSize.n[0]=0;

  //Init string[] interface variables
  _StaArrBlock=*ArrBlock;
  _StaArrIndex=ArrIndex;
  _StaOpr=StaOperation::Write;
  _StaLines=0;
  _StaWrittenLines=0;
  _StaReservedLines=0;

  //Debug message
  DebugMessage(DebugLevel::VrmRuntime,"STA: Open for write (arrblock="+HEXFORMAT(*ArrBlock)+")");
  
  //Return code
  return true;

}

//string[] array interface: STAWRL - Write line
bool ArrayComputer::STAWRL(CpuMbl StrBlock){
  if(_StaOpr==(StaOperation)0){
    System::Throw(SysExceptionCode::StaOperationNotOpen); 
    return false;
  }
  if(!_Aux->IsValid(StrBlock)){ 
    System::Throw(SysExceptionCode::InvalidStringBlock,ToString(StrBlock)); 
    return false;
  }
  _StaWrittenLines++;
  if(_StaWrittenLines>_StaReservedLines){
    _StaReservedLines+=_StaChunkLines;
    if(!_Aux->Realloc(_ScopeId,_ScopeNr,_StaArrBlock,sizeof(CpuMbl)*_StaReservedLines)){
      _DynFreeArrIndex(_StaArrIndex);
      _Aux->Free(_StaArrBlock);
      _StaArrBlock=0;
      System::Throw(SysExceptionCode::ArrayBlockAllocationFailure);
      return false;
    }
  }
  if(!_StC->SCOPY(&((CpuMbl *)_Aux->CharPtr(_StaArrBlock))[_StaWrittenLines-1],_Aux->CharPtr(StrBlock),_Aux->GetLen(StrBlock))){ return false; }
  _ArrMeta[_StaArrIndex].DimSize.n[0]=_StaWrittenLines;
  DebugMessage(DebugLevel::VrmRuntime,"STA: Write line from string block (arrblock="+HEXFORMAT(_StaArrBlock)+")");
  return true;
}

//string[] array interface: STAWRL - Write line
bool ArrayComputer::STAWRL(const String& Line){
  if(_StaOpr==(StaOperation)0){
    System::Throw(SysExceptionCode::StaOperationNotOpen); 
    return false;
  }
  _StaWrittenLines++;
  if(_StaWrittenLines>_StaReservedLines){
    _StaReservedLines+=_StaChunkLines;
    if(!_Aux->Realloc(_ScopeId,_ScopeNr,_StaArrBlock,sizeof(CpuMbl)*_StaReservedLines)){
      _DynFreeArrIndex(_StaArrIndex);
      _Aux->Free(_StaArrBlock);
      _StaArrBlock=0;
      System::Throw(SysExceptionCode::ArrayBlockAllocationFailure);
      return false;
    }
  }
  if(!_StC->SCOPY(&((CpuMbl *)_Aux->CharPtr(_StaArrBlock))[_StaWrittenLines-1],Line.CharPnt(),Line.Length())){ return false; }
  _ArrMeta[_StaArrIndex].DimSize.n[0]=_StaWrittenLines;
  DebugMessage(DebugLevel::VrmRuntime,"STA: Write line from string class (arrblock="+HEXFORMAT(_StaArrBlock)+")");
  return true;
}

//string[] array interface: STACLO - Close operation
bool ArrayComputer::STACLO(){
  
  //Check operation closd open already
  if(_StaOpr==(StaOperation)0){
    System::Throw(SysExceptionCode::StaOperationAlreadyClosed); 
    return false;
  }

  //Final array reallocation to return exactly written lines
  if(_StaOpr==StaOperation::Write && _StaWrittenLines!=_StaReservedLines){
    if(!_Aux->Realloc(_ScopeId,_ScopeNr,_StaArrBlock,sizeof(CpuMbl)*_StaWrittenLines)){
      _DynFreeArrIndex(_StaArrIndex);
      _Aux->Free(_StaArrBlock);
      _StaArrBlock=0;
      System::Throw(SysExceptionCode::ArrayBlockAllocationFailure);
      return false;
    }
  }

  //Debug message
  DebugMessage(DebugLevel::VrmRuntime,"STA: Close array (arrblock="+HEXFORMAT(_StaArrBlock)+")");

  //Close operation
  _StaArrBlock=-1;
  _StaArrIndex=-1;
  _StaOpr=(StaOperation)0;
  _StaLines=0;
  _StaWrittenLines=0;
  _StaReservedLines=0;

  //Return code
  return true;

}

//Return command line arguments as string[]
bool ArrayComputer::GETARG(CpuMbl *Arr,int ArgNr,char *Arg[],int ArgStart){

  //Split part;
  struct SplitPart{
    CpuWrd Pos;
    CpuWrd Len;
  };

  //Variables
  int i;
  int j;
  int ArrIndex;
  int Elements;
  CpuWrd ArrSize;

  //Calculate array size
  Elements=ArgNr-ArgStart;
  ArrSize=sizeof(CpuMbl)*Elements;

  //Check array as destination
  if(!_DynCheckAsDestin(Arr,ArrSize,&ArrIndex)){ return false; }

  //Reallocate size and set metadata
  if(!_Aux->Realloc(_ScopeId,_ScopeNr,*Arr,ArrSize)){
    _DynFreeArrIndex(ArrIndex);
    _Aux->Free(*Arr);
    *Arr=0;
    System::Throw(SysExceptionCode::ArrayBlockAllocationFailure);
    return false;
  }
  _ArrMeta[ArrIndex].DimNr=1;
  _ArrMeta[ArrIndex].CellSize=sizeof(CpuMbl);
  _ArrMeta[ArrIndex].DimSize.n[0]=Elements;

  //Copy arguments
  j=0;
  for(i=ArgStart;i<ArgNr;i++){
    if(!_StC->SCOPY((CpuMbl *)&_Aux->CharPtr(*Arr)[j*sizeof(CpuMbl)],(char *)Arg[i])){ 
      _Aux->Free(*Arr);
      _DynFreeArrIndex(ArrIndex);
      *Arr=0;
      return false; 
    }
    j++;
  }

  //Return code
  return true;

}

