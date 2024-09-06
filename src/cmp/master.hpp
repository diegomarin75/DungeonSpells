//Master.hpp: Header file for master data class
#ifndef _MASTER_HPP
#define _MASTER_HPP

//Include library manager
#include "lib/libman.hpp"
 
//Scopes
enum class ScopeKind:char{ Public=0, Private, Local };

//Sub scopes
enum class SubScopeKind:char{ None=0, Public, Private };

//Master datatypes
enum class MasterType:int{ Boolean=0, Char, Short, Integer, Long, Float, String, Enum, Class, FixArray, DynArray };

//Function kinds
enum class FunctionKind:int{ Function=0, MasterMth, Member, SysCall, SysFunc, DlFunc, Operator };

//Grant types
enum class GrantKind:int{ 
  ClassToClass=0, //Grant class to entire class
  ClassToField,   //Grant class to field member
  ClassToFunMb,   //Grant class to function member
  FunctToClass,   //Grant function to entire class
  FunctToField,   //Grant function to field member
  FunctToFunMb,   //Grant function to function member
  FunMbToClass,   //Grant function member to entire class
  FunMbToField,   //Grant function member to field member
  FunMbToFunMb,   //Grant function member to function member
  OpertToClass,   //Grant function operator to entire class
  OpertToField,   //Grant function operator to field member
  OpertToFunMb    //Grant function operator to function member
};

//Temporary var kinds
enum class TempVarKind:int{
  Regular,   //Regular temporary variable
  Promotion, //Temporary variables for promotions
  Master     //Temporary variables for master method execution
};

//Meta const case
enum class MetaConstCase:int{
  VarName=1,
  TypName=2,
  FldNames=3,
  FldTypes=4
};

//ObjectId prefixes (for undefined references)
#define OBJECTID_MVN "MVN" //Variable name metadata
#define OBJECTID_MTN "MTN" //Type name metadata
#define OBJECTID_MFN "MFN" //Field names metadata
#define OBJECTID_MFT "MFT" //Field types metadata

//Scope definition
struct ScopeDef{
  ScopeKind Kind;    //Scope kind
  int ModIndex;      //module index
  int FunIndex;      //function index
  int Depth;         //Scope depth
};

//Sub scope
struct SubScopeDef{
  SubScopeKind Kind;     //Sub scope kind
  int TypIndex;          //class type index
};

//Scope operators
bool operator==(const ScopeDef& Scope1,const ScopeDef& Scope2);
bool operator!=(const ScopeDef& Scope1,const ScopeDef& Scope2);
bool operator==(const SubScopeDef& SubScope1,const SubScopeDef& SubScope2);
bool operator!=(const SubScopeDef& SubScope1,const SubScopeDef& SubScope2);

