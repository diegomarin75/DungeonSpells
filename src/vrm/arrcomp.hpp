//arrcomp.hpp: CPU array computer

//Wrap include
#ifndef _ARRCOMP_HPP
#define _ARRCOMP_HPP

//Array computer class
class ArrayComputer{
  
  //Private menbers
  private:

    //Array geometries
    struct ArrayGeometry{
      int DimNr;
      CpuWrd CellSize;
      ArrayIndexes DimSize;
      ArrayIndexes DimValue;
      CpuWrd LoopIndex;
      CpuAdr IndexVarAddr;
      CpuDecMode IndexVarMode;
      CpuAdr ExitAdr;
    };
    
    //Array metadata (used by runtime to store array metadata, more information is necessary)
    struct ArrayMeta{
      int ScopeId;
      CpuLon ScopeNr;
      bool Used;
      int DimNr;
      CpuWrd CellSize;
      ArrayIndexes PrevSize;
      ArrayIndexes DimSize;
      ArrayIndexes DimValue;
      CpuWrd LoopIndex;
      CpuAdr IndexVarAddr;
      CpuDecMode IndexVarMode;
      CpuAdr ExitAdr;
    };
    
    //Fixed array internal data
    RamBuffer<ArrayGeometry> _ArrGeom; //Array geometries
    CpuAgx _ABP;                       //Array geometry base pointer

    //Dyn array internal data
    int _ScopeId;
    CpuLon _ScopeNr;
    AuxMemoryManager *_Aux;
    StringComputer *_StC;
    RamBuffer<ArrayMeta> _ArrMeta; //Array metadata

    //Fixed array internal methods
    inline CpuAgx _FixAgxDecoder(CpuAgx ArrGeom){ return (ArrGeom&ARRGEOMMASK80?ArrGeom&ARRGEOMMASK7F:_ABP+ArrGeom); }
    bool _FixCreateIndex(CpuAgx AbsGeom);
    void _FixMoveNext(CpuAgx AbsGeom,ArrayIndexes& DimValue,bool& LastReached);
    bool _FixHasElement(CpuAgx AbsGeom,ArrayIndexes& DimValue);

    //Dyn array internal methods
    int _DynNewArrIndex();
    void _DynFreeArrIndex(int ArrIndex);
    CpuWrd _DynGetElements(int ArrIndex);
    CpuWrd _DynCalculateArraySize(int ArrIndex);
    void _DynCopyArrayElements(int DimNr,CpuWrd CellSize,CpuMbl SrcBlock, ArrayIndexes SrcDims,CpuMbl DstBlock, ArrayIndexes DstDims);
    bool _DynCheckAsDestin(CpuMbl *ArrBlock,CpuWrd InitSize,int *ArrIndex);
    bool _DynCheckAsSource(CpuMbl ArrBlock,int *ArrIndex);
    void _DynMoveNext(int ArrIndex,ArrayIndexes& DimValue,bool& LastReached);
    bool _DynHasElement(int ArrIndex,ArrayIndexes& DimValue);

  //Public members
  public:
    
    //Fixed array public methods
    void FixInit(int ProcessId,long ArrGeomChunkSize);
    inline void FixSetBP(CpuAgx BasePointer){ _ABP=BasePointer; }
    inline CpuAgx FixGetBP(){ return _ABP; }
    inline CpuWrd FixGetCellSize(CpuAgx ArrGeom){ return _ArrGeom[_FixAgxDecoder(ArrGeom)].CellSize; }
    CpuWrd FixGetElements(CpuAgx ArrGeom);
    bool FixStoreGeom(int DimNr,CpuWrd CellSize,ArrayIndexes DimSize);
    String FixPrint(CpuAgx ArrGeom);
    
    //Dyn array public methods
    void DynInit(int ProcessId,long ArrMetaChunkSize,AuxMemoryManager *Aux,StringComputer *StC);
    void DynSetScope(int ScopeId,CpuLon ScopeNr);
    CpuWrd DynGetCellSize(CpuMbl ArrBlock){ return _ArrMeta[_Aux->GetArrIndex(ArrBlock)].CellSize; }
    CpuWrd DynGetElements(CpuMbl ArrBlock){ return _DynGetElements(_Aux->GetArrIndex(ArrBlock)); }
    bool DynStoreMeta(int ScopeId,CpuLon ScopeNr,int DimNr,CpuWrd CellSize,ArrayIndexes DimSize);
    String DynPrint(CpuMbl ArrBlock);
    
    //1-dim Fix array instructions
    bool AD1EM(CpuMbl *Arr,CpuWrd CellSize);
    bool AF1OF(CpuAgx ArrGeom,CpuWrd DimValue,CpuWrd *Offset);
    bool AF1RW(CpuAgx ArrGeom,CpuAdr IndexVarAddr,CpuDecMode IndexVarMode,CpuAdr ExitAdr);
    bool AF1FO(CpuAgx ArrGeom,CpuWrd *Offset,CpuAdr *JumpAdr);
    bool AF1NX(CpuAgx ArrGeom,CpuAdr *IndexVarAddr,CpuDecMode *IndexVarMode);

