//Parser.cpp: Parser class
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

//Parser module private constants
const long _MaxStringLimit=2000000000L;              //Maximun permitted length for string litteral (IsString)
const String _LineJoiner=" \\";                      //Line joiner token (GetSourceLine)
const char _LineSplitter=';';                        //Line splitter token (GeSourceLine)
const char _SplitterMark='#';                        //Splitter mark (to keep splitters in source)
const char _TextQualifier='\"';                      //Text qualifier, delimitation of string litterals (GetSourceLine,IsString)
const char _TextEscapeMarker='\"';                   //Text escape marker, for escaping text qualifier inside a string (IsString)
const String _RawStrBeg="r\"[";                      //Raw string beginnng
const String _RawStrEnd="]\"";                       //Raw string ending
const String _SplitLevelUp="([{";                    //Split level up chars (GetSourceLine)
const String _SplitLevelDown=")]}";                  //Split level up chars (GetSourceLine)
const String _CommentMark="//";                      //Comment mark
const String _BooleanTrue="true";                    //Boolean true value
const String _BooleanFalse="false";                  //Boolean false value
const char _TypeIdSeparator=',';                     //Type identifiers separator char
const int _CodeLabelLen=5;                           //Code label length
const String _KwdXlvSet=SYSTEM_NAMESPACE "xlvset";   //Complex value initialization
const String _KwdInitVar=SYSTEM_NAMESPACE "initvar"; //Initialize variable statement

//String to replace line splitters so they are preserved
const String _SplitPreserve=String(_SplitterMark)+String(_LineSplitter);

//Valid identifier chars
const char *_ValidIdChars="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";

//Code blocks parser actions
enum class CodeBlockAction { Push, Pop, Keep, Replace };

//PrKeyword table
const int _KwdNr=62;
const String _Kwd[_KwdNr]={
  ".libs"          , //PrKeyword::Libs
  ".public"        , //PrKeyword::Public
  ".private"       , //PrKeyword::Private
  ".implem"        , //PrKeyword::Implem
  "set"            , //PrKeyword::Set
  "import"         , //PrKeyword::Import
  "include"        , //PrKeyword::Include
  "as"             , //PrKeyword::As
  "version"        , //PrKeyword::Version
  "static"         , //PrKeyword::Static
  "var"            , //PrKeyword::Var
  "const"          , //PrKeyword::Const
  "type"           , //PrKeyword::DefType
  "class"          , //PrKeyword::DefClass
  ".publ"          , //PrKeyword::Publ
  ".priv"          , //PrKeyword::Priv
  ":class"         , //PrKeyword::EndClass
  "allow"          , //PrKeyword::Allow
  "to"             , //PrKeyword::To
  "from"           , //PrKeyword::From
  "enum"           , //PrKeyword::DefEnum
  ":enum"          , //PrKeyword::EndEnum
  "void"           , //PrKeyword::Void
  "main:"          , //PrKeyword::Main
  ":main"          , //PrKeyword::EndMain
  "func"           , //PrKeyword::Function
  ":func"          , //PrKeyword::EndFunction
  "fmem"           , //PrKeyword::Member
  ":fmem"          , //PrKeyword::EndMember
  "oper"           , //PrKeyword::Operator
  ":oper"          , //PrKeyword::EndOperator
  "let"            , //PrKeyword::Let
  "init"           , //PrKeyword::Init
  "return"         , //PrKeyword::Return
  "ref"            , //PrKeyword::Ref
  "if"             , //PrKeyword::If
  "elif"           , //PrKeyword::ElseIf
  "else"           , //PrKeyword::Else
  ":if"            , //PrKeyword::EndIf
  "while"          , //PrKeyword::While
  ":while"         , //PrKeyword::EndWhile
  "do"             , //PrKeyword::Do
  ":loop"          , //PrKeyword::Loop
  "for"            , //PrKeyword::For
  ":for"           , //PrKeyword::EndFor
  "walk"           , //PrKeyword::Walk
  ":walk"          , //PrKeyword::EndWalk
  "on"             , //PrKeyword::On
  "switch"         , //PrKeyword::Switch
  "when"           , //PrKeyword::When
  "default:"       , //PrKeyword::Default
  ":switch"        , //PrKeyword::EndSwitch
  "break"          , //PrKeyword::Break
  "continue"       , //PrKeyword::Continue
  "array"          , //PrKeyword::Array
  "index"          , //PrKeyword::Index
  "syscall"        , //PrKeyword::SystemCall
  "sysfunc"        , //PrKeyword::SystemFunc
  "dlfunc"         , //PrKeyword::DlFunction
  "dltype"         , //PrKeyword::DlFunction
  _KwdXlvSet       , //PrKeyword::XlvSet
  _KwdInitVar        //PrKeyword::InitVar
};

//Operator table (sorted by operator length)
const int _OprNr=40;
const String _Opr[_OprNr]={
  "(++)"   , //PrOperator::PrefixIncrement   Prefix increment (returns modified argument)
  "(--)"   , //PrOperator::PrefixDecrement   Prefix decrement (returns modified argument)
  "(+)"    , //PrOperator::Plus              Unary plus
  "(-)"    , //PrOperator::Minus             Unary minus
  "<<="    , //PrOperator::ShlAssign         Assignment by bitwise left shift
  ">>="    , //PrOperator::ShrAssign         Assignment by bitwise right shift
  "++"     , //PrOperator::PostfixIncrement  Postfix increment (returns original argument)
  "--"     , //PrOperator::PostfixDecrement  Postfix decrement (returns original argument)
  "<<"     , //PrOperator::ShiftLeft         Bitwise left shift
  ">>"     , //PrOperator::ShiftRight        Bitwise right shift
  "<="     , //PrOperator::LessEqual         Less than or equal to
  ">="     , //PrOperator::GreaterEqual      Greater than or equal to
  "=="     , //PrOperator::Equal             Equal to
  "!="     , //PrOperator::Distinct          Not equal to
  "&&"     , //PrOperator::LogicalAnd        Logical AND
  "||"     , //PrOperator::LogicalOr         Logical OR
  "+="     , //PrOperator::AddAssign         Assignment by sum
  "-="     , //PrOperator::SubAssign         Assignment by difference
  "*="     , //PrOperator::MulAssign         Assignment by product
  "/="     , //PrOperator::DivAssign         Assignment by quotient
  "%="     , //PrOperator::ModAssign         Assignment by remainder
  "&="     , //PrOperator::AndAssign         Assignment by bitwise AND
  "^="     , //PrOperator::XorAssign         Assignment by bitwise XOR
  "|="     , //PrOperator::OrAssign          Assignment by bitwise OR
  "->"     , //PrOperator::SeqOper           Sequence operation (comma operator)
  "?"      , //PrOperator::TernaryIf         Ternary if
  "."      , //PrOperator::Member            Class member addressing
  "!"      , //PrOperator::LogicalNot        Logical NOT
  "~"      , //PrOperator::BitwiseNot        Bitwise NOT
  "*"      , //PrOperator::Asterisk          Asterisk
  "/"      , //PrOperator::Division          Division
  "%"      , //PrOperator::Modulus           Modulo (remainder)
  "+"      , //PrOperator::Add               Addition
  "-"      , //PrOperator::Sub               Subtraction
  "<"      , //PrOperator::Less              Less than
  ">"      , //PrOperator::Greater           Greater than
  "&"      , //PrOperator::BitwiseAnd        Bitwise AND
  "^"      , //PrOperator::BitwiseXor        Bitwise XOR (exclusive or)
  "|"      , //PrOperator::BitwiseOr         Bitwise OR (inclusive or)
  "="        //PrOperator::Assign            Direct assignment
};

//PrPunctuator table
const int _PncNr=9;
const String _Pnc[_PncNr]={
  "(",          //PrPunctuator::BegParen   ( Beginning parenthesys
  ")",          //PrPunctuator::EndParen   ) Endding bracket
  "[",          //PrPunctuator::BegBracket [ Beginning parenthesys
  "]",          //PrPunctuator::EndBracket ] Endding bracket
  "{",          //PrPunctuator::BegCurly   { Beginning curly brace
  "}",          //PrPunctuator::EndCurly   } Endding curly brace
  ",",          //PrPunctuator::Comma      , Comma
  ":",          //PrPunctuator::Colon      : Colon
  _LineSplitter //PrPunctuator::Splitter   Line Splitter
};

//Jump mode
enum class JumpMode{
  None,       //Nothing to do
  BlockBeg,   //Block beginning (function, member, operator)
  BlockEnd,   //Block endding (endfunction, endmember, endoperator)
  LoopBeg,    //Loop beginning (for,do,while,walk)
  LoopEnd,    //Loop endding (endfir,loop,endwhile,endwalk)
  FirstCase,  //First case (if,first switch case)
  NextCase,   //Next case (elseif, next switch case)
  LastCase,   //Last case (else, default)
  EndCase,    //End case (endif, endswitch)
};

//Sentence table definition
struct SentenceDef{ 
  SentenceId Id;          //Sentence identifier
  CodeBlockAction Action; //Code block action
  CodeBlock NewBlock;     //New block defined by sentence
  JumpMode Jmp;           //Jump mode
  bool PushDelStack;      //Store block on delete stack
  bool PopDelStack;       //Release block from delete stack
  int AllowedBlocks;      //Granted blocks for sentence
};

//Sentence table (syntax tokens below)
const int _LocalScope=
(int)CodeBlock::Local|(int)CodeBlock::FirstWhen|(int)CodeBlock::NextWhen|(int)CodeBlock::Default|(int)CodeBlock::DoLoop|
(int)CodeBlock::While|(int)CodeBlock::If|(int)CodeBlock::ElseIf|(int)CodeBlock::Else|(int)CodeBlock::For|(int)CodeBlock::Walk;
const int _StnNr=54;
const SentenceDef _Stn[_StnNr]={
// Type                     Action                    NewBlock              Jmp                  PshDel PopDel AllowedBlocks
  {SentenceId::Libs       , CodeBlockAction::Replace, CodeBlock::Libs,      JumpMode::None,      false, false, (int)CodeBlock::Init },
  {SentenceId::Public     , CodeBlockAction::Replace, CodeBlock::Public,    JumpMode::None,      false, false, (int)CodeBlock::Init|(int)CodeBlock::Libs },
  {SentenceId::Private    , CodeBlockAction::Replace, CodeBlock::Private,   JumpMode::None,      false, false, (int)CodeBlock::Init|(int)CodeBlock::Libs|(int)CodeBlock::Public },
  {SentenceId::Implem     , CodeBlockAction::Replace, CodeBlock::Implem,    JumpMode::None,      false, false, (int)CodeBlock::Init|(int)CodeBlock::Libs|(int)CodeBlock::Public|(int)CodeBlock::Private },
  {SentenceId::Set        , CodeBlockAction::Keep,    (CodeBlock)0,         JumpMode::None,      false, false, (int)CodeBlock::Init },
  {SentenceId::Import     , CodeBlockAction::Keep,    (CodeBlock)0,         JumpMode::None,      false, false, (int)CodeBlock::Libs },
  {SentenceId::Include    , CodeBlockAction::Keep,    (CodeBlock)0,         JumpMode::None,      false, false, (int)CodeBlock::Libs },
  {SentenceId::DefType    , CodeBlockAction::Keep,    (CodeBlock)0,         JumpMode::None,      false, false, (int)CodeBlock::Public|(int)CodeBlock::Private|(int)CodeBlock::Local|(int)CodeBlock::Class|(int)CodeBlock::Publ|(int)CodeBlock::Priv},
  {SentenceId::DefClass   , CodeBlockAction::Push,    CodeBlock::Class,     JumpMode::None,      false, false, (int)CodeBlock::Public|(int)CodeBlock::Private|(int)CodeBlock::Local|(int)CodeBlock::Class|(int)CodeBlock::Publ|(int)CodeBlock::Priv},
  {SentenceId::Publ       , CodeBlockAction::Replace, CodeBlock::Publ,      JumpMode::None,      false, false, (int)CodeBlock::Class },
  {SentenceId::Priv       , CodeBlockAction::Replace, CodeBlock::Priv,      JumpMode::None,      false, false, (int)CodeBlock::Class|(int)CodeBlock::Publ },
  {SentenceId::EndClass   , CodeBlockAction::Pop,     (CodeBlock)0,         JumpMode::None,      false, false, (int)CodeBlock::Class|(int)CodeBlock::Publ|(int)CodeBlock::Priv },
  {SentenceId::Allow      , CodeBlockAction::Keep,    (CodeBlock)0,         JumpMode::None,      false, false, (int)CodeBlock::Priv },
  {SentenceId::DefEnum    , CodeBlockAction::Push,    CodeBlock::Enum,      JumpMode::None,      false, false, (int)CodeBlock::Public|(int)CodeBlock::Private|(int)CodeBlock::Local|(int)CodeBlock::Class|(int)CodeBlock::Publ|(int)CodeBlock::Priv},
  {SentenceId::EnumField  , CodeBlockAction::Keep,    (CodeBlock)0,         JumpMode::None,      false, false, (int)CodeBlock::Enum },
  {SentenceId::EndEnum    , CodeBlockAction::Pop,     (CodeBlock)0,         JumpMode::None,      false, false, (int)CodeBlock::Enum },
  {SentenceId::Const      , CodeBlockAction::Keep,    (CodeBlock)0,         JumpMode::None,      false, false, (int)CodeBlock::Public|(int)CodeBlock::Private|(int)CodeBlock::Local},
  {SentenceId::VarDecl    , CodeBlockAction::Keep,    (CodeBlock)0,         JumpMode::None,      false, false, (int)CodeBlock::Public|(int)CodeBlock::Private|_LocalScope|(int)CodeBlock::Class|(int)CodeBlock::Publ|(int)CodeBlock::Priv },
  {SentenceId::FunDecl    , CodeBlockAction::Keep,    (CodeBlock)0,         JumpMode::None,      false, false, (int)CodeBlock::Public|(int)CodeBlock::Private|(int)CodeBlock::Class|(int)CodeBlock::Publ|(int)CodeBlock::Priv },
  {SentenceId::SystemCall , CodeBlockAction::Keep,    (CodeBlock)0,         JumpMode::None,      false, false, (int)CodeBlock::Public|(int)CodeBlock::Private },
  {SentenceId::SystemFunc , CodeBlockAction::Keep,    (CodeBlock)0,         JumpMode::None,      false, false, (int)CodeBlock::Public|(int)CodeBlock::Private },
  {SentenceId::DlFunction , CodeBlockAction::Keep,    (CodeBlock)0,         JumpMode::None,      false, false, (int)CodeBlock::Public|(int)CodeBlock::Private },
  {SentenceId::DlType     , CodeBlockAction::Keep,    (CodeBlock)0,         JumpMode::None,      false, false, (int)CodeBlock::Public|(int)CodeBlock::Private },
  {SentenceId::XlvSet     , CodeBlockAction::Keep,    (CodeBlock)0,         JumpMode::None,      false, false, (int)CodeBlock::Public|(int)CodeBlock::Private|(int)CodeBlock::Local },
  {SentenceId::InitVar    , CodeBlockAction::Keep,    (CodeBlock)0,         JumpMode::None,      false, false, (int)CodeBlock::Local },
  {SentenceId::Main       , CodeBlockAction::Push,    CodeBlock::Local,     JumpMode::BlockBeg,  false, false, (int)CodeBlock::Implem },
  {SentenceId::EndMain    , CodeBlockAction::Pop,     (CodeBlock)0,         JumpMode::BlockEnd,  false, false, (int)CodeBlock::Local  },
  {SentenceId::Function   , CodeBlockAction::Push,    CodeBlock::Local,     JumpMode::BlockBeg,  false, false, (int)CodeBlock::Implem|(int)CodeBlock::Local },
  {SentenceId::EndFunction, CodeBlockAction::Pop,     (CodeBlock)0,         JumpMode::BlockEnd,  false, false, (int)CodeBlock::Local  },
  {SentenceId::Operator   , CodeBlockAction::Push,    CodeBlock::Local,     JumpMode::BlockBeg,  false, false, (int)CodeBlock::Implem|(int)CodeBlock::Local },
  {SentenceId::EndOperator, CodeBlockAction::Pop,     (CodeBlock)0,         JumpMode::BlockEnd,  false, false, (int)CodeBlock::Local  },
  {SentenceId::Member     , CodeBlockAction::Push,    CodeBlock::Local,     JumpMode::BlockBeg,  false, false, (int)CodeBlock::Implem|(int)CodeBlock::Local },
  {SentenceId::EndMember  , CodeBlockAction::Pop,     (CodeBlock)0,         JumpMode::BlockEnd,  false, false, (int)CodeBlock::Local  },
  {SentenceId::Return     , CodeBlockAction::Keep,    (CodeBlock)0,         JumpMode::None,      false, false, _LocalScope },
  {SentenceId::If         , CodeBlockAction::Push,    CodeBlock::If,        JumpMode::FirstCase, false, false, _LocalScope },
  {SentenceId::ElseIf     , CodeBlockAction::Replace, CodeBlock::ElseIf,    JumpMode::NextCase,  false, false, (int)CodeBlock::If|(int)CodeBlock::ElseIf },
  {SentenceId::Else       , CodeBlockAction::Replace, CodeBlock::Else,      JumpMode::LastCase,  false, false, (int)CodeBlock::If|(int)CodeBlock::ElseIf },
  {SentenceId::EndIf      , CodeBlockAction::Pop,     (CodeBlock)0,         JumpMode::EndCase,   false, false, (int)CodeBlock::If|(int)CodeBlock::ElseIf|(int)CodeBlock::Else },
  {SentenceId::While      , CodeBlockAction::Push,    CodeBlock::While,     JumpMode::LoopBeg,   false, false, _LocalScope },
  {SentenceId::EndWhile   , CodeBlockAction::Pop,     (CodeBlock)0,         JumpMode::LoopEnd,   false, false, (int)CodeBlock::While },
  {SentenceId::Do         , CodeBlockAction::Push,    CodeBlock::DoLoop,    JumpMode::LoopBeg,   false, false, _LocalScope },
  {SentenceId::Loop       , CodeBlockAction::Pop,     (CodeBlock)0,         JumpMode::LoopEnd,   false, false, (int)CodeBlock::DoLoop },
  {SentenceId::For        , CodeBlockAction::Push,    CodeBlock::For,       JumpMode::LoopBeg,   false, false, _LocalScope },
  {SentenceId::EndFor     , CodeBlockAction::Pop,     (CodeBlock)0,         JumpMode::LoopEnd,   false, false, (int)CodeBlock::For },
  {SentenceId::Walk       , CodeBlockAction::Push,    CodeBlock::Walk,      JumpMode::LoopBeg,   false, false, _LocalScope },
  {SentenceId::EndWalk    , CodeBlockAction::Pop,     (CodeBlock)0,         JumpMode::LoopEnd,   false, false, (int)CodeBlock::Walk },
  {SentenceId::Switch     , CodeBlockAction::Push,    CodeBlock::Switch,    JumpMode::None,      true,  false, _LocalScope },
  {SentenceId::When       , CodeBlockAction::Replace, CodeBlock::FirstWhen, JumpMode::FirstCase, false, false, (int)CodeBlock::Switch },
  {SentenceId::When       , CodeBlockAction::Replace, CodeBlock::NextWhen,  JumpMode::NextCase,  false, false, (int)CodeBlock::FirstWhen|(int)CodeBlock::NextWhen },
  {SentenceId::Default    , CodeBlockAction::Replace, CodeBlock::Default,   JumpMode::LastCase,  false, false, (int)CodeBlock::FirstWhen|(int)CodeBlock::NextWhen },
  {SentenceId::EndSwitch  , CodeBlockAction::Pop,     (CodeBlock)0,         JumpMode::EndCase,   false, true,  (int)CodeBlock::FirstWhen|(int)CodeBlock::NextWhen|(int)CodeBlock::Default },
  {SentenceId::Break      , CodeBlockAction::Keep,    (CodeBlock)0,         JumpMode::None,      false, false, (int)CodeBlock::If|(int)CodeBlock::ElseIf|(int)CodeBlock::Else|(int)CodeBlock::FirstWhen|(int)CodeBlock::NextWhen|(int)CodeBlock::Default|(int)CodeBlock::DoLoop|(int)CodeBlock::While|(int)CodeBlock::For|(int)CodeBlock::Walk },
  {SentenceId::Continue   , CodeBlockAction::Keep,    (CodeBlock)0,         JumpMode::None,      false, false, (int)CodeBlock::If|(int)CodeBlock::ElseIf|(int)CodeBlock::Else|(int)CodeBlock::FirstWhen|(int)CodeBlock::NextWhen|(int)CodeBlock::Default|(int)CodeBlock::DoLoop|(int)CodeBlock::While|(int)CodeBlock::For|(int)CodeBlock::Walk },
  {SentenceId::Expression , CodeBlockAction::Keep,    (CodeBlock)0,         JumpMode::None,      false, false, _LocalScope }
};

