//Binary.hpp: Header file for binary class
#ifndef _BINARY_HPP
#define _BINARY_HPP

//Public constants
const String ObjIdSep="#";      //Object id separator

//Meta instruction codes
enum class CpuMetaInst:int{
  NEG=0,ADD,SUB,MUL,DIV,MOD,INC,DEC,PINC,PDEC,BNOT,BAND,BOR,BXOR,SHL,SHR,LES,
  LEQ,GRE,GEQ,EQU,DIS,LOAD,MV,MVAD,MVSU,MVMU,MVDI,MVMO,MVSL,MVSR,MVAN,MVXO,MVOR
};

//Assembler file sections
enum class AsmSection:int{
  Head, //Header section
  Data, //Data section
  Decl, //Declare section
  Star, //Start code section
  DLit, //DLit section
  Temp, //Temp section
  Init, //Init section
  Body, //Body section
  Foot  //Footer section
};

//Relocation types
enum class RelocType:int{
  FunAddress=0, //Function address (uses LocAdr)
  GloAddress=1, //Global variable address (uses LocAdr)
  ArrGeom=2,    //Fixed array geometry index (uses LocAdr)
  DlCall=3,     //Dynamic library call id
  GloBlock=4,   //Block inside global buffer (uses LocAdr)
  BlkBlock=5    //Block inside block (uses LocAdr/LocBlk)
};

//Undefined reference kind
enum class UndefRefKind:int{
  Glo=0, //Global data address
  Fun=2, //Function address
  Agx=3  //Fixed array geometry index
};

//Symbol table kinds
enum class LnkSymKind:int{
  Dim, //Array dimension symbol
  Typ, //Data type symbol
  Var, //Variable symbol
  Fld, //Fields symbol
  Fun, //Function symbol
  Par  //Parameter symbol
};

//Class assembler argument
class AsmArg{
  
  //Private
  private:

    //Assembler argument value
    union AsmArgValue{  
      
      //Union fields
      CpuBol Bol;        //Boolean
      CpuChr Chr;        //Char
      CpuShr Shr;        //Short
      CpuInt Int;        //Integer
      CpuLon Lon;        //Long
      CpuFlo Flo;        //Float
      CpuAdr Adr;        //Memory address
      CpuAdr Agx;        //Array geometry index
    
      //Constructors/Destructors (assignment and copy constructor never used)
      AsmArgValue(){}
      ~AsmArgValue(){}
    
    };
    
  //Public area
  public:
    
    //Members
    bool IsNull;
    bool IsError;
    bool Glob;
    bool IsUndefined;
    CpuDataType Type;
    CpuAdrMode AdrMode;
    AsmArgValue Value;
    String Name;
    String ObjectId;
    int ScopeDepth;

    //Members
    int BinLength() const;
    Buffer ToCode() const;
    String ToText() const;
    String ToHex() const;

    //Constructors/Destructors (assignment and copy constructor never used)
    AsmArg(){ 
      IsNull=true; 
      IsError=false;
      IsUndefined=false;
      Glob=false;
      AdrMode=(CpuAdrMode)0; 
      Name="";
      ObjectId="";
      ScopeDepth=-1;
      Type=(CpuDataType)0;
    }
    ~AsmArg(){}

};

//Dependencies table
struct Dependency{
  CpuChr Module[_MaxIdLen+1]; //Module name
  CpuShr LibMajorVers;  //Library major version
  CpuShr LibMinorVers;  //Library minos version
  CpuShr LibRevisionNr; //Library revision number
};

//Undefined references table
struct UndefinedRefer{
  CpuChr Module[_MaxIdLen+1]; //Module name
  CpuChr Kind;                //Undefined reference kind
  CpuAdr CodeAdr;             //Code address at which undefined reference is located
  String ObjectName;          //Object name
};

//Linker symbol tables class
struct LnkSymTables{
  
  //Array dimension symbol table
  struct LnkSymDimension{
    ArrayIndexes DimSize;    //Dimension sizes
    CpuAgx GeomIndex;        //Fixed array geometry index
    CpuInt TypIndex;         //Type index that owns this dimension record
  };

