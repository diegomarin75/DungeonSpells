//Binary.cpp: Binar file  class
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
#include "cmp/binary.hpp"

//Constants
const int _DefaultAsmIndentation=36;
const int _HexIndentation=100;
const String _AsmComment=";";
const String _AsmSeparator=_AsmComment+String(_HexIndentation-1,'-');
const String _AsmGlobVarTemplate="[name]";
const String _AsmLoclVarTemplate="<name>";
const String _AsmFunctionTemplate="(name)";
const String _AsmJumpAddrTemplate="{name}";
const String _AsmLitValVarPrefix=SYSTEM_NAMESPACE "ltv";
const String _AsmLitValVarTemplate=_AsmLitValVarPrefix+"<type>_<value>h";
const String _AsmReplIdBeg="[$$replid:";
const String _AsmReplIdEnd="]";

//Meta instruction table
struct MetaInst{
  String Mnemonic;
  CpuInstCode Inst[8];
};
    
//Meta instruction table
const int _MetaNr=34;
const MetaInst _Meta[_MetaNr]={
// Mnemo    CpuDataType::Boolean CpuDataType::Char    CpuDataType::Short   CpuDataType::Integer CpuDataType::Long    CpuDataType::Float   CpuDataType::StrBlk           
{ "NEG"  , {(CpuInstCode)-1    , CpuInstCode::NEGc  , CpuInstCode::NEGw  , CpuInstCode::NEGi  , CpuInstCode::NEGl  , CpuInstCode::NEGf  , (CpuInstCode)-1    } }, //Negative conversion
{ "ADD"  , {(CpuInstCode)-1    , CpuInstCode::ADDc  , CpuInstCode::ADDw  , CpuInstCode::ADDi  , CpuInstCode::ADDl  , CpuInstCode::ADDf  , CpuInstCode::SCONC } }, //Addition
{ "SUB"  , {(CpuInstCode)-1    , CpuInstCode::SUBc  , CpuInstCode::SUBw  , CpuInstCode::SUBi  , CpuInstCode::SUBl  , CpuInstCode::SUBf  , (CpuInstCode)-1    } }, //Substraction
{ "MUL"  , {(CpuInstCode)-1    , CpuInstCode::MULc  , CpuInstCode::MULw  , CpuInstCode::MULi  , CpuInstCode::MULl  , CpuInstCode::MULf  , (CpuInstCode)-1    } }, //Multiplication
{ "DIV"  , {(CpuInstCode)-1    , CpuInstCode::DIVc  , CpuInstCode::DIVw  , CpuInstCode::DIVi  , CpuInstCode::DIVl  , CpuInstCode::DIVf  , (CpuInstCode)-1    } }, //Division
{ "MOD"  , {(CpuInstCode)-1    , CpuInstCode::MODc  , CpuInstCode::MODw  , CpuInstCode::MODi  , CpuInstCode::MODl  , (CpuInstCode)-1    , (CpuInstCode)-1    } }, //Modulus
{ "INC"  , {(CpuInstCode)-1    , CpuInstCode::INCc  , CpuInstCode::INCw  , CpuInstCode::INCi  , CpuInstCode::INCl  , CpuInstCode::INCf  , (CpuInstCode)-1    } }, //Increment
{ "DEC"  , {(CpuInstCode)-1    , CpuInstCode::DECc  , CpuInstCode::DECw  , CpuInstCode::DECi  , CpuInstCode::DECl  , CpuInstCode::DECf  , (CpuInstCode)-1    } }, //Decrement
{ "PINC" , {(CpuInstCode)-1    , CpuInstCode::PINCc , CpuInstCode::PINCw , CpuInstCode::PINCi , CpuInstCode::PINCl , CpuInstCode::PINCf , (CpuInstCode)-1    } }, //Postfix increment
{ "PDEC" , {(CpuInstCode)-1    , CpuInstCode::PDECc , CpuInstCode::PDECw , CpuInstCode::PDECi , CpuInstCode::PDECl , CpuInstCode::PDECf , (CpuInstCode)-1    } }, //Postfix decrement
{ "BNOT" , {(CpuInstCode)-1    , CpuInstCode::BNOTc , CpuInstCode::BNOTw , CpuInstCode::BNOTi , CpuInstCode::BNOTl , (CpuInstCode)-1    , (CpuInstCode)-1    } }, //Bitwise not
{ "BAND" , {(CpuInstCode)-1    , CpuInstCode::BANDc , CpuInstCode::BANDw , CpuInstCode::BANDi , CpuInstCode::BANDl , (CpuInstCode)-1    , (CpuInstCode)-1    } }, //Bitwise and
{ "BOR"  , {(CpuInstCode)-1    , CpuInstCode::BORc  , CpuInstCode::BORw  , CpuInstCode::BORi  , CpuInstCode::BORl  , (CpuInstCode)-1    , (CpuInstCode)-1    } }, //Bitwise or
{ "BXOR" , {(CpuInstCode)-1    , CpuInstCode::BXORc , CpuInstCode::BXORw , CpuInstCode::BXORi , CpuInstCode::BXORl , (CpuInstCode)-1    , (CpuInstCode)-1    } }, //Bitwise xor
{ "SHL"  , {(CpuInstCode)-1    , CpuInstCode::SHLc  , CpuInstCode::SHLw  , CpuInstCode::SHLi  , CpuInstCode::SHLl  , (CpuInstCode)-1    , (CpuInstCode)-1    } }, //Shift left
{ "SHR"  , {(CpuInstCode)-1    , CpuInstCode::SHRc  , CpuInstCode::SHRw  , CpuInstCode::SHRi  , CpuInstCode::SHRl  , (CpuInstCode)-1    , (CpuInstCode)-1    } }, //Shift right
{ "LES"  , {CpuInstCode::LESb  , CpuInstCode::LESc  , CpuInstCode::LESw  , CpuInstCode::LESi  , CpuInstCode::LESl  , CpuInstCode::LESf  , CpuInstCode::LESs  } }, //Less
{ "LEQ"  , {CpuInstCode::LEQb  , CpuInstCode::LEQc  , CpuInstCode::LEQw  , CpuInstCode::LEQi  , CpuInstCode::LEQl  , CpuInstCode::LEQf  , CpuInstCode::LEQs  } }, //Less or equal
{ "GRE"  , {CpuInstCode::GREb  , CpuInstCode::GREc  , CpuInstCode::GREw  , CpuInstCode::GREi  , CpuInstCode::GREl  , CpuInstCode::GREf  , CpuInstCode::GREs  } }, //Greater
{ "GEQ"  , {CpuInstCode::GEQb  , CpuInstCode::GEQc  , CpuInstCode::GEQw  , CpuInstCode::GEQi  , CpuInstCode::GEQl  , CpuInstCode::GEQf  , CpuInstCode::GEQs  } }, //Greater or equal
{ "EQU"  , {CpuInstCode::EQUb  , CpuInstCode::EQUc  , CpuInstCode::EQUw  , CpuInstCode::EQUi  , CpuInstCode::EQUl  , CpuInstCode::EQUf  , CpuInstCode::EQUs  } }, //Equal
{ "DIS"  , {CpuInstCode::DISb  , CpuInstCode::DISc  , CpuInstCode::DISw  , CpuInstCode::DISi  , CpuInstCode::DISl  , CpuInstCode::DISf  , CpuInstCode::DISs  } }, //Distinct
{ "LOAD" , {CpuInstCode::LOADb , CpuInstCode::LOADc , CpuInstCode::LOADw , CpuInstCode::LOADi , CpuInstCode::LOADl , CpuInstCode::LOADf , (CpuInstCode)-1    } }, //Load litteral value
{ "MV"   , {CpuInstCode::MVb   , CpuInstCode::MVc   , CpuInstCode::MVw   , CpuInstCode::MVi   , CpuInstCode::MVl   , CpuInstCode::MVf   , CpuInstCode::SCOPY } }, //Move
{ "MVAD" , {(CpuInstCode)-1    , CpuInstCode::MVADc , CpuInstCode::MVADw , CpuInstCode::MVADi , CpuInstCode::MVADl , CpuInstCode::MVADf , CpuInstCode::SMVCO } }, //Move and addition
{ "MVSU" , {(CpuInstCode)-1    , CpuInstCode::MVSUc , CpuInstCode::MVSUw , CpuInstCode::MVSUi , CpuInstCode::MVSUl , CpuInstCode::MVSUf , (CpuInstCode)-1    } }, //Move and substraction
{ "MVMU" , {(CpuInstCode)-1    , CpuInstCode::MVMUc , CpuInstCode::MVMUw , CpuInstCode::MVMUi , CpuInstCode::MVMUl , CpuInstCode::MVMUf , (CpuInstCode)-1    } }, //Moveand multiplication
{ "MVDI" , {(CpuInstCode)-1    , CpuInstCode::MVDIc , CpuInstCode::MVDIw , CpuInstCode::MVDIi , CpuInstCode::MVDIl , CpuInstCode::MVDIf , (CpuInstCode)-1    } }, //Move and division
{ "MVMO" , {(CpuInstCode)-1    , CpuInstCode::MVMOc , CpuInstCode::MVMOw , CpuInstCode::MVMOi , CpuInstCode::MVMOl , (CpuInstCode)-1    , (CpuInstCode)-1    } }, //Move and modulus
{ "MVSL" , {(CpuInstCode)-1    , CpuInstCode::MVSLc , CpuInstCode::MVSLw , CpuInstCode::MVSLi , CpuInstCode::MVSLl , (CpuInstCode)-1    , (CpuInstCode)-1    } }, //Move and shift left
{ "MVSR" , {(CpuInstCode)-1    , CpuInstCode::MVSRc , CpuInstCode::MVSRw , CpuInstCode::MVSRi , CpuInstCode::MVSRl , (CpuInstCode)-1    , (CpuInstCode)-1    } }, //Move and shift right
{ "MVAN" , {(CpuInstCode)-1    , CpuInstCode::MVANc , CpuInstCode::MVANw , CpuInstCode::MVANi , CpuInstCode::MVANl , (CpuInstCode)-1    , (CpuInstCode)-1    } }, //Move and bitwise and
{ "MVXO" , {(CpuInstCode)-1    , CpuInstCode::MVXOc , CpuInstCode::MVXOw , CpuInstCode::MVXOi , CpuInstCode::MVXOl , (CpuInstCode)-1    , (CpuInstCode)-1    } }, //Move and bitwise xor
{ "MVOR" , {(CpuInstCode)-1    , CpuInstCode::MVORc , CpuInstCode::MVORw , CpuInstCode::MVORi , CpuInstCode::MVORl , (CpuInstCode)-1    , (CpuInstCode)-1    } }  //Move and bitwise or
};

//Binary length of argument
int AsmArg::BinLength() const {
  int Length;
  switch(AdrMode){
    case CpuAdrMode::LitValue:
      switch(Type){      
        case CpuDataType::Boolean:  Length=sizeof(CpuBol); break;
        case CpuDataType::Char:     Length=sizeof(CpuChr); break;
        case CpuDataType::Short:    Length=sizeof(CpuShr); break;
        case CpuDataType::Integer:  Length=sizeof(CpuInt); break;
        case CpuDataType::Long:     Length=sizeof(CpuLon); break;
        case CpuDataType::Float:    Length=sizeof(CpuFlo); break;
        case CpuDataType::ArrGeom:  Length=sizeof(CpuAgx); break;
        case CpuDataType::FunAddr:  Length=sizeof(CpuAdr); break;
        case CpuDataType::JumpAddr: Length=sizeof(CpuAdr); break;
        case CpuDataType::VarAddr:  Length=sizeof(CpuAdr); break;
        case CpuDataType::StrBlk:
        case CpuDataType::ArrBlk: 
        case CpuDataType::Undefined: 
          SysMessage(79).Delay();
          break;
      }
      break;
    case CpuAdrMode::Address:
    case CpuAdrMode::Indirection:
      Length=sizeof(CpuAdr); 
      break;
  }
  return Length;
}

//Calculate internal representation of assembler argument as buffer
Buffer AsmArg::ToCode() const {
  
  //Variables
  Buffer Result;
  CpuAgx ArrGeom;
  CpuAdr Address;
  
  //Switch on addressing mode
  switch(AdrMode){

    //Litteral values
    case CpuAdrMode::LitValue:
      switch(Type){      
        case CpuDataType::Boolean:
          Result=(char)Value.Bol; 
          break;
        case CpuDataType::Char:
          Result=Buffer((char)Value.Chr); 
          break;
        case CpuDataType::Short: 
          { const char *p=reinterpret_cast<const char *>(&Value.Shr);
            for(int i=0;i<(int)sizeof(CpuShr);i++){ Result+=p[i]; } }
          break;
        case CpuDataType::Integer: 
          { const char *p=reinterpret_cast<const char *>(&Value.Int);
            for(int i=0;i<(int)sizeof(CpuInt);i++){ Result+=p[i]; } }
          break;
        case CpuDataType::Long: 
          { const char *p=reinterpret_cast<const char *>(&Value.Lon);
            for(int i=0;i<(int)sizeof(CpuLon);i++){ Result+=p[i]; } }
          break;
        case CpuDataType::Float: 
          { const char *p=reinterpret_cast<const char *>(&Value.Flo);
            for(int i=0;i<(int)sizeof(CpuFlo);i++){ Result+=p[i]; } }
          break;
        case CpuDataType::ArrGeom: 
          ArrGeom=Value.Agx;
          if(Glob){ ArrGeom=(ArrGeom|ARRGEOMMASK80); }
          { const char *p=reinterpret_cast<const char *>(&ArrGeom);
            for(int i=0;i<(int)sizeof(CpuAgx);i++){ Result+=p[i]; } }
          break;
        case CpuDataType::FunAddr: 
          { const char *p=reinterpret_cast<const char *>(&Value.Adr);
            for(int i=0;i<(int)sizeof(CpuAdr);i++){ Result+=p[i]; } }
          break;
        case CpuDataType::JumpAddr: 
          { const char *p=reinterpret_cast<const char *>(&Value.Adr);
            for(int i=0;i<(int)sizeof(CpuAdr);i++){ Result+=p[i]; } }
          break;
        case CpuDataType::VarAddr: 
          { const char *p=reinterpret_cast<const char *>(&Value.Adr);
            for(int i=0;i<(int)sizeof(CpuAdr);i++){ Result+=p[i]; } }
          break;
        case CpuDataType::StrBlk:
        case CpuDataType::ArrBlk: 
        case CpuDataType::Undefined: 
          SysMessage(79).Delay();
          break;
      }
      break;
  
    //Data addresses and indirections use memory addressing
    case CpuAdrMode::Address:
    case CpuAdrMode::Indirection:
      Address=Value.Adr;
      const char *p=reinterpret_cast<const char *>(&Address);
      for(int i=0;i<(int)sizeof(CpuAdr);i++){ Result+=p[i]; }
      break;
  }

  //Return result
  return Result;

}

//Calculate assembler representation of argument 
String AsmArg::ToText() const {
  
  //Variables
  String Result="";
  
  //Switch on addressing mode
  switch(AdrMode){

    //Litteral values
    case CpuAdrMode::LitValue:
      switch(Type){
        case CpuDataType::Boolean:
          Result="("+CpuDataTypeCharId(Type)+")"+(Value.Bol?"true":"false"); 
          break;
        case CpuDataType::Char:
          Result="("+CpuDataTypeCharId(Type)+")"+ToString(Value.Chr);
          break;
        case CpuDataType::Short:
          Result="("+CpuDataTypeCharId(Type)+")"+ToString(Value.Shr);
          break;
        case CpuDataType::Integer:
          Result="("+CpuDataTypeCharId(Type)+")"+ToString(Value.Int);
          break;
        case CpuDataType::Long:
          Result="("+CpuDataTypeCharId(Type)+")"+ToString(Value.Lon);
          break;
        case CpuDataType::Float:
          Result="("+CpuDataTypeCharId(Type)+")"+ToString(Value.Flo);
          break;
        case CpuDataType::ArrGeom:
          Result=(Glob?_AsmGlobVarTemplate:_AsmLoclVarTemplate).Replace("name","AGX("+ToString(Value.Agx,"%0"+ToString((int)(sizeof(CpuAgx)*2))+"X")+"h)");
          break;
        case CpuDataType::FunAddr:
          Result=_AsmFunctionTemplate.Replace("name",Name);
          break;
        case CpuDataType::JumpAddr:
          Result=_AsmJumpAddrTemplate.Replace("name",Name);
          break;
        case CpuDataType::VarAddr:
          Result="&"+(Glob?_AsmGlobVarTemplate:_AsmLoclVarTemplate).Replace("name",Name);
          break;
        case CpuDataType::StrBlk:
        case CpuDataType::ArrBlk: 
        case CpuDataType::Undefined: 
          SysMessage(79).Delay();
          break;
      }
      break;

    //Address
    case CpuAdrMode::Address:
      Result=(Glob?_AsmGlobVarTemplate:_AsmLoclVarTemplate).Replace("name",Name);
      break;
    
    //Indirection
    case CpuAdrMode::Indirection:
      Result="*"+(Glob?_AsmGlobVarTemplate:_AsmLoclVarTemplate).Replace("name",Name);
      break;
  }
  
  //Return result
  return Result;

}

//Calculate assembler representation of argument 
String AsmArg::ToHex() const {
  
  //Variables
  String Result="";
  
  //Switch on addressing mode
  switch(AdrMode){

    //Litteral values
    case CpuAdrMode::LitValue:
      
      //Switch on cpu data type
      switch(Type){
        
        //Ones with direct translation to binary
        case CpuDataType::Boolean:
        case CpuDataType::Char:
        case CpuDataType::Short:
        case CpuDataType::Integer:
        case CpuDataType::Long:
        case CpuDataType::Float:
        case CpuDataType::ArrGeom:
        case CpuDataType::VarAddr:
          Result=String(ToCode().ToHex().BuffPnt()); 
          break;
        
        //Ones with further resolution are displayed as text
        case CpuDataType::FunAddr:
        case CpuDataType::JumpAddr:
          Result=ToText();
          break;

        //Undefined are errors
        case CpuDataType::StrBlk:
        case CpuDataType::ArrBlk: 
        case CpuDataType::Undefined: 
          SysMessage(79).Delay();
          break;
      
      }
      break;
      
    //Address & Indirection
    case CpuAdrMode::Address:
      Result=String(ToCode().ToHex().BuffPnt()); 
      break;

    //Indirection
    case CpuAdrMode::Indirection:
      Result=String(ToCode().ToHex().BuffPnt()); 
      break;

  }
  
  //Return result
  return Result;

}

//AssemblerLine copy constructor
Binary::AssemblerLine::AssemblerLine(const AssemblerLine& AsmLine){
  _Move(AsmLine);
}

//Constructor from values
Binary::AssemblerLine::AssemblerLine(int Id,const String& Lin){
  NestId=Id;
  Line=Lin;
}

//AssemblerLine assignment
Binary::AssemblerLine& Binary::AssemblerLine::operator=(const AssemblerLine& AsmLine){
  _Move(AsmLine);
  return *this;
}

//AssemblerLine move
void Binary::AssemblerLine::_Move(const AssemblerLine& AsmLine){
  NestId=AsmLine.NestId;
  Line=AsmLine.Line;
}

//Binary file constructor
Binary::Binary(){
  
  //Init block buffers (Add one entry so first block is instead of 0,zerp is reserved for uninitialized block)
  _Block.Add((BlockDef{-1,(Buffer)""}));

  //Initialize global buffer with one byte to avoid usage of address 0 since it means not defined when solving undefined references in libraries
  _GlobBuffer+=0x7F;

  //Initialize fixed array geometries to start at 1 because zero means not defined when solving undefined references in libraries
  _GlobGeometryNr=1;
  StoreArrFixDef((ArrayFixDef){0,0,0,{0}});

  //Initialize variables
  _StackSize=0;
  _LoclGeometryNr=0;
  _SuperInitAdr=0;
  _HasInitAdr=false;
  _InitAddress=0;
  _AsmEnabled=false;
  _AsmHnd=-1;
  _AsmNestId=0;
  _AsmNestCounter=0;
  _MemUnitSize=DEFAULTMEMUNITSIZE;
  _MemUnits=DEFAULTMEMUNITS;
  _ChunkMemUnits=DEFAULTCHUNKMEMUNITS;
  _BlockMax=DEFAULTBLOCKMAX;
  _AsmIndentation=_DefaultAsmIndentation;
  _CompileToLibrary=false;
  _LibMajorVers=0;
  _LibMinorVers=0;
  _LibRevisionNr=0;
      
  //Init Source
  _FileName="";
  _LineNr=0;
  _ColNr=0;
}

//Return boolean assembler operand
AsmArg Binary::AsmLitBol(CpuBol Bol) const {
  AsmArg Arg;
  Arg.IsNull=false;
  Arg.IsError=false;
  Arg.IsUndefined=false;
  Arg.AdrMode=CpuAdrMode::LitValue;
  Arg.Name="";
  Arg.ObjectId="";
  Arg.Type=CpuDataType::Boolean; 
  Arg.Value.Bol=Bol;
  Arg.ScopeDepth=-1;
  return Arg;
}

//Return char assembler operand
AsmArg Binary::AsmLitChr(CpuChr Chr) const {
  AsmArg Arg;
  Arg.IsNull=false;
  Arg.IsError=false;
  Arg.IsUndefined=false;
  Arg.AdrMode=CpuAdrMode::LitValue;
  Arg.Name="";
  Arg.ObjectId="";
  Arg.Type=CpuDataType::Char; 
  Arg.Value.Chr=Chr;
  Arg.ScopeDepth=-1;
  return Arg;
}

//Return short assembler operand
AsmArg Binary::AsmLitShr(CpuShr Shr) const {
  AsmArg Arg;
  Arg.IsNull=false;
  Arg.IsError=false;
  Arg.IsUndefined=false;
  Arg.AdrMode=CpuAdrMode::LitValue;
  Arg.Name="";
  Arg.ObjectId="";
  Arg.Type=CpuDataType::Short; 
  Arg.Value.Shr=Shr;
  Arg.ScopeDepth=-1;
  return Arg;
}

//Return integer assembler operand
AsmArg Binary::AsmLitInt(CpuInt Int) const {
  AsmArg Arg;
  Arg.IsNull=false;
  Arg.IsError=false;
  Arg.IsUndefined=false;
  Arg.AdrMode=CpuAdrMode::LitValue;
  Arg.Name="";
  Arg.ObjectId="";
  Arg.Type=CpuDataType::Integer; 
  Arg.Value.Int=Int;
  Arg.ScopeDepth=-1;
  return Arg;
}

//Return long assembler operand
AsmArg Binary::AsmLitLon(CpuLon Lon) const {
  AsmArg Arg;
  Arg.IsNull=false;
  Arg.IsError=false;
  Arg.IsUndefined=false;
  Arg.AdrMode=CpuAdrMode::LitValue;
  Arg.Name="";
  Arg.ObjectId="";
  Arg.Type=CpuDataType::Long; 
  Arg.Value.Lon=Lon;
  Arg.ScopeDepth=-1;
  return Arg;
}

//Return float assembler operand
AsmArg Binary::AsmLitFlo(CpuFlo Flo) const {
  AsmArg Arg;
  Arg.IsNull=false;
  Arg.IsError=false;
  Arg.IsUndefined=false;
  Arg.AdrMode=CpuAdrMode::LitValue;
  Arg.Name="";
  Arg.ObjectId="";
  Arg.Type=CpuDataType::Float; 
  Arg.Value.Flo=Flo;
  Arg.ScopeDepth=-1;
  return Arg;
}

//Return string assembler operand (used only for litteral values)
AsmArg Binary::AsmLitStr(CpuAdr Adr) const {
  AsmArg Arg;
  Arg.IsNull=false;
  Arg.IsError=false;
  Arg.AdrMode=CpuAdrMode::Address;
  Arg.Name="STR("+ToString(Adr,"%0"+ToString((int)(sizeof(CpuAdr)*2))+"X")+"h)";
  Arg.ObjectId="";
  Arg.Type=CpuDataType::StrBlk; 
  Arg.Value.Adr=Adr;
  Arg.Glob=true;
  Arg.ScopeDepth=-1;
  return Arg;
}


//Return cpu word assembler operand
AsmArg Binary::AsmLitWrd(CpuWrd Wrd) const {
  if(GetArchitecture()==32){
    return AsmLitInt(Wrd);
  }
  else{
    return AsmLitLon(Wrd);
  }
}

//Return assembler indentation
int Binary::GetAsmIndentation(){
  return _AsmIndentation;
}

//Set assembler indentation
void Binary::SetAsmIndentation(int Indent){
  _AsmIndentation=Indent;
}

//Set assembler default indentation
void Binary::SetAsmDefaultIndentation(){
  _AsmIndentation=_DefaultAsmIndentation;
}

//Get assembler default indentation
int Binary::GetAsmDefaultIndentation(){
  return _DefaultAsmIndentation;
}

//Return hex part indentation
int Binary::GetHexIndentation(){
  return _HexIndentation;
}

//Set binary file name
void Binary::SetBinaryFile(const String& BinaryFile){
  _BinaryFile=BinaryFile;
}

//Set source
void Binary::SetSource(const SourceInfo& SrcInfo){
  _FileName=SrcInfo.FileName;
  _LineNr=SrcInfo.LineNr;
  _ColNr=SrcInfo.ColNr;
}

//Calculate function assembler argument for non-declared function
AsmArg Binary::SubroutineAsmArg(CpuAdr Address,const String& Module,const String& Name,int ScopeDepth) const {
  AsmArg Arg;
  Arg.IsNull=false;
  Arg.IsError=false;
  Arg.IsUndefined=false;
  Arg.AdrMode=CpuAdrMode::LitValue;
  Arg.Name=Module+"_"+Name;
  Arg.ObjectId=Module+"."+Name+"()"+ObjIdSep+Module;
  Arg.Type=CpuDataType::FunAddr; 
  Arg.Value.Adr=Address;
  Arg.ScopeDepth=ScopeDepth;
  return Arg;
}

//Set global flag for litteral replacement variables
void Binary::SetLitValueReplacementsGlobal(bool Glob){
  _GlobReplLitValues=Glob;
}

//Init address of current module set?
bool Binary::HasInitAddress(){
  return _HasInitAdr;
}

//Set current init address of current module 
void Binary::SetInitAddress(CpuAdr Address,const String& Module){
  _HasInitAdr=true;
  _InitAddress=Address;
  DebugMessage(DebugLevel::CmpInit,"Opened init routine for module "+Module+" at address "+HEXFORMAT(Address));
}

//Clear current init address of current module 
void Binary::ClearInitAddress(){
  _HasInitAdr=false;
  _InitAddress=0;
  DebugMessage(DebugLevel::CmpInit,"Init address was reset");
}

//Get current init address of current module 
CpuAdr Binary::GetInitAddress(){
  return _InitAddress;
}

//Append init address to list
void Binary::AppendInitRoutine(CpuAdr Address,const String& Module,const String& Name,int ScopeDepth){
  _InitRoutines.Add((InitRoutine){Module,Name,Address,ScopeDepth});
  DebugMessage(DebugLevel::CmpInit,"Init routine list append: module="+Module+" name="+Name+" address="+(Address!=0?HEXFORMAT(Address):"unresolved")+", scopedepth="+ToString(ScopeDepth));
  if(Address!=0){ StoreFunctionAddress(Module+"_"+Name,Module+"."+Name+"()",ScopeDepth,Address); }
}

//Generate super init routine
bool Binary::CallInitRoutines(){
  
  //Variables
  int i;

  //Generate calls to all initialization routines
  for(i=0;i<_InitRoutines.Length();i++){
    if(!AsmWriteCode(CpuInstCode::CALL,SubroutineAsmArg(_InitRoutines[i].Address,_InitRoutines[i].Module,_InitRoutines[i].Name,_InitRoutines[i].ScopeDepth))){ return false; }
    DebugMessage(DebugLevel::CmpInit,"Added call to init routine "+_InitRoutines[i].Name+" on address "+HEXFORMAT(_InitRoutines[i].Address)+" (scopedepth="+ToString(_InitRoutines[i].ScopeDepth)+")");
  }

  //Return code
  return true;

}

//Set super init address
void Binary::SetSuperInitAddress(CpuAdr Address,const String& Module,const String& Name){
  _SuperInitAdr=Address;
  DebugMessage(DebugLevel::CmpInit,"Solved super init routine address to "+HEXFORMAT(Address));
  StoreFunctionAddress(Module+"_"+Name,Module+"."+Name+"()",0,Address);
}

//Get super init address
CpuAdr Binary::GetSuperInitAddress(){
  return _SuperInitAdr;
}

//Modify code buffer and set an address
void Binary::CodeBufferModify(CpuAdr At,CpuAdr Value){
  DebugMessage(DebugLevel::CmpBinary,"Modify code buffer at "+HEXFORMAT(At)+" to write address "+HEXFORMAT(Value))
  const char *p=reinterpret_cast<const char *>(&Value);
  for(int j=0;j<(int)sizeof(CpuAdr);j++){ _CodeBuffer[At+j]=p[j]; }
}

//Modify code buffer and set an address
void Binary::CodeBufferModify(CpuAdr At,CpuAgx Value){
  DebugMessage(DebugLevel::CmpBinary,"Modify code buffer at "+HEXFORMAT(At)+" to write agx "+HEXFORMAT(Value))
  const char *p=reinterpret_cast<const char *>(&Value);
  for(int j=0;j<(int)sizeof(CpuAgx);j++){ _CodeBuffer[At+j]=p[j]; }
}

//Enable/disable assembler file generation
void Binary::EnableAssemblerFile(bool Enable){
  _AsmEnabled=Enable;
}

//Open assembler file
bool Binary::OpenAssembler(const String& FileName){
  if(!_AsmEnabled){ return true; }
  if(!_Stl->FileSystem.GetHandler(_AsmHnd)){ SysMessage(237).Print(FileName); return false; }
  if(!_Stl->FileSystem.OpenForWrite(_AsmHnd,FileName)){
    SysMessage(61).Print(FileName,_Stl->LastError());
    return false;
  }
  return true;
}

//Close assembler file
bool Binary::CloseAssembler(){
  if(!_AsmEnabled){ return true; }
  if(!_Stl->FileSystem.CloseFile(_AsmHnd)){ SysMessage(238).Print(_Stl->FileSystem.Hnd2File(_AsmHnd),_Stl->LastError()); return false; }
  if(!_Stl->FileSystem.FreeHandler(_AsmHnd)){ SysMessage(239).Print(_Stl->FileSystem.Hnd2File(_AsmHnd),_Stl->LastError()); return false; }
  return true;
}

//Set Compile tolibrary flag
void Binary::SetCompileToLibrary(bool CompileToLibrary){
  _CompileToLibrary=CompileToLibrary;
}

//Set library version
void Binary::SetLibraryVersion(CpuShr LibMajorVers,CpuShr LibMinorVers,CpuShr LibRevisionNr){
  _LibMajorVers=LibMajorVers;
  _LibMinorVers=LibMinorVers;
  _LibRevisionNr=LibRevisionNr;
}

//Set memory configuration mem unit size
void Binary::SetMemoryConfigMemUnitSize(CpuWrd MemUnitSize){
  _MemUnitSize=MemUnitSize;
}

//Set memory configuration mem units
void Binary::SetMemoryConfigMemUnits(CpuWrd MemUnits){
  _MemUnits=MemUnits;
}

//Set memory configuration chunk mem units
void Binary::SetMemoryConfigChunkMemUnits(CpuWrd ChunkMemUnits){
  _ChunkMemUnits=ChunkMemUnits;
}

//Set memory configuration blockmax
void Binary::SetMemoryConfigBlockMax(CpuMbl BlockMax){
  _BlockMax=BlockMax;
}

//Write binary error
void Binary::_WriteBinaryError(int Hnd,const char *FileMark,const String& Index){
  SysMessage(121).Print(_Stl->FileSystem.Hnd2File(Hnd),_Stl->LastError(),FileMark,Index,ToString(_Stl->FileSystem.GetPrevPos(Hnd)));
}