//Internal function declarations
String _JumpModeText(JumpMode JMode);
String _ParserGetCodeLabel(CodeLabelId LabelId,long BaseLabel,long SubLabel,const String& BlockId,long LoopLabel,const String& LoopId);
String _CodeBlockActionText(CodeBlockAction Action);
bool _GetEscapeChar(char EscapeChar,char& Result);
String _EscapeString(const String& Str);

//Destroy token id
void PrToken::_DestroyId(){
  switch(_Id){
    case PrTokenId::Keyword   : Value.Kwd=(PrKeyword)-1; break;
    case PrTokenId::Operator  : Value.Opr=(PrOperator)-1; break;
    case PrTokenId::Punctuator: Value.Pnc=(PrPunctuator)-1; break;
    case PrTokenId::TypeName  : Value.Typ.~String(); break;
    case PrTokenId::Identifier: Value.Idn.~String(); break;
    case PrTokenId::Boolean   : Value.Bol=0; break;
    case PrTokenId::Char      : Value.Chr=0; break;
    case PrTokenId::Short     : Value.Shr=0; break;
    case PrTokenId::Integer   : Value.Int=0; break;
    case PrTokenId::Long      : Value.Lon=0; break;
    case PrTokenId::Float     : Value.Flo=0; break;
    case PrTokenId::String    : Value.Str.~String(); break;
  }
}

//PrToken move
void PrToken::_Move(const PrToken& Token){
  FileName=Token.FileName;
  LineNr=Token.LineNr;
  ColNr=Token.ColNr;
  Id(Token._Id);
  switch(_Id){
    case PrTokenId::Keyword   : Value.Kwd=Token.Value.Kwd; break;
    case PrTokenId::Operator  : Value.Opr=Token.Value.Opr; break;
    case PrTokenId::Punctuator: Value.Pnc=Token.Value.Pnc; break;
    case PrTokenId::TypeName  : Value.Typ=Token.Value.Typ; break;
    case PrTokenId::Identifier: Value.Idn=Token.Value.Idn; break;
    case PrTokenId::Boolean   : Value.Bol=Token.Value.Bol; break;
    case PrTokenId::Char      : Value.Chr=Token.Value.Chr; break;
    case PrTokenId::Short     : Value.Shr=Token.Value.Shr; break;
    case PrTokenId::Integer   : Value.Int=Token.Value.Int; break;
    case PrTokenId::Long      : Value.Lon=Token.Value.Lon; break;
    case PrTokenId::Float     : Value.Flo=Token.Value.Flo; break;
    case PrTokenId::String    : Value.Str=Token.Value.Str; break;
  }
}

//Set token id
//Syntax for strings is weird but that is the way to invoke object constructor for existing object
void PrToken::Id(PrTokenId Id){
  _DestroyId();
  _Id=Id;
  switch(_Id){
    case PrTokenId::Keyword   : Value.Kwd=(PrKeyword)-1; break;
    case PrTokenId::Operator  : Value.Opr=(PrOperator)-1; break;
    case PrTokenId::Punctuator: Value.Pnc=(PrPunctuator)-1; break;
    case PrTokenId::TypeName  : new(&Value.Typ) String(); break;
    case PrTokenId::Identifier: new(&Value.Idn) String(); break;
    case PrTokenId::Boolean   : Value.Bol=0; break;
    case PrTokenId::Char      : Value.Chr=0; break;
    case PrTokenId::Short     : Value.Shr=0; break;
    case PrTokenId::Integer   : Value.Int=0; break;
    case PrTokenId::Long      : Value.Lon=0; break;
    case PrTokenId::Float     : Value.Flo=0; break;
    case PrTokenId::String    : new(&Value.Str) String(); break;
  }
}

//Parser token message
SysMessage PrToken::Msg(int Code) const {
  return SysMessage(Code,FileName,LineNr,ColNr);
}

//Parser token get description
String PrToken::Description() const {
  String Desc;
  switch(_Id){
    case PrTokenId::Keyword   : Desc="keyword \""+_Kwd[(int)Value.Kwd]+"\""; break;
    case PrTokenId::Operator  : Desc="operator \""+_Opr[(int)Value.Opr]+"\""; break;
    case PrTokenId::Punctuator: Desc="punctuator \""+_Pnc[(int)Value.Pnc]+"\""; break;
    case PrTokenId::TypeName  : Desc="type name"; break;
    case PrTokenId::Identifier: Desc="identifier"; break;
    case PrTokenId::Boolean   : Desc="boolean litteral"; break;
    case PrTokenId::Char      : Desc="char litteral"; break;
    case PrTokenId::Short     : Desc="short integer litteral"; break;
    case PrTokenId::Integer   : Desc="integer litteral"; break;
    case PrTokenId::Long      : Desc="long integer litteral"; break;
    case PrTokenId::Float     : Desc="floating point litteral"; break;
    case PrTokenId::String    : Desc="string litteral"; break;
  }
  return Desc;
}

//Parser token get text
String PrToken::Text() const {
  String Text;
  switch(_Id){
    case PrTokenId::Keyword   : Text=_Kwd[(int)Value.Kwd]; break;
    case PrTokenId::Operator  : Text=_Opr[(int)Value.Opr]; break;
    case PrTokenId::Punctuator: Text=_Pnc[(int)Value.Pnc]; break;
    case PrTokenId::TypeName  : Text=Value.Typ; break;
    case PrTokenId::Identifier: Text=Value.Idn; break;
    case PrTokenId::Boolean   : Text=(Value.Bol?"true":"false"); break;
    case PrTokenId::Char      : Text=(Value.Chr<32?ToString(Value.Chr)+"R":"'"+_EscapeString(String(Value.Chr))+"'"); break;
    case PrTokenId::Short     : Text=ToString(Value.Shr)+"S"; break;
    case PrTokenId::Integer   : Text=ToString(Value.Int); break;
    case PrTokenId::Long      : Text=ToString(Value.Lon)+"L"; break;
    case PrTokenId::Float     : Text=ToString(Value.Flo); break;
    case PrTokenId::String    : Text="\""+_EscapeString(Value.Str)+"\""; break;
  }
  return Text;
}

//Parser token get printout
String PrToken::Print() const {
  String Output;
  switch(_Id){
    case PrTokenId::Keyword   : Output="kw("+Text()+")"; break;
    case PrTokenId::Operator  : Output="op("+Text()+")"; break;
    case PrTokenId::Punctuator: Output="pu("+Text()+")"; break;
    case PrTokenId::TypeName  : Output="ty("+Text()+")"; break;
    case PrTokenId::Identifier: Output="id("+Text()+")"; break;
    case PrTokenId::Boolean   : Output="bo("+Text()+")"; break;
    case PrTokenId::Char      : Output="ch("+Text()+")"; break;
    case PrTokenId::Short     : Output="sh("+Text()+")"; break;
    case PrTokenId::Integer   : Output="in("+Text()+")"; break;
    case PrTokenId::Long      : Output="lo("+Text()+")"; break;
    case PrTokenId::Float     : Output="fl("+Text()+")"; break;
    case PrTokenId::String    : Output="st("+Text()+")"; break;
  }
  if(_Id==(PrTokenId)-1){ Output="init"; }
  return Output;
}

//Parser token clear
void PrToken::Clear(){
  FileName="";
  LineNr=0;
  ColNr=-1;
  _Id=(PrTokenId)-1;
}

//Source information
SourceInfo PrToken::SrcInfo() const {
  return SourceInfo(FileName,LineNr,ColNr);
}

//Parser token constructor
PrToken::PrToken(){
  Clear();
}

//Parser token destructor
PrToken::~PrToken(){
  _DestroyId();
}

//PrToken copy
PrToken::PrToken(const PrToken& Token){
  FileName="";
  LineNr=0;
  ColNr=-1;
  _Id=(PrTokenId)-1;
  _Move(Token);
}

//PrToken assigment
PrToken& PrToken::operator=(const PrToken& Token){
  _Move(Token);
  return *this;
}

//Check char is valid for identifier
bool Sentence::_ValidIdChar(char c) const {
  return String(c).IsAny(_ValidIdChars);
}

//Get character escape sequence
bool _GetEscapeChar(char EscapeChar,char& Result){
  bool RetCode;
  switch(EscapeChar){
    case 'a' : Result='\a'; RetCode=true; break;
    case 'b' : Result='\b'; RetCode=true; break;
    case 'f' : Result='\f'; RetCode=true; break;
    case 'n' : Result='\n'; RetCode=true; break;
    case 'r' : Result='\r'; RetCode=true; break;
    case 't' : Result='\t'; RetCode=true; break;
    case 'v' : Result='\v'; RetCode=true; break;
    case '\'': Result='\''; RetCode=true; break;
    case '\\': Result='\\'; RetCode=true; break;
    default: RetCode=false; break;
  }
  return RetCode;
}

//Escape all characters in string
String _EscapeString(const String& Str){
  String Output;
  for(long i=0;i<Str.Length();i++){
    if(Str[i]==_TextQualifier){
      Output+=String(_TextEscapeMarker)+String(_TextQualifier);
    }
    else if(Str[i]=='\''){
      Output+="\\\'";
    }
    else if(Str[i]=='\\'){
      Output+="\\\\";
    }
    else if(Str[i]>=32 && Str[i]<=126){
      Output+=Str[i];
    }
    else{
      Output+="\\x"+HEXVALUE(Str[i]);
    }
  }
  return Output;
}

//Check next token is keyword
bool Sentence::_IsKeyWord(const String& Line,int ColNr,OrigBuffer Origin,PrKeyword& Kwd,int& Length) const {
  
  //Variables
  int FoundIndex;
  
  //Keyword check loop
  FoundIndex=-1;
  for(int i=0;i<_KwdNr;i++){
    if(Line.SearchAt(_Kwd[i],ColNr)){
      if(ColNr+_Kwd[i].Length()<=Line.Length()-1){
        if(!_ValidIdChar(Line[ColNr+_Kwd[i].Length()])){ FoundIndex=i; }
      }
      else{ 
        FoundIndex=i;
      }
    }
    if(FoundIndex!=-1){
      break;
    }
  }

  //Keyword is not found
  if(FoundIndex==-1){ return false; }

  //If keyword starts with system namespace it can only come from inserted/added code by compiler
  //(if this is not the case we ignore it and therefore it will be interpreted as invalid token)
  if(_Kwd[FoundIndex].StartsWith(SYSTEM_NAMESPACE) && Origin!=OrigBuffer::Insertion && Origin!=OrigBuffer::Addition){
    return false;
  }
  
  //Return keyword found
  Length=_Kwd[FoundIndex].Length();
  Kwd=(PrKeyword)FoundIndex;
  return true;

}

//Check next token is operator
bool Sentence::_IsOperator(const String& Line,int ColNr,PrOperator& Opr,int& Length) const {
  
  //Operator check loop
  for(int i=0;i<_OprNr;i++){
    if(Line.SearchAt(_Opr[i],ColNr)){
      Length=_Opr[i].Length();
      Opr=(PrOperator)i;
      return true ;
    }
  }

  //Not operator
  return false;
}

//Check next token is puncuator
bool Sentence::_IsPunctuator(const String& Line,int ColNr,PrPunctuator& Pnc,int& Length) const {
  
  //Punctuator check loop
  for(int i=0;i<_PncNr;i++){
    if(Line.SearchAt(_Pnc[i],ColNr)){
      Length=_Pnc[i].Length();
      Pnc=(PrPunctuator)i;
      return true ;
    }
  }

  //Not punctuator
  return false;
}

//Check next token is a type name
//bool _TokenIsTypeName(char *Line,int ColNr,char *Name,int *Length, bool *Error){
bool Sentence::_IsTypeName(const String& Line,const Array<String>& TypeList,int ColNr,String& Typ,int& Length) const {
  
  //Variables
  String WorkLine;

  //Get work line
  WorkLine=Line.CutLeft(ColNr);
  
  //Check type list
  Typ="";
  for(int i=0;i<TypeList.Length();i++){
    if(WorkLine.StartsWith(TypeList[i])){
      if(WorkLine.Length()==TypeList[i].Length()){ 
        Typ=TypeList[i]; 
        break; 
      }
      else if(WorkLine.Length()>TypeList[i].Length() && String(_ValidIdChars).Search(WorkLine.Mid(TypeList[i].Length(),1))==-1){ 
        Typ=TypeList[i]; 
        break; 
      }
    }
  }
  if(Typ.Length()==0){ return false; }

  //Return valid type name
  Length=Typ.Length();
  return true;
}

//Check next token is identifier
//bool _TokenIsIdentifier(char *Line,int ColNr,char *Name,int *Length, bool *Error){
bool Sentence::_IsIdentifier(const String& Line,int ColNr,int BaseColNr,String& Idn,int& Length, bool& Error) const {
  
  //Get characters until non valid identifier char
  Idn=Line.CutLeft(ColNr).GetWhile(_ValidIdChars+GetSystemNameSpace());
  
  //Return no identifier found
  if(Idn.Length()==0){
    return false;
  }
  
  //It is not an identifier if it only contains numbers
  if(Idn.ContainsOnly("0123456789")){
    return false;
  }

  //Error if identifier starts by number
  if(Idn[0]>='0' && Idn[0]<='9'){
    SysMessage(1,_FileName,_LineNr,BaseColNr+ColNr).Print(Idn);
    Error=true;
    return true;
  }

  //Error if identifier exceeds permitted length
  if(Idn.Length()>_MaxIdLen){
    SysMessage(268,_FileName,_LineNr,BaseColNr+ColNr).Print(Idn,ToString(_MaxIdLen));
    Error=true;
    return true;
  }

  //System namespace marker only allowed if enabled and if in first position
  if(Idn.SearchCount(GetSystemNameSpace())!=0){
    if(!_EnableSysNameSpace){
      SysMessage(286,_FileName,_LineNr,BaseColNr+ColNr).Print(Idn);
      Error=true;
      return true;
    }
  }

  //Return valid identifier
  Error=false;
  Length=Idn.Length();
  return true;
}

//Check next token is boolean
bool Sentence::_IsBoolean(const String& Line,int ColNr,CpuBol& Bol,int& Length) const {
  
  //Variables
  String Boolean;

  //Get characters until non valid identifier char
  Boolean=Line.CutLeft(ColNr).GetWhile(_ValidIdChars);
  
  //Return no boolean found
  if(Boolean.Length()==0){
    return false;
  }
  
  //Parse boolean
  if(Boolean==_BooleanTrue){
    Bol=true;
  }
  else if(Boolean==_BooleanFalse){
    Bol=false;
  }
  else{
    return false;
  }

  //Return valid boolean
  Length=Boolean.Length();
  return true;
}

