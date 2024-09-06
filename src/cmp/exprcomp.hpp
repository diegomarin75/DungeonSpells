//exprcomp.hpp: Header file for expression compiler class
#ifndef _EXPRCOMP_HPP
#define _EXPRCOMP_HPP
 
//Function call type
enum class ExprCallType:int{
  Function,   //Function call
  Method,     //Class member call
  Constructor //Class constructor call
};

//Expression operators
enum class ExprOperator:int{
  PostfixInc=0  ,   //++      Postfix increment (returns original operand) 
  PostfixDec    ,   //--      Postfix decrement (returns original operand)
  PrefixInc     ,   //++      Prefix increment (returns modified operand)
  PrefixDec     ,   //--      Prefix decrement (returns modified operand)
  UnaryPlus     ,   //+       Unary plus
  UnaryMinus    ,   //-       Unary minus
  LogicalNot    ,   //!       Logical NOT
  BitwiseNot    ,   //~       Bitwise NOT (One's Complement)
  TypeCast      ,   //(type)  Type cast operator
  Multiplication,   //*       Multiplication
  Division      ,   ///       Division
  Modulus       ,   //%       Modulo (remainder)
  Addition      ,   //+       Addition
  Substraction  ,   //-       Subtraction
  ShiftLeft     ,   //<<      Bitwise left shift
  ShiftRight    ,   //>>      Bitwise right shift
  Less          ,   //<       Less than
  LessEqual     ,   //<=      Less than or equal to
  Greater       ,   //>       Greater than
  GreaterEqual  ,   //>=      Greater than or equal to
  Equal         ,   //==      Equal to
  Distinct      ,   //!=      Not equal to
  BitwiseAnd    ,   //&       Bitwise AND
  BitwiseXor    ,   //^       Bitwise XOR (exclusive or)
  BitwiseOr     ,   //|       Bitwise OR (inclusive or)
  LogicalAnd    ,   //&&      Logical AND
  LogicalOr     ,   //||      Logical OR
  Initializ     ,   //=       Initialization of declared variable
  Assign        ,   //=       Direct assignment
  AddAssign     ,   //+=      Assignment by sum
  SubAssign     ,   //-=      Assignment by difference
  MulAssign     ,   //*=      Assignment by product
  DivAssign     ,   ///=      Assignment by quotient
  ModAssign     ,   //%=      Assignment by remainder
  ShlAssign     ,   //<<=     Assignment by bitwise left shift
  ShrAssign     ,   //>>=     Assignment by bitwise right shift
  AndAssign     ,   //&=      Assignment by bitwise AND
  XorAssign     ,   //^=      Assignment by bitwise XOR
  OrAssign      ,   //|=      Assignment by bitwise OR
  SeqOper       ,   //->      Sequence operation (comma operator)
};

//Expression low level operators
enum class ExprLowLevelOpr:int{
  TernaryCond=0, //? Ternary operator question mark
  TernaryMid,    //: Ternary operator semicolon
  TernaryEnd     //) Ternary operator end parenthesys
};

//Expression flow operators
//(operators for on and index are special constants since they are identified by parser function do not go on RPN)
#define FLOW_ARR_ON -2 //On without index afterwrads 
#define FLOW_ARR_OX -3 //On with index afterwards
#define FLOW_ARR_IX -4 //Index
#define FLOW_ARR_IF -5 //If
#define FLOW_ARR_AS -6 //As
enum class ExprFlowOpr:int{
  ForBeg=0,  //for begin
  ForIf,     //for if
  ForDo,     //for do
  ForRet,    //for return
  ForEnd,    //for end
  ArrBeg,    //array begin
  ArrOnvar,  //Array element variable
  ArrIxvar,  //Array index variable
  ArrInit,   //Array loop init
  ArrAsif,   //array as (with if)
  ArrEnd     //array end
};

//Expression delimiters
enum class ExprDelimiter:int{
  BegParen,   //Beginning parenthesys
  EndParen,   //Closing parenthesys
  BegBracket, //Beginning bracket
  EndBracket, //Closing bracket
  BegCurly,   //Begining curly brace
  EndCurly,   //Closing curly brace
  Comma       //Comma separator
};