  //Datatype symbol table
  struct LnkSymType{
    CpuChr Name[_MaxIdLen+1];   //Data type name
    CpuChr MstType;             //Master type
    CpuInt FunIndex;            //Function index (for local scope)
    CpuInt SupTypIndex;         //Type index of supperior type (if any)
    CpuBol IsTypedef;           //Type comes from type definition
    CpuInt OrigTypIndex;        //Original typw when type comes from type definition
    CpuBol IsSystemDef;         //Type is defined by system
    CpuLon Length;              //Type size
    CpuChr DimNr;               //Number of array dimensions
    CpuInt ElemTypIndex;        //Data type of array element
    CpuInt DimIndex;            //Array dimension index
    CpuInt FieldLow;            //Field low in member table (class variables)
    CpuInt FieldHigh;           //Field high in member table (class variables)
    CpuInt MemberLow;           //Function member low in function table (class variables)
    CpuInt MemberHigh;          //Function member high in function table (class variables)
    CpuAdr MetaName;            //Global litteral value address that holds type name metadata
    CpuAdr MetaStNames;         //Global litteral value address that holds class field names array
    CpuAdr MetaStTypes;         //Global litteral value address that holds class field types array
    CpuChr DlName[_MaxIdLen+1]; //Dynamic library name
    CpuChr DlType[_MaxIdLen+1]; //Dynamiclibrary type name
  };

  //Variable symbol table
  struct LnkSymVariable{
    CpuChr Name[_MaxIdLen+1]; //Variable name
    CpuInt FunIndex;          //Function index (for local scope)
    CpuInt TypIndex;          //Data type table index
    CpuLon Address;           //Memory address
    CpuBol IsConst;           //Is constant ?
    CpuAdr MetaName;          //Global littealvalue address that holds var name metadata
  };

  //Field symbol table
  struct LnkSymField{
    CpuChr Name[_MaxIdLen+1]; //Field name
    CpuInt SupTypIndex;       //Type index of supperior type (if any)
    CpuChr SubScope;          //SubScope
    CpuInt TypIndex;          //Data type
    CpuLon Offset;            //Field offset from start address
    CpuChr IsStatic;          //Is field static ?
    CpuInt EnumValue;         //Value for enumerated fields
  };
   
  //Function symbol table
  struct LnkSymFunction{
    CpuChr Kind;                   //Function kind
    CpuChr Name[_MaxIdLen+1];      //Function name
    CpuInt SupTypIndex;            //Type index of supperior type (if any)
    CpuChr SubScope;               //SubScope
    CpuLon Address;                //Code address
    CpuInt TypIndex;               //Return data type
    CpuBol IsVoid;                 //Is function returning void ?
    CpuBol IsInitializer;          //Is initializer method ? (sets class initial value)
    CpuBol IsMetaMethod;           //Is a master method that returns metadata? (.name() .dtype() ...) 
    CpuInt ParmNr;                 //Number of function parameters
    CpuInt ParmLow;                //Parameter low in parameter table
    CpuInt ParmHigh;               //Parameter high in parameter table
    CpuChr MstType;                //Corresponding master type (master method only)
    CpuShr MstMethod;              //Master method identifier (master method only)
    CpuInt SysCallNr;              //SystemCall number
    CpuShr InstCode;               //Instruction code for system function
    CpuChr DlName[_MaxIdLen+1];     //Function name
    CpuChr DlFunction[_MaxIdLen+1]; //Function name
  };
  
  //Parameter symbol table
  struct LnkSymParameter{
    CpuChr Name[_MaxIdLen+1]; //Parameter name
    CpuInt TypIndex;         //Data type table index
    CpuBol IsConst;          //Is constant ?
    CpuBol IsReference;      //Is variable a reference ? 
    CpuInt ParmOrder;        //Parameter order number in function argument list
    CpuInt FunIndex;         //Function index to which this parameter belongs to
  };

  //Symbol tables
  Array<LnkSymDimension> Dim; //Array dimension symbols
  Array<LnkSymType> Typ;      //Data type symbols
  Array<LnkSymVariable> Var;  //Variable symbols
  Array<LnkSymField> Fld;     //Fields symbols
  Array<LnkSymFunction> Fun;  //Function symbols
  Array<LnkSymParameter> Par; //Parameter symbols

};