//Check next token is char value
//Return value: Indicates if token is found or not
//Error: Indicates if syntax error is found (return value must be 1 to stop if-elseif chain)
//bool _TokenIsChar(char *Line,int ColIndex,char *Char,int *Length, bool *Error){
bool Sentence::_IsChar(const String& Line,int ColNr,int BaseColNr,CpuChr& Chr,int& Length, bool& Error) const {
  
  //Variables
  int EndQuotePos;
  int Nibble1,Nibble2;
  char Char;
  char ResChr;
  
  //Check token starts by quote
  if(Line[ColNr]!='\''){ return false; }

  //Char with escape sequence
  if(ColNr+2<Line.Length() && (Line.Mid(ColNr+1,1)==String(_TextEscapeMarker) || Line.Mid(ColNr+1,1)==String('\\'))){

    //Escape text qualifier
    if(Line.Mid(ColNr+1,1)==String(_TextEscapeMarker)){
      if(Line.Mid(ColNr+2,1)==String(_TextQualifier)){
        ResChr=_TextQualifier;
        Length=4;
        EndQuotePos=ColNr+3;
      }
      else{
        SysMessage(360,_FileName,_LineNr,BaseColNr+ColNr+2).Print(String(_TextEscapeMarker)+String(Line[ColNr+2]));
        Error=true;
        return true;
      }
    }

    //Escape sequence
    else if(_GetEscapeChar(Line[ColNr+2],ResChr)){
      Length=4;
      EndQuotePos=ColNr+3;
    }

    else{
      
      //Hex escape sequence
      if(Line[ColNr+2]=='x' && ColNr+5<Line.Length()){

        //Length and ending quote
        Length=6;
        EndQuotePos=ColNr+5;
    
        //Hex byte sequence (nibble 1)  
        Char=Line[ColNr+3];
        if(!((Char>='0' && Char<='9') || (Char>='A' && Char<='F'))){
          SysMessage(11,_FileName,_LineNr,BaseColNr+ColNr).Print("\\"+String(Char));
          Error=true;
          return true;
        }
        Nibble1=(Char>='0' && Char<='9'?Char-'0':Char-'A'+10);

        //Hex byte sequence (nibble 2)  
        Char=Line[ColNr+4];
        if(!((Char>='0' && Char<='9') || (Char>='A' && Char<='F'))){
          SysMessage(11,_FileName,_LineNr,BaseColNr+ColNr).Print("\\"+String(Char));
          Error=true;
          return true;
        }
        Nibble2=(Char>='0' && Char<='9'?Char-'0':Char-'A'+10);
        
        //Set character
        ResChr=(Nibble1<<4)|Nibble2;

      }
      
      //Error: Unknown escape sequence
      else{
        SysMessage(10,_FileName,_LineNr,BaseColNr+ColNr+2).Print("\\"+String(Line[ColNr+2]));
        Error=true;
        return true;
      }

    }

  }

  //Char without escape sequence
  else{

    //Length and ending quote position
    Length=3;
    EndQuotePos=ColNr+2;

    //Get character
    ResChr=Line[ColNr+1];

  }


  //Check there is ending quote
  if(EndQuotePos>=Line.Length() || Line[EndQuotePos]!='\''){
    SysMessage(125,_FileName,_LineNr,BaseColNr+EndQuotePos).Print();
    Error=true;
    return true;
  }

  //Return result
  Chr=ResChr;
  Error=false;
  return true;

}

//Check next token is unsigned integer
//Return value: Indicates if token is found or not
//Error: Indicates if syntax error is found (return value must be 1 to stop if-elseif chain)
bool Sentence::_IsNumber(const String& Line,int ColNr,int BaseColNr,PrToken& NumToken,int& Length, bool& Error) const {
   
  //Variables
  long long Num;
  int Base;
  int PrefixLength;
  String NumberStr;
  String Suffix;
  String BaseName;

  //Get number prefix and determine numeric base
  if(Line.Mid(ColNr,2)=="0c"){ Base=8; }
  else if(Line.Mid(ColNr,2).Lower()=="0x"){ Base=16; }
  else{ Base=10; }

  //Get number string
  switch(Base){
    case 8 :
      BaseName="octal";
      PrefixLength=2;
      NumberStr=Line.CutLeft(ColNr+PrefixLength).GetWhile("01234567"); 
      if(NumberStr.Length()==0){ SysMessage(231,_FileName,_LineNr,BaseColNr+ColNr).Print(); Error=true; return true; }
      break;
    case 16: 
      BaseName="hexadecimal";
      PrefixLength=2;
      NumberStr=Line.CutLeft(ColNr+PrefixLength).GetWhile("0123456789abcdefABCDEF"); 
      if(NumberStr.Length()==0){ SysMessage(13,_FileName,_LineNr,BaseColNr+ColNr).Print(); Error=true; return true; }
      break;
    case 10: 
      BaseName="decimal";
      PrefixLength=0;
      NumberStr=Line.CutLeft(ColNr+PrefixLength).GetWhile("0123456789"); 
      if(NumberStr.Length()==0){ return false; }
      break;
  }
  
  //Check next character is not . or e or E (because then number is not an integer)
  if(Line.Mid(ColNr+PrefixLength+NumberStr.Length(),1).Upper().IsAny(".E")){
    return false;
  }

  //Get suffix (if any)
  if(Line.Mid(ColNr+PrefixLength+NumberStr.Length(),1).Upper().IsAny("RSNL")){
    Suffix=Line.Mid(ColNr+PrefixLength+NumberStr.Length(),1).Upper();
  }
  else{
    Suffix="";
  }

  //Convert number to unsigned long
  Num=NumberStr.ToLongLong(Error,Base);
  if(Error){
    SysMessage(2,_FileName,_LineNr,BaseColNr+ColNr).Print(NumberStr,BaseName);
    Error=true;
    return true;
  }

  //Derive data type
  if(Suffix==""){
    if(Num>=MIN_CHR && Num<=MAX_CHR){ 
      NumToken.Id(PrTokenId::Char); 
      NumToken.Value.Chr=(CpuChr)Num; 
    }
    else if(Num>=MIN_SHR && Num<=MAX_SHR){ 
      NumToken.Id(PrTokenId::Short); 
      NumToken.Value.Shr=(CpuShr)Num; 
    }
    else if(Num>=MIN_INT && Num<=MAX_INT){ 
      NumToken.Id(PrTokenId::Integer); 
      NumToken.Value.Int=(CpuInt)Num; 
    }
    else{                                  
      NumToken.Id(PrTokenId::Long);    
      NumToken.Value.Lon=(CpuLon)Num; 
    }
  }
  else if(Suffix=="R"){
    if(Num>=MIN_CHR && Num<=MAX_CHR){ 
      NumToken.Id(PrTokenId::Char); 
      NumToken.Value.Chr=(CpuChr)Num; 
    } 
    else{ 
      SysMessage(221,_FileName,_LineNr,BaseColNr+ColNr).Print(NumberStr,Suffix,"char"); 
      Error=true; 
      return true; 
    }
  }
  else if(Suffix=="S"){
    if(Num>=MIN_SHR && Num<=MAX_SHR){ 
      NumToken.Id(PrTokenId::Short); 
      NumToken.Value.Shr=(CpuShr)Num; 
    } 
    else{ 
      SysMessage(221,_FileName,_LineNr,BaseColNr+ColNr).Print(NumberStr,Suffix,"short"); 
      Error=true; 
      return true; 
    }
  }
  else if(Suffix=="N"){
    if(Num>=MIN_INT && Num<=MAX_INT){ 
      NumToken.Id(PrTokenId::Integer); 
      NumToken.Value.Int=(CpuInt)Num; 
    } 
    else{ 
      SysMessage(221,_FileName,_LineNr,BaseColNr+ColNr).Print(NumberStr,Suffix,"int"); 
      Error=true; 
      return true; 
    }
  }
  else if(Suffix=="L"){
    NumToken.Id(PrTokenId::Long); 
    NumToken.Value.Lon=(CpuLon)Num; 
  }

  //Return valid identifier
  Error=false;
  Length=PrefixLength+NumberStr.Length()+Suffix.Length();
  return true;

}

//Check next token is decimal or floating point number
//Return value: Indicates if token is found or not
//Error: Indicates if syntax error is found (return value must be 1 to stop if-elseif chain)
bool Sentence::_IsFloat(const String& Line,int ColNr,int BaseColNr,CpuFlo& Flo,int& Length, bool& Error) const {
  
  //Variables
  String NumberStr;
  String Exponent;
  
  //Get characters until non numeric char
  NumberStr=Line.CutLeft(ColNr).GetWhile("0123456789.");
  if(NumberStr.Length()==0){ return false; }
  
  //Exit if more than one decimal dot is present
  if(NumberStr.SearchCount(".")>1){
    SysMessage(4,_FileName,_LineNr,BaseColNr+ColNr).Print(NumberStr);
    Error=true;
    return true;
  }
  
  //Get exponent if present
  if(Line.CutLeft(ColNr+NumberStr.Length()).Upper().StartsWith("E")){
    String ExpSign;
    String ExpNumber;
    int BaseIndex=ColNr+NumberStr.Length();
    if(Line.Mid(BaseIndex+1,1).IsAny("+-")){ ExpSign=Line.Mid(BaseIndex+1,1); } else{ ExpSign=""; }
    ExpNumber=Line.CutLeft(BaseIndex+1+ExpSign.Length()).GetWhile("0123456789");
    NumberStr+="E"+ExpSign+ExpNumber;
    if(ExpNumber.Length()==0){
      SysMessage(5,_FileName,_LineNr,BaseColNr+ColNr).Print(NumberStr);
      Error=true;
      return true;
    }
  }
  
  //Convert double unless suffix is specified
  Flo=NumberStr.ToDouble(Error);
  if(Error){
    SysMessage(8,_FileName,_LineNr,BaseColNr+ColNr).Print(Exponent);
    Error=true;
    return true;
  }
  
  //Return valid identifier
  Error=false;
  Length=NumberStr.Length();
  return true;
}


//Return value: Indicates if token is found or not
//Error: Indicates if syntax error is found (return value must be 1 to stop if-elseif chain)
//bool Parser::_TokenIsString(char *Line,int ColNr,char *String,int *Length, bool *Error){
bool Sentence::_IsString(const String& Line,int ColNr,int BaseColNr,String& Str,int& Length, bool& Error) const {

  //Variables
  int i;
  bool EscapeSeq;
  int EscapeMode;
  int Nibble1,Nibble2;
  bool Exit;
  bool StrOpen;
  char Chr;
  char EscapeChr;

  //Check token starts by text qualifier
  if(Line[ColNr]!=_TextQualifier){ return false; }

  //Get all characters until string end
  i=0;
  StrOpen=false;
  Exit=false;
  EscapeSeq=false;
  EscapeMode=0;
  Length=0;
  while(ColNr+i<Line.Length() && !Exit){ 
    
    //Check litteral string limit
    if(i==_MaxStringLimit){
      SysMessage(9,_FileName,_LineNr,BaseColNr+ColNr).Print(ToString(_MaxStringLimit));
      Error=true;
      return true;
    }
    
    //Get character
    Chr=Line[ColNr+i];
    Length++;
    
    //Normal mode
    if(EscapeSeq==false){
      
      //Text escape
      if(Chr==_TextQualifier){
        if(StrOpen && ColNr+i+1<Line.Length() && Line.Mid(ColNr+i+1,1)==_TextEscapeMarker){
          Str+=_TextQualifier;
          i++;
          Length++;
        }
        else{
          StrOpen=(StrOpen?false:true);
        }
      }

      //Escape sequence
      else if(Chr=='\\'){
        EscapeSeq=true;
        EscapeMode=0;
      }
      
      //Rest of chars
      else{
        Str+=Chr;
      }
    
    }
    
    //Escape sequence mode
    else{

      //Switch on escape mode
      switch(EscapeMode){
      
        //Mode 0: Single character escape sequences with slash
        case 0:
          if(_GetEscapeChar(Chr,EscapeChr)){
            Str+=EscapeChr;
            EscapeSeq=false; 
            EscapeMode=0;
          }
          else{
            if(Chr=='x'){
              EscapeMode=1; 
            }
            else{
              SysMessage(10,_FileName,_LineNr,BaseColNr+ColNr).Print("\\"+String(Chr));
              Error=true;
              return true;
            }
          }
          break;
      
        //Mode 1: Hex byte sequence (nibble 1)  
        case 1:
          if(!((Chr>='0' && Chr<='9') || (Chr>='A' && Chr<='F'))){
            SysMessage(11,_FileName,_LineNr,BaseColNr+ColNr).Print("\\"+String(Chr));
            Error=true;
            return true;
          }
          Nibble1=(Chr>='0' && Chr<='9'?Chr-'0':Chr-'A'+10);
          EscapeMode=2;
          break;

        //Mode 2: Hex byte sequence (nibble 2)  
        case 2:
          if(!((Chr>='0' && Chr<='9') || (Chr>='A' && Chr<='F'))){
            SysMessage(11,_FileName,_LineNr,BaseColNr+ColNr).Print("\\"+String(Chr));
            Error=true;
            return true;
          }
          Nibble2=(Chr>='0' && Chr<='9'?Chr-'0':Chr-'A'+10);
          Str+=(Nibble1<<4)|Nibble2;
          EscapeSeq=false;
          EscapeMode=0;
          break;

      }
    }
    
    //Exit if string is not open at first position (we do not have a string litteral)
    if(i==0 && StrOpen==false){
      return(false);
    }
    
    //Exit when string is closed
    if(i!=0 && StrOpen==false){
      break;
    }
    
    //Increase pointer
    i++;

  }
  
  //Error: Loop was exited with string not closed: string is open in source
  if(StrOpen){
    SysMessage(12,_FileName,_LineNr,BaseColNr+ColNr).Print();
    Error=true;
    return true;
  }
  
  //Return result
  Error=false;
  return true;

}

//Return value: Indicates if token is found or not
//Error: Indicates if syntax error is found (return value must be 1 to stop if-elseif chain)
bool Sentence::_IsRawString(const String& Line,int ColNr,int BaseColNr,String& Str,int& Length, bool& Error) const {

  //Variables
  long EndPos;

  //Check token starts by raw string begin
  if(!Line.SearchAt(_RawStrBeg,ColNr)){ return false; }

  //Find raw sring end
  if((EndPos=Line.Search(_RawStrEnd,ColNr))==-1){ 
    SysMessage(337,_FileName,_LineNr,BaseColNr+ColNr).Print();
    Error=true;
    return true;
  }

  //Extract raw string from source line
  Str=Line.Mid(ColNr+_RawStrBeg.Length(),EndPos-ColNr-_RawStrBeg.Length());

  //Calculate read chars
  Length=_RawStrBeg.Length()+Str.Length()+_RawStrEnd.Length();
  
  //Return result
  Error=false;
  return true;

}

//Number token assignment
inline void AssignNumberToken(PrToken& Destin,PrToken& Source){
  switch(Source.Id()){
    case PrTokenId::Char      : Destin.Id(Source.Id()); Destin.Value.Chr=Source.Value.Chr; break;
    case PrTokenId::Short     : Destin.Id(Source.Id()); Destin.Value.Shr=Source.Value.Shr; break;
    case PrTokenId::Integer   : Destin.Id(Source.Id()); Destin.Value.Int=Source.Value.Int; break;
    case PrTokenId::Long      : Destin.Id(Source.Id()); Destin.Value.Lon=Source.Value.Lon; break;
    case PrTokenId::Boolean   :
    case PrTokenId::Float     :
    case PrTokenId::Keyword   :
    case PrTokenId::Operator  :
    case PrTokenId::Punctuator:
    case PrTokenId::TypeName  :
    case PrTokenId::Identifier:
    case PrTokenId::String    :
      break;
  }
}