//Expresion tokens
enum class ExprTokenId:int {
  Operand,      //Expression operand
  UndefVar,     //Undefined variable
  Operator,     //Operator
  LowLevelOpr,  //Low level operator
  FlowOpr,      //Flow operator
  Field,        //Field member
  Method,       //Function member
  Constructor,  //Class constructor call
  Subscript,    //Array subscipt
  Function,     //Function index
  Complex,      //Complex value
  Delimiter,    //Expression delimiter
  VoidRes       //Void function result
};

//Data promotion modes
enum class ExprPromMode:int{ 
  ToResult=1,  //Data promotion to result
  ToMaximun,   //Data promotion to maximun
  ToOther      //Data promotion to other
};

//Operator cases
struct ExprOperCaseRule{
  ExprOperator Operator;
  bool Promotion[2];
  ExprPromMode PromMode;
  MasterType MstPromType;
  MasterType MstResult;
  long OperandCases[2];
};

//Low level operator data struct
struct TernarySeed{
  long LabelSeed;
  int VarIndex;
  bool Reused;
};

//Flow operator parse struct
struct FlowOprAttr{
  ExprFlowOpr Opr;
  CpuLon Label;
};

//Inner / Outer curly brace initializer
enum class CurlyBraceClass{
  Outer, //At array dimension 1 or class
  Inner  //At array dimension >1
};

//Meta const attributes
struct MetaAttr{
  MetaConstCase Case;
  int TypIndex;
  int VarIndex;
};

//Additional attributes for complex lit value processing
//(only filled for curly braces)
struct ComplexAttr{ 
  bool Used;                  //Used flag
  int TypIndex;               //Indicates type for array or class (only on outer curly braces)
  ArrayIndexes DimSize;       //Calculated dimensions for array according to curly braces and commas in complex type (only on outer curly braces)
  CurlyBraceClass CurlyClass; //Type of curly brace
};

//Expression token
class ExprToken{
  
  //Private area
  private:

    //Expression token value
    union ExprTokenValue{  
      
      //Union fields
      ExprOperator Operator;       //Operator
      ExprLowLevelOpr LowLevelOpr; //Low level operator
      ExprFlowOpr FlowOpr;         //Flow operator
      ExprDelimiter Delimiter;     //Delimiter
      int VarIndex;                //Variable index
      String VarName;              //Variable name
      String Function;             //Function name
      String Field;                //Field name
      String Method;               //Method name
      String VoidFuncName;         //Void function name
      int CCTypIndex;              //Class type index (constructor call)
      int ComplexTypIndex;         //Complex value type index
      int DimNr;                   //Number of array dimensions
      CpuInt Enu;                  //For litteral enumerated values
      CpuBol Bol;                  //Boolean
      CpuChr Chr;                  //Char
      CpuShr Shr;                  //Short
      CpuInt Int;                  //Integer
      CpuLon Lon;                  //Long
      CpuFlo Flo;                  //Float         
      CpuAdr Adr;                  //Address (used by litteral strings and arrays)
    
      //Constructors/Destructors (assignment and copy constructor never used)
      ExprTokenValue(){}
      ~ExprTokenValue(){}
    
    };

    //Fields
    ExprTokenId _Id;  //Expresion token Id
    MasterData *_Md;  //Reference to program master tables
    String _FileName; //Current module name
    int _LineNr;      //Current file line number of token
    int _ColNr;       //Current line column number of token
    
    //Members
    void _DestroyId();
    void _Reset(bool ObjectExists);
    void _Move(const ExprToken& Token);
    bool _New(MasterData *Md,const ScopeDef& Scope,CpuLon CodeBlockId,int TypIndex,CpuAdrMode Mode,bool IsReference,bool IsConstant,const SourceInfo& SrcInfo,TempVarKind TempKind);
  
  //Public area
  public:
    
    //Fields
    CpuAdrMode AdrMode;     //Addressing mode
    ExprTokenValue Value;   //Expression token value
    bool IsConst;           //Is token constant ?
    bool IsCalculated;      //Is token calculated (for expression evaluation)
    bool HasInitialization; //Token is a variable declaration that has initialization
    int LitNumTypIndex;     //Type index for litteral value token
    int CastTypIndex;       //Type cast operator casting type
    int CallParmNr;         //Number of parameters for function/method call
    int FunModIndex;        //Module index number related to function calls
    int SourceVarIndex;     //Source variable index (related variable to set up IsSourceUsed flag, when Value.VarIndex does not point to it)
    long LabelSeed;         //Label generator seed (ternary operator)
    CpuLon FlowLabel;       //Label generator seed (flow operators)
    MetaAttr Meta;          //Meta constant attributes
    ArrayIndexes DimSize;   //Dimension sizes of complex value when it is an array

