//Parser.hpp: Header file for parser class
#ifndef _PARSER_HPP
#define _PARSER_HPP

//Constants seen outsize
extern const char _TypeIdSeparator;
extern const String _KwdInitVar;

//Labels for keywords, Operators, Punctuators, ParserSymbols, ParserTokens and Sentences
enum class PrKeyword:int{
  Libs=0,Public,Private,Implem,Set,Import,Include,As,Version,Static,Var,Const,DefType,DefClass,Publ,Priv,EndClass,Allow,To,From,DefEnum,EndEnum,
  Void,Main,EndMain,Function,EndFunction,Member,EndMember,Operator,EndOperator,Let,Init,Return,Ref,If,ElseIf,Else,EndIf,While,EndWhile,Do,Loop,For,EndFor,
  Walk,EndWalk,On,Switch,When,Default,EndSwitch,Break,Continue,Array,Index,SystemCall,SystemFunc,DlFunction,DlType,XlvSet,InitVar
};
enum class PrOperator:int{
  PrefixIncrement=0,PrefixDecrement,Plus,Minus,ShlAssign,ShrAssign,PostfixIncrement,PostfixDecrement,ShiftLeft,ShiftRight,LessEqual,GreaterEqual,
  Equal,Distinct,LogicalAnd,LogicalOr,AddAssign,SubAssign,MulAssign,DivAssign,ModAssign,AndAssign,XorAssign,OrAssign,SeqOper,TernaryIf,
  Member,LogicalNot,BitwiseNot,Asterisk,Division,Modulus,Add,Sub,Less,Greater,BitwiseAnd,BitwiseXor,BitwiseOr,Assign
};
enum class PrPunctuator:int{ 
  BegParen=0,EndParen,BegBracket,EndBracket,BegCurly,EndCurly,Comma,Colon,Splitter
};
enum class PrTokenId{
  Keyword,Operator,Punctuator,TypeName,Identifier,Boolean,Char,Short,Integer,Long,Float,String
};
enum class SentenceId{
  Libs=0,Public,Private,Implem,Set,Import,Include,Const,VarDecl,DefType,DefClass,Publ,Priv,EndClass,Allow,DefEnum,EnumField,EndEnum,
  FunDecl,Main,EndMain,Function,EndFunction,Member,EndMember,Operator,EndOperator,Return,If,ElseIf,Else,EndIf,While,EndWhile,Do,Loop,
  For,EndFor,Walk,EndWalk,Switch,When,Default,EndSwitch,Break,Continue,Expression,SystemCall,SystemFunc,DlFunction,DlType,XlvSet,InitVar,Empty
};

//Label codes
enum class CodeLabelId{
  NextBlock, //Next block
  LoopBeg,   //Loop beginning
  LoopEnd,   //Loop ending
  LoopExit,  //Most inner loop exit label
  LoopNext,  //Most inner loop end label
  CurrCond,  //Current condition
  PrevCond,  //Prev condition
  NextCond,  //Next condition
  Exit       //Exit label
};

//Code blocks
enum class CodeBlock:CpuInt{
  Init      = 0x000001, //Init block
  Libs      = 0x000002, //Libraries block
  Public    = 0x000004, //Public block
  Private   = 0x000008, //Private block
  Implem    = 0x000010, //Implementation block
  Local     = 0x000020, //Local scope
  Class     = 0x000040, //Class definition
  Publ      = 0x000080, //Public class section
  Priv      = 0x000100, //Private class section
  Enum      = 0x000200, //Enum definition
  Switch    = 0x000400, //Switch scope
  FirstWhen = 0x000800, //First case scope
  NextWhen  = 0x001000, //Next case scope
  Default   = 0x002000, //Default scope
  DoLoop    = 0x004000, //Do loop scope
  While     = 0x008000, //While loop scope
  If        = 0x010000, //If scope
  ElseIf    = 0x020000, //ElseIf scope
  Else      = 0x040000, //Else scope
  For       = 0x080000, //For scope
  Walk      = 0x100000  //Walk scope
};