//Obtain token from source line
bool Sentence::_GetToken(const String& Line,const Array<String>& TypeList,int CumulLen,OrigBuffer Origin,int AtPos,int& EndPos, PrToken& Token) const {

  //Token case jump labels
  static const void *TokenCase[256]={
    &&GetTokenErr,          //  0
    &&GetTokenErr,          //  1
    &&GetTokenErr,          //  2
    &&GetTokenErr,          //  3
    &&GetTokenErr,          //  4
    &&GetTokenErr,          //  5
    &&GetTokenErr,          //  6
    &&GetTokenErr,          //  7
    &&GetTokenErr,          //  8
    &&GetTokenErr,          //  9
    &&GetTokenErr,          // 10
    &&GetTokenErr,          // 11
    &&GetTokenErr,          // 12
    &&GetTokenErr,          // 13
    &&GetTokenErr,          // 14
    &&GetTokenErr,          // 15
    &&GetTokenErr,          // 16
    &&GetTokenErr,          // 17
    &&GetTokenErr,          // 18
    &&GetTokenErr,          // 19
    &&GetTokenErr,          // 20
    &&GetTokenErr,          // 21
    &&GetTokenErr,          // 22
    &&GetTokenErr,          // 23
    &&GetTokenErr,          // 24
    &&GetTokenErr,          // 25
    &&GetTokenErr,          // 26
    &&GetTokenErr,          // 27
    &&GetTokenErr,          // 28
    &&GetTokenErr,          // 29
    &&GetTokenErr,          // 30
    &&GetTokenErr,          // 31
    &&GetTokenErr,          // 32 space Space
    &&GetTokenOpr,          // 33 ! exclamation mark
    &&GetTokenStr,          // 34 " double quote
    &&GetTokenErr,          // 35 # number
    &&GetTokenKwdIdn,       // 36 $ dollar
    &&GetTokenOpr,          // 37 % percent
    &&GetTokenOpr,          // 38 & ampersand
    &&GetTokenChr,          // 39 ' single quote
    &&GetTokenOprPnc,       // 40 ( left parenthesis
    &&GetTokenOprPnc,       // 41 ) right parenthesis
    &&GetTokenOpr,          // 42 * asterisk
    &&GetTokenOpr,          // 43 + plus
    &&GetTokenPnc,          // 44 , comma
    &&GetTokenOpr,          // 45 - minus
    &&GetTokenKwdOpr,       // 46 . period
    &&GetTokenOpr,          // 47 / slash
    &&GetTokenNumFlo,       // 48 0 zero
    &&GetTokenNumFlo,       // 49 1 one
    &&GetTokenNumFlo,       // 50 2 two
    &&GetTokenNumFlo,       // 51 3 three
    &&GetTokenNumFlo,       // 52 4 four
    &&GetTokenNumFlo,       // 53 5 five
    &&GetTokenNumFlo,       // 54 6 six
    &&GetTokenNumFlo,       // 55 7 seven
    &&GetTokenNumFlo,       // 56 8 eight
    &&GetTokenNumFlo,       // 57 9 nine
    &&GetTokenKwdOprPnc,    // 58 : colon
    &&GetTokenPnc,          // 59 ; semicolon
    &&GetTokenOpr,          // 60 < less than
    &&GetTokenOpr,          // 61 = equality sign
    &&GetTokenOpr,          // 62 > greater than
    &&GetTokenOpr,          // 63 ? question mark
    &&GetTokenErr,          // 64 @ at sign
    &&GetTokenTypIdn,       // 65 A  
    &&GetTokenTypIdn,       // 66 B  
    &&GetTokenTypIdn,       // 67 C  
    &&GetTokenTypIdn,       // 68 D  
    &&GetTokenTypIdn,       // 69 E  
    &&GetTokenTypIdn,       // 70 F  
    &&GetTokenTypIdn,       // 71 G  
    &&GetTokenTypIdn,       // 72 H  
    &&GetTokenTypIdn,       // 73 I  
    &&GetTokenTypIdn,       // 74 J  
    &&GetTokenTypIdn,       // 75 K  
    &&GetTokenTypIdn,       // 76 L  
    &&GetTokenTypIdn,       // 77 M  
    &&GetTokenTypIdn,       // 78 N  
    &&GetTokenTypIdn,       // 79 O  
    &&GetTokenTypIdn,       // 80 P  
    &&GetTokenTypIdn,       // 81 Q  
    &&GetTokenTypIdn,       // 82 R  
    &&GetTokenTypIdn,       // 83 S  
    &&GetTokenTypIdn,       // 84 T  
    &&GetTokenTypIdn,       // 85 U  
    &&GetTokenTypIdn,       // 86 V  
    &&GetTokenTypIdn,       // 87 W  
    &&GetTokenTypIdn,       // 88 X  
    &&GetTokenTypIdn,       // 89 Y  
    &&GetTokenTypIdn,       // 90 Z  
    &&GetTokenPnc,          // 91 [ left square bracket
    &&GetTokenErr,          // 92 \ backslash
    &&GetTokenPnc,          // 93 ] right square bracket
    &&GetTokenOpr,          // 94 ^ caret / circumflex
    &&GetTokenTypIdn,       // 95 _ underscore
    &&GetTokenErr,          // 96 ` grave / accent
    &&GetTokenKwdTypIdn,    // 97 a  
    &&GetTokenKwdTypIdn,    // 98 b  
    &&GetTokenKwdTypIdn,    // 99 c  
    &&GetTokenKwdTypIdn,    //100 d  
    &&GetTokenKwdTypIdn,    //101 e  
    &&GetTokenKwdTypBolIdn, //102 f  
    &&GetTokenKwdTypIdn,    //103 g  
    &&GetTokenKwdTypIdn,    //104 h  
    &&GetTokenKwdTypIdn,    //105 i  
    &&GetTokenKwdTypIdn,    //106 j  
    &&GetTokenKwdTypIdn,    //107 k  
    &&GetTokenKwdTypIdn,    //108 l  
    &&GetTokenKwdTypIdn,    //109 m  
    &&GetTokenKwdTypIdn,    //110 n  
    &&GetTokenKwdTypIdn,    //111 o  
    &&GetTokenKwdTypIdn,    //112 p 
    &&GetTokenKwdTypIdn,    //113 q  
    &&GetTokenKwdRawTypIdn, //114 r  
    &&GetTokenKwdTypIdn,    //115 s  
    &&GetTokenKwdTypBolIdn, //116 t  
    &&GetTokenKwdTypIdn,    //117 u  
    &&GetTokenKwdTypIdn,    //118 v  
    &&GetTokenKwdTypIdn,    //119 w  
    &&GetTokenKwdTypIdn,    //120 x  
    &&GetTokenKwdTypIdn,    //121 y  
    &&GetTokenKwdTypIdn,    //122 z  
    &&GetTokenPnc,          //123 { left curly bracket
    &&GetTokenOpr,          //124 | vertical bar
    &&GetTokenPnc,          //125 } right curly bracket
    &&GetTokenOpr,          //126 ~ tilde
    &&GetTokenErr,          //127
    &&GetTokenErr,          //128
    &&GetTokenErr,          //129
    &&GetTokenErr,          //130
    &&GetTokenErr,          //131
    &&GetTokenErr,          //132
    &&GetTokenErr,          //133
    &&GetTokenErr,          //134
    &&GetTokenErr,          //135
    &&GetTokenErr,          //136
    &&GetTokenErr,          //137
    &&GetTokenErr,          //138
    &&GetTokenErr,          //139
    &&GetTokenErr,          //140
    &&GetTokenErr,          //141
    &&GetTokenErr,          //142
    &&GetTokenErr,          //143
    &&GetTokenErr,          //144
    &&GetTokenErr,          //145
    &&GetTokenErr,          //146
    &&GetTokenErr,          //147
    &&GetTokenErr,          //148
    &&GetTokenErr,          //149
    &&GetTokenErr,          //150
    &&GetTokenErr,          //151
    &&GetTokenErr,          //152
    &&GetTokenErr,          //153
    &&GetTokenErr,          //154
    &&GetTokenErr,          //155
    &&GetTokenErr,          //156
    &&GetTokenErr,          //157
    &&GetTokenErr,          //158
    &&GetTokenErr,          //159
    &&GetTokenErr,          //160
    &&GetTokenErr,          //161
    &&GetTokenErr,          //162
    &&GetTokenErr,          //163
    &&GetTokenErr,          //164
    &&GetTokenErr,          //165
    &&GetTokenErr,          //166
    &&GetTokenErr,          //167
    &&GetTokenErr,          //168
    &&GetTokenErr,          //169
    &&GetTokenErr,          //170
    &&GetTokenErr,          //171
    &&GetTokenErr,          //172
    &&GetTokenErr,          //173
    &&GetTokenErr,          //174
    &&GetTokenErr,          //175
    &&GetTokenErr,          //176
    &&GetTokenErr,          //177
    &&GetTokenErr,          //178
    &&GetTokenErr,          //179
    &&GetTokenErr,          //180
    &&GetTokenErr,          //181
    &&GetTokenErr,          //182
    &&GetTokenErr,          //183
    &&GetTokenErr,          //184
    &&GetTokenErr,          //185
    &&GetTokenErr,          //186
    &&GetTokenErr,          //187
    &&GetTokenErr,          //188
    &&GetTokenErr,          //189
    &&GetTokenErr,          //190
    &&GetTokenErr,          //191
    &&GetTokenErr,          //192
    &&GetTokenErr,          //193
    &&GetTokenErr,          //194
    &&GetTokenErr,          //195
    &&GetTokenErr,          //196
    &&GetTokenErr,          //197
    &&GetTokenErr,          //198
    &&GetTokenErr,          //199
    &&GetTokenErr,          //200
    &&GetTokenErr,          //201
    &&GetTokenErr,          //202
    &&GetTokenErr,          //203
    &&GetTokenErr,          //204
    &&GetTokenErr,          //205
    &&GetTokenErr,          //206
    &&GetTokenErr,          //207
    &&GetTokenErr,          //208
    &&GetTokenErr,          //209
    &&GetTokenErr,          //210
    &&GetTokenErr,          //211
    &&GetTokenErr,          //212
    &&GetTokenErr,          //213
    &&GetTokenErr,          //214
    &&GetTokenErr,          //215
    &&GetTokenErr,          //216
    &&GetTokenErr,          //217
    &&GetTokenErr,          //218
    &&GetTokenErr,          //219
    &&GetTokenErr,          //220
    &&GetTokenErr,          //221
    &&GetTokenErr,          //222
    &&GetTokenErr,          //223
    &&GetTokenErr,          //224
    &&GetTokenErr,          //225
    &&GetTokenErr,          //226
    &&GetTokenErr,          //227
    &&GetTokenErr,          //228
    &&GetTokenErr,          //229
    &&GetTokenErr,          //230
    &&GetTokenErr,          //231
    &&GetTokenErr,          //232
    &&GetTokenErr,          //233
    &&GetTokenErr,          //234
    &&GetTokenErr,          //235
    &&GetTokenErr,          //236
    &&GetTokenErr,          //237
    &&GetTokenErr,          //238
    &&GetTokenErr,          //239
    &&GetTokenErr,          //240
    &&GetTokenErr,          //241
    &&GetTokenErr,          //242
    &&GetTokenErr,          //243
    &&GetTokenErr,          //244
    &&GetTokenErr,          //245
    &&GetTokenErr,          //246
    &&GetTokenErr,          //247
    &&GetTokenErr,          //248
    &&GetTokenErr,          //249
    &&GetTokenErr,          //250
    &&GetTokenErr,          //251
    &&GetTokenErr,          //252
    &&GetTokenErr,          //253
    &&GetTokenErr,          //254
    &&GetTokenErr           //255
  };

  //Variables
  int Pos;
  int Length;
  int BaseColNr;
  int TokenLen;
  bool Error;
  PrKeyword Kwd;
  PrOperator Opr;
  PrPunctuator Pnc;
  String Typ;
  String Idn;
  CpuBol Bol;
  CpuChr Chr;
  CpuFlo Flo;
  String Str;
  PrToken NumToken;

  //Init
  Error=false;
  BaseColNr=CumulLen-Line.Length();

  //Eat spaces
  for(Pos=AtPos;Line[Pos]==' ';Pos++);

  //Get case address and jump
  goto *TokenCase[(unsigned char)Line[Pos]];

  GetTokenChr:;          if(_IsChar(Line,Pos,BaseColNr,Chr,Length,Error)){ Token.Id(PrTokenId::Char); Token.Value.Chr=Chr; goto GetTokenEnd; }
                         else{ goto GetTokenErr; }
  
  GetTokenKwdIdn:;       if(_IsKeyWord(Line,Pos,Origin,Kwd,Length)){ Token.Id(PrTokenId::Keyword); Token.Value.Kwd=Kwd; goto GetTokenEnd; }
                         else if(_IsIdentifier(Line,Pos,BaseColNr,Idn,Length,Error)){ Token.Id(PrTokenId::Identifier); Token.Value.Idn=Idn; goto GetTokenEnd; }
                         else{ goto GetTokenErr; }
  
  GetTokenKwdOpr:;       if(_IsKeyWord(Line,Pos,Origin,Kwd,Length)){ Token.Id(PrTokenId::Keyword); Token.Value.Kwd=Kwd; goto GetTokenEnd; }
                         else if(_IsOperator(Line,Pos,Opr,Length)){ Token.Id(PrTokenId::Operator); Token.Value.Opr=Opr; goto GetTokenEnd; }
                         else{ goto GetTokenErr; }
  
  GetTokenKwdOprPnc:;    if(_IsKeyWord(Line,Pos,Origin,Kwd,Length)){ Token.Id(PrTokenId::Keyword); Token.Value.Kwd=Kwd; goto GetTokenEnd; }
                         else if(_IsOperator(Line,Pos,Opr,Length)){ Token.Id(PrTokenId::Operator); Token.Value.Opr=Opr; goto GetTokenEnd; }
                         else if(_IsPunctuator(Line,Pos,Pnc,Length)){ Token.Id(PrTokenId::Punctuator); Token.Value.Pnc=Pnc; goto GetTokenEnd; }
                         else{ goto GetTokenErr; }
  
  GetTokenOprPnc:;       if(_IsOperator(Line,Pos,Opr,Length)){ Token.Id(PrTokenId::Operator); Token.Value.Opr=Opr; goto GetTokenEnd; }
                         else if(_IsPunctuator(Line,Pos,Pnc,Length)){ Token.Id(PrTokenId::Punctuator); Token.Value.Pnc=Pnc; goto GetTokenEnd; }
                         else{ goto GetTokenErr; }
  
  GetTokenNumFlo:;       if(_IsNumber(Line,Pos,BaseColNr,NumToken,Length,Error)){ AssignNumberToken(Token,NumToken); goto GetTokenEnd; }  
                         else if(_IsFloat(Line,Pos,BaseColNr,Flo,Length,Error)){ Token.Id(PrTokenId::Float); Token.Value.Flo=Flo; goto GetTokenEnd; }
                         else{ goto GetTokenErr; }
  
  GetTokenOpr:;          if(_IsOperator(Line,Pos,Opr,Length)){ Token.Id(PrTokenId::Operator); Token.Value.Opr=Opr; goto GetTokenEnd; }
                         else{ goto GetTokenErr; }
  
  GetTokenPnc:;          if(_IsPunctuator(Line,Pos,Pnc,Length)){ Token.Id(PrTokenId::Punctuator); Token.Value.Pnc=Pnc; goto GetTokenEnd; }
                         else{ goto GetTokenErr; }
  
  GetTokenKwdRawTypIdn:; if(_IsKeyWord(Line,Pos,Origin,Kwd,Length)){ Token.Id(PrTokenId::Keyword); Token.Value.Kwd=Kwd; goto GetTokenEnd; }
                         else if(_IsRawString(Line,Pos,BaseColNr,Str,Length,Error)){ Token.Id(PrTokenId::String); Token.Value.Str=Str; goto GetTokenEnd; }
                         else if(_IsTypeName(Line,TypeList,Pos,Typ,Length)){ Token.Id(PrTokenId::TypeName); Token.Value.Typ=Typ; goto GetTokenEnd; }
                         else if(_IsIdentifier(Line,Pos,BaseColNr,Idn,Length,Error)){ Token.Id(PrTokenId::Identifier); Token.Value.Idn=Idn; goto GetTokenEnd; }
                         else{ goto GetTokenErr; }
  
  GetTokenStr:;          if(_IsString(Line,Pos,BaseColNr,Str,Length,Error)){ Token.Id(PrTokenId::String); Token.Value.Str=Str; goto GetTokenEnd; }
                         else{ goto GetTokenErr; }
  
  GetTokenKwdTypIdn:;    if(_IsKeyWord(Line,Pos,Origin,Kwd,Length)){ Token.Id(PrTokenId::Keyword); Token.Value.Kwd=Kwd; goto GetTokenEnd; }
                         else if(_IsTypeName(Line,TypeList,Pos,Typ,Length)){ Token.Id(PrTokenId::TypeName); Token.Value.Typ=Typ; goto GetTokenEnd; }
                         else if(_IsIdentifier(Line,Pos,BaseColNr,Idn,Length,Error)){ Token.Id(PrTokenId::Identifier); Token.Value.Idn=Idn; goto GetTokenEnd; }
                         else{ goto GetTokenErr; }
  
  GetTokenTypIdn:;       if(_IsTypeName(Line,TypeList,Pos,Typ,Length)){ Token.Id(PrTokenId::TypeName); Token.Value.Typ=Typ; goto GetTokenEnd; }
                         else if(_IsIdentifier(Line,Pos,BaseColNr,Idn,Length,Error)){ Token.Id(PrTokenId::Identifier); Token.Value.Idn=Idn; goto GetTokenEnd; }
                         else{ goto GetTokenErr; }
  
  GetTokenKwdTypBolIdn:; if(_IsKeyWord(Line,Pos,Origin,Kwd,Length)){ Token.Id(PrTokenId::Keyword); Token.Value.Kwd=Kwd; goto GetTokenEnd; }
                         else if(_IsTypeName(Line,TypeList,Pos,Typ,Length)){ Token.Id(PrTokenId::TypeName); Token.Value.Typ=Typ; goto GetTokenEnd; }
                         else if(_IsBoolean(Line,Pos,Bol,Length)){ Token.Id(PrTokenId::Boolean); Token.Value.Bol=Bol; goto GetTokenEnd; }
                         else if(_IsIdentifier(Line,Pos,BaseColNr,Idn,Length,Error)){ Token.Id(PrTokenId::Identifier); Token.Value.Idn=Idn; goto GetTokenEnd; }
                         else{ goto GetTokenErr; }

  GetTokenErr:;          TokenLen=(Line.Search(" ",Pos)>Pos?Line.Search(" ",Pos)-Pos:Line.Length()-Pos);
                         SysMessage(14,_FileName,_LineNr,BaseColNr+Pos).Print(Line.Mid(Pos,TokenLen));
                         return false;

  //Get token end of cases
  GetTokenEnd:;
  
  //Report error
  if(Error){ return false; }

  //Return end position
  EndPos=Pos+Length;

  //Update token position
  Token.FileName=_FileName;
  Token.LineNr=_LineNr;
  Token.ColNr=BaseColNr+Pos;
  
  //Return coden
  return true;

}

//Get sentence type (along with sentence modifiers)
bool Sentence::_GetSentenceModifiers(const String& Line,const Array<String>& TypeList,int CumulLen,OrigBuffer Origin,PrToken& Static,PrToken& Let,PrToken& Init,int& EndPos) const {
  
  //Variables
  int Pos1,Pos2;
  PrToken Token;

  //Init sentence modifiers
  Static.Clear();
  Let.Clear();
  Init.Clear();

  //When first token is a sentence modifier get one more token
  Pos1=0;
  while(true){
    
    //Get token
    if(!_GetToken(Line,TypeList,CumulLen,Origin,Pos1,Pos2,Token)){ return false; }

    //No keyword? Finish here
    if(Token.Id()!=PrTokenId::Keyword){ break; }

    //Get sentence modifiers
    if(Token.Value.Kwd==PrKeyword::Static){
      Static=Token;
    }
    else if(Token.Value.Kwd==PrKeyword::Let){
      Let=Token;
    }
    else if(Token.Value.Kwd==PrKeyword::Init){
      Init=Token;
    }
    else{
      break;
    }

    //Update position
    Pos1=Pos2;

  }

  //Return code and position
  EndPos=Pos1;
  return true;

}