//Master methods
enum class MasterMethod{
  //Fixed arrays
  ArfDsize,
  //Fixed 1-dim arrays
  ArfLen,ArfJoin,
  //Dynamic arrays
  ArdRsize1,ArdRsize2,ArdRsize3,ArdRsize4,ArdRsize5,ArdDsize,ArdReset,
  //Dynamic 1-dim arrays
  ArdLen,ArdAdd,ArdIns,ArdDel,ArdJoin,
  //Chars
  ChrUpper,ChrLower,
  //Strings
  StrLen,StrSub,StrRight,StrLeft,StrCutRight,StrCutLeft,StrSearch,StrReplace,StrTrim,StrUpper,StrLower,StrLJust1,StrRJust1,StrLJust2,
  StrRJust2,StrMatch,StrLike,StrReplicate,StrSplit,StrStartswith,StrEndswith,StrIsbool,StrIschar,StrIsshort,StrIsint,StrIslong,StrIsfloat,
  //Data conversions
  BolTochar,BolToshort,BolToint,BolTolong,BolTofloat,BolTostring,
  ChrTobool,ChrToshort,ChrToint,ChrTolong,ChrTofloat,ChrTostring,ChrFormat,
  ShrTobool,ShrTochar,ShrToint,ShrTolong,ShrTofloat,ShrTostring,ShrFormat,
  IntTobool,IntTochar,IntToshort,IntTolong,IntTofloat,IntTostring,IntFormat,
  LonTobool,LonTochar,LonToshort,LonToint,LonTofloat,LonTostring,LonFormat,
  FloTobool,FloTochar,FloToshort,FloToint,FloTolong,FloTostring,FloFormat,
  StrTobool,StrTochar,StrToshort,StrToint,StrTolong,StrTofloat,
  //Classes
  ClaFieldCount,ClaFieldNames,ClaFieldTypes,
  //Enums
  EnuTochar,EnuToshort,EnuToint,EnuTolong,EnuTofloat,EnuTostring,EnuFieldCount,EnuFieldNames,EnuFieldTypes,
  //Generic methods
  BolName,ChrName,ShrName,IntName,LonName,FloName,StrName,ClaName,EnuName,ArfName,ArdName,
  BolType,ChrType,ShrType,IntType,LonType,FloType,StrType,ClaType,EnuType,ArfType,ArdType,
  BolSizeof,ChrSizeof,ShrSizeof,IntSizeof,LonSizeof,FloSizeof,StrSizeof,EnuSizeof,ArfSizeof,ArdSizeof,
  BolToBytes,ChrToBytes,ShrToBytes,IntToBytes,LonToBytes,FloToBytes,StrToBytes,ClaToBytes,EnuToBytes,ArfToBytes,ArdToBytes,
  BolFromBytes,ChrFromBytes,ShrFromBytes,IntFromBytes,LonFromBytes,FloFromBytes,StrFromBytes,ClaFromBytes,EnuFromBytes,ArfFromBytes,ArdFromBytes
};

//Compiler master data class
class MasterData{  
  
  //Private members
  private:
  
    //Seach index
    struct SearchIndex{
      String Name;
      int Pos;
      const String& SortKey() const { return Name; }
      SearchIndex(){}
      SearchIndex(const String& IndexName,int IndexPos);
      ~SearchIndex(){}  
      SearchIndex(const SearchIndex& Index);
      SearchIndex& operator=(const SearchIndex& Index);
      void _Move(const SearchIndex& Index);
    };
    
    //Scope stack definition
    struct ScopeStkDfn{
      ScopeDef Scope;
      SubScopeDef SubScope;
      SortedArray<SearchIndex,const String&> Mod;
      SortedArray<SearchIndex,const String&> Trk;
      SortedArray<SearchIndex,const String&> Typ;
      SortedArray<SearchIndex,const String&> Var;
      SortedArray<SearchIndex,const String&> Fun;
      SortedArray<SearchIndex,const String&> Fnc;
      SortedArray<SearchIndex,const String&> Gra;
      ScopeStkDfn(){}  
      ScopeStkDfn(ScopeDef NewScope,SubScopeDef NewSubScope);
      ~ScopeStkDfn(){}  
      ScopeStkDfn(const ScopeStkDfn& ScopeStk);
      ScopeStkDfn& operator=(const ScopeStkDfn& ScopeStk);
      void _Move(const ScopeStkDfn& ScopeStk);
    };

    //Dynamic library handlers
    struct LibraryDef{
      String Name;
      void *Handler;
      FunPtrInitDispatcher InitDispatcher;
      FunPtrSearchType SearchType;
      FunPtrSearchFunction SearchFunction;
      FunPtrGetFunctionDef GetFunctionDef;
      FunPtrCloseDispatcher CloseDispatcher;
    };

    //Scope stack and module search index
    Stack<ScopeStkDfn> _ScopeStk;

    //Dynamic libraries
    Array<LibraryDef> _Library;

    //Used by ternary operator to generate unique labels
    int _LabelGenerator1; 
    int _LabelGenerator2; 

    //Used by flow operators to generate unique labels
    CpuInt _FlowLabelGenerator1; 
    CpuInt _FlowLabelGenerator2; 

    //Counters for generating litteral values (string and string[])
    int _LitStrGenerator;
    int _LitStaGenerator;