//Binary class
class Binary{  
  
  //Private members
  private:

    //Debug symbol tables
    struct DbgSymTables{
      Array<DbgSymModule> Mod;    //Modules
      Array<DbgSymType> Typ;      //Types
      Array<DbgSymVariable> Var;  //Variables
      Array<DbgSymField> Fld;     //Fields
      Array<DbgSymFunction> Fun;  //Functions
      Array<DbgSymParameter> Par; //Parameters
      Array<DbgSymLine> Lin;      //Source code lines
    };

    //Destination jump addresses table
    struct DestAddress{
      int ScopeDepth;   //Corresponding scope depth on which label is located
      String DestLabel; //Destination label
      CpuAdr DestAdr;   //Jump destination address
      const String& SortKey() const { return DestLabel; }
    };
    
    //Destination jump addresses table 2 (to be able to search labels by DestAdr as on previous table is not possible)
    //This table is only used for optimization purposes
    struct DestAddress2{
      int ScopeDepth;   //Corresponding scope depth on which label is located
      String DestLabel; //Destination label
      CpuAdr DestAdr;   //Jump destination address
      CpuAdr SortKey() const { return DestAdr; }
    };
    
    //Origin jump addresses table
    struct OrigAddress{
      int ScopeDepth;   //Corresponding scope depth on which label is called
      String OrigLabel; //Origin label
      CpuAdr CodeAdr;   //Code address at which resolved jump needs to be inserted
      CpuAdr InstAdr;   //Address of jump instruction
      String FileName;  //File name
      int LineNr;       //Line number
    };
    
    //Function addresses
    struct FunAddress{
      String SearchName; //Function search name (ScopeDepth+Id)
      String FullName;   //Function full name
      int ScopeDepth;    //Function scope depth
      CpuAdr Address;    //Function address
      const String& SortKey() const { return SearchName; }
    };
    
    //Forward function calls
    struct FunFwdCall{ 
      String Id;         //Function id
      String FullName;   //Function full name
      int ScopeDepth;    //Function scope depth
      CpuAdr CodeAdr;    //Code address at which fuction is called
      String FileName;   //Module name at which function is called
      int LineNr;        //LineNr name at which function is called
      int ColNr;         //Column number at which function is called
      String SourceLine; //Source line at which function is called
    };  

    //Replaced numeric litteral values by unresolved variables
    struct LitNumValueVars{
      int ScopeDepth;           //Corresponding scope depth on which lit value was replaced
      CpuAdr CodeAdr;           //Code address at which local variable address must be resolved
      bool Glob;                //Global flag
      char LitValueBytes[8];    //Litteral value bytes
      int LitValueLen;          //Litteral value length
      CpuDataType LitValueType; //Litteral value type
      String LitValueHex;       //Hex representation of litteral value
      String LitValueText;      //Text representation of litteral value
      String ReplId;            //Replacement id (for assembler code)
      SourceInfo SrcInfo;       //Source information
      String SourceLine;        //Declaration source line
    };

    //Relocation table definition
    struct RelocItem{
      RelocType Type;              //Relocation type
      CpuAdr LocBlk;               //Location block (Place at which relocation address is located, it is a block inside _StrBlocks _ArrBlocks)
      CpuAdr LocAdr;               //Location address (Place at which relocation address is located, it is an address inside _GlobBuffer, _CodeBuffer or a inside an array block from _ArrBlock)
      CpuChr Module[_MaxIdLen+1];  //Module on which relocation is originted
      CpuChr ObjName[_MaxIdLen+1]; //Object name related to relocation
      CpuInt CopyCount;            //0 when relocation is added for first time, then each relocation copy on Binary::ImportLibrary() adds 1
    };

    //Initialization routines
    struct InitRoutine{
      String Module;
      String Name;
      CpuAdr Address;
      int ScopeDepth;
    };