//Get sentence identifier
bool Sentence::_GetSentenceId(CodeBlock Block,SentenceId& StnId){
  
  //Variables
  int EndBracketIndex;
  int NextTokenIndex;

  //Determine sentence type when first token is keyword
  if(Tokens[0].Id()==PrTokenId::Keyword){
    switch(Tokens[0].Value.Kwd){
      case PrKeyword::Libs:        StnId=SentenceId::Libs;        break;
      case PrKeyword::Public:      StnId=SentenceId::Public;      break;
      case PrKeyword::Private:     StnId=SentenceId::Private;     break;
      case PrKeyword::Implem:      StnId=SentenceId::Implem;      break;
      case PrKeyword::Set:         StnId=SentenceId::Set;         break;
      case PrKeyword::Import:      StnId=SentenceId::Import;      break;
      case PrKeyword::Include:     StnId=SentenceId::Include;     break;
      case PrKeyword::Var:         StnId=SentenceId::VarDecl;     break;
      case PrKeyword::Const:       StnId=SentenceId::Const;       break;
      case PrKeyword::DefType:     StnId=SentenceId::DefType;     break;
      case PrKeyword::DefClass:    StnId=SentenceId::DefClass;    break;
      case PrKeyword::Publ:        StnId=SentenceId::Publ;        break;
      case PrKeyword::Priv:        StnId=SentenceId::Priv;        break;
      case PrKeyword::EndClass:    StnId=SentenceId::EndClass;    break;
      case PrKeyword::Allow:       StnId=SentenceId::Allow;       break;
      case PrKeyword::DefEnum:     StnId=SentenceId::DefEnum;     break;
      case PrKeyword::EndEnum:     StnId=SentenceId::EndEnum;     break;
      case PrKeyword::SystemCall:  StnId=SentenceId::SystemCall;  break;
      case PrKeyword::SystemFunc:  StnId=SentenceId::SystemFunc;  break;
      case PrKeyword::DlFunction:  StnId=SentenceId::DlFunction;  break;
      case PrKeyword::DlType:      StnId=SentenceId::DlType;      break;
      case PrKeyword::Main:        StnId=SentenceId::Main;        break;
      case PrKeyword::EndMain:     StnId=SentenceId::EndMain;     break;
      case PrKeyword::Function:    StnId=SentenceId::Function;    break;
      case PrKeyword::EndFunction: StnId=SentenceId::EndFunction; break;
      case PrKeyword::Member:      StnId=SentenceId::Member;      break;
      case PrKeyword::EndMember:   StnId=SentenceId::EndMember;   break;
      case PrKeyword::Operator:    StnId=SentenceId::Operator;    break;
      case PrKeyword::EndOperator: StnId=SentenceId::EndOperator; break;
      case PrKeyword::Return:      StnId=SentenceId::Return;      break;
      case PrKeyword::If:          StnId=SentenceId::If;          break;
      case PrKeyword::ElseIf:      StnId=SentenceId::ElseIf;      break;
      case PrKeyword::Else:        StnId=SentenceId::Else;        break;
      case PrKeyword::EndIf:       StnId=SentenceId::EndIf;       break;
      case PrKeyword::While:       StnId=SentenceId::While;       break;
      case PrKeyword::EndWhile:    StnId=SentenceId::EndWhile;    break;
      case PrKeyword::Do:          StnId=SentenceId::Do;          break;
      case PrKeyword::Loop:        StnId=SentenceId::Loop;        break;
      case PrKeyword::For:         StnId=SentenceId::For;         break;
      case PrKeyword::EndFor:      StnId=SentenceId::EndFor;      break;
      case PrKeyword::Switch:      StnId=SentenceId::Switch;      break;
      case PrKeyword::When:        StnId=SentenceId::When;        break;
      case PrKeyword::Default:     StnId=SentenceId::Default;     break;
      case PrKeyword::EndSwitch:   StnId=SentenceId::EndSwitch;   break;
      case PrKeyword::Break:       StnId=SentenceId::Break;       break;
      case PrKeyword::Continue:    StnId=SentenceId::Continue;    break;
      case PrKeyword::Walk:        StnId=SentenceId::Walk;        break;
      case PrKeyword::EndWalk:     StnId=SentenceId::EndWalk;     break;
      case PrKeyword::Void:        StnId=SentenceId::FunDecl;     break;
      case PrKeyword::XlvSet:      StnId=SentenceId::XlvSet;      break;
      case PrKeyword::InitVar:     StnId=SentenceId::InitVar;     break;
      case PrKeyword::As:
      case PrKeyword::Version:
      case PrKeyword::Static:
      case PrKeyword::Let:
      case PrKeyword::Init:
      case PrKeyword::Ref:
      case PrKeyword::On:
      case PrKeyword::To:
      case PrKeyword::From:
      case PrKeyword::Array:
      case PrKeyword::Index:
        Tokens[0].Msg(15).Print(_Kwd[(int)Tokens[0].Value.Kwd]);
        return false;
    }
  }

  //Sentence type when first token is a typename
  //(if we have a parenthesys after the type we have a function (class constructor),
  //if not we find the next token after the type specification ignoring brackets and:
  //if we have a parenthesys right after (operator) or in next position (regular function) we have a function declaration,
  //if not a variable declaration)
  else if(Tokens[0].Id()==PrTokenId::TypeName){
    if(1<=Tokens.Length()-1 && Tokens[1].Id()==PrTokenId::Punctuator && Tokens[1].Value.Pnc==PrPunctuator::BegParen){
      StnId=SentenceId::FunDecl;
    }
    else{
      if(1<=Tokens.Length()-1 && Tokens[1].Id()==PrTokenId::Punctuator && Tokens[1].Value.Pnc==PrPunctuator::BegBracket){
        EndBracketIndex=ZeroFind(PrPunctuator::EndBracket,2);
        if(EndBracketIndex==-1){ Tokens[1].Msg(560).Print(); return false; }
        NextTokenIndex=EndBracketIndex+1;
      }
      else{
        NextTokenIndex=1;
      }
      if(NextTokenIndex<=Tokens.Length()-1 && Tokens[NextTokenIndex].Id()==PrTokenId::Punctuator && Tokens[NextTokenIndex].Value.Pnc==PrPunctuator::BegParen){
        StnId=SentenceId::FunDecl;
      }
      else if(NextTokenIndex+1<=Tokens.Length()-1 && Tokens[NextTokenIndex+1].Id()==PrTokenId::Punctuator && Tokens[NextTokenIndex+1].Value.Pnc==PrPunctuator::BegParen){
        StnId=SentenceId::FunDecl;
      }
      else{
        StnId=SentenceId::VarDecl;
      }
    }
  }

  //Sentence type when first token is identifier
  else if(Tokens[0].Id()==PrTokenId::Identifier){
    if(Block==CodeBlock::Enum){
      StnId=SentenceId::EnumField;
    }
    else{
      StnId=SentenceId::Expression;
    }
  }

  //Rest of cases we have a expression
  else{
    StnId=SentenceId::Expression;
  }

  //Return code
  return true;

}

//Find token in level zero from arbitrary position
int Sentence::ZeroFind(PrPunctuator Pnc,int FromIndex) const {
  int i;
  int Index;
  int ParLevel;
  int BraLevel;
  int ClyLevel;
  Index=-1;
  ParLevel=0;
  BraLevel=0;
  ClyLevel=0;
  for(i=FromIndex;i<Tokens.Length();i++){
    if(Tokens[i].Id()==PrTokenId::Punctuator){
      if(Tokens[i].Value.Pnc==PrPunctuator::BegParen  ){ ParLevel++; }
      if(Tokens[i].Value.Pnc==PrPunctuator::BegBracket){ BraLevel++; }
      if(Tokens[i].Value.Pnc==PrPunctuator::BegCurly  ){ ClyLevel++; }
      if(Tokens[i].Value.Pnc==Pnc && ParLevel==0 && BraLevel==0 && ClyLevel==0){ Index=i; break; }
      if(Tokens[i].Value.Pnc==PrPunctuator::EndParen  ){ ParLevel--; }
      if(Tokens[i].Value.Pnc==PrPunctuator::EndBracket){ BraLevel--; }
      if(Tokens[i].Value.Pnc==PrPunctuator::EndCurly  ){ ClyLevel--; }
    }
  }
  return Index;
}

//Find token in level zero from processing position
int Sentence::ZeroFind(PrPunctuator Pnc) const {
  return ZeroFind(Pnc,_ProcPos);
}

//Process sentence token
Sentence& Sentence::_ProcToken(PrToken& Token,SentenceProcMode Mode){
  
  //Propagate processing error
  if(_ProcError){ return *this; }
  
  //End of sentence reached
  if(_ProcPos>=Tokens.Length()){
    SysMessage(29,_FileName,_LineNr,Tokens[Tokens.Length()-1].ColNr).Print(Token.Description(),ToString(Tokens[Tokens.Length()-1].ColNr));
    _ProcError=true;
    return *this;
  }

  //Sentence operation
  switch(Mode){

    //Read operation
    case SentenceProcMode::Read:
      if(Tokens[_ProcPos].Id()!=Token.Id()){ Tokens[_ProcPos].Msg(27).Print(PrTokenIdText(Token.Id()),PrTokenIdText(Tokens[_ProcPos].Id())); _ProcError=true; return *this; } 
      Token=Tokens[_ProcPos];
      _ProcPos++;
      break;

    //Get operation
    case SentenceProcMode::Get:
      bool Error=false;
      switch(Token.Id()){
        case PrTokenId::Keyword   : if(Tokens[_ProcPos].Value.Kwd!=Token.Value.Kwd){ Error=true; } break;
        case PrTokenId::Operator  : if(Tokens[_ProcPos].Value.Opr!=Token.Value.Opr){ Error=true; } break;
        case PrTokenId::Punctuator: if(Tokens[_ProcPos].Value.Pnc!=Token.Value.Pnc){ Error=true; } break;
        case PrTokenId::TypeName  :
        case PrTokenId::Identifier:
        case PrTokenId::Boolean   :
        case PrTokenId::Char      :
        case PrTokenId::Short     :
        case PrTokenId::Integer   :
        case PrTokenId::Long      :
        case PrTokenId::Float     :
        case PrTokenId::String    :
          Tokens[_ProcPos].Msg(28).Print(Mode==SentenceProcMode::Get?".Get*()":".Is*()");
          break;
      }
      if(Error){
        Tokens[_ProcPos].Msg(27).Print(Token.Description(),PrTokenIdText(Tokens[_ProcPos].Id()));
        _ProcError=true;
        return *this;
      }
      _ProcPos++;
      break;
  }

  //Return 
  return *this;
  
}

//Sentence token parsing functions
bool Sentence::Ok() const { return !_ProcError; }
void Sentence::ClearError() { _ProcError=false; }
int  Sentence::TokensLeft() const { return Tokens.Length()-_ProcPos; }
int  Sentence::GetProcIndex() const { return _ProcPos; }
void Sentence::SetProcIndex(int Index){ _ProcPos=Index; }
int  Sentence::CurrentLineNr() const { return (_ProcPos>=Tokens.Length()?Tokens[Tokens.Length()-1].LineNr:Tokens[_ProcPos].LineNr); }
int  Sentence::CurrentColNr() const { return (_ProcPos>=Tokens.Length()?Tokens[Tokens.Length()-1].ColNr:Tokens[_ProcPos].ColNr); }
int  Sentence::LastLineNr() const { return (_ProcPos==0?0:Tokens[_ProcPos-1].LineNr); }
int  Sentence::LastColNr() const { return (_ProcPos==0?0:Tokens[_ProcPos-1].ColNr); }
SourceInfo Sentence::LastSrcInfo() const { return (_ProcPos==0?SourceInfo("",-1,0):Tokens[_ProcPos-1].SrcInfo()); }
PrToken Sentence::CurrentToken() const { return Tokens[_ProcPos]; }
bool Sentence::Is(PrTokenId    Id ) const { if(_ProcPos>=Tokens.Length()){ return false; } if(Tokens[_ProcPos].Id()==Id){ return true; } else { return false; } }    
bool Sentence::Is(PrKeyword    Kwd) const { if(_ProcPos>=Tokens.Length()){ return false; } if(Tokens[_ProcPos].Id()==PrTokenId::Keyword    && Tokens[_ProcPos].Value.Kwd==Kwd){ return true; } else { return false; } }    
bool Sentence::Is(PrOperator   Opr) const { if(_ProcPos>=Tokens.Length()){ return false; } if(Tokens[_ProcPos].Id()==PrTokenId::Operator   && Tokens[_ProcPos].Value.Opr==Opr){ return true; } else { return false; } }    
bool Sentence::Is(PrPunctuator Pnc) const { if(_ProcPos>=Tokens.Length()){ return false; } if(Tokens[_ProcPos].Id()==PrTokenId::Punctuator && Tokens[_ProcPos].Value.Pnc==Pnc){ return true; } else { return false; } }    
bool Sentence::Is(PrTokenId    Id ,int Offset) const { if(_ProcPos+Offset<0 || _ProcPos+Offset>=Tokens.Length()){ return false; } if(Tokens[_ProcPos+Offset].Id()==Id){ return true; } else { return false; } }    
bool Sentence::Is(PrKeyword    Kwd,int Offset) const { if(_ProcPos+Offset<0 || _ProcPos+Offset>=Tokens.Length()){ return false; } if(Tokens[_ProcPos+Offset].Id()==PrTokenId::Keyword    && Tokens[_ProcPos+Offset].Value.Kwd==Kwd){ return true; } else { return false; } }    
bool Sentence::Is(PrOperator   Opr,int Offset) const { if(_ProcPos+Offset<0 || _ProcPos+Offset>=Tokens.Length()){ return false; } if(Tokens[_ProcPos+Offset].Id()==PrTokenId::Operator   && Tokens[_ProcPos+Offset].Value.Opr==Opr){ return true; } else { return false; } }    
bool Sentence::Is(PrPunctuator Pnc,int Offset) const { if(_ProcPos+Offset<0 || _ProcPos+Offset>=Tokens.Length()){ return false; } if(Tokens[_ProcPos+Offset].Id()==PrTokenId::Punctuator && Tokens[_ProcPos+Offset].Value.Pnc==Pnc){ return true; } else { return false; } }    
bool Sentence::IsBol() const { if(_ProcPos>=Tokens.Length()){ return false; } if(Tokens[_ProcPos].Id()==PrTokenId::Boolean){ return true; } else { return false; } }    
bool Sentence::IsChr() const { if(_ProcPos>=Tokens.Length()){ return false; } if(Tokens[_ProcPos].Id()==PrTokenId::Char   ){ return true; } else { return false; } }    
bool Sentence::IsShr() const { if(_ProcPos>=Tokens.Length()){ return false; } if(Tokens[_ProcPos].Id()==PrTokenId::Short  ){ return true; } else { return false; } }    
bool Sentence::IsInt() const { if(_ProcPos>=Tokens.Length()){ return false; } if(Tokens[_ProcPos].Id()==PrTokenId::Integer){ return true; } else { return false; } }    
bool Sentence::IsLon() const { if(_ProcPos>=Tokens.Length()){ return false; } if(Tokens[_ProcPos].Id()==PrTokenId::Long   ){ return true; } else { return false; } }    
bool Sentence::IsFlo() const { if(_ProcPos>=Tokens.Length()){ return false; } if(Tokens[_ProcPos].Id()==PrTokenId::Float  ){ return true; } else { return false; } }    
bool Sentence::IsStr() const { if(_ProcPos>=Tokens.Length()){ return false; } if(Tokens[_ProcPos].Id()==PrTokenId::String ){ return true; } else { return false; } }    
Sentence& Sentence::Get(PrKeyword    Kwd){ PrToken Token; Token.Id(PrTokenId::Keyword);    Token.Value.Kwd=Kwd; return _ProcToken(Token,SentenceProcMode::Get); }    
Sentence& Sentence::Get(PrOperator   Opr){ PrToken Token; Token.Id(PrTokenId::Operator);   Token.Value.Opr=Opr; return _ProcToken(Token,SentenceProcMode::Get); }    
Sentence& Sentence::Get(PrPunctuator Pnc){ PrToken Token; Token.Id(PrTokenId::Punctuator); Token.Value.Pnc=Pnc; return _ProcToken(Token,SentenceProcMode::Get); }    
Sentence& Sentence::ReadKw(PrKeyword&    Kwd){ PrToken Token; Token.Id(PrTokenId::Keyword);    _ProcToken(Token,SentenceProcMode::Read); if(!_ProcError){ Kwd=Token.Value.Kwd; } return *this; }
Sentence& Sentence::ReadOp(PrOperator&   Opr){ PrToken Token; Token.Id(PrTokenId::Operator);   _ProcToken(Token,SentenceProcMode::Read); if(!_ProcError){ Opr=Token.Value.Opr; } return *this; }
Sentence& Sentence::ReadPn(PrPunctuator& Pnc){ PrToken Token; Token.Id(PrTokenId::Punctuator); _ProcToken(Token,SentenceProcMode::Read); if(!_ProcError){ Pnc=Token.Value.Pnc; } return *this; }
Sentence& Sentence::ReadTy(String&       Typ){ PrToken Token; Token.Id(PrTokenId::TypeName);   _ProcToken(Token,SentenceProcMode::Read); if(!_ProcError){ Typ=Token.Value.Typ; } return *this; }
Sentence& Sentence::ReadId(String&       Idn){ PrToken Token; Token.Id(PrTokenId::Identifier); _ProcToken(Token,SentenceProcMode::Read); if(!_ProcError){ Idn=Token.Value.Idn; } return *this; }
Sentence& Sentence::ReadBo(CpuBol&       Bol){ PrToken Token; Token.Id(PrTokenId::Boolean);    _ProcToken(Token,SentenceProcMode::Read); if(!_ProcError){ Bol=Token.Value.Bol; } return *this; }
Sentence& Sentence::ReadCh(CpuChr&       Chr){ PrToken Token; Token.Id(PrTokenId::Char);       _ProcToken(Token,SentenceProcMode::Read); if(!_ProcError){ Chr=Token.Value.Chr; } return *this; }
Sentence& Sentence::ReadSh(CpuShr&       Shr){ PrToken Token; Token.Id(PrTokenId::Short);      _ProcToken(Token,SentenceProcMode::Read); if(!_ProcError){ Shr=Token.Value.Shr; } return *this; }
Sentence& Sentence::ReadIn(CpuInt&       Int){ PrToken Token; Token.Id(PrTokenId::Integer);    _ProcToken(Token,SentenceProcMode::Read); if(!_ProcError){ Int=Token.Value.Int; } return *this; }
Sentence& Sentence::ReadLo(CpuLon&       Lon){ PrToken Token; Token.Id(PrTokenId::Long);       _ProcToken(Token,SentenceProcMode::Read); if(!_ProcError){ Lon=Token.Value.Lon; } return *this; }
Sentence& Sentence::ReadFl(CpuFlo&       Flo){ PrToken Token; Token.Id(PrTokenId::Float);      _ProcToken(Token,SentenceProcMode::Read); if(!_ProcError){ Flo=Token.Value.Flo; } return *this; }
Sentence& Sentence::ReadSt(String&       Str){ PrToken Token; Token.Id(PrTokenId::String);     _ProcToken(Token,SentenceProcMode::Read); if(!_ProcError){ Str=Token.Value.Str; } return *this; }

//Get expression till keyword is found
Sentence& Sentence::ReadEx(PrKeyword Kwd,int& Start,int& End){
  int i;
  int Index;
  int ParLevel;
  int BraLevel;
  int ClyLevel;
  if(_ProcError){ return *this; }
  Index=-1;
  ParLevel=0;
  BraLevel=0;
  ClyLevel=0;
  for(i=_ProcPos;i<Tokens.Length();i++){
    if(     Tokens[i].Id()==PrTokenId::Punctuator && Tokens[i].Value.Pnc==PrPunctuator::BegParen  ){ ParLevel++; }
    else if(Tokens[i].Id()==PrTokenId::Punctuator && Tokens[i].Value.Pnc==PrPunctuator::BegBracket){ BraLevel++; }
    else if(Tokens[i].Id()==PrTokenId::Punctuator && Tokens[i].Value.Pnc==PrPunctuator::BegCurly  ){ ClyLevel++; }
    else if(Tokens[i].Id()==PrTokenId::Keyword && Tokens[i].Value.Kwd==Kwd && ParLevel==0 && BraLevel==0 && ClyLevel==0){ Index=i; break; }
    else if(Tokens[i].Id()==PrTokenId::Punctuator && Tokens[i].Value.Pnc==PrPunctuator::EndParen  ){ ParLevel--; }
    else if(Tokens[i].Id()==PrTokenId::Punctuator && Tokens[i].Value.Pnc==PrPunctuator::EndBracket){ BraLevel--; }
    else if(Tokens[i].Id()==PrTokenId::Punctuator && Tokens[i].Value.Pnc==PrPunctuator::EndCurly  ){ ClyLevel--; }
  }
  if(Index==-1){
    Tokens[Tokens.Length()-1].Msg(102).Print(_Kwd[(int)Kwd]);
    _ProcError=true;
    return *this;
  }
  if(Index==_ProcPos){
    Tokens[Index].Msg(103).Print(_Kwd[(int)Kwd]);
    _ProcError=true;
    return *this;
  }
  Start=_ProcPos;
  End=Index-1;
  _ProcPos=Index;
  return *this;
}