    //Members
    AsmArg _AsmVarIndex(int VarIndex,bool Indirection) const;
    Buffer _BlockNumberBytes(int Block) const;
    int _InnerPrivScope() const;
    void _InnerStoreFunction(const ScopeDef& Scope,const SubScopeDef& Sub,FunctionKind Kind,const String& Name,int TypIndex,bool IsVoid,bool IsNested,bool IsInitializer,bool IsMetaMethod,MasterType MstType,MasterMethod MstMethod,int SysCallNr,CpuInstCode InstCode,const String& DlName,const String& DlFunction,const SourceInfo& SrcInfo,const String& SourceLine);
    int _LoadDynLibrary(const String& DlName,const SourceInfo& SrcInfo);
    int _FunSearch(const String& SearchName,const String& Name,const String& Parms,const String& ConvParms,String *Matched,bool ByName=false) const;
    void _FunNameMatches(int ScopeIndex,int FncIndex,int& MatchCount,String& NameMatches) const;

  //Public members
  public:
  
    //Modules table
    struct ModuleTable{
      String Name;            //Module name
      String Path;            //Module full name
      bool IsModLibrary;      //Is module a library ?
      int DbgSymIndex;        //Debug symbol index
    };

    //Module tracker table
    struct TrackerTable{
      String Name;            //Module tracker
      int ModIndex;           //Module index
    };

    //Array dimensions
    struct DimensionTable{
      ArrayIndexes DimSize;   //Dimension sizes
      CpuAgx GeomIndex;       //Array geometry index
      int TypIndex;           //Type index that owns this dimension record
      int LnkSymIndex;        //Linker symbol index
    };

    //Datatypes table
    struct TypeTable{
      String Name;             //Data type name
      MasterType MstType;      //Master type
      ScopeDef Scope;          //Scope
      SubScopeDef SubScope;    //SubScope
      bool IsTypedef;          //Is type comming from deftype sentence?
      int OrigTypIndex;        //Original type when type comes from a typedef sentence
      bool IsSystemDef;        //Is type defined by system?
      int DimNr;               //Number of dimensions
      int ElemTypIndex;        //Data type of array element
      int DimIndex;            //Array dimension table index
      int FieldLow;            //Field low in member table (class variables)
      int FieldHigh;           //Field high in member table (class variables)
      int MemberLow;           //Function member low in function table (class variables)
      int MemberHigh;          //Function member high in function table (class variables)
      CpuWrd Length;           //Type size
      CpuInt EnumCount;        //Current enum field count
      CpuWrd AuxFieldOffset;   //Not data, only used for field offset calculation
      CpuAdr MetaName;         //Global litteral value address that holds type name metadata
      CpuAdr MetaStNames;      //Global litteral value address that holds class field names array
      CpuAdr MetaStTypes;      //Global litteral value address that holds class field types array
      String DlName;           //Dynamic library name
      String DlType;           //Dynamic library type name
      int LnkSymIndex;         //Linker symbol index
      int DbgSymIndex;         //Debug symbol index
    };
    
    //Variable table
    struct VariableTable{
      String Name;          //Variable name
      int TypIndex;         //Data type
      ScopeDef Scope;       //Scope
      CpuLon CodeBlockId;   //Local code block
      CpuLon FlowLabel;     //Local flow operator label
      CpuAdr Address;       //Memory address
      bool IsConst;         //Is constant ?
      bool IsComputed;      //Is Computed ? (indicates constants that have a value determined at compile time)
      bool IsStatic;        //Is static ?
      bool IsParameter;     //Is function parameter ?
      bool IsReference;     //Is variable a reference ?
      bool IsTempVar;       //Is a temporary variable ?
      bool IsLitVar;        //Is a litteral value variable ?
      bool IsSystemDef;     //Is variable defined by system ?
      bool IsTempLocked;    //Is temporary variable used ? (controls reuse of temporary variables)
      bool IsSourceUsed;    //Is variable used as source in expressions ?
      bool IsInitialized;   //Is variable initialized ?
      bool IsHidden;        //Is variable hidden ? (out of code block scope)
      CpuAdr MetaName;      //Global litteral value address that holds var name metadata
      int LnkSymIndex;      //Linker symbol index
      int DbgSymIndex;      //Debug symbol index
      int LineNr;           //Declaration source line
      int ColNr;            //Declaration column number
      String SourceLine;    //Declaration source line
    };