    //Assembler file lines
    struct AssemblerLine{
      int NestId;                                             //Assembler id (used to distinguish between different nested functions in buffer)
      String Line;                                            //Assembler line 
      AssemblerLine(){}                                       //Constructor
      ~AssemblerLine(){}                                      //Destructor
      AssemblerLine(int Lev,const String& Lin);               //Constructor from values
      AssemblerLine(const AssemblerLine& AsmLine);            //Copy constructor
      AssemblerLine& operator=(const AssemblerLine& AsmLine); //Assignment operator
      void _Move(const AssemblerLine& AsmLine);               //Move
    };

    //Program data and buffers
    Buffer _GlobBuffer;                             //Global variable buffer
    Buffer _CodeBuffer;                             //Code buffer
    Buffer _InitBuffer;                             //Code initialization buffer
    Array<BlockDef> _Block;                         //Block table (for strings and arrays)
    Array<ArrayFixDef> _ArrFixDef;                  //Fixed array definitions buffer (for global fixed arrays)
    Array<ArrayDynDef> _ArrDynDef;                  //Dynamic array definitions buffer (for array litterals)
    SortedArray<FunAddress,const String&> _FunAddr; //Function addresses
    Array<FunFwdCall> _FunCall;                     //Function forward calls
    Array<RelocItem> _RelocTable;                   //Relocation table
    Array<DlCallDef> _DlCall;                       //Dynamic library calls
    CpuWrd _StackSize;                              //Local variable buffer (we only keep size, since it is dynamically created/destroyed)
    CpuAgx _GlobGeometryNr;                         //Number of defined global fixed array geometries
    CpuAgx _LoclGeometryNr;                         //Number of defined local fixed array geometries
    
    //Initialization routines and start code
    CpuAdr _SuperInitAdr;                     //Address of supper initialization routine
    bool _HasInitAdr;                         //Address of initialization code set already
    CpuAdr _InitAddress;                      //Address of initialization code
    Array<InitRoutine> _InitRoutines;         //Addresses of initialization routines of loaded libraries and compiled modules
        
    //Dependencies & output undefined references
    Array<Dependency> _Depen;
    Array<UndefinedRefer> _OUndRef;

    //Tables of exported linker symbols (used by GenerateLibrary, written in output library file)
    LnkSymTables _OLnkSymTables;

    //Debug symbol tables
    DbgSymTables _ODbgSymTables;
    DbgSymTables _IDbgSymTables;

    //Jump calculation tables
    SortedArray<DestAddress,const String&>  _DestAdr; //Destination jump addresses
    SortedArray<DestAddress2,CpuAdr> _DestAdr2;       //Destination jump addresses (second table for speed up search by address)
    Array<OrigAddress> _OrigAdr;                      //Origin jump addresses

    //Replaced litteral values by local variables
    bool _GlobReplLitValues;
    Array<LitNumValueVars> _ReplLitValues;

    //Library optios
    bool _CompileToLibrary; //Compile to library option
    CpuShr _LibMajorVers;   //Library major version
    CpuShr _LibMinorVers;   //Library minor version
    CpuShr _LibRevisionNr;  //Library revision number

    //Memory controller configuration (to be saved in binary header)
    CpuWrd _MemUnitSize;   //Memory assignment unit
    CpuWrd _MemUnits;      //Starting number of memory units
    CpuWrd _ChunkMemUnits; //Increase size of memory units
    CpuMbl _BlockMax;      //Memory handler table initial size

    //Assembler file objects
    int _AsmHnd;                   //Assembler file handler
    int _AsmNestId;                //Current line nested id
    int _AsmNestCounter;           //Counter to assign unique nest ids
    bool _AsmEnabled;              //Enable generation of assembler file
    Stack<int> _AsmNestIdStack;    //Stack that keeps history of nested ids
    Array<AssemblerLine> _AsmHead; //Assembler head buffer
    Array<AssemblerLine> _AsmData; //Assembler data buffer
    Array<AssemblerLine> _AsmDecl; //Assembler decl buffer
    Array<AssemblerLine> _AsmDLit; //Assembler dlit buffer
    Array<AssemblerLine> _AsmStar; //Assembler star buffer
    Array<AssemblerLine> _AsmTemp; //Assembler temp buffer
    Array<AssemblerLine> _AsmInit; //Assembler init buffer
    Array<AssemblerLine> _AsmBody; //Assembler body buffer
    Array<AssemblerLine> _AsmFoot; //Assembler foot buffer
    int _AsmIndentation;           //Assembler file indentation