    //Members
    void Clear();
    void Id(ExprTokenId Id);
    inline ExprTokenId Id() const { return _Id; };
    void ThisEnu(MasterData *Md,int EnumTypIndex,CpuInt EnumValue,const SourceInfo& SrcInfo);
    void ThisBol(MasterData *Md,CpuBol Bol,const SourceInfo& SrcInfo);
    void ThisChr(MasterData *Md,CpuChr Chr,const SourceInfo& SrcInfo);
    void ThisShr(MasterData *Md,CpuShr Shr,const SourceInfo& SrcInfo);
    void ThisInt(MasterData *Md,CpuInt Int,const SourceInfo& SrcInfo);
    void ThisLon(MasterData *Md,CpuLon Lon,const SourceInfo& SrcInfo);
    void ThisFlo(MasterData *Md,CpuFlo Flo,const SourceInfo& SrcInfo);
    void ThisWrd(MasterData *Md,CpuWrd Wrd,const SourceInfo& SrcInfo);
    void ThisVar(MasterData *Md,int VarIndex,const SourceInfo& SrcInfo);
    void ThisInd(MasterData *Md,int VarIndex,const SourceInfo& SrcInfo);
    void AsMetaFldNames(MasterData *Md,int TypIndex,const SourceInfo& SrcInfo);
    void AsMetaFldTypes(MasterData *Md,int TypIndex,const SourceInfo& SrcInfo);
    void AsMetaTypName(MasterData *Md,int TypIndex,const SourceInfo& SrcInfo);
    void AsMetaVarName(MasterData *Md,int VarIndex,const SourceInfo& SrcInfo);
    bool NewConst(MasterData *Md,MasterType MstType,const SourceInfo& SrcInfo);
    bool NewConst(MasterData *Md,const ScopeDef& Scope,CpuLon CodeBlockId,int TypIndex,const SourceInfo& SrcInfo);
    bool NewVar(MasterData *Md,const ScopeDef& Scope,CpuLon CodeBlockId,MasterType MstType,const SourceInfo& SrcInfo);
    bool NewVar(MasterData *Md,const ScopeDef& Scope,CpuLon CodeBlockId,MasterType MstType,const SourceInfo& SrcInfo,TempVarKind TempKind);
    bool NewVar(MasterData *Md,const ScopeDef& Scope,CpuLon CodeBlockId,int TypIndex,const SourceInfo& SrcInfo);
    bool NewVar(MasterData *Md,const ScopeDef& Scope,CpuLon CodeBlockId,int TypIndex,const SourceInfo& SrcInfo,bool& Reused);
    bool NewInd(MasterData *Md,const ScopeDef& Scope,CpuLon CodeBlockId,int TypIndex,bool IsConstant,const SourceInfo& SrcInfo,TempVarKind TempKind=TempVarKind::Regular);
    bool NewPtr(MasterData *Md,const ScopeDef& Scope,CpuLon CodeBlockId,int TypIndex,bool IsConstant,const SourceInfo& SrcInfo);
    bool ToBol();
    bool ToChr();
    bool ToShr();
    bool ToInt();
    bool ToLon();
    bool ToFlo();
    bool ToWrd();
    bool ToLitValue(bool SendError=true);
    bool IsComputableOperand();
    bool IsComputableOperator();
    bool CopyValue(char *Ptr);
    void Release();
    void Lock();
    void SetSourceUsed(const ScopeDef& Scope,bool Forced);
    void SetMdPointer(MasterData *Md);
    void SetPosition(const SourceInfo& SrcInfo);
    bool IsLValue() const;
    bool IsInitialized() const;
    bool IsFuncResult() const;
    int TypIndex() const;
    int VarIndex() const;
    MasterType MstType() const;
    bool IsMasterAtomic() const;
    AsmArg Asm(bool KeepReferences=false) const;
    String Print() const;
    SysMessage Msg(int Code) const;
    String Name() const;
    String FileName() const;
    int LineNr() const;     
    int ColNr() const;      
    SourceInfo SrcInfo() const;