//Write binary file
bool Binary::_WriteBinary(const String& FileName,bool Library,bool DebugSymbols){
  
  //Variables
  int i;
  int Hnd;
  CpuWrd BlockLength;
  CpuInt ObjIdLength;
  BinaryHeader Hdr;

  //Debug message
  DebugMessage(DebugLevel::CmpBinary,"Save binary file "+FileName);

  //Get handler
  if(!_Stl->FileSystem.GetHandler(Hnd)){ SysMessage(237).Print(FileName); return false; }

  //Open library file
  if(!_Stl->FileSystem.OpenForWrite(Hnd,FileName)){ SysMessage(230).Print(FileName,_Stl->LastError()); return false; }

  //Fill binary header
  memset((void *)&Hdr,0,sizeof(BinaryHeader));
  if(Library){ 
    MemCpy(Hdr.FileMark,FILEMARKLIBR,strlen(FILEMARKLIBR));
  }
  else{
    MemCpy(Hdr.FileMark,FILEMARKEXEC,strlen(FILEMARKEXEC));
  }
  Hdr.BinFormat=BINARY_FORMAT;
  Hdr.Arquitecture=GetArchitecture();
  strncpy(Hdr.SysVersion,VERSION_NUMBER,VERSION_MAXLEN);
  strncpy(Hdr.SysBuildDate,BuildDate().CharPnt(),10);
  strncpy(Hdr.SysBuildTime,BuildTime().CharPnt(),8);
  Hdr.IsBinLibrary=(Library?true:false);
  Hdr.DebugSymbols=DebugSymbols;
  Hdr.GlobBufferNr=_GlobBuffer.Length();
  Hdr.BlockNr=_Block.Length();
  Hdr.ArrFixDefNr=_ArrFixDef.Length();
  Hdr.ArrDynDefNr=_ArrDynDef.Length();
  Hdr.CodeBufferNr=_CodeBuffer.Length();
  Hdr.DlCallNr=_DlCall.Length();
  Hdr.MemUnitSize=(Library?0:_MemUnitSize);
  Hdr.MemUnits=(Library?0:_MemUnits);
  Hdr.ChunkMemUnits=(Library?0:_ChunkMemUnits);
  Hdr.BlockMax=(Library?0:_BlockMax);
  Hdr.LibMajorVers=_LibMajorVers;
  Hdr.LibMinorVers=_LibMinorVers;
  Hdr.LibRevisionNr=_LibRevisionNr;
  Hdr.DependencyNr=(Library?_Depen.Length():0);
  Hdr.UndefRefNr=(Library?_OUndRef.Length():0);
  Hdr.RelocTableNr=(Library?_RelocTable.Length():0);
  Hdr.LnkSymDimNr=(Library?_OLnkSymTables.Dim.Length():0);
  Hdr.LnkSymTypNr=(Library?_OLnkSymTables.Typ.Length():0);
  Hdr.LnkSymVarNr=(Library?_OLnkSymTables.Var.Length():0);
  Hdr.LnkSymFldNr=(Library?_OLnkSymTables.Fld.Length():0);
  Hdr.LnkSymFunNr=(Library?_OLnkSymTables.Fun.Length():0);
  Hdr.LnkSymParNr=(Library?_OLnkSymTables.Par.Length():0);
  Hdr.DbgSymModNr=(DebugSymbols?_ODbgSymTables.Mod.Length():0);
  Hdr.DbgSymTypNr=(DebugSymbols?_ODbgSymTables.Typ.Length():0);
  Hdr.DbgSymVarNr=(DebugSymbols?_ODbgSymTables.Var.Length():0);
  Hdr.DbgSymFldNr=(DebugSymbols?_ODbgSymTables.Fld.Length():0);
  Hdr.DbgSymFunNr=(DebugSymbols?_ODbgSymTables.Fun.Length():0);
  Hdr.DbgSymParNr=(DebugSymbols?_ODbgSymTables.Par.Length():0);
  Hdr.DbgSymLinNr=(DebugSymbols?_ODbgSymTables.Lin.Length():0);
  Hdr.SuperInitAdr=(Library?_SuperInitAdr:0);
  
  //Print program header
  DebugMessage(DebugLevel::CmpBinary,"Written binary header");
  DebugMessage(DebugLevel::CmpBinary,"FileMark     : "+String(Buffer(Hdr.FileMark,sizeof(Hdr.FileMark))));
  DebugMessage(DebugLevel::CmpBinary,"BinFormat    : "+ToString(Hdr.BinFormat));
  DebugMessage(DebugLevel::CmpBinary,"Arquitecture : "+ToString(Hdr.Arquitecture));
  DebugMessage(DebugLevel::CmpBinary,"SysVersion   : "+String(Hdr.SysVersion));
  DebugMessage(DebugLevel::CmpBinary,"SysBuildDate : "+String(Hdr.SysBuildDate));
  DebugMessage(DebugLevel::CmpBinary,"SysBuildTime : "+String(Hdr.SysBuildTime));
  DebugMessage(DebugLevel::CmpBinary,"IsBinLibrary : "+ToString(Hdr.IsBinLibrary));
  DebugMessage(DebugLevel::CmpBinary,"DebugSymbols : "+ToString(Hdr.DebugSymbols));
  DebugMessage(DebugLevel::CmpBinary,"GlobBufferNr : "+ToString(Hdr.GlobBufferNr));
  DebugMessage(DebugLevel::CmpBinary,"BlockNr      : "+ToString(Hdr.BlockNr));
  DebugMessage(DebugLevel::CmpBinary,"ArrFixDefNr  : "+ToString(Hdr.ArrFixDefNr));
  DebugMessage(DebugLevel::CmpBinary,"ArrDynDefNr  : "+ToString(Hdr.ArrDynDefNr));
  DebugMessage(DebugLevel::CmpBinary,"CodeBufferNr : "+ToString(Hdr.CodeBufferNr));
  DebugMessage(DebugLevel::CmpBinary,"DlCallNr     : "+ToString(Hdr.DlCallNr));
  DebugMessage(DebugLevel::CmpBinary,"MemUnitSize  : "+ToString(Hdr.MemUnitSize));
  DebugMessage(DebugLevel::CmpBinary,"MemUnits     : "+ToString(Hdr.MemUnits));
  DebugMessage(DebugLevel::CmpBinary,"ChunkMemUnits: "+ToString(Hdr.ChunkMemUnits));
  DebugMessage(DebugLevel::CmpBinary,"BlockMax     : "+ToString(Hdr.BlockMax));
  DebugMessage(DebugLevel::CmpBinary,"LibMajorVers : "+ToString(Hdr.LibMajorVers));
  DebugMessage(DebugLevel::CmpBinary,"LibMinorVers : "+ToString(Hdr.LibMinorVers));
  DebugMessage(DebugLevel::CmpBinary,"LibRevisionNr: "+ToString(Hdr.LibRevisionNr));
  DebugMessage(DebugLevel::CmpBinary,"DependencyNr : "+ToString(Hdr.DependencyNr));
  DebugMessage(DebugLevel::CmpBinary,"UndefRefNr   : "+ToString(Hdr.UndefRefNr));
  DebugMessage(DebugLevel::CmpBinary,"RelocTableNr : "+ToString(Hdr.RelocTableNr));
  DebugMessage(DebugLevel::CmpBinary,"LnkSymDimNr  : "+ToString(Hdr.LnkSymDimNr));
  DebugMessage(DebugLevel::CmpBinary,"LnkSymTypNr  : "+ToString(Hdr.LnkSymTypNr));
  DebugMessage(DebugLevel::CmpBinary,"LnkSymVarNr  : "+ToString(Hdr.LnkSymVarNr));
  DebugMessage(DebugLevel::CmpBinary,"LnkSymFldNr  : "+ToString(Hdr.LnkSymFldNr));
  DebugMessage(DebugLevel::CmpBinary,"LnkSymFunNr  : "+ToString(Hdr.LnkSymFunNr));
  DebugMessage(DebugLevel::CmpBinary,"LnkSymParNr  : "+ToString(Hdr.LnkSymParNr));
  DebugMessage(DebugLevel::CmpBinary,"DbgSymModNr  : "+ToString(Hdr.DbgSymModNr));
  DebugMessage(DebugLevel::CmpBinary,"DbgSymTypNr  : "+ToString(Hdr.DbgSymTypNr));
  DebugMessage(DebugLevel::CmpBinary,"DbgSymVarNr  : "+ToString(Hdr.DbgSymVarNr));
  DebugMessage(DebugLevel::CmpBinary,"DbgSymFldNr  : "+ToString(Hdr.DbgSymFldNr));
  DebugMessage(DebugLevel::CmpBinary,"DbgSymFunNr  : "+ToString(Hdr.DbgSymFunNr));
  DebugMessage(DebugLevel::CmpBinary,"DbgSymParNr  : "+ToString(Hdr.DbgSymParNr));
  DebugMessage(DebugLevel::CmpBinary,"DbgSymLinNr  : "+ToString(Hdr.DbgSymLinNr));
  DebugMessage(DebugLevel::CmpBinary,"SuperInitAdr : "+HEXFORMAT(Hdr.SuperInitAdr));
  
  //Write objects to file
  if(!_Stl->FileSystem.Write(Hnd,Buffer((char *)&Hdr,sizeof(Hdr)))){ _WriteBinaryError(Hnd,"HEAD","0"); return false; }
  if(!_Stl->FileSystem.Write(Hnd,Buffer(FILEMARKGLOB))){ _WriteBinaryError(Hnd,FILEMARKGLOB,INDEXHEAD); return false; } 
  if(!_Stl->FileSystem.Write(Hnd,_GlobBuffer)){ _WriteBinaryError(Hnd,FILEMARKGLOB,"0"); return false; } 
  if(!_Stl->FileSystem.Write(Hnd,Buffer(FILEMARKCODE))){ _WriteBinaryError(Hnd,FILEMARKCODE,INDEXHEAD); return false; } 
  if(!_Stl->FileSystem.Write(Hnd,_CodeBuffer)){ _WriteBinaryError(Hnd,FILEMARKCODE,"0"); return false; } 
  if(!_Stl->FileSystem.Write(Hnd,Buffer(FILEMARKFARR))){ _WriteBinaryError(Hnd,FILEMARKFARR,INDEXHEAD); return false; } 
  if(_ArrFixDef.Length()!=0){
    if(!_Stl->FileSystem.Write(Hnd,Buffer((char *)&_ArrFixDef[0],sizeof(ArrayFixDef)*_ArrFixDef.Length()))){ _WriteBinaryError(Hnd,FILEMARKFARR,"0"); return false; } 
  }
  if(!_Stl->FileSystem.Write(Hnd,Buffer(FILEMARKDARR))){ _WriteBinaryError(Hnd,FILEMARKDARR,INDEXHEAD); return false; } 
  if(_ArrDynDef.Length()!=0){
    if(!_Stl->FileSystem.Write(Hnd,Buffer((char *)&_ArrDynDef[0],sizeof(ArrayDynDef)*_ArrDynDef.Length()))){ _WriteBinaryError(Hnd,FILEMARKDARR,"0"); return false; } 
  }
  if(!_Stl->FileSystem.Write(Hnd,Buffer(FILEMARKBLCK))){ _WriteBinaryError(Hnd,FILEMARKBLCK,INDEXHEAD); return false; } 
  for(i=0;i<_Block.Length();i++){ 
    BlockLength=_Block[i].Buff.Length();
    if(!_Stl->FileSystem.Write(Hnd,Buffer((char *)&_Block[i].ArrIndex,sizeof(_Block[i].ArrIndex)))){ _WriteBinaryError(Hnd,FILEMARKBLCK,ToString(i)+".1"); return false; } 
    if(!_Stl->FileSystem.Write(Hnd,Buffer((char *)&BlockLength,sizeof(BlockLength)))){ _WriteBinaryError(Hnd,FILEMARKBLCK,ToString(i)+".2"); return false; } 
    if(!_Stl->FileSystem.Write(Hnd,_Block[i].Buff)){ _WriteBinaryError(Hnd,FILEMARKBLCK,ToString(i)+".3"); return false; } 
  }
  if(!_Stl->FileSystem.Write(Hnd,Buffer(FILEMARKDLCA))){ _WriteBinaryError(Hnd,FILEMARKDLCA,INDEXHEAD); return false; } 
  for(i=0;i<_DlCall.Length();i++){ 
    if(!_Stl->FileSystem.Write(Hnd,Buffer((char *)&_DlCall[i],sizeof(_DlCall[i])))){ _WriteBinaryError(Hnd,FILEMARKDLCA,ToString(i)); return false; } 
  }
  if(Library){
    if(!_Stl->FileSystem.Write(Hnd,Buffer(FILEMARKDEPN))){ _WriteBinaryError(Hnd,FILEMARKDEPN,INDEXHEAD); return false; } 
    for(i=0;i<_Depen.Length();i++){ 
      if(!_Stl->FileSystem.Write(Hnd,Buffer((char *)&_Depen[i],sizeof(_Depen[i])))){ _WriteBinaryError(Hnd,FILEMARKDEPN,ToString(i)); return false; } 
    }
    if(!_Stl->FileSystem.Write(Hnd,Buffer(FILEMARKUREF))){ _WriteBinaryError(Hnd,FILEMARKUREF,INDEXHEAD); return false; } 
    for(i=0;i<_OUndRef.Length();i++){ 
      ObjIdLength=_OUndRef[i].ObjectName.Length();
      if(!_Stl->FileSystem.Write(Hnd,Buffer((char *)&_OUndRef[i].Module,sizeof(_OUndRef[i].Module)))){ _WriteBinaryError(Hnd,FILEMARKUREF,ToString(i)+".1"); return false; } 
      if(!_Stl->FileSystem.Write(Hnd,Buffer((char *)&_OUndRef[i].Kind,sizeof(_OUndRef[i].Kind)))){ _WriteBinaryError(Hnd,FILEMARKUREF,ToString(i)+".2"); return false; } 
      if(!_Stl->FileSystem.Write(Hnd,Buffer((char *)&_OUndRef[i].CodeAdr,sizeof(_OUndRef[i].CodeAdr)))){ _WriteBinaryError(Hnd,FILEMARKUREF,ToString(i)+".3"); return false; } 
      if(!_Stl->FileSystem.Write(Hnd,Buffer((char *)&ObjIdLength,sizeof(ObjIdLength)))){ _WriteBinaryError(Hnd,FILEMARKUREF,ToString(i)+".4"); return false; } 
      if(!_Stl->FileSystem.Write(Hnd,_OUndRef[i].ObjectName.ToBuffer())){ _WriteBinaryError(Hnd,FILEMARKUREF,ToString(i)+".5"); return false; } 
    }
    if(!_Stl->FileSystem.Write(Hnd,Buffer(FILEMARKRELO))){ _WriteBinaryError(Hnd,FILEMARKRELO,INDEXHEAD); return false; } 
    for(i=0;i<_RelocTable.Length();i++){ 
      if(!_Stl->FileSystem.Write(Hnd,Buffer((char *)&_RelocTable[i],sizeof(_RelocTable[i])))){ _WriteBinaryError(Hnd,FILEMARKRELO,ToString(i)); return false; } 
      DebugMessage(DebugLevel::CmpRelocation,"Relocation saved: Type="+RelocationTypeText(_RelocTable[i].Type)+", LocAdr="+HEXFORMAT(_RelocTable[i].LocAdr)+", LocBlk="+HEXFORMAT(_RelocTable[i].LocBlk)+", Module="+String((char *)_RelocTable[i].Module)+", ObjName="+String((char *)_RelocTable[i].ObjName)+", CopyCount="+ToString(_RelocTable[i].CopyCount));
    }
    if(!_Stl->FileSystem.Write(Hnd,Buffer(FILEMARKSDIM))){ _WriteBinaryError(Hnd,FILEMARKSDIM,INDEXHEAD); return false; } 
    for(i=0;i<_OLnkSymTables.Dim.Length();i++){ if(!_Stl->FileSystem.Write(Hnd,Buffer((char *)&_OLnkSymTables.Dim[i],sizeof(_OLnkSymTables.Dim[i])))){ _WriteBinaryError(Hnd,FILEMARKSDIM,ToString(i)); return false; } } 
    if(!_Stl->FileSystem.Write(Hnd,Buffer(FILEMARKSTYP))){ _WriteBinaryError(Hnd,FILEMARKSTYP,INDEXHEAD); return false; } 
    for(i=0;i<_OLnkSymTables.Typ.Length();i++){ if(!_Stl->FileSystem.Write(Hnd,Buffer((char *)&_OLnkSymTables.Typ[i],sizeof(_OLnkSymTables.Typ[i])))){ _WriteBinaryError(Hnd,FILEMARKSTYP,ToString(i)); return false; } } 
    if(!_Stl->FileSystem.Write(Hnd,Buffer(FILEMARKSVAR))){ _WriteBinaryError(Hnd,FILEMARKSVAR,INDEXHEAD); return false; } 
    for(i=0;i<_OLnkSymTables.Var.Length();i++){ if(!_Stl->FileSystem.Write(Hnd,Buffer((char *)&_OLnkSymTables.Var[i],sizeof(_OLnkSymTables.Var[i])))){ _WriteBinaryError(Hnd,FILEMARKSVAR,ToString(i)); return false; } } 
    if(!_Stl->FileSystem.Write(Hnd,Buffer(FILEMARKSFLD))){ _WriteBinaryError(Hnd,FILEMARKSFLD,INDEXHEAD); return false; } 
    for(i=0;i<_OLnkSymTables.Fld.Length();i++){ if(!_Stl->FileSystem.Write(Hnd,Buffer((char *)&_OLnkSymTables.Fld[i],sizeof(_OLnkSymTables.Fld[i])))){ _WriteBinaryError(Hnd,FILEMARKSFLD,ToString(i)); return false; } } 
    if(!_Stl->FileSystem.Write(Hnd,Buffer(FILEMARKSFUN))){ _WriteBinaryError(Hnd,FILEMARKSFUN,INDEXHEAD); return false; } 
    for(i=0;i<_OLnkSymTables.Fun.Length();i++){ if(!_Stl->FileSystem.Write(Hnd,Buffer((char *)&_OLnkSymTables.Fun[i],sizeof(_OLnkSymTables.Fun[i])))){ _WriteBinaryError(Hnd,FILEMARKSFUN,ToString(i)); return false; } } 
    if(!_Stl->FileSystem.Write(Hnd,Buffer(FILEMARKSPAR))){ _WriteBinaryError(Hnd,FILEMARKSPAR,INDEXHEAD); return false; } 
    for(i=0;i<_OLnkSymTables.Par.Length();i++){ if(!_Stl->FileSystem.Write(Hnd,Buffer((char *)&_OLnkSymTables.Par[i],sizeof(_OLnkSymTables.Par[i])))){ _WriteBinaryError(Hnd,FILEMARKSPAR,ToString(i)); return false; } } 
  }
  if(DebugSymbols){
    if(!_Stl->FileSystem.Write(Hnd,Buffer(FILEMARKDMOD))){ _WriteBinaryError(Hnd,FILEMARKDMOD,INDEXHEAD); return false; } 
    for(i=0;i<_ODbgSymTables.Mod.Length();i++){ if(!_Stl->FileSystem.Write(Hnd,Buffer((char *)&_ODbgSymTables.Mod[i],sizeof(_ODbgSymTables.Mod[i])))){ _WriteBinaryError(Hnd,FILEMARKDMOD,ToString(i)); return false; } } 
    if(!_Stl->FileSystem.Write(Hnd,Buffer(FILEMARKDTYP))){ _WriteBinaryError(Hnd,FILEMARKDTYP,INDEXHEAD); return false; } 
    for(i=0;i<_ODbgSymTables.Typ.Length();i++){ if(!_Stl->FileSystem.Write(Hnd,Buffer((char *)&_ODbgSymTables.Typ[i],sizeof(_ODbgSymTables.Typ[i])))){ _WriteBinaryError(Hnd,FILEMARKDTYP,ToString(i)); return false; } } 
    if(!_Stl->FileSystem.Write(Hnd,Buffer(FILEMARKDVAR))){ _WriteBinaryError(Hnd,FILEMARKDVAR,INDEXHEAD); return false; } 
    for(i=0;i<_ODbgSymTables.Var.Length();i++){ if(!_Stl->FileSystem.Write(Hnd,Buffer((char *)&_ODbgSymTables.Var[i],sizeof(_ODbgSymTables.Var[i])))){ _WriteBinaryError(Hnd,FILEMARKDVAR,ToString(i)); return false; } } 
    if(!_Stl->FileSystem.Write(Hnd,Buffer(FILEMARKDFLD))){ _WriteBinaryError(Hnd,FILEMARKDFLD,INDEXHEAD); return false; } 
    for(i=0;i<_ODbgSymTables.Fld.Length();i++){ if(!_Stl->FileSystem.Write(Hnd,Buffer((char *)&_ODbgSymTables.Fld[i],sizeof(_ODbgSymTables.Fld[i])))){ _WriteBinaryError(Hnd,FILEMARKDFLD,ToString(i)); return false; } } 
    if(!_Stl->FileSystem.Write(Hnd,Buffer(FILEMARKDFUN))){ _WriteBinaryError(Hnd,FILEMARKDFUN,INDEXHEAD); return false; } 
    for(i=0;i<_ODbgSymTables.Fun.Length();i++){ if(!_Stl->FileSystem.Write(Hnd,Buffer((char *)&_ODbgSymTables.Fun[i],sizeof(_ODbgSymTables.Fun[i])))){ _WriteBinaryError(Hnd,FILEMARKDFUN,ToString(i)); return false; } } 
    if(!_Stl->FileSystem.Write(Hnd,Buffer(FILEMARKDPAR))){ _WriteBinaryError(Hnd,FILEMARKDPAR,INDEXHEAD); return false; } 
    for(i=0;i<_ODbgSymTables.Par.Length();i++){ if(!_Stl->FileSystem.Write(Hnd,Buffer((char *)&_ODbgSymTables.Par[i],sizeof(_ODbgSymTables.Par[i])))){ _WriteBinaryError(Hnd,FILEMARKDPAR,ToString(i)); return false; } } 
    if(!_Stl->FileSystem.Write(Hnd,Buffer(FILEMARKDLIN))){ _WriteBinaryError(Hnd,FILEMARKDLIN,INDEXHEAD); return false; } 
    for(i=0;i<_ODbgSymTables.Lin.Length();i++){ if(!_Stl->FileSystem.Write(Hnd,Buffer((char *)&_ODbgSymTables.Lin[i],sizeof(_ODbgSymTables.Lin[i])))){ _WriteBinaryError(Hnd,FILEMARKDLIN,ToString(i)); return false; } } 
  }

  //Close file
  if(!_Stl->FileSystem.CloseFile(Hnd)){ SysMessage(238).Print(FileName,_Stl->LastError()); return false; }

  //Free handler
  if(!_Stl->FileSystem.FreeHandler(Hnd)){ SysMessage(239).Print(FileName,_Stl->LastError()); return false; }

  //Return code
  return true;

}

//Read binary error
void Binary::_ReadBinaryError(int Hnd,const char *FileMark,const String& Index,const SourceInfo& SrcInfo){
  SrcInfo.Msg(123).Print(_Stl->FileSystem.Hnd2File(Hnd),_Stl->LastError(),FileMark,Index,ToString(_Stl->FileSystem.GetPrevPos(Hnd)));
}