//Sentence origin buffer
#define PARSER_BUFFERS 4
enum class OrigBuffer{
  Source   =0,
  Split    =1,
  Insertion=2,
  Addition =3
};

//Code block definition
struct CodeBlockDef{
  CodeBlock Block;
  CpuShr BaseLabel;
  CpuShr SubLabel;
};

//Token value
union PrTokenValue{
  
  //Union fields
  PrKeyword Kwd;
  PrOperator Opr;
  PrPunctuator Pnc;
  String Idn;
  String Typ;
  CpuBol Bol; 
  CpuChr Chr; 
  CpuShr Shr; 
  CpuInt Int; 
  CpuLon Lon; 
  CpuFlo Flo;
  String Str;
  
  //Constructors/Destructors (assignment and copy constructor never used)
  PrTokenValue(){}
  ~PrTokenValue(){}

};

//Parser token
class PrToken{
  
  //Private 
  private:
    
    //Private fields
    PrTokenId _Id;

    //Functions
    void _DestroyId();
    void _Move(const PrToken& Token);

  //Public
  public:

    PrTokenValue Value;
    String FileName;
    int LineNr;
    int ColNr;
  
    //Functions
    void Id(PrTokenId Id);
    inline PrTokenId Id() const { return _Id; };
    SysMessage Msg(int Code) const;
    String Description() const;
    String Text() const;
    String Print() const;
    void Clear();
    SourceInfo SrcInfo() const;
  
    //Constructors/Destructors and assignment
    PrToken();
    ~PrToken();
    PrToken(const PrToken& Token);
    PrToken& operator=(const PrToken& Token);
};

//Parser sentence class
class Sentence{
  
  //Private menbers
  private:
    
    //Sentence token processing modes
    enum class SentenceProcMode:int{ Read, Get };

    //Internal variables
    String _FileName;          //Source file being parsed
    int _LineNr;               //Number of line being parsed
    int _ProcPos;              //Processing token position
    bool _ProcError;           //Processing error
    long _BaseLabel;           //Base label for obtaining code labels
    long _SubLabel;            //Sub-label for obtaining code labels
    String _BlockId;           //Jump block id
    long _LoopLabel;           //First outer loop base label
    String _LoopId;            //First outer loop block id
    bool _EnableSysNameSpace;  //Enable parsing identifiers in system name space
    OrigBuffer _Origin;        //Buffer sentence comes from
    CpuLon _CodeBlockId;       //Code block id

    //Internal functions
    bool _ValidIdChar(char c) const;
    bool _IsKeyWord(const String& Line,int ColNr,OrigBuffer Origin,PrKeyword& Kwd,int& Length) const;
    bool _IsOperator(const String& Line,int ColNr,PrOperator& Opr,int& Length) const;
    bool _IsPunctuator(const String& Line,int ColNr,PrPunctuator& Pnc,int& Length) const;
    bool _IsTypeName(const String& Line,const Array<String>& TypeList,int ColNr,String& Typ,int& Length) const;
    bool _IsIdentifier(const String& Line,int ColNr,int BaseColNr,String& Idn,int& Length, bool& Error) const;
    bool _IsBoolean(const String& Line,int ColNr,CpuBol& Bol,int& Length) const;
    bool _IsChar(const String& Line,int ColNr,int BaseColNr,CpuChr& Chr,int& Length, bool& Error) const;
    bool _IsNumber(const String& Line,int ColNr,int BaseColNr,PrToken& NumToken,int& Length, bool& Error) const;
    bool _IsFloat(const String& Line,int ColNr,int BaseColNr,CpuFlo& Flo,int& Length, bool& Error) const;
    bool _IsString(const String& Line,int ColNr,int BaseColNr,String& Str,int& Length, bool& Error) const;
    bool _IsRawString(const String& Line,int ColNr,int BaseColNr,String& Str,int& Length, bool& Error) const;
    bool _GetToken(const String& Line,const Array<String>& TypeList,int CumulLen,OrigBuffer Origin,int AtPos,int& EndPos, PrToken& Token) const;
    bool _GetSentenceModifiers(const String& Line,const Array<String>& TypeList,int CumulLen,OrigBuffer Origin,PrToken& Static,PrToken& Let,PrToken& Init,int& EndPos) const;
    bool _GetSentenceId(CodeBlock Block,SentenceId& StnId);
    Sentence& _ProcToken(PrToken& Token,SentenceProcMode Mode);