    //Fixed array instructions
    bool AFDEF(CpuAgx ArrGeom,int DimNr,CpuWrd CellSize);
    bool AFSSZ(CpuAgx ArrGeom,int DimIndex,CpuWrd Size);
    bool AFGET(CpuAgx ArrGeom,int DimIndex,CpuWrd *Size);
    bool AFIDX(CpuAgx ArrGeom,int DimIndex,CpuWrd DimValue);
    bool AFOFN(CpuAgx ArrGeom,CpuWrd *Offset);
    bool AF1SJ(CpuMbl *Str,char *Data,CpuAgx ArrGeom,CpuMbl Sep);
    bool AF1CJ(CpuMbl *Str,char *Data,CpuAgx ArrGeom,CpuMbl Sep);

    //1-dim Dyn array instructions
    bool AD1EM(CpuMbl *Arr);
    bool AD1DF(CpuMbl *ArrBlock);
    bool AD1AP(CpuMbl ArrBlock,CpuWrd *Offset,CpuWrd CellSize);
    bool AD1IN(CpuMbl ArrBlock,CpuWrd DimValue,CpuWrd *Offset,CpuWrd CellSize);
    bool AD1DE(CpuMbl ArrBlock,CpuWrd DimValue);
    bool AD1OF(CpuMbl ArrBlock,CpuWrd DimValue,CpuWrd *Offset);
    bool AD1RS(CpuMbl *ArrBlock);
    bool AD1RW(CpuMbl ArrBlock,CpuAdr IndexVarAddr,CpuDecMode IndexVarMode,CpuAdr ExitAdr);
    bool AD1FO(CpuMbl ArrBlock,CpuWrd *Offset,CpuAdr *JumpAdr);
    bool AD1NX(CpuMbl ArrBlock,CpuAdr *IndexVarAddr,CpuDecMode *IndexVarMode);
    bool AD1SJ(CpuMbl *Str,CpuMbl ArrBlock,CpuMbl Sep);
    bool AD1CJ(CpuMbl *Str,CpuMbl ArrBlock,CpuMbl Sep);

    //Dyn array instructions
    bool ACOPY(CpuMbl *Destin,CpuMbl Source);
    bool ADEMP(CpuMbl *Destin,int DimNr,CpuWrd CellSize);
    bool ADDEF(CpuMbl *ArrBlock,int DimNr,CpuWrd CellSize);
    bool ADSET(CpuMbl *ArrBlock,int DimIndex,CpuWrd Size);
    bool ADRSZ(CpuMbl *ArrBlock);
    bool ADGET(CpuMbl ArrBlock,int DimIndex,CpuWrd *Size);
    bool ADRST(CpuMbl *ArrBlock);
    bool ADIDX(CpuMbl ArrBlock,int DimIndex,CpuWrd DimValue);
    bool ADOFN(CpuMbl ArrBlock,CpuWrd *Offset);
    bool ADSIZ(CpuMbl ArrBlock,CpuWrd *Size);
    bool SSPL(CpuMbl *ArrBlock,CpuMbl Str,CpuMbl Sep);
    bool ADVCP(CpuMbl *ArrBlock,void *Data,CpuLon Elements);
    bool RDCH(CpuBol *RetCode,CpuInt Handler,CpuMbl *ArrBlock,CpuLon Length);
    bool WRCH(CpuBol *RetCode,CpuInt Handler,CpuMbl ArrBlock,CpuLon Length);
    bool RDALCH(CpuBol *RetCode,CpuInt Handler,CpuMbl *ArrBlock);
    bool WRALCH(CpuBol *RetCode,CpuInt Handler,CpuMbl ArrBlock);
    bool RDALST(CpuBol *RetCode,CpuInt Handler,CpuMbl *ArrBlock);
    bool WRALST(CpuBol *RetCode,CpuInt Handler,CpuMbl ArrBlock);
    bool TOCA(CpuMbl *ArrBlock,CpuDat *Data,CpuWrd Length);
    bool STOCA(CpuMbl *ArrBlock,CpuMbl Str);
    bool ATOCA(CpuMbl *Destin,CpuMbl Source);
    bool FRCA(CpuDat *Data,CpuMbl ArrBlock,CpuWrd Length);
    bool SFRCA(CpuMbl *Str,CpuMbl ArrBlock);
    bool AFRCA(CpuMbl Destin,CpuMbl Source);

    //Array casting instructions
    bool AF2F(CpuDat *DestDat,CpuAgx DestGeom,CpuDat *SourDat,CpuAgx SourGeom);
    bool AF2D(CpuMbl *DestArrBlock,CpuDat *SourDat,CpuAgx SourGeom);
    bool AD2F(CpuDat *DestDat,CpuAgx DestGeom,CpuMbl SourArrBlock);
    bool AD2D(CpuMbl *DestArrBlock,CpuMbl SourArrBlock);

    //string[] array interface 
    bool STAOPR(CpuMbl ArrBlock,CpuWrd *Lines);
    bool STARDL(CpuWrd Index,CpuMbl *StrBlock);
    bool STAOPW(CpuMbl *ArrBlock);
    bool STAWRL(CpuMbl StrBlock);
    bool STAWRL(const String& Line);
    bool STACLO();

    //Command line arguments
    bool GETARG(CpuMbl *Arr,int ArgNr,char *Arg[],int ArgStart);
};

#endif