//Read binary file
bool Binary::_ReadBinary(const String& FileName,bool Library,BinaryHeader& Hdr,Buffer& AuxGlobBuffer,Buffer& AuxCodeBuffer,Array<BlockDef>& AuxBlock,
                        Array<ArrayFixDef>& AuxArrFixDef,Array<ArrayDynDef>& AuxArrDynDef,Array<DlCallDef>& AuxDlCall,Array<Dependency>& AuxDepen,Array<UndefinedRefer>& AuxUndRef,
                        Array<RelocItem>& AuxRelocTable,LnkSymTables& LnkSym,DbgSymTables& DbgSym,const SourceInfo& SrcInfo){

  //Variables
  int i;
  int Hnd;
  Buffer Buff;
  CpuWrd Length;
  CpuWrd ArrIndex;
  BlockDef Blk;
  ArrayFixDef ArrFixDef;
  ArrayDynDef ArrDynDef;
  DlCallDef DlCall;
  Dependency DepItem;
  String URefModule;
  CpuChr URefKind;
  CpuAdr URefCodeAdr;
  CpuInt URefObjNameLength;
  String URefObjName;
  UndefinedRefer URefItem;
  RelocItem RelItem;
  LnkSymTables::LnkSymDimension SymDimItem;
  LnkSymTables::LnkSymType SymTypItem;
  LnkSymTables::LnkSymVariable SymVarItem;
  LnkSymTables::LnkSymField SymFldItem;
  LnkSymTables::LnkSymFunction SymFunItem;
  LnkSymTables::LnkSymParameter SymParItem;
  DbgSymModule DbgModItem;
  DbgSymType DbgTypItem;
  DbgSymVariable DbgVarItem;
  DbgSymField DbgFldItem;
  DbgSymFunction DbgFunItem;
  DbgSymParameter DbgParItem;
  DbgSymLine DbgLinItem;

  //Debug message
  DebugMessage(DebugLevel::CmpBinary,"Load binary file "+FileName);

  //Get handler
  if(!_Stl->FileSystem.GetHandler(Hnd)){ SrcInfo.Msg(237).Print(FileName); return false; }

  //Open file
  if(!_Stl->FileSystem.OpenForRead(Hnd,FileName)){ SrcInfo.Msg(230).Print(FileName,_Stl->LastError()); return false; }

  //Read header
  if(!_Stl->FileSystem.Read(Hnd,Buff,sizeof(Hdr))){ _ReadBinaryError(Hnd,"HEAD","0",SrcInfo); return false; }
  Hdr=*reinterpret_cast<const BinaryHeader *>(Buff.BuffPnt());
  
  //Print program header
  DebugMessage(DebugLevel::CmpBinary,"Loaded binary header");
  DebugMessage(DebugLevel::CmpBinary,"FileMark     : "+String(Buffer(Hdr.FileMark,sizeof(Hdr.FileMark))));
  DebugMessage(DebugLevel::CmpBinary,"BinFormat    : "+ToString(Hdr.BinFormat));
  DebugMessage(DebugLevel::CmpBinary,"Arquitecture : "+ToString(Hdr.Arquitecture));
  DebugMessage(DebugLevel::CmpBinary,"SysVersion   : "+String(Hdr.SysVersion));
  DebugMessage(DebugLevel::CmpBinary,"SysBuildDate : "+String(Hdr.SysBuildDate));
  DebugMessage(DebugLevel::CmpBinary,"SysBuildTime : "+String(Hdr.SysBuildTime));
  DebugMessage(DebugLevel::CmpBinary,"IsBinLibrary : "+ToString(Hdr.IsBinLibrary));
  DebugMessage(DebugLevel::CmpBinary,"DebugSymbols : "+ToString(Hdr.DebugSymbols));
  DebugMessage(DebugLevel::CmpBinary,"GlobBufferNr : "+ToString(Hdr.GlobBufferNr));
  DebugMessage(DebugLevel::CmpBinary,"BlockNr      : "+ToString(Hdr.BlockNr));
  DebugMessage(DebugLevel::CmpBinary,"ArrFixDefNr  : "+ToString(Hdr.ArrFixDefNr));
  DebugMessage(DebugLevel::CmpBinary,"ArrDynDefNr  : "+ToString(Hdr.ArrDynDefNr));
  DebugMessage(DebugLevel::CmpBinary,"CodeBufferNr : "+ToString(Hdr.CodeBufferNr));
  DebugMessage(DebugLevel::CmpBinary,"DlCallNr     : "+ToString(Hdr.DlCallNr));
  DebugMessage(DebugLevel::CmpBinary,"MemUnitSize  : "+ToString(Hdr.MemUnitSize));
  DebugMessage(DebugLevel::CmpBinary,"MemUnits     : "+ToString(Hdr.MemUnits));
  DebugMessage(DebugLevel::CmpBinary,"ChunkMemUnits: "+ToString(Hdr.ChunkMemUnits));
  DebugMessage(DebugLevel::CmpBinary,"BlockMax     : "+ToString(Hdr.BlockMax));
  DebugMessage(DebugLevel::CmpBinary,"LibMajorVers : "+ToString(Hdr.LibMajorVers));
  DebugMessage(DebugLevel::CmpBinary,"LibMinorVers : "+ToString(Hdr.LibMinorVers));
  DebugMessage(DebugLevel::CmpBinary,"LibRevisionNr: "+ToString(Hdr.LibRevisionNr));
  DebugMessage(DebugLevel::CmpBinary,"DependencyNr : "+ToString(Hdr.DependencyNr));
  DebugMessage(DebugLevel::CmpBinary,"UndefRefNr   : "+ToString(Hdr.UndefRefNr));
  DebugMessage(DebugLevel::CmpBinary,"RelocTableNr : "+ToString(Hdr.RelocTableNr));
  DebugMessage(DebugLevel::CmpBinary,"LnkSymDimNr  : "+ToString(Hdr.LnkSymDimNr));
  DebugMessage(DebugLevel::CmpBinary,"LnkSymTypNr  : "+ToString(Hdr.LnkSymTypNr));
  DebugMessage(DebugLevel::CmpBinary,"LnkSymVarNr  : "+ToString(Hdr.LnkSymVarNr));
  DebugMessage(DebugLevel::CmpBinary,"LnkSymFldNr  : "+ToString(Hdr.LnkSymFldNr));
  DebugMessage(DebugLevel::CmpBinary,"LnkSymFunNr  : "+ToString(Hdr.LnkSymFunNr));
  DebugMessage(DebugLevel::CmpBinary,"LnkSymParNr  : "+ToString(Hdr.LnkSymParNr));
  DebugMessage(DebugLevel::CmpBinary,"DbgSymModNr  : "+ToString(Hdr.DbgSymModNr));
  DebugMessage(DebugLevel::CmpBinary,"DbgSymTypNr  : "+ToString(Hdr.DbgSymTypNr));
  DebugMessage(DebugLevel::CmpBinary,"DbgSymVarNr  : "+ToString(Hdr.DbgSymVarNr));
  DebugMessage(DebugLevel::CmpBinary,"DbgSymFldNr  : "+ToString(Hdr.DbgSymFldNr));
  DebugMessage(DebugLevel::CmpBinary,"DbgSymFunNr  : "+ToString(Hdr.DbgSymFunNr));
  DebugMessage(DebugLevel::CmpBinary,"DbgSymParNr  : "+ToString(Hdr.DbgSymParNr));
  DebugMessage(DebugLevel::CmpBinary,"DbgSymLinNr  : "+ToString(Hdr.DbgSymLinNr));
  DebugMessage(DebugLevel::CmpBinary,"SuperInitAdr : "+HEXFORMAT(Hdr.SuperInitAdr));
 
  //Check architecture matches
  if(Hdr.Arquitecture!=GetArchitecture()){
    SrcInfo.Msg(133).Print(ToString(Hdr.Arquitecture),FileName,ToString(GetArchitecture()));
    return false;
  }

  //Check binary format
  if(Hdr.BinFormat!=BINARY_FORMAT){
    SrcInfo.Msg(436).Print(FileName,ToString(Hdr.BinFormat));
    return false;
  }
 
  //Read library data
  if(!_Stl->FileSystem.Read(Hnd,Buff,strlen(FILEMARKGLOB))){ _ReadBinaryError(Hnd,FILEMARKGLOB,INDEXHEAD,SrcInfo); return false; }
  if(!_Stl->FileSystem.Read(Hnd,AuxGlobBuffer,Hdr.GlobBufferNr)){ _ReadBinaryError(Hnd,FILEMARKGLOB,"0",SrcInfo); return false; }
  if(!_Stl->FileSystem.Read(Hnd,Buff,strlen(FILEMARKCODE))){ _ReadBinaryError(Hnd,FILEMARKCODE,INDEXHEAD,SrcInfo); return false; }
  if(!_Stl->FileSystem.Read(Hnd,AuxCodeBuffer,Hdr.CodeBufferNr)){ _ReadBinaryError(Hnd,FILEMARKCODE,"0",SrcInfo); return false; }
  if(!_Stl->FileSystem.Read(Hnd,Buff,strlen(FILEMARKFARR))){ _ReadBinaryError(Hnd,FILEMARKFARR,INDEXHEAD,SrcInfo); return false; }
  for(i=0;i<Hdr.ArrFixDefNr;i++){ 
    if(!_Stl->FileSystem.Read(Hnd,Buff,sizeof(ArrayFixDef))){ _ReadBinaryError(Hnd,FILEMARKFARR,ToString(i),SrcInfo); return false; }
    ArrFixDef=*reinterpret_cast<const ArrayFixDef *>(Buff.BuffPnt());
    AuxArrFixDef.Add(ArrFixDef);
  }
  if(!_Stl->FileSystem.Read(Hnd,Buff,strlen(FILEMARKDARR))){ _ReadBinaryError(Hnd,FILEMARKDARR,INDEXHEAD,SrcInfo); return false; }
  for(i=0;i<Hdr.ArrDynDefNr;i++){ 
    if(!_Stl->FileSystem.Read(Hnd,Buff,sizeof(ArrayDynDef))){ _ReadBinaryError(Hnd,FILEMARKDARR,ToString(i),SrcInfo); return false; }
    ArrDynDef=*reinterpret_cast<const ArrayDynDef *>(Buff.BuffPnt());
    AuxArrDynDef.Add(ArrDynDef);
  }
  if(!_Stl->FileSystem.Read(Hnd,Buff,strlen(FILEMARKBLCK))){ _ReadBinaryError(Hnd,FILEMARKBLCK,INDEXHEAD,SrcInfo); return false; }
  for(i=0;i<Hdr.BlockNr;i++){ 
     if(!_Stl->FileSystem.Read(Hnd,Buff,sizeof(ArrIndex))){ _ReadBinaryError(Hnd,FILEMARKBLCK,ToString(i)+".1",SrcInfo); return false; }
    ArrIndex=*reinterpret_cast<const CpuWrd *>(Buff.BuffPnt());
    if(!_Stl->FileSystem.Read(Hnd,Buff,sizeof(Length))){ _ReadBinaryError(Hnd,FILEMARKBLCK,ToString(i)+ ".2",SrcInfo); return false; }
    Length=*reinterpret_cast<const CpuWrd *>(Buff.BuffPnt());
    if(!_Stl->FileSystem.Read(Hnd,Buff,Length)){ _ReadBinaryError(Hnd,FILEMARKBLCK,ToString(i)+".3",SrcInfo); return false; }
    Blk.ArrIndex=ArrIndex;
    Blk.Buff=Buff;
    AuxBlock.Add(Blk);
  }
   if(!_Stl->FileSystem.Read(Hnd,Buff,strlen(FILEMARKDLCA))){ _ReadBinaryError(Hnd,FILEMARKDLCA,INDEXHEAD,SrcInfo); return false; }
  for(i=0;i<Hdr.DlCallNr;i++){ 
    if(!_Stl->FileSystem.Read(Hnd,Buff,sizeof(DlCall))){ _ReadBinaryError(Hnd,FILEMARKDLCA,ToString(i),SrcInfo); return false; }
    DlCall=*reinterpret_cast<const DlCallDef *>(Buff.BuffPnt());
    AuxDlCall.Add(DlCall);
  }
  if(Library){
     if(!_Stl->FileSystem.Read(Hnd,Buff,strlen(FILEMARKDEPN))){ _ReadBinaryError(Hnd,FILEMARKDEPN,INDEXHEAD,SrcInfo); return false; }
    for(i=0;i<Hdr.DependencyNr;i++){ 
      if(!_Stl->FileSystem.Read(Hnd,Buff,sizeof(DepItem))){ _ReadBinaryError(Hnd,FILEMARKDEPN,ToString(i),SrcInfo); return false; }
      DepItem=*reinterpret_cast<const Dependency *>(Buff.BuffPnt());
      AuxDepen.Add(DepItem);
     }
    if(!_Stl->FileSystem.Read(Hnd,Buff,strlen(FILEMARKUREF))){ _ReadBinaryError(Hnd,FILEMARKUREF,INDEXHEAD,SrcInfo); return false; }
    for(i=0;i<Hdr.UndefRefNr;i++){ 
      if(!_Stl->FileSystem.Read(Hnd,Buff,sizeof(UndefinedRefer::Module))){ _ReadBinaryError(Hnd,FILEMARKUREF,ToString(i)+".1",SrcInfo); return false; }
      URefModule=String(Buff.BuffPnt());
      if(!_Stl->FileSystem.Read(Hnd,Buff,sizeof(URefKind))){ _ReadBinaryError(Hnd,FILEMARKUREF,ToString(i)+".2",SrcInfo); return false; }
      URefKind=*reinterpret_cast<const CpuChr *>(Buff.BuffPnt());
      if(!_Stl->FileSystem.Read(Hnd,Buff,sizeof(URefCodeAdr))){ _ReadBinaryError(Hnd,FILEMARKUREF,ToString(i)+".3",SrcInfo); return false; }
      URefCodeAdr=*reinterpret_cast<const CpuAdr *>(Buff.BuffPnt());
      if(!_Stl->FileSystem.Read(Hnd,Buff,sizeof(URefObjNameLength))){ _ReadBinaryError(Hnd,FILEMARKUREF,ToString(i)+".4",SrcInfo); return false; }
      URefObjNameLength=*reinterpret_cast<const CpuInt *>(Buff.BuffPnt());
      if(!_Stl->FileSystem.Read(Hnd,Buff,URefObjNameLength)){ _ReadBinaryError(Hnd,FILEMARKUREF,ToString(i)+".5",SrcInfo); return false; }
      URefObjName=String(Buff);
      URefModule.Copy((char *)&URefItem.Module[0],sizeof(UndefinedRefer::Module));
      URefItem.Kind=URefKind;
      URefItem.CodeAdr=URefCodeAdr;
      URefItem.ObjectName=URefObjName; 
      AuxUndRef.Add(URefItem);
    }
    if(!_Stl->FileSystem.Read(Hnd,Buff,strlen(FILEMARKRELO))){ _ReadBinaryError(Hnd,FILEMARKRELO,INDEXHEAD,SrcInfo); return false; }
    for(i=0;i<Hdr.RelocTableNr;i++){ 
      if(!_Stl->FileSystem.Read(Hnd,Buff,sizeof(RelItem))){ _ReadBinaryError(Hnd,FILEMARKRELO,ToString(i),SrcInfo); return false; }
      RelItem=*reinterpret_cast<const RelocItem *>(Buff.BuffPnt());
      AuxRelocTable.Add(RelItem);
      DebugMessage(DebugLevel::CmpRelocation,"Relocation loaded: Type="+RelocationTypeText(RelItem.Type)+", LocAdr="+HEXFORMAT(RelItem.LocAdr)+", LocBlk="+HEXFORMAT(RelItem.LocBlk)+", Module="+String((char *)RelItem.Module)+", ObjName="+String((char *)RelItem.ObjName)+", CopyCount="+ToString(RelItem.CopyCount));
    }
    if(!_Stl->FileSystem.Read(Hnd,Buff,strlen(FILEMARKSDIM))){ _ReadBinaryError(Hnd,FILEMARKSDIM,INDEXHEAD,SrcInfo); return false; }
    for(i=0;i<Hdr.LnkSymDimNr;i++){ 
      if(!_Stl->FileSystem.Read(Hnd,Buff,sizeof(LnkSymTables::LnkSymDimension))){ _ReadBinaryError(Hnd,FILEMARKSDIM,ToString(i),SrcInfo); return false; }
      SymDimItem=*reinterpret_cast<const LnkSymTables::LnkSymDimension *>(Buff.BuffPnt());
      LnkSym.Dim .Add(SymDimItem);
      DebugMessage(DebugLevel::CmpLnkSymbol,"Load IDIM["+ToString(LnkSym.Dim.Length()-1)+"]: "+_GetLnkSymDebugStr(LnkSym,LnkSymKind::Dim));
     }
    if(!_Stl->FileSystem.Read(Hnd,Buff,strlen(FILEMARKSTYP))){ _ReadBinaryError(Hnd,FILEMARKSTYP,INDEXHEAD,SrcInfo); return false; }
    for(i=0;i<Hdr.LnkSymTypNr;i++){ 
      if(!_Stl->FileSystem.Read(Hnd,Buff,sizeof(LnkSymTables::LnkSymType))){ _ReadBinaryError(Hnd,FILEMARKSTYP,ToString(i),SrcInfo); return false; }
      SymTypItem=*reinterpret_cast<const LnkSymTables::LnkSymType *>(Buff.BuffPnt());
       LnkSym.Typ.Add(SymTypItem);
      DebugMessage(DebugLevel::CmpLnkSymbol,"Load ITYP["+ToString(LnkSym.Typ.Length()-1)+"]: "+_GetLnkSymDebugStr(LnkSym,LnkSymKind::Typ));
    }
    if(!_Stl->FileSystem.Read(Hnd,Buff,strlen(FILEMARKSVAR))){ _ReadBinaryError(Hnd,FILEMARKSVAR,INDEXHEAD,SrcInfo); return false; }
    for(i=0;i<Hdr.LnkSymVarNr;i++){ 
      if(!_Stl->FileSystem.Read(Hnd,Buff,sizeof(LnkSymTables::LnkSymVariable))){ _ReadBinaryError(Hnd,FILEMARKSVAR,ToString(i),SrcInfo); return false; }
      SymVarItem=*reinterpret_cast<const LnkSymTables::LnkSymVariable *>(Buff.BuffPnt());
      LnkSym.Var.Add(SymVarItem);
      DebugMessage(DebugLevel::CmpLnkSymbol,"Load IVAR["+ToString(LnkSym.Var.Length()-1)+"]: "+_GetLnkSymDebugStr(LnkSym,LnkSymKind::Var));
    }
    if(!_Stl->FileSystem.Read(Hnd,Buff,strlen(FILEMARKSFLD))){ _ReadBinaryError(Hnd,FILEMARKSFLD,INDEXHEAD,SrcInfo); return false; }
    for(i=0;i<Hdr.LnkSymFldNr;i++){ 
      if(!_Stl->FileSystem.Read(Hnd,Buff,sizeof(LnkSymTables::LnkSymField))){ _ReadBinaryError(Hnd,FILEMARKSFLD,ToString(i),SrcInfo); return false; }
      SymFldItem=*reinterpret_cast<const LnkSymTables::LnkSymField *>(Buff.BuffPnt());
       LnkSym.Fld.Add(SymFldItem);
      DebugMessage(DebugLevel::CmpLnkSymbol,"Load IFLD["+ToString(LnkSym.Fld.Length()-1)+"]: "+_GetLnkSymDebugStr(LnkSym,LnkSymKind::Fld));
    }
    if(!_Stl->FileSystem.Read(Hnd,Buff,strlen(FILEMARKSFUN))){ _ReadBinaryError(Hnd,FILEMARKSFUN,INDEXHEAD,SrcInfo); return false; }
    for(i=0;i<Hdr.LnkSymFunNr;i++){ 
      if(!_Stl->FileSystem.Read(Hnd,Buff,sizeof(LnkSymTables::LnkSymFunction))){ _ReadBinaryError(Hnd,FILEMARKSFUN,ToString(i),SrcInfo); return false; }
      SymFunItem=*reinterpret_cast<const LnkSymTables::LnkSymFunction *>(Buff.BuffPnt());
      LnkSym.Fun.Add(SymFunItem);
      DebugMessage(DebugLevel::CmpLnkSymbol,"Load IFUN["+ToString(LnkSym.Fun.Length()-1)+"]: "+_GetLnkSymDebugStr(LnkSym,LnkSymKind::Fun));
    }
    if(!_Stl->FileSystem.Read(Hnd,Buff,strlen(FILEMARKSPAR))){ _ReadBinaryError(Hnd,FILEMARKSPAR,INDEXHEAD,SrcInfo); return false; }
    for(i=0;i<Hdr.LnkSymParNr;i++){ 
      if(!_Stl->FileSystem.Read(Hnd,Buff,sizeof(LnkSymTables::LnkSymParameter))){ _ReadBinaryError(Hnd,FILEMARKSPAR,ToString(i),SrcInfo); return false; }
      SymParItem=*reinterpret_cast<const LnkSymTables::LnkSymParameter *>(Buff.BuffPnt());
      LnkSym.Par.Add(SymParItem);
      DebugMessage(DebugLevel::CmpLnkSymbol,"Load IPAR["+ToString(LnkSym.Par.Length()-1)+"]: "+_GetLnkSymDebugStr(LnkSym,LnkSymKind::Par));
     }
  }
  if(Hdr.DebugSymbols){
    if(!_Stl->FileSystem.Read(Hnd,Buff,strlen(FILEMARKDMOD))){ _ReadBinaryError(Hnd,FILEMARKDMOD,INDEXHEAD,SrcInfo); return false; }
    for(i=0;i<Hdr.DbgSymModNr;i++){ 
      if(!_Stl->FileSystem.Read(Hnd,Buff,sizeof(DbgSymModule))){ _ReadBinaryError(Hnd,FILEMARKDMOD,ToString(i),SrcInfo); return false; }
      DbgModItem=*reinterpret_cast<const DbgSymModule *>(Buff.BuffPnt());
      DbgSym.Mod.Add(DbgModItem);
      DebugMessage(DebugLevel::CmpDbgSymbol,"Load DMOD["+ToString(DbgSym.Mod.Length()-1)+"]: "+System::GetDbgSymDebugStr(DbgSym.Mod[i]));
    }
    if(!_Stl->FileSystem.Read(Hnd,Buff,strlen(FILEMARKDTYP))){ _ReadBinaryError(Hnd,FILEMARKDTYP,INDEXHEAD,SrcInfo); return false; }
    for(i=0;i<Hdr.DbgSymTypNr;i++){ 
      if(!_Stl->FileSystem.Read(Hnd,Buff,sizeof(DbgSymType))){ _ReadBinaryError(Hnd,FILEMARKDTYP,ToString(i),SrcInfo); return false; }
      DbgTypItem=*reinterpret_cast<const DbgSymType *>(Buff.BuffPnt());
      DbgSym.Typ.Add(DbgTypItem);
      DebugMessage(DebugLevel::CmpDbgSymbol,"Load DTYP["+ToString(DbgSym.Typ.Length()-1)+"]: "+System::GetDbgSymDebugStr(DbgSym.Typ[i]));
    }
    if(!_Stl->FileSystem.Read(Hnd,Buff,strlen(FILEMARKDVAR))){ _ReadBinaryError(Hnd,FILEMARKDVAR,INDEXHEAD,SrcInfo); return false; }
    for(i=0;i<Hdr.DbgSymVarNr;i++){ 
      if(!_Stl->FileSystem.Read(Hnd,Buff,sizeof(DbgSymVariable))){ _ReadBinaryError(Hnd,FILEMARKDVAR,ToString(i),SrcInfo); return false; }
      DbgVarItem=*reinterpret_cast<const DbgSymVariable *>(Buff.BuffPnt());
      DbgSym.Var.Add(DbgVarItem);
      DebugMessage(DebugLevel::CmpDbgSymbol,"Load DVAR["+ToString(DbgSym.Var.Length()-1)+"]: "+System::GetDbgSymDebugStr(DbgSym.Var[i]));
    }
    if(!_Stl->FileSystem.Read(Hnd,Buff,strlen(FILEMARKDFLD))){ _ReadBinaryError(Hnd,FILEMARKDFLD,INDEXHEAD,SrcInfo); return false; }
    for(i=0;i<Hdr.DbgSymFldNr;i++){ 
      if(!_Stl->FileSystem.Read(Hnd,Buff,sizeof(DbgSymField))){ _ReadBinaryError(Hnd,FILEMARKDFLD,ToString(i),SrcInfo); return false; }
      DbgFldItem=*reinterpret_cast<const DbgSymField *>(Buff.BuffPnt());
      DbgSym.Fld.Add(DbgFldItem);
      DebugMessage(DebugLevel::CmpDbgSymbol,"Load DFLD["+ToString(DbgSym.Fld.Length()-1)+"]: "+System::GetDbgSymDebugStr(DbgSym.Fld[i]));
    }
    if(!_Stl->FileSystem.Read(Hnd,Buff,strlen(FILEMARKDFUN))){ _ReadBinaryError(Hnd,FILEMARKDFUN,INDEXHEAD,SrcInfo); return false; }
    for(i=0;i<Hdr.DbgSymFunNr;i++){ 
      if(!_Stl->FileSystem.Read(Hnd,Buff,sizeof(DbgSymFunction))){ _ReadBinaryError(Hnd,FILEMARKDFUN,ToString(i),SrcInfo); return false; }
      DbgFunItem=*reinterpret_cast<const DbgSymFunction *>(Buff.BuffPnt());
      DbgSym.Fun.Add(DbgFunItem);
      DebugMessage(DebugLevel::CmpDbgSymbol,"Load DFUN["+ToString(DbgSym.Fun.Length()-1)+"]: "+System::GetDbgSymDebugStr(DbgSym.Fun[i]));
    }
    if(!_Stl->FileSystem.Read(Hnd,Buff,strlen(FILEMARKDPAR))){ _ReadBinaryError(Hnd,FILEMARKDPAR,INDEXHEAD,SrcInfo); return false; }
    for(i=0;i<Hdr.DbgSymParNr;i++){ 
      if(!_Stl->FileSystem.Read(Hnd,Buff,sizeof(DbgSymParameter))){ _ReadBinaryError(Hnd,FILEMARKDPAR,ToString(i),SrcInfo); return false; }
      DbgParItem=*reinterpret_cast<const DbgSymParameter *>(Buff.BuffPnt());
      DbgSym.Par.Add(DbgParItem);
      DebugMessage(DebugLevel::CmpDbgSymbol,"Load DPAR["+ToString(DbgSym.Par.Length()-1)+"]: "+System::GetDbgSymDebugStr(DbgSym.Par[i]));
    }
    if(!_Stl->FileSystem.Read(Hnd,Buff,strlen(FILEMARKDLIN))){ _ReadBinaryError(Hnd,FILEMARKDLIN,INDEXHEAD,SrcInfo); return false; }
    for(i=0;i<Hdr.DbgSymLinNr;i++){ 
      if(!_Stl->FileSystem.Read(Hnd,Buff,sizeof(DbgSymLine))){ _ReadBinaryError(Hnd,FILEMARKDLIN,ToString(i),SrcInfo); return false; }
      DbgLinItem=*reinterpret_cast<const DbgSymLine *>(Buff.BuffPnt());
      DbgSym.Lin.Add(DbgLinItem);
      DebugMessage(DebugLevel::CmpDbgSymbol,"Load DLIN["+ToString(DbgSym.Lin.Length()-1)+"]: "+System::GetDbgSymDebugStr(DbgSym.Lin[i]));
    }
  }

  //Close file
  if(!_Stl->FileSystem.CloseFile(Hnd)){ SrcInfo.Msg(238).Print(FileName,_Stl->LastError()); return false; }

  //Free handler
  if(!_Stl->FileSystem.FreeHandler(Hnd)){ SrcInfo.Msg(239).Print(FileName,_Stl->LastError()); return false; }

  //Return code
  return true;

}

//GenerateLibrary
bool Binary::GenerateLibrary(bool DebugSymbols){
  return _WriteBinary(_BinaryFile,true,DebugSymbols);
}

//GenerateExecutable
bool Binary::GenerateExecutable(bool DebugSymbols){
  return _WriteBinary(_BinaryFile,false,DebugSymbols);
}

//Readlibrary but only importlibrary dependencies
bool Binary::LoadLibraryDependencies(const String& FileName,BinaryHeader& Hdr,Array<Dependency>& Depen,const SourceInfo& SrcInfo){
  
  //Variables
  Buffer AuxGlobBuffer;
  Buffer AuxCodeBuffer;
  Array<BlockDef> AuxBlock;
  Array<ArrayFixDef> AuxArrFixDef;
  Array<ArrayDynDef> AuxArrDynDef;
  Array<DlCallDef> AuxDlCall;
  Array<UndefinedRefer> AuxUndRef;
  Array<RelocItem> AuxRelocTable;
  LnkSymTables AuxLnkSym;
  DbgSymTables AuxDbgSym;

  //Debug message
  DebugMessage(DebugLevel::CmpBinary,"Load dependencies for library file "+FileName);

  //Read binary file
  if(!_ReadBinary(FileName,true,Hdr,AuxGlobBuffer,AuxCodeBuffer,AuxBlock,AuxArrFixDef,AuxArrDynDef,AuxDlCall,Depen,AuxUndRef,AuxRelocTable,AuxLnkSym,AuxDbgSym,SrcInfo)){ return false; }

  //Return code
  return true;

}

//Import library file
//HardLinkage: true=Normal library linkage when building final executable, false=No code or data is imported, just linker symbols
bool Binary::ImportLibrary(const String& FileName,bool HardLinkage,CpuAdr& InitAddress,CpuShr Major,CpuShr Minor,CpuShr RevNr,const SourceInfo& SrcInfo){
  
  //Program buffers (local copy)
  Buffer AuxGlobBuffer;
  Buffer AuxCodeBuffer;
  Array<BlockDef> AuxBlock;
  Array<ArrayFixDef> AuxArrFixDef;
  Array<ArrayDynDef> AuxArrDynDef;
  Array<DlCallDef> AuxDlCall;
  Array<Dependency> AuxDepen;
  Array<UndefinedRefer> AuxUndRef;
  Array<RelocItem> AuxRelocTable;

  //Variables
  int i,j;
  const char *p;
  Buffer Buff;
  CpuAdr Address;
  CpuMbl Block;
  CpuAgx GeomIndex;
  CpuInt DlCallId;
  BinaryHeader Hdr;
  BlockDef Blk;
  RelocItem RelItem;

  //Debug message
  DebugMessage(DebugLevel::CmpBinary,"Import library file "+FileName);

  //Clear tables
  IUndRef.Reset();
  ILnkSymTables.Dim.Reset();
  ILnkSymTables.Typ.Reset();
  ILnkSymTables.Var.Reset();
  ILnkSymTables.Fld.Reset();
  ILnkSymTables.Fun.Reset();
  ILnkSymTables.Par.Reset();
  _IDbgSymTables.Mod.Reset();
  _IDbgSymTables.Typ.Reset();
  _IDbgSymTables.Var.Reset();
  _IDbgSymTables.Fld.Reset();
  _IDbgSymTables.Fun.Reset();
  _IDbgSymTables.Par.Reset();
  _IDbgSymTables.Lin.Reset();

  //Read binary file
  if(!_ReadBinary(FileName,true,Hdr,AuxGlobBuffer,AuxCodeBuffer,AuxBlock,AuxArrFixDef,AuxArrDynDef,AuxDlCall,AuxDepen,IUndRef,AuxRelocTable,ILnkSymTables,_IDbgSymTables,SrcInfo)){ return false; }

  //Check version requirement
  if(!(Major==-1 && Minor==-1 && RevNr==-1)){
    if(Hdr.LibMajorVers<Major
    ||(Hdr.LibMajorVers==Major && Hdr.LibMinorVers<Minor)
    ||(Hdr.LibMajorVers==Major && Hdr.LibMinorVers==Minor && Hdr.LibRevisionNr<RevNr)){

      //Clear tables
      IUndRef.Reset();
      ILnkSymTables.Dim.Reset();
      ILnkSymTables.Typ.Reset();
      ILnkSymTables.Var.Reset();
      ILnkSymTables.Fld.Reset();
      ILnkSymTables.Fun.Reset();
      ILnkSymTables.Par.Reset();
      _IDbgSymTables.Mod.Reset();
      _IDbgSymTables.Typ.Reset();
      _IDbgSymTables.Var.Reset();
      _IDbgSymTables.Fld.Reset();
      _IDbgSymTables.Fun.Reset();
      _IDbgSymTables.Par.Reset();
      _IDbgSymTables.Lin.Reset();

      //Inform about error and return
      SrcInfo.Msg(441).Print(_Stl->FileSystem.GetFileName(FileName),
      ToString(Hdr.LibMajorVers)+"."+ToString(Hdr.LibMinorVers)+"."+ToString(Hdr.LibRevisionNr),
      ToString(Major)+"."+ToString(Minor)+"."+ToString(RevNr));
      return false;
    
    }
  }

  //Hard linkage mode (actual import of everything, code, data and all)
  if(HardLinkage){ 

    //Process relocation table
    for(i=0;i<AuxRelocTable.Length();i++){

      //Debug message
      DebugAppend(DebugLevel::CmpRelocation,"Relocation apply: Type="+RelocationTypeText(AuxRelocTable[i].Type)+", LocAdr="+HEXFORMAT(AuxRelocTable[i].LocAdr)+
      ", LocBlk="+HEXFORMAT(AuxRelocTable[i].LocBlk)+", Module="+String((char *)AuxRelocTable[i].Module)+", ObjName="+String((char *)AuxRelocTable[i].ObjName)+
      ", CopyCount="+ToString(AuxRelocTable[i].CopyCount)+" ");
      
      //Apply reloction of items
      switch(AuxRelocTable[i].Type){
        
        //Function address (uses LocAdr)
        case RelocType::FunAddress: 
          Address=*reinterpret_cast<const CpuAdr *>(&AuxCodeBuffer[AuxRelocTable[i].LocAdr]);
          DebugAppend(DebugLevel::CmpRelocation,"{Address="+HEXFORMAT(Address)+ " -> ");
          Address+=_CodeBuffer.Length();
          DebugMessage(DebugLevel::CmpRelocation,HEXFORMAT(Address)+ "}");
          p=reinterpret_cast<const char *>(&Address);
          for(j=0;j<(int)sizeof(CpuAdr);j++){ AuxCodeBuffer[AuxRelocTable[i].LocAdr+j]=p[j]; }
          break;
        
        //Global variable address (uses LocAdr)
        case RelocType::GloAddress: 
          Address=*reinterpret_cast<const CpuAdr *>(&AuxCodeBuffer[AuxRelocTable[i].LocAdr]);
          DebugAppend(DebugLevel::CmpRelocation,"{Address="+HEXFORMAT(Address)+ " -> ");
          Address+=_GlobBuffer.Length();
          DebugMessage(DebugLevel::CmpRelocation,HEXFORMAT(Address)+ "}");
          p=reinterpret_cast<const char *>(&Address);
          for(j=0;j<(int)sizeof(CpuAdr);j++){ AuxCodeBuffer[AuxRelocTable[i].LocAdr+j]=p[j]; }
          break;
        
        //Fixed array geometry index (uses LocAdr)
        case RelocType::ArrGeom: 
          GeomIndex=*reinterpret_cast<const CpuAgx *>(&AuxCodeBuffer[AuxRelocTable[i].LocAdr]);
          DebugAppend(DebugLevel::CmpRelocation,"{GeomIndex="+HEXFORMAT(GeomIndex)+ " -> ");
          GeomIndex+=_GlobGeometryNr;
          DebugMessage(DebugLevel::CmpRelocation,HEXFORMAT(GeomIndex)+ "}");
          p=reinterpret_cast<const char *>(&GeomIndex);
          for(j=0;j<(int)sizeof(CpuAgx);j++){ AuxCodeBuffer[AuxRelocTable[i].LocAdr+j]=p[j]; }
          break;
        
        //Dynamic library call id (uses LocAdr)
        case RelocType::DlCall: 
          DlCallId=*reinterpret_cast<const CpuInt *>(&AuxCodeBuffer[AuxRelocTable[i].LocAdr]);
          DebugAppend(DebugLevel::CmpRelocation,"{DlCallId="+HEXFORMAT(DlCallId)+ " -> ");
          DlCallId+=_DlCall.Length();
          DebugMessage(DebugLevel::CmpRelocation,HEXFORMAT(DlCallId)+ "}");
          p=reinterpret_cast<const char *>(&DlCallId);
          for(j=0;j<(int)sizeof(CpuInt);j++){ AuxCodeBuffer[AuxRelocTable[i].LocAdr+j]=p[j]; }
          break;
        
        //Block inside global buffer (uses LocAdr)
        case RelocType::GloBlock:  
          Block=*reinterpret_cast<const CpuMbl *>(&AuxGlobBuffer[AuxRelocTable[i].LocAdr]);
          DebugAppend(DebugLevel::CmpRelocation,"{Block="+HEXFORMAT(Block)+ " -> ");
          Block+=_Block.Length();
          DebugMessage(DebugLevel::CmpRelocation,HEXFORMAT(Block)+ "}");
          p=reinterpret_cast<const char *>(&Block);
          for(j=0;j<(int)sizeof(CpuMbl);j++){ AuxGlobBuffer[AuxRelocTable[i].LocAdr+j]=p[j]; }
          break;
        
        //Block inside block (uses LocAdr/LocBlk)
        case RelocType::BlkBlock:  
          Block=*reinterpret_cast<const CpuMbl *>(&AuxBlock[AuxRelocTable[i].LocBlk].Buff[AuxRelocTable[i].LocAdr]);
          DebugAppend(DebugLevel::CmpRelocation,"{Block="+HEXFORMAT(Block)+ " -> ");
          Block+=_Block.Length();
          DebugMessage(DebugLevel::CmpRelocation,HEXFORMAT(Block)+ "}");
          p=reinterpret_cast<const char *>(&Block);
          for(j=0;j<(int)sizeof(CpuMbl);j++){ AuxBlock[AuxRelocTable[i].LocBlk].Buff[AuxRelocTable[i].LocAdr+j]=p[j]; }
          break;

      }

      //Modify relocation entries, so they are still valid after applying relocation above
      //In case we load libraries and compile as library the relocation table from loaded libraries is saved as well in the relocation table of the final library
      switch(AuxRelocTable[i].Type){
        case RelocType::FunAddress: AuxRelocTable[i].LocAdr+=_CodeBuffer.Length(); break;
        case RelocType::GloAddress: AuxRelocTable[i].LocAdr+=_CodeBuffer.Length(); break;
        case RelocType::ArrGeom:    AuxRelocTable[i].LocAdr+=_CodeBuffer.Length(); break;
        case RelocType::DlCall:     AuxRelocTable[i].LocAdr+=_CodeBuffer.Length(); break;
        case RelocType::GloBlock:   AuxRelocTable[i].LocAdr+=_GlobBuffer.Length(); break;
        case RelocType::BlkBlock:   AuxRelocTable[i].LocBlk+=_Block.Length();  break;
      }

    }

    //Return super init address (applying relocation)
    InitAddress=Hdr.SuperInitAdr+_CodeBuffer.Length();  

    //Relocate addresses in linker symbol tables
    for(i=0;i<ILnkSymTables.Dim.Length();i++){
      //GeomIndex
      DebugAppend(DebugLevel::CmpRelocation,"Linker symbol relocation: [GeomIndex] GeometryNr="+HEXFORMAT(ILnkSymTables.Dim[i].GeomIndex));
      ILnkSymTables.Dim[i].GeomIndex+=_GlobGeometryNr;
      DebugMessage(DebugLevel::CmpRelocation," -> "+HEXFORMAT(ILnkSymTables.Dim[i].GeomIndex));
    }
    for(i=0;i<ILnkSymTables.Typ.Length();i++){
      //TypMetaName
      DebugAppend(DebugLevel::CmpRelocation,"Linker symbol relocation: [TypMetaName] TypeName="+String((char *)ILnkSymTables.Typ[i].Name)+", GlobAddress="+HEXFORMAT(ILnkSymTables.Typ[i].MetaName));
      ILnkSymTables.Typ[i].MetaName+=_GlobBuffer.Length();
      DebugMessage(DebugLevel::CmpRelocation," -> "+HEXFORMAT(ILnkSymTables.Typ[i].MetaName));
      //TypMetaStNames
      DebugAppend(DebugLevel::CmpRelocation,"Linker symbol relocation: [TypMetaStNames] TypeName="+String((char *)ILnkSymTables.Typ[i].Name)+", GlobAddress="+HEXFORMAT(ILnkSymTables.Typ[i].MetaStNames));
      ILnkSymTables.Typ[i].MetaStNames+=_GlobBuffer.Length();
      DebugMessage(DebugLevel::CmpRelocation," -> "+HEXFORMAT(ILnkSymTables.Typ[i].MetaStNames));
      //TypMetaStTypes
      DebugAppend(DebugLevel::CmpRelocation,"Linker symbol relocation: [TypMetaStTypes] TypeName="+String((char *)ILnkSymTables.Typ[i].Name)+", GlobAddress="+HEXFORMAT(ILnkSymTables.Typ[i].MetaStTypes));
      ILnkSymTables.Typ[i].MetaStTypes+=_GlobBuffer.Length();
      DebugMessage(DebugLevel::CmpRelocation," -> "+HEXFORMAT(ILnkSymTables.Typ[i].MetaStTypes));
    }
    for(i=0;i<ILnkSymTables.Var.Length();i++){
      //Variable
      DebugAppend(DebugLevel::CmpRelocation,"Linker symbol relocation: [Variable] VarName="+String((char *)ILnkSymTables.Var[i].Name)+", GlobAddress="+HEXFORMAT(ILnkSymTables.Var[i].Address));
      ILnkSymTables.Var[i].Address+=_GlobBuffer.Length(); 
      DebugMessage(DebugLevel::CmpRelocation," -> "+HEXFORMAT(ILnkSymTables.Var[i].Address));
      //VarMetaName
      DebugAppend(DebugLevel::CmpRelocation,"Linker symbol relocation: [VarMetaName] VarName="+String((char *)ILnkSymTables.Var[i].Name)+", GlobAddress="+HEXFORMAT(ILnkSymTables.Var[i].MetaName));
      ILnkSymTables.Var[i].MetaName+=_GlobBuffer.Length();
      DebugMessage(DebugLevel::CmpRelocation," -> "+HEXFORMAT(ILnkSymTables.Var[i].MetaName));
    }
    for(i=0;i<ILnkSymTables.Fun.Length();i++){
      //Function
      DebugAppend(DebugLevel::CmpRelocation,"Linker symbol relocation: [Function] FunName="+String((char *)ILnkSymTables.Fun[i].Name)+", CodeAddress="+HEXFORMAT(ILnkSymTables.Fun[i].Address));
      ILnkSymTables.Fun[i].Address+=_CodeBuffer.Length();
      DebugMessage(DebugLevel::CmpRelocation," -> "+HEXFORMAT(ILnkSymTables.Fun[i].Address));
    }

    //Relocate addresses in debug symbol tables
    for(i=0;i<_IDbgSymTables.Var.Length();i++){
      DebugAppend(DebugLevel::CmpRelocation,"Debug symbol relocation: [Variable] VarName="+String((char *)_IDbgSymTables.Var[i].Name)+", GlobAddress="+HEXFORMAT(_IDbgSymTables.Var[i].Address));
      _IDbgSymTables.Var[i].Address+=_GlobBuffer.Length(); DebugMessage(DebugLevel::CmpRelocation," -> "+HEXFORMAT(_IDbgSymTables.Var[i].Address));
    }
    for(i=0;i<_IDbgSymTables.Fun.Length();i++){
      DebugAppend(DebugLevel::CmpRelocation,"Debug symbol relocation: [Function] FunName="+String((char *)_IDbgSymTables.Fun[i].Name)+", BegAddress="+HEXFORMAT(_IDbgSymTables.Fun[i].BegAddress));
      _IDbgSymTables.Fun[i].BegAddress+=_CodeBuffer.Length(); DebugMessage(DebugLevel::CmpRelocation," -> "+HEXFORMAT(_IDbgSymTables.Fun[i].BegAddress));
      DebugAppend(DebugLevel::CmpRelocation,"Debug symbol relocation: [Function] FunName="+String((char *)_IDbgSymTables.Fun[i].Name)+", EndAddress="+HEXFORMAT(_IDbgSymTables.Fun[i].EndAddress));
      _IDbgSymTables.Fun[i].EndAddress+=_CodeBuffer.Length(); DebugMessage(DebugLevel::CmpRelocation," -> "+HEXFORMAT(_IDbgSymTables.Fun[i].EndAddress));
    }
    for(i=0;i<_IDbgSymTables.Lin.Length();i++){
      DebugAppend(DebugLevel::CmpRelocation,"Debug symbol relocation: [Line] Module="+String((char *)_IDbgSymTables.Mod[_IDbgSymTables.Lin[i].ModIndex].Name)+", LineNr="+ToString(_IDbgSymTables.Lin[i].LineNr)+", BegAddress="+HEXFORMAT(_IDbgSymTables.Lin[i].BegAddress));
      _IDbgSymTables.Lin[i].BegAddress+=_CodeBuffer.Length(); DebugMessage(DebugLevel::CmpRelocation," -> "+HEXFORMAT(_IDbgSymTables.Lin[i].BegAddress));
      DebugAppend(DebugLevel::CmpRelocation,"Debug symbol relocation: [Line] Module="+String((char *)_IDbgSymTables.Mod[_IDbgSymTables.Lin[i].ModIndex].Name)+", LineNr="+ToString(_IDbgSymTables.Lin[i].LineNr)+", EndAddress="+HEXFORMAT(_IDbgSymTables.Lin[i].EndAddress));
      _IDbgSymTables.Lin[i].EndAddress+=_CodeBuffer.Length(); DebugMessage(DebugLevel::CmpRelocation," -> "+HEXFORMAT(_IDbgSymTables.Lin[i].EndAddress));
    }

    //Relocate addresses in undefined references table
    for(i=0;i<IUndRef.Length();i++){
      DebugAppend(DebugLevel::CmpRelocation,"Undefined reference relocation: Module="+String((char *)IUndRef[i].Module)+" Kind="+ToString(IUndRef[i].Kind)+" ObjectName="+IUndRef[i].ObjectName+" Address="+HEXFORMAT(IUndRef[i].CodeAdr));
      IUndRef[i].CodeAdr+=_CodeBuffer.Length();
      DebugMessage(DebugLevel::CmpRelocation," -> "+HEXFORMAT(IUndRef[i].CodeAdr));
    }

    //Relocate dynamic array indexes in global blocks
    for(i=0;i<AuxBlock.Length();i++){
      if(AuxBlock[i].ArrIndex!=-1){ AuxBlock[i].ArrIndex+=_ArrDynDef.Length(); }
    }

    //Relocate indexes in debug symbol tables
    for(i=0;i<_IDbgSymTables.Typ.Length();i++){
      if(_IDbgSymTables.Typ[i].FieldLow!=-1){ _IDbgSymTables.Typ[i].FieldLow+=_ODbgSymTables.Fld.Length(); }
      if(_IDbgSymTables.Typ[i].FieldHigh!=-1){ _IDbgSymTables.Typ[i].FieldHigh+=_ODbgSymTables.Fld.Length(); }
    }
    for(i=0;i<_IDbgSymTables.Var.Length();i++){
      if(_IDbgSymTables.Var[i].ModIndex!=-1){ _IDbgSymTables.Var[i].ModIndex+=_ODbgSymTables.Mod.Length(); }
      if(_IDbgSymTables.Var[i].FunIndex!=-1){ _IDbgSymTables.Var[i].FunIndex+=_ODbgSymTables.Fun.Length(); }
      if(_IDbgSymTables.Var[i].TypIndex!=-1){ _IDbgSymTables.Var[i].TypIndex+=_ODbgSymTables.Typ.Length(); }
    }
    for(i=0;i<_IDbgSymTables.Fld.Length();i++){
      if(_IDbgSymTables.Fld[i].TypIndex!=-1){ _IDbgSymTables.Fld[i].TypIndex+=_ODbgSymTables.Typ.Length(); }
    }
    for(i=0;i<_IDbgSymTables.Fun.Length();i++){
      if(_IDbgSymTables.Fun[i].ModIndex!=-1){ _IDbgSymTables.Fun[i].ModIndex+=_ODbgSymTables.Mod.Length(); }
      if(_IDbgSymTables.Fun[i].TypIndex!=-1){ _IDbgSymTables.Fun[i].TypIndex+=_ODbgSymTables.Typ.Length(); }
      if(_IDbgSymTables.Fun[i].ParmLow!=-1){ _IDbgSymTables.Fun[i].ParmLow+=_ODbgSymTables.Par.Length(); }
      if(_IDbgSymTables.Fun[i].ParmHigh!=-1){ _IDbgSymTables.Fun[i].ParmHigh+=_ODbgSymTables.Par.Length(); }
    }
    for(i=0;i<_IDbgSymTables.Par.Length();i++){
      if(_IDbgSymTables.Par[i].TypIndex!=-1){ _IDbgSymTables.Par[i].TypIndex+=_ODbgSymTables.Typ.Length(); }
      if(_IDbgSymTables.Par[i].FunIndex!=-1){ _IDbgSymTables.Par[i].FunIndex+=_ODbgSymTables.Fun.Length(); }
    }
    for(i=0;i<_IDbgSymTables.Lin.Length();i++){
      if(_IDbgSymTables.Lin[i].ModIndex!=-1){ _IDbgSymTables.Lin[i].ModIndex+=_ODbgSymTables.Mod.Length(); }
    }

    //Append debug symbols to output tables
    for(i=0;i<_IDbgSymTables.Mod.Length();i++){ StoreDbgSymModule(String((char *)_IDbgSymTables.Mod[i].Name),String((char *)_IDbgSymTables.Mod[i].Path),SrcInfo); }
    for(i=0;i<_IDbgSymTables.Typ.Length();i++){ StoreDbgSymType(String((char *)_IDbgSymTables.Typ[i].Name),_IDbgSymTables.Typ[i].CpuType,_IDbgSymTables.Typ[i].MstType,_IDbgSymTables.Typ[i].Length,_IDbgSymTables.Typ[i].FieldLow,_IDbgSymTables.Typ[i].FieldHigh,SrcInfo); }
    for(i=0;i<_IDbgSymTables.Var.Length();i++){ StoreDbgSymVariable(String((char *)_IDbgSymTables.Var[i].Name),_IDbgSymTables.Var[i].ModIndex,_IDbgSymTables.Var[i].Glob,_IDbgSymTables.Var[i].FunIndex,_IDbgSymTables.Var[i].TypIndex,_IDbgSymTables.Var[i].Address,_IDbgSymTables.Var[i].IsReference,SrcInfo); }
    for(i=0;i<_IDbgSymTables.Fld.Length();i++){ StoreDbgSymField(String((char *)_IDbgSymTables.Fld[i].Name),_IDbgSymTables.Fld[i].TypIndex,_IDbgSymTables.Fld[i].Offset,SrcInfo); }
    for(i=0;i<_IDbgSymTables.Fun.Length();i++){ StoreDbgSymFunction(_IDbgSymTables.Fun[i].Kind,String((char *)_IDbgSymTables.Fun[i].Name),_IDbgSymTables.Fun[i].ModIndex,_IDbgSymTables.Fun[i].BegAddress,_IDbgSymTables.Fun[i].EndAddress,_IDbgSymTables.Fun[i].TypIndex,_IDbgSymTables.Fun[i].IsVoid,_IDbgSymTables.Fun[i].ParmNr,_IDbgSymTables.Fun[i].ParmLow,_IDbgSymTables.Fun[i].ParmHigh,SrcInfo); }
    for(i=0;i<_IDbgSymTables.Par.Length();i++){ StoreDbgSymParameter(String((char *)_IDbgSymTables.Par[i].Name),_IDbgSymTables.Par[i].TypIndex,_IDbgSymTables.Par[i].FunIndex,_IDbgSymTables.Par[i].IsReference,SrcInfo); }
    for(i=0;i<_IDbgSymTables.Lin.Length();i++){ StoreDbgSymLine(_IDbgSymTables.Lin[i].ModIndex,_IDbgSymTables.Lin[i].BegAddress,_IDbgSymTables.Lin[i].EndAddress,_IDbgSymTables.Lin[i].LineNr); }

    //Append library objects to current program
    #ifdef __DEV__
    CpuAdr GlobSize=_GlobBuffer.Length();
    CpuAdr CodeSize=_CodeBuffer.Length();
    #endif
    DebugAppend(DebugLevel::CmpLibrary,"Library linkage: Glob,Code,Geom ("+HEXFORMAT(GlobSize)+","+HEXFORMAT(CodeSize)+","+ToString(_GlobGeometryNr)+")");
    _GlobBuffer+=AuxGlobBuffer;
    _CodeBuffer+=AuxCodeBuffer;
    _GlobGeometryNr+=Hdr.ArrFixDefNr;
    for(i=0;i<AuxArrFixDef.Length();i++){ _ArrFixDef.Add(AuxArrFixDef[i]); }
    for(i=0;i<AuxArrDynDef.Length();i++){ _ArrDynDef.Add(AuxArrDynDef[i]); }
    for(i=0;i<AuxBlock.Length();i++){ _Block.Add(AuxBlock[i]); }
    for(i=0;i<AuxDlCall.Length();i++){ 
      _DlCall.Add(AuxDlCall[i]); 
    }
    #ifdef __DEV__
    GlobSize=_GlobBuffer.Length();
    CodeSize=_CodeBuffer.Length();
    #endif
    DebugMessage(DebugLevel::CmpLibrary," -> ("+HEXFORMAT(GlobSize)+","+HEXFORMAT(CodeSize)+","+ToString(_GlobGeometryNr)+")");
    
    //Copy relocation table into current binary
    for(i=0;i<AuxRelocTable.Length();i++){ 
      RelItem=AuxRelocTable[i];
      RelItem.CopyCount++;
      _RelocTable.Add(RelItem); 
      DebugMessage(DebugLevel::CmpRelocation,"Relocation copied: Type="+RelocationTypeText(RelItem.Type)+", LocAdr="+HEXFORMAT(RelItem.LocAdr)+", LocBlk="+HEXFORMAT(RelItem.LocBlk)+
      ", Module="+String((char *)RelItem.Module)+", ObjName="+String((char *)RelItem.ObjName)+", CopyCount="+ToString(RelItem.CopyCount));
    }

  }

  //Soft linkage mode
  //Only linker symbols and undefined references are imported
  //All addresses and geom indexes are set to zero (unresolved)
  else{

    //Set as unresolved all adresses and geom indexes
    for(i=0;i<ILnkSymTables.Dim.Length();i++){
      ILnkSymTables.Dim[i].GeomIndex=0;
    }
    for(i=0;i<ILnkSymTables.Typ.Length();i++){
      ILnkSymTables.Typ[i].MetaName=0;
      ILnkSymTables.Typ[i].MetaStNames=0;
      ILnkSymTables.Typ[i].MetaStTypes=0;
    }
    for(i=0;i<ILnkSymTables.Var.Length();i++){
      ILnkSymTables.Var[i].Address=0;
      ILnkSymTables.Var[i].MetaName=0;
    }
    for(i=0;i<ILnkSymTables.Fun.Length();i++){
      ILnkSymTables.Fun[i].Address=0;
    }

    //Clear imported debug symbol tables
    _IDbgSymTables.Mod.Reset();
    _IDbgSymTables.Typ.Reset();
    _IDbgSymTables.Var.Reset();
    _IDbgSymTables.Fld.Reset();
    _IDbgSymTables.Fun.Reset();
    _IDbgSymTables.Par.Reset();
    _IDbgSymTables.Lin.Reset();

    //Clear addresses in undefined references table
    for(i=0;i<IUndRef.Length();i++){
      IUndRef[i].CodeAdr=0;
    }

  }

  //Return code
  return true;

}