    //Field table
    struct FieldTable{
      String Name;          //Field name
      int TypIndex;         //Data type
      SubScopeDef SubScope; //Belonging subscope
      bool IsStatic;        //Is field static ?
      CpuWrd Offset;        //Field offset from start address
      CpuInt EnumValue;     //Value in case of enumerated field
      int LnkSymIndex;      //Linker symbol index
      int DbgSymIndex;         //Debug symbol index
    };
     
    //Function table
    struct FunctionTable{
      FunctionKind Kind;      //Function kind
      String Name;            //Function name
      String FullName;        //Unique function name
      String Fid;             //Function id (mangled name)
      ScopeDef Scope;         //Scope
      SubScopeDef SubScope;   //Belonging subscope
      CpuAdr Address;         //Code address
      int TypIndex;           //Return data type
      bool IsVoid;            //Is function returning void?
      bool IsNested;          //Is nested function?
      bool IsDefined;         //Is function defined already?
      bool IsInitializer;     //Is initializer function? (sets class initial value) 
      bool IsMetaMethod;      //Is a master method that returns metadata? (.name() .dtype() ...) 
      int ParmNr;             //Number of function parameters
      int ParmLow;            //Parameter low in parameter table
      int ParmHigh;           //Parameter high in parameter table
      MasterType MstType;     //Corresponding master type (master method only)
      MasterMethod MstMethod; //Master method identifier (master method only)
      int SysCallNr;          //SystemCall number (system call only)
      CpuInstCode InstCode;   //Instruction code for system function (system function only)
      String DlName;          //Dynamic library name
      String DlFunction;      //Dynamic library function
      int LnkSymIndex;        //Linker symbol index
      int DbgSymIndex;        //Debug symbol index
      int LineNr;             //Declaration source line
      int ColNr;              //Declaration column number
      String SourceLine;      //Declration source line
    };
    
    //Parameter table
    struct ParameterTable{
      String Name;          //Parameter name
      int TypIndex;         //Data type table index
      bool IsConst;         //Is constant ?
      bool IsReference;     //Is parameter a reference ?
      int ParmOrder;        //Parameter order number in function argument list
      int FunIndex;         //Function index to which this parameter belongs to
      CpuAdr Address;       //Corresponding address of parameter variable after copying parameters into function scope (only local functions)
      int LnkSymIndex;      //Linker symbol index
      int DbgSymIndex;      //Debug symbol index
      int LineNr;           //Declaration source line
      int ColNr;            //Declaration column number
      String SourceLine;    //Declration source line
    };

    //Grants table
    struct GrantTable{
      GrantKind Kind;       //Grant kind
      int TypIndex;         //Type index on which grant is defined
      String FrTypName;     //From class name
      String FrFunName;     //From function name
      int ToFldIndex;       //To field index
      int ToFunIndex;       //To function member index
      int FrLineNr;         //Line number corresponding to from object
      int FrColNr;          //Column number corresponding to from object
    };

    //Master attributes
    bool InitializeVars;   //Produce default initialization for all declared variables
    bool CompileToLibrary; //Program is compiled to library
    bool DebugSymbols;     //Generate debug symbols on binary file
    String DynLibPath;     //Path for dynamiclibraries

    //Compiler tables
    Array<ModuleTable> Modules;       //Module table
    Array<TrackerTable> Trackers;     //Module tracker table
    Array<DimensionTable> Dimensions; //Array dimensions table
    Array<TypeTable> Types;           //Data types table
    Array<VariableTable> Variables;   //Variable table
    Array<FieldTable> Fields;         //Fields table
    Array<FunctionTable> Functions;   //Function table
    Array<ParameterTable> Parameters; //Parameter table
    Array<GrantTable> Grants;         //Grants table
  
    //Binary file handler class
    Binary Bin;

    //Bas ic type indexes
    int BolTypIndex; //boolean
    int ChrTypIndex; //char
    int ShrTypIndex; //short
    int IntTypIndex; //integer
    int LonTypIndex; //long
    int FloTypIndex; //float
    int StrTypIndex; //string
    int ChaTypIndex; //char[]
    int StaTypIndex; //string[]
    int WrdTypIndex; //word