//Get expression till operator is found
Sentence& Sentence::ReadEx(PrOperator Opr,int& Start,int& End){
  int i;
  int Index;
  int ParLevel;
  int BraLevel;
  int ClyLevel;
  if(_ProcError){ return *this; }
  Index=-1;
  ParLevel=0;
  BraLevel=0;
  ClyLevel=0;
  for(i=_ProcPos;i<Tokens.Length();i++){
    if(     Tokens[i].Id()==PrTokenId::Punctuator && Tokens[i].Value.Pnc==PrPunctuator::BegParen  ){ ParLevel++; }
    else if(Tokens[i].Id()==PrTokenId::Punctuator && Tokens[i].Value.Pnc==PrPunctuator::BegBracket){ BraLevel++; }
    else if(Tokens[i].Id()==PrTokenId::Punctuator && Tokens[i].Value.Pnc==PrPunctuator::BegCurly  ){ ClyLevel++; }
    else if(Tokens[i].Id()==PrTokenId::Operator && Tokens[i].Value.Opr==Opr && ParLevel==0 && BraLevel==0 && ClyLevel==0){ Index=i; break; }
    else if(Tokens[i].Id()==PrTokenId::Punctuator && Tokens[i].Value.Pnc==PrPunctuator::EndParen  ){ ParLevel--; }
    else if(Tokens[i].Id()==PrTokenId::Punctuator && Tokens[i].Value.Pnc==PrPunctuator::EndBracket){ BraLevel--; }
    else if(Tokens[i].Id()==PrTokenId::Punctuator && Tokens[i].Value.Pnc==PrPunctuator::EndCurly  ){ ClyLevel--; }
  }
  if(Index==-1){
    Tokens[Tokens.Length()-1].Msg(107).Print(_Opr[(int)Opr]);
    _ProcError=true;
    return *this;
  }
  if(Index==_ProcPos){
    Tokens[Index].Msg(108).Print(_Opr[(int)Opr]);
    _ProcError=true;
    return *this;
  }
  Start=_ProcPos;
  End=Index-1;
  _ProcPos=Index;
  return *this;
}

//Get expression till punctuator is found
Sentence& Sentence::ReadEx(PrPunctuator Pnc,int& Start,int& End){
  int i;
  int Index;
  int ParLevel;
  int BraLevel;
  int ClyLevel;
  if(_ProcError){ return *this; }
  Index=-1;
  ParLevel=0;
  BraLevel=0;
  ClyLevel=0;
  for(i=_ProcPos;i<Tokens.Length();i++){
    if(     Tokens[i].Id()==PrTokenId::Punctuator && Tokens[i].Value.Pnc==PrPunctuator::BegParen  ){ ParLevel++; }
    else if(Tokens[i].Id()==PrTokenId::Punctuator && Tokens[i].Value.Pnc==PrPunctuator::BegBracket){ BraLevel++; }
    else if(Tokens[i].Id()==PrTokenId::Punctuator && Tokens[i].Value.Pnc==PrPunctuator::BegCurly  ){ ClyLevel++; }
    else if(Tokens[i].Id()==PrTokenId::Punctuator && Tokens[i].Value.Pnc==Pnc && ParLevel==0 && BraLevel==0 && ClyLevel==0){ Index=i; break; }
    else if(Tokens[i].Id()==PrTokenId::Punctuator && Tokens[i].Value.Pnc==PrPunctuator::EndParen  ){ ParLevel--; }
    else if(Tokens[i].Id()==PrTokenId::Punctuator && Tokens[i].Value.Pnc==PrPunctuator::EndBracket){ BraLevel--; }
    else if(Tokens[i].Id()==PrTokenId::Punctuator && Tokens[i].Value.Pnc==PrPunctuator::EndCurly  ){ ClyLevel--; }
  }
  if(Index==-1){
    Tokens[Tokens.Length()-1].Msg(129).Print(_Pnc[(int)Pnc]);
    _ProcError=true;
    return *this;
  }
  if(Index==_ProcPos){
    Tokens[Index].Msg(130).Print(_Pnc[(int)Pnc]);
    _ProcError=true;
    return *this;
  }
  Start=_ProcPos;
  End=Index-1;
  _ProcPos=Index;
  return *this;
}

//Get expression till end of sentence
Sentence& Sentence::ReadEx(int& Start,int& End){
  if(_ProcError){ return *this; }
  if(_ProcPos>=Tokens.Length()){
    Tokens[Tokens.Length()-1].Msg(101).Print();
    _ProcError=true;
    return *this;
  }
  Start=_ProcPos;
  End=Tokens.Length()-1;
  _ProcPos=Tokens.Length();
  return *this;
}

//PrPunctuator parsing counter
Sentence& Sentence::Count(PrPunctuator Pnc,int& Nr){ 
  if(_ProcError){ return *this; }
  Nr=0;
  PrToken Token; 
  Token.Id(PrTokenId::Punctuator); 
  Token.Value.Pnc=Pnc; 
  while(Is(Pnc)){
    Get(Pnc);
    Nr++;
  }
  return *this; 
}    

//Return part of a sentence
Sentence Sentence::SubSentence(int Start,int End) const {
  int i;
  Sentence Stn;
  Stn=*this;
  Stn.Tokens.Reset();
  for(i=Start;i<=End;i++){
    Stn.Tokens.Add(Tokens[i]);
  }
  Stn._ProcPos=0;
  return Stn;
} 

//Add operator token to sentence
Sentence& Sentence::Add(PrOperator Opr){
  PrToken Token;
  Token=Tokens[Tokens.Length()-1];
  Token.Id(PrTokenId::Operator);
  Token.Value.Opr=Opr;
  Token.ColNr=0;
  Tokens.Add(Token);
  return *this;
}

//Add punctuator token to sentence
Sentence& Sentence::Add(PrPunctuator Pnc){
  PrToken Token;
  Token=Tokens[Tokens.Length()-1];
  Token.Id(PrTokenId::Punctuator);
  Token.Value.Pnc=Pnc;
  Token.ColNr=0;
  Tokens.Add(Token);
  return *this;
}

//Add char token to sentence
Sentence& Sentence::Add(CpuChr Number){
  PrToken Token;
  Token=Tokens[Tokens.Length()-1];
  Token.Id(PrTokenId::Char);
  Token.Value.Chr=Number;
  Token.ColNr=0;
  Tokens.Add(Token);
  return *this;
}

//Add integer token to sentence
Sentence& Sentence::Add(CpuInt Number){
  PrToken Token;
  Token=Tokens[Tokens.Length()-1];
  Token.Id(PrTokenId::Integer);
  Token.Value.Int=Number;
  Token.ColNr=0;
  Tokens.Add(Token);
  return *this;
}

//Add identifier token to sentence
Sentence& Sentence::Add(const String& Idn){
  PrToken Token;
  Token=Tokens[Tokens.Length()-1];
  Token.Id(PrTokenId::Identifier);
  Token.Value.Idn=Idn;
  Token.ColNr=0;
  Tokens.Add(Token);
  return *this;
}

//Insert keyword token to sentence
Sentence& Sentence::InsKwd(PrKeyword Kwd,int Position){
  PrToken Token;
  Token=Tokens[Tokens.Length()-1];
  Token.Id(PrTokenId::Keyword);
  Token.Value.Kwd=Kwd;
  Token.ColNr=0;
  Tokens.Insert(Position,Token);
  return *this;
}

//Insert operator token to sentence
Sentence& Sentence::InsOpr(PrOperator Opr,int Position){
  PrToken Token;
  Token=Tokens[Tokens.Length()-1];
  Token.Id(PrTokenId::Operator);
  Token.Value.Opr=Opr;
  Token.ColNr=0;
  Tokens.Insert(Position,Token);
  return *this;
}

//Insert punctuator token to sentence
Sentence& Sentence::InsPnc(PrPunctuator Pnc,int Position){
  PrToken Token;
  Token=Tokens[Tokens.Length()-1];
  Token.Id(PrTokenId::Punctuator);
  Token.Value.Pnc=Pnc;
  Token.ColNr=0;
  Tokens.Insert(Position,Token);
  return *this;
}

//Insert identifier token to sentence
Sentence& Sentence::InsIdn(const String& Idn,int Position){
  PrToken Token;
  Token=Tokens[Tokens.Length()-1];
  Token.Id(PrTokenId::Identifier);
  Token.Value.Idn=Idn;
  Token.ColNr=0;
  Tokens.Insert(Position,Token);
  return *this;
}

//Insert type name token to sentence
Sentence& Sentence::InsTyp(const String& Typ,int Position){
  PrToken Token;
  Token=Tokens[Tokens.Length()-1];
  Token.Id(PrTokenId::TypeName);
  Token.Value.Typ=Typ;
  Token.ColNr=0;
  Tokens.Insert(Position,Token);
  return *this;
}

//Concatenate two sentences
Sentence operator+(const Sentence& Stn1,const Sentence& Stn2){
  int i;
  Sentence Stn;
  Stn=Stn1;
  for(i=0;i<Stn2.Tokens.Length();i++){
    Stn.Tokens.Add(Stn2.Tokens[i]);
  }
  Stn._ProcPos=0;
  return Stn;
}

//Sentence set source line
bool Sentence::Parse(const String& FileName,int LineNr,const String& Line,int CumulLen,const Array<String>& TypeList,CodeBlock Block,OrigBuffer Origin){
  
  //Variables
  int Pos;
  int EndPos;
  PrToken Token;
  PrToken TokenStatic;
  PrToken TokenLet;
  PrToken TokenInit;

  //Initialize internal variables
  _FileName=FileName;
  _LineNr=LineNr;
  _ProcPos=0;
  _ProcError=false;
  _BaseLabel=0;
  _SubLabel=0;
  _BlockId="";
  _LoopLabel=0;
  _LoopId="";
  _Origin=Origin;
  _CodeBlockId=0;
  Static=false; 
  Let=false; 
  Init=false; 
  Tokens.Clear();

  //Detect empty sentence (Empty sentence is only happening at end of source, we decrease line number to provide a real line number)
  if(Line.Length()==0 || Line.ContainsOnly(String(_LineSplitter)+" ")){ Id=SentenceId::Empty; return true; }
  
  //Get sentence modifiers
  if(!_GetSentenceModifiers(Line,TypeList,CumulLen,Origin,TokenStatic,TokenLet,TokenInit,EndPos)){ return false; } else{ Pos=EndPos; }
  if(TokenStatic.Id()!=(PrTokenId)-1){ Static=true; }
  if(TokenLet.Id()!=(PrTokenId)-1){ Let=true;  }
  if(TokenInit.Id()!=(PrTokenId)-1){ Init=true; }

  //Token parse loop
  Pos=EndPos;
  while(Pos<Line.Length()){
    if(!_GetToken(Line,TypeList,CumulLen,Origin,Pos,EndPos,Token)){ return false; }
    Tokens.Add(Token);
    Pos=EndPos;
  };

  //Derive sentence type
  if(!_GetSentenceId(Block,Id)){ return false; }

  //Check validity of sentence modifiers
  if(Static && Id!=SentenceId::VarDecl){ TokenStatic.Msg(16).Print(); return false;  }
  if(Let && ((Id!=SentenceId::Function && Id!=SentenceId::Operator) || Block!=CodeBlock::Local)){ TokenLet.Msg(190).Print(); return false;  }
  if(Init && Id!=SentenceId::FunDecl){ TokenInit.Msg(546).Print(); return false; }

  //Error if sentence is empty but it has sentence modifiers
  if(Tokens.Length()==0){
    if(Static){ TokenStatic.Msg(97).Print(); return false; }
    if(Let){ TokenLet.Msg(98).Print(); return false; }
    if(Init){ TokenInit.Msg(559).Print(); return false; }
  }

  //Ignore line splitter token at end of sentence
  //(necessary to be on source lines for correct positioning of tokens but can be totally ignored)
  if(Tokens.Length()!=0 && Tokens[Tokens.Length()-1].Id()==PrTokenId::Punctuator && Tokens[Tokens.Length()-1].Value.Pnc==PrPunctuator::Splitter){
    Tokens.Delete(Tokens.Length()-1);
  }
  
  //Return code
  return true;

}

//Enable parsing identifiers in system name space
void Sentence::EnableSysNameSpace(bool Enable){
  _EnableSysNameSpace=Enable;
}

//Set labels
void Sentence::SetLabels(long BaseLabel,long SubLabel,const String& BlockId,long LoopLabel,const String& LoopId){
  _BaseLabel=BaseLabel;
  _SubLabel=SubLabel;
  _BlockId=BlockId;
  _LoopLabel=LoopLabel;
  _LoopId=LoopId;
}

//Get sentence label
String Sentence::GetLabel(CodeLabelId LabelId){
  return _ParserGetCodeLabel(LabelId,_BaseLabel,_SubLabel,_BlockId,_LoopLabel,_LoopId);
}

//Set code block id
void Sentence::SetCodeBlockId(CpuLon CodeBlockId){
  _CodeBlockId=CodeBlockId;
} 

//Get code block id
CpuLon Sentence::GetCodeBlockId() const {
  return _CodeBlockId;
}

//Return sentenceorigin
OrigBuffer Sentence::Origin(){
  return _Origin;
}

//Is sentence inside a loop
bool Sentence::InsideLoop(){
  return _LoopLabel!=-1?true:false;
}

//Sentence printout
String Sentence::Print() const {
  return Print(0,Tokens.Length()-1);
}

//Sentence printout with begining and ending token
String Sentence::Print(int BegToken,int EndToken) const {
  String Output;
  for(int i=BegToken;i<=EndToken;i++){
    Output+=(Output.Length()>0?" ":"")+Tokens[i].Print();
  }
  return "["+SentenceIdText(Id)+"]"+(Static?"[static]":"")+(Let?"[let]":"")+(Init?"[init]":"")+" "+Output;
}

//Sentence text (original code)
String Sentence::Text() const {
  return Text(0,Tokens.Length()-1);
}

//Sentence text (original code) with begining and ending token
String Sentence::Text(int BegToken,int EndToken) const {
  PrTokenId PrevId;
  String Output;
  PrevId=(PrTokenId)0;
  for(int i=BegToken;i<=EndToken;i++){
    if(i==BegToken){
      Output=Tokens[i].Text();
    }
    else{
      if((PrevId==PrTokenId::Keyword || PrevId==PrTokenId::TypeName || PrevId==PrTokenId::Identifier || PrevId==PrTokenId::Boolean)
      && (Tokens[i].Id()==PrTokenId::Keyword || Tokens[i].Id()==PrTokenId::TypeName || Tokens[i].Id()==PrTokenId::Identifier || Tokens[i].Id()==PrTokenId::Boolean)){
        Output+=" ";
      }
      Output+=Tokens[i].Text();
    }
    PrevId=Tokens[i].Id();
  }
  return Output;
}

//Sentence message over last processed message
SysMessage Sentence::Msg(int Code) const {
  if(_ProcPos!=0){
    return SysMessage(Code,_FileName,_LineNr,Tokens[_ProcPos-1].ColNr);
  }
  else{
    return SysMessage(Code,_FileName,_LineNr);
  }
}

//Get current file name
String Sentence::FileName() const {
  return _FileName;
}

//Get current line
int Sentence::LineNr() const {
  return _LineNr;
} 

//Sentence constructor
Sentence::Sentence(){
  _FileName="";
  _LineNr=0;
  _ProcPos=0;
  _ProcError=false;
  _BaseLabel=0;
  _SubLabel=0;
  _BlockId="";
  _LoopLabel=0;
  _LoopId="";
  _EnableSysNameSpace=false;
  _Origin=(OrigBuffer)-1;
  _CodeBlockId=0;
  Static=false;
  Let=false;
  Init=false;
  Id=(SentenceId)-1;
  Tokens.Reset();
}

//Sentence copy constructor
Sentence::Sentence(const Sentence& Stn){
  _Move(Stn);
}

//Sentence assignment                
Sentence& Sentence::operator=(const Sentence& Stn){
  _Move(Stn);
  return *this;
}

//Sentence move
void Sentence::_Move(const Sentence& Stn){
  _FileName=Stn._FileName;
  _LineNr=Stn._LineNr;
  _ProcPos=Stn._ProcPos;
  _ProcError=Stn._ProcError;
  _BaseLabel=Stn._BaseLabel;
  _SubLabel=Stn._SubLabel;
  _BlockId=Stn._BlockId;
  _LoopLabel=Stn._LoopLabel;
  _LoopId=Stn._LoopId;
  _EnableSysNameSpace=Stn._EnableSysNameSpace;
  _Origin=Stn._Origin;
  _CodeBlockId=Stn._CodeBlockId;
  Static=Stn.Static;
  Let=Stn.Let;
  Init=Stn.Init;
  Id=Stn.Id;
  Init=Stn.Init;
  Tokens=Stn.Tokens;
}

//Reset parser state
void ParserState::Reset(){
  GlobalBaseLabel=0;
  CodeBlock.Reset();
  ClosedBlocks.Reset();
  DelStack.Reset();
  TypeList.Reset();
}

//Parser state constructor
ParserState::ParserState(){
  Reset();
}

//Parser state copy constructor
ParserState::ParserState(const ParserState& State){
  _Move(State);
}

//Parser state assignment
ParserState& ParserState::operator=(const ParserState& State){
  _Move(State);
  return *this;
}

//Parser state move
void ParserState::_Move(const ParserState& State){
  TypeList=State.TypeList;
  CodeBlock=State.CodeBlock;
  ClosedBlocks=State.ClosedBlocks;
  DelStack=State.DelStack;
  GlobalBaseLabel=State.GlobalBaseLabel;
}