//Print binary info
bool Binary::PrintBinary(const String& FileName,bool Library){
  
  //Table delimiter
  const String Delimiter=";";

  //Program buffers (local copy)
  Buffer AuxGlobBuffer;
  Buffer AuxCodeBuffer;
  Buffer Content;
  Array<BlockDef> AuxBlock;
  Array<ArrayFixDef> AuxArrFixDef;
  Array<ArrayDynDef> AuxArrDynDef;
  Array<DlCallDef> AuxDlCall;
  Array<Dependency> AuxDepen;
  Array<UndefinedRefer> AuxUndRef;
  Array<RelocItem> AuxRelocTable;
  LnkSymTables AuxLnkSym;
  DbgSymTables AuxDbgSym;

  //Variables
  int i;
  Buffer Buff;
  BinaryHeader Hdr;
  BlockDef Blk;
  String Headings;
  Array<String> Lines;
  String DimSizes;
  String RelocEntry;

  //Read binary file
  if(!_ReadBinary(FileName,Library,Hdr,AuxGlobBuffer,AuxCodeBuffer,AuxBlock,AuxArrFixDef,AuxArrDynDef,AuxDlCall,AuxDepen,AuxUndRef,AuxRelocTable,AuxLnkSym,AuxDbgSym,SourceInfo())){ return false; }

  //Print file name
  Headings="Binary file dump - "+FileName;
  _Stl->Console.PrintLine(String(Headings.Length(),'-'));
  _Stl->Console.PrintLine(Headings);
  _Stl->Console.PrintLine(String(Headings.Length(),'-'));
  _Stl->Console.PrintLine("");

  //Print program header
  _Stl->Console.PrintLine("[ Binary header ]");
  _Stl->Console.PrintLine("FileMark     : "+String(Buffer(Hdr.FileMark,sizeof(Hdr.FileMark))));
  _Stl->Console.PrintLine("BinFormat    : "+ToString(Hdr.BinFormat));
  _Stl->Console.PrintLine("Arquitecture : "+ToString(Hdr.Arquitecture));
  _Stl->Console.PrintLine("SysVersion   : "+String(Hdr.SysVersion));
  _Stl->Console.PrintLine("SysBuildDate : "+String(Hdr.SysBuildDate));
  _Stl->Console.PrintLine("SysBuildTime : "+String(Hdr.SysBuildTime));
  _Stl->Console.PrintLine("IsBinLibrary : "+ToString(Hdr.IsBinLibrary));
  _Stl->Console.PrintLine("DebugSymbols : "+ToString(Hdr.DebugSymbols));
  _Stl->Console.PrintLine("GlobBufferNr : "+ToString(Hdr.GlobBufferNr));
  _Stl->Console.PrintLine("BlockNr      : "+ToString(Hdr.BlockNr));
  _Stl->Console.PrintLine("ArrFixDefNr  : "+ToString(Hdr.ArrFixDefNr));
  _Stl->Console.PrintLine("ArrDynDefNr  : "+ToString(Hdr.ArrDynDefNr));
  _Stl->Console.PrintLine("CodeBufferNr : "+ToString(Hdr.CodeBufferNr));
  _Stl->Console.PrintLine("DlCallNr     : "+ToString(Hdr.DlCallNr));
  _Stl->Console.PrintLine("MemUnitSize  : "+ToString(Hdr.MemUnitSize));
  _Stl->Console.PrintLine("MemUnits     : "+ToString(Hdr.MemUnits));
  _Stl->Console.PrintLine("ChunkMemUnits: "+ToString(Hdr.ChunkMemUnits));
  _Stl->Console.PrintLine("BlockMax     : "+ToString(Hdr.BlockMax));
  _Stl->Console.PrintLine("LibMajorVers : "+ToString(Hdr.LibMajorVers));
  _Stl->Console.PrintLine("LibMinorVers : "+ToString(Hdr.LibMinorVers));
  _Stl->Console.PrintLine("LibRevisionNr: "+ToString(Hdr.LibRevisionNr));
  _Stl->Console.PrintLine("DependencyNr : "+ToString(Hdr.DependencyNr));
  _Stl->Console.PrintLine("UndefRefNr   : "+ToString(Hdr.UndefRefNr));
  _Stl->Console.PrintLine("RelocTableNr : "+ToString(Hdr.RelocTableNr));
  _Stl->Console.PrintLine("LnkSymDimNr  : "+ToString(Hdr.LnkSymDimNr));
  _Stl->Console.PrintLine("LnkSymTypNr  : "+ToString(Hdr.LnkSymTypNr));
  _Stl->Console.PrintLine("LnkSymVarNr  : "+ToString(Hdr.LnkSymVarNr));
  _Stl->Console.PrintLine("LnkSymFldNr  : "+ToString(Hdr.LnkSymFldNr));
  _Stl->Console.PrintLine("LnkSymFunNr  : "+ToString(Hdr.LnkSymFunNr));
  _Stl->Console.PrintLine("LnkSymParNr  : "+ToString(Hdr.LnkSymParNr));
  _Stl->Console.PrintLine("DbgSymModNr  : "+ToString(Hdr.DbgSymModNr));
  _Stl->Console.PrintLine("DbgSymTypNr  : "+ToString(Hdr.DbgSymTypNr));
  _Stl->Console.PrintLine("DbgSymVarNr  : "+ToString(Hdr.DbgSymVarNr));
  _Stl->Console.PrintLine("DbgSymFldNr  : "+ToString(Hdr.DbgSymFldNr));
  _Stl->Console.PrintLine("DbgSymFunNr  : "+ToString(Hdr.DbgSymFunNr));
  _Stl->Console.PrintLine("DbgSymParNr  : "+ToString(Hdr.DbgSymParNr));
  _Stl->Console.PrintLine("DbgSymLinNr  : "+ToString(Hdr.DbgSymLinNr));
  _Stl->Console.PrintLine("SuperInitAdr : "+HEXFORMAT(Hdr.SuperInitAdr));
  _Stl->Console.PrintLine("");

  //Print fixed array definitions
  if(Hdr.ArrFixDefNr!=0){
    _Stl->Console.PrintLine("[ Global fixed array geometries ]");
    Headings=String("Index<delim>GeomIndex<delim>DimNr<delim>CellSize<delim>DimensionSizes").Replace("<delim>",Delimiter);
    Lines.Reset();
    for(i=0;i<Hdr.ArrFixDefNr;i++){ 
      DimSizes=""; for(int j=0;j<AuxArrFixDef[i].DimNr;j++){ DimSizes+=" "+ToString(AuxArrFixDef[i].DimSize.n[j]); }
      Lines.Add((ToString(i)+"<delim>"+HEXFORMAT(AuxArrFixDef[i].GeomIndex)+"<delim>"+ToString(AuxArrFixDef[i].DimNr)+"<delim>"+ToString(AuxArrFixDef[i].CellSize)+"<delim>"+DimSizes).Replace("<delim>",Delimiter));
    }
    _Stl->Console.PrintTable(Headings,Lines,Delimiter);
    _Stl->Console.PrintLine("");
  }

  //Print dynamic array definitions
  if(Hdr.ArrDynDefNr!=0){
    _Stl->Console.PrintLine("[ Global dynamic array definitions ]");
    Headings=String("Index<delim>DimNr<delim>CellSize<delim>DimensionSizes").Replace("<delim>",Delimiter);
    Lines.Reset();
    for(i=0;i<Hdr.ArrDynDefNr;i++){ 
      DimSizes=""; for(int j=0;j<AuxArrDynDef[i].DimNr;j++){ DimSizes+=" "+ToString(AuxArrDynDef[i].DimSize.n[j]); }
      Lines.Add((ToString(i)+"<delim>"+ToString(AuxArrDynDef[i].DimNr)+"<delim>"+ToString(AuxArrDynDef[i].CellSize)+"<delim>"+DimSizes).Replace("<delim>",Delimiter));
    }
    _Stl->Console.PrintTable(Headings,Lines,Delimiter);
    _Stl->Console.PrintLine("");
  }

  //Print global blocks
  if(Hdr.BlockNr!=0){
    _Stl->Console.PrintLine("[ Global blocks ]");
    Headings=String("Index<delim>ArrIndex<delim>BufferLength<delim>Content").Replace("<delim>",Delimiter);
    Lines.Reset();
    for(i=0;i<Hdr.BlockNr;i++){ 
      if(AuxBlock[i].Buff.Length()>50){
        Content=Buffer(AuxBlock[i].Buff.BuffPnt(),50);
        Content+="...";
      }
      else{
        Content=AuxBlock[i].Buff;
      }
      Lines.Add((ToString(i)+"<delim>"+ToString(AuxBlock[i].ArrIndex)+"<delim>"+ToString(AuxBlock[i].Buff.Length())+"<delim>"+NonAsciiEscape(Content)).Replace("<delim>",Delimiter));
    }
    _Stl->Console.PrintTable(Headings,Lines,Delimiter);
    _Stl->Console.PrintLine("");
  }

  //Print dynamic library calls
  if(Hdr.DlCallNr!=0){
    _Stl->Console.PrintLine("[ Dynamic library calls ]");
    Headings=String("Index<delim>DlName<delim>DlFunction").Replace("<delim>",Delimiter);
    Lines.Reset();
    for(i=0;i<Hdr.DlCallNr;i++){ 
      Lines.Add((ToString(i)+"<delim>"+String((char *)AuxDlCall[i].DlName)+"<delim>"+String((char *)AuxDlCall[i].DlFunction)).Replace("<delim>",Delimiter));
    }
    _Stl->Console.PrintTable(Headings,Lines,Delimiter);
    _Stl->Console.PrintLine("");
  }

  //Print dependencies
  if(Library && Hdr.DependencyNr!=0){
    _Stl->Console.PrintLine("[ Dependencies ]");
    Headings=String("Index<delim>Module<delim>Version").Replace("<delim>",Delimiter);
    Lines.Reset();
    for(i=0;i<Hdr.DependencyNr;i++){ 
      Lines.Add((ToString(i)+"<delim>"
      +String((char *)AuxDepen[i].Module)+"<delim>"
      +ToString(AuxDepen[i].LibMajorVers)+"."+ToString(AuxDepen[i].LibMinorVers)+"."+ToString(AuxDepen[i].LibRevisionNr))
      .Replace("<delim>",Delimiter));
    }
    _Stl->Console.PrintTable(Headings,Lines,Delimiter);
    _Stl->Console.PrintLine("");
  }

  //Print unresolved references
  if(Library && Hdr.UndefRefNr!=0){
    _Stl->Console.PrintLine("[ Undefined references ]");
    Headings=String("Index<delim>Module<delim>Kind<delim>CodeAddress<delim>ObjectName").Replace("<delim>",Delimiter);
    Lines.Reset();
    for(i=0;i<Hdr.UndefRefNr;i++){ 
      Lines.Add((ToString(i)+"<delim>"
      +String((char *)AuxUndRef[i].Module)+"<delim>"
      +ToString(AuxUndRef[i].Kind)+"<delim>"
      +HEXFORMAT(AuxUndRef[i].CodeAdr)+"<delim>"
      +AuxUndRef[i].ObjectName)
      .Replace("<delim>",Delimiter));
    }
    _Stl->Console.PrintTable(Headings,Lines,Delimiter);
    _Stl->Console.PrintLine("");
  }

  //Print relocation entries
  if(Library && Hdr.RelocTableNr!=0){
    _Stl->Console.PrintLine("[ Relocation table ]");
    Headings=String("Index<delim>CodeAddress<delim>GlobAddress<delim>BlockOffset<delim>RelocationType<delim>Module<delim>ObjName<delim>CopyCount").Replace("<delim>",Delimiter);
    Lines.Reset();
    for(i=0;i<Hdr.RelocTableNr;i++){ 
      switch(AuxRelocTable[i].Type){
        case RelocType::FunAddress: RelocEntry=ToString(i)+"<delim>"+HEXFORMAT(AuxRelocTable[i].LocAdr)+"<delim><delim><delim>Function address relocation"; break;
        case RelocType::GloAddress: RelocEntry=ToString(i)+"<delim>"+HEXFORMAT(AuxRelocTable[i].LocAdr)+"<delim><delim><delim>Global variable address relocation"; break;
        case RelocType::ArrGeom   : RelocEntry=ToString(i)+"<delim>"+HEXFORMAT(AuxRelocTable[i].LocAdr)+"<delim><delim><delim>Fixed array geometry index relocation"; break;
        case RelocType::DlCall    : RelocEntry=ToString(i)+"<delim>"+HEXFORMAT(AuxRelocTable[i].LocAdr)+"<delim><delim><delim>Dynamic library call id relocation"; break;
        case RelocType::GloBlock  : RelocEntry=ToString(i)+"<delim><delim>"+HEXFORMAT(AuxRelocTable[i].LocAdr)+"<delim><delim>Block (inside global buffer) relocation"; break;
        case RelocType::BlkBlock  : RelocEntry=ToString(i)+"<delim><delim>"+HEXFORMAT(AuxRelocTable[i].LocAdr)+"<delim>"+HEXFORMAT(AuxRelocTable[i].LocBlk)+"<delim>Block (inside global block) relocation"; break;
      }
      RelocEntry+="<delim>"+String((char *)AuxRelocTable[i].Module)+"<delim>"+String((char *)AuxRelocTable[i].ObjName)+"<delim>"+ToString(AuxRelocTable[i].CopyCount);
      RelocEntry=RelocEntry.Replace("<delim>",Delimiter);
      Lines.Add(RelocEntry);
    }
    _Stl->Console.PrintTable(Headings,Lines,Delimiter);
    _Stl->Console.PrintLine("");
  }
  
  //Print dimension symbols
  if(Library && Hdr.LnkSymDimNr!=0){
    _Stl->Console.PrintLine("[ Linker symbols: Dimensions ]");
    Headings=String("Index<delim>GeomIndex<delim>DimensionSizes").Replace("<delim>",Delimiter);
    Lines.Reset();
    for(i=0;i<Hdr.LnkSymDimNr;i++){ 
      DimSizes=""; for(int j=0;j<_MaxArrayDims;j++){ DimSizes+=" "+ToString(AuxLnkSym.Dim[i].DimSize.n[j]); }
      Lines.Add(("["+ToString(i)+"<delim>"+ToString(AuxLnkSym.Dim[i].GeomIndex)+"<delim>"+DimSizes).Replace("<delim>",Delimiter));
    }
    _Stl->Console.PrintTable(Headings,Lines,Delimiter);
    _Stl->Console.PrintLine("");
  }

  //Print type symbols
  if(Library && Hdr.LnkSymTypNr!=0){
    _Stl->Console.PrintLine("[ Linker symbols: Data types ]");
    Headings=String("Index<delim>Name<delim>MstType<delim>FunIndex<delim>SupTypIndex<delim>IsTypedef<delim>OrigTypIndex<delim>IsSystemDef<delim>Length<delim>DimNr<delim>ElemTypIndex<delim>DimIndex<delim>FieldLow<delim>FieldHigh<delim>MemberLow<delim>MemberHigh").Replace("<delim>",Delimiter);
    Lines.Reset();
    for(i=0;i<Hdr.LnkSymTypNr;i++){ 
      Lines.Add((ToString(i)+"<delim>"
      +String((char *)AuxLnkSym.Typ[i].Name)+"<delim>"
      +ToString(AuxLnkSym.Typ[i].MstType)+"<delim>"
      +ToString(AuxLnkSym.Typ[i].FunIndex)+"<delim>"
      +ToString(AuxLnkSym.Typ[i].SupTypIndex)+"<delim>"
      +ToString(AuxLnkSym.Typ[i].IsTypedef)+"<delim>"
      +ToString(AuxLnkSym.Typ[i].OrigTypIndex)+"<delim>"
      +ToString(AuxLnkSym.Typ[i].IsSystemDef)+"<delim>"
      +ToString(AuxLnkSym.Typ[i].Length)+"<delim>"
      +ToString(AuxLnkSym.Typ[i].DimNr)+"<delim>"
      +ToString(AuxLnkSym.Typ[i].ElemTypIndex)+"<delim>"
      +ToString(AuxLnkSym.Typ[i].DimIndex)+"<delim>"
      +ToString(AuxLnkSym.Typ[i].FieldLow)+"<delim>"
      +ToString(AuxLnkSym.Typ[i].FieldHigh)+"<delim>"
      +ToString(AuxLnkSym.Typ[i].MemberLow)+"<delim>"
      +ToString(AuxLnkSym.Typ[i].MemberHigh))
      .Replace("<delim>",Delimiter));
    }
    _Stl->Console.PrintTable(Headings,Lines,Delimiter);
    _Stl->Console.PrintLine("");
  }


  //Print variable symbols
  if(Library && Hdr.LnkSymVarNr!=0){
    _Stl->Console.PrintLine("[ Linker symbols: Global variables ]");
    Headings=String("Index<delim>Name<delim>FunIndex<delim>TypIndex<delim>Address<delim>IsConst").Replace("<delim>",Delimiter);
    Lines.Reset();
    for(i=0;i<Hdr.LnkSymVarNr;i++){ 
      Lines.Add((ToString(i)+"<delim>"
      +String((char *)AuxLnkSym.Var[i].Name)+"<delim>"
      +ToString(AuxLnkSym.Var[i].FunIndex)+"<delim>"
      +ToString(AuxLnkSym.Var[i].TypIndex)+"<delim>"
      +HEXFORMAT(AuxLnkSym.Var[i].Address)+"<delim>"
      +ToString(AuxLnkSym.Var[i].IsConst))
      .Replace("<delim>",Delimiter));
    }
    _Stl->Console.PrintTable(Headings,Lines,Delimiter);
    _Stl->Console.PrintLine("");
  }

  //Print class field symbols
  if(Library && Hdr.LnkSymFldNr!=0){
    _Stl->Console.PrintLine("[ Linker symbols: Class / Enum fields ]");
    Headings=String("Index<delim>Name<delim>SupTypIndex<delim>SubScope<delim>TypIndex<delim>Offset<delim>IsStatic<delim>EnumValue").Replace("<delim>",Delimiter);
    Lines.Reset();
    for(i=0;i<Hdr.LnkSymFldNr;i++){ 
      Lines.Add((ToString(i)+"<delim>"
      +String((char *)AuxLnkSym.Fld[i].Name)+"<delim>"
      +ToString(AuxLnkSym.Fld[i].SupTypIndex)+"<delim>"
      +ToString(AuxLnkSym.Fld[i].SubScope)+"<delim>"
      +ToString(AuxLnkSym.Fld[i].TypIndex)+"<delim>"
      +ToString(AuxLnkSym.Fld[i].Offset)+"<delim>"
      +ToString(AuxLnkSym.Fld[i].IsStatic)+"<delim>"
      +ToString(AuxLnkSym.Fld[i].EnumValue))
      .Replace("<delim>",Delimiter));
    }
    _Stl->Console.PrintTable(Headings,Lines,Delimiter);
    _Stl->Console.PrintLine("");
  }

  //Print functions
  if(Library && Hdr.LnkSymFunNr!=0){
    _Stl->Console.PrintLine("[ Linker symbols: Functions ]");
    Headings=String("Index<delim>Name<delim>Kind<delim>SupTypIndex<delim>SubScope<delim>Address<delim>TypIndex<delim>IsVoid<delim>IsInit<delim>IsMeta<delim>ParmNr<delim>ParmLow<delim>ParmHigh<delim>MstType<delim>MstMethod<delim>SysCallNr<delim>InstCode<delim>DlName<delim>DlFunction").Replace("<delim>",Delimiter);
    Lines.Reset();
    for(i=0;i<Hdr.LnkSymFunNr;i++){ 
      Lines.Add((ToString(i)+"<delim>"
      +String((char *)AuxLnkSym.Fun[i].Name)+"<delim>"
      +ToString(AuxLnkSym.Fun[i].Kind)+"<delim>"
      +ToString(AuxLnkSym.Fun[i].SupTypIndex)+"<delim>"
      +ToString(AuxLnkSym.Fun[i].SubScope)+"<delim>"
      +HEXFORMAT(AuxLnkSym.Fun[i].Address)+"<delim>"
      +ToString(AuxLnkSym.Fun[i].TypIndex)+"<delim>"
      +ToString(AuxLnkSym.Fun[i].IsVoid)+"<delim>"
      +ToString(AuxLnkSym.Fun[i].IsInitializer)+"<delim>"
      +ToString(AuxLnkSym.Fun[i].IsMetaMethod)+"<delim>"
      +ToString(AuxLnkSym.Fun[i].ParmNr)+"<delim>"
      +ToString(AuxLnkSym.Fun[i].ParmLow)+"<delim>"
      +ToString(AuxLnkSym.Fun[i].ParmHigh)+"<delim>"
      +ToString(AuxLnkSym.Fun[i].MstType)+"<delim>"
      +ToString(AuxLnkSym.Fun[i].MstMethod)+"<delim>"
      +ToString(AuxLnkSym.Fun[i].SysCallNr)+"<delim>"
      +ToString(AuxLnkSym.Fun[i].InstCode)+"<delim>"
      +String((char *)AuxLnkSym.Fun[i].DlName)+"<delim>"
      +String((char *)AuxLnkSym.Fun[i].DlFunction))
      .Replace("<delim>",Delimiter));
    }
    _Stl->Console.PrintTable(Headings,Lines,Delimiter);
    _Stl->Console.PrintLine("");
  }

  //Print function parameters
  if(Library && Hdr.LnkSymParNr!=0){
    _Stl->Console.PrintLine("[ Linker symbols: Parameters ]");
    Headings=String("Index<delim>Name<delim>TypIndex<delim>IsConst<delim>IsReference<delim>ParmOrder<delim>FunIndex").Replace("<delim>",Delimiter);
    Lines.Reset();
    for(i=0;i<Hdr.LnkSymParNr;i++){ 
      Lines.Add((ToString(i)+"<delim>"
      +String((char *)AuxLnkSym.Par[i].Name)+"<delim>"
      +ToString(AuxLnkSym.Par[i].TypIndex)+"<delim>"
      +ToString(AuxLnkSym.Par[i].IsConst)+"<delim>"
      +ToString(AuxLnkSym.Par[i].IsReference)+"<delim>"
      +ToString(AuxLnkSym.Par[i].ParmOrder)+"<delim>"
      +ToString(AuxLnkSym.Par[i].FunIndex))
      .Replace("<delim>",Delimiter));
    }
    _Stl->Console.PrintTable(Headings,Lines,Delimiter);
    _Stl->Console.PrintLine("");
  }

  //Debug symbols modules
  if(Hdr.DebugSymbols && Hdr.DbgSymModNr!=0){
    _Stl->Console.PrintLine("[ Debug symbols: Modules ]");
    Headings=String("Index<delim>Name<delim>Path").Replace("<delim>",Delimiter);
    Lines.Reset();
    for(i=0;i<Hdr.DbgSymModNr;i++){ 
      Lines.Add((ToString(i)+"<delim>"
      +String((char *)AuxDbgSym.Mod[i].Name)+"<delim>"
      +String((char *)AuxDbgSym.Mod[i].Path))
      .Replace("<delim>",Delimiter));
    }
    _Stl->Console.PrintTable(Headings,Lines,Delimiter);
    _Stl->Console.PrintLine("");
  }

  //Debug symbols types
  if(Hdr.DebugSymbols && Hdr.DbgSymTypNr!=0){
    _Stl->Console.PrintLine("[ Debug symbols: Types ]");
    Headings=String("Index<delim>Name<delim>CpuType<delim>MstType<delim>Length<delim>FieldLow<delim>FieldHigh").Replace("<delim>",Delimiter);
    Lines.Reset();
    for(i=0;i<Hdr.DbgSymTypNr;i++){ 
      Lines.Add((ToString(i)+"<delim>"
      +String((char *)AuxDbgSym.Typ[i].Name)+"<delim>"
      +CpuDataTypeName((CpuDataType)AuxDbgSym.Typ[i].CpuType)+"<delim>"
      +ToString(AuxDbgSym.Typ[i].MstType)+"<delim>"
      +ToString(AuxDbgSym.Typ[i].Length)+"<delim>"
      +ToString(AuxDbgSym.Typ[i].FieldLow)+"<delim>"
      +ToString(AuxDbgSym.Typ[i].FieldHigh))
      .Replace("<delim>",Delimiter));
    }
    _Stl->Console.PrintTable(Headings,Lines,Delimiter);
    _Stl->Console.PrintLine("");
  }

  //Debug symbols variables
  if(Hdr.DebugSymbols && Hdr.DbgSymVarNr!=0){
    _Stl->Console.PrintLine("[ Debug symbols: Variables ]");
    Headings=String("Index<delim>Name<delim>ModIndex<delim>Glob<delim>FunIndex<delim>TypIndex<delim>Address<delim>IsReference").Replace("<delim>",Delimiter);
    Lines.Reset();
    for(i=0;i<Hdr.DbgSymVarNr;i++){ 
      Lines.Add((ToString(i)+"<delim>"
      +String((char *)AuxDbgSym.Var[i].Name)+"<delim>"
      +ToString(AuxDbgSym.Var[i].ModIndex)+"<delim>"
      +ToString(AuxDbgSym.Var[i].Glob)+"<delim>"
      +ToString(AuxDbgSym.Var[i].FunIndex)+"<delim>"
      +ToString(AuxDbgSym.Var[i].TypIndex)+"<delim>"
      +HEXFORMAT(AuxDbgSym.Var[i].Address)+"<delim>"
      +ToString(AuxDbgSym.Var[i].IsReference))
      .Replace("<delim>",Delimiter));
    }
    _Stl->Console.PrintTable(Headings,Lines,Delimiter);
    _Stl->Console.PrintLine("");
  }

  //Debug symbols fields
  if(Hdr.DebugSymbols && Hdr.DbgSymFldNr!=0){
    _Stl->Console.PrintLine("[ Debug symbols: Fields ]");
    Headings=String("Index<delim>Name<delim>TypIndex<delim>Offset").Replace("<delim>",Delimiter);
    Lines.Reset();
    for(i=0;i<Hdr.DbgSymFldNr;i++){ 
      Lines.Add((ToString(i)+"<delim>"
      +String((char *)AuxDbgSym.Fld[i].Name)+"<delim>"
      +ToString(AuxDbgSym.Fld[i].TypIndex)+"<delim>"
      +ToString(AuxDbgSym.Fld[i].Offset))
      .Replace("<delim>",Delimiter));
    }
    _Stl->Console.PrintTable(Headings,Lines,Delimiter);
    _Stl->Console.PrintLine("");
  }

  //Debug symbols functions
  if(Hdr.DebugSymbols && Hdr.DbgSymFunNr!=0){
    _Stl->Console.PrintLine("[ Debug symbols: Functions ]");
    Headings=String("Index<delim>Kind<delim>Name<delim>ModIndex<delim>BegAddress<delim>EndAddress<delim>TypIndex<delim>IsVoid<delim>ParmNr<delim>ParmLow<delim>ParmHigh").Replace("<delim>",Delimiter);
    Lines.Reset();
    for(i=0;i<Hdr.DbgSymFunNr;i++){ 
      Lines.Add((ToString(i)+"<delim>"
      +ToString(AuxDbgSym.Fun[i].Kind)+"<delim>"
      +String((char *)AuxDbgSym.Fun[i].Name)+"<delim>"
      +ToString(AuxDbgSym.Fun[i].ModIndex)+"<delim>"
      +HEXFORMAT(AuxDbgSym.Fun[i].BegAddress)+"<delim>"
      +HEXFORMAT(AuxDbgSym.Fun[i].EndAddress)+"<delim>"
      +ToString(AuxDbgSym.Fun[i].TypIndex)+"<delim>"
      +ToString(AuxDbgSym.Fun[i].IsVoid)+"<delim>"
      +ToString(AuxDbgSym.Fun[i].ParmNr)+"<delim>"
      +ToString(AuxDbgSym.Fun[i].ParmLow)+"<delim>"
      +ToString(AuxDbgSym.Fun[i].ParmHigh))
      .Replace("<delim>",Delimiter));
    }
    _Stl->Console.PrintTable(Headings,Lines,Delimiter);
    _Stl->Console.PrintLine("");
  }

  //Debug symbols parameters
  if(Hdr.DebugSymbols && Hdr.DbgSymParNr!=0){
    _Stl->Console.PrintLine("[ Debug symbols: Parameters ]");
    Headings=String("Index<delim>Name<delim>TypIndex<delim>FunIndex<delim>IsReference").Replace("<delim>",Delimiter);
    Lines.Reset();
    for(i=0;i<Hdr.DbgSymParNr;i++){ 
      Lines.Add((ToString(i)+"<delim>"
      +String((char *)AuxDbgSym.Par[i].Name)+"<delim>"
      +ToString(AuxDbgSym.Par[i].TypIndex)+"<delim>"
      +ToString(AuxDbgSym.Par[i].FunIndex)+"<delim>"
      +ToString(AuxDbgSym.Par[i].IsReference))
      .Replace("<delim>",Delimiter));
    }
    _Stl->Console.PrintTable(Headings,Lines,Delimiter);
    _Stl->Console.PrintLine("");
  }

  //Debug symbols lines
  if(Hdr.DebugSymbols && Hdr.DbgSymLinNr!=0){
    _Stl->Console.PrintLine("[ Debug symbols: Source code lines ]");
    Headings=String("Index<delim>ModIndex<delim>BegAddress<delim>EndAddress<delim>LineNr").Replace("<delim>",Delimiter);
    Lines.Reset();
    for(i=0;i<Hdr.DbgSymLinNr;i++){ 
      Lines.Add((ToString(i)+"<delim>"
      +ToString(AuxDbgSym.Lin[i].ModIndex)+"<delim>"
      +HEXFORMAT(AuxDbgSym.Lin[i].BegAddress)+"<delim>"
      +HEXFORMAT(AuxDbgSym.Lin[i].EndAddress)+"<delim>"
      +ToString(AuxDbgSym.Lin[i].LineNr))
      .Replace("<delim>",Delimiter));
    }
    _Stl->Console.PrintTable(Headings,Lines,Delimiter);
    _Stl->Console.PrintLine("");
  }

  //Return code
  return true;

}