    //Error reporting variables
    String _FileName;   //File name
    int _LineNr;        //Line number
    int _ColNr;         //Column number

    //Name of output file
    String _BinaryFile;   //Binary file name
    
    //Members
    Buffer InstCodeToBuffer(CpuInstCode InstCode);
    void _AsmInstCodeReplacements(CpuInstCode &InstCode,int ArgNr,const AsmArg *Arg);
    bool _AsmLitValueReplacements(CpuInstCode InstCode,int ArgNr,AsmArg *Arg,String& ReplIds,int *ReplIndexes);
    bool _AsmChecks(CpuInstCode InstCode,int ArgNr,const AsmArg *Arg);
    bool _AsmProgDecoders(AsmSection Section,CpuInstCode InstCode,int ArgNr,AsmArg *Arg);
    bool _AsmWriteCode(CpuInstCode InstCode,AsmSection Section,bool StrArg,const AsmArg& Arg1=AsmArg(),const AsmArg& Arg2=AsmArg(),const AsmArg& Arg3=AsmArg(),const AsmArg& Arg4=AsmArg());
    void _AsmOutCode(AsmSection Section,CpuAdr InstAdr,Array<String>& Labels,CpuInstCode InstCode,bool StrArg,AsmArg *Arg,int ArgNr,const String& Tag);
    Array<String> _AsmBufferFilter(const Array<AssemblerLine>& Buff,int NestId);
    void _AsmOutRaw(AsmSection Section,const String& OutLine);
    void _WriteBinaryError(int Hnd,const char *FileMark,const String& Index);
    bool _WriteBinary(const String& FileName,bool Library,bool DebugSymbols);
    void _CalcRelocations(CpuInstCode InstCode,int ArgIndex,const AsmArg& Arg);
    void _ReadBinaryError(int Hnd,const char *FileMark,const String& Index,const SourceInfo& SrcInfo);
    bool _ReadBinary(const String& FileName,bool Library,BinaryHeader& Hdr,Buffer& AuxGlobBuffer,Buffer& AuxCodeBuffer,Array<BlockDef>& AuxBlock,Array<ArrayFixDef>& AuxArrFixDef,Array<ArrayDynDef>& AuxArrDynDef,Array<DlCallDef>& AuxDlCall,Array<Dependency>& AuxDepen,Array<UndefinedRefer>& AuxUndRef,Array<RelocItem>& AuxRelocTable,LnkSymTables& LnkSym,DbgSymTables& DbgSym,const SourceInfo& SrcInfo);
    String _GetLnkSymDebugStr(const LnkSymTables& Sym,LnkSymKind Kind);

  //Public members
  public:
  
    //Imported undefined references
    Array<UndefinedRefer> IUndRef;

    //Tables of imported symbols (used by load library, used by compiler to generate masterdata)
    LnkSymTables ILnkSymTables;

