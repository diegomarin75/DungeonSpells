//strcomp.hpp: CPU string computer

//Wrap include
#ifndef _STRCOMP_HPP
#define _STRCOMP_HPP

//String computer class
class StringComputer{
  
  //Private menbers
  private:

    //Internal data
    int _ScopeId;
    CpuLon _ScopeNr;
    AuxMemoryManager *_Aux;
    CpuMbl _Prv;

    //Internal methods
    inline bool _EmptyAlloc(CpuMbl *Str){
      if(!_Aux->EmptyAlloc(_ScopeId,_ScopeNr,Str)){ 
        System::Throw(SysExceptionCode::NullStringAllocationError); 
        return false; 
      }
      return true;
    }
    inline bool _NewString(CpuMbl *Str,CpuWrd Length){
      if(!_Aux->Alloc(_ScopeId,_ScopeNr,Length+1,-1,Str)){ 
        System::Throw(SysExceptionCode::StringAllocationError,ToString(Length)); 
        return false; 
      }
      return true;
    }
    inline bool _Allocate(CpuMbl Str,CpuWrd Length){
      if(!_Aux->Realloc(_ScopeId,_ScopeNr,Str,Length+1)){ 
        System::Throw(SysExceptionCode::StringAllocationError,ToString(Length)); 
        return false; 
      }
      return true;
    }

  //Public members
  public:
    
    //Handler methods
    void Init(AuxMemoryManager *Aux);
    inline void SetScope(int ScopeId,CpuLon ScopeNr){ _ScopeId=ScopeId; _ScopeNr=ScopeNr; }
    inline bool IsValid(CpuMbl Str,bool Throw){ if(!_Aux->IsValid(Str) || Str==0){ if(Throw){ System::Throw(SysExceptionCode::InvalidStringBlock,ToString(Str)); } return false; } return true; }
    inline char *CharPtr(CpuMbl Str){ return _Aux->CharPtr(Str); }
    inline bool IsAllocated(CpuMbl Str){ return _Aux->IsValid(Str); }

    //String operations
    bool SEMP(CpuMbl *Des);
    bool SCOPY(CpuMbl *Des,char *Src);
    bool SCOPY(CpuMbl *Des,char *Src,CpuWrd Length);
    bool SCOPY(CpuMbl *Des,CpuMbl Src);
    bool SSWCP(CpuMbl *Des,CpuMbl Src);
    bool SLES(CpuBol *Res,CpuMbl Str1,CpuMbl Str2);
    bool SLEQ(CpuBol *Res,CpuMbl Str1,CpuMbl Str2);
    bool SGRE(CpuBol *Res,CpuMbl Str1,CpuMbl Str2);
    bool SGEQ(CpuBol *Res,CpuMbl Str1,CpuMbl Str2);
    bool SEQU(CpuBol *Res,CpuMbl Str1,CpuMbl Str2);
    bool SDIS(CpuBol *Res,CpuMbl Str1,CpuMbl Str2);
    bool SLEN(CpuWrd *Res,CpuMbl Str);
    bool SMID(CpuMbl *Res,CpuMbl Str,CpuWrd Pos,CpuWrd Len);
    bool SINDX(CpuRef *Res,CpuMbl Str,CpuWrd Idx);
    bool SRGHT(CpuMbl *Res,CpuMbl Str,CpuWrd Len);
    bool SLEFT(CpuMbl *Res,CpuMbl Str,CpuWrd Len);
    bool SCUTR(CpuMbl *Res,CpuMbl Str,CpuWrd Len);
    bool SCUTL(CpuMbl *Res,CpuMbl Str,CpuWrd Len);
    bool SAPPN(CpuMbl *Des,char *Src);
    bool SAPPN(CpuMbl *Des,char *Src,CpuWrd Length);
    bool SAPPN(CpuMbl *Des,CpuMbl Str);
    bool SCONC(CpuMbl *Res,CpuMbl Str1,CpuMbl Str2);
    bool SMVCO(CpuMbl *Des,CpuMbl Str);
    bool SMVRC(CpuMbl *Des,CpuMbl Str);
    bool SFIND(CpuWrd *Res,CpuMbl Str,CpuMbl Sub,CpuWrd Beg);
    bool SSUBS(CpuMbl *Res,CpuMbl Str,CpuMbl Old,CpuMbl New);
    bool STRIM(CpuMbl *Res,CpuMbl Str);
    bool SUPPR(CpuMbl *Res,CpuMbl Str);
    bool SLOWR(CpuMbl *Res,CpuMbl Str);
    bool SLJUS(CpuMbl *Res,CpuMbl Str,CpuWrd Len,CpuChr Fill);
    bool SRJUS(CpuMbl *Res,CpuMbl Str,CpuWrd Len,CpuChr Fill);
    bool SMATC(CpuBol *Res,CpuMbl Str,CpuMbl Regex);
    bool SLIKE(CpuBol *Res,CpuMbl Str,CpuMbl Pattern);
    bool SREPL(CpuMbl *Res,CpuMbl Str,CpuWrd Num);
    bool SSTWI(CpuBol *Res,CpuMbl Str,CpuMbl Sub);
    bool SENWI(CpuBol *Res,CpuMbl Str,CpuMbl Sub);
    bool SISBO(CpuBol *Res,CpuMbl Str);
    bool SISCH(CpuBol *Res,CpuMbl Str);
    bool SISSH(CpuBol *Res,CpuMbl Str);
    bool SISIN(CpuBol *Res,CpuMbl Str);
    bool SISLO(CpuBol *Res,CpuMbl Str);
    bool SISFL(CpuBol *Res,CpuMbl Str);
    bool SST2B(CpuBol *Res,CpuMbl Str);
    bool SST2C(CpuChr *Res,CpuMbl Str);
    bool SST2W(CpuShr *Res,CpuMbl Str);
    bool SST2I(CpuInt *Res,CpuMbl Str);
    bool SST2L(CpuLon *Res,CpuMbl Str);
    bool SST2F(CpuFlo *Res,CpuMbl Str);
    bool SBO2S(CpuMbl *Res,CpuBol Bol);
    bool SCH2S(CpuMbl *Res,CpuChr Chr);
    bool SSH2S(CpuMbl *Res,CpuShr Shr);
    bool SIN2S(CpuMbl *Res,CpuInt Int);
    bool SLO2S(CpuMbl *Res,CpuLon Lon);
    bool SFL2S(CpuMbl *Res,CpuFlo Flo);
    bool SCHFM(CpuMbl *Res,CpuChr Chr,CpuMbl Fmt);
    bool SSHFM(CpuMbl *Res,CpuShr Shr,CpuMbl Fmt);
    bool SINFM(CpuMbl *Res,CpuInt Int,CpuMbl Fmt);
    bool SLOFM(CpuMbl *Res,CpuLon Lon,CpuMbl Fmt);
    bool SFLFM(CpuMbl *Res,CpuFlo Flo,CpuMbl Fmt);

    //Constructors/Destructors
    StringComputer(){}
    ~StringComputer(){}

};

#endif