//Store value in global variable buffer
void Binary::StoreGlobValue(const Buffer& Value){
  _GlobBuffer+=Value;
}

//Rewind global buffer
void Binary::GlobBufferRewind(long Bytes){
  _GlobBuffer.Rewind(Bytes);
}

//Get pointer to data in glob buffer
char *Binary::GlobValuePointer(CpuAdr Address){
  return (_GlobBuffer.BuffPnt()+Address);
}

//Reset stack size to zero
void Binary::ResetStackSize(){
  _StackSize=0;
}

//Store value in stack (we only kep size)
void Binary::CumulStackSize(int Size){
  _StackSize+=Size;
}

//Get symbol debug string
String Binary::_GetLnkSymDebugStr(const LnkSymTables& Sym,LnkSymKind Kind){
  
  //Variables
  int i;
  String Text;
  String DimString;
  
  //Switch on symbol kind
  switch(Kind){
    
    //Symbol Dim
    case LnkSymKind::Dim:
      i=Sym.Dim.Length()-1;
      DimString="";
      for(int j=0;j<_MaxArrayDims;j++){ DimString+=(DimString.Length()!=0?",":"")+ToString(Sym.Dim[i].DimSize.n[j]); }
      Text="geomindex="+ToString(Sym.Dim[i].GeomIndex)+" dimsizes=("+DimString+")";
      break;
    
    //Symbol Typ
    case LnkSymKind::Typ:
      i=Sym.Typ.Length()-1;
      Text="name="+String((char *)Sym.Typ[i].Name)+" msttype="+ToString(Sym.Typ[i].MstType)+" FunIndex="+ToString(Sym.Typ[i].FunIndex)
      +" SupTypIndex="+ToString(Sym.Typ[i].SupTypIndex)+" IsTypedef="+ToString(Sym.Typ[i].IsTypedef)+" OrigTypIndex="+ToString(Sym.Typ[i].OrigTypIndex)
      +" IsSystemDef="+ToString(Sym.Typ[i].IsSystemDef)+" Length="+ToString(Sym.Typ[i].Length)+" DimNr="+ToString(Sym.Typ[i].DimNr)
      +" ElemTypIndex="+ToString(Sym.Typ[i].ElemTypIndex)+" DimIndex="+ToString(Sym.Typ[i].DimIndex)+" FieldLow="+ToString(Sym.Typ[i].FieldLow)
      +" FieldHigh="+ToString(Sym.Typ[i].FieldHigh)+" MemberLow="+ToString(Sym.Typ[i].MemberLow)+" MemberHigh="+ToString(Sym.Typ[i].MemberHigh);
      break;
    
    //Symbol Var
    case LnkSymKind::Var:
      i=Sym.Var.Length()-1;
      Text="name="+String((char *)Sym.Var[i].Name)+" FunIndex="+ToString(Sym.Var[i].FunIndex)+" TypIndex="+ToString(Sym.Var[i].TypIndex)
      +" Address="+ToString(Sym.Var[i].Address)+" IsConst="+ToString(Sym.Var[i].IsConst);
      break;
    
    //Symbol Fld
    case LnkSymKind::Fld:
      i=Sym.Fld.Length()-1;
      Text="name="+String((char *)Sym.Fld[i].Name)+" SupTypIndex="+ToString(Sym.Fld[i].SupTypIndex)+" SubScope="+ToString(Sym.Fld[i].SubScope)
      +" TypIndex="+ToString(Sym.Fld[i].TypIndex)+" Offset="+ToString(Sym.Fld[i].Offset)+" IsStatic="+ToString(Sym.Fld[i].IsStatic)+" EnumValue="+ToString(Sym.Fld[i].EnumValue);
      break;
    
    //Symbol Fun
    case LnkSymKind::Fun:
      i=Sym.Fun.Length()-1;
      Text="name="+String((char *)Sym.Fun[i].Name)+" Kind="+ToString(Sym.Fun[i].Kind)+" SupTypIndex="+ToString(Sym.Fun[i].SupTypIndex)
      +" SubScope="+ToString(Sym.Fun[i].SubScope)+" Address="+ToString(Sym.Fun[i].Address)+" TypIndex="+ToString(Sym.Fun[i].TypIndex)
      +" IsVoid="+ToString(Sym.Fun[i].IsVoid)+" IsInitializer="+ToString(Sym.Fun[i].IsInitializer)+" IsMetaMethod="+ToString(Sym.Fun[i].IsMetaMethod)
      +" ParmNr="+ToString(Sym.Fun[i].ParmNr)+" ParmLow="+ToString(Sym.Fun[i].ParmLow)+" ParmHigh="+ToString(Sym.Fun[i].ParmHigh)
      +" MstType="+ToString(Sym.Fun[i].MstType)+" MstMethod="+ToString(Sym.Fun[i].MstMethod)+" SysCallNr="+ToString(Sym.Fun[i].SysCallNr)
      +" InstCode="+ToString(Sym.Fun[i].InstCode)+" DlName="+String((char *)Sym.Fun[i].DlName)+" DlFunction="+String((char *)Sym.Fun[i].DlFunction);
      break;
    
    //Symbol Par
    case LnkSymKind::Par:
      i=Sym.Par.Length()-1;
      Text="name="+String((char *)Sym.Par[i].Name)+" TypIndex="+ToString(Sym.Par[i].TypIndex)+" IsConst="+ToString(Sym.Par[i].IsConst)
      +" IsReference="+ToString(Sym.Par[i].IsReference)+" ParmOrder="+ToString(Sym.Par[i].ParmOrder)+" FunIndex="+ToString(Sym.Par[i].FunIndex);
      break;

  }

  //Return result
  return Text;

}

//Store library dependency
int Binary::StoreDependency(const String& Module,CpuShr LibMajorVers,CpuShr LibMinorVers,CpuShr LibRevisionNr){

  //Variables
  Dependency Depen;

  //Set dependency fields
  Module.Copy((char *)Depen.Module,_MaxIdLen);
  Depen.LibMajorVers=LibMajorVers;
  Depen.LibMinorVers=LibMinorVers;
  Depen.LibRevisionNr=LibRevisionNr;

  //Add to symbol table
  _Depen.Add(Depen);

  //Debug message
  DebugMessage(DebugLevel::CmpLibrary,"Store ODEPN["+ToString(_Depen.Length()-1)+"]: module="+Module
  +" libmajorvers="+ToString(LibMajorVers)+" libminorvers="+ToString(LibMinorVers)+" librevisionnr="+ToString(LibRevisionNr));

  //Return index
  return _Depen.Length()-1;

}

//Store undefined reference
int Binary::StoreUndefReference(const String& ObjectId,CpuChr Kind,CpuAdr CodeAdr){

  //Variables
  String Module;
  String ObjectName;
  UndefinedRefer UndRef;

  //Get object name and module
  ObjectName=ObjectId.Split(ObjIdSep)[0];
  Module=ObjectId.Split(ObjIdSep)[1];

  //Set undefined reference fields
  Module.Copy((char *)UndRef.Module,_MaxIdLen);
  UndRef.Kind=Kind;
  UndRef.CodeAdr=CodeAdr;
  UndRef.ObjectName=ObjectName;

  //Add to symbol table
  _OUndRef.Add(UndRef);

  //Debug message
  DebugMessage(DebugLevel::CmpLibrary,"Store OUREF["+ToString(_OUndRef.Length()-1)+"]: module="+Module
  +" kind="+ToString(Kind)+" address="+HEXFORMAT(CodeAdr)+" objectName="+ObjectName);

  //Return index
  return _OUndRef.Length()-1;

}

//Store Dimension symbol
int Binary::StoreLnkSymDimension(ArrayIndexes DimSize,CpuAgx GeomIndex){

  //Variables
  LnkSymTables::LnkSymDimension LnkSym;

  //Set symbol fields
  LnkSym.DimSize=DimSize;
  LnkSym.GeomIndex=GeomIndex;

  //Add to symbol table
  _OLnkSymTables.Dim.Add(LnkSym);

  //Debug message
  DebugMessage(DebugLevel::CmpLnkSymbol,"Store ODIM["+ToString(_OLnkSymTables.Dim.Length()-1)+"]: "+_GetLnkSymDebugStr(_OLnkSymTables,LnkSymKind::Dim));

  //Return symbol index
  return _OLnkSymTables.Dim.Length()-1;

}

//Store Type symbol
int Binary::StoreLnkSymType(const String& Name,CpuChr MstType,CpuInt FunIndex,CpuInt SupTypIndex,CpuBol IsTypedef,CpuInt OrigTypIndex,CpuBol IsSystemDef,
                            CpuLon Length,CpuChr DimNr,CpuInt ElemTypIndex,CpuInt DimIndex,CpuInt FieldLow,CpuInt FieldHigh,CpuInt MemberLow,
                            CpuInt MemberHigh,CpuAdr MetaName,CpuAdr MetaStNames,CpuAdr MetaStTypes,const SourceInfo& SrcInfo){
  
  //Variables
  LnkSymTables::LnkSymType LnkSym;

  //Detect invalid identifiers
  if(Name.Length()>_MaxIdLen){ SrcInfo.Msg(268).Delay(Name,ToString(_MaxIdLen)); }

  //Set symbol fields
  Name.Copy((char *)LnkSym.Name,_MaxIdLen);
  LnkSym.MstType=MstType;
  LnkSym.FunIndex=FunIndex;
  LnkSym.SupTypIndex=SupTypIndex;
  LnkSym.IsTypedef=IsTypedef;
  LnkSym.OrigTypIndex=OrigTypIndex;
  LnkSym.IsSystemDef=IsSystemDef;
  LnkSym.Length=Length;
  LnkSym.DimNr=DimNr;
  LnkSym.ElemTypIndex=ElemTypIndex;
  LnkSym.DimIndex=DimIndex;
  LnkSym.FieldLow=FieldLow;
  LnkSym.FieldHigh=FieldHigh;
  LnkSym.MemberLow=MemberLow;
  LnkSym.MemberHigh=MemberHigh;
  LnkSym.MetaName=MetaName;
  LnkSym.MetaStNames=MetaStNames;
  LnkSym.MetaStTypes=MetaStTypes;
  memset(LnkSym.DlName,0,_MaxIdLen+1);
  memset(LnkSym.DlType,0,_MaxIdLen+1);

  //Add to symbol table
  _OLnkSymTables.Typ.Add(LnkSym);

  //Debug message
  DebugMessage(DebugLevel::CmpLnkSymbol,"Store OTYP["+ToString(_OLnkSymTables.Typ.Length()-1)+"]: "+_GetLnkSymDebugStr(_OLnkSymTables,LnkSymKind::Typ));

  //Return symbol index
  return _OLnkSymTables.Typ.Length()-1;

}

//Store Variable symbol
int Binary::StoreLnkSymVariable(const String& Name,CpuInt FunIndex,CpuInt TypIndex,CpuLon Address,CpuBol IsConst,CpuAdr MetaName,const SourceInfo& SrcInfo){
  
  //Variables
  LnkSymTables::LnkSymVariable LnkSym;

  //Detect invalid identifiers
  if(Name.Length()>_MaxIdLen){ SrcInfo.Msg(268).Delay(Name,ToString(_MaxIdLen)); }

  //Set symbol fields
  Name.Copy((char *)LnkSym.Name,_MaxIdLen);
  LnkSym.FunIndex=FunIndex;
  LnkSym.TypIndex=TypIndex;
  LnkSym.Address=Address;
  LnkSym.IsConst=IsConst;
  LnkSym.MetaName=MetaName;

  //Add to symbol table
  _OLnkSymTables.Var.Add(LnkSym);

  //Debug message
  DebugMessage(DebugLevel::CmpLnkSymbol,"Store OVAR["+ToString(_OLnkSymTables.Var.Length()-1)+"]: "+_GetLnkSymDebugStr(_OLnkSymTables,LnkSymKind::Var));

  //Return symbol index
  return _OLnkSymTables.Var.Length()-1;

}

//Store Field symbol
int Binary::StoreLnkSymField(const String& Name,CpuInt SupTypIndex,CpuChr SubScope,CpuInt TypIndex,CpuLon Offset,CpuChr IsStatic,CpuInt EnumValue,const SourceInfo& SrcInfo){
  
  //Variables
  LnkSymTables::LnkSymField LnkSym;

  //Detect invalid identifiers
  if(Name.Length()>_MaxIdLen){ SrcInfo.Msg(268).Delay(Name,ToString(_MaxIdLen)); }

  //Set symbol fields
  Name.Copy((char *)LnkSym.Name,_MaxIdLen);
  LnkSym.SupTypIndex=SupTypIndex;
  LnkSym.SubScope=SubScope;
  LnkSym.TypIndex=TypIndex;
  LnkSym.Offset=Offset;
  LnkSym.IsStatic=IsStatic;
  LnkSym.EnumValue=EnumValue;

  //Add to symbol table
  _OLnkSymTables.Fld.Add(LnkSym);

  //Debug message
  DebugMessage(DebugLevel::CmpLnkSymbol,"Store OFLD["+ToString(_OLnkSymTables.Fld.Length()-1)+"]: "+_GetLnkSymDebugStr(_OLnkSymTables,LnkSymKind::Fld));

  //Return symbol index
  return _OLnkSymTables.Fld.Length()-1;

}

//Store Function symbol
int Binary::StoreLnkSymFunction(CpuChr Kind,const String& Name,CpuInt SupTypIndex,CpuChr SubScope,CpuLon Address,CpuInt TypIndex,CpuBol IsVoid,CpuBol IsInitializer,
                                CpuBol IsMetaMethod,CpuInt ParmNr,CpuInt ParmLow,CpuInt ParmHigh,CpuChr MstType,CpuShr MstMethod,CpuInt SysCallNr,CpuShr InstCode,
                                const String& DlName,const String& DlFunction,const SourceInfo& SrcInfo){
  
  //Variables
  LnkSymTables::LnkSymFunction LnkSym;

  //Detect invalid identifiers
  if(Name.Length()>_MaxIdLen){ SrcInfo.Msg(268).Delay(Name,ToString(_MaxIdLen)); }
  if(DlName.Length()>_MaxIdLen){ SrcInfo.Msg(268).Delay(DlName,ToString(_MaxIdLen)); }
  if(DlFunction.Length()>_MaxIdLen){ SrcInfo.Msg(268).Delay(DlFunction,ToString(_MaxIdLen)); }

  //Set symbol fields
  Name.Copy((char *)LnkSym.Name,_MaxIdLen);
  LnkSym.Kind=Kind;
  LnkSym.SupTypIndex=SupTypIndex;
  LnkSym.SubScope=SubScope;
  LnkSym.Address=Address;
  LnkSym.TypIndex=TypIndex;
  LnkSym.IsVoid=IsVoid;
  LnkSym.IsInitializer=IsInitializer;
  LnkSym.IsMetaMethod=IsMetaMethod;
  LnkSym.ParmNr=ParmNr;
  LnkSym.ParmLow=ParmLow;
  LnkSym.ParmHigh=ParmHigh;
  LnkSym.MstType=MstType;
  LnkSym.MstMethod=MstMethod;
  LnkSym.SysCallNr=SysCallNr;
  LnkSym.InstCode=InstCode;
  DlName.Copy((char *)LnkSym.DlName,_MaxIdLen);
  DlFunction.Copy((char *)LnkSym.DlFunction,_MaxIdLen);

  //Add to symbol table
  _OLnkSymTables.Fun.Add(LnkSym);

  //Debug message
  DebugMessage(DebugLevel::CmpLnkSymbol,"Store OFUN["+ToString(_OLnkSymTables.Fun.Length()-1)+"]: "+_GetLnkSymDebugStr(_OLnkSymTables,LnkSymKind::Fun));

  //Return symbol index
  return _OLnkSymTables.Fun.Length()-1;

}

//Store Parameter symbol
int Binary::StoreLnkSymParameter(const String& Name,CpuInt TypIndex,CpuBol IsConst,CpuBol IsReference,CpuInt ParmOrder,CpuInt FunIndex,const SourceInfo& SrcInfo){
  
  //Variables
  LnkSymTables::LnkSymParameter LnkSym;

  //Detect invalid identifiers
  if(Name.Length()>_MaxIdLen){ SrcInfo.Msg(268).Delay(Name,ToString(_MaxIdLen)); }

  //Set symbol fields
  Name.Copy((char *)LnkSym.Name,_MaxIdLen);
  LnkSym.TypIndex=TypIndex;
  LnkSym.IsConst=IsConst;
  LnkSym.IsReference=IsReference;
  LnkSym.ParmOrder=ParmOrder;
  LnkSym.FunIndex=FunIndex;

  //Add to symbol table
  _OLnkSymTables.Par.Add(LnkSym);

  //Debug message
  DebugMessage(DebugLevel::CmpLnkSymbol,"Store OPAR["+ToString(_OLnkSymTables.Par.Length()-1)+"]: "+_GetLnkSymDebugStr(_OLnkSymTables,LnkSymKind::Par));

  //Return symbol index
  return _OLnkSymTables.Par.Length()-1;

}

//Update Dimension symbol
void Binary::UpdateLnkSymDimension(int LnkSymIndex,int TypIndex){
  _OLnkSymTables.Dim[LnkSymIndex].TypIndex=TypIndex;
  DebugMessage(DebugLevel::CmpLnkSymbol,"Update ODIM["+ToString(LnkSymIndex)+"]: TypIndex="+ToString(TypIndex));
}

//Update Type symbol for field
void Binary::UpdateLnkSymTypeForField(int LnkSymIndex,CpuAdr MetaStNames,CpuAdr MetaStTypes,CpuLon Length,CpuInt FieldLow,CpuInt FieldHigh){
  _OLnkSymTables.Typ[LnkSymIndex].MetaStNames=MetaStNames;
  _OLnkSymTables.Typ[LnkSymIndex].MetaStTypes=MetaStTypes;
  _OLnkSymTables.Typ[LnkSymIndex].Length=Length;
  _OLnkSymTables.Typ[LnkSymIndex].FieldLow=FieldLow;
  _OLnkSymTables.Typ[LnkSymIndex].FieldHigh=FieldHigh;
  DebugMessage(DebugLevel::CmpLnkSymbol,"Update OTYP["+ToString(LnkSymIndex)+"]: MetaStNames="+ToString(MetaStNames)+" MetaStTypes="+ToString(MetaStTypes)+" Length="+ToString(Length)+" FieldLow="+ToString(FieldLow)+" FieldHigh="+ToString(FieldHigh));
}

//Update Type symbol for function member
void Binary::UpdateLnkSymTypeForFunctionMember(int LnkSymIndex,CpuInt MemberLow,CpuInt MemberHigh){
  _OLnkSymTables.Typ[LnkSymIndex].MemberLow=MemberLow;
  _OLnkSymTables.Typ[LnkSymIndex].MemberHigh=MemberHigh;
  DebugMessage(DebugLevel::CmpLnkSymbol,"Update OTYP["+ToString(LnkSymIndex)+"]: MemberLow="+ToString(MemberLow)+" MemberHigh="+ToString(MemberHigh));
}

//Update Type symbol for dyn lib type
void Binary::UpdateLnkSymTypeForDlType(int LnkSymIndex,const String& DlName,const String& DlType){
  DlName.Copy((char *)_OLnkSymTables.Typ[LnkSymIndex].DlName,_MaxIdLen);
  DlType.Copy((char *)_OLnkSymTables.Typ[LnkSymIndex].DlType,_MaxIdLen);
  DebugMessage(DebugLevel::CmpLnkSymbol,"Update OTYP["+ToString(LnkSymIndex)+"]: DlName="+DlName+" DlType="+DlType);
}

//Update function symbol
void Binary::UpdateLnkSymFunction(int LnkSymIndex,CpuInt ParmNr,CpuInt ParmLow,CpuInt ParmHigh){
  _OLnkSymTables.Fun[LnkSymIndex].ParmNr=ParmNr;
  _OLnkSymTables.Fun[LnkSymIndex].ParmLow=ParmLow;
  _OLnkSymTables.Fun[LnkSymIndex].ParmHigh=ParmHigh;
  DebugMessage(DebugLevel::CmpLnkSymbol,"Update OFUN["+ToString(LnkSymIndex)+"]: ParmNr="+ToString(ParmNr)+" ParmLow="+ToString(ParmLow)+" ParmHigh="+ToString(ParmHigh));
}

//Update function symbol address
void Binary::UpdateLnkSymFunctionAddress(int LnkSymIndex,CpuAdr Address){
  _OLnkSymTables.Fun[LnkSymIndex].Address=Address;
  DebugMessage(DebugLevel::CmpLnkSymbol,"Update OFUN["+ToString(LnkSymIndex)+"]: Address="+ToString(Address));
}

//Delete function symbol
void Binary::DeleteLnkSymFunction(int LnkSymIndex){
  _OLnkSymTables.Fun.Delete(LnkSymIndex);
}

//Delete parameter symbol
void Binary::DeleteLnkSymParameter(int LnkSymIndex){
  _OLnkSymTables.Par.Delete(LnkSymIndex);
}

//Store Modules debug symbol table
int Binary::StoreDbgSymModule(const String& Name,const String& Path,const SourceInfo& SrcInfo){
  
  //Variables
  DbgSymModule DbgSym;

  //Detect invalid identifiers
  if(Name.Length()>_MaxIdLen){ SrcInfo.Msg(512).Print(Name,ToString(_MaxIdLen)); }
  if(Path.Length()>FILEPATHLEN){ SrcInfo.Msg(534).Print(Path,ToString(FILEPATHLEN)); }

  //Set symbol fields
  Name.Copy((char *)DbgSym.Name,_MaxIdLen);
  Path.Copy((char *)DbgSym.Path,FILEPATHLEN);

  //Add to symbol table
  _ODbgSymTables.Mod.Add(DbgSym);

  //Debug message
  DebugMessage(DebugLevel::CmpDbgSymbol,"Store DMOD["+ToString(_ODbgSymTables.Mod.Length()-1)+"]: "+System::GetDbgSymDebugStr(_ODbgSymTables.Mod[_ODbgSymTables.Mod.Length()-1]));

  //Return symbol index
  return _ODbgSymTables.Mod.Length()-1;

}

//Store Datatype debug symbol table
int Binary::StoreDbgSymType(const String& Name,CpuChr CpuType,CpuChr MstType,CpuLon Length,CpuInt FieldLow,CpuInt FieldHigh,const SourceInfo& SrcInfo){
  
  //Variables
  DbgSymType DbgSym;

  //Detect invalid identifiers
  if(Name.Length()>_MaxIdLen){ SrcInfo.Msg(268).Print(Name,ToString(_MaxIdLen)); }

  //Set symbol fields
  Name.Copy((char *)DbgSym.Name,_MaxIdLen);
  DbgSym.CpuType=CpuType;
  DbgSym.MstType=MstType;
  DbgSym.Length=Length;
  DbgSym.FieldLow=FieldLow;
  DbgSym.FieldHigh=FieldHigh;

  //Add to symbol table
  _ODbgSymTables.Typ.Add(DbgSym);

  //Debug message
  DebugMessage(DebugLevel::CmpDbgSymbol,"Store DTYP["+ToString(_ODbgSymTables.Typ.Length()-1)+"]: "+System::GetDbgSymDebugStr(_ODbgSymTables.Typ[_ODbgSymTables.Typ.Length()-1]));

  //Return symbol index
  return _ODbgSymTables.Typ.Length()-1;

}