  //Public members
  public:
    
    //Sentence modifiers
    bool Static;
    bool Let;
    bool Init;

    //Parsed sentence data
    SentenceId Id;
    Array<PrToken> Tokens;
  
    //Token parsing functions
    bool Ok() const;                                        //Returns processing error
    void ClearError();                                      //Clear processing error
    int  TokensLeft() const;                                //Returns number of tokens unprocessed
    int  GetProcIndex() const;                              //Return current processing position
    void SetProcIndex(int Index);                           //Sets current processing position
    int  CurrentLineNr() const;                             //Returns line number of current token
    int  CurrentColNr() const;                              //Returns column index of current token
    int  LastLineNr() const;                                //Returns line number of last processed token
    int  LastColNr() const;                                 //Returns column index of last processed token
    SourceInfo LastSrcInfo() const;                         //Source information for last processed token
    int ZeroFind(PrPunctuator Pnc,int FromIndex) const;     //Find punctuator in level zero from arbitrary position
    int ZeroFind(PrPunctuator Pnc) const;                   //Find punctuator in level zero from processing position
    PrToken CurrentToken() const;                           //Returns current toke at processing position                       
    bool Is(PrTokenId Id) const;                            //Check token is certain token id keep processing pointer
    bool Is(PrKeyword Kwd) const;                           //Check token is certain keyword is a value and keep processing pointer
    bool Is(PrOperator Opr) const;                          //Check token is certain operator is a value and keep processing pointer
    bool Is(PrPunctuator Pnc) const;                        //Check token is certain punctuator is a value and keep processing pointer
    bool Is(PrTokenId Id,int Offset) const;                 //Check token is certain keyword is a value and keep processing pointer plus offset
    bool Is(PrKeyword Kwd,int Offset) const;                //Check token is certain keyword is a value and keep processing pointer plus offset
    bool Is(PrOperator Opr,int Offset) const;               //Check token is certain operator is a value and keep processing pointer plus offset
    bool Is(PrPunctuator Pnc,int Offset) const;             //Check token is certain punctuator is a value and keep processing pointer plus offset
    bool IsBol() const;                                     //Check token is Bol and keepprocessing pointer
    bool IsChr() const;                                     //Check token is Chr and keepprocessing pointer
    bool IsShr() const;                                     //Check token is Shr and keepprocessing pointer
    bool IsInt() const;                                     //Check token is Int and keepprocessing pointer
    bool IsLon() const;                                     //Check token is Lon and keepprocessing pointer
    bool IsFlo() const;                                     //Check token is Flo and keepprocessing pointer
    bool IsStr() const;                                     //Check token is Str and keepprocessing pointer
    Sentence& Get(PrKeyword Kwd);                           //Check token is certain keyword is a value and move processing pointer
    Sentence& Get(PrOperator Opr);                          //Check token is certain operator is a value and move processing pointer
    Sentence& Get(PrPunctuator Pnc);                        //Check token is certain punctuator is a value and move processing pointer
    Sentence& ReadKw(PrKeyword& Kwd);                       //Get Keyword and move processing pointer
    Sentence& ReadOp(PrOperator& Opr);                      //Get Operator and move processing pointer
    Sentence& ReadPn(PrPunctuator& Pnc);                    //Get PrPunctuator and move processing pointer
    Sentence& ReadTy(String& Typ);                          //Get Type name and move processing pointer
    Sentence& ReadId(String& Idn);                          //Get Identifier and move processing pointer
    Sentence& ReadBo(CpuBol& Bol);                          //Get Boolean and move processing pointer
    Sentence& ReadCh(CpuChr& Chr);                          //Get Char and move processing pointer
    Sentence& ReadSh(CpuShr& Shr);                          //Get Short and move processing pointer
    Sentence& ReadIn(CpuInt& Int);                          //Get Integer and move processing pointer
    Sentence& ReadLo(CpuLon& Lon);                          //Get Long and move processing pointer
    Sentence& ReadFl(CpuFlo& Flo);                          //Get Float and move processing pointer
    Sentence& ReadSt(String& Str);                          //Get String and move processing pointer
    Sentence& ReadEx(PrKeyword Kwd,int& Start,int& End);    //Get expression till keyword is found
    Sentence& ReadEx(PrOperator Opr,int& Start,int& End);   //Get expression till operator is found
    Sentence& ReadEx(PrPunctuator Pnc,int& Start,int& End); //Get expression till punctuator is found
    Sentence& ReadEx(int& Start,int& End);                  //Get expression till end of sentence
    Sentence& Count(PrPunctuator Pnc,int& Nr);              //Counts and get number of consecutive given punctuators
    