    //Master data functions
    void SetMainDefinitions();
    String GetStartFunctionName() const;
    String GetInitFunctionName(const String& Module) const;
    bool IsDelayedInitFunction(const String& FunctionName) const;
    String GetDelayedInitFunctionName(const String& Module) const;
    String GetSuperInitFunctionName() const;
    String GetMainFunctionName() const;
    String GetFuncResultName() const;
    String GetElementTypePrefix() const;
    String GetSelfRefName() const;
    String GetOperatorFuncName(const String& Opr) const;
    String GetStaticVarName(int FunIndex,const String& VarName) const;
    String GetStaticFldName(int FldIndex) const;
    String GetComplexLitValuePrefix() const;
    bool IsComplexLitVariable(int VarIndex) const;
    int MainModIndex() const;
    bool IsInitRoutineStarted();
    void InitRoutineReset();
    void InitRoutineStart();
    bool InitRoutineEnd();
    void InitScopeStack();
    bool OpenScope(ScopeKind Kind,int ModIndex,int FunIndex,CpuLon CodeBlockId);
    bool CloseScope(ScopeKind Kind,bool CompileToLibrary,const Sentence& Stn);
    void DeleteScope();
    String CurrentScopeStack() const;
    String CurrentModule() const;
    const ScopeDef& CurrentScope() const;
    const ScopeDef& ParentScope() const;
    String ScopeDescription(const ScopeDef& Scope) const;
    String ScopeName(const ScopeDef& Scope) const;
    int ScopeFindIndex(const ScopeDef& Scope) const;
    ScopeDef InnerPrivScope() const;
    bool SubScopeBegin(SubScopeKind Kind,int TypIndex);
    bool SubScopeEnd();
    const SubScopeDef& CurrentSubScope() const;
    String SubScopeName(const SubScopeDef& Scope) const;
    void IncreaseLabelGenerator();
    long GetLabelGenerator();
    void ResetLabelGenerator();
    void InitLabelGenerator();
    void IncreaseFlowLabelGenerator();
    CpuLon GetFlowLabelGenerator();
    void ResetFlowLabelGenerator();
    void InitFlowLabelGenerator();
    String FunctionKindName(FunctionKind Kind) const;
    bool AreAllFieldsVisible(const ScopeDef& Scope,int ClassTypIndex) const;
    bool IsMemberVisible(const ScopeDef& Scope,int ClassTypIndex,int FldIndex,int FunIndex) const;
    bool TypeDetach(int TypIndex);
    bool DotCollissionCheck(const String& Idn,String& FoundObject) const;
    int ModSearch(const String& Name) const;
    int TrkSearch(const String& Name) const;
    int TypSearch(const String& Name,int ModIndex) const;
    int VarSearch(const String& Name,int ModIndex,bool FindHidden=false,int *SearchIndex=nullptr) const;
    int FldSearch(const String& Name,int TypIndex) const;
    int ParSearch(const String& Name,int FunIndex) const;
    int GfnSearch(const String& SearchName,const String& Parms) const;
    int FunSearch(const String& Name,int ModIndex,const String& Parms,const String& ConvParms,String *Matches,bool ByName=false) const;
    int MmtSearch(const String& Name,MasterType MstType,const String& Parms,const String& ConvParms,String *Matches,bool ByName=false) const;
    int FmbSearch(const String& Name,int ModIndex,int ClassTypIndex,const String& Parms,const String& ConvParms,String *Matches,bool ByName=false) const;
    int OprSearch(const String& Name,const String& Parms,const String& ConvParms,String *Matches,bool ByName=false) const;
    int GraSearch(GrantKind Kind,int TypIndex,const String& FrTypName,const String& FrFunName,int ToFldIndex,int ToFunIndex) const;
    String GetTypeIdentifiers(const ScopeDef& Scope) const;
    bool CopyPublicIndexes(int ScopeIndex);
    void PurgeScopeObjects(const ScopeDef& Scope);
    void CheckUnusedScopeObjects(const ScopeDef& Scope,const SourceInfo& SrcInfo,CpuLon CodeBlockId,CpuLon FlowLabel);
    void CheckUndefinedFunctions(const ScopeDef& Scope,const SourceInfo& SrcInfo);
    String GrantFrName(GrantKind Kind,int TypIndex,const String& FrTypName,const String& FrFunName) const;
    String GrantToName(GrantKind Kind,int TypIndex,int ToFldIndex,int ToFunIndex) const;
    String GrantName(GrantKind Kind,int TypIndex,const String& FrTypName,const String& FrFunName,int ToFldIndex,int ToFunIndex) const;
    String GrantName(int GraIndex) const;
    bool CheckGrants(const ScopeDef& Scope);
    void CreateParmVariables(const ScopeDef& Scope,CpuLon CodeBlockId);
    CpuWrd ArrayElements(int DimNr,ArrayIndexes& DimSize) const;
    bool EquivalentArrays(int TypIndex1,int TypIndex2) const;
    int WordTypeFilter(int TypIndex,bool CurrentArchitecture) const;
    int SystemTypeFilter(int TypIndex) const;
    String CannonicalTypeName(int TypIndex) const;
    void DecoupleTypeName(const String& ParserTypeName,String& TypeName,int& ModIndex) const;
    bool IsMasterAtomic(MasterType MstType) const;
    bool IsMasterAtomic(int TypIndex) const;
    bool IsEmptyClass(int TypIndex) const;
    bool IsStaticClass(int TypIndex) const;
    CpuWrd TypLength(int TypIndex) const;
    CpuWrd VarLength(int VarIndex) const;
    void StoreModule(const String& Name,const String& Path,bool IsModLibrary,const SourceInfo& SrcInfo);
    void StoreTracker(const String& Name,int ModIndex);
    void StoreDimension(ArrayIndexes DimSize,CpuAgx GeomIndex);
    void StoreType(const ScopeDef& Scope,const SubScopeDef& SubScope,const String& Name,MasterType MstType,bool IsTypedef,int OrigTypIndex,bool IsSystemDef,CpuWrd Length,int DimNr,int ElemTypIndex,int DimIndex,int FieldLow,int FieldHigh,int MemberLow,int MemberHigh,bool CreateMetaInfo,const SourceInfo& SrcInfo);
    bool ReuseVariable(const ScopeDef& Scope,CpuLon CodeBlockId,CpuLon FlowLabel,const String& Name,int TypIndex,bool IsStatic,bool IsConst,bool IsReference,const SourceInfo& SrcInfo,const String& SourceLine,int& VarIndex);
    void CleanHidden(const ScopeDef& Scope,const String& Name);
    void StoreVariable(const ScopeDef& Scope,CpuLon CodeBlockId,CpuLon FlowLabel,const String& Name,int TypIndex,bool IsStatic,bool IsConst,bool IsParameter,bool IsReference,bool IsTempVar,bool IsLitVar,bool IsSystemDef,bool BufferStore,bool CreateMetaInfo,const SourceInfo& SrcInfo,const String& SourceLine);
    void StoreField(const SubScopeDef& SubScope,const String& Name,int TypIndex,bool IsStatic,CpuInt EnumValue,const SourceInfo& SrcInfo);
    void StoreRegularFunction(const ScopeDef& Scope,const String& Name,int TypIndex,bool IsVoid,bool IsNested,const SourceInfo& SrcInfo,const String& SourceLine);
    void StoreSystemCall(const ScopeDef& Scope,const String& Name,int TypIndex,bool IsVoid,int SysCallNr,const SourceInfo& SrcInfo);
    void StoreSystemFunction(const ScopeDef& Scope,const String& Name,int TypIndex,bool IsVoid,CpuInstCode InstCode,const SourceInfo& SrcInfo);
    void StoreDlFunction(const ScopeDef& Scope,const String& Name,int TypIndex,bool IsVoid,const String& DlName,const String& DlFunction,const SourceInfo& SrcInfo);
    void StoreMasterMethod(const String& Name,int TypIndex,bool IsVoid,bool IsInitilizer,bool IsMetaMethod,MasterType MstType,MasterMethod MstMethod);
    void StoreMemberFunction(const SubScopeDef& SubScope,const String& Name,int TypIndex,bool IsVoid,bool IsInitializer,const SourceInfo& SrcInfo,const String& SourceLine);
    void StoreOperatorFunction(const ScopeDef& Scope,const String& Name,int TypIndex,bool IsVoid,bool IsNested,const SourceInfo& SrcInfo,const String& SourceLine);
    void StoreParameter(int FunIndex,const String& Name,int TypIndex,bool IsConst,bool IsReference,int ParmOrder,bool ModifyFunParmIndexes,const SourceInfo& SrcInfo,const String& SourceLine);
    void StoreGrant(GrantKind Kind,int TypIndex,const String& FrTypName,const String& FrFunName,int ToFldIndex,int ToFunIndex,int FrLineNr,int FrColNr);
    void HideLocalVariables(const ScopeDef& Scope,const Array<CpuLon>& ClosedBlocks,CpuLon FlowLabel,const SourceInfo& SrcInfo);
    String ConvertibleTypeName(int TypIndex) const;
    String FunctionSearchName(int FunIndex) const;
    String GetFunctionParameterList(int FunIndex,bool UseConvertibleTypes) const;
    void StoreFunctionSearchIndex(int FunInenx,int ScopeStkIndex=-1);
    void DeleteFunction(int FunIndex);
    String GetParentFunctionName(int FunIndex) const;
    void ModifyNestedFunctionAddresses(int FunIndex);
    void StoreFunctionId(int FunIndex);
    bool DlParameterMatch(int FunIndex,const String& DlName,bool Result,int ParmIndex,const String& DlType,const SourceInfo& SrcInfo);
    bool DlFunctionCheck(int FunIndex,const SourceInfo& SrcInfo);
    bool DlTypeCheck(const String& TypeName,const String& DlName,const SourceInfo& SrcInfo);
    int StoreSystemInteger(const ScopeDef& Scope,CpuLon CodeBlockId,const String& VarName,CpuInt Value);
    int StoreSystemString(const ScopeDef& Scope,CpuLon CodeBlockId,const String& VarName,const String& Value);
    CpuAdr StoreLitString(const String& Value);
    CpuAdr StoreLitStringArray(Array<String>& Values);
    bool TempVarNew(const ScopeDef& Scope,CpuLon CodeBlockId,int TypIndex,bool IsReference,const SourceInfo& SrcInfo,TempVarKind TempKind,int& VarIndex);
    void TempVarLock(int VarIndex);
    void TempVarUnlock(int VarIndex);
    void TempVarUnlockAll();
    bool HasInnerBlocks(int TypIndex);
    CpuDataType CpuDataTypeFromMstType(MasterType MstType) const;
    bool UndefinedObject(int ModIndex) const;
    int GetSourceType(int TypIndex) const;
    AsmArg AsmPar(int ParmIndex) const;
    AsmArg AsmVar(int VarIndex) const;
    AsmArg AsmVad(int VarIndex) const;
    AsmArg AsmNva() const;
    AsmArg AsmInd(int VarIndex) const;
    AsmArg AsmMta(MetaConstCase Case,int Index) const;
    AsmArg AsmAgx(int TypIndex) const;
    AsmArg AsmFun(int FunIndex) const;
    AsmArg AsmSub(CpuAdr Address,const String& Module,const String& Name,int ScopeDepth) const;
    AsmArg AsmJmp(const String& Label) const;
    AsmArg AsmErr() const;
    void AsmWriteFunction(int FunIndex);
    void AsmFunDeclaration(int FunIndex,bool Import,bool Export,const String& Library);
    void AsmInitHeader(int ModIndex);
    void AsmExports(void);
    void AsmDlCallIds(void);
    bool GenerateSuperInitRoutine();
    bool GenerateStartCode();
    int FieldLoop(int TypIndex,int FldIndex) const;  
    int FieldCount(int TypIndex) const;  
    int MemberLoop(int TypIndex,int FunIndex) const;  
    Sentence ReplaceStaticVarNames(const ScopeDef& Scope,const Sentence& Stn) const;

  //Public members
  public:

    //Constructor / Destructor
    MasterData();
    ~MasterData();

};

//Inline functions
inline MasterType WordMasterType(){
  return GetArchitecture()==32?MasterType::Integer:MasterType::Long;
}

//Declarations
String MasterTypeName(MasterType MstType);
String GrantKindShortName(GrantKind Kind);

#endif   