//Store Variable debug symbol table
int Binary::StoreDbgSymVariable(const String& Name,CpuInt ModIndex,CpuBol Glob,CpuInt FunIndex,CpuInt TypIndex,CpuLon Address,CpuBol IsReference,const SourceInfo& SrcInfo){
  
  //Variables
  DbgSymVariable DbgSym;

  //Detect invalid identifiers
  if(Name.Length()>_MaxIdLen){ SrcInfo.Msg(268).Print(Name,ToString(_MaxIdLen)); }

  //Set symbol fields
  Name.Copy((char *)DbgSym.Name,_MaxIdLen);
  DbgSym.ModIndex=ModIndex;
  DbgSym.Glob=Glob;
  DbgSym.FunIndex=FunIndex;
  DbgSym.TypIndex=TypIndex;
  DbgSym.Address=Address;
  DbgSym.IsReference=IsReference;

  //Add to symbol table
  _ODbgSymTables.Var.Add(DbgSym);

  //Debug message
  DebugMessage(DebugLevel::CmpDbgSymbol,"Store DVAR["+ToString(_ODbgSymTables.Var.Length()-1)+"]: "+System::GetDbgSymDebugStr(_ODbgSymTables.Var[_ODbgSymTables.Var.Length()-1]));

  //Return symbol index
  return _ODbgSymTables.Var.Length()-1;

}

//Store Field debug symbol table
int Binary::StoreDbgSymField(const String& Name,CpuInt TypIndex,CpuLon Offset,const SourceInfo& SrcInfo){
  
  //Variables
  DbgSymField DbgSym;

  //Detect invalid identifiers
  if(Name.Length()>_MaxIdLen){ SrcInfo.Msg(268).Print(Name,ToString(_MaxIdLen)); }

  //Set symbol fields
  Name.Copy((char *)DbgSym.Name,_MaxIdLen);
  DbgSym.TypIndex=TypIndex;
  DbgSym.Offset=Offset;

  //Add to symbol table
  _ODbgSymTables.Fld.Add(DbgSym);

  //Debug message
  DebugMessage(DebugLevel::CmpDbgSymbol,"Store DFIE["+ToString(_ODbgSymTables.Fld.Length()-1)+"]: "+System::GetDbgSymDebugStr(_ODbgSymTables.Fld[_ODbgSymTables.Fld.Length()-1]));

  //Return symbol index
  return _ODbgSymTables.Fld.Length()-1;

}
 
//Store Function debug symbol table
int Binary::StoreDbgSymFunction(CpuChr Kind,const String& Name,CpuInt ModIndex,CpuLon BegAddress,CpuLon EndAddress,CpuInt TypIndex,CpuBol IsVoid,CpuInt ParmNr,CpuInt ParmLow,CpuInt ParmHigh,const SourceInfo& SrcInfo){
  
  //Variables
  DbgSymFunction DbgSym;

  //Detect invalid identifiers
  if(Name.Length()>_MaxIdLen){ SrcInfo.Msg(268).Print(Name,ToString(_MaxIdLen)); }

  //Set symbol fields
  Name.Copy((char *)DbgSym.Name,_MaxIdLen);
  DbgSym.ModIndex=ModIndex;
  DbgSym.Kind=Kind;
  DbgSym.BegAddress=BegAddress;
  DbgSym.EndAddress=EndAddress;
  DbgSym.TypIndex=TypIndex;
  DbgSym.IsVoid=IsVoid;
  DbgSym.ParmNr=ParmNr;
  DbgSym.ParmLow=ParmLow;
  DbgSym.ParmHigh=ParmHigh;

  //Add to symbol table
  _ODbgSymTables.Fun.Add(DbgSym);

  //Debug message
  DebugMessage(DebugLevel::CmpDbgSymbol,"Store DFUN["+ToString(_ODbgSymTables.Fun.Length()-1)+"]: "+System::GetDbgSymDebugStr(_ODbgSymTables.Fun[_ODbgSymTables.Fun.Length()-1]));

  //Return symbol index
  return _ODbgSymTables.Fun.Length()-1;

}

//Store Parameter debug symbol table
int Binary::StoreDbgSymParameter(const String& Name,CpuInt TypIndex,CpuInt FunIndex,CpuBol IsReference,const SourceInfo& SrcInfo){
  
  //Variables
  DbgSymParameter DbgSym;

  //Detect invalid identifiers
  if(Name.Length()>_MaxIdLen){ SrcInfo.Msg(268).Print(Name,ToString(_MaxIdLen)); }

  //Set symbol fields
  Name.Copy((char *)DbgSym.Name,_MaxIdLen);
  DbgSym.TypIndex=TypIndex;
  DbgSym.FunIndex=FunIndex;
  DbgSym.IsReference=IsReference;

  //Add to symbol table
  _ODbgSymTables.Par.Add(DbgSym);

  //Debug message
  DebugMessage(DebugLevel::CmpDbgSymbol,"Store DPAR["+ToString(_ODbgSymTables.Par.Length()-1)+"]: "+System::GetDbgSymDebugStr(_ODbgSymTables.Par[_ODbgSymTables.Par.Length()-1]));

  //Return symbol index
  return _ODbgSymTables.Par.Length()-1;

}

//Store Source lines debug symbol
int Binary::StoreDbgSymLine(CpuInt ModIndex,CpuLon BegAddress,CpuLon EndAddress,CpuInt LineNr){
  
  //Variables
  DbgSymLine DbgSym;

  //Set symbol fields
  DbgSym.ModIndex=ModIndex;
  DbgSym.BegAddress=BegAddress;
  DbgSym.EndAddress=EndAddress;
  DbgSym.LineNr=LineNr;

  //Add to symbol table
  _ODbgSymTables.Lin.Add(DbgSym);

  //Debug message
  DebugMessage(DebugLevel::CmpDbgSymbol,"Store DLIN["+ToString(_ODbgSymTables.Lin.Length()-1)+"]: "+System::GetDbgSymDebugStr(_ODbgSymTables.Lin[_ODbgSymTables.Lin.Length()-1]));

  //Return symbol index
  return _ODbgSymTables.Lin.Length()-1;

}

//Update Type symbol for field
void Binary::UpdateDbgSymTypeForField(int DbgSymIndex,CpuLon Length,CpuInt FieldLow,CpuInt FieldHigh){
  _ODbgSymTables.Typ[DbgSymIndex].Length=Length;
  _ODbgSymTables.Typ[DbgSymIndex].FieldLow=FieldLow;
  _ODbgSymTables.Typ[DbgSymIndex].FieldHigh=FieldHigh;
  DebugMessage(DebugLevel::CmpDbgSymbol,"Update DTYP["+ToString(DbgSymIndex)+"]: Length="+ToString(Length)+" FieldLow="+ToString(FieldLow)+" FieldHigh="+ToString(FieldHigh));
}

//Update function symbol
void Binary::UpdateDbgSymFunction(int DbgSymIndex,CpuInt ParmNr,CpuInt ParmLow,CpuInt ParmHigh){
  _ODbgSymTables.Fun[DbgSymIndex].ParmNr=ParmNr;
  _ODbgSymTables.Fun[DbgSymIndex].ParmLow=ParmLow;
  _ODbgSymTables.Fun[DbgSymIndex].ParmHigh=ParmHigh;
  DebugMessage(DebugLevel::CmpDbgSymbol,"Update DFUN["+ToString(DbgSymIndex)+"]: ParmNr="+ToString(ParmNr)+" ParmLow="+ToString(ParmLow)+" ParmHigh="+ToString(ParmHigh));
}

//Update function symbol begining address
void Binary::UpdateDbgSymFunctionBegAddress(int DbgSymIndex,CpuAdr BegAddress){
  _ODbgSymTables.Fun[DbgSymIndex].BegAddress=BegAddress;
  DebugMessage(DebugLevel::CmpDbgSymbol,"Update DFUN["+ToString(DbgSymIndex)+"]: BegAddress="+ToString(BegAddress));
}

//Update function symbol end address
void Binary::UpdateDbgSymFunctionEndAddress(int DbgSymIndex,CpuAdr EndAddress){
  _ODbgSymTables.Fun[DbgSymIndex].EndAddress=EndAddress;
  DebugMessage(DebugLevel::CmpDbgSymbol,"Update DFUN["+ToString(DbgSymIndex)+"]: EndAddress="+ToString(EndAddress));
}

//Delete function symbol
void Binary::DeleteDbgSymFunction(int DbgSymIndex){
  _ODbgSymTables.Fun.Delete(DbgSymIndex);
}

//Delete parameter symbol
void Binary::DeleteDbgSymParameter(int DbgSymIndex){
  _ODbgSymTables.Par.Delete(DbgSymIndex);
}

//Get Glob Geometry Index
CpuAgx Binary::GetGlobGeometryIndex(){
  _GlobGeometryNr++;
  return _GlobGeometryNr-1;
}

//Get Locl Geometry Index
CpuAgx Binary::GetLoclGeometryIndex(){
  _LoclGeometryNr++;
  return _LoclGeometryNr-1;
}

//Reset Locl Geometry Index
void Binary::ResetLoclGeometryIndex(){
  _LoclGeometryNr=0;
}

//Store dynamic library call and get call id
int Binary::StoreDlCall(const String& DlName,const String& DlFunction){

  //Variables
  int i;
  int CallId;

  //Check if already stored
  CallId=-1;
  for(i=0;i<_DlCall.Length();i++){
    if(String((char *)_DlCall[i].DlName)==DlName && String((char *)_DlCall[i].DlFunction)==DlFunction){
      CallId=i;
      break;
    }
  }
  if(CallId!=-1){ return CallId; }

  //Store new call id if not found
  _DlCall.Add((DlCallDef){"",""});
  CallId=_DlCall.Length()-1;
  DlName.Copy((char *)_DlCall[CallId].DlName,_MaxIdLen);
  DlFunction.Copy((char *)_DlCall[CallId].DlFunction,_MaxIdLen);
  return CallId;

}

//Store string value in global block buffer
void Binary::StoreStrBlockValue(const Buffer& Value){
  BlockDef Blk;
  Blk.ArrIndex=-1;
  Blk.Buff=Value;
  _Block.Add(Blk);
}

//Store value in global array block buffer
void Binary::StoreArrBlockValue(const Buffer& Value,ArrayDynDef ArrDynDef){
  _ArrDynDef.Add(ArrDynDef);
  BlockDef Blk;
  Blk.ArrIndex=_ArrDynDef.Length()-1;
  Blk.Buff=Value;
  _Block.Add(Blk);
}
    
//Store fixed array geometry
void Binary::StoreArrFixDef(ArrayFixDef ArrFixDef){
  _ArrFixDef.Add(ArrFixDef);
}
    
//Store jump destination
void Binary::StoreJumpDestination(const String& Label,int ScopeDepth,CpuAdr DestAdr){
  if(Label.Length()!=0){
    _DestAdr.Add((DestAddress){ScopeDepth,Label,DestAdr});
    _DestAdr2.Add((DestAddress2){ScopeDepth,Label,DestAdr});
    DebugMessage(DebugLevel::CmpJump,"Stored jump destination: scopedepth="+ToString(ScopeDepth)+" label="+Label+" address="+HEXFORMAT(DestAdr));
  }
}

//Store jump origin
void Binary::StoreJumpOrigin(const String& Label,int ScopeDepth,CpuAdr CodeAdr,CpuAdr InstAdr,const String& FileName,int LineNr){
  _OrigAdr.Add((OrigAddress){ScopeDepth,Label,CodeAdr,InstAdr,FileName,LineNr});
    DebugMessage(DebugLevel::CmpJump,"Stored jump origin: scopedepth="+ToString(ScopeDepth)+" label="+Label
    +" codeadr="+HEXFORMAT(CodeAdr)+" instadr="+HEXFORMAT(InstAdr));
}

//Solve all jump addresses
bool Binary::SolveJumpAddresses(int ScopeDepth,const String& ScopeName){

  //Variables
  int i;
  int Index;
  bool Error;
  bool Found;
  CpuAdr RelAdr;

  //Debug message
  DebugMessage(DebugLevel::CmpJump,"Solving jump labels for scope depth "+ToString(ScopeDepth)+"...");

  //Resolve all jump addresses
  Error=false;
  for(i=0;i<_OrigAdr.Length();i++){
    if(_OrigAdr[i].ScopeDepth==ScopeDepth){
      if((Index=_DestAdr.Search(_OrigAdr[i].OrigLabel))==-1){
        SysMessage(100,_OrigAdr[i].FileName,_OrigAdr[i].LineNr).Print(_OrigAdr[i].OrigLabel);
        Error=true;
        continue;
      }
      RelAdr=_DestAdr[Index].DestAdr-_OrigAdr[i].InstAdr;
      CodeBufferModify(_OrigAdr[i].CodeAdr,RelAdr);
      DebugMessage(DebugLevel::CmpJump,"Solved jump: scopedepth="+ToString(ScopeDepth)
      +" codeadr="+HEXFORMAT(_OrigAdr[i].CodeAdr)+" instadr="+HEXFORMAT(_OrigAdr[i].InstAdr)
      +" destadr="+HEXFORMAT(_DestAdr[Index].DestAdr)+" relative="+HEXFORMAT(RelAdr)
      +" label="+_OrigAdr[i].OrigLabel);
    }
  }

  //Emit on assembler file resolution of function addresses for scope
  Found=false;
  for(i=0;i<_DestAdr.Length();i++){
    if(_DestAdr[i].ScopeDepth==ScopeDepth){ 
      if(!Found){
        AsmOutNewLine(AsmSection::Foot);
        AsmOutCommentLine(AsmSection::Foot,"Jump addresses on scope "+ScopeName+" (depth="+ToString(ScopeDepth)+"):",false);
        Found=true;
      }
      AsmOutCommentLine(AsmSection::Foot,"{"+HEXFORMAT(_DestAdr[i].DestAdr)+"} = "+_AsmJumpAddrTemplate.Replace("name",_DestAdr[i].DestLabel),false);
    }
  }

  //Purge origin addresses table
  do{
    Found=false;
    for(i=0;i<_OrigAdr.Length();i++){
      if(_OrigAdr[i].ScopeDepth==ScopeDepth){ 
        DebugMessage(DebugLevel::CmpJump,"Purge jump origin "+_OrigAdr[i].OrigLabel);
        _OrigAdr.Delete(i); 
        Found=true; 
        break; 
      }
    }
  }while(Found==true);

  //Purge destination addresses table
  do{
    Found=false;
    for(i=0;i<_DestAdr.Length();i++){
      if(_DestAdr[i].ScopeDepth==ScopeDepth){ 
        DebugMessage(DebugLevel::CmpJump,"Purge jump destination "+_DestAdr[i].DestLabel);
        _DestAdr.Delete(i); 
        Found=true; 
        break; 
      }
    }
  }while(Found==true);

  //Purge second destination addresses table
  do{
    Found=false;
    for(i=0;i<_DestAdr2.Length();i++){
      if(_DestAdr2[i].ScopeDepth==ScopeDepth){ 
        DebugMessage(DebugLevel::CmpJump,"Purge jump aux destination "+_DestAdr2[i].DestLabel);
        _DestAdr2.Delete(i); 
        Found=true; 
        break; 
      }
    }
  }while(Found==true);

  //Return code
  if(Error){ 
    return false; 
  }
  else{
    return true;
  }

}

//Store function address
void Binary::StoreFunctionAddress(const String& FunId,const String& FullName,int ScopeDepth,CpuAdr Address){
  _FunAddr.Add((FunAddress){ToString(ScopeDepth)+":"+FunId,FullName,ScopeDepth,Address});
  DebugMessage(DebugLevel::CmpFwdCall,"Stored function address: id="+FunId+" fullname="+FullName+" address="+
  HEXFORMAT(Address)+" scopedepth="+ToString(ScopeDepth));
}

//Store forward function call
void Binary::StoreForwardFunCall(const String& FunId,const String& FullName,int ScopeDepth,CpuAdr CodeAdr,const String& FileName,int LineNr,int ColNr){
  _FunCall.Add((FunFwdCall){FunId,FullName,ScopeDepth,CodeAdr,FileName,LineNr,ColNr,SysMsgDispatcher::GetSourceLine()});
  DebugMessage(DebugLevel::CmpFwdCall,"Stored forward call: id="+FunId+" fullname="+FullName+" at="+
  HEXFORMAT(CodeAdr)+" scopedepth="+ToString(ScopeDepth));
}

//Set function address in code buffer
void Binary::SetFunctionAddress(CpuAdr CodeAdr,CpuAdr FunAdr){
  CodeBufferModify(CodeAdr,FunAdr);
  DebugMessage(DebugLevel::CmpFwdCall,"Function address is set in code: codeadr="+HEXFORMAT(CodeAdr)+" funadr="+HEXFORMAT(FunAdr));
}

//Solve forward function calls
bool Binary::SolveForwardCalls(int ScopeDepth,const String& ScopeName){

  //Variables
  int i;
  int Index;
  bool Error;
  bool Found;
  String SearchName;

  //Debug message
  DebugMessage(DebugLevel::CmpFwdCall,"Solving forward calls for scope depth "+ToString(ScopeDepth)+"...");

  //List tables
  for(i=0;i<_FunAddr.Length();i++){
    DebugMessage(DebugLevel::CmpFwdCall,"Addr: ScopeDepth="+ToString(_FunAddr[i].ScopeDepth)+" Function="+_FunAddr[i].FullName);
  }
  for(i=0;i<_FunCall.Length();i++){
    DebugMessage(DebugLevel::CmpFwdCall,"Call: ScopeDepth="+ToString(_FunCall[i].ScopeDepth)+" Function="+_FunCall[i].FullName);
  }

  //Loop over all forward calls for the function belonging to top scope
  Error=false;
  for(i=0;i<_FunCall.Length();i++){
    if(_FunCall[i].ScopeDepth>=ScopeDepth){
      SearchName=ToString(_FunCall[i].ScopeDepth)+":"+_FunCall[i].Id;
      if((Index=_FunAddr.Search(SearchName))==-1){
        DebugMessage(DebugLevel::CmpFwdCall,"Unable to find function address for searchname="+SearchName);
        SysMessage(184,_FunCall[i].FileName,_FunCall[i].LineNr,_FunCall[i].ColNr,_FunCall[i].SourceLine).Print((_FunCall[i].FullName.Contains(ObjIdSep)?_FunCall[i].FullName.Split(ObjIdSep)[0]:_FunCall[i].FullName));
        Error=true;
      }
      else{
        DebugMessage(DebugLevel::CmpFwdCall,"Solved forward call to "+_FunCall[i].FullName);
        SetFunctionAddress(_FunCall[i].CodeAdr,_FunAddr[Index].Address);
      }
    }
  }

  //On error print to debug function addresstable
  if(Error){
    DebugMessage(DebugLevel::CmpFwdCall,"Function address table in scope "+ToString(ScopeDepth)+":");
    for(i=0;i<_FunAddr.Length();i++){
      if(_FunAddr[i].ScopeDepth>=ScopeDepth){
        DebugMessage(DebugLevel::CmpFwdCall,"searchname="+_FunAddr[i].SearchName+", fullname="+_FunAddr[i].FullName+", address="+HEXFORMAT(_FunAddr[i].Address));
      }
    }
  }

  //Emit on assembler file resolution of function addresses for scope
  Found=false;
  for(i=0;i<_FunAddr.Length();i++){
    if(_FunAddr[i].ScopeDepth>=ScopeDepth){ 
      if(!Found){
        AsmOutNewLine(AsmSection::Foot);
        AsmOutSeparator(AsmSection::Foot);
        AsmOutCommentLine(AsmSection::Foot,"Function addresses on scope "+ScopeName+" (depth="+ToString(ScopeDepth)+"):",false);
        AsmOutSeparator(AsmSection::Foot);
        Found=true;
      }
      AsmOutCommentLine(AsmSection::Foot,"{"+HEXFORMAT(_FunAddr[i].Address)+"} = "+_FunAddr[i].FullName,false);
    }
  }

  //Purge function addresses table
  //We do not purge functions containning system name space (init routines and delayed init routines since they are called from superinit routine at end)
  do{
    Found=false;
    for(i=0;i<_FunAddr.Length();i++){
      if(_FunAddr[i].ScopeDepth>=ScopeDepth && !_FunAddr[i].FullName.Contains(SYSTEM_NAMESPACE)){ 
        DebugMessage(DebugLevel::CmpFwdCall,"Purge function address "+_FunAddr[i].FullName);
        _FunAddr.Delete(i); 
        Found=true; 
        break; 
      }
    }
  }while(Found==true);

  //Purge forward function call table
  do{
    Found=false;
    for(i=0;i<_FunCall.Length();i++){
      if(_FunCall[i].ScopeDepth>=ScopeDepth){ 
        DebugMessage(DebugLevel::CmpFwdCall,"Purge function forward call "+_FunCall[i].FullName);
        _FunCall.Delete(i); 
        Found=true; 
        break; 
      }
    }
  }while(Found==true);

  //Return code
  return (Error?false:true);

}

//Solve litteral value variables
bool Binary::SolveLitValueVars(bool DebugSymbols,int DbgSymModIndex,int DbgSymFunIndex){

  //Addresses of resolved variables
  struct LitNumVarSolved{
    CpuAdr LitVarAddr;//Litteral value variable adress
    String VarName;   //Litteral value variable name
    const String& SortKey() const { return VarName; }
  };
  SortedArray<LitNumVarSolved,const String&> SolvedVars;

  //Variables
  int i,j;
  int VarIndex;
  bool ReUsed;
  bool Found;
  int DbgSymTypIndex;
  String LitVarName;
  CpuAdr LitVarAddr;
  AsmArg LitVarArg;
  AsmArg LitValArg;
  String ReplId;
  String LitVarAsmName;
  String AsmLineBefore;
  String AsmLineAfter;

  //Debug message
  DebugMessage(DebugLevel::CmpLitValRepl,"Solving litteral value variables ...");

  //Commet in assembler buffer for litteral value variable initialization
  if(_ReplLitValues.Length()!=0){ 
    AsmOutNewLine(AsmSection::Init); 
    AsmOutCommentLine(AsmSection::Init,"Initialize litteral value variables",true); 
  }

  //Resolve all litteral value variables
  for(i=0;i<_ReplLitValues.Length();i++){
    
    //Get litteral value variable name
    LitVarName=_AsmLitValVarTemplate.Replace("<type>",CpuDataTypeCharId(_ReplLitValues[i].LitValueType).Lower()).Replace("<value>",_ReplLitValues[i].LitValueHex);
    if(_ReplLitValues[i].Glob){ LitVarAsmName=_AsmGlobVarTemplate.Replace("name",LitVarName); }
    else{                       LitVarAsmName=_AsmLoclVarTemplate.Replace("name",LitVarName); }

    //Check it if we have already a variable for that value
    if((VarIndex=SolvedVars.Search(LitVarName))!=-1){
      LitVarAddr=SolvedVars[VarIndex].LitVarAddr;
      ReUsed=true;
    }

    //If not, create new variable address
    else{

      //Global scopes (initialization routine)
      if(_ReplLitValues[i].Glob){
        LitVarAddr=_GlobBuffer.Length();
        _GlobBuffer+=Buffer(&_ReplLitValues[i].LitValueBytes[0],_ReplLitValues[i].LitValueLen);
        SolvedVars.Add((LitNumVarSolved){LitVarAddr,LitVarName});
        AsmOutVarDecl(AsmSection::DLit,true,false,false,false,false,LitVarName,_ReplLitValues[i].LitValueType,
        _ReplLitValues[i].LitValueLen,LitVarAddr,"",false,false,"",_ReplLitValues[i].LitValueHex+"h");
        ReUsed=false;
      }

      //Local scopes
      else{
        LitVarAddr=_StackSize;
        _StackSize+=_ReplLitValues[i].LitValueLen;
        SolvedVars.Add((LitNumVarSolved){LitVarAddr,LitVarName});
        AsmOutVarDecl(AsmSection::DLit,false,false,false,false,false,LitVarName,_ReplLitValues[i].LitValueType,
        _ReplLitValues[i].LitValueLen,LitVarAddr,"",false,false,"",_ReplLitValues[i].LitValueHex+"h");
        ReUsed=false;
      }

      //Store debug symbol for created variable
      if(DebugSymbols){

        //Find data type in debug symbols
        DbgSymTypIndex=-1;
        for(j=0;j<_ODbgSymTables.Typ.Length();j++){
          if((CpuDataType)_ODbgSymTables.Typ[j].CpuType==_ReplLitValues[i].LitValueType){
            DbgSymTypIndex=j;
            break;
          }
        }

        //Store debug symbol
        StoreDbgSymVariable(LitVarName,(CpuInt)DbgSymModIndex,(_ReplLitValues[i].Glob?1:0),(CpuInt)DbgSymFunIndex,(CpuInt)DbgSymTypIndex,LitVarAddr,false,_ReplLitValues[i].SrcInfo);
      
      }

    }

    //Solve variable address in code
    CodeBufferModify(_ReplLitValues[i].CodeAdr,LitVarAddr);
    DebugMessage(DebugLevel::CmpLitValRepl,"Resolved literal value variable in code: at="+HEXFORMAT(_ReplLitValues[i].CodeAdr)+" name="+LitVarName+" address="+HEXFORMAT(LitVarAddr)
    +" type="+CpuDataTypeShortName(_ReplLitValues[i].LitValueType)+" length="+ToString(_ReplLitValues[i].LitValueLen)+" hexvalue="+_ReplLitValues[i].LitValueHex);
    
    //Solve variable in assembler buffer
    if(_AsmEnabled){
      Found=false;
      ReplId=_ReplLitValues[i].ReplId;
      for(j=0;j<_AsmBody.Length();j++){
        if(_AsmBody[j].Line.Contains(ReplId)){
          #ifdef __DEV__
          String AsmLineBefore=_AsmBody[j].Line.Trim();
          while(AsmLineBefore.Contains("  ")){ AsmLineBefore=AsmLineBefore.Replace("  "," "); }
          #endif
          String Part1=_AsmBody[j].Line.Split(";")[0].Replace(LitVarName,LitVarAsmName).TrimRight();
          String Part2=_AsmBody[j].Line.Split(";")[1].Replace(LitVarName,HEXVALUE(LitVarAddr)).Replace(ReplId,"").Trim();
          _AsmBody[j].Line=Part1+String(_HexIndentation-1-Part1.Length(),' ')+";"+Part2;
          #ifdef __DEV__
          String AsmLineAfter=_AsmBody[j].Line.Trim();
          while(AsmLineAfter.Contains("  ")){ AsmLineAfter=AsmLineAfter.Replace("  "," "); }
          #endif
          DebugMessage(DebugLevel::CmpLitValRepl,"Resolved literal value variable in asm: replid="+ReplId+" index="+ToString(j)
          +" litvaluetext="+_ReplLitValues[i].LitValueText+" litvalueasmvar="+LitVarAsmName+" before=\""+AsmLineBefore+"\" after=\""+AsmLineAfter+"\"");
          Found=true;
          break;
        }
      }
      if(!Found){
        _ReplLitValues[i].SrcInfo.Msg(575,_ReplLitValues[i].SourceLine).Print(ReplId,_ReplLitValues[i].LitValueText,LitVarAsmName);
        return false;
      }
    }

    //Emit initialization instruction
    if(!ReUsed){

      //Set lit value variable as asm argument
      LitVarArg.IsNull=false;
      LitVarArg.IsError=false;
      LitVarArg.AdrMode=CpuAdrMode::Address;
      LitVarArg.IsUndefined=false;
      LitVarArg.Value.Adr=LitVarAddr; 
      LitVarArg.Glob=_ReplLitValues[i].Glob;
      LitVarArg.Name=LitVarName;
      LitVarArg.ObjectId="";
      LitVarArg.Type=_ReplLitValues[i].LitValueType;
      LitVarArg.ScopeDepth=-1;

      //Set lit value as asm argument
      LitValArg.IsNull=false;
      LitValArg.IsError=false;
      LitValArg.IsUndefined=false;
      LitValArg.AdrMode=CpuAdrMode::LitValue;
      LitValArg.Name="";
      LitValArg.ObjectId="";
      LitValArg.Type=_ReplLitValues[i].LitValueType; 
      LitValArg.ScopeDepth=-1;

      //Add initialization instructions
      if(     _ReplLitValues[i].LitValueType==CpuDataType::Boolean){ 
        MemCpy(&LitValArg.Value.Bol,_ReplLitValues[i].LitValueBytes,_ReplLitValues[i].LitValueLen); 
        if(!AsmWriteInitCode(CpuInstCode::LOADb,false,LitVarArg,LitValArg)){ return false; } 
      }
      else if(_ReplLitValues[i].LitValueType==CpuDataType::Char   ){ 
        MemCpy(&LitValArg.Value.Chr,_ReplLitValues[i].LitValueBytes,_ReplLitValues[i].LitValueLen); 
        if(!AsmWriteInitCode(CpuInstCode::LOADc,false,LitVarArg,LitValArg)){ return false; } 
      }
      else if(_ReplLitValues[i].LitValueType==CpuDataType::Short  ){ 
        MemCpy(&LitValArg.Value.Shr,_ReplLitValues[i].LitValueBytes,_ReplLitValues[i].LitValueLen); 
        if(!AsmWriteInitCode(CpuInstCode::LOADw,false,LitVarArg,LitValArg)){ return false; } 
      }
      else if(_ReplLitValues[i].LitValueType==CpuDataType::Integer){ 
        MemCpy(&LitValArg.Value.Int,_ReplLitValues[i].LitValueBytes,_ReplLitValues[i].LitValueLen); 
        if(!AsmWriteInitCode(CpuInstCode::LOADi,false,LitVarArg,LitValArg)){ return false; } 
      }
      else if(_ReplLitValues[i].LitValueType==CpuDataType::Long   ){ 
        MemCpy(&LitValArg.Value.Lon,_ReplLitValues[i].LitValueBytes,_ReplLitValues[i].LitValueLen); 
        if(!AsmWriteInitCode(CpuInstCode::LOADl,false,LitVarArg,LitValArg)){ return false; } 
      }
      else if(_ReplLitValues[i].LitValueType==CpuDataType::Float  ){ 
        MemCpy(&LitValArg.Value.Flo,_ReplLitValues[i].LitValueBytes,_ReplLitValues[i].LitValueLen); 
        if(!AsmWriteInitCode(CpuInstCode::LOADf,false,LitVarArg,LitValArg)){ return false; } 
      }

    }

  }

  //Reset lit values table
  _ReplLitValues.Reset();

  //Return success
  return true;

}