    //Members
    AsmArg AsmLitBol(CpuBol Bol) const;
    AsmArg AsmLitChr(CpuChr Chr) const;
    AsmArg AsmLitShr(CpuShr Shr) const;
    AsmArg AsmLitInt(CpuInt Int) const;
    AsmArg AsmLitLon(CpuLon Lon) const;
    AsmArg AsmLitFlo(CpuFlo Flo) const;
    AsmArg AsmLitStr(CpuAdr Adr) const;
    AsmArg AsmLitWrd(CpuWrd Wrd) const;
    int GetAsmIndentation();
    void SetAsmIndentation(int Indent);
    void SetAsmDefaultIndentation();
    int GetAsmDefaultIndentation();
    int GetHexIndentation();
    AsmArg SubroutineAsmArg(CpuAdr Address,const String& Module,const String& Name,int ScopeDepth) const;
    void SetLitValueReplacementsGlobal(bool Glob);
    void SetBinaryFile(const String& BinaryFile);
    void SetSource(const SourceInfo& SrcInfo);
    bool HasInitAddress();
    void SetInitAddress(CpuAdr Address,const String& Module);
    void ClearInitAddress();
    CpuAdr GetInitAddress();
    void AppendInitRoutine(CpuAdr Address,const String& Module,const String& Name,int ScopeDepth);
    bool CallInitRoutines();
    void SetSuperInitAddress(CpuAdr Address,const String& Module,const String& Name);
    CpuAdr GetSuperInitAddress();
    void CodeBufferModify(CpuAdr At,CpuAdr Value);
    void CodeBufferModify(CpuAdr At,CpuAgx Value);
    void EnableAssemblerFile(bool Enable);
    bool OpenAssembler(const String& FileName);
    bool CloseAssembler();
    void SetCompileToLibrary(bool CompileToLibrary);
    void SetLibraryVersion(CpuShr LibMajorVers,CpuShr LibMinorVers,CpuShr LibRevisionNr);
    void SetMemoryConfigMemUnitSize(CpuWrd MemUnitSize);
    void SetMemoryConfigMemUnits(CpuWrd MemUnits);
    void SetMemoryConfigChunkMemUnits(CpuWrd ChunkMemUnits);
    void SetMemoryConfigBlockMax(CpuMbl BlockMax);
    bool GenerateLibrary(bool DebugSymbols);
    bool GenerateExecutable(bool DebugSymbols);
    bool LoadLibraryDependencies(const String& FileName,BinaryHeader& Hdr,Array<Dependency>& Depen,const SourceInfo& SrcInfo);
    bool ImportLibrary(const String& FileName,bool HardLinkage,CpuAdr& InitAddress,CpuShr Major,CpuShr Minor,CpuShr RevNr,const SourceInfo& SrcInfo);
    bool PrintBinary(const String& FileName,bool Library);
    CpuAgx GetGlobGeometryIndex();
    CpuAgx GetLoclGeometryIndex();
    void ResetLoclGeometryIndex();
    void StoreGlobValue(const Buffer& Value);
    void GlobBufferRewind(long Bytes);
    char *GlobValuePointer(CpuAdr Address);
    void ResetStackSize();
    void CumulStackSize(int Size);
    int StoreDependency(const String& Module,CpuShr LibMajorVers,CpuShr LibMinorVers,CpuShr LibRevisionNr);
    int StoreUndefReference(const String& ObjectId,CpuChr Kind,CpuAdr CodeAdr);
    int StoreLnkSymDimension(ArrayIndexes DimSize,CpuAgx GeomIndex);
    int StoreLnkSymType(const String& Name,CpuChr MstType,CpuInt FunIndex,CpuInt SupTypIndex,CpuBol IsTypedef,CpuInt OrigTypIndex,CpuBol IsSystemDef,CpuLon Length,CpuChr DimNr,CpuInt ElemTypIndex,CpuInt DimIndex,CpuInt FieldLow,CpuInt FieldHigh,CpuInt MemberLow,CpuInt MemberHigh,CpuAdr MetaName,CpuAdr MetaStNames,CpuAdr MetaStTypes,const SourceInfo& SrcInfo);
    int StoreLnkSymVariable(const String& Name,CpuInt FunIndex,CpuInt TypIndex,CpuLon Address,CpuBol IsConst,CpuAdr MetaName,const SourceInfo& SrcInfo);
    int StoreLnkSymField(const String& Name,CpuInt SupTypIndex,CpuChr SubScope,CpuInt TypIndex,CpuLon Offset,CpuChr IsStatic,CpuInt EnumValue,const SourceInfo& SrcInfo);
    int StoreLnkSymFunction(CpuChr Kind,const String& Name,CpuInt SupTypIndex,CpuChr SubScope,CpuLon Address,CpuInt TypIndex,CpuBol IsVoid,CpuBol IsInitializer,CpuBol IsMetaMethod,CpuInt ParmNr,CpuInt ParmLow,CpuInt ParmHigh,CpuChr MstType,CpuShr MstMethod,CpuInt SysCallNr,CpuShr InstCode,const String& DlName,const String& DlFunction,const SourceInfo& SrcInfo);
    int StoreLnkSymParameter(const String& Name,CpuInt TypIndex,CpuBol IsConst,CpuBol IsReference,CpuInt ParmOrder,CpuInt FunIndex,const SourceInfo& SrcInfo);
    void UpdateLnkSymDimension(int LnkSymIndex,int TypIndex);
    void UpdateLnkSymTypeForField(int LnkSymIndex,CpuAdr MetaStNames,CpuAdr MetaStTypes,CpuLon Length,CpuInt FieldLow,CpuInt FieldHigh);
    void UpdateLnkSymTypeForFunctionMember(int LnkSymIndex,CpuInt MemberLow,CpuInt MemberHigh);
    void UpdateLnkSymTypeForDlType(int LnkSymIndex,const String& DlName,const String& DlType);
    void UpdateLnkSymFunction(int LnkSymIndex,CpuInt ParmNr,CpuInt ParmLow,CpuInt ParmHigh);
    void UpdateLnkSymFunctionAddress(int LnkSymIndex,CpuAdr Address);
    void DeleteLnkSymFunction(int LnkSymIndex);
    void DeleteLnkSymParameter(int LnkSymIndex);
    int StoreDbgSymModule(const String& Name,const String& Path,const SourceInfo& SrcInfo);
    int StoreDbgSymType(const String& Name,CpuChr CpuType,CpuChr MstType,CpuLon Length,CpuInt FieldLow,CpuInt FieldHigh,const SourceInfo& SrcInfo);
    int StoreDbgSymVariable(const String& Name,CpuInt ModIndex,CpuBol Glob,CpuInt FunIndex,CpuInt TypIndex,CpuLon Address,CpuBol IsReference,const SourceInfo& SrcInfo);
    int StoreDbgSymField(const String& Name,CpuInt TypIndex,CpuLon Offset,const SourceInfo& SrcInfo);
    int StoreDbgSymFunction(CpuChr Kind,const String& Name,CpuInt ModIndex,CpuLon BegAddress,CpuLon EndAddress,CpuInt TypIndex,CpuBol IsVoid,CpuInt ParmNr,CpuInt ParmLow,CpuInt ParmHigh,const SourceInfo& SrcInfo);
    int StoreDbgSymParameter(const String& Name,CpuInt TypIndex,CpuInt FunIndex,CpuBol IsReference,const SourceInfo& SrcInfo);
    int StoreDbgSymLine(CpuInt ModIndex,CpuLon BegAddress,CpuLon EndAddress,CpuInt LineNr);
    void UpdateDbgSymTypeForField(int DbgSymIndex,CpuLon Length,CpuInt FieldLow,CpuInt FieldHigh);
    void UpdateDbgSymFunction(int DbgSymIndex,CpuInt ParmNr,CpuInt ParmLow,CpuInt ParmHigh);
    void UpdateDbgSymFunctionBegAddress(int DbgSymIndex,CpuAdr BegAddress);
    void UpdateDbgSymFunctionEndAddress(int DbgSymIndex,CpuAdr EndAddress);
    void DeleteDbgSymFunction(int DbgSymIndex);
    void DeleteDbgSymParameter(int DbgSymIndex);
    int StoreDlCall(const String& DlName,const String& DlFunction);
    void StoreStrBlockValue(const Buffer& Value);
    void StoreArrBlockValue(const Buffer& Value,ArrayDynDef ArrDynDef);
    void StoreArrFixDef(ArrayFixDef ArrFixDef);
    void StoreJumpDestination(const String& Label,int ScopeDepth,CpuAdr DestAdr);
    void StoreJumpOrigin(const String& Label,int ScopeDepth,CpuAdr CodeAdr,CpuAdr InstAdr,const String& FileName,int LineNr);
    bool SolveJumpAddresses(int ScopeDepth,const String& ScopeName);
    void StoreFunctionAddress(const String& FunId,const String& FullName,int ScopeDepth,CpuAdr Address);
    void StoreForwardFunCall(const String& FunId,const String& FullName,int ScopeDepth,CpuAdr CodeAdr,const String& FileName,int LineNr,int ColNr);
    void SetFunctionAddress(CpuAdr CodeAdr,CpuAdr FunAdr);
    bool SolveForwardCalls(int ScopeDepth,const String& ScopeName);
    bool SolveLitValueVars(bool DebugSymbols,int DbgSymModIndex,int DbgSymFunIndex);
    void MergeCodeBuffers(CpuAdr FromAdr);
    CpuAdr CurrentGlobAddress();
    CpuAdr CurrentStackAddress();
    CpuAdr CurrentCodeAddress();
    CpuMbl CurrentBlockAddress();
    CpuInt CurrentDlCallId();
    CpuLon GetInitBufferSize();
    bool SetFunctionStackInst();
    void SetFunctionStackSize(CpuAdr FunAddress);
    bool AsmWriteCode(CpuMetaInst Meta,const AsmArg& Arg1=AsmArg(),const AsmArg& Arg2=AsmArg(),const AsmArg& Arg3=AsmArg(),const AsmArg& Arg4=AsmArg());
    bool AsmWriteCode(CpuMetaInst Meta,int DriverArg,const AsmArg& Arg1=AsmArg(),const AsmArg& Arg2=AsmArg(),const AsmArg& Arg3=AsmArg(),const AsmArg& Arg4=AsmArg());
    bool AsmWriteCode(CpuInstCode InstCode,const AsmArg& Arg1=AsmArg(),const AsmArg& Arg2=AsmArg(),const AsmArg& Arg3=AsmArg(),const AsmArg& Arg4=AsmArg());
    bool AsmWriteCode(CpuInstCode InstCode,AsmSection Section,const AsmArg& Arg1=AsmArg(),const AsmArg& Arg2=AsmArg(),const AsmArg& Arg3=AsmArg(),const AsmArg& Arg4=AsmArg());
    bool AsmWriteInitCodeStrArg(CpuInstCode InstCode,const AsmArg& Arg1=AsmArg(),const AsmArg& Arg2=AsmArg(),const AsmArg& Arg3=AsmArg(),const AsmArg& Arg4=AsmArg());
    bool AsmWriteInitCode(CpuInstCode InstCode,bool StrArg,const AsmArg& Arg1=AsmArg(),const AsmArg& Arg2=AsmArg(),const AsmArg& Arg3=AsmArg(),const AsmArg& Arg4=AsmArg());
    void AsmOutCommentLine(AsmSection Section,const String& SourceLine,bool Indented);
    void AsmOutLine(AsmSection Section,const String& Label,const String& Line);
    void AsmOutLine(AsmSection Section,const String& Label,const String& Line,const String& HexPart);
    void AsmOutSectionName(AsmSection Section,String Name);
    void AsmOutSeparator(AsmSection Section);
    void AsmOutNewLine(AsmSection Section);
    void AsmOutVarDecl(AsmSection Section,bool Glob,bool IsParameter,bool IsReference,bool IsConst,bool IsStatic,const String& Name,CpuDataType Type,CpuWrd Length,CpuAdr Address,const String& SourceLine,bool Import,bool Export,const String& Library,const String& InitValue);
    void AsmOutFunDecl(AsmSection Section,const String& Name,const String& Parms,bool Import,bool Export,bool Local,const String& Library,CpuAdr Address,const String& As);
    void AsmOutDlCallTable(AsmSection Section);
    void AsmDeleteLast(AsmSection Section);
    void AsmOpenNestId();
    bool AsmCloseNestId();
    bool AsmSetParentNestId();
    void AsmRestoreCurrentNestId();
    void AsmNestIdCounter();
    bool AsmFlush();
    void AsmResetBuffer(AsmSection Section);
    String GetAsmParameter(bool IsReference,bool IsConst,CpuDataType Type,const String& Name);
    String GetAsmSectionName(AsmSection Section);
    void AddAdrRelocation(RelocType Type,CpuAdr LocAdr,const String& Module,const String& ObjName);
    void AddBlkRelocation(RelocType Type,CpuAdr LocAdr,const String& Module,const String& ObjName);
    void AddBlkBlkRelocation(RelocType Type,CpuMbl LocBlk,CpuAdr LocAdr,const String& Module,const String& ObjName);
    String RelocationTypeText(RelocType Type);
    CpuInstCode SearchMnemonic(const String& Mnemonic);
    String GetMnemonic(CpuInstCode InstCode);
    int InstArgNr(CpuInstCode InstCode);

    //Constructor / Destructor
    Binary();
    ~Binary(){}   

};

#endif  