    //Sentence manipulation
    Sentence SubSentence(int Start,int End) const;                        //Return part of a sentence
    Sentence& Add(PrOperator Opr);                                        //Add operator token to sentence
    Sentence& Add(PrPunctuator Pnc);                                      //Add punctuator token to sentence
    Sentence& Add(CpuChr Number);                                         //Add integer token to sentence
    Sentence& Add(CpuInt Number);                                         //Add integer token to sentence
    Sentence& Add(const String& Idn);                                     //Add identifier token to sentence
    Sentence& InsKwd(PrKeyword Kwd,int Position);                         //Insert keyword token to sentence
    Sentence& InsOpr(PrOperator Opr,int Position);                        //Insert operator token to sentence
    Sentence& InsPnc(PrPunctuator Pnc,int Position);                      //Insert punctuator token to sentence
    Sentence& InsIdn(const String& Idn,int Position);                     //Insert identifier token to sentence
    Sentence& InsTyp(const String& Typ,int Position);                     //Insert type name token to sentence
    friend Sentence operator+(const Sentence& Stn1,const Sentence& Stn2); //Concatenate two sentences

    //Parser
    bool Parse(const String& FileName,int LineNr,const String& Line,int CumulLen,const Array<String>& TypeList,CodeBlock Block,OrigBuffer Origin);
  
    //Rest of functions
    void EnableSysNameSpace(bool Enable);                                                                   //Enable parsing identifiers in system name space
    void SetLabels(long BaseLabel,long SubLabel,const String& BlockId,long LoopLabel,const String& LoopId); //Set labels
    String GetLabel(CodeLabelId LabelId);                                                                   //Get label id
    void SetCodeBlockId(CpuLon CodeBlockId);                                                                //Set code block id
    CpuLon GetCodeBlockId() const;                                                                          //Get code block id
    OrigBuffer Origin();                                                                                    //Return sentence origin
    bool InsideLoop();                                                                                      //Is sentence inside a loop ?
    String Print() const;                                                                                   //Sentence printout
    String Print(int BegToken,int EndToken) const;                                                          //Sentence printout
    String Text() const;                                                                                    //Sentence printout
    String Text(int BegToken,int EndToken) const;                                                           //Sentence printout
    SysMessage Msg(int Code) const;                                                                         //SysMessage over last processed token
    String FileName() const;                                                                                //Get current file name
    int LineNr() const;                                                                                     //Get current line number
  
    //Constructors/Destructors and assignment
    Sentence();
    ~Sentence(){}
    Sentence(const Sentence& Stn);
    Sentence& operator=(const Sentence& Stn);
    void _Move(const Sentence& Stn);

};