//Merge init and code buffers
void Binary::MergeCodeBuffers(CpuAdr FromAdr){

  //Variables
  int i;
  int HexAdrLen;
  String HexAddress;
  CpuAdr CodeAddress;
  CpuAdr CodeAddress2;
  String NewAddress;
  String OldAddress;

  //Do nothing if init buffer is empty
  if(_InitBuffer.Length()==0){
    DebugMessage(DebugLevel::CmpMergeCode,"Merging of code buffers skipped as init buffer is empty");
    return;
  }

  //Message
  DebugMessage(DebugLevel::CmpMergeCode,"Merging code buffers from address "+HEXFORMAT(FromAdr));

  //Merge init and code buffers
  _CodeBuffer=_CodeBuffer.Part(0,FromAdr)+_InitBuffer+_CodeBuffer.Part(FromAdr,_CodeBuffer.Length()-FromAdr);

  //Modify code addresses in assembler init buffer
  HexAdrLen=2*GetArchitecture()/8;
  for(i=0;i<_AsmInit.Length();i++){
    HexAddress=_AsmInit[i].Line.Mid(_HexIndentation+1,HexAdrLen);
    if(HexAddress.Length()!=0){
      CodeAddress=(CpuAdr)HexAddress.ToLongLong(16);
      CodeAddress2=FromAdr+CodeAddress;
      NewAddress="{"+HEXFORMAT(CodeAddress2)+"}";
      OldAddress="{"+HexAddress+"h}";
      _AsmInit[i].Line=_AsmInit[i].Line.Replace(OldAddress,NewAddress);
      DebugMessage(DebugLevel::CmpMergeCode,"Assembler init address changed from "+OldAddress+" to "+NewAddress+" on line "+ToString(i)+" ("+_AsmInit[i].Line.Trim()+")");
    }
  }

  //Modify code addresses in assembler body buffer
  for(i=0;i<_AsmBody.Length();i++){
    HexAddress=_AsmBody[i].Line.Mid(_HexIndentation+1,HexAdrLen);
    if(HexAddress.Length()!=0){
      CodeAddress=(CpuAdr)HexAddress.ToLongLong(16);
      if(CodeAddress>=FromAdr){
        CodeAddress2=CodeAddress+_InitBuffer.Length();
        NewAddress="{"+HEXFORMAT(CodeAddress2)+"}";
        OldAddress="{"+HexAddress+"h}";
        _AsmBody[i].Line=_AsmBody[i].Line.Replace(OldAddress,NewAddress);
        DebugMessage(DebugLevel::CmpMergeCode,"Assembler body address changed from "+OldAddress+" to "+NewAddress+" on line "+ToString(i)+" ("+_AsmBody[i].Line.Trim()+")");
      }
    }
  }

  //Modify all affected binary objects on which code addresses appear
  //(we must modify code asdresses by adding the length of init buffer)
  for(i=0;i<_OUndRef.Length();i++){ 
    if(_OUndRef[i].CodeAdr>=FromAdr){ 
      #ifdef __DEV__
      CpuAdr OldAdr=_OUndRef[i].CodeAdr;
      CpuAdr NewAdr=_OUndRef[i].CodeAdr+_InitBuffer.Length();
      #endif
      DebugMessage(DebugLevel::CmpMergeCode,"Changed address in output undefined references table for object "+_OUndRef[i].ObjectName+" from "+HEXFORMAT(OldAdr)+" to "+HEXFORMAT(NewAdr));
      _OUndRef[i].CodeAdr+=_InitBuffer.Length(); 
    } 
  }
  for(i=0;i<_DestAdr.Length();i++){ 
    if(_DestAdr[i].DestAdr>FromAdr){ 
      #ifdef __DEV__
      CpuAdr OldAdr=_DestAdr[i].DestAdr;
      CpuAdr NewAdr=_DestAdr[i].DestAdr+_InitBuffer.Length();
      #endif
      DebugMessage(DebugLevel::CmpMergeCode,"Changed address in jump destination address table for label "+_DestAdr[i].DestLabel+" from "+HEXFORMAT(OldAdr)+" to "+HEXFORMAT(NewAdr));
      _DestAdr[i].DestAdr+=_InitBuffer.Length(); 
    } 
  }
  for(i=0;i<_DestAdr2.Length();i++){ 
    if(_DestAdr2[i].DestAdr>FromAdr){ 
      #ifdef __DEV__
      CpuAdr OldAdr=_DestAdr2[i].DestAdr;
      CpuAdr NewAdr=_DestAdr2[i].DestAdr+_InitBuffer.Length();
      #endif
      DebugMessage(DebugLevel::CmpMergeCode,"Changed address in jump destination address table 2 for label "+_DestAdr2[i].DestLabel+" from "+HEXFORMAT(OldAdr)+" to "+HEXFORMAT(NewAdr));
      _DestAdr2[i].DestAdr+=_InitBuffer.Length(); 
    } 
  }
  for(i=0;i<_OrigAdr.Length();i++){ 
    if(_OrigAdr[i].CodeAdr>FromAdr){ 
      #ifdef __DEV__
      CpuAdr OldAdr=_OrigAdr[i].CodeAdr;
      CpuAdr NewAdr=_OrigAdr[i].CodeAdr+_InitBuffer.Length();
      #endif
      DebugMessage(DebugLevel::CmpMergeCode,"Changed code address in jump origin table for label "+_OrigAdr[i].OrigLabel+" from "+HEXFORMAT(OldAdr)+" to "+HEXFORMAT(NewAdr));
      _OrigAdr[i].CodeAdr+=_InitBuffer.Length(); 
    } 
    if(_OrigAdr[i].InstAdr>FromAdr){ 
      #ifdef __DEV__
      CpuAdr OldAdr=_OrigAdr[i].InstAdr;
      CpuAdr NewAdr=_OrigAdr[i].InstAdr+_InitBuffer.Length();
      #endif
      DebugMessage(DebugLevel::CmpMergeCode,"Changed instruction address in jump origin table for label "+_OrigAdr[i].OrigLabel+" from "+HEXFORMAT(OldAdr)+" to "+HEXFORMAT(NewAdr));
      _OrigAdr[i].InstAdr+=_InitBuffer.Length(); 
    } 
  }
  for(i=0;i<_FunAddr.Length();i++){ 
    if(_FunAddr[i].Address>FromAdr){ 
      #ifdef __DEV__
      CpuAdr OldAdr=_FunAddr[i].Address;
      CpuAdr NewAdr=_FunAddr[i].Address+_InitBuffer.Length();
      #endif
      DebugMessage(DebugLevel::CmpMergeCode,"Changed address in function address table for function "+_FunAddr[i].FullName+" from "+HEXFORMAT(OldAdr)+" to "+HEXFORMAT(NewAdr));
      _FunAddr[i].Address+=_InitBuffer.Length(); 
    } 
  }
  for(i=0;i<_FunCall.Length();i++){ 
    if(_FunCall[i].CodeAdr>=FromAdr){ 
      #ifdef __DEV__
      CpuAdr OldAdr=_FunCall[i].CodeAdr;
      CpuAdr NewAdr=_FunCall[i].CodeAdr+_InitBuffer.Length();
      #endif
      DebugMessage(DebugLevel::CmpMergeCode,"Changed address in function call table for code function "+_FunCall[i].FullName+" from "+HEXFORMAT(OldAdr)+" to "+HEXFORMAT(NewAdr));
      _FunCall[i].CodeAdr+=_InitBuffer.Length(); 
    } 
  }
  for(i=0;i<_OLnkSymTables.Fun.Length();i++){ 
    if(_OLnkSymTables.Fun[i].Address>FromAdr){ 
      #ifdef __DEV__
      CpuAdr OldAdr=_OLnkSymTables.Fun[i].Address;
      CpuAdr NewAdr=_OLnkSymTables.Fun[i].Address+_InitBuffer.Length();
      #endif
      DebugMessage(DebugLevel::CmpMergeCode,"Changed address in output linker symbol function table for function "+String((char *)&_OLnkSymTables.Fun[i].Name[0])+" from "+HEXFORMAT(OldAdr)+" to "+HEXFORMAT(NewAdr));
      _OLnkSymTables.Fun[i].Address+=_InitBuffer.Length(); 
    } 
  }
  for(i=0;i<_ODbgSymTables.Fun.Length();i++){ 
    if(_ODbgSymTables.Fun[i].BegAddress> FromAdr){ 
      #ifdef __DEV__
      CpuAdr OldAdr=_ODbgSymTables.Fun[i].BegAddress;
      CpuAdr NewAdr=_ODbgSymTables.Fun[i].BegAddress+_InitBuffer.Length();
      #endif
      DebugMessage(DebugLevel::CmpMergeCode,"Changed beginning address in output debug symbol function table for function "+String((char *)&_ODbgSymTables.Fun[i].Name[0])+" from "+HEXFORMAT(OldAdr)+" to "+HEXFORMAT(NewAdr));
      _ODbgSymTables.Fun[i].BegAddress+=_InitBuffer.Length(); 
    } 
    if(_ODbgSymTables.Fun[i].EndAddress>=FromAdr){ 
      #ifdef __DEV__
      CpuAdr OldAdr=_ODbgSymTables.Fun[i].EndAddress;
      CpuAdr NewAdr=_ODbgSymTables.Fun[i].EndAddress+_InitBuffer.Length();
      #endif
      DebugMessage(DebugLevel::CmpMergeCode,"Changed ending address in output debug symbol function table for function "+String((char *)&_ODbgSymTables.Fun[i].Name[0])+" from "+HEXFORMAT(OldAdr)+" to "+HEXFORMAT(NewAdr));
      _ODbgSymTables.Fun[i].EndAddress+=_InitBuffer.Length(); 
    } 
  }
  for(i=0;i<_ODbgSymTables.Lin.Length();i++){ 
    if(_ODbgSymTables.Lin[i].BegAddress>=FromAdr){ 
      #ifdef __DEV__
      CpuAdr OldAdr=_ODbgSymTables.Lin[i].BegAddress;
      CpuAdr NewAdr=_ODbgSymTables.Lin[i].BegAddress+_InitBuffer.Length();
      #endif
      DebugMessage(DebugLevel::CmpMergeCode,"Changed beginning address in output debug symbol lines table from "+HEXFORMAT(OldAdr)+" to "+HEXFORMAT(NewAdr));
      _ODbgSymTables.Lin[i].BegAddress+=_InitBuffer.Length(); 
    } 
    if(_ODbgSymTables.Lin[i].EndAddress>=FromAdr){ 
      #ifdef __DEV__
      CpuAdr OldAdr=_ODbgSymTables.Lin[i].EndAddress;
      CpuAdr NewAdr=_ODbgSymTables.Lin[i].EndAddress+_InitBuffer.Length();
      #endif
      DebugMessage(DebugLevel::CmpMergeCode,"Changed ending address in output debug symbol lines table from "+HEXFORMAT(OldAdr)+" to "+HEXFORMAT(NewAdr));
      _ODbgSymTables.Lin[i].EndAddress+=_InitBuffer.Length(); 
    } 
  }
  for(i=0;i<_RelocTable.Length();i++){
    switch(_RelocTable[i].Type){
      case RelocType::FunAddress: 
        if(_RelocTable[i].LocAdr>=FromAdr){ 
          #ifdef __DEV__
          CpuAdr OldAdr=_RelocTable[i].LocAdr;
          CpuAdr NewAdr=_RelocTable[i].LocAdr+_InitBuffer.Length();
          #endif
          DebugMessage(DebugLevel::CmpMergeCode,"Changed relocation table entry for "+RelocationTypeText(_RelocTable[i].Type)+" from "+HEXFORMAT(OldAdr)+" to "+HEXFORMAT(NewAdr));
          _RelocTable[i].LocAdr+=_InitBuffer.Length(); 
        } 
        break;
      case RelocType::GloAddress: 
        if(_RelocTable[i].LocAdr>=FromAdr){ 
          #ifdef __DEV__
          CpuAdr OldAdr=_RelocTable[i].LocAdr;
          CpuAdr NewAdr=_RelocTable[i].LocAdr+_InitBuffer.Length();
          #endif
          DebugMessage(DebugLevel::CmpMergeCode,"Changed relocation table entry for "+RelocationTypeText(_RelocTable[i].Type)+" from "+HEXFORMAT(OldAdr)+" to "+HEXFORMAT(NewAdr));
          _RelocTable[i].LocAdr+=_InitBuffer.Length(); 
        } 
        break;
      case RelocType::ArrGeom:    
        if(_RelocTable[i].LocAdr>=FromAdr){ 
          #ifdef __DEV__
          CpuAdr OldAdr=_RelocTable[i].LocAdr;
          CpuAdr NewAdr=_RelocTable[i].LocAdr+_InitBuffer.Length();
          #endif
          DebugMessage(DebugLevel::CmpMergeCode,"Changed relocation table entry for "+RelocationTypeText(_RelocTable[i].Type)+" from "+HEXFORMAT(OldAdr)+" to "+HEXFORMAT(NewAdr));
          _RelocTable[i].LocAdr+=_InitBuffer.Length(); 
        } 
        break;
      case RelocType::DlCall:     
        if(_RelocTable[i].LocAdr>=FromAdr){ 
          #ifdef __DEV__
          CpuAdr OldAdr=_RelocTable[i].LocAdr;
          CpuAdr NewAdr=_RelocTable[i].LocAdr+_InitBuffer.Length();
          #endif
          DebugMessage(DebugLevel::CmpMergeCode,"Changed relocation table entry for "+RelocationTypeText(_RelocTable[i].Type)+" from "+HEXFORMAT(OldAdr)+" to "+HEXFORMAT(NewAdr));
          _RelocTable[i].LocAdr+=_InitBuffer.Length(); 
        } 
        break;
      case RelocType::GloBlock:   
        break;
      case RelocType::BlkBlock:   
        break;
    }
  }
  
  //Reset init code buffers
  _InitBuffer.Reset();

}

//Get current global buffer address
CpuAdr Binary::CurrentGlobAddress(){
  return _GlobBuffer.Length();
}

//Get current stack address
CpuAdr Binary::CurrentStackAddress(){
  return _StackSize;
}

//Get current code address
CpuAdr Binary::CurrentCodeAddress(){
  return _CodeBuffer.Length();
}

//Get current global string block address
CpuMbl Binary::CurrentBlockAddress(){
  return _Block.Length();
}

//Current Dl call id
CpuInt Binary::CurrentDlCallId(){
  return _DlCall.Length();
}

//Get init buffer size
CpuLon Binary::GetInitBufferSize(){
  return _InitBuffer.Length();
}

//Emit function stack instruction
bool Binary::SetFunctionStackInst(){
  
  //Set stack size as zero
  AsmArg Arg;
  Arg.IsNull=false;
  Arg.IsError=false;
  Arg.AdrMode=CpuAdrMode::LitValue;
  Arg.Name="";
  Arg.Type=CpuDataType::Long; 
  Arg.Value.Lon=0;
  Arg.ScopeDepth=-1;

  //Out instruction without echo
  AsmOutNewLine(AsmSection::Init);
  AsmOutCommentLine(AsmSection::Init,"Reserve function stack size",true);
  if(!AsmWriteInitCodeStrArg(CpuInstCode::STACK,Arg)){ return false; }

  //Return code
  return true;
}

//Set function stack size
void Binary::SetFunctionStackSize(CpuAdr FunAddress){
  
  //Debug message
  #ifdef __DEV__
  CpuWrd CodeAddress=3;
  #endif
  DebugMessage(DebugLevel::CmpCodeBlock,"Set stack size "+ToString(_StackSize)+" at code address "+PTRFORMAT(CodeAddress));

  //Set stack size in init code buffer
  CpuWrd StackSize=_StackSize;
  const char *p=reinterpret_cast<const char *>(&StackSize);
  for(int i=0;i<(int)sizeof(CpuWrd);i++){ _InitBuffer[sizeof(CpuIcd)+sizeof(CpuWrd)+i]=p[i]; }

  //Set stack size argument
  AsmArg Arg;
  Arg.IsNull=false;
  Arg.IsError=false;
  Arg.AdrMode=CpuAdrMode::LitValue;
  Arg.Name="";
  Arg.Type=CpuDataType::Long; 
  Arg.Value.Lon=_StackSize;
  Arg.ScopeDepth=-1;

  //Set stack size in assembler file
  for(int i=0;i<_AsmInit.Length();i++){
    if(_AsmInit[i].NestId==_AsmNestId){
      if(_AsmInit[i].Line.Search(_Inst[(int)CpuInstCode::STACK].Mnemonic)!=-1){
        String TextArg="%TEXTARG0%";
        String HexArg="%HEXARG0%";
        _AsmInit[i].Line=_AsmInit[i].Line.Replace(TextArg,Arg.ToText()+(TextArg.Length()-Arg.ToText().Length()>0?String(TextArg.Length()-Arg.ToText().Length(),' '):""));
        _AsmInit[i].Line=_AsmInit[i].Line.Replace(HexArg,String(Arg.ToHex().CharPnt()));
        DebugMessage(DebugLevel::CmpCodeBlock,"Set stack size "+ToString(_StackSize)+" in assembler buffer at nest id "+ToString(_AsmNestId));
      }
    }
  }

}

//Convert instruction code into buffer
Buffer Binary::InstCodeToBuffer(CpuInstCode InstCode){
  CpuIcd Code;
  Code=(CpuIcd)InstCode;
  return Buffer((char *)&Code,sizeof(Code));
}

//Do replacements of instruction codes for specific cases that support litteral values directly
void Binary::_AsmInstCodeReplacements(CpuInstCode &InstCode,int ArgNr,const AsmArg *Arg){

  
  //Replace MV* instructions by LOAD* when source argument is a litteral value
  if(ArgNr==2 && Arg[1].AdrMode==CpuAdrMode::LitValue 
  &&(InstCode==CpuInstCode::MVb || InstCode==CpuInstCode::MVc || InstCode==CpuInstCode::MVw 
  || InstCode==CpuInstCode::MVi || InstCode==CpuInstCode::MVl || InstCode==CpuInstCode::MVf)){
    switch(InstCode){
      case CpuInstCode::MVb: InstCode=CpuInstCode::LOADb; break;
      case CpuInstCode::MVc: InstCode=CpuInstCode::LOADc; break;
      case CpuInstCode::MVw: InstCode=CpuInstCode::LOADw; break;
      case CpuInstCode::MVi: InstCode=CpuInstCode::LOADi; break;
      case CpuInstCode::MVl: InstCode=CpuInstCode::LOADl; break;
      case CpuInstCode::MVf: InstCode=CpuInstCode::LOADf; break;
      default: break;
    }
  }

}

//Annotate replacement of litteral value arguments by variables if addressing mode does not support
bool Binary::_AsmLitValueReplacements(CpuInstCode InstCode,int ArgNr,AsmArg *Arg,String& ReplIds,int *ReplIndexes){

  //Variables
  int i;
  LitNumValueVars ReplLitValue;

  //Init replacement ids
  ReplIds="";

  //Init reported replacement index
  for(i=0;i<_MaxInstructionArgs;i++){ ReplIndexes[i]=-1; }

  //Loop over arguments
  for(i=0;i<ArgNr;i++){
    
    //Take only arguments as defined by instruction since number of arguments is checked afterwards in _AsmCheck()
    if(i<=_Inst[(int)InstCode].ArgNr-1){
      
      //Detect unsuppported litteral value in argument
      if(Arg[i].AdrMode==CpuAdrMode::LitValue
      && (_Inst[(int)InstCode].AdrMode[i]!=CpuAdrMode::LitValue)
      && Arg[i].Type>=CpuDataType::Boolean && Arg[i].Type<=CpuDataType::Float){
        
        //Check argument supports cpu address mode
        if(_Inst[(int)InstCode].AdrMode[i]!=CpuAdrMode::Address){
          SysMessage(561,_FileName,_LineNr,_ColNr).Print(ToString(i+1),_Inst[(int)InstCode].Mnemonic);
          return false;
        }

        //Annotate litteral value replacement
        ReplLitValue.ScopeDepth=Arg[i].ScopeDepth;
        ReplLitValue.CodeAdr=0;
        ReplLitValue.Glob=_GlobReplLitValues;
        ReplLitValue.LitValueLen=Arg[i].BinLength();
        ReplLitValue.LitValueHex=Arg[i].ToHex();
        ReplLitValue.LitValueText=Arg[i].ToText();
        ReplLitValue.LitValueType=Arg[i].Type;
        ReplLitValue.ReplId=_AsmReplIdBeg+ToString(_ReplLitValues.Length())+_AsmReplIdEnd;
        ReplLitValue.SrcInfo=SourceInfo(_FileName,_LineNr,_ColNr);
        ReplLitValue.SourceLine=SysMsgDispatcher::GetSourceLine();
        MemCpy(ReplLitValue.LitValueBytes,Arg[i].ToCode().BuffPnt(),ReplLitValue.LitValueLen);
        _ReplLitValues.Add(ReplLitValue);
        
        //Change argment by unresolved local variable
        Arg[i].AdrMode=CpuAdrMode::Address;
        Arg[i].Glob=_GlobReplLitValues;
        Arg[i].Value.Adr=0;
        Arg[i].Name=_AsmLitValVarTemplate.Replace("<type>",CpuDataTypeCharId(ReplLitValue.LitValueType).Lower()).Replace("<value>",ReplLitValue.LitValueHex);

        //Keep replacement id
        ReplIds+=ReplLitValue.ReplId;

        //Report replacement index
        ReplIndexes[i]=_ReplLitValues.Length()-1;
        
        //Debug message
        DebugMessage(DebugLevel::CmpLitValRepl,"Stored literal value replacement: scopedepth="+ToString(ReplLitValue.ScopeDepth)
        +" type="+CpuDataTypeShortName(ReplLitValue.LitValueType)+" length="+ToString(ReplLitValue.LitValueLen)
        +" hexvalue="+ReplLitValue.LitValueHex+" textvalue="+ReplLitValue.LitValueText+" replids="+ReplIds+" index="+ToString(_ReplLitValues.Length()-1));

      }

    }
    
  } 

  //Return success
  return true;

}

//Assembler code checks
bool Binary::_AsmChecks(CpuInstCode InstCode,int ArgNr,const AsmArg *Arg){
  
  //Variables
  int i;
  CpuDataType ArgType;
  String ArgOrdinal;

  //Check number of arguments is correct
  if(ArgNr!=_Inst[(int)InstCode].ArgNr){
    SysMessage(62,_FileName,_LineNr,_ColNr).Print(_Inst[(int)InstCode].Mnemonic,ToString(ArgNr),ToString(_Inst[(int)InstCode].ArgNr));
    return false;
  }

  //Check data types and addresing modes of arguments
  for(i=0;i<ArgNr;i++){
    switch(i){
      case 0: ArgOrdinal="first"; break;
      case 1: ArgOrdinal="second"; break;
      case 2: ArgOrdinal="third"; break;
      case 3: ArgOrdinal="fourth"; break;
    }
    ArgType=(_Inst[(int)InstCode].Type[i]==(CpuDataType)-1?WordCpuDataType():_Inst[(int)InstCode].Type[i]);
    if(Arg[i].Type!=ArgType && ArgType!=CpuDataType::Undefined){
      SysMessage(63,_FileName,_LineNr,_ColNr).Print(CpuDataTypeName(Arg[i].Type),ArgOrdinal,_Inst[(int)InstCode].Mnemonic,CpuDataTypeName(ArgType));
      return false;
    }
    if(_Inst[(int)InstCode].AdrMode[i]!=Arg[i].AdrMode){
      SysMessage(64,_FileName,_LineNr,_ColNr).Print(CpuAdrModeName(Arg[i].AdrMode),ArgOrdinal,_Inst[(int)InstCode].Mnemonic);
      return false;
    }
  } 
  
  //Return success
  return true;

}

//Emit decoder programming instructions
bool Binary::_AsmProgDecoders(AsmSection Section,CpuInstCode InstCode,int ArgNr,AsmArg *Arg){

  //Instruction length table
  //(calculates instruction to be sent in advance)
  struct DecoderInstruction{
    CpuInstCode InstCode;
    int ArgIndex;
    int Length;
  };
  Array<DecoderInstruction> DecInst;

  //Variables
  int i;
  CpuShr InsOffset;
  CpuShr ArgOffset;
  CpuInstCode Code;
  
  //Calculate instructions that will be sent
  for(i=0;i<ArgNr;i++){
    if(Arg[i].AdrMode==CpuAdrMode::Address && Arg[i].Glob){
      switch(i){
        case 0: Code=CpuInstCode::DAGV1; DecInst.Add((DecoderInstruction){Code,i,_Inst[(int)Code].Length}); break;
        case 1: Code=CpuInstCode::DAGV2; DecInst.Add((DecoderInstruction){Code,i,_Inst[(int)Code].Length}); break;
        case 2: Code=CpuInstCode::DAGV3; DecInst.Add((DecoderInstruction){Code,i,_Inst[(int)Code].Length}); break;
        case 3: Code=CpuInstCode::DAGV4; DecInst.Add((DecoderInstruction){Code,i,_Inst[(int)Code].Length}); break;
      }
    }
    else if(Arg[i].AdrMode==CpuAdrMode::Indirection){
      if(Arg[i].Glob){
        switch(i){
          case 0: Code=CpuInstCode::DAGI1; DecInst.Add((DecoderInstruction){Code,i,_Inst[(int)Code].Length}); break;
          case 1: Code=CpuInstCode::DAGI2; DecInst.Add((DecoderInstruction){Code,i,_Inst[(int)Code].Length}); break;
          case 2: Code=CpuInstCode::DAGI3; DecInst.Add((DecoderInstruction){Code,i,_Inst[(int)Code].Length}); break;
          case 3: Code=CpuInstCode::DAGI4; DecInst.Add((DecoderInstruction){Code,i,_Inst[(int)Code].Length}); break;
        }
      }
      else{
        switch(i){
          case 0: Code=CpuInstCode::DALI1; DecInst.Add((DecoderInstruction){Code,i,_Inst[(int)Code].Length}); break;
          case 1: Code=CpuInstCode::DALI2; DecInst.Add((DecoderInstruction){Code,i,_Inst[(int)Code].Length}); break;
          case 2: Code=CpuInstCode::DALI3; DecInst.Add((DecoderInstruction){Code,i,_Inst[(int)Code].Length}); break;
          case 3: Code=CpuInstCode::DALI4; DecInst.Add((DecoderInstruction){Code,i,_Inst[(int)Code].Length}); break;
        }
      }
      Arg[i].AdrMode=CpuAdrMode::Address;
    }
  }  

  //Send decoder programming instructions
  for(i=0;i<DecInst.Length();i++){
    InsOffset=_Inst[(int)InstCode].Length;
    ArgOffset=_Inst[(int)InstCode].Offset[DecInst[i].ArgIndex];
    for(int j=i;j<DecInst.Length();j++){ 
      InsOffset+=_Inst[(int)DecInst[j].InstCode].Length;
      ArgOffset+=DecInst[j].Length; 
    }
    if(Section==AsmSection::Init){ 
      if(!AsmWriteInitCode(DecInst[i].InstCode,false,AsmLitShr(InsOffset),AsmLitShr(ArgOffset))){ return false; } 
    }
    else{ 
      if(!AsmWriteCode(DecInst[i].InstCode,AsmLitShr(InsOffset),AsmLitShr(ArgOffset))){ return false; } 
    }
  }

  //Return code
  return true;

}

//Code output routine
bool Binary::AsmWriteCode(CpuMetaInst Meta,const AsmArg& Arg1,const AsmArg& Arg2,const AsmArg& Arg3,const AsmArg& Arg4){
  return AsmWriteCode(Meta,1,Arg1,Arg2,Arg3,Arg4);
}

//Code output routine
bool Binary::AsmWriteCode(CpuMetaInst Meta,int DriverArg,const AsmArg& Arg1,const AsmArg& Arg2,const AsmArg& Arg3,const AsmArg& Arg4){
  
  //Variables
  CpuInstCode InstCode;

  //Derive instruction code from meta instruction code and driver argument type
  switch(DriverArg){
    case 1: 
      if((int)Arg1.Type>6){ SysMessage(367,_FileName,_LineNr,_ColNr).Print(ToString(DriverArg),_Meta[(int)Meta].Mnemonic,ToString((int)Arg1.Type)); return false; }
      InstCode=_Meta[(int)Meta].Inst[(int)Arg1.Type]; 
      break;
    case 2: 
      if((int)Arg2.Type>6){ SysMessage(367,_FileName,_LineNr,_ColNr).Print(ToString(DriverArg),_Meta[(int)Meta].Mnemonic,ToString((int)Arg2.Type)); return false; }
      InstCode=_Meta[(int)Meta].Inst[(int)Arg2.Type]; 
      break;
    case 3: 
      if((int)Arg3.Type>6){ SysMessage(367,_FileName,_LineNr,_ColNr).Print(ToString(DriverArg),_Meta[(int)Meta].Mnemonic,ToString((int)Arg3.Type)); return false; }
      InstCode=_Meta[(int)Meta].Inst[(int)Arg3.Type]; 
      break;
    case 4: 
      if((int)Arg4.Type>6){ SysMessage(367,_FileName,_LineNr,_ColNr).Print(ToString(DriverArg),_Meta[(int)Meta].Mnemonic,ToString((int)Arg4.Type)); return false; }
      InstCode=_Meta[(int)Meta].Inst[(int)Arg4.Type]; 
      break;
    default:
      SysMessage(224,_FileName,_LineNr,_ColNr).Print(ToString(DriverArg),_Meta[(int)Meta].Mnemonic);
      return false;
  }
  if(InstCode==(CpuInstCode)-1){
    SysMessage(71,_FileName,_LineNr,_ColNr).Print(_Meta[(int)Meta].Mnemonic,CpuDataTypeName(Arg1.Type));
    return false;
  }

  //Call base function
  return _AsmWriteCode(InstCode,AsmSection::Body,false,Arg1,Arg2,Arg3,Arg4);

}  

//Code output routine
bool Binary::AsmWriteCode(CpuInstCode InstCode,const AsmArg& Arg1,const AsmArg& Arg2,const AsmArg& Arg3,const AsmArg& Arg4){
  return _AsmWriteCode(InstCode,AsmSection::Body,false,Arg1,Arg2,Arg3,Arg4);
}

//Code output routine with assembler section
bool Binary::AsmWriteCode(CpuInstCode InstCode,AsmSection Section,const AsmArg& Arg1,const AsmArg& Arg2,const AsmArg& Arg3,const AsmArg& Arg4){
  return _AsmWriteCode(InstCode,Section,false,Arg1,Arg2,Arg3,Arg4);
}

//Code output with string arguments (to be replaced afterwards)
bool Binary::AsmWriteInitCodeStrArg(CpuInstCode InstCode,const AsmArg& Arg1,const AsmArg& Arg2,const AsmArg& Arg3,const AsmArg& Arg4){
  return AsmWriteInitCode(InstCode,true,Arg1,Arg2,Arg3,Arg4);
}

//Code output routine
bool Binary::_AsmWriteCode(CpuInstCode InstCode,AsmSection Section,bool StrArg,const AsmArg& Arg1,const AsmArg& Arg2,const AsmArg& Arg3,const AsmArg& Arg4){
  
  //Variables
  int i;
  int Index,From,To;
  CpuAdr CodeAddress;
  CpuAdr InstAdr;
  AsmArg Arg[_MaxInstructionArgs];
  AsmArg OrigArg[_MaxInstructionArgs];
  int ReplIndexes[_MaxInstructionArgs];
  int ArgNr=0;
  Array<String> Labels;
  String ReplIds;

  //Get argument array
  if(!Arg1.IsNull){ Arg[0]=Arg1; ArgNr++; if(!Arg2.IsNull){ Arg[1]=Arg2; ArgNr++; if(!Arg3.IsNull){ Arg[2]=Arg3; ArgNr++; if(!Arg4.IsNull){ Arg[3]=Arg4; ArgNr++; }}}}

  //Do replacements of instruction codes for specific cases that support litteral values directly
  _AsmInstCodeReplacements(InstCode,ArgNr,Arg);
  
  //Replacement and annotation of literal values by variables when instruction does not support them
  if(!_AsmLitValueReplacements(InstCode,ArgNr,Arg,ReplIds,ReplIndexes)){ return false; }
  for(i=0;i<ArgNr;i++){ OrigArg[i]=Arg[i]; }

  //Send decoder programming instructions
  if(!_AsmProgDecoders(Section,InstCode,ArgNr,Arg)){ return false; }

  //Calculate code positions of replaced litterals (cannot be done earlier as depends on emitted decoder instructions in between)
  if(ReplIds.Length()!=0){
    CodeAddress=_CodeBuffer.Length()+sizeof(CpuIcd)+sizeof(CpuWrd);
    for(i=0;i<ArgNr;i++){ 
      if(ReplIndexes[i]!=-1){ 
        _ReplLitValues[ReplIndexes[i]].CodeAdr=CodeAddress; 
        DebugMessage(DebugLevel::CmpLitValRepl,"Set code address for literal value replacement: codeadr="+HEXVALUE(_ReplLitValues[ReplIndexes[i]].CodeAdr)
        +" type="+CpuDataTypeShortName(_ReplLitValues[ReplIndexes[i]].LitValueType)+" length="+ToString(_ReplLitValues[ReplIndexes[i]].LitValueLen)
        +" hexvalue="+_ReplLitValues[ReplIndexes[i]].LitValueHex+" textvalue="+_ReplLitValues[ReplIndexes[i]].LitValueText+" replid="+_ReplLitValues[ReplIndexes[i]].ReplId);
      } 
      CodeAddress+=Arg[i].BinLength(); 
    }
  }
  
  //Instruction checks
  if(!_AsmChecks(InstCode,ArgNr,Arg)){ return false; }
  
  //Get instruction address
  InstAdr=_CodeBuffer.Length();

  //Get labels for current code address
  //This way of doing this search is optimal
  //First we search an occurence, then we get first occurence, then last occurence
  if((Index=_DestAdr2.Search(InstAdr))!=-1){
    while(_DestAdr2[Index].DestAdr==InstAdr && Index>0){ Index--; } 
    if(_DestAdr2[Index].DestAdr!=InstAdr){ Index++; }
    From=Index;
    while(_DestAdr2[Index].DestAdr==InstAdr && Index<_DestAdr2.Length()-1){ Index++; }
    if(_DestAdr2[Index].DestAdr!=InstAdr){ Index--; }
    To=Index;
    for(i=From;i<=To;i++){ Labels.Add(_DestAdr2[i].DestLabel); }
  }

  //Output instruction
  //(jump origins and forward function calls are recorded here)
  _CodeBuffer+=InstCodeToBuffer(InstCode);
  _CodeBuffer+=Buffer(sizeof(CpuWrd),0);
  for(i=0;i<ArgNr;i++){ 
    if(Arg[i].Type==CpuDataType::JumpAddr){ StoreJumpOrigin(Arg[i].Name,Arg[i].ScopeDepth,_CodeBuffer.Length(),InstAdr,_FileName,_LineNr); }
    else if(Arg[i].Type==CpuDataType::FunAddr && Arg[i].Value.Adr==0){ 
      if(Arg[i].IsUndefined){ 
        if(Arg[i].ObjectId.SearchCount(ObjIdSep)!=1){ SysMessage(442,_FileName,_LineNr,_ColNr).Print(ToString(i),_Inst[(int)InstCode].Mnemonic,Arg[i].ObjectId); return false; }
        StoreUndefReference(Arg[i].ObjectId,(CpuShr)UndefRefKind::Fun,CurrentCodeAddress());
      }
      else{
        StoreForwardFunCall(Arg[i].Name,Arg[i].ObjectId,Arg[i].ScopeDepth,_CodeBuffer.Length(),_FileName,_LineNr,_ColNr); 
      }
    }
    else if(Arg[i].Type==CpuDataType::ArrGeom && Arg[i].Value.Agx==0){ 
      if(Arg[i].IsUndefined && Arg[i].Glob){
        if(Arg[i].ObjectId.SearchCount(ObjIdSep)!=1){ SysMessage(442,_FileName,_LineNr,_ColNr).Print(ToString(i),_Inst[(int)InstCode].Mnemonic,Arg[i].ObjectId); return false; }
        StoreUndefReference(Arg[i].ObjectId,(CpuShr)UndefRefKind::Agx,CurrentCodeAddress()); 
      }
      else if(Arg[i].Glob){
        SysMessage(60,_FileName,_LineNr,_ColNr).Print(ToString(i),_Inst[(int)InstCode].Mnemonic);
        return false;
      }
    }
    else if(Arg[i].AdrMode!=CpuAdrMode::LitValue && Arg[i].Value.Adr==0){ 
      if(Arg[i].Glob && Arg[i].IsUndefined){
        if(Arg[i].ObjectId.SearchCount(ObjIdSep)!=1){ SysMessage(442,_FileName,_LineNr,_ColNr).Print(ToString(i),_Inst[(int)InstCode].Mnemonic,Arg[i].ObjectId); return false; }
        StoreUndefReference(Arg[i].ObjectId,(CpuShr)UndefRefKind::Glo,CurrentCodeAddress()); 
      }
      else if(Arg[i].Glob && Arg[i].Name.StartsWith(_AsmLitValVarPrefix)==false){
        SysMessage(120,_FileName,_LineNr,_ColNr).Print(ToString(i),_Inst[(int)InstCode].Mnemonic);
        return false;
      }
    }
    _CalcRelocations(InstCode,i,Arg[i]);
    _CodeBuffer+=Arg[i].ToCode(); 
  }

  //Output to assembler file
  _AsmOutCode(Section,InstAdr,Labels,InstCode,StrArg,OrigArg,ArgNr,ReplIds);

  //Check there are delayed error messages before exitting
  if(SysMessage().DelayCount()!=0){
    return false;
  }

  //Return code
  return true;

}