//Get source line (performs line joins and line splits)
bool Parser::_GetSourceLine(String& Line,OrigBuffer& Origin,bool& EndOfSource){

  //Text escape sequence
  const String TextEscapeSeq=String(_TextQualifier)+String(_TextEscapeMarker);

  //Variables
  int i;
  bool DoSplit;
  bool MaySplit;
  String SourceLine;
  String OriginName;
  Array<String> Lines;

  //Read non empty line loop
  EndOfSource=false;
  do{

    //Set variables
    int LineJoinNr=0;
    bool Continue=false;
    bool RawStrMode=false;

    //Read line loop
    do{
  
      //Set variables
      long CommentPos;
      String SingleLine;
      bool RawStrBeg=false;
      bool RawStrEnd=false;
      long RawStrBegPos;
      long RawStrEndPos;

      //Read single line from split, insertion or source buffer
      if(_InsBuffer.Length()!=0){ 
        SingleLine=_InsBuffer.Dequeue(); 
        Origin=OrigBuffer::Insertion;
        OriginName="ins";
        MaySplit=true;
      }
      else if(_AuxBuffer.Length()!=0){ 
        SingleLine=_AuxBuffer.Dequeue(); 
        Origin=OrigBuffer::Split;
        OriginName="spl";
        MaySplit=false;
      }
      else if(_BufferNr<_Buffer.Length()){
        SingleLine=_Buffer[_BufferNr];
        _BufferNr++;
        Origin=OrigBuffer::Source;
        OriginName="src";
        MaySplit=true;
      }
      else if(_AddBuffer.Length()!=0){ 
        SingleLine=_AddBuffer.Dequeue(); 
        Origin=OrigBuffer::Addition;
        OriginName="Add";
        MaySplit=true;
      }
      else{
        EndOfSource=true;
        SingleLine="";
        Origin=OrigBuffer::Source;
        OriginName="src";
        MaySplit=false;
      }

      //Filter comments
      if(!RawStrMode && (CommentPos=SingleLine.Search(_CommentMark,_TextQualifier,TextEscapeSeq,_RawStrBeg,_RawStrEnd,_SplitLevelUp,_SplitLevelDown))!=-1){
        SingleLine=SingleLine.Mid(0,(CommentPos==0?0:CommentPos));
      }      
      
      //Convert tabs to spaces and trim spaces on the right (on the left we do not trim since we do not want to alter original column indexes)
      if(!RawStrMode){
        SingleLine=SingleLine.Replace("\t",String(_TabSize,' ')).TrimRight();
      }

      //Detect raw string beginning (search last occurence of raw string markers) and ending (search first occurence of raw string ending)
      RawStrBegPos=SingleLine.Search(_RawStrBeg,0,0);
      RawStrEndPos=SingleLine.Search(_RawStrEnd,0,0);
      if(RawStrBegPos!=-1 && (RawStrEndPos==-1 || RawStrEndPos<RawStrBegPos)){ RawStrBeg=true; }
      RawStrEndPos=SingleLine.Search(_RawStrEnd,0,1);
      if(RawStrEndPos!=-1){ RawStrEnd=true; }

      //Calculate raw string mode openning
      if(RawStrBeg){ RawStrMode=true; }

      //If previous line is a continuation through explicit line join we do trim spaces on left
      if(Continue){ SingleLine=SingleLine.TrimLeft(); }
  
      //If we are in raw string mode then concatename and read new line 
      if(RawStrMode){
        SourceLine+=(SourceLine.Length()!=0?"\n":"")+SingleLine;
        LineJoinNr++;
      }

      //If line ends in line joiner token then concatename and read new line 
      //(they cannot come from split buffer because it contains only splitted lines)
      else if(Origin!=OrigBuffer::Split && SingleLine.EndsWith(_LineJoiner)){
        if(Origin==OrigBuffer::Insertion && _InsBuffer.Length()==0){ SysMessage(220,_FileName,_LineNr).Print(); return false; }
        else if(Origin==OrigBuffer::Addition && _AddBuffer.Length()==0){ SysMessage(330,_FileName,_LineNr).Print(); return false; }
        else if(Origin==OrigBuffer::Source && _BufferNr==_Buffer.Length()){ SysMessage(7,_FileName,_LineNr).Print(); return false; }
        SourceLine+=SingleLine.CutRight(_LineJoiner.Length());
        LineJoinNr++;
        Continue=true;
      }

      //Normal lines
      else{
        SourceLine+=SingleLine;
        Continue=false;
      }
  
      //Calculate raw string mode closing
      if(!RawStrBeg && RawStrEnd){ RawStrMode=false; }

    }while((Continue || RawStrMode) && !EndOfSource);

    //Error if end of source is reached and we have line continuation or rawstring open
    if(EndOfSource){
      if(Continue){ SysMessage(335,_FileName,_LineNr).Print(); return false; }
      if(RawStrMode){ SysMessage(336,_FileName,_LineNr).Print(); return false; }
    }

    //Calculate current source line
    if(Origin==OrigBuffer::Source){ _LineNr=_BufferNr-LineJoinNr-1; }
  
    //Perform line split (only for source lines and inserted lines, we insert here the automatic splitters if any)
    if(SourceLine.Length()!=0 && MaySplit && SourceLine.Contains(_SplitChars)){
      DoSplit=false;
      if(SourceLine.Search(_LineSplitter,_TextQualifier,TextEscapeSeq,_RawStrBeg,_RawStrEnd,_SplitLevelUp,_SplitLevelDown)!=-1){ 
        DoSplit=true;
      }
      else{
        for(i=0;i<_AutoSplit.Length();i++){
          if(SourceLine.Search(_AutoSplit[i],_TextQualifier,TextEscapeSeq,_RawStrBeg,_RawStrEnd,_SplitLevelUp,_SplitLevelDown)!=-1){ DoSplit=true; break; }
        }
      }
      if(DoSplit){
        _OrigLine=SourceLine;
        SourceLine.ReplaceWithin(_LineSplitter,_SplitPreserve,_TextQualifier,TextEscapeSeq,_RawStrBeg,_RawStrEnd,_SplitLevelUp,_SplitLevelDown);
        for(i=0;i<_AutoSplit.Length();i++){
          SourceLine.ReplaceWithin(_AutoSplit[i],_AutoSplit[i]+_LineSplitter,_TextQualifier,TextEscapeSeq,_RawStrBeg,_RawStrEnd,_SplitLevelUp,_SplitLevelDown);
        }
        Lines=SourceLine.Split(_LineSplitter,_TextQualifier,TextEscapeSeq,_RawStrBeg,_RawStrEnd,_SplitLevelUp,_SplitLevelDown);
        for(int i=0;i<Lines.Length();i++){ 
          if(Lines[i].Length()!=0){ 
            Lines[i].ReplaceWithin(_SplitterMark,_LineSplitter,_TextQualifier,TextEscapeSeq,_RawStrBeg,_RawStrEnd,_SplitLevelUp,_SplitLevelDown);
            if(i>0){
              DebugMessage(DebugLevel::CmpSplitLine,"Queued (spl): "+Lines[i]);
              _AuxBuffer.Enqueue(Lines[i]); 
            }
          }
        }
        SourceLine=Lines[0];
        _CumulLen=0;
      }
    }

    //Calculate cumulated line length
    //(for split buffer we add one char on cumulated length, but only when line was not splitted automatically)
    switch(Origin){
      case OrigBuffer::Source: 
        _CumulLen=SourceLine.Length(); 
        break;
      case OrigBuffer::Insertion: 
        _CumulLen=SourceLine.Length(); 
        break;
      case OrigBuffer::Addition: 
        _CumulLen=SourceLine.Length(); 
        break;
      case OrigBuffer::Split: 
        _CumulLen+=SourceLine.Length(); 
        break;
    }

  }while(SourceLine.Length()==0 && !EndOfSource);

  //Return result
  Line=SourceLine;

  //Debug message
  DebugMessage(DebugLevel::CmpReadLine,"Read line ("+OriginName+"):"+ToString(_LineNr+1)+":"+ToString(_CumulLen)+": \""+Line+"\"");

  //Set source line for compiler messages
  if(Origin==OrigBuffer::Split){
    SysMsgDispatcher::SetSourceLine(_OrigLine);
  }
  else if(Origin==OrigBuffer::Source){
    SysMsgDispatcher::SetSourceLine(SourceLine);
  }
  else{
    SysMsgDispatcher::SetSourceLine(SourceLine);
  }

  //Return code
  return true;

}

//Print code block stack
String Parser::_PrintCodeBlocks() const {
  String Output;
  for(int i=0;i<_CurrState.CodeBlock.Length();i++){
    Output+=(Output.Length()!=0?" ":"")+CodeBlockDefText(_CurrState.CodeBlock[i]);
  }
  return Output;
}

//Opens source file
bool Parser::Open(const String& FileName,bool FromStDin,int TabSize){
  
  //Variables
  int Hnd;
  String Line;

  //Reset buffers (just in case)
  Reset();

  //Set variables
  _FileName=FileName;
  _TabSize=TabSize;
  
  //Init code block stack
  _CurrState.CodeBlock.Push((CodeBlockDef){CodeBlock::Init,0,0});

  //Read file from stdin (linter mode)
  if(FromStDin){
    do{
      std::cin>>Line;
      if(!std::cin.good() && !std::cin.eof()){ SysMessage(401,_FileName,_LineNr).Print(); return false; }
      _Buffer.Add(Line);
      if(std::cin.eof()){ break; }
    }while(true);
  }

  //Read source from file system
  else{
    if(!_Stl->FileSystem.GetHandler(Hnd)){ SysMessage(237,_FileName,_LineNr).Print(FileName); return false; }
    if(!_Stl->FileSystem.OpenForRead(Hnd,FileName)){ SysMessage(359,_FileName,_LineNr).Print(FileName,_Stl->LastError()); return false; }
    if(!_Stl->FileSystem.FullRead(Hnd,_Buffer)){ SysMessage(240,_FileName,_LineNr).Print(FileName,_Stl->LastError()); return false; }
    if(!_Stl->FileSystem.CloseFile(Hnd)){ SysMessage(238).Print(FileName,_Stl->LastError()); return false; }
    if(!_Stl->FileSystem.FreeHandler(Hnd)){ SysMessage(239).Print(FileName,_Stl->LastError()); return false; }
  }

  //Remove extra chars line delimiters (relevant non-unix systems)
  if(GetHostSystem()==HostSystem::Windows){
    for(int i=0;i<_Buffer.Length();i++){
      _Buffer[i]=_Buffer[i].Replace(String((char)LINE_EXTRA),"");
    }
  }

  //Return code
  return true;

}

//Gets single parsed sentence
bool Parser::Get(Sentence& Stn,String& SourceLine,bool& EndOfSource){
  
  //Variables
  int i;
  int StnIndex;
  CodeBlock Block;
  CodeBlock LoopBlock;
  long BaseLabel;
  long SubLabel;
  long LoopLabel;
  String Line;
  OrigBuffer Origin;
  CodeBlockDef BlockDef;
  
  //Debug message
  DebugMessage(DebugLevel::CmpParser,String(85,'-'));

  //Save parser state
  _PrevState=_CurrState;

  //Get new line from source
  if(!_GetSourceLine(Line,Origin,EndOfSource)){ return false; }

  //Get sentence type
  if(Origin==OrigBuffer::Insertion || Origin==OrigBuffer::Addition){ Stn.EnableSysNameSpace(true); }
  if(!Stn.Parse(_FileName,_LineNr+1,Line,_CumulLen,_CurrState.TypeList,_CurrState.CodeBlock.Top().Block,Origin)){ return false; }
  if(Origin==OrigBuffer::Insertion || Origin==OrigBuffer::Addition){ Stn.EnableSysNameSpace(false); }
  DebugMessage(DebugLevel::CmpParser,"Parsed sentence: "+Stn.Print());
  SourceLine=Line;

  //If sentence is empty we end here
  if(Stn.Id==SentenceId::Empty){ return true; }

  //Find sentence definition
  StnIndex=-1;
  for(int i=0;i<_StnNr;i++){
    if((int)_CurrState.CodeBlock.Top(0).Block&_Stn[i].AllowedBlocks && Stn.Id==_Stn[i].Id){
      StnIndex=i;
      break;
    }
  }
  if(StnIndex==-1){
    if(Stn.Tokens.Length()!=0){
      Stn.Tokens[0].Msg(18).Print(SentenceIdText(Stn.Id).Lower(),CodeBlockText(_CurrState.CodeBlock.Top(0).Block).Lower());
    }
    else{
      SysMessage(18,_FileName,_LineNr+1).Print(SentenceIdText(Stn.Id).Lower(),CodeBlockText(_CurrState.CodeBlock.Top(0).Block).Lower());
    }
    return false;
  }

  //Do code block action
  switch(_Stn[StnIndex].Action){
      
    //Keep
    case CodeBlockAction::Keep: 
      break;

    //Push block
    case CodeBlockAction::Push: 
      BlockDef=(CodeBlockDef){_Stn[StnIndex].NewBlock,0,0};
      _CurrState.CodeBlock.Push(BlockDef);
      if(_Stn[StnIndex].PushDelStack){
        _CurrState.DelStack.Push(GetCodeBlockId(BlockDef));
      }
      break;

    //Pop block
    case CodeBlockAction::Pop: 
      if(_CurrState.CodeBlock.Length()==0){ SysMessage(19,_FileName,_LineNr).Print(); return false; }
      BlockDef=_CurrState.CodeBlock.Pop();
      _CurrState.ClosedBlocks.Add(GetCodeBlockId(BlockDef));
      if(_Stn[StnIndex].PopDelStack && _CurrState.DelStack.Length()>0){
        _CurrState.ClosedBlocks.Add(_CurrState.DelStack.Pop());
      }
      break;

    //Replace block
    case CodeBlockAction::Replace: 
      if(_CurrState.CodeBlock.Length()==0){ SysMessage(19,_FileName,_LineNr).Print(); return false; }
      BlockDef=_CurrState.CodeBlock.Pop();
      if(_CurrState.DelStack.Length()!=0 && _CurrState.DelStack.Top()!=GetCodeBlockId(BlockDef)){
        _CurrState.ClosedBlocks.Add(GetCodeBlockId(BlockDef));
      }
      _CurrState.CodeBlock.Push((CodeBlockDef){_Stn[StnIndex].NewBlock,BlockDef.BaseLabel,BlockDef.SubLabel}); 
      break;

  }

  //Calculate baselabel and sublabel depending on jump code
  switch(_Stn[StnIndex].Jmp){
    
    //Nothing to do, just get current block labels
    case JumpMode::None: 
      Block=_CurrState.CodeBlock.Top().Block;
      BaseLabel=_CurrState.CodeBlock.Top().BaseLabel;
      SubLabel=_CurrState.CodeBlock.Top().SubLabel;
      break; 
    
    //Block begin
    //(if previous block is local (local functions) we do nor reset labels to zero but just increase)
    case JumpMode::BlockBeg: 
      if(_PrevState.CodeBlock.Top().Block==CodeBlock::Local){
        if(_CurrState.GlobalBaseLabel==MAX_SHR-1){
          SysMessage(6,_FileName,_LineNr+1).Print();
          return false;
        }
        _CurrState.CodeBlock.Top().BaseLabel=(++_CurrState.GlobalBaseLabel);
        _CurrState.CodeBlock.Top().SubLabel=0;
      }
      else{
        _CurrState.GlobalBaseLabel=0;
        _CurrState.CodeBlock.Top().BaseLabel=0;
        _CurrState.CodeBlock.Top().SubLabel=0;
      }
      Block=_CurrState.CodeBlock.Top().Block;
      BaseLabel=_CurrState.CodeBlock.Top().BaseLabel;
      SubLabel=_CurrState.CodeBlock.Top().SubLabel;
      break; 
    
    //Block end
    case JumpMode::BlockEnd: 
      Block=BlockDef.Block;
      BaseLabel=BlockDef.BaseLabel;
      SubLabel=BlockDef.SubLabel;
      break; 
    
    //Loop beginning (for,do,while,walk)
    case JumpMode::LoopBeg: 
      if(_CurrState.GlobalBaseLabel==MAX_SHR-1){
        SysMessage(6,_FileName,_LineNr+1).Print();
        return false;
      }
      _CurrState.CodeBlock.Top().BaseLabel=(++_CurrState.GlobalBaseLabel);
      _CurrState.CodeBlock.Top().SubLabel=0;
      Block=_CurrState.CodeBlock.Top().Block;
      BaseLabel=_CurrState.CodeBlock.Top().BaseLabel;
      SubLabel=_CurrState.CodeBlock.Top().SubLabel;
      break; 
    
    //Loop endding (endif,loop,endwhile,endwalk)
    case JumpMode::LoopEnd: 
      Block=BlockDef.Block;
      BaseLabel=BlockDef.BaseLabel;
      SubLabel=BlockDef.SubLabel;
      break; 
    
    //First case (if,first switch case)
    case JumpMode::FirstCase: 
      if(_CurrState.GlobalBaseLabel==MAX_SHR-1){
        SysMessage(6,_FileName,_LineNr+1).Print();
        return false;
      }
      _CurrState.CodeBlock.Top().BaseLabel=(++_CurrState.GlobalBaseLabel);
      _CurrState.CodeBlock.Top().SubLabel=0;
      Block=_CurrState.CodeBlock.Top().Block;
      BaseLabel=_CurrState.CodeBlock.Top().BaseLabel;
      SubLabel=_CurrState.CodeBlock.Top().SubLabel;
      break; 
    
    //Next case (elseif, next switch case)
    case JumpMode::NextCase: 
      if(_CurrState.CodeBlock.Top().SubLabel==MAX_SHR-1){
        SysMessage(6,_FileName,_LineNr+1).Print();
        return false;
      }
      _CurrState.CodeBlock.Top().SubLabel++;
      Block=_CurrState.CodeBlock.Top().Block;
      BaseLabel=_CurrState.CodeBlock.Top().BaseLabel;
      SubLabel=_CurrState.CodeBlock.Top().SubLabel;
      break; 
    
    //Last case (else, default)
    case JumpMode::LastCase: 
      if(_CurrState.CodeBlock.Top().SubLabel==MAX_SHR-1){
        SysMessage(6,_FileName,_LineNr+1).Print();
        return false;
      }
      _CurrState.CodeBlock.Top().SubLabel++;
      Block=_CurrState.CodeBlock.Top().Block;
      BaseLabel=_CurrState.CodeBlock.Top().BaseLabel;
      SubLabel=_CurrState.CodeBlock.Top().SubLabel;
      break; 
    
    //End case (endif, endswitch)
    case JumpMode::EndCase: 
      Block=BlockDef.Block;
      BaseLabel=BlockDef.BaseLabel;
      SubLabel=BlockDef.SubLabel;
      break; 

  }

  //Check labels are under permitted length
  if(ToString(_CurrState.CodeBlock.Top().BaseLabel).Length()>_CodeLabelLen || ToString(_CurrState.CodeBlock.Top().SubLabel).Length()>_CodeLabelLen){
    SysMessage(6,_FileName,_LineNr+1).Print();
    return false;
  }

  //Find inner loop label
  LoopLabel=-1;
  LoopBlock=(CodeBlock)-1;
  for(i=_CurrState.CodeBlock.Length()-1;i>=0;i--){
    if(_CurrState.CodeBlock[i].Block==CodeBlock::DoLoop 
    || _CurrState.CodeBlock[i].Block==CodeBlock::While 
    || _CurrState.CodeBlock[i].Block==CodeBlock::For 
    || _CurrState.CodeBlock[i].Block==CodeBlock::Walk){
      LoopLabel=_CurrState.CodeBlock[i].BaseLabel;
      LoopBlock=_CurrState.CodeBlock[i].Block;
      break;
    }
  }

  //Copy labels into sentence
  Stn.SetLabels(BaseLabel,SubLabel,CodeBlockJumpId(Block),LoopLabel,CodeBlockJumpId(LoopBlock));

  //Set code block id
  Stn.SetCodeBlockId(GetCodeBlockId((CodeBlockDef){Block,(CpuShr)BaseLabel,(CpuShr)SubLabel}));

  //Show debug messages
  DebugMessage(DebugLevel::CmpJump, "Sentence labels: BaseLabel="+ToString(BaseLabel)+
  " SubLabel="+ToString(SubLabel)+" BlockId={"+CodeBlockJumpId(Block)+"}"
  " LoopLabel="+ToString(LoopLabel)+" LoopId={"+CodeBlockJumpId(LoopBlock)+"}");
  #ifdef __DEV__
  if(_Stn[StnIndex].Jmp!=JumpMode::None){
    DebugMessage(DebugLevel::CmpJump, "Jump event: "+_JumpModeText(_Stn[StnIndex].Jmp)+" GlobalBaseLabel="+ToString(_CurrState.GlobalBaseLabel));
    if(_Stn[StnIndex].Action!=CodeBlockAction::Keep && _Stn[StnIndex].Jmp!=JumpMode::None){
      DebugMessage(DebugLevel::CmpCodeBlock, "Code block stack updated: "+_CodeBlockActionText(_Stn[StnIndex].Action)+" --> {"+_PrintCodeBlocks()+"}  ");
    }
  }
  if(_CurrState.ClosedBlocks.Length()!=0){
    DebugAppend(DebugLevel::CmpCodeBlock,"Closed blocks: ");
    for(i=0;i<_CurrState.ClosedBlocks.Length();i++){
      DebugAppend(DebugLevel::CmpCodeBlock,(i!=0?",":"")+CodeBlockDefText(GetCodeBlockDef(_CurrState.ClosedBlocks[i])));
    }
    DebugMessage(DebugLevel::CmpCodeBlock,"");
  }
  #endif

  //Get sentence
  return true;

}