    //Constant expression operations
    friend bool OprUnaryPlus(ExprToken& Result,const ExprToken& a,const SourceInfo& SrcInfo);
    friend bool OprUnaryMinus(ExprToken& Result,const ExprToken& a,const SourceInfo& SrcInfo);
    friend bool OprMultiplication(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo);
    friend bool OprDivision(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo);
    friend bool OprAddition(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo);
    friend bool OprSubstraction(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo);
    friend bool OprLess(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo);
    friend bool OprLessEqual(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo);
    friend bool OprGreater(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo);
    friend bool OprGreaterEqual(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo);
    friend bool OprEqual(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo);
    friend bool OprDistinct(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo);
    friend bool OprBitwiseNot(ExprToken& Result,const ExprToken& a,const SourceInfo& SrcInfo);
    friend bool OprModulus(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo);
    friend bool OprShiftLeft(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo);
    friend bool OprShiftRight(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo);
    friend bool OprBitwiseAnd(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo);
    friend bool OprBitwiseXor(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo);
    friend bool OprBitwiseOr(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo);
    friend bool OprLogicalNot(ExprToken& Result,const ExprToken& a,const SourceInfo& SrcInfo);
    friend bool OprLogicalAnd(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo);
    friend bool OprLogicalOr(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo);

    //Constructors/Destructors and assignment
    ExprToken();
    ~ExprToken();
    ExprToken(const ExprToken& Token);
    ExprToken& operator=(const ExprToken& Token);

};

//Flow label kind
enum class ExprFlowLabelKind{ For, Array };

//Flow label stack
struct FlowLabelStack{
  ExprFlowLabelKind Kind;
  CpuLon Label;
  SourceInfo ArraySrcInfo;
  int OnVarIndex;
  int IxVarIndex;
  ExprToken OrigArray;
  ExprToken ResArray;
};

//Expression class
class Expression{

  //Private members
  private:
    
    //Private members
    MasterData *_Md;            //Reference to program master tables
    String _FileName;           //Module name
    int _LineNr;                //Current file line number of token
    Array<ExprToken> _Tokens;   //Expression tokens
    OrigBuffer _Origin;         //Origin of sentence as reported by parser