//Code output routine (for initialization buffer only)
bool Binary::AsmWriteInitCode(CpuInstCode InstCode,bool StrArg,const AsmArg& Arg1,const AsmArg& Arg2,const AsmArg& Arg3,const AsmArg& Arg4){
  
  //Variables
  int i;
  CpuAdr InstAdr;
  AsmArg Arg[_MaxInstructionArgs];
  AsmArg OrigArg[_MaxInstructionArgs];
  int ArgNr=0;
  Array<String> Labels;

  //Get argument array
  if(!Arg1.IsNull){ Arg[0]=Arg1; OrigArg[0]=Arg1; ArgNr++; 
  if(!Arg2.IsNull){ Arg[1]=Arg2; OrigArg[1]=Arg2; ArgNr++; 
  if(!Arg3.IsNull){ Arg[2]=Arg3; OrigArg[2]=Arg3; ArgNr++; 
  if(!Arg4.IsNull){ Arg[3]=Arg4; OrigArg[3]=Arg4; ArgNr++; 
  }}}}

  //Send decoder programming instructions
  if(!_AsmProgDecoders(AsmSection::Init,InstCode,ArgNr,Arg)){ return false; }
  
  //Instruction checks
  if(!_AsmChecks(InstCode,ArgNr,Arg)){ return false; }
  
  //Get instruction address
  InstAdr=_InitBuffer.Length();

  //Output instruction
  _InitBuffer+=InstCodeToBuffer(InstCode);
  _InitBuffer+=Buffer(sizeof(CpuWrd),0);
  for(i=0;i<ArgNr;i++){ 
    _InitBuffer+=Arg[i].ToCode(); 
  }

  //Output to assembler file
  _AsmOutCode(AsmSection::Init,InstAdr,Labels,InstCode,StrArg,OrigArg,ArgNr,"");

  //Check there are delayed error messages before exitting
  if(SysMessage().DelayCount()!=0){
    return false;
  }

  //Return code
  return true;

}

//Add address relocation
void Binary::AddAdrRelocation(RelocType Type,CpuAdr LocAdr,const String& Module,const String& ObjName){
  _RelocTable.Add((RelocItem){Type,0,LocAdr,{0},{0},0});
  strncpy((char *)_RelocTable[_RelocTable.Length()-1].Module,Module.CharPnt(),_MaxIdLen);   _RelocTable[_RelocTable.Length()-1].Module[_MaxIdLen]=0;
  strncpy((char *)_RelocTable[_RelocTable.Length()-1].ObjName,ObjName.CharPnt(),_MaxIdLen); _RelocTable[_RelocTable.Length()-1].ObjName[_MaxIdLen]=0;
  DebugMessage(DebugLevel::CmpRelocation,"Relocation added: Type="+RelocationTypeText(Type)+", LocAdr="+HEXFORMAT(LocAdr)+", Module="+Module+", ObjName="+ObjName+", CopyCount=0");
}

//Add block relocation
void Binary::AddBlkRelocation(RelocType Type,CpuAdr LocAdr,const String& Module,const String& ObjName){
  _RelocTable.Add((RelocItem){Type,0,LocAdr,{0},{0},0});
  strncpy((char *)_RelocTable[_RelocTable.Length()-1].Module,Module.CharPnt(),_MaxIdLen);   _RelocTable[_RelocTable.Length()-1].Module[_MaxIdLen]=0;
  strncpy((char *)_RelocTable[_RelocTable.Length()-1].ObjName,ObjName.CharPnt(),_MaxIdLen); _RelocTable[_RelocTable.Length()-1].ObjName[_MaxIdLen]=0;
  DebugMessage(DebugLevel::CmpRelocation,"Relocation added: Type="+RelocationTypeText(Type)+", LocAdr="+HEXFORMAT(LocAdr)+", Module="+Module+", ObjName="+ObjName+", CopyCount=0");
}

//Add block inside block relocation
void Binary::AddBlkBlkRelocation(RelocType Type,CpuMbl LocBlk,CpuAdr LocAdr,const String& Module,const String& ObjName){
  _RelocTable.Add((RelocItem){Type,LocBlk,LocAdr,{0},{0},0});
  strncpy((char *)_RelocTable[_RelocTable.Length()-1].Module,Module.CharPnt(),_MaxIdLen);   _RelocTable[_RelocTable.Length()-1].Module[_MaxIdLen]=0;
  strncpy((char *)_RelocTable[_RelocTable.Length()-1].ObjName,ObjName.CharPnt(),_MaxIdLen); _RelocTable[_RelocTable.Length()-1].ObjName[_MaxIdLen]=0;
  DebugMessage(DebugLevel::CmpRelocation,"Relocation added: Type="+RelocationTypeText(Type)+", LocAdr="+HEXFORMAT(LocAdr)+", LocBlk="+HEXFORMAT(LocBlk)+", Module="+Module+", ObjName="+ObjName+", CopyCount=0");
}

//Description text for relocation type
String Binary::RelocationTypeText(RelocType Type){
  String Text;
  switch(Type){
    case RelocType::FunAddress: Text="FunctionAddr"; break;
    case RelocType::GloAddress: Text="GlobVarvAddr"; break;
    case RelocType::ArrGeom   : Text="FixArrayGeom"; break;
    case RelocType::DlCall    : Text="DynLibFnCall"; break;
    case RelocType::GloBlock  : Text="BlkInsideGlo"; break;
    case RelocType::BlkBlock  : Text="BlkInsideBlk"; break;
  }
  return Text;
}

//Assembler output routine
void Binary::_AsmOutCode(AsmSection Section,CpuAdr InstAdr,Array<String>& Labels,CpuInstCode InstCode,bool StrArg,AsmArg *Arg,int ArgNr,const String& Tag){

  //Variables
  int i;
  String Line;
  Array<String> Lines;

  //Do nothing if assembler file generation is disabled
  if(!_AsmEnabled){ return; }
  
  //labels
  if(Labels.Length()==0){
    Line=String(_AsmIndentation-1,' ');
  }
  else{
    for(i=0;i<Labels.Length();i++){
      Line=("{"+Labels[i]+"}:").LJust(_AsmIndentation-1);
      if(i<Labels.Length()-1){
        _AsmOutRaw(AsmSection::Body,Line);
      }
    }
  }

  //Instruction code
  Line+=_Inst[(int)InstCode].Mnemonic;

  //Arguments
  Line+=" ";
  for(int i=0;i<ArgNr;i++){
    if(StrArg){
      Line+="%TEXTARG"+ToString(i)+"%";
    }
    else if(Arg[i].AdrMode==CpuAdrMode::Address && Arg[i].Value.Adr==0 && Arg[i].Name.StartsWith(_AsmLitValVarPrefix)){
      Line+=Arg[i].Name;
    }
    else{
      Line+=Arg[i].ToText();
    }
    if(i<ArgNr-1){ Line+=","; }
  }

  //Add hex part
  if(Line.Length()<_HexIndentation-1){ Line+=String(_HexIndentation-1-Line.Length(),' '); }
  Line+=";{"+HEXFORMAT(InstAdr)+"} ";
  Line+=String(InstCodeToBuffer(InstCode).ToHex().BuffPnt());
  Line+=" ";
  Line+=String(Buffer(sizeof(CpuWrd),0).ToHex().BuffPnt());
  for(int i=0;i<ArgNr;i++){
    Line+=" ";
    if(StrArg){
      Line+="%HEXARG"+ToString(i)+"%";
    }
    else if(Arg[i].AdrMode==CpuAdrMode::Address && Arg[i].Value.Adr==0 && Arg[i].Name.StartsWith(_AsmLitValVarPrefix)){
      Line+=Arg[i].Name;
    }
    else{
      Line+=String(Arg[i].ToHex().CharPnt());
    }
  }

  //Add tag
  if(Tag.Length()!=0){
    Line+=" "+Tag;
  }

  //Add to output
  _AsmOutRaw(Section,Line);

}

//Assembler raw output
void Binary::_AsmOutRaw(AsmSection Section,const String& OutLine){
  switch(Section){
    case AsmSection::Head: _AsmHead.Add(AssemblerLine(_AsmNestId,OutLine+"\n")); break;
    case AsmSection::Data: _AsmData.Add(AssemblerLine(_AsmNestId,OutLine+"\n")); break;
    case AsmSection::Decl: _AsmDecl.Add(AssemblerLine(_AsmNestId,OutLine+"\n")); break;
    case AsmSection::DLit: _AsmDLit.Add(AssemblerLine(_AsmNestId,OutLine+"\n")); break;
    case AsmSection::Star: _AsmStar.Add(AssemblerLine(_AsmNestId,OutLine+"\n")); break;
    case AsmSection::Temp: _AsmTemp.Add(AssemblerLine(_AsmNestId,OutLine+"\n")); break;
    case AsmSection::Init: _AsmInit.Add(AssemblerLine(_AsmNestId,OutLine+"\n")); break;
    case AsmSection::Body: _AsmBody.Add(AssemblerLine(_AsmNestId,OutLine+"\n")); break;
    case AsmSection::Foot: _AsmFoot.Add(AssemblerLine(_AsmNestId,OutLine+"\n")); break;
  }
  DebugMessage(DebugLevel::CmpAssembler,GetAsmSectionName(Section)+"("+ToString(_AsmNestId)+"): "+OutLine);
}

//PrintSourceLine
void Binary::AsmOutCommentLine(AsmSection Section,const String& Line,bool Indented){
  if(!_AsmEnabled){ return; }
  String OutLine=(Indented?String(_AsmIndentation-1,' ')+_AsmComment:_AsmComment+" ");
  OutLine+=Line;
  _AsmOutRaw(Section,OutLine);
}

//Print assembler line
void Binary::AsmOutLine(AsmSection Section,const String& Label,const String& Line){
  if(!_AsmEnabled){ return; }
  String OutLine;
  if(Label.Length()<_AsmIndentation-1){
    OutLine=Label+String(_AsmIndentation-1-Label.Length(),' ')+Line;
  }
  else{
    OutLine=Label+" "+Line;
  }
  _AsmOutRaw(Section,OutLine);
}

//Print assembler line with hex part
void Binary::AsmOutLine(AsmSection Section,const String& Label,const String& Line,const String& HexPart){
  if(!_AsmEnabled){ return; }
  String OutLine;
  if(Label.Length()<_AsmIndentation-1){
    OutLine=Label+String(_AsmIndentation-1-Label.Length(),' ')+Line;
  }
  else{
    OutLine=Label+" "+Line;
  }
  OutLine=(OutLine).LJust(_HexIndentation-1)+HexPart;
  _AsmOutRaw(Section,OutLine);
}

//Print section name
void Binary::AsmOutSectionName(AsmSection Section,String Name){
  if(!_AsmEnabled){ return; }
  String OutLine=String(".section ")+Name;
  _AsmOutRaw(Section,OutLine);
}

//Print Separator
void Binary::AsmOutSeparator(AsmSection Section){
  if(!_AsmEnabled){ return; }
  String OutLine=_AsmSeparator;
  _AsmOutRaw(Section,OutLine);
}

//PrintNewLine
void Binary::AsmOutNewLine(AsmSection Section){
  if(!_AsmEnabled){ return; }
  String OutLine="";
  _AsmOutRaw(Section,OutLine);
}

//Assembler variable declaration
void Binary::AsmOutVarDecl(AsmSection Section,bool Glob,bool IsParameter,bool IsReference,bool IsConst,bool IsStatic,const String& Name,CpuDataType Type,
                           CpuWrd Length,CpuAdr Address,const String& SourceLine,bool Import,bool Export,const String& Library,const String& InitValue){
  
  //Exit if assembler file generation is disabled
  if(!_AsmEnabled){ return; }

  //Variables
  bool GlobStore;
  String VarName;
  String DataType;
  String OutLine;
  Buffer AdrBuff;

  //Get variable name
  if(!Glob && (!IsConst || (IsConst && IsReference)) && !IsStatic){
    VarName=_AsmLoclVarTemplate.Replace("name",Name);
    GlobStore=false;
  }
  else{
    VarName=_AsmGlobVarTemplate.Replace("name",Name);
    GlobStore=true;
  }

  //Get data type name
  if(IsReference){
    DataType="REFERENCE";
  }
  else if(Type==CpuDataType::Undefined){
    DataType="BYTE["+ToString(Length)+"]";
  }
  else{
    DataType=CpuDataTypeName(Type).Upper();
  }
  if(IsConst){
    DataType="CONST "+DataType;
  }

  //Get address in hex format
  const char *p=reinterpret_cast<const char *>(&Address);
  for(int i=0;i<(int)sizeof(CpuAdr);i++){ AdrBuff+=p[i]; }

  //Get assembler line
  if(Import){
    OutLine=(VarName.LJust(_AsmIndentation-1)+"IMPORT VAR FROM \""+Library+"\" "+
    DataType).LJust(_HexIndentation-1)+
    ";Address="+(GlobStore?"["+HEXFORMAT(Address)+"]":"<"+HEXFORMAT(Address)+">");
  }
  else if(Export){
    OutLine=(VarName.LJust(_AsmIndentation-1)+"EXPORT VAR "+
    DataType).LJust(_HexIndentation-1)+
    ";Address="+(GlobStore?"["+HEXFORMAT(Address)+"]":"<"+HEXFORMAT(Address)+">");
  }
  else{
    OutLine=(VarName.LJust(_AsmIndentation-1)+(IsParameter?"PARM ":(GlobStore?"GVAR ":"VAR "))+
    DataType).LJust(_HexIndentation-1)+
    ";Address="+(GlobStore?"["+HEXFORMAT(Address)+"]":"<"+HEXFORMAT(Address)+">")+
    (InitValue.Length()!=0?" = "+InitValue:"")+
    (SourceLine.Length()!=0?" { "+SourceLine.Trim()+" }":"");
  }

  //Supress emission of variable declarations with size zero
  if(Length==0){
    DebugMessage(DebugLevel::CmpAssembler,"Supressed line: "+OutLine);
    return;
  }

  //Output to buffer
  _AsmOutRaw(Section,OutLine);
  
}

//Assembler function declaration
void Binary::AsmOutFunDecl(AsmSection Section,const String& Name,const String& Parms,bool Import,bool Export,bool Local,const String& Library,CpuAdr Address,const String &As){
  
  //Exit if assembler file generation is disabled
  if(!_AsmEnabled){ return; }

  //Variables
  String FunName;
  String OutLine;
  
  //Get function name
  FunName=_AsmFunctionTemplate.Replace("name",Name)+" ";

  //Get assembler line
  if(Import){
    OutLine=(FunName.LJust(_AsmIndentation-1)+"IMPORT FUNCTION FROM \""+Library+"\" {"+Parms+"} ")+(Address!=0?";Address="+HEXFORMAT(Address):"")+(As.Length()!=0?";"+As:"");
  }
  else if(Export){
    OutLine=(FunName.LJust(_AsmIndentation-1)+"EXPORT FUNCTION {"+Parms+"} ")+(Address!=0?";Address="+HEXFORMAT(Address):"")+(As.Length()!=0?";"+As:"");
  }
  else if(Local){
    OutLine=FunName.LJust(_AsmIndentation-1)+"LOCAL FUNCTION {"+Parms+"} "+(As.Length()!=0?"AS "+As+" ":"");
  }
  else{
    OutLine=FunName.LJust(_AsmIndentation-1)+"DECLARE {"+Parms+"} "+(As.Length()!=0?"AS "+As+" ":"");
  }
  
  //Output to buffer
  _AsmOutRaw(Section,OutLine);
  
}

//Output dynamic library call Ids
void Binary::AsmOutDlCallTable(AsmSection Section){
  if(!_AsmEnabled){ return; }
  for(int i=0;i<_DlCall.Length();i++){
    AsmOutCommentLine(Section,"["+ToString(i,"%04i")+"] -> DCALL("+String((char *)_DlCall[i].DlName)+","+String((char *)_DlCall[i].DlFunction)+")",false);
  }
}

//Assembler delete last line
void Binary::AsmDeleteLast(AsmSection Section){
  if(!_AsmEnabled){ return; }
  AssemblerLine AsmLine;
  switch(Section){
    case AsmSection::Head: AsmLine=_AsmHead.Last(); _AsmHead.Delete(_AsmHead.Length()-1); break;
    case AsmSection::Data: AsmLine=_AsmData.Last(); _AsmData.Delete(_AsmData.Length()-1); break;
    case AsmSection::Decl: AsmLine=_AsmDecl.Last(); _AsmDecl.Delete(_AsmDecl.Length()-1); break;
    case AsmSection::DLit: AsmLine=_AsmDLit.Last(); _AsmDLit.Delete(_AsmDLit.Length()-1); break;
    case AsmSection::Star: AsmLine=_AsmStar.Last(); _AsmStar.Delete(_AsmStar.Length()-1); break;
    case AsmSection::Temp: AsmLine=_AsmTemp.Last(); _AsmTemp.Delete(_AsmTemp.Length()-1); break;
    case AsmSection::Body: AsmLine=_AsmBody.Last(); _AsmBody.Delete(_AsmBody.Length()-1); break;
    case AsmSection::Init: AsmLine=_AsmInit.Last(); _AsmInit.Delete(_AsmInit.Length()-1); break;
    case AsmSection::Foot: AsmLine=_AsmFoot.Last(); _AsmFoot.Delete(_AsmFoot.Length()-1); break;
  }
  DebugMessage(DebugLevel::CmpAssembler,GetAsmSectionName(Section)+": DELETED --> "+AsmLine.Line);
}

//Assembler buffer filter
Array<String> Binary::_AsmBufferFilter(const Array<AssemblerLine>& Buff,int NestId){
  Array<String> Result;
  Result.Reset();
  for(int i=0;i<Buff.Length();i++){ if(Buff[i].NestId==NestId){ Result.Add(Buff[i].Line); } }
  return Result;
}

//Open new assembler nested
void Binary::AsmOpenNestId(){
  _AsmNestCounter++;
  _AsmNestId=_AsmNestCounter;
  _AsmNestIdStack.Push(_AsmNestId);
}

//Decrease level in assembler file lines
bool Binary::AsmCloseNestId(){
  if(_AsmNestIdStack.Length()==0){
    SysMessage(562,_FileName,_LineNr,_ColNr).Print();
    return false;
  }
  _AsmNestIdStack.Pop();
  if(_AsmNestIdStack.Length()==0){
    _AsmNestId=0;
  }
  else{
    _AsmNestId=_AsmNestIdStack.Top();
  }
  return true;
}

//Set parent neste id as current
bool Binary::AsmSetParentNestId(){
  if(_AsmNestIdStack.Length()==0){
    SysMessage(563,_FileName,_LineNr,_ColNr).Print();
    return false;
  }
  if(_AsmNestIdStack.Length()==1){
    _AsmNestId=0;
  }
  else{
    _AsmNestId=_AsmNestIdStack.Top(-1);
  }
  return true;
}

//Restore current nest id
void Binary::AsmRestoreCurrentNestId(){
  if(_AsmNestIdStack.Length()==0){
    _AsmNestId=0;
  }
  else{
    _AsmNestId=_AsmNestIdStack.Top();
  }
}

//Reset counter to assign unique nest ids
//(Called only from not nested scopes)
void Binary::AsmNestIdCounter(){
  _AsmNestCounter=0;
}

//Assmbler file flush
bool Binary::AsmFlush(){
  
  //Sorted array of nest ids
  struct SortNestIdList{
    int Id;
    int SortKey() const { return Id; }
  };
  SortedArray<SortNestIdList,int> SortNestIds;

  //Variables
  int i;
  Array<int> NestIds;
  Array<String> Buffer;
  Array<String> Buffer1;
  Array<String> Buffer2;
  Array<String> Buffer3;

  //Exit if assembler file generation is disabled
  if(!_AsmEnabled){ return true; }

  //Get all different nest ids in lines
  for(i=0;i<_AsmHead.Length();i++){ if(SortNestIds.Search(_AsmHead[i].NestId)==-1){ SortNestIds.Add((SortNestIdList){_AsmHead[i].NestId}); } }
  for(i=0;i<_AsmData.Length();i++){ if(SortNestIds.Search(_AsmData[i].NestId)==-1){ SortNestIds.Add((SortNestIdList){_AsmData[i].NestId}); } }
  for(i=0;i<_AsmDecl.Length();i++){ if(SortNestIds.Search(_AsmDecl[i].NestId)==-1){ SortNestIds.Add((SortNestIdList){_AsmDecl[i].NestId}); } }
  for(i=0;i<_AsmDLit.Length();i++){ if(SortNestIds.Search(_AsmDLit[i].NestId)==-1){ SortNestIds.Add((SortNestIdList){_AsmDLit[i].NestId}); } }
  for(i=0;i<_AsmStar.Length();i++){ if(SortNestIds.Search(_AsmStar[i].NestId)==-1){ SortNestIds.Add((SortNestIdList){_AsmStar[i].NestId}); } }
  for(i=0;i<_AsmTemp.Length();i++){ if(SortNestIds.Search(_AsmTemp[i].NestId)==-1){ SortNestIds.Add((SortNestIdList){_AsmTemp[i].NestId}); } }
  for(i=0;i<_AsmInit.Length();i++){ if(SortNestIds.Search(_AsmInit[i].NestId)==-1){ SortNestIds.Add((SortNestIdList){_AsmInit[i].NestId}); } }
  for(i=0;i<_AsmBody.Length();i++){ if(SortNestIds.Search(_AsmBody[i].NestId)==-1){ SortNestIds.Add((SortNestIdList){_AsmBody[i].NestId}); } }
  for(i=0;i<_AsmFoot.Length();i++){ if(SortNestIds.Search(_AsmFoot[i].NestId)==-1){ SortNestIds.Add((SortNestIdList){_AsmFoot[i].NestId}); } }

  //Prepare array with correct output order
  for(i=0;i<SortNestIds.Length();i++){ if(SortNestIds[i].Id!=0){ NestIds.Add(SortNestIds[i].Id); } }
  if(SortNestIds.Search(0)!=-1){
    NestIds.Add(0);
  }

  //Flush buffers id by id
  for(i=0;i<NestIds.Length();i++){

    //Head
    if(!_Stl->FileSystem.WriteArray(_AsmHnd,_AsmBufferFilter(_AsmHead,NestIds[i]))){ SysMessage(128,_FileName,_LineNr,_ColNr).Print(_Stl->LastError()); return false; }

    //Data
    Buffer=_AsmBufferFilter(_AsmData,NestIds[i]);
    if(Buffer.Length()!=0){
      if(!_Stl->FileSystem.Write(_AsmHnd,"\n.section "+GetAsmSectionName(AsmSection::Data)+"\n\n")){ SysMessage(128,_FileName,_LineNr,_ColNr).Print(_Stl->LastError()); return false; }
    }
    if(!_Stl->FileSystem.WriteArray(_AsmHnd,Buffer)){ SysMessage(128,_FileName,_LineNr,_ColNr).Print(_Stl->LastError()); return false; }

    //Decl section
    Buffer1=_AsmBufferFilter(_AsmDecl,NestIds[i]);
    Buffer2=_AsmBufferFilter(_AsmTemp,NestIds[i]);
    Buffer3=_AsmBufferFilter(_AsmDLit,NestIds[i]);
    if(Buffer1.Length()!=0 || Buffer2.Length()!=0 || Buffer3.Length()!=0){
      if(!_Stl->FileSystem.Write(_AsmHnd,"\n.section "+GetAsmSectionName(AsmSection::Decl)+"\n")){ SysMessage(128,_FileName,_LineNr,_ColNr).Print(_Stl->LastError()); return false; }
    }
    
    //Decl
    if(!_Stl->FileSystem.WriteArray(_AsmHnd,Buffer1)){ SysMessage(128,_FileName,_LineNr,_ColNr).Print(_Stl->LastError()); return false; }

    //Temp
    if(Buffer2.Length()!=0){
      if(!_Stl->FileSystem.Write(_AsmHnd,"\n"+String().LJust(_AsmIndentation-1)+_AsmComment+"Temporary variables\n")){ SysMessage(128,_FileName,_LineNr,_ColNr).Print(_Stl->LastError()); return false; }
    }
    if(!_Stl->FileSystem.WriteArray(_AsmHnd,Buffer2)){ SysMessage(128,_FileName,_LineNr,_ColNr).Print(_Stl->LastError()); return false; }

    //DLit
    if(Buffer3.Length()!=0){
      if(!_Stl->FileSystem.Write(_AsmHnd,"\n"+String().LJust(_AsmIndentation-1)+_AsmComment+"Literal value variables\n")){ SysMessage(128,_FileName,_LineNr,_ColNr).Print(_Stl->LastError()); return false; }
    }
    if(!_Stl->FileSystem.WriteArray(_AsmHnd,Buffer3)){ SysMessage(128,_FileName,_LineNr,_ColNr).Print(_Stl->LastError()); return false; }

    //Body section
    Buffer1=_AsmBufferFilter(_AsmStar,NestIds[i]);
    Buffer2=_AsmBufferFilter(_AsmInit,NestIds[i]);
    Buffer3=_AsmBufferFilter(_AsmBody,NestIds[i]);
    
    if(Buffer1.Length()!=0 || Buffer2.Length()!=0 || Buffer3.Length()!=0){
      if(!_Stl->FileSystem.Write(_AsmHnd,"\n.section "+GetAsmSectionName(AsmSection::Body)+"\n")){ SysMessage(128,_FileName,_LineNr,_ColNr).Print(_Stl->LastError()); return false; }
    }

    //Start
    if(!_Stl->FileSystem.WriteArray(_AsmHnd,Buffer1)){ SysMessage(128,_FileName,_LineNr,_ColNr).Print(_Stl->LastError()); return false; }

    //Init
    if(!_Stl->FileSystem.WriteArray(_AsmHnd,Buffer2)){ SysMessage(128,_FileName,_LineNr,_ColNr).Print(_Stl->LastError()); return false; }

    //Body
    if(!_Stl->FileSystem.WriteArray(_AsmHnd,Buffer3)){ SysMessage(128,_FileName,_LineNr,_ColNr).Print(_Stl->LastError()); return false; }

    //Footer
    if(!_Stl->FileSystem.WriteArray(_AsmHnd,_AsmBufferFilter(_AsmFoot,NestIds[i]))){ SysMessage(128,_FileName,_LineNr,_ColNr).Print(_Stl->LastError()); return false; }
    
    //Message
    DebugMessage(DebugLevel::CmpAssembler,"Assembler buffers flushed on level "+ToString(NestIds[i]));
  
  }

  //Reset assembler buffers
  _AsmHead.Reset();
  _AsmData.Reset();
  _AsmDecl.Reset();
  _AsmDLit.Reset();
  _AsmStar.Reset();
  _AsmTemp.Reset();
  _AsmInit.Reset();
  _AsmBody.Reset();
  _AsmFoot.Reset();

  //Return code
  return true;

}

//Clear assembler buffer
void Binary::AsmResetBuffer(AsmSection Section){
  if(!_AsmEnabled){ return; }
  switch(Section){
    case AsmSection::Head: _AsmHead.Reset(); break;
    case AsmSection::Data: _AsmData.Reset(); break;
    case AsmSection::Decl: _AsmDecl.Reset(); break;
    case AsmSection::DLit: _AsmDLit.Reset(); break;
    case AsmSection::Star: _AsmStar.Reset(); break;
    case AsmSection::Temp: _AsmTemp.Reset(); break;
    case AsmSection::Init: _AsmInit.Reset(); break;
    case AsmSection::Body: _AsmBody.Reset(); break;
    case AsmSection::Foot: _AsmFoot.Reset(); break;
  }
  DebugMessage(DebugLevel::CmpAssembler,GetAsmSectionName(Section)+": Buffer was reset");
}

//Assembler function parameter
String Binary::GetAsmParameter(bool IsReference,bool IsConst,CpuDataType Type,const String& Name){
  return String(IsConst?"CONST ":"")+String(IsReference?"REF ":"")+CpuDataTypeName(Type).Upper()+" "+Name;
}

//Assembler section name
String Binary::GetAsmSectionName(AsmSection Section){
  String Name;
  switch(Section){
    case AsmSection::Head: Name="HEAD"; break;
    case AsmSection::Data: Name="DATA"; break;
    case AsmSection::Decl: Name="DECL"; break;
    case AsmSection::DLit: Name="DLIT"; break;
    case AsmSection::Star: Name="STAR"; break;
    case AsmSection::Temp: Name="TEMP"; break;
    case AsmSection::Init: Name="INIT"; break;
    case AsmSection::Body: Name="CODE"; break;
    case AsmSection::Foot: Name="FOOT"; break;
  }
  return Name;
}

//Calculate relocations for assembler argument
void Binary::_CalcRelocations(CpuInstCode InstCode,int ArgIndex,const AsmArg& Arg){
  
  //Dynamic library calls
  if(InstCode==CpuInstCode::LCALL && ArgIndex==0){
    AddAdrRelocation(RelocType::DlCall,CurrentCodeAddress(),(char *)_DlCall[Arg.Value.Int].DlName,(char *)_DlCall[Arg.Value.Int].DlFunction);
    return;
  }

  //Function addresses
  if(Arg.AdrMode==CpuAdrMode::LitValue){
    if(Arg.Type==CpuDataType::FunAddr){ 
      AddAdrRelocation(RelocType::FunAddress,CurrentCodeAddress(),_Stl->FileSystem.GetFileNameNoExt(_FileName),Arg.Name);
    }
    else if(Arg.Type==CpuDataType::ArrGeom){ 
      AddAdrRelocation(RelocType::ArrGeom,CurrentCodeAddress(),_Stl->FileSystem.GetFileNameNoExt(_FileName),"AGX:"+ToString(Arg.Value.Agx));
    }
    return;
  }
  
  //Global variables
  if(Arg.Glob){
    AddAdrRelocation(RelocType::GloAddress,CurrentCodeAddress(),_Stl->FileSystem.GetFileNameNoExt(_FileName),Arg.Name); 
    return;
  }

}

//Search instruction mnemonic
CpuInstCode Binary::SearchMnemonic(const String& Mnemonic){
  int i;
  for(i=0;i<_InstructionNr;i++){
    if(_Inst[i].Mnemonic==Mnemonic){
      return (CpuInstCode)i;
    }
  }
  return (CpuInstCode)-1;
}

//Get instruction mnemonic
String Binary::GetMnemonic(CpuInstCode InstCode){
  return _Inst[(int)InstCode].Mnemonic;
}
  
//Getnumber of arguments for instruction code
int Binary::InstArgNr(CpuInstCode InstCode){
  return _Inst[(int)InstCode].ArgNr;
}