//Queue line in the insertion buffer
void Parser::Insert(const String& Line){
  _InsBuffer.Enqueue(Line);
  DebugMessage(DebugLevel::CmpInsLine,"Queued (ins): "+Line);
}

//Queue line in the addition buffer
void Parser::Add(const String& Line,bool AtTop){
  if(AtTop){
    _AddBuffer.EnqueueFirst(Line);
    DebugMessage(DebugLevel::CmpInsLine,"Queued first (add): "+Line);
  }
  else{
    _AddBuffer.Enqueue(Line);
    DebugMessage(DebugLevel::CmpInsLine,"Queued (add): "+Line);
  }
}

//Reset parser
void Parser::Reset(){
  _FileName="";
  _TabSize=0;
  _BufferNr=0;
  _LineNr=0;
  _CumulLen=0;
  _OrigLine="";
  _Buffer.Reset();
  _AuxBuffer.Reset();
  _InsBuffer.Reset();
  _AddBuffer.Reset();
  _CurrState.Reset();
  _PrevState.Reset();
}
//Restore parser state
void Parser::StateBack(){
  _CurrState=_PrevState;
  DebugMessage(DebugLevel::CmpParser,"Parser state restored");
  DebugMessage(DebugLevel::CmpJump, "Global base label restored:  GlobalBaseLabel="+ToString(_CurrState.GlobalBaseLabel));
  DebugMessage(DebugLevel::CmpCodeBlock, "Code block stack restored: {"+_PrintCodeBlocks()+"}  ");
}

//Get current parser code block
CodeBlock Parser::CurrentBlock() const {
  return _CurrState.CodeBlock.Top().Block;
}

//Get closed blocks in parser state
const Array<CpuLon>& Parser::GetClosedBlocks() const {
  return _CurrState.ClosedBlocks;
}

//Clear closed blocks in parser state
void Parser::ClearClosedBlocks() {
  _CurrState.ClosedBlocks.Reset();
}

//Set type identifiers
void Parser::SetTypeIds(const String& TypeIds) {
  _CurrState.TypeList=TypeIds.Split(_TypeIdSeparator);
  DebugMessage(DebugLevel::CmpParser,"Updated parser type list: "+TypeIds);
}

//Look for library option enabled
bool Parser::LibraryOptionFound() const {
  
  //Variables
  int i;
  String WorkLine;
  
  //Loop over source lines
  for(i=0;i<_Buffer.Length();i++){
    
    //Clean spaces in line
    WorkLine=_Buffer[i].Replace("\t"," ");
    WorkLine=WorkLine.Trim();
    while(WorkLine.Search("  ")!=-1){ WorkLine=WorkLine.Replace("  "," "); }
    while(WorkLine.Search("= ")!=-1){ WorkLine=WorkLine.Replace("= ","="); }
    while(WorkLine.Search(" =")!=-1){ WorkLine=WorkLine.Replace(" =","="); }
    
    //Find relevant sentences
    if(WorkLine.StartsWith("set library=true")){
      return true;
    }
    else if(WorkLine.StartsWith(".libs") || WorkLine.StartsWith(".public") || WorkLine.StartsWith(".private") || WorkLine.StartsWith(".implem")){
      return false;
    }
  
  }
  
  //Return not found
  return false;

}

//Parser constructor
Parser::Parser(){
  
  //Variables
  int i;

  //Reset fields
  Reset();

  //Calculate automatic splitters
  //(Everyplace where ): is found, plus all keywords that end or start by : except loop, which expects condition right after)
  _AutoSplit.Add("):");
  for(i=0;i<_KwdNr;i++){
    if((String(_Kwd[i]).StartsWith(":") || String(_Kwd[i]).EndsWith(":")) && i!=(int)PrKeyword::Loop){
      _AutoSplit.Add(_Kwd[i]);
    }
  }

  //Calculate chars that produce splitting
  _SplitChars.Add(':');
  _SplitChars.Add(_LineSplitter);

}

//Parser copy constructor
Parser::Parser(const Parser& Prs){
  _Move(Prs);
}

//Parser assignment
Parser& Parser::operator=(const Parser& Prs){
  _Move(Prs);
  return *this;
}

//Parser move
void Parser::_Move(const Parser& Prs){
  _Buffer=Prs._Buffer;
  _FileName=Prs._FileName;
  _TabSize=Prs._TabSize;
  _BufferNr=Prs._BufferNr;
  _LineNr=Prs._LineNr;
  _OrigLine=Prs._OrigLine;
  _CumulLen=Prs._CumulLen;
  _AuxBuffer=Prs._AuxBuffer;
  _InsBuffer=Prs._InsBuffer;
  _AddBuffer=Prs._AddBuffer;
  _CurrState=Prs._CurrState;
  _PrevState=Prs._PrevState;
  _AutoSplit=Prs._AutoSplit;
  _SplitChars=Prs._SplitChars;
}

//Return system name space prefix
String GetSystemNameSpace(){ 
  return String(SYSTEM_NAMESPACE); 
}

//Code block id
String CodeBlockJumpId(CodeBlock Block){
  
  //Variables
  String Id;

  //Switch on block
  switch(Block){

    //Blocks that generate jumps
    case CodeBlock::Switch   : Id="swi"; break;
    case CodeBlock::FirstWhen: Id="swi"; break;
    case CodeBlock::NextWhen : Id="swi"; break;
    case CodeBlock::Default  : Id="swi"; break;
    case CodeBlock::DoLoop   : Id="dlp"; break;
    case CodeBlock::While    : Id="whi"; break;
    case CodeBlock::If       : Id="ifs"; break;
    case CodeBlock::ElseIf   : Id="ifs"; break;
    case CodeBlock::Else     : Id="ifs"; break;
    case CodeBlock::For      : Id="for"; break;
    case CodeBlock::Walk     : Id="wlk"; break;
    
    //Blocks that do not generate jumps
    case CodeBlock::Init     : 
    case CodeBlock::Libs     : 
    case CodeBlock::Public   : 
    case CodeBlock::Private  : 
    case CodeBlock::Implem   : 
    case CodeBlock::Local    : 
    case CodeBlock::Class    : 
    case CodeBlock::Publ     : 
    case CodeBlock::Priv     : 
    case CodeBlock::Enum     : 
      Id="";
      break;
  } 

  //Return result
  return Id; 

}

//Code block name
String CodeBlockText(CodeBlock Block){
  String Name;
  switch(Block){
    case CodeBlock::Init     : Name="Init";           break;
    case CodeBlock::Libs     : Name="Libs";           break;
    case CodeBlock::Public   : Name="ModPublic";      break;
    case CodeBlock::Private  : Name="ModPrivate";     break;
    case CodeBlock::Implem   : Name="Implementation"; break;
    case CodeBlock::Local    : Name="Local";          break;
    case CodeBlock::Class    : Name="Class";          break;
    case CodeBlock::Publ     : Name="ClassPublic";    break;
    case CodeBlock::Priv     : Name="ClassPrivate";   break;
    case CodeBlock::Enum     : Name="Enum";           break;
    case CodeBlock::Switch   : Name="Switch";         break;
    case CodeBlock::FirstWhen: Name="FirstCase";      break;
    case CodeBlock::NextWhen : Name="NextCase";       break;
    case CodeBlock::Default  : Name="Default";        break;
    case CodeBlock::DoLoop   : Name="DoLoop";         break;
    case CodeBlock::While    : Name="WhileLoop";      break;
    case CodeBlock::If       : Name="If";             break;
    case CodeBlock::ElseIf   : Name="ElseIf";         break;
    case CodeBlock::Else     : Name="Else";           break;
    case CodeBlock::For      : Name="For";            break;
    case CodeBlock::Walk     : Name="Walk";           break;
  } 
  return Name; 
}

//Print code block definition
String CodeBlockDefText(CodeBlockDef BlockDef){
  return "{"+CodeBlockText(BlockDef.Block)+":"+ToString(BlockDef.BaseLabel)+":"+ToString(BlockDef.SubLabel)+"}";
}

//Calculate code block id
CpuLon GetCodeBlockId(CodeBlockDef BlockDef){
  return ((int64_t)BlockDef.Block<<32)+((int64_t)BlockDef.BaseLabel<<16)+(int64_t)BlockDef.SubLabel;
}

//Calculate block definition from block id
CodeBlockDef GetCodeBlockDef(CpuLon CodeBlockId){
  return (CodeBlockDef){(CodeBlock)((CodeBlockId&0xFFFFFFFF00000000)>>32),(CpuShr)((CodeBlockId&0x00000000FFFF0000)>>16),(CpuShr)(CodeBlockId&0x000000000000FFFF)};  
}

//Code block name
String CodeBlockIdName(CpuLon CodeBlockId){
  return CodeBlockText(GetCodeBlockDef(CodeBlockId).Block);
}

//Parser token type text
String PrTokenIdText(PrTokenId Id){
  String Text;
  switch(Id){
    case PrTokenId::Keyword   : Text="keyword"; break;
    case PrTokenId::Operator  : Text="operator"; break;
    case PrTokenId::Punctuator: Text="punctuator"; break;
    case PrTokenId::TypeName  : Text="type name"; break;
    case PrTokenId::Identifier: Text="identifier"; break;
    case PrTokenId::Boolean   : Text="boolean litteral"; break;
    case PrTokenId::Char      : Text="char litteral"; break;
    case PrTokenId::Short     : Text="short litteral"; break;
    case PrTokenId::Integer   : Text="integer litteral"; break;
    case PrTokenId::Long      : Text="long litteral"; break;
    case PrTokenId::Float     : Text="float litteral"; break;
    case PrTokenId::String    : Text="string litteral"; break;
  }
  return Text;
}

//Sentence if text
String SentenceIdText(SentenceId Id){
  String Text;
  switch(Id){
    case SentenceId::Libs       : Text="Libs";        break;
    case SentenceId::Public     : Text="Public";      break;
    case SentenceId::Private    : Text="Private";     break;
    case SentenceId::Implem     : Text="Implem";      break;
    case SentenceId::Set        : Text="Set";         break;
    case SentenceId::Import     : Text="Import";      break;
    case SentenceId::Include    : Text="Include";     break;
    case SentenceId::Const      : Text="Const";       break;
    case SentenceId::VarDecl    : Text="VarDecl";     break;
    case SentenceId::DefType    : Text="DefType";     break;
    case SentenceId::DefClass   : Text="DefClass";    break;
    case SentenceId::Publ       : Text="Publ";        break;
    case SentenceId::Priv       : Text="Priv";        break;
    case SentenceId::EndClass   : Text="EndClass";    break;
    case SentenceId::Allow      : Text="Allow";       break;
    case SentenceId::DefEnum    : Text="DefEnum";     break;
    case SentenceId::EnumField  : Text="EnumField";   break;
    case SentenceId::EndEnum    : Text="EndEnum";     break;
    case SentenceId::FunDecl    : Text="FunDecl";     break;
    case SentenceId::Main       : Text="Main";        break;
    case SentenceId::EndMain    : Text="EndMain";     break;
    case SentenceId::Function   : Text="Function";    break;
    case SentenceId::EndFunction: Text="EndFunction"; break;
    case SentenceId::Member     : Text="Member";      break;
    case SentenceId::EndMember  : Text="EndMember";   break;
    case SentenceId::Operator   : Text="Operator";    break;
    case SentenceId::EndOperator: Text="EndOperator"; break;
    case SentenceId::Return     : Text="Return";      break;
    case SentenceId::If         : Text="If";          break;
    case SentenceId::ElseIf     : Text="ElseIf";      break;
    case SentenceId::Else       : Text="Else";        break;
    case SentenceId::EndIf      : Text="EndIf";       break;
    case SentenceId::While      : Text="While";       break;
    case SentenceId::EndWhile   : Text="EndWhile";    break;
    case SentenceId::Do         : Text="Do";          break;
    case SentenceId::Loop       : Text="Loop";        break;
    case SentenceId::For        : Text="For";         break;
    case SentenceId::EndFor     : Text="EndFor";      break;
    case SentenceId::Walk       : Text="Walk";        break;
    case SentenceId::EndWalk    : Text="EndWalk";     break;
    case SentenceId::Switch     : Text="Switch";      break;
    case SentenceId::When       : Text="When";        break;
    case SentenceId::Default    : Text="Default";     break;
    case SentenceId::EndSwitch  : Text="EndSwitch";   break;
    case SentenceId::Break      : Text="Break";       break;
    case SentenceId::Continue   : Text="Continue";    break;
    case SentenceId::Expression : Text="Expression";  break;
    case SentenceId::SystemCall : Text="SystemCall";  break;
    case SentenceId::SystemFunc : Text="SystemFunc";  break;
    case SentenceId::DlFunction : Text="DlFunction";  break;
    case SentenceId::DlType     : Text="DlType";      break;
    case SentenceId::XlvSet     : Text="XlvSet";      break;
    case SentenceId::InitVar    : Text="InitVar";     break;
    case SentenceId::Empty      : Text="Empty";       break;
  }
  return Text;
}

//Jump mode text
String _JumpModeText(JumpMode JMode){
  String Text;
  switch(JMode){
    case JumpMode::None     : Text="None";      break;
    case JumpMode::BlockBeg : Text="BlockBeg";  break;
    case JumpMode::BlockEnd : Text="BlockEnd";  break;
    case JumpMode::LoopBeg  : Text="LoopBeg";   break;
    case JumpMode::LoopEnd  : Text="LoopEnd";   break;
    case JumpMode::FirstCase: Text="FirstCase"; break;
    case JumpMode::NextCase : Text="NextCase";  break;
    case JumpMode::LastCase : Text="LastCase";  break;
    case JumpMode::EndCase  : Text="EndCase";   break;
  }
  return Text;
}

//Parser get code label from id, baselabel and sublabel
String _ParserGetCodeLabel(CodeLabelId LabelId,long BaseLabel,long SubLabel,const String& BlockId,long LoopLabel,const String& LoopId){
  String Label;
  switch(LabelId){
    case CodeLabelId::NextBlock: Label=ToString(BaseLabel).RJust(_CodeLabelLen,'0')+BlockId+"-next"; break;
    case CodeLabelId::LoopBeg  : Label=ToString(BaseLabel).RJust(_CodeLabelLen,'0')+BlockId+"-beg"; break;
    case CodeLabelId::LoopEnd  : Label=ToString(BaseLabel).RJust(_CodeLabelLen,'0')+BlockId+"-end"; break;
    case CodeLabelId::LoopExit : Label=ToString(LoopLabel).RJust(_CodeLabelLen,'0')+LoopId +"-exit"; break;
    case CodeLabelId::LoopNext : Label=ToString(LoopLabel).RJust(_CodeLabelLen,'0')+LoopId +"-end"; break;
    case CodeLabelId::CurrCond : Label=ToString(BaseLabel).RJust(_CodeLabelLen,'0')+BlockId+"-cond"+ToString(SubLabel); break;
    case CodeLabelId::PrevCond : Label=ToString(BaseLabel).RJust(_CodeLabelLen,'0')+BlockId+"-cond"+ToString(SubLabel-1); break;
    case CodeLabelId::NextCond : Label=ToString(BaseLabel).RJust(_CodeLabelLen,'0')+BlockId+"-cond"+ToString(SubLabel+1); break;
    case CodeLabelId::Exit     : Label=ToString(BaseLabel).RJust(_CodeLabelLen,'0')+BlockId+"-exit"; break;
  }
  return Label;
}

//Code block action Text
String _CodeBlockActionText(CodeBlockAction Action){
  String Text="";
  switch(Action){
    case CodeBlockAction::Push   : Text="Push";    break;
    case CodeBlockAction::Pop    : Text="Pop";     break;
    case CodeBlockAction::Keep   : Text="Keep";    break;
    case CodeBlockAction::Replace: Text="Replace"; break;
  }
  return Text;
} 