    //Functions
    bool _HasOperandOnRight(int Index) const;
    bool _HasOperandOnLeft(int Index) const;
    bool _IsLastTokenOperand() const;
    bool _SameOperand(ExprToken Opnd1,ExprToken Opnd2) const;
    bool _IsDataTypePromotionAutomatic(MasterType FrType,MasterType ToType) const;
    bool _CountParameters(const Sentence& Stn,int StartToken,int& ParmCount) const;
    bool _ComplexLitValueTokenize(const Sentence& Stn,int TypIndex,int BegToken,Array<ComplexAttr>& CmplxAttr,int& ReadTokens,int RecurLevel=0) const;
    static ExprOperator _TranslateOperator(PrOperator Opr);
    bool _Tokenize(const ScopeDef& Scope,const Sentence& Stn,int BegToken,int EndToken);
    bool _CheckConsystency() const;
    bool _FlowOperatorParse(const Sentence& Stn,int CurrToken,int EndToken,Array<FlowOprAttr>& FlowOpr) const;
    bool _TernaryOperatorTokenize(const ScopeDef& Scope);
    bool _Infix2RPN();
    bool _InnerBlockReplication(MasterData *Md,const ExprToken& Destin,const ExprToken& Source) const;
    bool _InnerBlockReplicationRecur(MasterData *Md,int Phase,int RecurLevel,CpuWrd CumulOffset,int TypIndex,int ColNr) const;
    bool _InnerBlockInitialization(MasterData *Md,const ExprToken& Destin) const;
    bool _InnerBlockInitializationRecur(MasterData *Md,int Phase,int RecurLevel,CpuWrd CumulOffset,int TypIndex,int ColNr) const;
    Array<int> _GetStaticFields(MasterData *Md,int TypIndex) const;
    bool _CompileDataTypePromotion(const ScopeDef& Scope,CpuLon CodeBlockId,ExprToken& Opnd,MasterType ToMstType) const;
    bool _CompileDataTypePromotion(const ScopeDef& Scope,CpuLon CodeBlockId,ExprToken& Opnd,ExprOperCaseRule& CaseRule,MasterType MstMaxType) const;
    bool _CompileDataTypePromotion(const ScopeDef& Scope,CpuLon CodeBlockId,ExprToken& Opnd,ExprOperCaseRule& CaseRule,MasterType MstMaxType,bool& Computed) const;
    bool _PrepareCompileOperation(const ScopeDef& Scope,CpuLon CodeBlockId,const ExprToken& Opr,ExprToken& Opnd1,ExprToken& Opnd2,ExprToken& Result,bool ForceIsResultFirst) const;
    bool _ComplexValueCall(const ScopeDef& Scope,CpuLon CodeBlockId,int CurrToken,Stack<ExprToken>& OpndStack,ExprToken& Result);
    bool _SubscriptCall(const ScopeDef& Scope,CpuLon CodeBlockId,int CurrToken,Stack<ExprToken>& OpndStack,ExprToken& Result);
    bool _OperatorCall(const ScopeDef& Scope,CpuLon CodeBlockId,int FunIndex,const ExprToken& OprToken,ExprToken& Opnd1,const ExprToken& Opnd2,bool IsOprStackEmpty,ExprToken& Result);
    bool _FunctionMethodCall(const ScopeDef& Scope,CpuLon CodeBlockId,int CurrToken,Stack<ExprToken>& OpndStack,ExprCallType CallType,bool IsOprStackEmpty,ExprToken& Result);
    bool _MasterMethodExecute(const ScopeDef& Scope,CpuLon CodeBlockId,int FunIndex,const ExprToken& FunToken,ExprToken& SelfToken,Array<ExprToken>& ParmTokens,ExprToken& Result);
    bool _LowLevelOperatorCall(const ScopeDef& Scope,CpuLon CodeBlockId,int CurrToken,Stack<ExprToken>& OpndStack,Array<TernarySeed>& Seed,ExprToken& Result);
    bool _FlowOperatorCall(const ScopeDef& Scope,CpuLon CodeBlockId,int CurrToken,Stack<ExprToken>& OpndStack,Stack<FlowLabelStack>& FlowLabel);
    bool _Compile(const ScopeDef& Scope,CpuLon CodeBlockId,bool ResultIsMandatory,ExprToken& Result);
    bool _ComputeDataTypePromotion(const ScopeDef& Scope,ExprToken& Opnd,ExprOperCaseRule& CaseRule,MasterType MstMaxType) const;
    bool _PrepareComputeOperation(const ScopeDef& Scope,const ExprToken& Opr,ExprToken& Opnd1,ExprToken& Opnd2,ExprToken& Result) const;
    bool _ComputeOperation(const ScopeDef& Scope,const ExprToken& Oper,const ExprToken& Opnd1,const ExprToken& Opnd2,ExprToken& Result) const;
    bool _Compute(const ScopeDef& Scope,ExprToken& Result) const;
    bool _Compute(const ScopeDef& Scope,ExprToken& Result,bool TestMode,bool& IsComputable) const;
    bool _IsComputable(const ScopeDef& Scope,bool& IsComp) const;

  //Public members
  public:
    
    //Functions
    String Print() const;
    static bool IsOverloadableOperator(PrOperator Opr);
    bool CompileDataTypePromotion(MasterData *Md,const ScopeDef& Scope,CpuLon CodeBlockId,ExprToken& Opnd,MasterType ToMstType,bool& Computed);
    bool CopyOperand(MasterData *Md,ExprToken& Dest,const ExprToken& Sour) const;
    bool InitOperand(MasterData *Md,ExprToken& Dest,bool& CodeGenerated) const;
    bool Compile(MasterData *Md,const ScopeDef& Scope,Sentence& Stn,int BegToken,int EndToken,ExprToken& Result);
    bool Compile(MasterData *Md,const ScopeDef& Scope,Sentence& Stn,int BegToken,int EndToken);
    bool Compile(MasterData *Md,const ScopeDef& Scope,Sentence& Stn,ExprToken& Result);
    bool Compile(MasterData *Md,const ScopeDef& Scope,Sentence& Stn);
    bool Compile(MasterData *Md,const ScopeDef& Scope,Sentence& Stn,int BegToken,int EndToken,ExprToken& Result,bool& Computed);
    bool Compute(MasterData *Md,const ScopeDef& Scope,Sentence& Stn,int BegToken,int EndToken,ExprToken& Result);

    //Constructors/Destructors and assignment
    Expression(){}
    ~Expression(){}
    Expression(const Expression& Expr);
    Expression& operator=(const Expression& Expr);
};

//Expression token id text
String ExprTokenIdText(ExprTokenId Id);

//Parse type specification
bool ReadTypeSpec(MasterData *Md,Sentence& Stn,ScopeDef Scope,int& TypIndex,bool AsmCommentFixArray=true);

#endif  