//Parser state class
class ParserState{
  
  //Private
  private:
    
    //Private functions
    void _Move(const ParserState& State);
  
  //Public
  public:
    
    //Patser state variables
    long GlobalBaseLabel;          //Base label for obtaining code labels (global variable)
    Stack<CodeBlockDef> CodeBlock; //Code block stack
    Array<CpuLon> ClosedBlocks;    //Closed code block ids
    Stack<CpuLon> DelStack;        //Closed code block ids
    Array<String> TypeList;        //Type ids so type names can beidentified

    //Public functions
    void Reset();

    //Constructors/Destructors and assignment
    ParserState();
    ~ParserState(){}
    ParserState(const ParserState& State);
    ParserState& operator=(const ParserState& State);

};

//Parser class
class Parser {
  
  //Private members
  private:
    
    //Splitter replacements
    struct SplitReplace{
      String Old;
      String New;
    };

    //Code label operations
    enum class LabelOper{ CountUp, CountDown, LevelIns, LevelOut };
    
    //Static state variables (do not change while parsing)
    int _TabSize;                   //Tab size (relevant only to report correct column indexes in error mesages)
    int _BufferNr;                  //Current line number being processed
    int _LineNr;                    //Current line to report 
    int _CumulLen;                  //Cumulated line length read
    String _OrigLine;               //Original source line (before splits)
    String _FileName;               //Source file being parsed
    Array<String> _Buffer;          //Buffer to store source lines
    Array<String> _AutoSplit;       //Strings that produce sentence split
    Array<char> _SplitChars;        //Chars that produce splitting
    Queue<String> _AuxBuffer;       //Split sentence buffer
    Queue<String> _InsBuffer;       //Insertion buffer
    Queue<String> _AddBuffer;       //Addition buffer

    //State variables (ones that can be restored on sentence compilation error)
    ParserState _CurrState;            //Current state
    ParserState _PrevState;            //Previous state (used to cancel state change)

    //Private methods
    bool _GetSourceLine(String& Line,OrigBuffer& Origin,bool& EndOfSource);
    String _PrintCodeBlocks() const;

  //Public members
  public:
    
    //Parser functions
    bool Open(const String& FileName,bool FromStdin,int TabSize=1);  //Opens source file
    bool Get(Sentence& Stn,String& SourceLine,bool& EndOfSource);    //Gets single parsed sentence
    void Insert(const String& Line);                                 //Queue line in the insertion buffer
    void Add(const String& Line,bool AtTop=false);                   //Queue line in the addition buffer
    void Reset();                                                    //Releases memory and initializes state
    void StateBack();                                                //Restore parser state
    CodeBlock CurrentBlock() const;                                  //Get current parser code block
    const Array<CpuLon>& GetClosedBlocks() const;                    //Get closed blocks in parser state
    void ClearClosedBlocks();                                        //Clear closed blocks in parser state
    void SetTypeIds(const String& TypeIds);                          //Set type identifiers
    bool LibraryOptionFound() const;                                 //Look in module header for library option enabled
    
    //Constructors/Destructors and assignment
    Parser();                             //Constructor
    ~Parser(){}                           //Destructor
    Parser(const Parser& Prs);            //Copy constructor
    Parser& operator=(const Parser& Prs); //Assignment operator
    void _Move(const Parser& Prs);        //Move

};

//Other functions
String GetSystemNameSpace();
String CodeBlockJumpId(CodeBlock Block);
String CodeBlockText(CodeBlock Block);
String CodeBlockDefText(CodeBlockDef BlockDef);
CpuLon GetCodeBlockId(CodeBlockDef BlockDef);
CodeBlockDef GetCodeBlockDef(CpuLon CodeBlockId);
String CodeBlockIdName(CpuLon CodeBlockId);
String PrTokenIdText(PrTokenId Id);
String SentenceIdText(SentenceId Id);

#endif