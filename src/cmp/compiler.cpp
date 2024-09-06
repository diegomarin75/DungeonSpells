//Compiler.cpp: Compiler class
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
#include "cmp/exprcomp.hpp"
#include "cmp/compiler.hpp"

//Init routine mode
enum class InitRoutineMode{
  None,
  Public,
  Private
};

//Configuration copy constructor
CompilerConfig::CompilerConfig(const CompilerConfig& Config){
  _Move(Config);
}

//Configuration copy
CompilerConfig& CompilerConfig::operator=(const CompilerConfig& Config){
  _Move(Config);
  return *this;
}

//Compiler configuration move
void CompilerConfig::_Move(const CompilerConfig& Config){
  MaxErrorNr=Config.MaxErrorNr;
  MaxWarningNr=Config.MaxWarningNr;
  PassOnWarnings=Config.PassOnWarnings;
  EnableAsmFile=Config.EnableAsmFile;
  DebugSymbols=Config.DebugSymbols;
  CompilerStats=Config.CompilerStats;
  LinterMode=Config.LinterMode;
  CompileToApp=Config.CompileToApp;
}

//Compiler constructor
Compiler::Compiler(){
  _Config=CompilerConfig();
  _CmplxLitValNr=0;
  _LibMajorVers=0;
  _LibMinorVers=0;
  _LibRevisionNr=0;
}

//Set compiler configuration
void Compiler::SetConfig(const CompilerConfig& Config){
  _Config=Config;
}

//Delayed initialization routine start
void Compiler::_DelayedInitStart(){
  
  //Variables
  String FunName=_Md->GetDelayedInitFunctionName(_Md->Modules[_Md->CurrentScope().ModIndex].Name);

  //Add delayed initialization routine to init routine list
  _Md->Bin.AppendInitRoutine(0,_Md->Modules[_Md->CurrentScope().ModIndex].Name,FunName,_Md->CurrentScope().Depth);

  //Declare delayed init function
  _Md->StoreRegularFunction(_Md->CurrentScope(),FunName,-1,true,false,SourceInfo(),"");
  _Md->StoreFunctionSearchIndex(_Md->Functions.Length()-1);
  _Md->StoreFunctionId(_Md->Functions.Length()-1);

  //Add delayed function header to addition buffer
  //(here we use AtTop=true to ensure that declaration of delayed init routine happens at beginning of addition buffer
  //because when we have static fields in classes with initialization this is emitted into addition buffer before this declaration)
  _PsStack.Top().Add("func void "+FunName+"():",true);

  //Set delayed init not ended flag
  _DelayedInitEnded=false;

  //Return
  return;

}

//Delayed initialization routine end
void Compiler::_DelayedInitEnd(){
  
  //Add delayed function end to addition buffer
  _PsStack.Top().Add(":func");

  //Set delayed init was ended
  _DelayedInitEnded=true;

  //Return
  return;

}

//Compile function
bool Compiler::Compile(const String& SourceFile,const String& OutputFile,const String& IncludePath,const String& LibraryPath,const String& DynLibPath,bool& CompileToLibrary){

  //Variables
  int i;
  bool Error;
  bool EndOfSource;
  bool AlreadyLoaded;
  int Start;
  int End;
  String Module;
  String BinaryFile;
  MasterData Md;
  Sentence Stn;
  Sentence LastStn;
  Array<String> InsertionLines;
  Stack<Sentence> ForStep;
  Stack<Sentence> SwitchExpr;
  Stack<ExprToken> WalkArray;
  Expression Expr;
  String AssemblerFile;
  String DefaultPath;
  Sentence SubStn;
  int DefinedTypes;
  int SupTypIndex;
  int FunIndex;
  int MainFunIndex;
  int ClosedFunIndex;
  int LineCnt[PARSER_BUFFERS];
  ClockPoint CompStart;
  ClockPoint CompEnd;
  ClockPoint StnStart;
  int LineDiscount;
  double TimeDiscount;
  ScopeDef CurrScope;
  SubScopeDef CurrSubScope;
  CpuAdr PrevCodeAddress;
  CpuInt PrevModSymIndex;
  int PrevLineNr;

  //Init variables
  DefinedTypes=0;
  AlreadyLoaded=false;
  CompileToLibrary=false;
  DefaultPath=_Stl->FileSystem.GetDirName(SourceFile);
  for(i=0;i<PARSER_BUFFERS;i++){ LineCnt[i]=0; }

  //Init parser
  _PsStack=Stack<Parser>(_Config.ParserStackDefaultSize);
  _PsStack.Push(Parser());
  if(!_PsStack.Top().Open(SourceFile,_Config.LinterMode?true:false)){ return false; }

  //Find library option in module
  if(_PsStack.Top().LibraryOptionFound()){
    CompileToLibrary=true;
    BinaryFile=OutputFile+LIBRARY_EXT;
    DebugMessage(DebugLevel::CmpInit,"Compiling to library, output file: "+BinaryFile);
  }
  else{
    BinaryFile=OutputFile+EXECUTABLE_EXT;
    DebugMessage(DebugLevel::CmpInit,"Compiling to executable, output file: "+BinaryFile);
  }

  //Init master data & binary module
  _Md=&Md;
  _Md->InitializeVars=false;
  _Md->CompileToLibrary=CompileToLibrary;
  _Md->DebugSymbols=_Config.DebugSymbols;
  _Md->DynLibPath=DynLibPath;
  _Md->InitScopeStack();
  _Md->StoreModule(_Stl->FileSystem.GetFileNameNoExt(SourceFile),SourceFile,CompileToLibrary,SourceInfo());
  _Md->StoreTracker(_Stl->FileSystem.GetFileNameNoExt(SourceFile),_Md->Modules.Length()-1);
  _Md->SetMainDefinitions();
  _Md->InitLabelGenerator();
  _Md->InitFlowLabelGenerator();
  _Md->InitRoutineReset();
  _Md->Bin.SetLitValueReplacementsGlobal(true);
  _Md->Bin.SetBinaryFile(BinaryFile);
  _Md->Bin.SetCompileToLibrary(CompileToLibrary);
  _Md->Bin.SetSource(SourceInfo(_Md->CurrentModule(),0,-1));
  _Md->Bin.EnableAssemblerFile(_Config.EnableAsmFile);
  _Md->Bin.SetLibraryVersion(_LibMajorVers,_LibMinorVers,_LibRevisionNr);
  CurrScope=_Md->CurrentScope();
  CurrSubScope=_Md->CurrentSubScope();

  //Init assembler file
  if(!_Md->Bin.OpenAssembler(_Stl->FileSystem.GetDirName(BinaryFile)+_Stl->FileSystem.GetFileNameNoExt(BinaryFile)+".asm")){ return false; }

  //Assembler file header
  _Md->Bin.AsmOutSeparator(AsmSection::Head);
  _Md->Bin.AsmOutCommentLine(AsmSection::Head,MASTER_NAME " "+ToString(GetArchitecture())+"bit (v"+String(VERSION_NUMBER)+") assembler file",false);
  _Md->Bin.AsmOutCommentLine(AsmSection::Head,"Source file.....: "+SourceFile,false);
  _Md->Bin.AsmOutCommentLine(AsmSection::Head,"Output file.....: "+BinaryFile,false);
  _Md->Bin.AsmOutCommentLine(AsmSection::Head,"Compiler output.: "+String(CompileToLibrary?"Library file":"Executable file"),false);
  _Md->Bin.AsmOutSeparator(AsmSection::Head);
  _Md->Bin.AsmOutNewLine(AsmSection::Star);
  _Md->Bin.AsmOutSeparator(AsmSection::Star);
  _Md->Bin.AsmOutCommentLine(AsmSection::Star,(!CompileToLibrary?"Start code & ":"")+String("Initializations"),false);
  _Md->Bin.AsmOutSeparator(AsmSection::Star);

  //Emit start code for executable files or emit NOP for libraries to avoid any function having address 0 (as FwdFunctionCall mecanism thinks they are unresolved calls)
  if(!CompileToLibrary){ 
    if(!_Md->GenerateStartCode()){ return false; }
  }
  else{
    if(!_Md->Bin.AsmWriteCode(CpuInstCode::NOP,AsmSection::Star)){ return false; }    
  }

  //Compiler start time
  CompStart=ClockGet();
  LineDiscount=0;
  TimeDiscount=0;

  //Parser loop
  Error=false;
  SysMsgDispatcher::SetMaximunErrorCount(_Config.MaxErrorNr);
  SysMsgDispatcher::SetMaximunWarningCount(_Config.MaxWarningNr);
  do{

    //Check error/warning count and exit compiler loop (checked on loop beginning because of error conditions that end with continue)
    if(Error || SysMsgDispatcher::GetWarningCount()>=_Config.MaxWarningNr){
      if(SysMsgDispatcher::GetErrorCount()>=_Config.MaxErrorNr){
        SysMsgDispatcher::SetForceOutput(true);
        SysMessage(23).Print(ToString(_Config.MaxErrorNr));
        SysMsgDispatcher::SetForceOutput(false);
        break;
      }
      else if(SysMsgDispatcher::GetWarningCount()>=_Config.MaxWarningNr){
        SysMsgDispatcher::SetForceOutput(true);
        SysMessage(391).Print(ToString(_Config.MaxWarningNr));
        SysMsgDispatcher::SetForceOutput(false);
        break;
      }
    }

    //Init error flag
    Error=false;
    
    //Define type identifiers for parser
    if(DefinedTypes!=_Md->Types.Length() || CurrScope!=_Md->CurrentScope() || CurrSubScope!=_Md->CurrentSubScope()){
      _PsStack.Top().SetTypeIds(_Md->GetTypeIdentifiers(_Md->CurrentScope()));
      DefinedTypes=_Md->Types.Length();
      CurrScope=_Md->CurrentScope();
      CurrSubScope=_Md->CurrentSubScope();
    }
    
    //Get source line
    if(!_PsStack.Top().Get(Stn,_SourceLine,EndOfSource)){ _PsStack.Top().StateBack(); Error=true; continue; }

    //Count source lines
    if(_Config.CompilerStats){ LineCnt[(int)Stn.Origin()]++; }

    //Emit source line in assembler file
    _EmitSourceLine(Stn,_SourceLine);

    //Set file name and line number for binary class
    _Md->Bin.SetSource(SourceInfo(Stn.FileName(),Stn.LineNr(),(Stn.Tokens.Length()!=0?Stn.Tokens[0].ColNr:-1)));

    //Record debug symbols for source code line numbers
    if(_Md->CurrentScope().ModIndex!=-1 && (Stn.Origin()==OrigBuffer::Source || Stn.Origin()==OrigBuffer::Split)){
      PrevCodeAddress=_Md->Bin.CurrentCodeAddress();
      PrevModSymIndex=_Md->Modules[_Md->CurrentScope().ModIndex].DbgSymIndex;
      PrevLineNr=Stn.LineNr();
    }
    else{
      PrevCodeAddress=0;
    }

    //Decide on sentence type
    switch(Stn.Id){
      
      //.module
      case SentenceId::Libs:
        if(!_CompileLibs(Stn)){ Error=true; break; }
        break;

      //.public
      //(Does not open new scope as it is open when entering modules)
      case SentenceId::Public:
        if(!_CompilePublic(Stn)){ Error=true; break; }
        _Md->InitRoutineStart();
        break;

      //.private
      //(Defines new scope)
      case SentenceId::Private:
        if(!_CompilePrivate(Stn)){ Error=true; break; }
        if(!_Md->IsInitRoutineStarted()){ _Md->InitRoutineStart(); }
        if(!_Md->OpenScope(ScopeKind::Private,_Md->CurrentScope().ModIndex,-1,Stn.GetCodeBlockId())){ Error=true; break; }
        break;

      //.implem
      //(Opens module private scope in case it is not open already, because static variables are created in this scope)
      //(We call init routine start in case we have a source without public or private sections, it does nothing if init routine was started already)
      case SentenceId::Implem:
        if(!_CompileImplem(Stn)){ Error=true; break; }
        if(_Md->CurrentScope().Kind!=ScopeKind::Private){
          if(!_Md->OpenScope(ScopeKind::Private,_Md->CurrentScope().ModIndex,-1,Stn.GetCodeBlockId())){ Error=true; break; }
        }
        if(!_Md->IsInitRoutineStarted()){ _Md->InitRoutineStart(); }
        if(!_Md->InitRoutineEnd()){ Error=true; break; }
        _DelayedInitStart();
        if(!_Md->GenerateSuperInitRoutine()){ Error=true; break; }
        _Md->Bin.AsmFlush();
        _Md->Bin.SetLitValueReplacementsGlobal(false);
        break;

      //Set
      case SentenceId::Set:
        if(!_CompileSet(Stn,CompileToLibrary)){ Error=true; break; }
        break;

      //Import
      //This sentence is not counted on statistics as it produces distortion (disk access is much slower)
      case SentenceId::Import:
        StnStart=ClockGet();
        if(!_CompileImport(Stn,LibraryPath,DefaultPath)){ Error=true; break; }
        TimeDiscount+=ClockIntervalSec(ClockGet(),StnStart);
        LineDiscount++;
        break;

      //Include
      //(Defines new module public scope)
      //This sentence is not counted on statistics as it produces distortion (disk access is much slower)
      case SentenceId::Include:
        StnStart=ClockGet();
        if(!_CompileInclude(Stn,AlreadyLoaded,IncludePath,DefaultPath)){ Error=true; break; }
        if(!AlreadyLoaded){
          if(!_Md->OpenScope(ScopeKind::Public,_Md->Modules.Length()-1,-1,Stn.GetCodeBlockId())){ Error=true; break; }
          _PsStack.Push(Parser());
          if(!_PsStack.Top().Open(_Md->Modules[_Md->CurrentScope().ModIndex].Path,false)){ _Md->DeleteScope(); _PsStack.Pop(); Error=true; break; }
        }
        TimeDiscount+=ClockIntervalSec(ClockGet(),StnStart);
        LineDiscount++;
        break;

      //Const
      case SentenceId::Const:
        if(!_CompileConst(Stn)){ Error=true; break; }
        break;

      //Var
      case SentenceId::VarDecl:
        if(_Md->CurrentSubScope().Kind==SubScopeKind::Public || _Md->CurrentSubScope().Kind==SubScopeKind::Private){
          if(!_CompileClassField(Stn)){ Error=true; break; }
        }
        else{
          if(!_CompileVar(Stn)){ Error=true; break; }
        }
        break;

      //DefType
      case SentenceId::DefType:
        if(!_CompileDeftype(Stn)){ Error=true; break; }
        break;

      //DlType
      case SentenceId::DlType:
        if(!_CompileDlType(Stn)){ Error=true; break; }
        break;

      //DefClass
      case SentenceId::DefClass:
        if(!_CompileDefClass(Stn)){ Error=true; break; }
        if(!_Md->SubScopeBegin(SubScopeKind::Public,_Md->Types.Length()-1)){ Error=true; break; }
        break;

      //Publ
      case SentenceId::Publ:
        if(!_CompilePubl(Stn)){ Error=true; break; }
        break;

      //Priv
      case SentenceId::Priv:
        if(!_CompilePriv(Stn)){ Error=true; break; }
        SupTypIndex=_Md->CurrentSubScope().TypIndex;
        if(!_Md->SubScopeEnd()){ Error=true; break; }
        if(!_Md->SubScopeBegin(SubScopeKind::Private,SupTypIndex)){ Error=true; break; }
        break;

      //EndClass
      case SentenceId::EndClass:
        if(!_CompileEndClass(Stn)){ Error=true; break; }
        if(!_Md->SubScopeEnd()){ Error=true; break; }
        break;

      //Grant
      case SentenceId::Allow:
        if(!_CompileAllow(Stn)){ Error=true; break; }
        break;

      //DefEnum
      case SentenceId::DefEnum:
        if(!_CompileDefEnum(Stn)){ Error=true; break; }
        if(!_Md->SubScopeBegin(SubScopeKind::Public,_Md->Types.Length()-1)){ Error=true; break; }
        break;

      //EnumField
      case SentenceId::EnumField:
        if(!_CompileEnumField(Stn)){ Error=true; break; }
        break;

      //EndEnum
      case SentenceId::EndEnum:
        if(!_CompileEndEnum(Stn)){ Error=true; break; }
        if(!_Md->SubScopeEnd()){ Error=true; break; }
        break;

      //SystemCall
      case SentenceId::SystemCall:
        if(!_CompileFuncDecl(Stn,FunIndex)){ Error=true; break; }
        break;

      //SystemFunc
      case SentenceId::SystemFunc:
        if(!_CompileFuncDecl(Stn,FunIndex)){ Error=true; break; }
        break;

      //Dynamic library function declaration
      case SentenceId::DlFunction:
        if(!_CompileFuncDecl(Stn,FunIndex)){ Error=true; break; }
        break;

      //Functon declaration
      case SentenceId::FunDecl:
        if(!_CompileFuncDecl(Stn,FunIndex)){ Error=true; break; }
        break;

      //Main
      case SentenceId::Main:
        _EmitFunctionSourceLine(_SourceLine);
        if(!_CompileMain(Stn,_Md->GetMainFunctionName(),CompileToLibrary)){ Error=true; break; }
        if(!_Md->OpenScope(ScopeKind::Local,_Md->CurrentScope().ModIndex,_Md->Functions.Length()-1,Stn.GetCodeBlockId())){ Error=true; break; }
        break;

      //EndMain
      case SentenceId::EndMain:
        ClosedFunIndex=_Md->CurrentScope().FunIndex;
        if(!_CompileEndMain(Stn,_Md->GetMainFunctionName(),LastStn.Id)){ Error=true; break; }
        if(!_Md->CloseScope(ScopeKind::Local,CompileToLibrary,Stn)){ Error=true; break; }
        if(ClosedFunIndex!=-1){ _Md->AsmWriteFunction(ClosedFunIndex); }
        _Md->Bin.AsmFlush();
        break;

      //Function
      case SentenceId::Function:
        if(_Md->CurrentScope().Kind==ScopeKind::Local && Stn.Let){
          _Md->Bin.AsmOpenNestId();
          _EmitFunctionSourceLine(_SourceLine);
          SubStn=Stn.SubSentence(1,Stn.Tokens.Length()-2);
          if(!_CompileFuncDecl(SubStn,FunIndex)){ Error=true; break; }
        }
        else{
          _EmitFunctionSourceLine(_SourceLine);
        }
        if(!_CompileFuncDefn(Stn,FunIndex)){ Error=true; break; }
        if(!_Md->OpenScope(ScopeKind::Local,_Md->CurrentScope().ModIndex,FunIndex,Stn.GetCodeBlockId())){ Error=true; break; }
        break;

      //EndFunction
      case SentenceId::EndFunction:
        ClosedFunIndex=_Md->CurrentScope().FunIndex;
        if(!_CompileEndFunction(Stn,LastStn.Id)){ Error=true; break; }
        if(!_Md->CloseScope(ScopeKind::Local,CompileToLibrary,Stn)){ Error=true; break; }
        if(ClosedFunIndex!=-1 && _Md->CurrentScope().Kind!=ScopeKind::Local){ _Md->AsmWriteFunction(ClosedFunIndex); }
        if(_Md->CurrentScope().Kind==ScopeKind::Local){ _Md->Bin.AsmCloseNestId(); }
        if(!_Md->Functions[ClosedFunIndex].IsNested){ _Md->Bin.AsmFlush(); _Md->Bin.AsmNestIdCounter(); }
        break;

      //Member
      case SentenceId::Member:
        _EmitFunctionSourceLine(_SourceLine);
        if(!_CompileFuncDefn(Stn,FunIndex)){ Error=true; break; }
        if(!_Md->OpenScope(ScopeKind::Local,_Md->CurrentScope().ModIndex,FunIndex,Stn.GetCodeBlockId())){ Error=true; break; }
        break;

      //EndMember
      case SentenceId::EndMember:
        ClosedFunIndex=_Md->CurrentScope().FunIndex;
        if(!_CompileEndMember(Stn,LastStn.Id)){ Error=true; break; }
        if(!_Md->CloseScope(ScopeKind::Local,CompileToLibrary,Stn)){ Error=true; break; }
        if(ClosedFunIndex!=-1 && _Md->CurrentScope().Kind!=ScopeKind::Local){ _Md->AsmWriteFunction(ClosedFunIndex); }
        if(_Md->CurrentScope().Kind==ScopeKind::Local){ _Md->Bin.AsmCloseNestId(); }
        if(!_Md->Functions[ClosedFunIndex].IsNested){ _Md->Bin.AsmFlush(); _Md->Bin.AsmNestIdCounter(); }
        break;

      //Operator
      case SentenceId::Operator:
        if(_Md->CurrentScope().Kind==ScopeKind::Local && Stn.Let){
          _Md->Bin.AsmOpenNestId();
          _EmitFunctionSourceLine(_SourceLine);
          SubStn=Stn.SubSentence(1,Stn.Tokens.Length()-2);
          if(!_CompileFuncDecl(SubStn,FunIndex)){ Error=true; break; }
        }
        else{
          _EmitFunctionSourceLine(_SourceLine);
        }
        if(!_CompileFuncDefn(Stn,FunIndex)){ Error=true; break; }
        if(!_Md->OpenScope(ScopeKind::Local,_Md->CurrentScope().ModIndex,FunIndex,Stn.GetCodeBlockId())){ Error=true; break; }
        break;

      //EndOperator
      case SentenceId::EndOperator:
        ClosedFunIndex=_Md->CurrentScope().FunIndex;
        if(!_CompileEndOperator(Stn,LastStn.Id)){ Error=true; break; }
        if(!_Md->CloseScope(ScopeKind::Local,CompileToLibrary,Stn)){ Error=true; break; }
        if(ClosedFunIndex!=-1 && _Md->CurrentScope().Kind!=ScopeKind::Local){ _Md->AsmWriteFunction(ClosedFunIndex); }
        if(_Md->CurrentScope().Kind==ScopeKind::Local){ _Md->Bin.AsmCloseNestId(); }
        if(!_Md->Functions[ClosedFunIndex].IsNested){ _Md->Bin.AsmFlush(); _Md->Bin.AsmNestIdCounter(); }
        break;

      //Return
      case SentenceId::Return:
        if(!_CompileReturn(Stn,_Md->GetMainFunctionName())){ Error=true; break; }
        break;

      //If
      case SentenceId::If:
        if(!_CompileIf(Stn)){ Error=true; break; }
        break;

      //ElseIf
      case SentenceId::ElseIf:
        if(!_CompileElseIf(Stn)){ Error=true; break; }
        break;

      //Else
      case SentenceId::Else:
        if(!_CompileElse(Stn)){ Error=true; break; }
        break;

      //EndIf
      case SentenceId::EndIf:
        if(!_CompileEndIf(Stn)){ Error=true; break; }
        break;

      //While
      case SentenceId::While:
        if(!_CompileWhile(Stn)){ Error=true; break; }
        break;

      //EndWhile
      case SentenceId::EndWhile:
        if(!_CompileEndWhile(Stn)){ Error=true; break; }
        break;

      //Do
      case SentenceId::Do:
        if(!_CompileDo(Stn)){ Error=true; break; }
        break;

      //Loop
      case SentenceId::Loop:
        if(!_CompileLoop(Stn)){ Error=true; break; }
        break;

      //For
      case SentenceId::For:
        if(!_CompileFor(Stn,ForStep)){ Error=true; break; }
        break;

      //EndFor
      case SentenceId::EndFor:
        if(!_CompileEndFor(Stn,ForStep)){ Error=true; break; }
        break;

      //Walk
      case SentenceId::Walk:
        if(!_CompileWalk(Stn,WalkArray)){ Error=true; break; }
        break;

      //EndWalk
      case SentenceId::EndWalk:
        if(!_CompileEndWalk(Stn,WalkArray)){ Error=true; break; }
        break;

      //Switch
      case SentenceId::Switch:
        if(!_CompileSwitch(Stn,SwitchExpr)){ Error=true; break; }
        break;

      //When
      case SentenceId::When:
        if(_PsStack.Top().CurrentBlock()==CodeBlock::FirstWhen){
          if(!_CompileWhen(Stn,SwitchExpr,true)){ Error=true; break; }
        }
        else{
          if(!_CompileWhen(Stn,SwitchExpr,false)){ Error=true; break; }
        }          
        break;

      //Default
      case SentenceId::Default:
        if(!_CompileDefault(Stn,SwitchExpr)){ Error=true; break; }
        break;

      //EndSwitch
      case SentenceId::EndSwitch:
        if(!_CompileEndSwitch(Stn,SwitchExpr)){ Error=true; break; }
        break;

      //Break
      case SentenceId::Break:
        if(_PsStack.Top().CurrentBlock()==CodeBlock::FirstWhen 
        || _PsStack.Top().CurrentBlock()==CodeBlock::NextWhen 
        || _PsStack.Top().CurrentBlock()==CodeBlock::Default){
          if(!_CompileBreak(Stn,true,Stn.InsideLoop())){ Error=true; break; }
        }
        else{
          if(!_CompileBreak(Stn,false,Stn.InsideLoop())){ Error=true; break; }
        }
        break;

      //Continue
      case SentenceId::Continue:
        if(!_CompileContinue(Stn,Stn.InsideLoop())){ Error=true; break; }
        break;

      //Expression (when comming from complex literal value substitutions in global scope)
      case SentenceId::XlvSet:
        if(!Stn.Get(PrKeyword::XlvSet).ReadEx(Start,End).Ok()){ Error=true; break; }
        if(!Expr.Compile(_Md,_Md->CurrentScope(),Stn,Start,End)){ Error=true; break; }
        break;

      //Initialize variable (for initialization of static variables in delayed routine)
      case SentenceId::InitVar:
        if(!_CompileInitVar(Stn)){ Error=true; break; }
        break;

      //Expression
      case SentenceId::Expression:
        if(!Stn.ReadEx(Start,End).Ok()){ Error=true; break; }
        if(!Expr.Compile(_Md,_Md->CurrentScope(),Stn,Start,End)){ Error=true; break; }
        break;

      //Empty (nothing to do)
      case SentenceId::Empty:
        break;

    }

    //Store debug symbol source line number
    if(PrevCodeAddress!=0 && PrevCodeAddress!=_Md->Bin.CurrentCodeAddress() 
    && Stn.Id!=SentenceId::Libs && Stn.Id!=SentenceId::Public && Stn.Id!=SentenceId::Private && Stn.Id!=SentenceId::Implem && Stn.Id!=SentenceId::Import){
      _Md->Bin.StoreDbgSymLine(PrevModSymIndex,PrevCodeAddress,_Md->Bin.CurrentCodeAddress()-1,PrevLineNr);
    }

    //Check there are delayed error messages
    if(SysMessage().DelayCount()!=0){
      SysMessage().FlushDelayedMessages(Stn.FileName(),Stn.LineNr());
    }

    //Check sentence does not have more tokens to process
    if(!Error && Stn.TokensLeft()!=0){
      SysMessage(30,Stn.FileName(),Stn.LineNr(),Stn.CurrentColNr()).Print(SentenceIdText(Stn.Id));
    }

    //Hide local variables on closed blocks
    if(_PsStack.Top().GetClosedBlocks().Length()!=0){
      if(_Md->CurrentScope().Kind==ScopeKind::Local){
        _Md->HideLocalVariables(_Md->CurrentScope(),_PsStack.Top().GetClosedBlocks(),-1,Stn.Tokens[0].SrcInfo());
      }
      _PsStack.Top().ClearClosedBlocks();
    }

    //Emit delayed init routine end
    if(Stn.Origin()==OrigBuffer::Addition && !_DelayedInitEnded){ _DelayedInitEnd(); EndOfSource=false; }

    //Finish intialization routine if end of source is reached before implementation block
    //(We call init routine start in case we have a source without public or private sections, it does nothing if init routine was started already)
    if(_Md->CurrentScope().ModIndex==_Md->MainModIndex() && EndOfSource 
    && (_PsStack.Top().CurrentBlock()==CodeBlock::Public || _PsStack.Top().CurrentBlock()==CodeBlock::Private)){
      if(!_Md->IsInitRoutineStarted()){ _Md->InitRoutineStart(); }
      _Md->InitRoutineEnd();
      _Md->GenerateSuperInitRoutine();
    }

    //End of source file detected
    if(EndOfSource){
      
      //Check class/enum definition is not left open
      if(_Md->CurrentSubScope().Kind!=SubScopeKind::None){
        SysMessage(136,Stn.FileName(),Stn.LineNr()).Print(_Md->CannonicalTypeName(_Md->CurrentSubScope().TypIndex)) ;
      }

      //Check local scope is not left open
      if(_Md->CurrentScope().Kind==ScopeKind::Local){
        SysMessage(137,Stn.FileName(),Stn.LineNr()).Print(_Md->Functions[_Md->CurrentScope().FunIndex].Name);
      }
      
      //Terminate compilation on end of source / Pop parser stack on end of module
      if(_PsStack.Length()==1){
        break;
      }
      else{
        _PsStack.Top().Reset();
        _PsStack.Pop();
        if(_Md->CurrentScope().Kind==ScopeKind::Private){
          _Md->CloseScope(ScopeKind::Private,CompileToLibrary,Stn);
        }
        _Md->CloseScope(ScopeKind::Public,CompileToLibrary,Stn);
        _Md->Bin.AsmFlush();
        _Md->InitRoutineReset();
      }

    } 

    //Save last sentence
    if(!Error){ if(Stn.Id!=SentenceId::Empty){ LastStn=Stn; } }

    //Restore parser state on error
    if(Error){ _PsStack.Top().StateBack(); }

  }while(true);

  //Get compiler end time
  CompEnd=ClockGet();
  
  //Keep error flag
  if(SysMsgDispatcher::GetErrorCount()!=0){ Error=true; } else{ Error=false; }

  //Check program has entry point
  if(!Error){
    if(!CompileToLibrary){
      if((MainFunIndex=_Md->FunSearch(_Md->GetMainFunctionName(),_Md->MainModIndex(),"","",nullptr))==-1){
        SysMessage(21,SourceFile,(LastStn.LineNr()==0?1:LastStn.LineNr())).Print();
        return false;
      }
    }
  }
  
  //Close main module scopes
  if(!Error){
    if(_Md->CurrentScope().Kind==ScopeKind::Private){
      if(!_Md->CloseScope(ScopeKind::Private,CompileToLibrary,Stn)){ Error=true; }
    }
    if(!Error){
      if(!_Md->CloseScope(ScopeKind::Public,CompileToLibrary,Stn)){ Error=true; }
    }
  }

  //Generate output files
  if(!Error && !_Config.LinterMode){
    if(SysMsgDispatcher::GetWarningCount()==0 || _Config.PassOnWarnings){
      if(CompileToLibrary){
        _Md->Bin.SetLibraryVersion(_LibMajorVers,_LibMinorVers,_LibRevisionNr);
        if(!_Md->Bin.GenerateLibrary(_Config.DebugSymbols)){ Error=true; }
      }
      else{
        if(!_Md->Bin.GenerateExecutable(_Config.DebugSymbols)){ Error=true; }
      }
    }
    else{
      SysMessage(393).Print();
    }
  }
  
  //Assembler file flush
  if(!_Md->Bin.AsmFlush()){ Error=true; }
       
  //Close assembler file
  if(!_Md->Bin.CloseAssembler()){ Error=true; }


  //Output compiler statistcs
  if(_Config.CompilerStats){ 
    int TotLines=LineCnt[(int)OrigBuffer::Source]+LineCnt[(int)OrigBuffer::Split]+LineCnt[(int)OrigBuffer::Insertion]+LineCnt[(int)OrigBuffer::Addition];
    double CompSec=ClockIntervalSec(CompEnd,CompStart);
    double TrueSpeed=(TotLines-LineDiscount>0 && CompSec-TimeDiscount>0?(TotLines-LineDiscount)/(CompSec-TimeDiscount):0);
    _Stl->Console.PrintLine("Source lines.....: "+ToString(LineCnt[(int)OrigBuffer::Source])); 
    _Stl->Console.PrintLine("Source sentences.: "+ToString(LineCnt[(int)OrigBuffer::Source]+LineCnt[(int)OrigBuffer::Split])); 
    _Stl->Console.PrintLine("Inserted lines...: "+ToString(LineCnt[(int)OrigBuffer::Insertion])); 
    _Stl->Console.PrintLine("Appended lines...: "+ToString(LineCnt[(int)OrigBuffer::Addition])); 
    _Stl->Console.PrintLine("Total lines......: "+ToString(TotLines)); 
    _Stl->Console.PrintLine("Time elapsed.....: "+ToString(CompSec,"%0.2f")+" s"); 
    _Stl->Console.PrintLine("Compiler speed...: "+ToString(TrueSpeed,"%0.2f")+" lines/s"); 
    _Stl->Console.PrintLine("Disc. lines/time.: "+ToString(LineDiscount)+" lines / "+ToString(TimeDiscount,"%0.2f")+" s"); 
  }
  
  //Return code
  if(Error || (SysMsgDispatcher::GetWarningCount()!=0 && _Config.PassOnWarnings==false)){ 
    return false; 
  }
  else{
    return true;
  }

}

//Complex litteral value name
String Compiler::_GetNewCmplxLitValName(const String& TypeName){
  String Name=_Md->GetComplexLitValuePrefix()+TypeName.Replace("[","").Replace(",","x").Replace("]","_")+ToString(_CmplxLitValNr);
  _CmplxLitValNr++;
  return Name;
}

//Check if sentence contains operator function definition
int Compiler::_FindOperatorFunction(const Sentence& Stn) const {
  
  //Variables
  int i;
  int FoundIndex;
  
  //Search [ operator ] in sentence
  FoundIndex=-1;
  for(i=0;i<Stn.Tokens.Length();i++){
    if(i+2<=Stn.Tokens.Length()-1
    && Stn.Tokens[i+0].Id()==PrTokenId::Punctuator 
    && Stn.Tokens[i+0].Value.Pnc==PrPunctuator::BegBracket
    && Stn.Tokens[i+1].Id()==PrTokenId::Operator
    && Stn.Tokens[i+2].Id()==PrTokenId::Punctuator 
    && Stn.Tokens[i+2].Value.Pnc==PrPunctuator::EndBracket){
      FoundIndex=i+1;
      break;
    }
  }
  
  //Return index found
  return FoundIndex;

}


//Check identifier is found in expression
bool Compiler::_FindIdentifier(const Sentence& Stn,int BegToken,int EndToken,const String& VarName) const {

  //Variables
  int i;
  int FoundIndex;

  //Init flags
  FoundIndex=-1;

  //Token loop
  for(i=BegToken;i<=EndToken;i++){
    
    //Identifier is found
    if(Stn.Tokens[i].Id()==PrTokenId::Identifier && Stn.Tokens[i].Value.Idn==VarName){
      
      //Identifier found at first position
      if(i==BegToken){
        FoundIndex=i;
        break;
      }

      //Identifier found at other position and previous token is not a member operator
      else if(i>BegToken && EndToken>BegToken && (Stn.Tokens[i-1].Id()!=PrTokenId::Operator || (Stn.Tokens[i-1].Id()==PrTokenId::Operator && Stn.Tokens[i-1].Value.Opr!=PrOperator::Member))){
        FoundIndex=i;
        break;
      }

    }

  }

  //Send error
  if(FoundIndex!=-1){
    Stn.Tokens[FoundIndex].Msg(469).Print();
    return false;
  }

  //Return value
  return true;

}

//Process Libs sentence
//.Libs
bool Compiler::_CompileLibs(Sentence& Stn){

  //Check syntax
  if(!Stn.Get(PrKeyword::Libs).Ok()){ return false; }

  //Return code
  return true;

}

//Process Public sentence
//.public
bool Compiler::_CompilePublic(Sentence& Stn){

  //Check syntax
  if(!Stn.Get(PrKeyword::Public).Ok()){ return false; }

  //Return code
  return true;

}

//Process Private sentence
//.private
bool Compiler::_CompilePrivate(Sentence& Stn){

  //Check syntax
  if(!Stn.Get(PrKeyword::Private).Ok()){ return false; }

  //Return code
  return true;

}

//Process Implem sentence
//.implementation
bool Compiler::_CompileImplem(Sentence& Stn){

  //Check syntax
  if(!Stn.Get(PrKeyword::Implem).Ok()){ return false; }

  //Return code
  return true;

}

//Process Set sentence
//set option=value
bool Compiler::_CompileSet(Sentence& Stn,bool CompileToLibrary){

  //Variables
  CpuBol ValueBol;
  CpuChr ValueChr;
  CpuShr ValueShr;
  CpuInt ValueInt;
  CpuLon ValueLon;
  CpuLon ConfigValue;
  String ConfigVar;

  //Set sentence only allowed on main module
  if(_Md->CurrentScope().ModIndex!=_Md->MainModIndex()){ 
    Stn.Tokens[0].Msg(218).Print();
    return false; 
  }

  //Parse sentence up to assign symbol
  if(!Stn.Get(PrKeyword::Set).ReadId(ConfigVar).Get(PrOperator::Assign).Ok()){ return false; }

  //Boolean options
  if(ConfigVar=="init_vars"){

    //Get value
    if(Stn.IsBol()){ 
      if(!Stn.ReadBo(ValueBol).Ok()){ return false; }
    }
    else{
      Stn.Msg(142).Print(ConfigVar);
      return false;
    }

    //Init variables option
    if(ConfigVar=="init_vars"){
      _Md->InitializeVars=ValueBol;
    }

  }

  //Numeric options
  else if(ConfigVar=="memory_unit" 
  || ConfigVar=="start_units" 
  || ConfigVar=="chunk_units" 
  || ConfigVar=="block_count"
  || ConfigVar=="major_vers"
  || ConfigVar=="minor_vers"
  || ConfigVar=="revision"){

    //These options cannot happen for libraries
    if(CompileToLibrary && (ConfigVar=="memory_unit" || ConfigVar=="start_units" || ConfigVar=="chunk_units" || ConfigVar=="block_count")){
      Stn.Tokens[1].Msg(402).Print(ConfigVar);
      return false; 
    }

    //These options happen only for libraries
    if(!CompileToLibrary && (ConfigVar=="major_vers" || ConfigVar=="minor_vers" || ConfigVar=="revision")){
      Stn.Tokens[1].Msg(437).Print(ConfigVar);
      return false; 
    }

    //Get value
    if(Stn.IsLon()){ 
      if(!Stn.ReadLo(ValueLon).Ok()){ return false; }
      ConfigValue=ValueLon;
    }
    else if(Stn.IsInt()){ 
      if(!Stn.ReadIn(ValueInt).Ok()){ return false; }
      ConfigValue=ValueInt;
    }
    else if(Stn.IsShr()){ 
      if(!Stn.ReadSh(ValueShr).Ok()){ return false; }
      ConfigValue=ValueShr;
    }
    else if(Stn.IsChr()){ 
      if(!Stn.ReadCh(ValueChr).Ok()){ return false; }
      ConfigValue=ValueChr;
    }
    else{
      Stn.Msg(142).Print(ConfigVar);
      return false;
    }

    //Read memory_unit option
    if(ConfigVar=="memory_unit"){
      if(GetArchitecture()==32 && (ConfigValue<0 || ConfigValue>MAX_INT)){
        Stn.Msg(363).Print();
        return false;
      }
      else if(GetArchitecture()==64 && ConfigValue<0){
        Stn.Msg(363).Print();
        return false;
      }
      _Md->Bin.SetMemoryConfigMemUnitSize((CpuWrd)ConfigValue);
    }

    //Read start_units option
    else if(ConfigVar=="start_units"){
      if(GetArchitecture()==32 && (ConfigValue<0 || ConfigValue>MAX_INT)){
        Stn.Msg(364).Print();
        return false;
      }
      else if(GetArchitecture()==64 && ConfigValue<0){
        Stn.Msg(364).Print();
        return false;
      }
      _Md->Bin.SetMemoryConfigMemUnits((CpuWrd)ConfigValue);
    }

    //Read chunk_units option
    else if(ConfigVar=="chunk_units"){
      if(GetArchitecture()==32 && (ConfigValue<0 || ConfigValue>MAX_INT)){
        Stn.Msg(365).Print();
        return false;
      }
      else if(GetArchitecture()==64 && ConfigValue<0){
        Stn.Msg(365).Print();
        return false;
      }
      _Md->Bin.SetMemoryConfigChunkMemUnits((CpuWrd)ConfigValue);
    }

    //Read bloc_count option
    else if(ConfigVar=="block_count"){
      if(GetArchitecture()==32 && (ConfigValue<0 || ConfigValue>MAX_SHR)){
        Stn.Msg(222).Print();
        return false;
      }
      else if(GetArchitecture()==64 && (ConfigValue<0 || ConfigValue>MAX_INT)){
        Stn.Msg(222).Print();
        return false;
      }
      _Md->Bin.SetMemoryConfigBlockMax((CpuMbl)ConfigValue);
    }

    //Library major version number
    else if(ConfigVar=="major_vers"){
      if(ConfigValue<0 || ConfigValue>MAX_SHR){
        Stn.Msg(438).Print();
        return false;
      }
      _LibMajorVers=(CpuShr)ConfigValue;
    }

    //Library minor version number
    else if(ConfigVar=="minor_vers"){
      if(ConfigValue<0 || ConfigValue>MAX_SHR){
        Stn.Msg(439).Print();
        return false;
      }
      _LibMinorVers=(CpuShr)ConfigValue;
    }

    //Library revision number
    else if(ConfigVar=="revision"){
      if(ConfigValue<0 || ConfigValue>MAX_SHR){
        Stn.Msg(440).Print();
        return false;
      }
      _LibRevisionNr=(CpuShr)ConfigValue;
    }

  }

  //Library option (does nothing only checking, as it is processed before in call to LibraryOptionFound())
  else if(ConfigVar=="library"){
    if(Stn.IsBol()){ 
      if(!Stn.ReadBo(ValueBol).Ok()){ return false; }
    }
    else{
      Stn.Msg(145).Print();
      return false;
    }
    if(_Config.CompileToApp && ValueBol){
      Stn.Tokens[3].Msg(338).Print();
      return false;
    }
  }

  //Unknown configuration variable
  else{
    Stn.Msg(143).Print(ConfigVar);
    return false;
  }

  //Return code
  return true;

}

//Solve undefined references from library
bool Compiler::_SolveLibraryUndefRefs(bool HardLinkage,const SourceInfo& SrcInfo){

  //Variables
  int i;
  int URefModIndex;
  int VarIndex;
  int TypIndex;
  int FunIndex;
  String SearchName;
  String ParmStr;

  //Check & resolve all undefined references
  for(i=0;i<_Md->Bin.IUndRef.Length();i++){

    //Locate module
    if((URefModIndex=_Md->ModSearch(String((char *)_Md->Bin.IUndRef[i].Module)))==-1){
      SrcInfo.Msg(122).Print(String((char *)_Md->Bin.IUndRef[i].Module),_Md->Bin.IUndRef[i].ObjectName); 
      return false;
    }

    //Switch on undefined reference kind
    switch((UndefRefKind)_Md->Bin.IUndRef[i].Kind){
      
      //Global data address
      case UndefRefKind::Glo:
        if(_Md->Bin.IUndRef[i].ObjectName.StartsWith(OBJECTID_MVN":")){
          if((VarIndex=_Md->VarSearch(_Md->Bin.IUndRef[i].ObjectName.CutLeft(strlen(OBJECTID_MVN)+1),URefModIndex))==-1){
            SrcInfo.Msg(279).Print(_Md->Bin.IUndRef[i].ObjectName,String((char *)_Md->Bin.IUndRef[i].Module)); 
            return false;
          }
          DebugMessage(DebugLevel::CmpLibrary,"Undefined reference "+_Md->Bin.IUndRef[i].ObjectName+" from module "+String((char *)_Md->Bin.IUndRef[i].Module)+" at code address "+HEXFORMAT(_Md->Bin.IUndRef[i].CodeAdr)+
          " solved to global variable "+_Md->Modules[_Md->Variables[VarIndex].Scope.ModIndex].Name+"."+_Md->Variables[VarIndex].Name+
          (HardLinkage?" (address="+HEXFORMAT(_Md->Variables[VarIndex].MetaName)+")":""));
          if(HardLinkage){
            if(_Md->Variables[VarIndex].MetaName==0){
              SrcInfo.Msg(283).Print(_Md->Bin.IUndRef[i].ObjectName,String((char *)_Md->Bin.IUndRef[i].Module)); 
              return false;
            }
            _Md->Bin.CodeBufferModify(_Md->Bin.IUndRef[i].CodeAdr,_Md->Variables[VarIndex].MetaName);
          }
        }
        else if(_Md->Bin.IUndRef[i].ObjectName.StartsWith(OBJECTID_MTN":")){
          if((TypIndex=_Md->TypSearch(_Md->Bin.IUndRef[i].ObjectName.CutLeft(strlen(OBJECTID_MTN)+1),URefModIndex))==-1){
            SrcInfo.Msg(291).Print(_Md->Bin.IUndRef[i].ObjectName,String((char *)_Md->Bin.IUndRef[i].Module)); 
            return false;
          }
          DebugMessage(DebugLevel::CmpLibrary,"Undefined reference "+_Md->Bin.IUndRef[i].ObjectName+" from module "+String((char *)_Md->Bin.IUndRef[i].Module)+" at code address "+HEXFORMAT(_Md->Bin.IUndRef[i].CodeAdr)+
          " solved to data type "+_Md->CannonicalTypeName(TypIndex)+
          (HardLinkage?" (address="+HEXFORMAT(_Md->Types[TypIndex].MetaName)+")":""));
          if(HardLinkage){
            if(_Md->Types[TypIndex].MetaName==0){
              SrcInfo.Msg(292).Print(_Md->Bin.IUndRef[i].ObjectName,String((char *)_Md->Bin.IUndRef[i].Module)); 
              return false;
            }
            _Md->Bin.CodeBufferModify(_Md->Bin.IUndRef[i].CodeAdr,_Md->Types[TypIndex].MetaName);
          }
        }
        else if(_Md->Bin.IUndRef[i].ObjectName.StartsWith(OBJECTID_MFN":")){
          if((TypIndex=_Md->TypSearch(_Md->Bin.IUndRef[i].ObjectName.CutLeft(strlen(OBJECTID_MFN)+1),URefModIndex))==-1){
            SrcInfo.Msg(293).Print(_Md->Bin.IUndRef[i].ObjectName,String((char *)_Md->Bin.IUndRef[i].Module)); 
            return false;
          }
          DebugMessage(DebugLevel::CmpLibrary,"Undefined reference "+_Md->Bin.IUndRef[i].ObjectName+" from module "+String((char *)_Md->Bin.IUndRef[i].Module)+" at code address "+HEXFORMAT(_Md->Bin.IUndRef[i].CodeAdr)+
          " solved to data type "+_Md->CannonicalTypeName(TypIndex)+
          (HardLinkage?" (address="+HEXFORMAT(_Md->Types[TypIndex].MetaStNames)+")":""));
          if(HardLinkage){
            if(_Md->Types[TypIndex].MetaStNames==0){
              SrcInfo.Msg(294).Print(_Md->Bin.IUndRef[i].ObjectName,String((char *)_Md->Bin.IUndRef[i].Module)); 
              return false;
            }
            _Md->Bin.CodeBufferModify(_Md->Bin.IUndRef[i].CodeAdr,_Md->Types[TypIndex].MetaStNames);
          }
        }
        else if(_Md->Bin.IUndRef[i].ObjectName.StartsWith(OBJECTID_MFT":")){
          if((TypIndex=_Md->TypSearch(_Md->Bin.IUndRef[i].ObjectName.CutLeft(strlen(OBJECTID_MFT)+1),URefModIndex))==-1){
            SrcInfo.Msg(295).Print(_Md->Bin.IUndRef[i].ObjectName,String((char *)_Md->Bin.IUndRef[i].Module)); 
            return false;
          }
          DebugMessage(DebugLevel::CmpLibrary,"Undefined reference "+_Md->Bin.IUndRef[i].ObjectName+" from module "+String((char *)_Md->Bin.IUndRef[i].Module)+" at code address "+HEXFORMAT(_Md->Bin.IUndRef[i].CodeAdr)+
          " solved to data type "+_Md->CannonicalTypeName(TypIndex)+
          (HardLinkage?" (address="+HEXFORMAT(_Md->Types[TypIndex].MetaStTypes)+")":""));
          if(HardLinkage){
            if(_Md->Types[TypIndex].MetaStTypes==0){
              SrcInfo.Msg(368).Print(_Md->Bin.IUndRef[i].ObjectName,String((char *)_Md->Bin.IUndRef[i].Module)); 
              return false;
            }
            _Md->Bin.CodeBufferModify(_Md->Bin.IUndRef[i].CodeAdr,_Md->Types[TypIndex].MetaStTypes);
          }
        }
        else{
          if((VarIndex=_Md->VarSearch(_Md->Bin.IUndRef[i].ObjectName,URefModIndex))==-1){
            SrcInfo.Msg(396).Print(_Md->Bin.IUndRef[i].ObjectName,String((char *)_Md->Bin.IUndRef[i].Module)); 
            return false;
          }
          DebugMessage(DebugLevel::CmpLibrary,"Undefined reference "+_Md->Bin.IUndRef[i].ObjectName+" from module "+String((char *)_Md->Bin.IUndRef[i].Module)+" at code address "+HEXFORMAT(_Md->Bin.IUndRef[i].CodeAdr)+
          " solved to global variable "+_Md->Modules[_Md->Variables[VarIndex].Scope.ModIndex].Name+"."+_Md->Variables[VarIndex].Name+
          (HardLinkage?" (address="+HEXFORMAT(_Md->Variables[VarIndex].Address)+")":""));
          if(HardLinkage){
            if(_Md->Variables[VarIndex].Address==0){
              SrcInfo.Msg(397).Print(_Md->Bin.IUndRef[i].ObjectName,String((char *)_Md->Bin.IUndRef[i].Module)); 
              return false;
            }
            _Md->Bin.CodeBufferModify(_Md->Bin.IUndRef[i].CodeAdr,_Md->Variables[VarIndex].Address);
          }
        }
        break;
      
      //Function address
      case UndefRefKind::Fun:
        if(_Md->Bin.IUndRef[i].ObjectName.SearchCount("(")!=1){
          SrcInfo.Msg(398).Print(_Md->Bin.IUndRef[i].ObjectName,String((char *)_Md->Bin.IUndRef[i].Module)); 
          return false;
        }
        SearchName=_Md->Bin.IUndRef[i].ObjectName.Split("(")[0];
        ParmStr=_Md->Bin.IUndRef[i].ObjectName.Split("(")[1].Replace(")","");
        if((FunIndex=_Md->GfnSearch(SearchName,ParmStr))==-1){
          SrcInfo.Msg(399).Print(_Md->Bin.IUndRef[i].ObjectName,String((char *)_Md->Bin.IUndRef[i].Module)); 
          return false;
        }
        DebugMessage(DebugLevel::CmpLibrary,"Undefined reference "+_Md->Bin.IUndRef[i].ObjectName+" from module "+String((char *)_Md->Bin.IUndRef[i].Module)+" at code address "+HEXFORMAT(_Md->Bin.IUndRef[i].CodeAdr)+
        " solved to function "+_Md->Functions[FunIndex].FullName+
        (HardLinkage?" (address="+HEXFORMAT(_Md->Functions[FunIndex].Address)+")":""));
        if(HardLinkage){
          if(_Md->Functions[FunIndex].Address==0){
            SrcInfo.Msg(400).Print(_Md->Bin.IUndRef[i].ObjectName,String((char *)_Md->Bin.IUndRef[i].Module)); 
            return false;
          }
          _Md->Bin.CodeBufferModify(_Md->Bin.IUndRef[i].CodeAdr,_Md->Functions[FunIndex].Address);
        }
        break;
      
      //Fixed array geometry index
      case UndefRefKind::Agx:
        if((TypIndex=_Md->TypSearch(_Md->Bin.IUndRef[i].ObjectName,URefModIndex))==-1){
          SrcInfo.Msg(433).Print(_Md->Bin.IUndRef[i].ObjectName,String((char *)_Md->Bin.IUndRef[i].Module)); 
          return false;
        }
        DebugMessage(DebugLevel::CmpLibrary,"Undefined reference "+_Md->Bin.IUndRef[i].ObjectName+" from module "+String((char *)_Md->Bin.IUndRef[i].Module)+" at code address "+HEXFORMAT(_Md->Bin.IUndRef[i].CodeAdr)+
        " solved to data type "+_Md->CannonicalTypeName(TypIndex)+
        (HardLinkage?" (geomindex="+ToString((int)_Md->Dimensions[_Md->Types[TypIndex].DimIndex].GeomIndex)+")":""));
        if(HardLinkage){
          if(_Md->Types[TypIndex].DimIndex==-1){
            SrcInfo.Msg(434).Print(_Md->Bin.IUndRef[i].ObjectName,String((char *)_Md->Bin.IUndRef[i].Module)); 
            return false;
          }
          if(_Md->Dimensions[_Md->Types[TypIndex].DimIndex].GeomIndex==0){
            SrcInfo.Msg(435).Print(_Md->Bin.IUndRef[i].ObjectName,String((char *)_Md->Bin.IUndRef[i].Module)); 
            return false;
          }
          _Md->Bin.CodeBufferModify(_Md->Bin.IUndRef[i].CodeAdr,(CpuAgx)(_Md->Dimensions[_Md->Types[TypIndex].DimIndex].GeomIndex|ARRGEOMMASK80));
        }
        break;
    }

  }

  //Return code
  return true;

}

//Import single library
bool Compiler::_ImportLibrarySingle(const String& FilePath,const String& Tracker,CpuShr Major,CpuShr Minor,CpuShr RevNr,const SourceInfo& SrcInfo){

  //Variables
  int i;
  int ModIndex;
  String Module;
  String Library;
  CpuAdr InitAddress;
  ScopeDef Scope;
  SubScopeDef SubScope;
  Array<int> LoadedVar;
  Array<int> LoadedFun;
  bool HardLinkage;

  //Get library file path and tracker
  Module=_Stl->FileSystem.GetFileNameNoExt(FilePath);
  Library=_Stl->FileSystem.GetFileName(FilePath);

  //Import library
  HardLinkage=(_Md->CompileToLibrary?false:true);
  if(!_Md->Bin.ImportLibrary(FilePath,HardLinkage,InitAddress,Major,Minor,RevNr,SrcInfo)){
    return false;
  }

  //Deactivate generation of debug symbols (as they are copied within ImportLibrary() and also generated when calling Master::Store*() routines)
  if(_Config.DebugSymbols){
    _Md->DebugSymbols=false;
  }

  //Store module
  _Md->StoreModule(Module,FilePath,true,SrcInfo);
  ModIndex=_Md->Modules.Length()-1;
  _Md->StoreTracker(Tracker,ModIndex);

  //Relocate master data indexes in symbol tables
  //(All indexes are checked in case they are undefined (-1) for simplicity, although there are indexes that are always defined)
  for(i=0;i<_Md->Bin.ILnkSymTables.Dim.Length();i++){
    if(_Md->Bin.ILnkSymTables.Dim[i].TypIndex!=-1){ _Md->Bin.ILnkSymTables.Dim[i].TypIndex+=_Md->Types.Length(); }
  }
  for(i=0;i<_Md->Bin.ILnkSymTables.Typ.Length();i++){
    if(_Md->Bin.ILnkSymTables.Typ[i].FunIndex!=-1){ _Md->Bin.ILnkSymTables.Typ[i].FunIndex+=_Md->Functions.Length(); }
    if(_Md->Bin.ILnkSymTables.Typ[i].SupTypIndex!=-1){ _Md->Bin.ILnkSymTables.Typ[i].SupTypIndex+=_Md->Types.Length(); }
    if(_Md->Bin.ILnkSymTables.Typ[i].OrigTypIndex!=-1){ _Md->Bin.ILnkSymTables.Typ[i].OrigTypIndex+=_Md->Types.Length(); }
    if(_Md->Bin.ILnkSymTables.Typ[i].ElemTypIndex!=-1){ _Md->Bin.ILnkSymTables.Typ[i].ElemTypIndex+=_Md->Types.Length(); }
    if(_Md->Bin.ILnkSymTables.Typ[i].DimIndex!=-1){ _Md->Bin.ILnkSymTables.Typ[i].DimIndex+=_Md->Dimensions.Length(); }
    if(_Md->Bin.ILnkSymTables.Typ[i].FieldLow!=-1){ _Md->Bin.ILnkSymTables.Typ[i].FieldLow+=_Md->Fields.Length(); }
    if(_Md->Bin.ILnkSymTables.Typ[i].FieldHigh!=-1){ _Md->Bin.ILnkSymTables.Typ[i].FieldHigh+=_Md->Fields.Length(); }
    if(_Md->Bin.ILnkSymTables.Typ[i].MemberLow!=-1){ _Md->Bin.ILnkSymTables.Typ[i].MemberLow+=_Md->Functions.Length(); }
    if(_Md->Bin.ILnkSymTables.Typ[i].MemberHigh!=-1){ _Md->Bin.ILnkSymTables.Typ[i].MemberHigh+=_Md->Functions.Length(); }
  }
  for(i=0;i<_Md->Bin.ILnkSymTables.Var.Length();i++){
    if(_Md->Bin.ILnkSymTables.Var[i].FunIndex!=-1){ _Md->Bin.ILnkSymTables.Var[i].FunIndex+=_Md->Functions.Length(); }
    if(_Md->Bin.ILnkSymTables.Var[i].TypIndex!=-1){ _Md->Bin.ILnkSymTables.Var[i].TypIndex+=_Md->Types.Length(); }
  }
  for(i=0;i<_Md->Bin.ILnkSymTables.Fld.Length();i++){
    if(_Md->Bin.ILnkSymTables.Fld[i].TypIndex!=-1){ _Md->Bin.ILnkSymTables.Fld[i].TypIndex+=_Md->Types.Length(); }
    if(_Md->Bin.ILnkSymTables.Fld[i].SupTypIndex!=-1){ _Md->Bin.ILnkSymTables.Fld[i].SupTypIndex+=_Md->Types.Length(); }
  }
  for(i=0;i<_Md->Bin.ILnkSymTables.Fun.Length();i++){
    if(_Md->Bin.ILnkSymTables.Fun[i].TypIndex!=-1){ _Md->Bin.ILnkSymTables.Fun[i].TypIndex+=_Md->Types.Length(); }
    if(_Md->Bin.ILnkSymTables.Fun[i].SupTypIndex!=-1){ _Md->Bin.ILnkSymTables.Fun[i].SupTypIndex+=_Md->Types.Length(); }
    if(_Md->Bin.ILnkSymTables.Fun[i].ParmLow!=-1){ _Md->Bin.ILnkSymTables.Fun[i].ParmLow+=_Md->Parameters.Length(); }
    if(_Md->Bin.ILnkSymTables.Fun[i].ParmHigh!=-1){ _Md->Bin.ILnkSymTables.Fun[i].ParmHigh+=_Md->Parameters.Length(); }
  }
  for(i=0;i<_Md->Bin.ILnkSymTables.Par.Length();i++){
    if(_Md->Bin.ILnkSymTables.Par[i].FunIndex!=-1){ _Md->Bin.ILnkSymTables.Par[i].FunIndex+=_Md->Functions.Length(); }
    if(_Md->Bin.ILnkSymTables.Par[i].TypIndex!=-1){ _Md->Bin.ILnkSymTables.Par[i].TypIndex+=_Md->Types.Length(); }
  }

  //Import dimension indexes
  for(i=0;i<_Md->Bin.ILnkSymTables.Dim.Length();i++){
    _Md->StoreDimension((ArrayIndexes)_Md->Bin.ILnkSymTables.Dim[i].DimSize,(int)_Md->Bin.ILnkSymTables.Dim[i].GeomIndex);
    _Md->Dimensions[_Md->Dimensions.Length()-1].TypIndex=(int)_Md->Bin.ILnkSymTables.Dim[i].TypIndex;
  }

  //Import types
  for(i=0;i<_Md->Bin.ILnkSymTables.Typ.Length();i++){
    Scope=(ScopeDef){ScopeKind::Public,ModIndex,-1};
    SubScope=(SubScopeDef){(int)_Md->Bin.ILnkSymTables.Typ[i].SupTypIndex!=-1?SubScopeKind::Public:SubScopeKind::None,(int)_Md->Bin.ILnkSymTables.Typ[i].SupTypIndex};
    _Md->StoreType(Scope,SubScope,String((char *)_Md->Bin.ILnkSymTables.Typ[i].Name),(MasterType)_Md->Bin.ILnkSymTables.Typ[i].MstType,
    (bool)_Md->Bin.ILnkSymTables.Typ[i].IsTypedef,(int)_Md->Bin.ILnkSymTables.Typ[i].OrigTypIndex,(bool)_Md->Bin.ILnkSymTables.Typ[i].IsSystemDef,(CpuWrd)_Md->Bin.ILnkSymTables.Typ[i].Length,
    (int)_Md->Bin.ILnkSymTables.Typ[i].DimNr,(int)_Md->Bin.ILnkSymTables.Typ[i].ElemTypIndex,(int)_Md->Bin.ILnkSymTables.Typ[i].DimIndex,(int)_Md->Bin.ILnkSymTables.Typ[i].FieldLow,
    (int)_Md->Bin.ILnkSymTables.Typ[i].FieldHigh,(int)_Md->Bin.ILnkSymTables.Typ[i].MemberLow,(int)_Md->Bin.ILnkSymTables.Typ[i].MemberHigh,false,SrcInfo);
    _Md->Types[_Md->Types.Length()-1].AuxFieldOffset=0;
    _Md->Types[_Md->Types.Length()-1].MetaName=_Md->Bin.ILnkSymTables.Typ[i].MetaName;
    _Md->Types[_Md->Types.Length()-1].MetaStNames=_Md->Bin.ILnkSymTables.Typ[i].MetaStNames;
    _Md->Types[_Md->Types.Length()-1].MetaStTypes=_Md->Bin.ILnkSymTables.Typ[i].MetaStTypes;
    _Md->Types[_Md->Types.Length()-1].DlName=String((const char *)_Md->Bin.ILnkSymTables.Typ[i].DlName);
    _Md->Types[_Md->Types.Length()-1].DlType=String((const char *)_Md->Bin.ILnkSymTables.Typ[i].DlType);
  }

  //Import variables
  for(i=0;i<_Md->Bin.ILnkSymTables.Var.Length();i++){
    Scope=(ScopeDef){ScopeKind::Public,ModIndex,-1};
    _Md->StoreVariable(Scope,0,-1,String((char *)_Md->Bin.ILnkSymTables.Var[i].Name),(int)_Md->Bin.ILnkSymTables.Var[i].TypIndex,false,_Md->Bin.ILnkSymTables.Var[i].IsConst,false,false,false,false,false,false,false,SrcInfo,"");
    _Md->Variables[_Md->Variables.Length()-1].IsSourceUsed=true;
    _Md->Variables[_Md->Variables.Length()-1].IsInitialized=true;
    _Md->Variables[_Md->Variables.Length()-1].MetaName=_Md->Bin.ILnkSymTables.Var[i].MetaName;
    _Md->Variables[_Md->Variables.Length()-1].Address=_Md->Bin.ILnkSymTables.Var[i].Address;
    LoadedVar.Add(_Md->Variables.Length()-1);
    DebugMessage(DebugLevel::CmpExpression,"Source used flag set for variable "+_Md->Variables[_Md->Variables.Length()-1].Name+" in scope "+_Md->ScopeName(_Md->Variables[_Md->Variables.Length()-1].Scope));
    DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[_Md->Variables.Length()-1].Name+" in scope "+_Md->ScopeName(_Md->Variables[_Md->Variables.Length()-1].Scope));
  }

  //Import fields
  for(i=0;i<_Md->Bin.ILnkSymTables.Fld.Length();i++){
    SubScope=(SubScopeDef){(SubScopeKind)_Md->Bin.ILnkSymTables.Fld[i].SubScope,(int)_Md->Bin.ILnkSymTables.Fld[i].SupTypIndex};
    _Md->StoreField(SubScope,String((char *)_Md->Bin.ILnkSymTables.Fld[i].Name),(int)_Md->Bin.ILnkSymTables.Fld[i].TypIndex,(bool)_Md->Bin.ILnkSymTables.Fld[i].IsStatic,_Md->Bin.ILnkSymTables.Fld[i].EnumValue,SrcInfo);
    _Md->Fields[_Md->Fields.Length()-1].Offset=_Md->Bin.ILnkSymTables.Fld[i].Offset;
  }

  //Import functions & parameters
  for(i=0;i<_Md->Bin.ILnkSymTables.Fun.Length();i++){
    switch((FunctionKind)_Md->Bin.ILnkSymTables.Fun[i].Kind){
      case FunctionKind::Function:  
        Scope=(ScopeDef){ScopeKind::Public,ModIndex,-1};
        _Md->StoreRegularFunction(Scope,String((char *)_Md->Bin.ILnkSymTables.Fun[i].Name),(int)_Md->Bin.ILnkSymTables.Fun[i].TypIndex,(int)_Md->Bin.ILnkSymTables.Fun[i].IsVoid,false,SrcInfo,"");
        break;
      case FunctionKind::MasterMth: 
        _Md->StoreMasterMethod(String((char *)_Md->Bin.ILnkSymTables.Fun[i].Name),(int)_Md->Bin.ILnkSymTables.Fun[i].TypIndex,(int)_Md->Bin.ILnkSymTables.Fun[i].IsVoid,(int)_Md->Bin.ILnkSymTables.Fun[i].IsInitializer,(int)_Md->Bin.ILnkSymTables.Fun[i].IsMetaMethod,(MasterType)_Md->Bin.ILnkSymTables.Fun[i].MstType,(MasterMethod)_Md->Bin.ILnkSymTables.Fun[i].MstMethod);
        break;
      case FunctionKind::Member:    
        SubScope=(SubScopeDef){SubScopeKind::Public,(int)_Md->Bin.ILnkSymTables.Fun[i].SupTypIndex};
        _Md->StoreMemberFunction(SubScope,String((char *)_Md->Bin.ILnkSymTables.Fun[i].Name),(int)_Md->Bin.ILnkSymTables.Fun[i].TypIndex,(int)_Md->Bin.ILnkSymTables.Fun[i].IsVoid,(int)_Md->Bin.ILnkSymTables.Fun[i].IsInitializer,SrcInfo,"");
        break;
      case FunctionKind::SysCall:   
        Scope=(ScopeDef){ScopeKind::Public,ModIndex,-1};
        _Md->StoreSystemCall(Scope,String((char *)_Md->Bin.ILnkSymTables.Fun[i].Name),(int)_Md->Bin.ILnkSymTables.Fun[i].TypIndex,(int)_Md->Bin.ILnkSymTables.Fun[i].IsVoid,(int)_Md->Bin.ILnkSymTables.Fun[i].SysCallNr,SrcInfo);
        break;
      case FunctionKind::SysFunc:   
        Scope=(ScopeDef){ScopeKind::Public,ModIndex,-1};
        _Md->StoreSystemFunction(Scope,String((char *)_Md->Bin.ILnkSymTables.Fun[i].Name),(int)_Md->Bin.ILnkSymTables.Fun[i].TypIndex,(int)_Md->Bin.ILnkSymTables.Fun[i].IsVoid,(CpuInstCode)_Md->Bin.ILnkSymTables.Fun[i].InstCode,SrcInfo);
        break;  
      case FunctionKind::DlFunc:   
        Scope=(ScopeDef){ScopeKind::Public,ModIndex,-1};
        _Md->StoreDlFunction(Scope,String((char *)_Md->Bin.ILnkSymTables.Fun[i].Name),(int)_Md->Bin.ILnkSymTables.Fun[i].TypIndex,(int)_Md->Bin.ILnkSymTables.Fun[i].IsVoid,String((char *)_Md->Bin.ILnkSymTables.Fun[i].DlName),String((char *)_Md->Bin.ILnkSymTables.Fun[i].DlFunction),SrcInfo);        
        break;
      case FunctionKind::Operator:  
        Scope=(ScopeDef){ScopeKind::Public,ModIndex,-1};
        _Md->StoreOperatorFunction(Scope,String((char *)_Md->Bin.ILnkSymTables.Fun[i].Name),(int)_Md->Bin.ILnkSymTables.Fun[i].TypIndex,(int)_Md->Bin.ILnkSymTables.Fun[i].IsVoid,false,SrcInfo,"");
        break;
    }
    _Md->Functions[_Md->Functions.Length()-1].ParmNr=_Md->Bin.ILnkSymTables.Fun[i].ParmNr;
    _Md->Functions[_Md->Functions.Length()-1].ParmLow=_Md->Bin.ILnkSymTables.Fun[i].ParmLow;
    _Md->Functions[_Md->Functions.Length()-1].ParmHigh=_Md->Bin.ILnkSymTables.Fun[i].ParmHigh;
    _Md->Functions[_Md->Functions.Length()-1].Address=_Md->Bin.ILnkSymTables.Fun[i].Address;
    LoadedFun.Add(_Md->Functions.Length()-1);
  }
  for(i=0;i<_Md->Bin.ILnkSymTables.Par.Length();i++){
    _Md->StoreParameter((int)_Md->Bin.ILnkSymTables.Par[i].FunIndex,String((char *)_Md->Bin.ILnkSymTables.Par[i].Name),(int)_Md->Bin.ILnkSymTables.Par[i].TypIndex,
    (bool)_Md->Bin.ILnkSymTables.Par[i].IsConst,(bool)_Md->Bin.ILnkSymTables.Par[i].IsReference,(int)_Md->Bin.ILnkSymTables.Par[i].ParmOrder,false,SrcInfo,"");
  }
  for(i=0;i<LoadedFun.Length();i++){
    _Md->StoreFunctionSearchIndex(LoadedFun[i]);
    _Md->StoreFunctionId(LoadedFun[i]);
  }

  //Replace & detach system types in library by system types in current module
  for(i=0;i<_Md->Types.Length()-1;i++){
    if(_Md->Types[i].ElemTypIndex!=-1 && _Md->Types[_Md->Types[i].ElemTypIndex].IsSystemDef){
      if((_Md->Types[i].ElemTypIndex=_Md->SystemTypeFilter(_Md->Types[i].ElemTypIndex))==-1){ SrcInfo.Msg(232).Print(_Md->CannonicalTypeName(_Md->Types[i].ElemTypIndex),Module); return false; }
    }
  }
  for(i=0;i<_Md->Variables.Length()-1;i++){
    if(_Md->Variables[i].TypIndex!=-1 && _Md->Types[_Md->Variables[i].TypIndex].IsSystemDef){
      if((_Md->Variables[i].TypIndex=_Md->SystemTypeFilter(_Md->Variables[i].TypIndex))==-1){ SrcInfo.Msg(232).Print(_Md->CannonicalTypeName(_Md->Variables[i].TypIndex),Module); return false; }
    }
  }
  for(i=0;i<_Md->Fields.Length()-1;i++){
    if(_Md->Fields[i].TypIndex!=-1 && _Md->Types[_Md->Fields[i].TypIndex].IsSystemDef){
      if((_Md->Fields[i].TypIndex=_Md->SystemTypeFilter(_Md->Fields[i].TypIndex))==-1){ SrcInfo.Msg(232).Print(_Md->CannonicalTypeName(_Md->Fields[i].TypIndex),Module); return false; }
    }
  }
  for(i=0;i<_Md->Functions.Length()-1;i++){
    if(_Md->Functions[i].TypIndex!=-1 && _Md->Types[_Md->Functions[i].TypIndex].IsSystemDef){
      if((_Md->Functions[i].TypIndex=_Md->SystemTypeFilter(_Md->Functions[i].TypIndex))==-1){ SrcInfo.Msg(232).Print(_Md->CannonicalTypeName(_Md->Functions[i].TypIndex),Module); return false; }
    }
  }
  for(i=0;i<_Md->Parameters.Length()-1;i++){
    if(_Md->Parameters[i].TypIndex!=-1 && _Md->Types[_Md->Parameters[i].TypIndex].IsSystemDef){
      if((_Md->Parameters[i].TypIndex=_Md->SystemTypeFilter(_Md->Parameters[i].TypIndex))==-1){ SrcInfo.Msg(232).Print(_Md->CannonicalTypeName(_Md->Parameters[i].TypIndex),Module); return false; }
    }
  }
  for(i=0;i<_Md->Types.Length();i++){
    if(_Md->Types[i].Scope.Kind==ScopeKind::Public && _Md->Types[i].Scope.ModIndex==ModIndex && _Md->Types[i].IsSystemDef){
      if(!_Md->TypeDetach(i)){ SrcInfo.Msg(233).Print(_Md->CannonicalTypeName(i),Module); return false; }
    }
  }

  //Reactivate generation of debug symbols
  if(_Config.DebugSymbols){
    _Md->DebugSymbols=true;
  }

  //For dynamic functions check definition against library
  for(i=0;i<LoadedFun.Length();i++){
    if(_Md->Functions[LoadedFun[i]].Kind==FunctionKind::DlFunc){
      if(!_Md->DlFunctionCheck(LoadedFun[i],SrcInfo)){ return false; }
    }
  }

  //Solve library undefined references
  DebugMessage(DebugLevel::CmpLibrary,"Solve undefined references for library "+Module);
  if(!_SolveLibraryUndefRefs(HardLinkage,SrcInfo)){ return false; }

  //Emit variable imports in assembler
  for(i=0;i<LoadedVar.Length();i++){
    _Md->Bin.AsmOutVarDecl(AsmSection::Decl,(_Md->Variables[LoadedVar[i]].Scope.Kind!=ScopeKind::Local?true:false),false,
    _Md->Variables[LoadedVar[i]].IsReference,_Md->Variables[LoadedVar[i]].IsConst,_Md->Variables[LoadedVar[i]].IsStatic,_Md->Variables[LoadedVar[i]].Name,
    _Md->CpuDataTypeFromMstType(_Md->Types[_Md->Variables[LoadedVar[i]].TypIndex].MstType),
    _Md->VarLength(LoadedVar[i]),_Md->Variables[LoadedVar[i]].Address,"",true,false,Library,"");
  }

  //Record function addresses (strictly not necessary as all of them have address and do not need forward resolution, but they will be output in assembler function address list)
  for(i=0;i<LoadedFun.Length();i++){
    if(_Md->Functions[LoadedFun[i]].Kind==FunctionKind::Function 
    || _Md->Functions[LoadedFun[i]].Kind==FunctionKind::Member 
    || _Md->Functions[LoadedFun[i]].Kind==FunctionKind::Operator){
      _Md->Bin.StoreFunctionAddress(_Md->Functions[LoadedFun[i]].Fid,_Md->Functions[LoadedFun[i]].FullName,_Md->Functions[LoadedFun[i]].Scope.Depth,_Md->Functions[LoadedFun[i]].Address);
    }
  }

  //Emit function imports in assembler
  for(i=0;i<LoadedFun.Length();i++){
    _Md->AsmFunDeclaration(LoadedFun[i],true,false,Library);
  }
  _Md->Bin.AsmOutFunDecl(AsmSection::Decl,_Md->GetInitFunctionName(Module),"",true,false,false,Library,InitAddress,"");

  //Save init address of library to init routines list (only in hard linkage mode)
  if(HardLinkage){
    _Md->Bin.AppendInitRoutine(InitAddress,Module,_Md->GetInitFunctionName(Module),-1);
  }

  //Return result
  return true;

}


//Reslve library path
String Compiler::_ResolveLibraryPath(const String& ImportName,const String& LibraryPath,const String& DefaultPath){
  if(_Stl->FileSystem.FileExists(LibraryPath+ImportName+LIBRARY_EXT)){ 
    return LibraryPath+ImportName+LIBRARY_EXT; 
  }
  else{ 
    return DefaultPath+ImportName+LIBRARY_EXT; 
  }
}

//Import library recursive call
bool Compiler::_ImportLibraryRecur(const String& ImportName,const String& ImportTracker,const String& LibraryPath,const String& DefaultPath,
                                   CpuShr Major,CpuShr Minor,CpuShr RevNr,const SourceInfo& SrcInfo,int RecurLevel){

  //Variables
  int LibIndex;
  int ModIndex;
  bool LoadLibrary;
  String Tracker;
  String LibName;
  String Module;
  String FilePath;
  String Library;
  String FoundObject;
  BinaryHeader Hdr;
  Array<Dependency> Depen;

  //Debug message
  DebugMessage(DebugLevel::CmpBinary,"Import library "+ImportName);
  
  //Import library dependencies
  FilePath=_ResolveLibraryPath(ImportName,LibraryPath,DefaultPath);
  if(!_Md->Bin.LoadLibraryDependencies(FilePath,Hdr,Depen,SrcInfo)){
    return false;
  }

  //Import depending libraries and given library in loop (LibIndex>=0 for dependinglibs, -1 for given library)
  LibIndex=(Depen.Length()!=0?0:-1);
  do{

    //Select import path
    if(LibIndex==-1){
      LibName=ImportName;    
    }
    else{
      LibName=String((char *)Depen[LibIndex].Module);
    }
  
    //Resolve library path, module and library
    FilePath=_ResolveLibraryPath(LibName,LibraryPath,DefaultPath);
    Module=_Stl->FileSystem.GetFileNameNoExt(FilePath);
    Library=_Stl->FileSystem.GetFileName(FilePath);
    Tracker=(LibIndex==-1 && ImportTracker.Length()!=0?ImportTracker:Module);

    //Check given library is loaded already using same tracker
    if(LibIndex==-1 && _Md->ModSearch(Module)!=-1 && _Md->TrkSearch(Tracker)!=-1){
      SrcInfo.Msg(217).Print(Module);
      return false;
    }

    //Check just if module is loaded already
    LoadLibrary=true;
    if((ModIndex=_Md->ModSearch(Module))!=-1){
      if(LibIndex==-1){ 
        if(_Md->TrkSearch(Tracker)==-1){
          _Md->StoreTracker(Tracker,ModIndex); 
          DebugMessage(DebugLevel::CmpBinary,"Library "+Module+" already imported, just added new tracker "+Tracker);
        }
        else{
          DebugMessage(DebugLevel::CmpBinary,"Library "+Module+" already imported with tracker "+Tracker);
        }
      }
      else{
        DebugMessage(DebugLevel::CmpBinary,"Library "+Module+" already imported, skipped");
      }
      LoadLibrary=false;
    }

    //Load library
    if(LoadLibrary){

      //Dot collission check
      if(!_Md->DotCollissionCheck(Tracker,FoundObject)){
        SrcInfo.Msg(207).Print(Tracker,FoundObject);
        return false;
      }

      //Import library
      if(LibIndex==-1){
        if(!_ImportLibrarySingle(FilePath,Tracker,Major,Minor,RevNr,SrcInfo)){ return false; }
      }
      else{
        if(!_ImportLibraryRecur(Module,Tracker,LibraryPath,DefaultPath,Depen[LibIndex].LibMajorVers,
        Depen[LibIndex].LibMinorVers,Depen[LibIndex].LibRevisionNr,SrcInfo,RecurLevel+1)){ return false; }
      }

      //Store new dependency in current module
      if(_Md->CompileToLibrary){
        if(LibIndex==-1){
          _Md->Bin.StoreDependency(Module,Hdr.LibMajorVers,Hdr.LibMinorVers,Hdr.LibRevisionNr);
        }
        else{
          _Md->Bin.StoreDependency(Module,Depen[LibIndex].LibMajorVers,Depen[LibIndex].LibMinorVers,Depen[LibIndex].LibRevisionNr);
        }
      }


    }

    //Exit loop after importing given library
    if(LibIndex==-1){ break; }

    //Calculate next index
    if(LibIndex<Depen.Length()-1){
      LibIndex++;
    }
    else{
      LibIndex=-1;
    }

  }while(true);

  //Return code
  return true;

}

//Process import sentence
//import (<string>|<identifier>) [as <id>] [version n.n.n]
bool Compiler::_CompileImport(Sentence& Stn,const String& LibraryPath,const String& DefaultPath){

  //Variables
  String ImportName;
  String ImportTracker;
  CpuShr Major;
  CpuShr Minor;
  CpuShr RevNr;

  //Get library file path and tracker
  if(!Stn.Get(PrKeyword::Import).Ok()){ return false; }
  if(Stn.Is(PrTokenId::Identifier)){
    if(!Stn.ReadId(ImportName).Ok()){ return false; }
  }
  else{
    if(!Stn.ReadSt(ImportName).Ok()){ return false; }
  }

  //Get tracker
  if(Stn.Is(PrKeyword::As)){
    if(!Stn.Get(PrKeyword::As).ReadId(ImportTracker).Ok()){
      return false;
    }
  }
  else{
    ImportTracker="";
  }

  //Get version requirement
  if(Stn.Is(PrKeyword::Version)){
    if(!Stn.Get(PrKeyword::Version).ReadSh(Major).Get(PrOperator::Member).ReadSh(Minor).Get(PrOperator::Member).ReadSh(RevNr).Ok()){
      return false;
    }
  }
  else{
    Major=-1;
    Minor=-1;
    RevNr=-1;
  }

  //Import library
  if(!_ImportLibraryRecur(ImportName,ImportTracker,LibraryPath,DefaultPath,Major,Minor,RevNr,Stn.Tokens[1].SrcInfo(),0)){ return false; }

  //Return code
  return true;

}

//Process compile sentence
//include [<string>|<identifier] as <id>
bool Compiler::_CompileInclude(Sentence& Stn,bool& AlreadyLoaded,const String& IncludePath,const String& DefaultPath){

  //Variables
  int ModIndex;
  String Module;
  String Path;
  String Tracker;
  String FoundObject;

  //Get include path
  if(!Stn.Get(PrKeyword::Include).Ok()){ return false; }
  if(Stn.Is(PrTokenId::Identifier)){
    if(!Stn.ReadId(Path).Ok()){ return false; }
  }
  else{
    if(!Stn.ReadSt(Path).Ok()){ return false; }
  }
  if(_Stl->FileSystem.FileExists(IncludePath+Path+SOURCE_EXT)){ Path=IncludePath+Path+SOURCE_EXT; }
  else{ Path=DefaultPath+Path+SOURCE_EXT; }

  //Get module name
  Module=_Stl->FileSystem.GetFileNameNoExt(Path);

  //Get tracker
  if(Stn.Is(PrKeyword::As)){
    if(!Stn.Get(PrKeyword::As).ReadId(Tracker).Ok()){
      return false;
    }
  }
  else{
    Tracker=Module;
  }

  //Check module is loaded already using same tracker
  if(_Md->ModSearch(Module)!=-1 && _Md->TrkSearch(Tracker)!=-1){
    Stn.Msg(217).Print(Module);
    return false;
  }

  //Check just if module is loaded already
  if((ModIndex=_Md->ModSearch(Module))!=-1){
    _Md->StoreTracker(Tracker,ModIndex);
    AlreadyLoaded=true;
    return true;
  }
  else{
    AlreadyLoaded=false;
  }

  //Dot collission check
  if(!_Md->DotCollissionCheck(Tracker,FoundObject)){
    Stn.Msg(207).Print(Tracker,FoundObject);
    return false;
  }

  //Store module
  _Md->StoreModule(Module,Path,false,Stn.Tokens[1].SrcInfo());
  _Md->StoreTracker(Tracker,_Md->Modules.Length()-1);

  //Return result
  return true;

}

//Process deftype sentence
//type <typespec> as <id>
bool Compiler::_CompileDeftype(Sentence& Stn){

  //Variables
  int TypIndex;
  String TypeName;
  String FoundObject;
  SourceInfo SrcInfo;

  //Check syntax
  if(!Stn.Get(PrKeyword::DefType).Ok()){ return false; }

  //Get data type specification
  if(!ReadTypeSpec(_Md,Stn,_Md->CurrentScope(),TypIndex)){
    return false;
  }

  //Get destination type name
  if(!Stn.Get(PrKeyword::As).ReadId(TypeName).Ok()){ return false; }
  SrcInfo=Stn.LastSrcInfo();

  //Add supperior class name (if any)
  if(_Md->CurrentSubScope().Kind!=SubScopeKind::None){
    TypeName=_Md->Types[_Md->CurrentSubScope().TypIndex].Name+"."+TypeName;
  }

  //Destination datatype must not exist
  if(_Md->TypSearch(TypeName,_Md->CurrentScope().ModIndex)!=-1){
    Stn.Msg(26).Print(TypeName);
    return false;
  }

  //Dot collission check
  if(_Md->Types[TypIndex].MstType==MasterType::Class || _Md->Types[TypIndex].MstType==MasterType::Enum){
    if(!_Md->DotCollissionCheck(TypeName,FoundObject)){
      Stn.Msg(208).Print(TypeName,FoundObject);
      return false;
    }
  }

  //Store new type
  _Md->StoreType(_Md->CurrentScope(),_Md->CurrentSubScope(),TypeName,_Md->Types[TypIndex].MstType,true,TypIndex,false,_Md->Types[TypIndex].Length,
  _Md->Types[TypIndex].DimNr,_Md->Types[TypIndex].ElemTypIndex,_Md->Types[TypIndex].DimIndex,_Md->Types[TypIndex].FieldLow,_Md->Types[TypIndex].FieldHigh,
  _Md->Types[TypIndex].MemberLow,_Md->Types[TypIndex].MemberHigh,true,SrcInfo);
  
  //Return result
  return true;

}

//Process dltype sentence
//dltype<str,str> from <id>
bool Compiler::_CompileDlType(Sentence& Stn){

  //Variables
  int TypIndex;
  int ModIndex;
  String DlName;
  String DlType;
  String PrTypeName;
  String TypeName;

  //Parse sentence
  if(!Stn.Get(PrKeyword::DlType).Get(PrOperator::Less).ReadSt(DlName).Get(PrPunctuator::Comma)
  .ReadSt(DlType).Get(PrOperator::Greater).Get(PrKeyword::From).ReadTy(PrTypeName).Ok()){ return false; }

  //Translate type comming from parser
  _Md->DecoupleTypeName(PrTypeName,TypeName,ModIndex);

  //Check mapped data type exists
  if((TypIndex=_Md->TypSearch(TypeName,ModIndex))==-1){
    Stn.Msg(263).Print(PrTypeName);
    return false;
  }

  //Check DlName and DlType are not set already
  if(_Md->Types[TypIndex].DlName.Length()!=0 || _Md->Types[TypIndex].DlType.Length()!=0){
    Stn.Msg(266).Print(TypeName,_Md->Types[TypIndex].DlType,_Md->Types[TypIndex].DlName);
    return false;
  }

  //Check mapped data type is not an array (as dlfunctions have special syntax for arrays)
  if(_Md->Types[TypIndex].MstType==MasterType::FixArray || _Md->Types[TypIndex].MstType==MasterType::DynArray){
    Stn.Msg(264).Print(TypeName);
    return false;
  }

  //Length for DlName and DlType must not be larger than _MaxIdLen
  if(DlName.Length()>_MaxIdLen){
    Stn.Msg(261).Print(DlName,ToString(_MaxIdLen));
    return false;
  }
  if(DlType.Length()>_MaxIdLen){
    Stn.Msg(265).Print(DlType,ToString(_MaxIdLen));
    return false;
  }

  //Check library and type are existing
  if(!_Md->DlTypeCheck(DlType,DlName,Stn.Tokens[2].SrcInfo())){
    return false;
  }

  //Update data type and symbol
  _Md->Types[TypIndex].DlName=DlName;
  _Md->Types[TypIndex].DlType=DlType;
  if(_Md->Types[TypIndex].LnkSymIndex!=-1){
    _Md->Bin.UpdateLnkSymTypeForDlType(_Md->Types[TypIndex].LnkSymIndex,DlName,DlType);
  }
  
  //Return result
  return true;

}

//Constant declaration
//const <type> <id>
bool Compiler::_CompileConst(Sentence& Stn){

  //Variables
  int Start;
  int End;
  int TypIndex;
  int VarIndex; 
  bool Computed1;
  bool Computed2;
  String VarName;
  String StaticVarName;
  String InitExpr;
  Expression Expr;
  String FoundObject;
  ExprToken Result;
  ExprToken Const;
  SourceInfo SrcInfo;

  //Check syntax
  if(!Stn.Get(PrKeyword::Const).Ok()){ return false; }

  //Constant declaration with data type
  if(!Stn.Is(PrKeyword::Var)){

    //Get data type specification
    if(!ReadTypeSpec(_Md,Stn,_Md->CurrentScope(),TypIndex)){
      return false;
    }

    //Module public objects cannot use datatypes of inner modules since they are not exported
    if(_Md->CurrentScope().Kind==ScopeKind::Public && _Md->Types[TypIndex].Scope.ModIndex!=_Md->CurrentScope().ModIndex && !_Md->Types[TypIndex].IsSystemDef){
      Stn.Msg(206).Print(_Md->CannonicalTypeName(TypIndex),_Md->Modules[_Md->Types[TypIndex].Scope.ModIndex].Name);
      return false;
    }

    //Get constant name
    if(!Stn.ReadId(VarName).Ok()){ return false; }
    SrcInfo=Stn.LastSrcInfo();

    //Constant must not be already defined
    if(_Md->VarSearch(VarName,_Md->CurrentScope().ModIndex)!=-1){
      Stn.Msg(35).Print(VarName);
      return false;
    }

    //Dot collission check
    if(_Md->Types[TypIndex].MstType==MasterType::Class || _Md->Types[TypIndex].MstType==MasterType::Enum){
      if(!_Md->DotCollissionCheck(VarName,FoundObject)){
        Stn.Msg(185).Print(VarName,FoundObject);
        return false;
      }
    }

    //Local constants cannot be defined with local types (since initialization code is compiled in global scope)
    if(_Md->CurrentScope().Kind==ScopeKind::Local && _Md->Types[TypIndex].Scope.Kind==ScopeKind::Local){
      Stn.Msg(361).Print(VarName);
      return false;
    }

    //Store new constant (check local variable can be reused before creating new one)
    if(!_Md->ReuseVariable(_Md->CurrentScope(),Stn.GetCodeBlockId(),-1,VarName,TypIndex,false,true,false,SrcInfo,SysMsgDispatcher::GetSourceLine(),VarIndex)){ return false; }
    if(VarIndex==-1){
      _Md->CleanHidden(_Md->CurrentScope(),VarName);
      _Md->StoreVariable(_Md->CurrentScope(),Stn.GetCodeBlockId(),-1,VarName,TypIndex,false,true,false,false,false,false,false,true,true,SrcInfo,SysMsgDispatcher::GetSourceLine());
      VarIndex=_Md->Variables.Length()-1;
    }

    //Set constant as temp locked variable if it is identified as a complex litteral value
    if(VarName.StartsWith(_Md->GetComplexLitValuePrefix())){
      _Md->Variables[_Md->Variables.Length()-1].IsTempVar=true;
      _Md->Variables[_Md->Variables.Length()-1].IsTempLocked=true;
    }

    //Send error if initialization is not given for constant
    if(!Stn.Is(PrOperator::Assign)){
      Stn.Msg(127).Print();
      return false;
    }

    //For local constants initialization is emitted into delayed init routine so that it happens only once
    //(When source line comes from insertion buffer we avoid this since this is a line comming from 
    //complex lit value replacements and it might have depencies on same scope)
    if(_Md->CurrentScope().Kind==ScopeKind::Local && (Stn.Origin()==OrigBuffer::Source || Stn.Origin()==OrigBuffer::Split)){
      if(!Stn.Get(PrOperator::Assign).ReadEx(Start,End).Ok()){ return false; }
      StaticVarName=_Md->GetStaticVarName(_Md->CurrentScope().FunIndex,VarName);
      InitExpr=Stn.Text(Start,End);
      _PsStack.Top().Add(StaticVarName+"="+InitExpr);
    }
    
    //For global constants initialization expression is compiled / computed
    else{
      _Md->Bin.AsmOutNewLine(AsmSection::Body);
      _Md->Bin.AsmOutCommentLine(AsmSection::Body,_SourceLine.Trim(),true);
      if(!Stn.Get(PrOperator::Assign).ReadEx(Start,End).Ok()){ return false; }
      if(!Expr.Compile(_Md,_Md->CurrentScope(),Stn,Start,End,Result,Computed1)){ return false; }
      if(!Expression().CompileDataTypePromotion(_Md,_Md->CurrentScope(),Stn.GetCodeBlockId(),Result,_Md->Types[TypIndex].MstType,Computed2)){ return false; }
      if(Computed1 && Computed2 && _Md->CurrentScope().Kind!=ScopeKind::Local){
        _Md->Bin.AsmDeleteLast(AsmSection::Body);
        _Md->Bin.AsmDeleteLast(AsmSection::Body);
        if(!Result.CopyValue(_Md->Bin.GlobValuePointer(_Md->Variables[VarIndex].Address))){ return false; }
        _Md->Variables[VarIndex].IsComputed=true;
      }
      else{
        Const.ThisVar(_Md,VarIndex,SrcInfo);
        if(!Expression().CopyOperand(_Md,Const,Result)){ return false; } 
      }
    }
  
  }
  
  //Constant declaration without data type
  else{

    //Get constant name
    if(!Stn.Get(PrKeyword::Var).ReadId(VarName).Ok()){ return false; }
    SrcInfo=Stn.LastSrcInfo();

    //Constant must not be already defined
    if(_Md->VarSearch(VarName,_Md->CurrentScope().ModIndex)!=-1){
      Stn.Msg(35).Print(VarName);
      return false;
    }

    //Send error if initialization is not given for constant
    if(!Stn.Is(PrOperator::Assign)){
      Stn.Msg(127).Print();
      return false;
    }

    //Local constants need tobe declared with explicit data type
    //(Reason is that they are compiled elsewere in addition buffer so we cannot guess data type at this point)
    if(_Md->CurrentScope().Kind==ScopeKind::Local){
      Stn.Msg(333).Print();
      return false;
    }

    //Compile/Compute init value expression
    _Md->Bin.AsmOutNewLine(AsmSection::Body);
    _Md->Bin.AsmOutCommentLine(AsmSection::Body,_SourceLine.Trim(),true);
    if(!Stn.Get(PrOperator::Assign).ReadEx(Start,End).Ok()){ return false; }
    if(!_FindIdentifier(Stn,Start,End,VarName)){ return false; }
    if(!Expr.Compile(_Md,_Md->CurrentScope(),Stn,Start,End,Result,Computed1)){ return false; }
    TypIndex=Result.TypIndex();

    //Module public objects cannot use datatypes of inner modules since they are not exported
    if(_Md->CurrentScope().Kind==ScopeKind::Public && _Md->Types[TypIndex].Scope.ModIndex!=_Md->CurrentScope().ModIndex && !_Md->Types[TypIndex].IsSystemDef){
      Stn.Msg(206).Print(_Md->CannonicalTypeName(TypIndex),_Md->Modules[_Md->Types[TypIndex].Scope.ModIndex].Name);
      return false;
    }

    //Dot collission check
    if(_Md->Types[TypIndex].MstType==MasterType::Class || _Md->Types[TypIndex].MstType==MasterType::Enum){
      if(!_Md->DotCollissionCheck(VarName,FoundObject)){
        Stn.Msg(185).Print(VarName,FoundObject);
        return false;
      }
    }

    //Local constants cannot be defined local types (since initialization code is compiled in global scope)
    if(_Md->CurrentScope().Kind==ScopeKind::Local && _Md->Types[TypIndex].Scope.Kind==ScopeKind::Local){
      Stn.Msg(361).Print(VarName);
      return false;
    }

    //Store new constant
    if(!_Md->ReuseVariable(_Md->CurrentScope(),Stn.GetCodeBlockId(),-1,VarName,TypIndex,false,true,false,SrcInfo,SysMsgDispatcher::GetSourceLine(),VarIndex)){ return false; }
    if(VarIndex==-1){
      _Md->CleanHidden(_Md->CurrentScope(),VarName);
      _Md->StoreVariable(_Md->CurrentScope(),Stn.GetCodeBlockId(),-1,VarName,TypIndex,false,true,false,false,false,false,false,true,true,SrcInfo,SysMsgDispatcher::GetSourceLine());
      VarIndex=_Md->Variables.Length()-1;
    }

    //Set constant as temp locked variable if it is identified as a complex litteral value
    if(VarName.StartsWith(_Md->GetComplexLitValuePrefix())){
      _Md->Variables[_Md->Variables.Length()-1].IsTempVar=true;
      _Md->Variables[_Md->Variables.Length()-1].IsTempLocked=true;
    }

    //Copy init value expression result into variable
    if(Computed1 && _Md->CurrentScope().Kind!=ScopeKind::Local){
      _Md->Bin.AsmDeleteLast(AsmSection::Body);
      _Md->Bin.AsmDeleteLast(AsmSection::Body);
      if(!Result.CopyValue(_Md->Bin.GlobValuePointer(_Md->Variables[VarIndex].Address))){ return false; }
    }
    else{
      Const.ThisVar(_Md,VarIndex,SrcInfo);
      if(!Expression().CopyOperand(_Md,Const,Result)){ return false; } 
    }

  }

  //Set constant as initialized
  _Md->Variables[VarIndex].IsInitialized=true;
  DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[VarIndex].Name+" in scope "+_Md->ScopeName(_Md->Variables[VarIndex].Scope));

  //Constant declaration in assembler file
  _Md->Bin.AsmOutVarDecl(AsmSection::Decl,(_Md->CurrentScope().Kind!=ScopeKind::Local?true:false),false,false,true,false,VarName,
  _Md->CpuDataTypeFromMstType(_Md->Types[TypIndex].MstType),_Md->VarLength(VarIndex),
  _Md->Variables[VarIndex].Address,_SourceLine,false,false,"",(_Md->CurrentScope().Kind!=ScopeKind::Local?Result.Asm().ToText():"delayed{"+InitExpr+"}"));

  //Return result
  return true;

}

//Variable declaration
//<type> <id>
bool Compiler::_CompileVar(Sentence& Stn){

  //Variables
  int TypIndex;
  int Start;
  int End;
  int VarIndex;
  bool Computed1;
  bool Computed2;
  bool CodeGenerated;
  String VarName;
  String StaticVarName;
  String InitExpr;
  String InitValue;
  Expression Expr;
  String FoundObject;
  ExprToken Result;
  ExprToken Variv;
  SourceInfo SrcInfo;

  //Variable declaration with type
  if(!Stn.Is(PrKeyword::Var)){

    //Static variables can only happen in local scopes
    if(Stn.Static){
      if(_Md->CurrentScope().Kind!=ScopeKind::Local){
        Stn.Msg(329).Print(VarName);
        return false;
      }
    }

    //Get data type specification
    if(!ReadTypeSpec(_Md,Stn,_Md->CurrentScope(),TypIndex)){
      return false;
    }

    //Module public objects cannot use datatypes of inner modules since they are not exported
    if(_Md->CurrentScope().Kind==ScopeKind::Public && _Md->Types[TypIndex].Scope.ModIndex!=_Md->CurrentScope().ModIndex && !_Md->Types[TypIndex].IsSystemDef){
      Stn.Msg(206).Print(_Md->CannonicalTypeName(TypIndex),_Md->Modules[_Md->Types[TypIndex].Scope.ModIndex].Name);
      return false;
    }

    //Get variable name
    if(!Stn.ReadId(VarName).Ok()){ return false; }
    SrcInfo=Stn.LastSrcInfo();

    //Variable must not be already defined
    if(_Md->VarSearch(VarName,_Md->CurrentScope().ModIndex)!=-1){
      Stn.Msg(31).Print(VarName);
      return false;
    }

    //Dot collission check
    if(_Md->Types[TypIndex].MstType==MasterType::Class || _Md->Types[TypIndex].MstType==MasterType::Enum){
      if(!_Md->DotCollissionCheck(VarName,FoundObject)){
        Stn.Msg(185).Print(VarName,FoundObject);
        return false;
      }
    }

    //Store new variable
    if(!_Md->ReuseVariable(_Md->CurrentScope(),Stn.GetCodeBlockId(),-1,VarName,TypIndex,Stn.Static,false,false,SrcInfo,SysMsgDispatcher::GetSourceLine(),VarIndex)){ return false; }
    if(VarIndex==-1){
      _Md->CleanHidden(_Md->CurrentScope(),VarName);
      _Md->StoreVariable(_Md->CurrentScope(),Stn.GetCodeBlockId(),-1,VarName,TypIndex,Stn.Static,false,false,false,false,false,false,true,true,SrcInfo,SysMsgDispatcher::GetSourceLine());
      VarIndex=_Md->Variables.Length()-1;
    }

    //Compile initial value expression
    if(Stn.Is(PrOperator::Assign)){
      
      //Static variables cannot be defined using local types (since initialization code is compiled in global scope)
      if(Stn.Static && _Md->Types[TypIndex].Scope.Kind==ScopeKind::Local){
        Stn.Msg(362).Print(VarName);
        return false;
      }

      //For static variables initialization is emitted into delayed init routine so that it happens only once
      if(Stn.Static && (Stn.Origin()==OrigBuffer::Source || Stn.Origin()==OrigBuffer::Split)){
        if(!Stn.Get(PrOperator::Assign).ReadEx(Start,End).Ok()){ return false; }
        StaticVarName=_Md->GetStaticVarName(_Md->CurrentScope().FunIndex,VarName);
        InitExpr=_Md->ReplaceStaticVarNames(_Md->CurrentScope(),Stn.SubSentence(Start,End)).Text();
        _PsStack.Top().Add(StaticVarName+"="+InitExpr);
      }
      
      //For regular variables initialization expression is compiled / computed
      else{
        _Md->Bin.AsmOutNewLine(AsmSection::Body);
        _Md->Bin.AsmOutCommentLine(AsmSection::Body,_SourceLine.Trim(),true);
        if(!Stn.Get(PrOperator::Assign).ReadEx(Start,End).Ok()){ return false; }
        if(!Expr.Compile(_Md,_Md->CurrentScope(),Stn,Start,End,Result,Computed1)){ return false; }
        if(!Expression().CompileDataTypePromotion(_Md,_Md->CurrentScope(),Stn.GetCodeBlockId(),Result,_Md->Types[TypIndex].MstType,Computed2)){ return false; }
        if(Computed1 && Computed2 && (_Md->CurrentScope().Kind!=ScopeKind::Local)){
          _Md->Bin.AsmDeleteLast(AsmSection::Body);
          _Md->Bin.AsmDeleteLast(AsmSection::Body);
          if(!Result.CopyValue(_Md->Bin.GlobValuePointer(_Md->Variables[VarIndex].Address))){ return false; }
        }
        else{
          Variv.ThisVar(_Md,VarIndex,SrcInfo);
          if(!Expression().CopyOperand(_Md,Variv,Result)){ return false; } 
        }
      }

      //Set variable as initialized
      _Md->Variables[VarIndex].IsInitialized=true;
      DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[VarIndex].Name+" in scope "+_Md->ScopeName(_Md->Variables[VarIndex].Scope));

    }
  
    //Compile initial value expression
    else if(Stn.Is(PrOperator::Asterisk) || _Md->InitializeVars==true){
      
      //Read asterisk token
      if(Stn.Is(PrOperator::Asterisk)){
        if(!Stn.Get(PrOperator::Asterisk).Ok()){ return false; }
      }

      //Static variables cannot be defined using local types (since initialization code is compiled in global scope)
      if(Stn.Static && _Md->Types[TypIndex].Scope.Kind==ScopeKind::Local){
        Stn.Msg(362).Print(VarName);
        return false;
      }

      //Initialization for static variables is emitted into delayed init routine so that it happens only once
      if(Stn.Static && (Stn.Origin()==OrigBuffer::Source || Stn.Origin()==OrigBuffer::Split)){
        StaticVarName=_Md->GetStaticVarName(_Md->CurrentScope().FunIndex,VarName);
        _PsStack.Top().Add(_KwdInitVar+" "+StaticVarName);
      }
      
      //Initialization for regular variables
      else{
        _Md->Bin.AsmOutNewLine(AsmSection::Body);
        _Md->Bin.AsmOutCommentLine(AsmSection::Body,_SourceLine.Trim(),true);
        if(_Md->CurrentScope().Kind!=ScopeKind::Local 
        && (_Md->IsMasterAtomic(TypIndex) || _Md->Types[TypIndex].MstType==MasterType::Enum) 
        && _Md->Types[TypIndex].MstType!=MasterType::String){
          _Md->Bin.AsmDeleteLast(AsmSection::Body);
          _Md->Bin.AsmDeleteLast(AsmSection::Body);
          if(_Md->Types[TypIndex].MstType==MasterType::Enum){
            if(!Result.NewConst(_Md,_Md->Types[_Md->IntTypIndex].MstType,SrcInfo)){ return false; }
          }
          else{
            if(!Result.NewConst(_Md,_Md->Types[TypIndex].MstType,SrcInfo)){ return false; }
          }
          if(!Result.CopyValue(_Md->Bin.GlobValuePointer(_Md->Variables[VarIndex].Address))){ return false; }
        }
        else{
          Variv.ThisVar(_Md,VarIndex,SrcInfo);
          if(!Expression().InitOperand(_Md,Variv,CodeGenerated)){ return false; } 
          if(!CodeGenerated){
            _Md->Bin.AsmDeleteLast(AsmSection::Body);
            _Md->Bin.AsmDeleteLast(AsmSection::Body);
          }
        }
      }

      //Set variable as initialized
      _Md->Variables[VarIndex].IsInitialized=true;
      DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[VarIndex].Name+" in scope "+_Md->ScopeName(_Md->Variables[VarIndex].Scope));

    }

    //Error message in case there are tokens left
    else if(Stn.TokensLeft()!=0){
      Stn.Msg(544).Print();
      return false;
    }
  
  }

  //Variable declaration without type
  else{

    //Get variable name
    if(!Stn.Get(PrKeyword::Var).ReadId(VarName).Ok()){ return false; }
    SrcInfo=Stn.LastSrcInfo();

    //Variable must not be already defined
    if(_Md->VarSearch(VarName,_Md->CurrentScope().ModIndex)!=-1){
      Stn.Msg(31).Print(VarName);
      return false;
    }

    //Static variables must have exlicit data type
    //(Reason is that the init value expresion is compiled elsewhere in addition buffer so we cannot guess data type at this point)
    if(Stn.Static){
      Stn.Msg(332).Print();
      return false;
    }

    //Initial value expresion is mandatory if type is not given
    if(!Stn.Is(PrOperator::Assign)){
      Stn.Msg(331).Print(VarName);
      return false;
    }

    //Compile/Compute initial value expression
    _Md->Bin.AsmOutNewLine(AsmSection::Body);
    _Md->Bin.AsmOutCommentLine(AsmSection::Body,_SourceLine.Trim(),true);
    if(!Stn.Get(PrOperator::Assign).ReadEx(Start,End).Ok()){ return false; }
    if(!_FindIdentifier(Stn,Start,End,VarName)){ return false; }
    if(!Expr.Compile(_Md,_Md->CurrentScope(),Stn,Start,End,Result,Computed1)){ return false; }
    TypIndex=Result.TypIndex();

    //Module public objects cannot use datatypes of inner modules since they are not exported
    if(_Md->CurrentScope().Kind==ScopeKind::Public && _Md->Types[TypIndex].Scope.ModIndex!=_Md->CurrentScope().ModIndex && !_Md->Types[TypIndex].IsSystemDef){
      Stn.Msg(206).Print(_Md->CannonicalTypeName(TypIndex),_Md->Modules[_Md->Types[TypIndex].Scope.ModIndex].Name);
      return false;
    }

    //Dot collission check
    if(_Md->Types[TypIndex].MstType==MasterType::Class || _Md->Types[TypIndex].MstType==MasterType::Enum){
      if(!_Md->DotCollissionCheck(VarName,FoundObject)){
        Stn.Msg(185).Print(VarName,FoundObject);
        return false;
      }
    }

    //Store new variable
    if(!_Md->ReuseVariable(_Md->CurrentScope(),Stn.GetCodeBlockId(),-1,VarName,TypIndex,false,false,false,SrcInfo,SysMsgDispatcher::GetSourceLine(),VarIndex)){ return false; }
    if(VarIndex==-1){
      _Md->CleanHidden(_Md->CurrentScope(),VarName);
      _Md->StoreVariable(_Md->CurrentScope(),Stn.GetCodeBlockId(),-1,VarName,TypIndex,false,false,false,false,false,false,false,true,true,SrcInfo,SysMsgDispatcher::GetSourceLine());
      VarIndex=_Md->Variables.Length()-1;
    }

    //Copy init value into variable
    if(Computed1 && (_Md->CurrentScope().Kind!=ScopeKind::Local)){
      _Md->Bin.AsmDeleteLast(AsmSection::Body);
      _Md->Bin.AsmDeleteLast(AsmSection::Body);
      if(!Result.CopyValue(_Md->Bin.GlobValuePointer(_Md->Variables[VarIndex].Address))){ return false; }
    }
    else{
      Variv.ThisVar(_Md,VarIndex,SrcInfo);
      if(!Expression().CopyOperand(_Md,Variv,Result)){ return false; } 
    }

    //Set variable as initialized
    _Md->Variables[VarIndex].IsInitialized=true;
    DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[VarIndex].Name+" in scope "+_Md->ScopeName(_Md->Variables[VarIndex].Scope));

  }
  
  //Variable declaration in assembler file
  if((Stn.Tokens[Stn.GetProcIndex()-1].Id()==PrTokenId::Operator && Stn.Tokens[Stn.GetProcIndex()-1].Value.Opr==PrOperator::Asterisk) || _Md->InitializeVars==true){
    InitValue="*";
  }
  else{
    InitValue=(_Md->Variables[VarIndex].IsInitialized?(!Stn.Static?Result.Asm().ToText():"delayed{"+InitExpr+"}"):"");
  }
  _Md->Bin.AsmOutVarDecl(AsmSection::Decl,(_Md->CurrentScope().Kind!=ScopeKind::Local?true:false),false,false,false,Stn.Static,VarName,
  _Md->CpuDataTypeFromMstType(_Md->Types[TypIndex].MstType),_Md->VarLength(VarIndex),_Md->Variables[VarIndex].Address,
  _SourceLine,false,false,"",InitValue);

  //Return result
  return true;

}

//Field declaration
//<type> <id>
bool Compiler::_CompileClassField(Sentence& Stn){

  //Variables
  int TypIndex;
  int Start;
  int End;
  int VarIndex;
  int FldIndex;
  String FldName;
  String VarName;
  ScopeDef Scope;
  String InitExpr;
  String InitValue;
  SourceInfo SrcInfo;

  //Get data type specification
  if(!ReadTypeSpec(_Md,Stn,_Md->CurrentScope(),TypIndex)){
    return false;
  }

  //Module public objects cannot use datatypes of inner modules since they are not exported
  if(_Md->CurrentScope().Kind==ScopeKind::Public && _Md->Types[TypIndex].Scope.ModIndex!=_Md->CurrentScope().ModIndex && !_Md->Types[TypIndex].IsSystemDef){
    Stn.Msg(206).Print(_Md->CannonicalTypeName(TypIndex),_Md->Modules[_Md->Types[TypIndex].Scope.ModIndex].Name);
    return false;
  }

  //Get member name
  if(!Stn.ReadId(FldName).Ok()){ return false; }
  SrcInfo=Stn.LastSrcInfo();

  //Member must not be already defined
  if(_Md->FldSearch(FldName,_Md->CurrentSubScope().TypIndex)!=-1){
    Stn.Msg(37).Print(FldName,_Md->CannonicalTypeName(_Md->CurrentSubScope().TypIndex));
    return false;
  }

  //Store new field
  _Md->StoreField(_Md->CurrentSubScope(),FldName,TypIndex,Stn.Static,0,SrcInfo);
  FldIndex=_Md->Fields.Length()-1;

  //Update member indexes in parent datatype
  if(_Md->Types[_Md->CurrentSubScope().TypIndex].FieldLow==-1){ _Md->Types[_Md->CurrentSubScope().TypIndex].FieldLow=_Md->Fields.Length()-1; }
  _Md->Types[_Md->CurrentSubScope().TypIndex].FieldHigh=_Md->Fields.Length()-1;

  //If field is static we must create a variable in module private scope
  if(Stn.Static){
    
    //Get variable name
    Scope=_Md->CurrentScope();
    Scope.Kind=(_Md->CurrentSubScope().Kind==SubScopeKind::Public?ScopeKind::Public:ScopeKind::Private);
    VarName=_Md->GetStaticFldName(FldIndex);
    if(_Md->VarSearch(VarName,_Md->CurrentScope().ModIndex)!=-1){
      Stn.Msg(183).Print(_Md->CannonicalTypeName(_Md->CurrentSubScope().TypIndex),FldName);
      return false;
    }
    
    //Store variable
    _Md->StoreVariable(Scope,Stn.GetCodeBlockId(),-1,VarName,TypIndex,false,false,false,false,false,false,false,true,false,SrcInfo,SysMsgDispatcher::GetSourceLine());
    VarIndex=_Md->Variables.Length()-1;
    
    //Compile initial value
    if(Stn.Is(PrOperator::Assign)){
      if(!Stn.Get(PrOperator::Assign).ReadEx(Start,End).Ok()){ return false; }
      InitExpr=Stn.Text(Start,End);
      _PsStack.Top().Add(VarName+"="+InitExpr);
      _Md->Variables[VarIndex].IsInitialized=true;
      DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[VarIndex].Name+" in scope "+_Md->ScopeName(_Md->Variables[VarIndex].Scope));
    }
    
    //Compile initialization
    else if(Stn.Is(PrOperator::Asterisk) || _Md->InitializeVars==true){
      if(Stn.Is(PrOperator::Asterisk)){
        if(!Stn.Get(PrOperator::Asterisk).Ok()){ return false; }
      }
      _PsStack.Top().Add(_KwdInitVar+" "+VarName);
      _Md->Variables[VarIndex].IsInitialized=true;
      DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[VarIndex].Name+" in scope "+_Md->ScopeName(_Md->Variables[VarIndex].Scope));
    }
    
    //Error message in case there are tokens left
    else if(Stn.TokensLeft()!=0){
      Stn.Msg(545).Print();
      return false;
    }

    //Variable declaration in assembler file
    if((Stn.Tokens[Stn.GetProcIndex()-1].Id()==PrTokenId::Operator && Stn.Tokens[Stn.GetProcIndex()-1].Value.Opr==PrOperator::Asterisk) || _Md->InitializeVars==true){
      InitValue="*";
    }
    else{
      InitValue=(_Md->Variables[VarIndex].IsInitialized?"delayed{"+InitExpr+"}":"");
    }
    _Md->Bin.AsmOutVarDecl(AsmSection::Decl,true,false,false,false,false,VarName,
    _Md->CpuDataTypeFromMstType(_Md->Types[TypIndex].MstType),_Md->VarLength(VarIndex),_Md->Variables[VarIndex].Address,
    _SourceLine,false,false,"",InitValue);

  }

  //Return result
  return true;

}

//Process Public member scope
//.publ
bool Compiler::_CompilePubl(Sentence& Stn){

  //Check syntax
  if(!Stn.Get(PrKeyword::Publ).Ok()){ return false; }

  //Return code
  return true;

}

//Process Private member scope
//.priv
bool Compiler::_CompilePriv(Sentence& Stn){

  //Check syntax
  if(!Stn.Get(PrKeyword::Priv).Ok()){ return false; }

  //Return code
  return true;

}

//Process defclass sentence
//defclass <id>:
bool Compiler::_CompileDefClass(Sentence& Stn){

  //Variables
  int TypIndex;
  String TypeName;
  String FoundObject;

  //Get data type name
  if(!Stn.Get(PrKeyword::DefClass).ReadId(TypeName).Get(PrPunctuator::Colon).Ok()){ return false; }

  //Add supperior class name (if any)
  if(_Md->CurrentSubScope().Kind!=SubScopeKind::None){
    TypeName=_Md->Types[_Md->CurrentSubScope().TypIndex].Name+"."+TypeName;
  }

  //Data type must not exist already
  if((TypIndex=_Md->TypSearch(TypeName,_Md->CurrentScope().ModIndex))!=-1){
    Stn.Msg(39).Print(TypeName);
    return false;
  }
  
  //Dot collission check
  if(!_Md->DotCollissionCheck(TypeName,FoundObject)){
    Stn.Msg(208).Print(TypeName,FoundObject);
    return false;
  }

  //Store new type
  _Md->StoreType(_Md->CurrentScope(),_Md->CurrentSubScope(),TypeName,MasterType::Class,false,-1,false,0,0,-1,-1,-1,-1,-1,-1,true,Stn.Tokens[1].SrcInfo());

  //Return result
  return true;

}

//Process endclass sentence
//:endclass
bool Compiler::_CompileEndClass(Sentence& Stn){

  //Variables
  int i;
  bool Found;
  CpuAdr StNames;
  CpuAdr StTypes;
  CpuLon Length;
  Array<String> FieldNames;
  Array<String> FieldTypes;

  //Check syntax
  if(!Stn.Get(PrKeyword::EndClass).Ok()){ return false; }

  //Check that at least one member function or field was defined
  Found=false;
  for(i=0;i<_Md->Functions.Length();i++){
    if(_Md->Functions[i].SubScope.TypIndex==_Md->CurrentSubScope().TypIndex){ Found=true; break; }
  }
  if(!Found && _Md->Types[_Md->CurrentSubScope().TypIndex].FieldLow==-1){
    Stn.Msg(187).Print(_Md->CannonicalTypeName(_Md->CurrentSubScope().TypIndex));
    return false;
  }

  //Update class field metadata
  if(_Md->Types[_Md->CurrentSubScope().TypIndex].FieldLow !=-1
  && _Md->Types[_Md->CurrentSubScope().TypIndex].FieldHigh!=-1){
  
    //Get arrays with field names and field types
    i=-1;
    while((i=_Md->FieldLoop(_Md->CurrentSubScope().TypIndex,i))!=-1){
      FieldNames.Add(_Md->Fields[i].Name);
      FieldTypes.Add(_Md->Types[_Md->Fields[i].TypIndex].Name);
    }
  
    //Recalculate type length
    Length=_Md->TypLength(_Md->CurrentSubScope().TypIndex);

    //Define metadata variables
    StNames=_Md->StoreLitStringArray(FieldNames);
    StTypes=_Md->StoreLitStringArray(FieldTypes);

    //Update class variable
    _Md->Types[_Md->CurrentSubScope().TypIndex].Length=Length;
    _Md->Types[_Md->CurrentSubScope().TypIndex].MetaStNames=StNames;
    _Md->Types[_Md->CurrentSubScope().TypIndex].MetaStTypes=StTypes;

    //Update corresponding linker symbol
    if(_Md->Types[_Md->CurrentSubScope().TypIndex].LnkSymIndex!=-1){
      _Md->Bin.UpdateLnkSymTypeForField(_Md->Types[_Md->CurrentSubScope().TypIndex].LnkSymIndex,StNames,StTypes,Length,
      _Md->Fields[_Md->Types[_Md->CurrentSubScope().TypIndex].FieldLow].LnkSymIndex,
      _Md->Fields[_Md->Types[_Md->CurrentSubScope().TypIndex].FieldHigh].LnkSymIndex);
    }

    //Update corresponding debug symbol
    if(_Md->Types[_Md->CurrentSubScope().TypIndex].DbgSymIndex!=-1){
      _Md->Bin.UpdateDbgSymTypeForField(_Md->Types[_Md->CurrentSubScope().TypIndex].DbgSymIndex,Length,
      _Md->Fields[_Md->Types[_Md->CurrentSubScope().TypIndex].FieldLow].DbgSymIndex,
      _Md->Fields[_Md->Types[_Md->CurrentSubScope().TypIndex].FieldHigh].DbgSymIndex);
    }

  }

  //Update class function member metadata
  if(_Md->Types[_Md->CurrentSubScope().TypIndex].MemberLow !=-1
  && _Md->Types[_Md->CurrentSubScope().TypIndex].MemberHigh!=-1){
    if(_Md->Types[_Md->CurrentSubScope().TypIndex].LnkSymIndex!=-1){
      _Md->Bin.UpdateLnkSymTypeForFunctionMember(_Md->Types[_Md->CurrentSubScope().TypIndex].LnkSymIndex,
      _Md->Functions[_Md->Types[_Md->CurrentSubScope().TypIndex].MemberLow].LnkSymIndex,
      _Md->Functions[_Md->Types[_Md->CurrentSubScope().TypIndex].MemberHigh].LnkSymIndex);
    }
  }

  //Return result
  return true;

}

//Define grant
//Does recursive call when destination of grant is a field and it is a class
bool Compiler::_DefineGrant(const Sentence& Stn,GrantKind Kind,int TypIndex,const String& FrTypName,const String& FrFunName,int ToFldIndex,int ToFunIndex,int FrLineNr,int FrColNr,int RecurLevel){

  //Variables
  int i;
  GrantKind FunKind;
  String FrName;
  String ToName;

  //From and to names just for message
  FrName=_Md->GrantFrName(Kind,TypIndex,FrTypName,FrFunName);
  ToName=_Md->GrantToName(Kind,TypIndex,ToFldIndex,ToFunIndex);

  //Check grant is already defined
  if(_Md->GraSearch(Kind,TypIndex,FrTypName,FrFunName,ToFldIndex,ToFunIndex)!=-1){
    Stn.Msg(216).Print(FrName,ToName); 
    return false;
  }

  //Store grant
  _Md->StoreGrant(Kind,TypIndex,FrTypName,FrFunName,ToFldIndex,ToFunIndex,FrLineNr,FrColNr);

  //When destination of grant is a field and it is a class it must store grants for all its fields and function members as well
  if((Kind==GrantKind::ClassToField || Kind==GrantKind::FunctToField || Kind==GrantKind::FunMbToField || Kind==GrantKind::OpertToField) 
  && _Md->Types[_Md->Fields[ToFldIndex].TypIndex].MstType==MasterType::Class){
    
    //Store field grants
    i=-1;
    while((i=_Md->FieldLoop(_Md->Fields[ToFldIndex].TypIndex,i))!=-1){
      if(_Md->Fields[i].SubScope.Kind==SubScopeKind::Public){
        if(!_DefineGrant(Stn,Kind,_Md->Fields[ToFldIndex].TypIndex,FrTypName,FrFunName,i,-1,FrLineNr,FrColNr,RecurLevel+1)){
          return false;
        }
      }
    }

    //Store function member grants
    if(Kind==GrantKind::ClassToField){ FunKind=GrantKind::ClassToFunMb; } 
    else if(Kind==GrantKind::FunctToField){ FunKind=GrantKind::FunctToFunMb; }
    else if(Kind==GrantKind::FunMbToField){ FunKind=GrantKind::FunMbToFunMb; }
    else if(Kind==GrantKind::OpertToField){ FunKind=GrantKind::OpertToFunMb; }
    i=-1;
    while((i=_Md->MemberLoop(_Md->Fields[ToFldIndex].TypIndex,i))!=-1){
      if(_Md->Functions[i].SubScope.Kind==SubScopeKind::Public){
        if(!_DefineGrant(Stn,FunKind,_Md->Fields[ToFldIndex].TypIndex,FrTypName,FrFunName,-1,i,FrLineNr,FrColNr,RecurLevel+1)){
          return false;
        }
      }
    }

  }

  //Return code
  return true;

}

//Grant
//allow <frname> to <toname>
bool Compiler::_CompileAllow(Sentence& Stn){

  //Variables
  int ModIndex;
  int TrkIndex;
  String Id;
  String LongId;
  String PrevLongId;
  String PrTypeName;
  String TypeName;
  String FrTypName;
  String FrFunName;
  int ToFldIndex;
  int ToFunIndex;
  String FrCase;
  String ToCase;
  PrOperator Oper;
  GrantKind Kind;
  String FrName;
  String ToName;
  int FrLineNr;
  int FrColNr;

  //Process allow keyword
  if(!Stn.Get(PrKeyword::Allow).Ok()){ return false; }

  //Get from name
  if(Stn.Is(PrPunctuator::BegBracket)){
    if(!Stn.Get(PrPunctuator::BegBracket).ReadOp(Oper).Get(PrPunctuator::EndBracket).Get(PrPunctuator::BegParen).Get(PrPunctuator::EndParen).Ok()){ return false; }
    Id=_Md->GetOperatorFuncName(Stn.Tokens[Stn.GetProcIndex()-4].Text());
    FrTypName="";
    FrFunName=Id;
    FrCase="OP";
    FrLineNr=Stn.Tokens[2].LineNr;
    FrColNr=Stn.Tokens[2].ColNr;
  }
  else{
    FrTypName="";
    FrFunName="";
    if(Stn.Is(PrTokenId::TypeName)){
      if(!Stn.ReadTy(PrTypeName).Ok()){ return false; }
      _Md->DecoupleTypeName(PrTypeName,TypeName,ModIndex);
      if(ModIndex!=-1 && ModIndex!=_Md->CurrentScope().ModIndex){
        Stn.Msg(366).Print();
        return false;  
      }
      if(Stn.Is(PrOperator::Member)){
        if(!Stn.Get(PrOperator::Member).ReadId(Id).Get(PrPunctuator::BegParen).Get(PrPunctuator::EndParen).Ok()){ return false; }
        FrTypName=TypeName;
        FrFunName=Id;
        FrCase="FM";
      }
      else{
        FrTypName=TypeName;
        FrCase="CL";
      }
    }
    else{
      if(!Stn.ReadId(Id).Ok()){ return false; }
      TrkIndex=_Md->TrkSearch(Id);
      if(TrkIndex!=-1 && _Md->Trackers[TrkIndex].ModIndex!=_Md->CurrentScope().ModIndex){
        Stn.Msg(366).Print();
        return false;  
      }
      if(!Stn.Is(PrOperator::Member)){
        if(Stn.Is(PrPunctuator::BegParen)){
          if(!Stn.Get(PrPunctuator::BegParen).Get(PrPunctuator::EndParen).Ok()){ return false; }
          FrFunName=(Id==MAIN_FUNCTION?_Md->GetMainFunctionName():Id);
          FrCase="FU";
        }
        else{
          FrTypName=Id;
          FrCase="CL";
        }
      }
      else{
        PrevLongId="";
        LongId=Id;
        while(Stn.Is(PrOperator::Member)){
          if(!Stn.Get(PrOperator::Member).ReadId(Id).Ok()){ return false; }
          PrevLongId=LongId;
          LongId+="."+Id;
        }
        if(Stn.Is(PrPunctuator::BegParen)){
          if(!Stn.Get(PrPunctuator::BegParen).Get(PrPunctuator::EndParen).Ok()){ return false; }
          FrTypName=PrevLongId;
          FrFunName=Id;
          FrCase="FM";
        }
        else{
          FrTypName=LongId;
          FrCase="CL";
        }
      }
    }
    FrLineNr=Stn.Tokens[1].LineNr;
    FrColNr=Stn.Tokens[1].ColNr;
  }

  //Get to name
  if(Stn.Is(PrKeyword::To)){
    if(!Stn.Get(PrKeyword::To).ReadId(Id).Ok()){ return false; }
    if(Stn.Is(PrPunctuator::BegParen)){
      if(!Stn.Get(PrPunctuator::BegParen).Get(PrPunctuator::EndParen).Ok()){ return false; }
      if((ToFunIndex=_Md->FmbSearch(Id,_Md->CurrentScope().ModIndex,_Md->CurrentSubScope().TypIndex,"","",nullptr,true))==-1){
        Stn.Msg(211).Print(Id,_Md->CannonicalTypeName(_Md->CurrentSubScope().TypIndex));
        return false;
      }
      ToFldIndex=-1;
      ToCase="FM";
    }
    else{
      if((ToFldIndex=_Md->FldSearch(Id,_Md->CurrentSubScope().TypIndex))==-1){
        Stn.Msg(213).Print(Id,_Md->CannonicalTypeName(_Md->CurrentSubScope().TypIndex));
        return false;
      }
      ToFunIndex=-1;
      ToCase="FL";
    }
  }
  else if(Stn.TokensLeft()==0){
    ToFldIndex=-1;
    ToFunIndex=-1;
    ToCase="CL";
  }
  else{
    Stn.Msg(212).Print();
    return false;
  }

  //Calculate grant type
  if(FrCase=="CL"){
    if     (ToCase=="CL"){ Kind=GrantKind::ClassToClass; }
    else if(ToCase=="FL"){ Kind=GrantKind::ClassToField; }
    else if(ToCase=="FM"){ Kind=GrantKind::ClassToFunMb; }
    else{ Stn.Msg(215).Print(FrCase,ToCase); return false; }
  }
  else if(FrCase=="FU"){
    if     (ToCase=="CL"){ Kind=GrantKind::FunctToClass; }
    else if(ToCase=="FL"){ Kind=GrantKind::FunctToField; }
    else if(ToCase=="FM"){ Kind=GrantKind::FunctToFunMb; }
    else{ Stn.Msg(215).Print(FrCase,ToCase); return false; }
  }
  else if(FrCase=="FM"){
    if     (ToCase=="CL"){ Kind=GrantKind::FunMbToClass; }
    else if(ToCase=="FL"){ Kind=GrantKind::FunMbToField; }
    else if(ToCase=="FM"){ Kind=GrantKind::FunMbToFunMb; }
    else{ Stn.Msg(215).Print(FrCase,ToCase); return false; }
  }
  else if(FrCase=="OP"){
    if     (ToCase=="CL"){ Kind=GrantKind::OpertToClass; }
    else if(ToCase=="FL"){ Kind=GrantKind::OpertToField; }
    else if(ToCase=="FM"){ Kind=GrantKind::OpertToFunMb; }
    else{ Stn.Msg(215).Print(FrCase,ToCase); return false; }
  }
  else{
    Stn.Msg(215).Print(FrCase,ToCase); 
    return false;
  }

  //Define grant
  if(!_DefineGrant(Stn,Kind,_Md->CurrentSubScope().TypIndex,FrTypName,FrFunName,ToFldIndex,ToFunIndex,FrLineNr,FrColNr,0)){
    return false;
  }

  //Return code
  return true;

}

//Process DefEnum sentence
//defenum <id>:
bool Compiler::_CompileDefEnum(Sentence& Stn){

  //Variables
  int TypIndex;
  String TypeName;
  String FoundObject;

  //Get data type name
  if(!Stn.Get(PrKeyword::DefEnum).ReadId(TypeName).Get(PrPunctuator::Colon).Ok()){ return false; }

  //Add supperior class name (if any)
  if(_Md->CurrentSubScope().Kind!=SubScopeKind::None){
    TypeName=_Md->Types[_Md->CurrentSubScope().TypIndex].Name+"."+TypeName;
  }

  //Data type must not exist before
  if((TypIndex=_Md->TypSearch(TypeName,_Md->CurrentScope().ModIndex))!=-1){
    Stn.Msg(180).Print(TypeName);
    return false;
  }

  //Dot collission check
  if(!_Md->DotCollissionCheck(TypeName,FoundObject)){
    Stn.Msg(208).Print(TypeName,FoundObject);
    return false;
  }

  //Store new type
  _Md->StoreType(_Md->CurrentScope(),_Md->CurrentSubScope(),TypeName,MasterType::Enum,false,-1,false,0,0,-1,-1,-1,-1,-1,-1,true,Stn.Tokens[1].SrcInfo());

  //Return result
  return true;

}

//Process EnumField sentence
//<id>
//<id>=<expr>
bool Compiler::_CompileEnumField(Sentence& Stn){

  //Variables
  int Start;
  int End;
  String FldName;
  String VarName;
  Expression Expr;
  ExprToken Result;
  CpuInt Value;
  SourceInfo SrcInfo;

  //Get field name
  if(!Stn.ReadId(FldName).Ok()){ return false; }
  SrcInfo=Stn.LastSrcInfo();

  //Get value from expression or from internal counter
  if(Stn.Is(PrOperator::Assign)){
    if(!Stn.Get(PrOperator::Assign).ReadEx(Start,End).Ok()){ return false; }
    if(!Expr.Compute(_Md,_Md->CurrentScope(),Stn,Start,End,Result)){ return false; }
    if(Result.MstType()==MasterType::Char){
      Value=Result.Value.Chr;
    }
    else if(Result.MstType()==MasterType::Short){
      Value=Result.Value.Shr;
    }
    else if(Result.MstType()==MasterType::Integer){
      Value=Result.Value.Int;
    }
    else{
      Stn.Msg(182).Print(FldName,_Md->CannonicalTypeName(_Md->CurrentSubScope().TypIndex));
      return false;
    }
  }
  else{
    Value=_Md->Types[_Md->CurrentSubScope().TypIndex].EnumCount;
  }

  //Member must not be already defined
  if(_Md->FldSearch(FldName,_Md->CurrentSubScope().TypIndex)!=-1){
    Stn.Msg(181).Print(FldName,_Md->CannonicalTypeName(_Md->CurrentSubScope().TypIndex));
    return false;
  }

  //Store new field
  _Md->StoreField(_Md->CurrentSubScope(),FldName,_Md->IntTypIndex,false,Value,SrcInfo);

  //Update member high and enum counter in parent datatype
  if(_Md->Types[_Md->CurrentSubScope().TypIndex].FieldLow==-1){ _Md->Types[_Md->CurrentSubScope().TypIndex].FieldLow=_Md->Fields.Length()-1; }
  _Md->Types[_Md->CurrentSubScope().TypIndex].FieldHigh=_Md->Fields.Length()-1;
  _Md->Types[_Md->CurrentSubScope().TypIndex].EnumCount=Value+1;

  //Return result
  return true;
}

//Process EndEnum sentence
//:endenum
bool Compiler::_CompileEndEnum(Sentence& Stn){

  //Variables
  int i;
  CpuAdr StNames;
  CpuAdr StTypes;
  Array<String> FieldNames;
  Array<String> FieldTypes;

  //Check syntax
  if(!Stn.Get(PrKeyword::EndEnum).Ok()){ return false; }

  //Check that at least one field was defined
  if(_Md->Types[_Md->CurrentSubScope().TypIndex].FieldLow==-1){
    Stn.Msg(188).Print(_Md->CannonicalTypeName(_Md->CurrentSubScope().TypIndex));
    return false;
  }

  //Get arrays with field names and field types
  i=-1;
  while((i=_Md->FieldLoop(_Md->CurrentSubScope().TypIndex,i))!=-1){
    FieldNames.Add(_Md->Fields[i].Name);
    FieldTypes.Add(_Md->Types[_Md->Fields[i].TypIndex].Name);
  }

  //Define metadata variables
  StNames=_Md->StoreLitStringArray(FieldNames);
  StTypes=_Md->StoreLitStringArray(FieldTypes);

  //Update class variable
  _Md->Types[_Md->CurrentSubScope().TypIndex].MetaStNames=StNames;
  _Md->Types[_Md->CurrentSubScope().TypIndex].MetaStTypes=StTypes;

  //Update corresponding linker symbol
  if(_Md->Types[_Md->CurrentSubScope().TypIndex].LnkSymIndex!=-1){
    _Md->Bin.UpdateLnkSymTypeForField(_Md->Types[_Md->CurrentSubScope().TypIndex].LnkSymIndex,StNames,StTypes,
    _Md->Types[_Md->CurrentSubScope().TypIndex].Length,
    _Md->Fields[_Md->Types[_Md->CurrentSubScope().TypIndex].FieldLow].LnkSymIndex,
    _Md->Fields[_Md->Types[_Md->CurrentSubScope().TypIndex].FieldHigh].LnkSymIndex);
  }

  //Update corresponding debug symbol
  if(_Md->Types[_Md->CurrentSubScope().TypIndex].DbgSymIndex!=-1){
    _Md->Bin.UpdateDbgSymTypeForField(_Md->Types[_Md->CurrentSubScope().TypIndex].DbgSymIndex,
    _Md->Types[_Md->CurrentSubScope().TypIndex].Length,
    _Md->Fields[_Md->Types[_Md->CurrentSubScope().TypIndex].FieldLow].DbgSymIndex,
    _Md->Fields[_Md->Types[_Md->CurrentSubScope().TypIndex].FieldHigh].DbgSymIndex);
  }

  //Return result
  return true;

}

//Function declaration
//systemcall<n> <type><id>([ref] <id>,[ref] <id>, ...)
//systemfunc<inst> void <id>([ref] <id>,[ref] <id>, ...)
//dlfunction<"lib","func"> <type><id>([ref] <id>,[ref] <id>, ...)
//<type> <id>([ref] <id>,[ref] <id>, ...)
//<type> [op]([ref] <id>, <id>)
bool Compiler::_CompileFuncDecl(Sentence& Stn,int& FunIndex){

  //Variables
  bool IsVoid;
  bool PassByRef;
  bool ConstRef;
  bool IsConstructor;
  bool IsInitializer;
  String FunName;
  String ParName;
  String Namespace;
  int ResTypIndex;
  int ParTypIndex;
  int ParmNr;
  int SysCallNr;
  int OperIndex;
  int FoundFunIndex;
  String SysCallId;
  String InstCodeStr;
  String DlName;
  String DlFunction;
  CpuInstCode InstCode;
  FunctionKind Kind;
  String ParmStr;
  PrOperator Oper;
  String FoundObject;
  SourceInfo SrcInfo;

  //Default values
  SysCallNr=-1;
  InstCode=(CpuInstCode)-1;
  IsConstructor=false;

  //Detect function kind
  if(Stn.Id==SentenceId::FunDecl || Stn.Id==SentenceId::Function || Stn.Id==SentenceId::Operator){
    OperIndex=_FindOperatorFunction(Stn);
    if(_Md->CurrentSubScope().Kind!=SubScopeKind::None && OperIndex!=-1){
      Stn.Tokens[OperIndex].Msg(175).Print();
      return false;
    }
    if(_Md->CurrentSubScope().Kind!=SubScopeKind::None){
      if(Stn.Is(PrTokenId::TypeName) && Stn.TokensLeft()>=2 
      && Stn.Tokens[Stn.GetProcIndex()+0].Value.Typ==_Md->Types[_Md->CurrentSubScope().TypIndex].Name
      && Stn.Tokens[Stn.GetProcIndex()+1].Id()==PrTokenId::Punctuator
      && Stn.Tokens[Stn.GetProcIndex()+1].Value.Pnc==PrPunctuator::BegParen){
        IsConstructor=true;
      }
      Kind=FunctionKind::Member;
    }
    else if(OperIndex!=-1){
      if(!Expression::IsOverloadableOperator(Stn.Tokens[OperIndex].Value.Opr)){
        Stn.Tokens[OperIndex].Msg(172).Print(Stn.Tokens[OperIndex].Text());
        return false;
      }
      Kind=FunctionKind::Operator;
    }
    else{
      Kind=FunctionKind::Function;
    }
  }
  else if(Stn.Id==SentenceId::SystemCall){
    if(!Stn.Get(PrKeyword::SystemCall).Get(PrOperator::Less).ReadId(SysCallId).Get(PrOperator::Greater).Ok()){ return false; }
    if(_Md->CurrentScope().Kind==ScopeKind::Local){
      Stn.Msg(200).Print();
      return false;
    }
    if((SysCallNr=(int)FindSystemCall(SysCallId))==-1){
      Stn.Msg(246).Print(SysCallId);
      return false;
    }
    Kind=FunctionKind::SysCall;
  }
  else if(Stn.Id==SentenceId::SystemFunc){
    if(!Stn.Get(PrKeyword::SystemFunc).Get(PrOperator::Less).ReadSt(InstCodeStr).Get(PrOperator::Greater).Ok()){ return false; }
    if((int)(InstCode=_Md->Bin.SearchMnemonic(InstCodeStr))==-1){
      Stn.Msg(124).Print(InstCodeStr);
      return false;
    }
    if(_Md->CurrentScope().Kind==ScopeKind::Local){
      Stn.Msg(201).Print();
      return false;
    }
    Kind=FunctionKind::SysFunc;
  }
  else if(Stn.Id==SentenceId::DlFunction){
    if(!Stn.Get(PrKeyword::DlFunction).Get(PrOperator::Less).ReadSt(DlName).Get(PrPunctuator::Comma).ReadSt(DlFunction).Get(PrOperator::Greater).Ok()){ return false; }
    if(_Md->CurrentScope().Kind==ScopeKind::Local){
      Stn.Msg(247).Print();
      return false;
    }
    if(DlName.Length()>_MaxIdLen){
      Stn.Msg(261).Print(DlName,ToString(_MaxIdLen));
      return false;
    }
    if(DlFunction.Length()>_MaxIdLen){
      Stn.Msg(262).Print(DlFunction,ToString(_MaxIdLen));
      return false;
    }
    Kind=FunctionKind::DlFunc;
  }

  //Detect initializer function member
  if(Kind==FunctionKind::Member && Stn.Init){
    IsInitializer=true;
  }
  else{
    IsInitializer=false; 
  }

  //Detect void functions / Get returning data type
  if(IsConstructor){
    ResTypIndex=_Md->CurrentSubScope().TypIndex;
    IsVoid=false;
  }
  else if(Stn.Is(PrKeyword::Void)){
    Stn.Get(PrKeyword::Void);
    IsVoid=true;
  }
  else{
    if(Kind==FunctionKind::SysFunc){
      Stn.Msg(49).Print();
      return false;
    }
    if(!ReadTypeSpec(_Md,Stn,_Md->CurrentScope(),ResTypIndex)){
      return false;
    }
    if(_Md->CurrentScope().Kind==ScopeKind::Public && _Md->Types[ResTypIndex].Scope.ModIndex!=_Md->CurrentScope().ModIndex && !_Md->Types[ResTypIndex].IsSystemDef){
      Stn.Msg(206).Print(_Md->CannonicalTypeName(ResTypIndex),_Md->Modules[_Md->Types[ResTypIndex].Scope.ModIndex].Name);
      return false;
    }
    IsVoid=false;
  }

  //Get function name
  if(Kind==FunctionKind::Operator){
    if(!Stn.Get(PrPunctuator::BegBracket).ReadOp(Oper).Get(PrPunctuator::EndBracket).Ok()){ return false; }
    FunName=_Md->GetOperatorFuncName(Stn.Tokens[Stn.GetProcIndex()-2].Text());
    SrcInfo=Stn.Tokens[Stn.GetProcIndex()-2].SrcInfo();
  }
  else if(IsConstructor){
    if(!Stn.ReadTy(FunName).Ok()){ return false; }
    SrcInfo=Stn.LastSrcInfo();
  }
  else{
    if(!Stn.ReadId(FunName).Ok()){ return false; }
    SrcInfo=Stn.LastSrcInfo();
  }
  if(!Stn.Get(PrPunctuator::BegParen).Ok()){ return false; }

  //Store new function
  switch(Kind){
    case FunctionKind::Function   : _Md->StoreRegularFunction(_Md->CurrentScope(),FunName,(IsVoid?-1:ResTypIndex),IsVoid,_Md->CurrentScope().Kind==ScopeKind::Local?true:false,SrcInfo,SysMsgDispatcher::GetSourceLine()); break;
    case FunctionKind::Member     : _Md->StoreMemberFunction(_Md->CurrentSubScope(),FunName,(IsVoid?-1:ResTypIndex),IsVoid,IsInitializer,SrcInfo,SysMsgDispatcher::GetSourceLine()); break;
    case FunctionKind::SysCall    : _Md->StoreSystemCall(_Md->CurrentScope(),FunName,(IsVoid?-1:ResTypIndex),IsVoid,SysCallNr,SrcInfo); break;
    case FunctionKind::SysFunc    : _Md->StoreSystemFunction(_Md->CurrentScope(),FunName,(IsVoid?-1:ResTypIndex),IsVoid,InstCode,SrcInfo); break;
    case FunctionKind::DlFunc     : _Md->StoreDlFunction(_Md->CurrentScope(),FunName,(IsVoid?-1:ResTypIndex),IsVoid,DlName,DlFunction,SrcInfo); break;
    case FunctionKind::Operator   : _Md->StoreOperatorFunction(_Md->CurrentScope(),FunName,(IsVoid?-1:ResTypIndex),IsVoid,_Md->CurrentScope().Kind==ScopeKind::Local?true:false,SrcInfo,SysMsgDispatcher::GetSourceLine()); break;
    case FunctionKind::MasterMth  : Stn.Msg(296).Print(); return false;
  }
  FunIndex=_Md->Functions.Length()-1;

  //Update function member indexes in parent datatype
  if(Kind==FunctionKind::Member){
    if(_Md->Types[_Md->CurrentSubScope().TypIndex].MemberLow==-1){ _Md->Types[_Md->CurrentSubScope().TypIndex].MemberLow=FunIndex; }
    _Md->Types[_Md->CurrentSubScope().TypIndex].MemberHigh=FunIndex;
  }

  //Init parameter count
  ParmNr=0;

  //Store parameter for function result (passed by reference)
  if(!IsVoid){
    _Md->StoreParameter(FunIndex,_Md->GetFuncResultName(),ResTypIndex,false,true,ParmNr,true,SourceInfo(),"");
    ParmNr++;
  }

  //Store parameter for reference to self on member functions
  if(Kind==FunctionKind::Member){
    _Md->StoreParameter(FunIndex,_Md->GetSelfRefName(),_Md->CurrentSubScope().TypIndex,false,true,ParmNr,true,SourceInfo(),"");
    ParmNr++;
  }

  //Function without parameters
  if(Stn.Is(PrPunctuator::EndParen)){ 
    if(!Stn.Get(PrPunctuator::EndParen).Ok()){ 
      _Md->DeleteFunction(FunIndex);
      return false; 
    }
    if(IsConstructor){
      Stn.Msg(297).Print(FunName);
      _Md->DeleteFunction(FunIndex);
      return false; 
    }
    ParmNr=(IsVoid?0:1); 
    ParmStr=""; 
  }

  //Function with parameters
  else{
    
    //Parameter loop
    ParmStr="";
    do{
  
      //Init passing mode
      PassByRef=false;
      ConstRef=false;
  
      //Detect pass by reference
      if(Stn.Is(PrKeyword::Ref)){ PassByRef=true; Stn.Get(PrKeyword::Ref); }
  
      //Get data type specification
      if(!ReadTypeSpec(_Md,Stn,_Md->CurrentScope(),ParTypIndex)){
        _Md->DeleteFunction(FunIndex);
        return false;
      }
  
      //Get parameter
      if(!Stn.ReadId(ParName).Ok()){ _Md->DeleteFunction(FunIndex); return false; }
      SrcInfo=Stn.LastSrcInfo();
  
      //Module public objects cannot use datatypes of inner modules since they are not exported
      if(_Md->CurrentScope().Kind==ScopeKind::Public && _Md->Types[ParTypIndex].Scope.ModIndex!=_Md->CurrentScope().ModIndex && !_Md->Types[ParTypIndex].IsSystemDef){
        Stn.Msg(206).Print(_Md->CannonicalTypeName(ParTypIndex),_Md->Modules[_Md->Types[ParTypIndex].Scope.ModIndex].Name);
        _Md->DeleteFunction(FunIndex);
        return false;
      }
  
      //Parameter names cannot be repeated
      if(_Md->ParSearch(ParName,FunIndex)!=-1){
        SysMessage(48,Stn.FileName(),Stn.LineNr(),Stn.LastColNr()).Print(ParName,FunName);
        _Md->DeleteFunction(FunIndex);
        return false;
      }
  
      //Dot collission check
      if(_Md->Types[ParTypIndex].MstType==MasterType::Class || _Md->Types[ParTypIndex].MstType==MasterType::Enum){
        if(!_Md->DotCollissionCheck(ParName,FoundObject)){
          Stn.Msg(186).Print(ParName,FoundObject);
          _Md->DeleteFunction(FunIndex);
          return false;
        }
      }

      //Force pass by reference for classes, strings and arrays
      if(_Md->Types[ParTypIndex].MstType==MasterType::Class 
      || _Md->Types[ParTypIndex].MstType==MasterType::String 
      || _Md->Types[ParTypIndex].MstType==MasterType::DynArray
      || _Md->Types[ParTypIndex].MstType==MasterType::FixArray){
        if(!PassByRef){ ConstRef=true; }
        PassByRef=true;
      }
  
      //Store parameter
      _Md->StoreParameter(FunIndex,ParName,ParTypIndex,ConstRef,PassByRef,ParmNr,true,SrcInfo,SysMsgDispatcher::GetSourceLine());
      
      //Set argument string
      ParmStr+=(ParmStr.Length()!=0?",":"")+_Md->Types[ParTypIndex].Name;
  
      //Count parameter
      ParmNr++;
  
      //End of parameter list or continue ?
      if(Stn.Is(PrPunctuator::EndParen)){
        if(!Stn.Get(PrPunctuator::EndParen).Ok()){ 
          _Md->DeleteFunction(FunIndex);
          return false; 
        }
        break;
      }
      else if(Stn.Is(PrPunctuator::Comma)){
        if(!Stn.Get(PrPunctuator::Comma).Ok()){ 
          _Md->DeleteFunction(FunIndex);
          return false; 
        }
        continue;
      }
      else{
        if(Stn.TokensLeft()==0){
          SysMessage(284,Stn.FileName(),Stn.LineNr()).Print();
        }
        else{
          SysMessage(41,Stn.FileName(),Stn.LineNr(),Stn.CurrentColNr()).Print(ToString(Stn.CurrentColNr()));
        }
        _Md->DeleteFunction(FunIndex);
        return false;
      }
  
    }while(true);

  }

  //If we are creating a member function ensure that there is not a master method with same name
  if(Kind==FunctionKind::Member){
    if(_Md->MmtSearch(FunName,_Md->Types[_Md->CurrentSubScope().TypIndex].MstType,ParmStr,"",nullptr)!=-1){
      _Md->DeleteFunction(FunIndex);
      Stn.Msg(40).Print(_Md->CannonicalTypeName(_Md->CurrentSubScope().TypIndex),FunName);
      return false;
    }
  }

  //Function must not be already defined
  if(Kind==FunctionKind::Member){
    if((FoundFunIndex=_Md->FmbSearch(FunName,_Md->CurrentScope().ModIndex,_Md->CurrentSubScope().TypIndex,ParmStr,"",nullptr))!=-1){
      Stn.Msg(78).Print(FunName,ParmStr,_Md->CannonicalTypeName(_Md->CurrentSubScope().TypIndex));
      _Md->DeleteFunction(FunIndex);
      return false;
    }
  }
  else if(Kind==FunctionKind::Operator){
    if((FoundFunIndex=_Md->OprSearch(FunName,ParmStr,"",nullptr))!=-1){
      Stn.Msg(173).Print(FunName,ParmStr,_Md->Modules[_Md->Functions[FoundFunIndex].Scope.ModIndex].Name);
      _Md->DeleteFunction(FunIndex);
      return false;
    }
  }
  else{
    if((FoundFunIndex=_Md->FunSearch(FunName,_Md->CurrentScope().ModIndex,ParmStr,"",nullptr))!=-1){
      Stn.Msg(158).Print(FunName,ParmStr,_Md->Modules[_Md->Functions[FoundFunIndex].Scope.ModIndex].Name);
      _Md->DeleteFunction(FunIndex);
      return false;
    }
  }

  //If we have a dynamic library function we must check definition against library
  if(Kind==FunctionKind::DlFunc){
    if(!_Md->DlFunctionCheck(FunIndex,Stn.LastSrcInfo())){ return false; }
  }

  //Update function search index
  _Md->StoreFunctionSearchIndex(FunIndex);

  //Update function id
  _Md->StoreFunctionId(FunIndex);

  //Update corresponding linker symbol
  if(_Md->Functions[FunIndex].LnkSymIndex!=-1 && ParmNr!=0){
    _Md->Bin.UpdateLnkSymFunction(_Md->Functions[FunIndex].LnkSymIndex,ParmNr,
    _Md->Parameters[_Md->Functions[FunIndex].ParmLow].LnkSymIndex,_Md->Parameters[_Md->Functions[FunIndex].ParmHigh].LnkSymIndex);
  }

  //Update corresponding debug symbol
  if(_Md->Functions[FunIndex].DbgSymIndex!=-1 && ParmNr!=0){
    _Md->Bin.UpdateDbgSymFunction(_Md->Functions[FunIndex].DbgSymIndex,ParmNr,
    _Md->Parameters[_Md->Functions[FunIndex].ParmLow].DbgSymIndex,_Md->Parameters[_Md->Functions[FunIndex].ParmHigh].DbgSymIndex);
  }

  //Emit function declaration in assembler
  _Md->AsmFunDeclaration(FunIndex,false,false,"");

  //Return result
  return true;

}

//Function definition
//function <type> <id>([ref] <id>,[ref] <id>, ...):
//member <type> <type>.<id>([ref] <id>,[ref] <id>, ...):
//member <type>([ref] <id>,[ref] <id>, ...):
//operator <type> [op]([ref] <id>,[ref] <id>, ...):
bool Compiler::_CompileFuncDefn(Sentence& Stn,int& FunIndex){

  //Variables
  int i;
  int ModIndex;
  bool IsVoid;
  bool PassByRef;
  bool ConstRef;
  bool IsConstructor;
  int ParmStart;
  String Id;
  String PrClassName;
  String ClassName;
  String FunName;
  String ParName;
  String MsgFunName;
  int ClassTypIndex;
  int ResTypIndex;
  int ParTypIndex;
  int ParIndex;
  String ParmStr;
  Array<MasterData::ParameterTable> Parms;
  FunctionKind Kind;
  PrOperator Oper;

  //Default values
  IsConstructor=false;

  //Get First keyword
  if(Stn.Is(PrKeyword::Member)){
    if(!Stn.Get(PrKeyword::Member).Ok()){ return false; }
    if(Stn.TokensLeft()>=2 
    && Stn.Tokens[Stn.GetProcIndex()+0].Id()==PrTokenId::TypeName 
    && Stn.Tokens[Stn.GetProcIndex()+1].Id()==PrTokenId::Punctuator
    && Stn.Tokens[Stn.GetProcIndex()+1].Value.Pnc==PrPunctuator::BegParen){
      IsConstructor=true;
    }
    Kind=FunctionKind::Member;
  }
  else if(Stn.Is(PrKeyword::Operator)){
    if(!Stn.Get(PrKeyword::Operator).Ok()){ return false; }
    Kind=FunctionKind::Operator;
  }
  else{
    if(!Stn.Get(PrKeyword::Function).Ok()){ return false; }
    Kind=FunctionKind::Function;
  } 

  //Detect void functions / Get returning data type
  if(IsConstructor){
    if((ResTypIndex=_Md->TypSearch(Stn.Tokens[Stn.GetProcIndex()].Value.Typ,_Md->CurrentScope().ModIndex))==-1){
      Stn.Msg(299).Print(Stn.Tokens[Stn.GetProcIndex()].Value.Typ);
      return false;
    }
    IsVoid=false;
  }
  else if(Stn.Is(PrKeyword::Void)){
    Stn.Get(PrKeyword::Void);
    ResTypIndex=-1;
    IsVoid=true;
  }
  else{
    if(!ReadTypeSpec(_Md,Stn,_Md->CurrentScope(),ResTypIndex)){
      return false;
    }
    if(_Md->CurrentScope().Kind==ScopeKind::Public && _Md->Types[ResTypIndex].Scope.ModIndex!=_Md->CurrentScope().ModIndex && !_Md->Types[ResTypIndex].IsSystemDef){
      Stn.Msg(206).Print(_Md->CannonicalTypeName(ResTypIndex),_Md->Modules[_Md->Types[ResTypIndex].Scope.ModIndex].Name);
      return false;
    }
    IsVoid=false;
  }

  //Get class name and member name / operator / function name
  if(IsConstructor){
    if(!Stn.ReadTy(PrClassName).Get(PrPunctuator::BegParen).Ok()){ return false; }
    _Md->DecoupleTypeName(PrClassName,ClassName,ModIndex);
    if(ModIndex!=-1 && ModIndex!=_Md->CurrentScope().ModIndex){
      Stn.Msg(205).Print(PrClassName);
      return false;
    }
    if((ClassTypIndex=_Md->TypSearch(ClassName,_Md->CurrentScope().ModIndex))==-1){
      Stn.Msg(168).Print(ClassName);
      return false;
    }
    FunName=ClassName;
    MsgFunName="Constructor function "+FunName;
    Oper=(PrOperator)-1;
  }
  else if(Kind==FunctionKind::Member){
    if(!Stn.ReadTy(PrClassName).Get(PrOperator::Member).ReadId(FunName).Get(PrPunctuator::BegParen).Ok()){ return false; }
    _Md->DecoupleTypeName(PrClassName,ClassName,ModIndex);
    if(ModIndex!=-1 && ModIndex!=_Md->CurrentScope().ModIndex){
      Stn.Msg(205).Print(PrClassName);
      return false;
    }
    if((ClassTypIndex=_Md->TypSearch(ClassName,_Md->CurrentScope().ModIndex))==-1){
      Stn.Msg(168).Print(ClassName);
      return false;
    }
    MsgFunName="Member function "+ClassName+"."+FunName;
    Oper=(PrOperator)-1;
  }
  else if(Kind==FunctionKind::Operator){
    if(!Stn.Get(PrPunctuator::BegBracket).ReadOp(Oper).Get(PrPunctuator::EndBracket).Get(PrPunctuator::BegParen).Ok()){ return false; }
    FunName=_Md->GetOperatorFuncName(Stn.Tokens[Stn.GetProcIndex()-3].Text());
    MsgFunName="Operator function "+FunName;
    ClassTypIndex=-1;
  }
  else{
    if(!Stn.ReadId(FunName).Get(PrPunctuator::BegParen).Ok()){ return false; }
    MsgFunName="Function "+FunName;
    ClassTypIndex=-1;
    Oper=(PrOperator)-1;
  }

  //Function without parameters
  if(Stn.Is(PrPunctuator::EndParen)){ 
    if(IsConstructor){
      Stn.Msg(300).Print(FunName);
      return false;
    }
    if(!Stn.Get(PrPunctuator::EndParen).Get(PrPunctuator::Colon).Ok()){ return false; }
    ParmStr="";
  }

  //Function with parameters
  else{

    //Parse parameter string
    ParmStr="";
    do{
      if(Stn.Is(PrKeyword::Ref)){ PassByRef=true; Stn.Get(PrKeyword::Ref); } else{ PassByRef=false; }
      if(!ReadTypeSpec(_Md,Stn,_Md->CurrentScope(),ParTypIndex)){ 
        return false; 
      }
      ConstRef=false;
      if(_Md->Types[ParTypIndex].MstType==MasterType::Class 
      || _Md->Types[ParTypIndex].MstType==MasterType::String 
      || _Md->Types[ParTypIndex].MstType==MasterType::DynArray
      || _Md->Types[ParTypIndex].MstType==MasterType::FixArray){
        if(!PassByRef){ ConstRef=true; }
        PassByRef=true;
      }
      if(_Md->CurrentScope().Kind==ScopeKind::Public && _Md->Types[ParTypIndex].Scope.ModIndex!=_Md->CurrentScope().ModIndex && !_Md->Types[ParTypIndex].IsSystemDef){
        Stn.Msg(206).Print(_Md->CannonicalTypeName(ParTypIndex),_Md->Modules[_Md->Types[ParTypIndex].Scope.ModIndex].Name);
        return false;
      }
      if(!Stn.ReadId(ParName).Ok()){ return false; }
      Parms.Add((MasterData::ParameterTable){ParName,ParTypIndex,ConstRef,PassByRef,Parms.Length(),-1,-1});
      ParmStr+=(ParmStr.Length()!=0?",":"")+_Md->Types[ParTypIndex].Name;
      if(Stn.Is(PrPunctuator::EndParen)){
        if(!Stn.Get(PrPunctuator::EndParen).Get(PrPunctuator::Colon).Ok()){ return false; }
        break;
      }
      else if(Stn.Is(PrPunctuator::Comma)){
        if(!Stn.Get(PrPunctuator::Comma).Ok()){ return false; }
        continue;
      }
      else{
        if(Stn.TokensLeft()==0){
          SysMessage(284,Stn.FileName(),Stn.LineNr()).Print();
        }
        else{
          SysMessage(41,Stn.FileName(),Stn.LineNr(),Stn.CurrentColNr()).Print(ToString(Stn.CurrentColNr()));
        }
        return false;
      }
    }while(true);

  }

  //Message function name
  MsgFunName+="("+ParmStr+")";

  //Function must be already defined
  if(Kind==FunctionKind::Operator){
    if((FunIndex=_Md->OprSearch(FunName,ParmStr,"",nullptr))==-1){
      if(_Md->CurrentScope().Kind==ScopeKind::Local){
        Stn.Msg(432).Print(MsgFunName);
      }
      else{
        Stn.Msg(42).Print(MsgFunName);
      }
      return false;
    }
  }
  else if(Kind==FunctionKind::Member){
    if((FunIndex=_Md->FmbSearch(FunName,_Md->CurrentScope().ModIndex,ClassTypIndex,ParmStr,"",nullptr))==-1){
      if(_Md->Types[ClassTypIndex].Scope.Kind==ScopeKind::Local){
        Stn.Msg(432).Print(MsgFunName);
      }
      else{
        Stn.Msg(42).Print(MsgFunName);
      }
      return false;
    }
  }
  else{
    if((FunIndex=_Md->FunSearch(FunName,_Md->CurrentScope().ModIndex,ParmStr,"",nullptr))==-1){
      if(_Md->CurrentScope().Kind==ScopeKind::Local){
        Stn.Msg(432).Print(MsgFunName);
      }
      else{
        Stn.Msg(42).Print(MsgFunName);
      }
      return false;
    }
  }

  //Check function is not defined already
  if(_Md->Functions[FunIndex].IsDefined){
    Stn.Msg(328).Print(MsgFunName);
    return false;
  }

  //Check function returning type matches
  if(ResTypIndex!=_Md->Functions[FunIndex].TypIndex || IsVoid!=_Md->Functions[FunIndex].IsVoid){
    Stn.Msg(33).Print(MsgFunName);
    return false;
  }

  //Determine parenthensys parameter start (we must skip function result and reference to self)
  ParmStart=0;
  if(!IsVoid){ ParmStart++; }
  if(ClassTypIndex!=-1){ ParmStart++; }

  //Parameter check loop
  for(i=0;i<Parms.Length();i++){

    //Check parameter exists
    if((ParIndex=_Md->ParSearch(Parms[i].Name,FunIndex))==-1){
      Stn.Msg(43).Print(MsgFunName,Parms[i].Name);
      return false;
    }

    //Check parameter order
    if(_Md->Parameters[ParIndex].ParmOrder-ParmStart!=i){
      Stn.Msg(44).Print(MsgFunName,Parms[i].Name);
      return false;
    }

    //Check parameter type matches defined one
    if(Parms[i].TypIndex!=_Md->Parameters[ParIndex].TypIndex){
      Stn.Msg(34).Print(MsgFunName,Parms[i].Name);
      return false;
    }

    //Check pass by reference
    if(Parms[i].IsReference!=_Md->Parameters[ParIndex].IsReference || Parms[i].IsConst!=_Md->Parameters[ParIndex].IsConst){
      if(_Md->Parameters[ParIndex].IsReference && !_Md->Parameters[ParIndex].IsConst){
        Stn.Msg(46).Print(MsgFunName,ParName);
      }
      else{
        Stn.Msg(47).Print(MsgFunName,ParName);
      }
      return false;
    }

  }

  //Emit forward jump for nested functions
  //Assembler level is changed as the jump instruction belongs to the parent function
  if(_Md->Functions[FunIndex].IsNested){
    if(!_Md->Bin.AsmSetParentNestId()){ Stn.Msg(563).Print(); return false; }
    _Md->Bin.AsmOutNewLine(AsmSection::Body);
    _Md->Bin.AsmOutCommentLine(AsmSection::Body,"Jump local function code",true);
    if(!_Md->Bin.AsmWriteCode(CpuInstCode::JMP,_Md->AsmJmp(Stn.GetLabel(CodeLabelId::NextBlock)))){ return false; }
    _Md->Bin.AsmOutNewLine(AsmSection::Body);
    _Md->Bin.AsmOutCommentLine(AsmSection::Body,"(...) Code of function "+_Md->Functions[FunIndex].FullName,true);
    _Md->Bin.AsmRestoreCurrentNestId();
  }

  //Set function address and update symbols
  _Md->Functions[FunIndex].Address=_Md->Bin.CurrentCodeAddress();
  _Md->Bin.StoreFunctionAddress(_Md->Functions[FunIndex].Fid,_Md->Functions[FunIndex].FullName,_Md->Functions[FunIndex].Scope.Depth,_Md->Functions[FunIndex].Address);
  if(_Md->Functions[FunIndex].LnkSymIndex!=-1){
    _Md->Bin.UpdateLnkSymFunctionAddress(_Md->Functions[FunIndex].LnkSymIndex,_Md->Functions[FunIndex].Address);
  }
  if(_Md->Functions[FunIndex].DbgSymIndex!=-1){
    _Md->Bin.UpdateDbgSymFunctionBegAddress(_Md->Functions[FunIndex].DbgSymIndex,_Md->Functions[FunIndex].Address);
  }

  //Set function defined flag
  _Md->Functions[FunIndex].IsDefined=true;

  //Return result
  return true;

}

//Compile endfunction statement
//:endfuction
bool Compiler::_CompileEndFunction(Sentence& Stn,SentenceId LastStnId){

  //Parse sentence
  if(!Stn.Get(PrKeyword::EndFunction).Ok()){ 
    return false; 
  }

  //Check current scope is a function
  if(_Md->CurrentScope().Kind!=ScopeKind::Local){
    Stn.Tokens[0].Msg(202).Print();
    return false;
  }
  if(_Md->Functions[_Md->CurrentScope().FunIndex].Kind!=FunctionKind::Function){
    Stn.Tokens[0].Msg(202).Print();
    return false;
  }

  //Return statement did not happened before end function
  if(LastStnId!=SentenceId::Return){

    //Return statement before is mandatory
    if(!_Md->Functions[_Md->CurrentScope().FunIndex].IsVoid){
      Stn.Tokens[0].Msg(119).Print();
      return false;
    }
  
    //Emit return statement if fuction is void
    _Md->Bin.AsmOutNewLine(AsmSection::Body);
    _Md->Bin.AsmOutCommentLine(AsmSection::Body,"Return from function",true);
    if(_Md->ParentScope().Kind==ScopeKind::Local){
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::RETN)){ return false; }
    }
    else{
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::RET)){ return false; }
    }

  }
  
  //Store jump destination for nested functions
  //We store destination at current depth -1 because corresponding origin is emitted at that level
  if(_Md->ParentScope().Kind==ScopeKind::Local){
    _Md->Bin.StoreJumpDestination(Stn.GetLabel(CodeLabelId::NextBlock),_Md->CurrentScope().Depth-1,_Md->Bin.CurrentCodeAddress());
  }

  //Update function debug symbol
  if(_Md->Functions[_Md->CurrentScope().FunIndex].DbgSymIndex!=-1){
    _Md->Bin.UpdateDbgSymFunctionEndAddress(_Md->Functions[_Md->CurrentScope().FunIndex].DbgSymIndex,_Md->Bin.CurrentCodeAddress()-1);
  }

  //Return result
  return true;

}

//Compile endmember statement
//:endmember
bool Compiler::_CompileEndMember(Sentence& Stn,SentenceId LastStnId){

  //Parse sentence
  if(!Stn.Get(PrKeyword::EndMember).Ok()){ 
    return false; 
  }

  //Check current scope is a function member
  if(_Md->CurrentScope().Kind!=ScopeKind::Local){
    Stn.Tokens[0].Msg(203).Print();
    return false;
  }
  if(_Md->Functions[_Md->CurrentScope().FunIndex].Kind!=FunctionKind::Member){
    Stn.Tokens[0].Msg(203).Print();
    return false;
  }

  //Return statement did not happened before end member
  if(LastStnId!=SentenceId::Return){

    //Return statement before is mandatory
    if(!_Md->Functions[_Md->CurrentScope().FunIndex].IsVoid){
      Stn.Tokens[0].Msg(169).Print();
      return false;
    }
  
    //Emit return statement if member is void
    _Md->Bin.AsmOutNewLine(AsmSection::Body);
    _Md->Bin.AsmOutCommentLine(AsmSection::Body,"Return from function member",true);
    if(_Md->ParentScope().Kind==ScopeKind::Local){
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::RETN)){ return false; }
    }
    else{
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::RET)){ return false; }
    }

  }
  
  //Store jump destination for nested functions
  //We store destination at current depth -1 because corresponding origin is emitted at that level
  if(_Md->ParentScope().Kind==ScopeKind::Local){
    _Md->Bin.StoreJumpDestination(Stn.GetLabel(CodeLabelId::NextBlock),_Md->CurrentScope().Depth-1,_Md->Bin.CurrentCodeAddress());
  }

  //Update function debug symbol
  if(_Md->Functions[_Md->CurrentScope().FunIndex].DbgSymIndex!=-1){
    _Md->Bin.UpdateDbgSymFunctionEndAddress(_Md->Functions[_Md->CurrentScope().FunIndex].DbgSymIndex,_Md->Bin.CurrentCodeAddress()-1);
  }

  //Return result
  return true;

}

//Compile endoperator statement
//:endoperator
bool Compiler::_CompileEndOperator(Sentence& Stn,SentenceId LastStnId){

  //Parse sentence
  if(!Stn.Get(PrKeyword::EndOperator).Ok()){ 
    return false; 
  }

  //Check current scope is a function operator
  if(_Md->CurrentScope().Kind!=ScopeKind::Local){
    Stn.Tokens[0].Msg(204).Print();
    return false;
  }
  if(_Md->Functions[_Md->CurrentScope().FunIndex].Kind!=FunctionKind::Operator){
    Stn.Tokens[0].Msg(204).Print();
    return false;
  }

  //Return statement did not happened before end member
  if(LastStnId!=SentenceId::Return){

    //Return statement before is mandatory
    if(!_Md->Functions[_Md->CurrentScope().FunIndex].IsVoid){
      Stn.Tokens[0].Msg(174).Print();
      return false;
    }
  
    //Emit return statement if fuction is void
    _Md->Bin.AsmOutNewLine(AsmSection::Body);
    _Md->Bin.AsmOutCommentLine(AsmSection::Body,"Return from operator function",true);
    if(_Md->ParentScope().Kind==ScopeKind::Local){
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::RETN)){ return false; }
      _Md->Bin.StoreJumpDestination(Stn.GetLabel(CodeLabelId::NextBlock),_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());
    }
    else{
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::RET)){ return false; }
    }

  }
  
  //Store jump destination for nested functions
  //We store destination at current depth -1 because corresponding origin is emitted at that level
  if(_Md->ParentScope().Kind==ScopeKind::Local){
    _Md->Bin.StoreJumpDestination(Stn.GetLabel(CodeLabelId::NextBlock),_Md->CurrentScope().Depth-1,_Md->Bin.CurrentCodeAddress());
  }

  //Update function debug symbol
  if(_Md->Functions[_Md->CurrentScope().FunIndex].DbgSymIndex!=-1){
    _Md->Bin.UpdateDbgSymFunctionEndAddress(_Md->Functions[_Md->CurrentScope().FunIndex].DbgSymIndex,_Md->Bin.CurrentCodeAddress()-1);
  }

  //Return result
  return true;

}

//Main program
//main:
bool Compiler::_CompileMain(Sentence& Stn,const String& ProgName,bool CompileToLibrary){

  //Variables
  int FunIndex;
  int StkIndex;
  ScopeDef Scope;
  SourceInfo SrcInfo;

  //This sentence is not allowed when compiling libraries
  if(CompileToLibrary){
    Stn.Tokens[0].Msg(20).Print();
    return false;
  }

  //Check syntax
  if(!Stn.Get(PrKeyword::Main).Ok()){ 
    return false; 
  }
  SrcInfo=Stn.LastSrcInfo();

  //Program statement only allowed on main module
  if(_Md->CurrentScope().ModIndex!=_Md->MainModIndex()){ 
    Stn.Tokens[0].Msg(134).Print();
    return false; 
  }

  //Store new function (always on public scope since main() it is called from start code which is in scope public, this is necessary for the forward call mechanism)
  Scope=(ScopeDef){ScopeKind::Public,_Md->MainModIndex(),-1,0};
  _Md->StoreRegularFunction(Scope,ProgName,-1,true,false,SrcInfo,SysMsgDispatcher::GetSourceLine());
  FunIndex=_Md->Functions.Length()-1;

  //Find index of scope
  if((StkIndex=_Md->ScopeFindIndex(Scope))==-1){
    Stn.Tokens[0].Msg(96).Print(_Md->ScopeDescription(Scope));
    return false;
  }

  //Update search index and function id
  _Md->StoreFunctionSearchIndex(FunIndex,StkIndex);
  _Md->StoreFunctionId(FunIndex);

  //Set function address and update symbol
  _Md->Functions[FunIndex].Address=_Md->Bin.CurrentCodeAddress();
  _Md->Bin.StoreFunctionAddress(_Md->Functions[FunIndex].Fid,_Md->Functions[FunIndex].FullName,_Md->Functions[FunIndex].Scope.Depth,_Md->Functions[FunIndex].Address);

  //Update symbols
  if(_Md->Functions[FunIndex].LnkSymIndex!=-1){
    _Md->Bin.UpdateLnkSymFunctionAddress(_Md->Functions[FunIndex].LnkSymIndex,_Md->Functions[FunIndex].Address);
  }
  if(_Md->Functions[FunIndex].DbgSymIndex!=-1){
    _Md->Bin.UpdateDbgSymFunctionBegAddress(_Md->Functions[FunIndex].DbgSymIndex,_Md->Functions[FunIndex].Address);
  }

  //Set main function as defined
  _Md->Functions[FunIndex].IsDefined=true;

  //Return result
  return true;

}

//Compile endmain statement
//:endmain
bool Compiler::_CompileEndMain(Sentence& Stn,const String& ProgName,SentenceId LastStnId){

  //Parse sentence
  if(!Stn.Get(PrKeyword::EndMain).Ok()){ 
    return false; 
  }
  
  //Check current scope is main function
  if(_Md->CurrentScope().Kind!=ScopeKind::Local){
    Stn.Tokens[0].Msg(390).Print();
    return false;
  }
  if(_Md->Functions[_Md->CurrentScope().FunIndex].Name!=ProgName){
    Stn.Tokens[0].Msg(390).Print();
    return false;
  }

  //Emit system call for program exit if last sentence was not a return statement
  if(LastStnId!=SentenceId::Return){
    _Md->Bin.AsmOutNewLine(AsmSection::Body);
    _Md->Bin.AsmOutCommentLine(AsmSection::Body,"Return from function",true);
    if(!_Md->Bin.AsmWriteCode(CpuInstCode::RET)){ return false; }
  }

  //Update function debug symbol
  if(_Md->Functions[_Md->CurrentScope().FunIndex].DbgSymIndex!=-1){
    _Md->Bin.UpdateDbgSymFunctionEndAddress(_Md->Functions[_Md->CurrentScope().FunIndex].DbgSymIndex,_Md->Bin.CurrentCodeAddress()-1);
  }

  //Return result
  return true;

}

//Compile return
//return [<expr>]
bool Compiler::_CompileReturn(Sentence& Stn,const String& ProgName){

  //Variables
  int Begin;
  int End;
  bool HasRetValue;
  Expression Expr;
  ExprToken ResultVal;
  ExprToken ResultVar;
  Sentence RetStn;

  //Parse sentence
  if(Stn.Tokens.Length()==1){
    if(!Stn.Get(PrKeyword::Return).Ok()){ 
      return false;
    }
    HasRetValue=false;
  }
  else{
    if(!Stn.Get(PrKeyword::Return).ReadEx(Begin,End).Ok()){ 
      return false;
    }
    HasRetValue=true;
  }

  //Check return value is permitted
  if(_Md->Functions[_Md->CurrentScope().FunIndex].IsVoid && HasRetValue){
    Stn.Tokens[0].Msg(117).Print();
    return false;
  }
  else if(!_Md->Functions[_Md->CurrentScope().FunIndex].IsVoid && !HasRetValue){
    Stn.Tokens[0].Msg(118).Print();
    return false;
  }

  //Calculate result value
  if(HasRetValue){
    int RetIndex;
    if((RetIndex=_Md->VarSearch(_Md->GetFuncResultName(),_Md->CurrentScope().ModIndex))==-1){ Stn.Tokens[0].Msg(375).Print(); return false; }
    ResultVar.ThisInd(_Md,RetIndex,Stn.Tokens[0].SrcInfo());
    if(!Expr.Compile(_Md,_Md->CurrentScope(),Stn,Begin,End,ResultVal)){ return false; }
    if(!Expression().CopyOperand(_Md,ResultVar,ResultVal)){ return false; } 
  }

  //Return instruction
  if(_Md->Functions[_Md->CurrentScope().FunIndex].Name==ProgName){
    if(!_Md->Bin.AsmWriteCode(CpuInstCode::SCALL,_Md->Bin.AsmLitInt((int)SystemCall::ProgramExit))){ return false; }
  }
  else if(_Md->ParentScope().Kind==ScopeKind::Local){
    if(!_Md->Bin.AsmWriteCode(CpuInstCode::RETN)){ return false; }
  }
  else{
    if(!_Md->Bin.AsmWriteCode(CpuInstCode::RET)){ return false; }
  }
  
  //Return code
  return true;

}

//Compile if statement
//if(<expr>):
bool Compiler::_CompileIf(Sentence& Stn){

  //Variables
  int Start;
  int End;
  Expression Expr;
  ExprToken Result;

  //Parse sentence
  if(!Stn.Get(PrKeyword::If).Get(PrPunctuator::BegParen).ReadEx(PrPunctuator::EndParen,Start,End).Get(PrPunctuator::EndParen).Get(PrPunctuator::Colon).Ok()){ 
    return false; 
  }
  
  //Compile condition expression
  if(!Expr.Compile(_Md,_Md->CurrentScope(),Stn,Start,End,Result)){ return false; }

  //Expression must be evaluated to boolean
  if(Result.MstType()!=MasterType::Boolean){
    Stn.Tokens[Start].Msg(106).Print();
    return false;
  }

  //Emit instruction
  if(!_Md->Bin.AsmWriteCode(CpuInstCode::JMPFL,Result.Asm(),_Md->AsmJmp(Stn.GetLabel(CodeLabelId::NextCond)))){
    return false;
  }

  //Return result
  return true;

}

//Compile elseif statement
//elseif(<expr>):
bool Compiler::_CompileElseIf(Sentence& Stn){

  //Variables
  int Start;
  int End;
  Expression Expr;
  ExprToken Result;

  //Parse sentence
  if(!Stn.Get(PrKeyword::ElseIf).Get(PrPunctuator::BegParen).ReadEx(PrPunctuator::EndParen,Start,End).Get(PrPunctuator::EndParen).Get(PrPunctuator::Colon).Ok()){ 
    return false; 
  }
  
  //Exit jump of previous condition
  if(!_Md->Bin.AsmWriteCode(CpuInstCode::JMP,_Md->AsmJmp(Stn.GetLabel(CodeLabelId::Exit)))){
    return false;
  }

  //Set destination label from previous case
  _Md->Bin.StoreJumpDestination(Stn.GetLabel(CodeLabelId::CurrCond),_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());

  //Compile condition expression
  if(!Expr.Compile(_Md,_Md->CurrentScope(),Stn,Start,End,Result)){ return false; }

  //Expression must be evaluated to boolean
  if(Result.MstType()!=MasterType::Boolean){
    Stn.Tokens[Start].Msg(106).Print();
    return false;
  }

  //Emit instruction
  if(!_Md->Bin.AsmWriteCode(CpuInstCode::JMPFL,Result.Asm(),_Md->AsmJmp(Stn.GetLabel(CodeLabelId::NextCond)))){
    return false;
  }

  //Return result
  return true;

}

//Compile else statement
//else:
bool Compiler::_CompileElse(Sentence& Stn){

  //Parse sentence
  if(!Stn.Get(PrKeyword::Else).Get(PrPunctuator::Colon).Ok()){ 
    return false; 
  }
  
  //Exit jump of previous condition
  if(!_Md->Bin.AsmWriteCode(CpuInstCode::JMP,_Md->AsmJmp(Stn.GetLabel(CodeLabelId::Exit)))){
    return false;
  }

  //Record jump destination
  _Md->Bin.StoreJumpDestination(Stn.GetLabel(CodeLabelId::CurrCond),_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());
  
  //Return result
  return true;

}

//Compile endif statement
//:endif
bool Compiler::_CompileEndIf(Sentence& Stn){

  //Parse sentence
  if(!Stn.Get(PrKeyword::EndIf).Ok()){ 
    return false; 
  }
  
  //Record jump destination
  //(If always jumps to a condition label, but endif always stores destination to an exit label, when we have an if/endif block we need to store a condition label as destination)
  _Md->Bin.StoreJumpDestination(Stn.GetLabel(CodeLabelId::NextCond),_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());
  _Md->Bin.StoreJumpDestination(Stn.GetLabel(CodeLabelId::Exit),_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());

  //Return result
  return true;

}

//Compile while statement
//while(<expr>):
bool Compiler::_CompileWhile(Sentence& Stn){

  //Variables
  int Start;
  int End;
  Expression Expr;
  ExprToken Result;

  //Parse sentence
  if(!Stn.Get(PrKeyword::While).Get(PrPunctuator::BegParen).ReadEx(PrPunctuator::EndParen,Start,End).Get(PrPunctuator::EndParen).Get(PrPunctuator::Colon).Ok()){ 
    return false; 
  }
  
  //Record jump destination for loop begin
  _Md->Bin.StoreJumpDestination(Stn.GetLabel(CodeLabelId::LoopBeg),_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());

  //Compile condition expression
  if(!Expr.Compile(_Md,_Md->CurrentScope(),Stn,Start,End,Result)){ return false; }

  //Expression must be evaluated to boolean
  if(Result.MstType()!=MasterType::Boolean){
    Stn.Tokens[Start].Msg(106).Print();
    return false;
  }

  //Emit instruction
  if(!_Md->Bin.AsmWriteCode(CpuInstCode::JMPFL,Result.Asm(),_Md->AsmJmp(Stn.GetLabel(CodeLabelId::Exit)))){
    return false;
  }

  //Return result
  return true;

}

//Compile endwhile statement
//:endwhile
bool Compiler::_CompileEndWhile(Sentence& Stn){

  //Parse sentence
  if(!Stn.Get(PrKeyword::EndWhile).Ok()){ 
    return false; 
  }
  
  //Record jump destination for loop end
  _Md->Bin.StoreJumpDestination(Stn.GetLabel(CodeLabelId::LoopEnd),_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());
  
  //Emit instruction
  if(!_Md->Bin.AsmWriteCode(CpuInstCode::JMP,_Md->AsmJmp(Stn.GetLabel(CodeLabelId::LoopBeg)))){
    return false;
  }

  //Record jump destination for loop exit
  _Md->Bin.StoreJumpDestination(Stn.GetLabel(CodeLabelId::Exit),_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());
  
  //Return result
  return true;

}

//Compile do statement
//do:
bool Compiler::_CompileDo(Sentence& Stn){

  //Parse sentence
  if(!Stn.Get(PrKeyword::Do).Get(PrPunctuator::Colon).Ok()){ 
    return false; 
  }
  
  //Record active jump destination for loop begin
  _Md->Bin.StoreJumpDestination(Stn.GetLabel(CodeLabelId::LoopBeg),_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());
  
  //Return result
  return true;

}

//Compile Loop statement
//:loop(<expr>)
bool Compiler::_CompileLoop(Sentence& Stn){

  //Variables
  int Start;
  int End;
  Expression Expr;
  ExprToken Result;

  //Parse sentence
  if(!Stn.Get(PrKeyword::Loop).Get(PrPunctuator::BegParen).ReadEx(PrPunctuator::EndParen,Start,End).Get(PrPunctuator::EndParen).Ok()){ 
    return false; 
  }
  
  //Record active jump destination for loop end
  _Md->Bin.StoreJumpDestination(Stn.GetLabel(CodeLabelId::LoopEnd),_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());

  //Compile condition expression
  if(!Expr.Compile(_Md,_Md->CurrentScope(),Stn,Start,End,Result)){ return false; }

  //Expression must be evaluated to boolean
  if(Result.MstType()!=MasterType::Boolean){
    Stn.Tokens[Start].Msg(106).Print();
    return false;
  }

  //Emit instruction
  if(!_Md->Bin.AsmWriteCode(CpuInstCode::JMPTR,Result.Asm(),_Md->AsmJmp(Stn.GetLabel(CodeLabelId::LoopBeg)))){
    return false;
  }

  //Record jump destination
  _Md->Bin.StoreJumpDestination(Stn.GetLabel(CodeLabelId::Exit),_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());
  
  //Return result
  return true;

}

//Compile For statement
//for(<lvalue>=<expr>:<whileexpr>:<stepexpr>):
bool Compiler::_CompileFor(Sentence& Stn,Stack<Sentence>& ForStep){

  //Variables
  int Init[2];
  int While[2];
  int Step[2];
  Expression Expr;
  ExprToken Result;
  Sentence CondStn;

  //Parse sentence
  if(!Stn.Get(PrKeyword::For)
  .Get(PrPunctuator::BegParen)
  .ReadEx(PrKeyword::If,Init[0],Init[1])
  .Get(PrKeyword::If)
  .ReadEx(PrKeyword::Do,While[0],While[1])
  .Get(PrKeyword::Do)
  .ReadEx(PrPunctuator::EndParen,Step[0],Step[1])
  .Get(PrPunctuator::EndParen)
  .Get(PrPunctuator::Colon)
  .Ok()){ 
    return false; 
  }

  //Push step sentence
  ForStep.Push(Stn.SubSentence(Step[0],Step[1]));

  //Compile loop initialization
  if(!Expr.Compile(_Md,_Md->CurrentScope(),Stn,Init[0],Init[1],Result)){ return false; }

  //Record jump destination for loop begin
  _Md->Bin.StoreJumpDestination(Stn.GetLabel(CodeLabelId::LoopBeg),_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());

  //Compile condition expression
  CondStn=Stn.SubSentence(While[0],While[1]);
  if(!Expr.Compile(_Md,_Md->CurrentScope(),CondStn,Result)){ return false; }
  
  //Conditional sentence must be evaluated to boolean
  if(Result.MstType()!=MasterType::Boolean){
    Stn.Tokens[Init[0]].Msg(109).Print();
    return false;
  }

  //Emit instruction
  if(!_Md->Bin.AsmWriteCode(CpuInstCode::JMPFL,Result.Asm(),_Md->AsmJmp(Stn.GetLabel(CodeLabelId::Exit)))){
    return false;
  }

  //Return result
  return true;

}

//Compile EndFor
//:endfor
bool Compiler::_CompileEndFor(Sentence& Stn,Stack<Sentence>& ForStep){

  //Variables
  Expression Expr;
  Sentence StepStn;

  //Parse sentence
  if(!Stn.Get(PrKeyword::EndFor).Ok()){ 
    return false; 
  }

  //Get step sentence
  if(ForStep.Length()==0){
    Stn.Tokens[0].Msg(110).Print();
    return false;
  }
  StepStn=ForStep.Pop();

  //Record jump destination for loop end
  _Md->Bin.StoreJumpDestination(Stn.GetLabel(CodeLabelId::LoopEnd),_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());

  //Compile step sentence
  if(!Expr.Compile(_Md,_Md->CurrentScope(),StepStn)){ return false; }

  //Emit instruction
  if(!_Md->Bin.AsmWriteCode(CpuInstCode::JMP,_Md->AsmJmp(Stn.GetLabel(CodeLabelId::LoopBeg)))){
    return false;
  }

  //Record jump destination for loop exit
  _Md->Bin.StoreJumpDestination(Stn.GetLabel(CodeLabelId::Exit),_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());

  //Return result
  return true;

}

//Compile Walk statement
//walk(<arraylvalue> on <element> [index <indexvar>] [if <condition>]):
bool Compiler::_CompileWalk(Sentence& Stn,Stack<ExprToken>& WalkArray){

  //Variables
  int ArrExpr[2];
  int IfExpr[2];
  int ElemTypIndex;
  int OnVarIndex;
  int IxVarIndex;
  bool HasIndex;
  bool HasIfExpr;
  String OnVarName;
  String IxVarName;
  String FoundObject;
  ExprToken ArrToken;
  PrToken OnVarToken;
  PrToken IxVarToken;
  Expression Expr;
  ExprToken Result;

  //Parse sentence
  HasIndex=false;
  HasIfExpr=false;
  if(!Stn.Get(PrKeyword::Walk).Get(PrPunctuator::BegParen).ReadEx(PrKeyword::On,ArrExpr[0],ArrExpr[1]).Get(PrKeyword::On).ReadId(OnVarName).Ok()){ 
    return false; 
  }
  OnVarToken=Stn.Tokens[ArrExpr[1]+2];
  if(Stn.Is(PrKeyword::Index)){
    if(!Stn.Get(PrKeyword::Index).ReadId(IxVarName).Ok()){ 
      return false; 
    }
    HasIndex=true;
    IxVarToken=Stn.Tokens[Stn.GetProcIndex()-1];
  }
  if(Stn.Is(PrKeyword::If)){
    if(!Stn.Get(PrKeyword::If).ReadEx(PrPunctuator::EndParen,IfExpr[0],IfExpr[1]).Ok()){ 
      return false; 
    }
    HasIfExpr=true;
  }
  if(!Stn.Get(PrPunctuator::EndParen).Get(PrPunctuator::Colon).Ok()){ 
    return false;
  }

  //Compile array expression
  if(!Expr.Compile(_Md,_Md->CurrentScope(),Stn,ArrExpr[0],ArrExpr[1],ArrToken)){ return false; }

  //Check array expression results in array, lvalue and 1-dimensional
  if(ArrToken.MstType()!=MasterType::FixArray && ArrToken.MstType()!=MasterType::DynArray){
    Stn.Tokens[ArrExpr[0]].Msg(497).Print();
    return false;
  }
  if(_Md->Types[ArrToken.TypIndex()].DimNr!=1){
    Stn.Tokens[ArrExpr[0]].Msg(523).Print();
    return false;
  }

  //Push walk array
  WalkArray.Push(ArrToken);

  //Set array token as used
  ArrToken.SetSourceUsed(_Md->CurrentScope(),true);

  //Get element type index
  ElemTypIndex=_Md->Types[ArrToken.TypIndex()].ElemTypIndex;

  //On variable must not be already defined
  if(_Md->VarSearch(OnVarName,_Md->CurrentScope().ModIndex)!=-1){
    OnVarToken.Msg(524).Print(OnVarName);
    return false;
  }

  //Dot collission check
  if(_Md->Types[ElemTypIndex].MstType==MasterType::Class || _Md->Types[ElemTypIndex].MstType==MasterType::Enum){
    if(!_Md->DotCollissionCheck(OnVarName,FoundObject)){
      OnVarToken.Msg(525).Print(OnVarName,FoundObject);
      return false;
    }
  }

  //Store new variable for on variable
  if(!_Md->ReuseVariable(_Md->CurrentScope(),Stn.GetCodeBlockId(),-1,OnVarName,ElemTypIndex,false,false,true,OnVarToken.SrcInfo(),SysMsgDispatcher::GetSourceLine(),OnVarIndex)){ return false; }
  if(OnVarIndex==-1){
    _Md->CleanHidden(_Md->CurrentScope(),OnVarName);
    _Md->StoreVariable(_Md->CurrentScope(),Stn.GetCodeBlockId(),-1,OnVarName,ElemTypIndex,false,false,false,true,false,false,false,true,true,OnVarToken.SrcInfo(),SysMsgDispatcher::GetSourceLine());
    OnVarIndex=_Md->Variables.Length()-1;
    _Md->Bin.AsmOutNewLine(AsmSection::Decl);
    _Md->Bin.AsmOutCommentLine(AsmSection::Decl,"Declared from walk sentence",true);
    _Md->Bin.AsmOutVarDecl(AsmSection::Decl,(_Md->CurrentScope().Kind!=ScopeKind::Local?true:false),false,true,false,false,OnVarName,
    _Md->CpuDataTypeFromMstType(_Md->Types[ElemTypIndex].MstType),_Md->VarLength(OnVarIndex),_Md->Variables[OnVarIndex].Address,"",false,false,"","");
  }

  //Set on variable as initialized (as no initialization is required)
  _Md->Variables[OnVarIndex].IsInitialized=true;
  DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[OnVarIndex].Name+" in scope "+_Md->ScopeName(_Md->Variables[OnVarIndex].Scope));

  //Use / declare index variable
  if(HasIndex){

    //Use variable if it is declared
    if((IxVarIndex=_Md->VarSearch(IxVarName,_Md->CurrentScope().ModIndex))!=-1){

      //Variable must be regular variable not constant
      if(_Md->Variables[IxVarIndex].IsConst || _Md->Variables[IxVarIndex].IsTempVar || _Md->Variables[IxVarIndex].IsReference){
        IxVarToken.Msg(526).Print(); 
        return false; 
      }

      //Variable must be word master type
      if(GetArchitecture()==64 && _Md->Types[_Md->Variables[IxVarIndex].TypIndex].MstType!=MasterType::Long){
        IxVarToken.Msg(527).Print(_Md->CannonicalTypeName(_Md->LonTypIndex)); 
        return false; 
      } 
      if(GetArchitecture()==32 && _Md->Types[_Md->Variables[IxVarIndex].TypIndex].MstType!=MasterType::Integer){
        IxVarToken.Msg(527).Print(_Md->CannonicalTypeName(_Md->IntTypIndex)); 
        return false; 
      } 

    }
    
    //Define variable if it is undeclared
    else{

      //Store new variable
      if(!_Md->ReuseVariable(_Md->CurrentScope(),Stn.GetCodeBlockId(),-1,IxVarName,_Md->WrdTypIndex,false,false,false,IxVarToken.SrcInfo(),SysMsgDispatcher::GetSourceLine(),IxVarIndex)){ return false; }
      if(IxVarIndex==-1){
        _Md->CleanHidden(_Md->CurrentScope(),IxVarName);
        _Md->StoreVariable(_Md->CurrentScope(),Stn.GetCodeBlockId(),-1,IxVarName,_Md->WrdTypIndex,false,false,false,false,false,false,false,true,true,IxVarToken.SrcInfo(),SysMsgDispatcher::GetSourceLine());
        IxVarIndex=_Md->Variables.Length()-1;
        _Md->Bin.AsmOutNewLine(AsmSection::Decl);
        _Md->Bin.AsmOutCommentLine(AsmSection::Decl,"Declared from walk sentence",true);
        _Md->Bin.AsmOutVarDecl(AsmSection::Decl,(_Md->CurrentScope().Kind!=ScopeKind::Local?true:false),false,false,false,false,IxVarName,
        _Md->CpuDataTypeFromMstType(_Md->Types[_Md->WrdTypIndex].MstType),_Md->VarLength(IxVarIndex),_Md->Variables[IxVarIndex].Address,"",false,false,"","");
      }

    }

    //Set index variable as initialized (as no initialization is required)
    _Md->Variables[IxVarIndex].IsInitialized=true;
    DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[IxVarIndex].Name+" in scope "+_Md->ScopeName(_Md->Variables[IxVarIndex].Scope));

  }

  //Set variable index if array index variable is not used
  else{
    IxVarIndex=-1;
  }

  //Send initialization instructions for fixed arrays
  if(ArrToken.MstType()==MasterType::FixArray){
    if(!_Md->Bin.AsmWriteCode(CpuInstCode::AF1RW,_Md->AsmAgx(ArrToken.TypIndex()),(IxVarIndex==-1?_Md->AsmNva():_Md->AsmVad(IxVarIndex)),_Md->AsmJmp(Stn.GetLabel(CodeLabelId::Exit)))){ return false; }
    _Md->Bin.StoreJumpDestination(Stn.GetLabel(CodeLabelId::LoopBeg),_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());
    if(!_Md->Bin.AsmWriteCode(CpuInstCode::AF1FO,_Md->AsmVar(OnVarIndex),ArrToken.Asm(),_Md->AsmAgx(ArrToken.TypIndex()))){ return false; }
  }

  //Send initialization instructions for dynamic arrays
  else if(ArrToken.MstType()==MasterType::DynArray){
    if(!_Md->Bin.AsmWriteCode(CpuInstCode::AD1RW,ArrToken.Asm(),(IxVarIndex==-1?_Md->AsmNva():_Md->AsmVad(IxVarIndex)),_Md->AsmJmp(Stn.GetLabel(CodeLabelId::Exit)))){ return false; }
    _Md->Bin.StoreJumpDestination(Stn.GetLabel(CodeLabelId::LoopBeg),_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());
    if(!_Md->Bin.AsmWriteCode(CpuInstCode::AD1FO,_Md->AsmVar(OnVarIndex),ArrToken.Asm())){ return false; }
  }

  //Add if expression
  if(HasIfExpr){

    //Compile if expression
    if(!Expr.Compile(_Md,_Md->CurrentScope(),Stn,IfExpr[0],IfExpr[1],Result)){ return false; }

    //Check result is boolean
    if(Result.MstType()!=MasterType::Boolean){
      Result.Msg(528).Print();
      return false;
    }

    //Output conditional instruction
    if(!_Md->Bin.AsmWriteCode(CpuInstCode::JMPFL,Result.Asm(),_Md->AsmJmp(Stn.GetLabel(CodeLabelId::LoopEnd)))){
      return false;
    }

    //Release result token
    Result.Release();

  }

  //Return success
  return true;

}

//Compile EndWalk
//:endwalk
bool Compiler::_CompileEndWalk(Sentence& Stn,Stack<ExprToken>& WalkArray){

  //Variables
  Expression Expr;
  ExprToken ArrToken;

  //Parse sentence
  if(!Stn.Get(PrKeyword::EndWalk).Ok()){ 
    return false; 
  }

  //Get step sentence
  if(WalkArray.Length()==0){
    Stn.Tokens[0].Msg(112).Print();
    return false;
  }
  ArrToken=WalkArray.Pop();

  //Record jump destination for loop end
  _Md->Bin.StoreJumpDestination(Stn.GetLabel(CodeLabelId::LoopEnd),_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());

  //Emit loop end instruction
  if(_Md->Types[ArrToken.TypIndex()].MstType==MasterType::FixArray){
    if(!_Md->Bin.AsmWriteCode(CpuInstCode::AF1NX,_Md->AsmAgx(ArrToken.TypIndex()),_Md->AsmJmp(Stn.GetLabel(CodeLabelId::LoopBeg)))){ return false; }
  }
  else if(_Md->Types[ArrToken.TypIndex()].MstType==MasterType::DynArray){
    if(!_Md->Bin.AsmWriteCode(CpuInstCode::AD1NX,ArrToken.Asm(),_Md->AsmJmp(Stn.GetLabel(CodeLabelId::LoopBeg)))){ return false; }
  }

  //Record jump destination for loop exit
  _Md->Bin.StoreJumpDestination(Stn.GetLabel(CodeLabelId::Exit),_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());

  //Release array token
  ArrToken.Release();

  //Return result
  return true;

}

//Compile Switch
//switch(<expr>):
bool Compiler::_CompileSwitch(Sentence& Stn,Stack<Sentence>& SwitchExpr){

  //Variables
  int Begin;
  int End;

  //Parse sentence
  if(!Stn.Get(PrKeyword::Switch).Get(PrPunctuator::BegParen).ReadEx(PrPunctuator::EndParen,Begin,End).Get(PrPunctuator::EndParen).Get(PrPunctuator::Colon).Ok()){ 
    return false; 
  }

  //Push switch expresion
  SwitchExpr.Push(Stn.SubSentence(Begin,End));

 //Return result
  return true;

}

//Compile When
//when(<expr>):
bool Compiler::_CompileWhen(Sentence& Stn,Stack<Sentence>& SwitchExpr,bool FirstCase){

  //Variables
  int Begin;
  int End;
  Expression Expr;
  ExprToken Result;
  Sentence Switch;
  Sentence CondStn;

  //Parse sentence
  if(!Stn.Get(PrKeyword::When).Get(PrPunctuator::BegParen).ReadEx(PrPunctuator::EndParen,Begin,End).Get(PrPunctuator::EndParen).Get(PrPunctuator::Colon).Ok()){ 
    return false; 
  }

  //Get switch expression
  if(SwitchExpr.Length()==0){
    Stn.Tokens[0].Msg(113).Print();
    return false;
  }
  Switch=SwitchExpr.Top();

  //Record jump destination for current case
  _Md->Bin.StoreJumpDestination(Stn.GetLabel(CodeLabelId::CurrCond),_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());

  //Compile condition expression
  CondStn=Switch.Add(PrOperator::Equal)+Stn.SubSentence(Begin,End);
  if(!Expr.Compile(_Md,_Md->CurrentScope(),CondStn,Result)){ return false; }
  
  //Conditional sentence must be evaluated to boolean
  if(Result.MstType()!=MasterType::Boolean){
    Stn.Tokens[Begin].Msg(114).Print();
    return false;
  }

  //Emit instruction
  if(!_Md->Bin.AsmWriteCode(CpuInstCode::JMPFL,Result.Asm(),_Md->AsmJmp(Stn.GetLabel(CodeLabelId::NextCond)))){
    return false;
  }

  //Return result
  return true;

}

//Compile Default
//default:
bool Compiler::_CompileDefault(Sentence& Stn,Stack<Sentence>& SwitchExpr){

  //Parse sentence
  if(!Stn.Get(PrKeyword::Default).Ok()){ 
    return false; 
  }
 
  //Check switch expression
  if(SwitchExpr.Length()==0){
    Stn.Tokens[0].Msg(115).Print();
    return false;
  }

  //Record jump destination for current case
  _Md->Bin.StoreJumpDestination(Stn.GetLabel(CodeLabelId::CurrCond),_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());

  //Return result
  return true;

}

//Compile EndSwitch
//:endswitch
bool Compiler::_CompileEndSwitch(Sentence& Stn,Stack<Sentence>& SwitchExpr){

  //Parse sentence
  if(!Stn.Get(PrKeyword::EndSwitch).Ok()){ 
    return false; 
  }

  //Get step sentence
  if(SwitchExpr.Length()==0){
    Stn.Tokens[0].Msg(116).Print();
    return false;
  }
  SwitchExpr.Pop();


  //Record active jump destinations
  _Md->Bin.StoreJumpDestination(Stn.GetLabel(CodeLabelId::NextCond),_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());
  _Md->Bin.StoreJumpDestination(Stn.GetLabel(CodeLabelId::Exit),_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());

  //Return result
  return true;

}

//Compile Break
//break
bool Compiler::_CompileBreak(Sentence& Stn,bool InsideSwitch,bool InsideLoop){

  //Parse sentence
  if(!Stn.Get(PrKeyword::Break).Ok()){ 
    return false; 
  }

  //Emit instruction inside switch
  if(InsideSwitch){
    if(!_Md->Bin.AsmWriteCode(CpuInstCode::JMP,_Md->AsmJmp(Stn.GetLabel(CodeLabelId::Exit)))){
      return false;
    }
  }

  //Emit instruction inside loop
  else if(InsideLoop){
    if(!_Md->Bin.AsmWriteCode(CpuInstCode::JMP,_Md->AsmJmp(Stn.GetLabel(CodeLabelId::LoopExit)))){
      return false;
    }
  }

  //Rest of cases error
  else{
    Stn.Tokens[0].Msg(22);
    return false;
  }

  //Return result
  return true;

}

//Compile Continue
//continue
bool Compiler::_CompileContinue(Sentence& Stn,bool InsideLoop){

  //Parse sentence
  if(!Stn.Get(PrKeyword::Continue).Ok()){ 
    return false; 
  }

  //Emit instruction inside loop
  if(InsideLoop){
    if(!_Md->Bin.AsmWriteCode(CpuInstCode::JMP,_Md->AsmJmp(Stn.GetLabel(CodeLabelId::LoopNext)))){
      return false;
    }
  }

  //Rest of cases error
  else{
    Stn.Tokens[0].Msg(24);
    return false;
  }

  //Return result
  return true;

}

//Init variable
//$init <id>
bool Compiler::_CompileInitVar(Sentence& Stn){

  //Variables
  int TypIndex;
  int VarIndex;
  bool CodeGenerated;
  String VarName;
  ExprToken Result;
  ExprToken Variv;
  SourceInfo SrcInfo;

  //Get variable name
  if(!Stn.Get(PrKeyword::InitVar).ReadId(VarName).Ok()){ return false; }
  SrcInfo=Stn.LastSrcInfo();

  //Variable must be already defined
  if((VarIndex=_Md->VarSearch(VarName,_Md->CurrentScope().ModIndex))==-1){
    Stn.Msg(542).Print(VarName);
    return false;
  }
  TypIndex=_Md->Variables[VarIndex].TypIndex;

  //Initialization for variable
  _Md->Bin.AsmOutNewLine(AsmSection::Body);
  _Md->Bin.AsmOutCommentLine(AsmSection::Body,_SourceLine.Trim(),true);
  if((_Md->IsMasterAtomic(TypIndex) || _Md->Types[TypIndex].MstType==MasterType::Enum)
  && _Md->Types[TypIndex].MstType!=MasterType::String){
    _Md->Bin.AsmDeleteLast(AsmSection::Body);
    _Md->Bin.AsmDeleteLast(AsmSection::Body);
    if(_Md->Types[TypIndex].MstType==MasterType::Enum){
      if(!Result.NewConst(_Md,_Md->Types[_Md->IntTypIndex].MstType,SrcInfo)){ return false; }
    }
    else{
      if(!Result.NewConst(_Md,_Md->Types[TypIndex].MstType,SrcInfo)){ return false; }
    }
    if(!Result.CopyValue(_Md->Bin.GlobValuePointer(_Md->Variables[VarIndex].Address))){ return false; }
  }
  else{
    Variv.ThisVar(_Md,VarIndex,SrcInfo);
    if(!Expression().InitOperand(_Md,Variv,CodeGenerated)){ return false; } 
    if(!CodeGenerated){
      _Md->Bin.AsmDeleteLast(AsmSection::Body);
      _Md->Bin.AsmDeleteLast(AsmSection::Body);
    }
  }

  //Return result
  return true;

}

//Emit source function header into assembler file
void Compiler::_EmitFunctionSourceLine(const String& SourceLine){
  _Md->Bin.AsmOutNewLine(AsmSection::Head);
  _Md->Bin.AsmOutSeparator(AsmSection::Head);
  _Md->Bin.AsmOutCommentLine(AsmSection::Head,SourceLine.Trim(),false);
  _Md->Bin.AsmOutSeparator(AsmSection::Head);
  _Md->Bin.AsmOutNewLine(AsmSection::Head);
}

//Emit source line in code
void Compiler::_EmitSourceLine(const Sentence& Stn,const String& SourceLine){
  
  //Switch on sentence id
  switch(Stn.Id){
    
    //Blockm arkers and other sentences that do not emit code
    case SentenceId::Libs       : 
    case SentenceId::Public     : 
    case SentenceId::Private    : 
    case SentenceId::Implem     : 
    case SentenceId::Set        : 
    case SentenceId::Include    : 
    case SentenceId::DefType    : 
    case SentenceId::DefClass   : 
    case SentenceId::Publ       : 
    case SentenceId::Priv       : 
    case SentenceId::EndClass   : 
    case SentenceId::Allow      : 
    case SentenceId::DefEnum    : 
    case SentenceId::EnumField  : 
    case SentenceId::EndEnum    : 
    case SentenceId::DlType     : 
    case SentenceId::Main       :
    case SentenceId::Function   :
    case SentenceId::Operator   :
    case SentenceId::Member     :
    case SentenceId::EndMain    : 
    case SentenceId::EndFunction: 
    case SentenceId::EndOperator: 
    case SentenceId::EndMember  : 
    case SentenceId::Empty      : 
    case SentenceId::InitVar    : 
      break;
    
    //Variable declarations
    case SentenceId::VarDecl:
      if(_Md->CurrentSubScope().Kind==SubScopeKind::None){
        _Md->Bin.AsmOutNewLine(AsmSection::Decl);
        _Md->Bin.AsmOutCommentLine(AsmSection::Decl,SourceLine.Trim(),true);
      }
      break;

    //Rest of declarations
    case SentenceId::Import    : 
    case SentenceId::Const     :
    case SentenceId::FunDecl   :
    case SentenceId::SystemCall: 
    case SentenceId::SystemFunc: 
    case SentenceId::DlFunction: 
      _Md->Bin.AsmOutNewLine(AsmSection::Decl);
      _Md->Bin.AsmOutCommentLine(AsmSection::Decl,SourceLine.Trim(),true);
      break;

    //Control flow & expressions
    case SentenceId::Return    : 
    case SentenceId::If        : 
    case SentenceId::ElseIf    : 
    case SentenceId::Else      : 
    case SentenceId::EndIf     : 
    case SentenceId::While     : 
    case SentenceId::EndWhile  : 
    case SentenceId::Do        : 
    case SentenceId::Loop      : 
    case SentenceId::For       : 
    case SentenceId::EndFor    : 
    case SentenceId::Walk      : 
    case SentenceId::EndWalk   : 
    case SentenceId::Switch    : 
    case SentenceId::When      : 
    case SentenceId::Default   : 
    case SentenceId::EndSwitch : 
    case SentenceId::Break     : 
    case SentenceId::Continue  : 
    case SentenceId::XlvSet    : 
    case SentenceId::Expression: 
      _Md->Bin.AsmOutNewLine(AsmSection::Body);
      _Md->Bin.AsmOutCommentLine(AsmSection::Body,SourceLine.Trim(),true);
      break;
  }

}

//Create application package
bool Compiler::CreateAppPackage(const String& BinaryFile,const String& ContainerFile){

  //Variables
  int BinHnd;
  int ConHnd;
  long RomBufferPos;
  String AppFileName;
  String BinFileName;
  Buffer BinBuffer;
  Buffer ConBuffer;
  RomFileBuffer RomBuffer;
  
  //Load binary file into buffer
  if(!_Stl->FileSystem.GetHandler(BinHnd)){ SysMessage(339).Print(BinaryFile); return false; }
  if(!_Stl->FileSystem.OpenForRead(BinHnd,BinaryFile)){ SysMessage(340).Print(BinaryFile,_Stl->LastError()); return false; }
  if(!_Stl->FileSystem.FullRead(BinHnd,BinBuffer)){ SysMessage(341).Print(BinaryFile,_Stl->LastError()); return false; }
  if(!_Stl->FileSystem.CloseFile(BinHnd)){ SysMessage(342).Print(BinaryFile,_Stl->LastError()); return false; }
  if(!_Stl->FileSystem.FreeHandler(BinHnd)){ SysMessage(343).Print(BinaryFile,_Stl->LastError()); return false; }

  //Load container file into buffer
  if(!_Stl->FileSystem.GetHandler(ConHnd)){ SysMessage(344).Print(ContainerFile); return false; }
  if(!_Stl->FileSystem.OpenForRead(ConHnd,ContainerFile)){ SysMessage(345).Print(ContainerFile,_Stl->LastError()); return false; }
  if(!_Stl->FileSystem.FullRead(ConHnd,ConBuffer)){ SysMessage(346).Print(ContainerFile,_Stl->LastError()); return false; }
  if(!_Stl->FileSystem.CloseFile(ConHnd)){ SysMessage(347).Print(ContainerFile,_Stl->LastError()); return false; }
  if(!_Stl->FileSystem.FreeHandler(ConHnd)){ SysMessage(348).Print(ContainerFile,_Stl->LastError()); return false; }

  //Check that size of binary file is not bigger than maximun payload
  if(BinBuffer.Length()>(long)ROM_BUFF_PAYLOADKB*(long)1024){
    SysMessage(349).Print(_Stl->FileSystem.GetFileName(BinaryFile),ToString(BinBuffer.Length()),_Stl->FileSystem.GetFileName(ContainerFile),ToString((long)ROM_BUFF_PAYLOADKB*(long)1024));
    return false;
  }

  //Get binary file name
  BinFileName=_Stl->FileSystem.GetFileName(BinaryFile);

  //Setup ROM file buffer
  RomBuffer.Loaded=1;
  RomBuffer.Length=BinBuffer.Length();
  RomBuffer.End=0x77FF;
  MemCpy(&RomBuffer.Label[0],ROM_BUFF_LABEL,strlen(ROM_BUFF_LABEL)+1);
  MemCpy(&RomBuffer.FileName[0],BinFileName.CharPnt(),BinFileName.Length()+1);
  MemCpy(&RomBuffer.Buff[0],BinBuffer.BuffPnt(),BinBuffer.Length());

  //Seek ROM buffer label inside container file
  if((RomBufferPos=ConBuffer.Search(Buffer(ROM_BUFF_LABEL)))==-1){
    SysMessage(350).Print(ContainerFile); 
    return false;
  }

  //Write ROM buffer inside container file buffer
  MemCpy(&ConBuffer.BuffPnt()[RomBufferPos],&RomBuffer,sizeof(RomBuffer));

  //Compose application file name
  AppFileName=_Stl->FileSystem.GetDirName(ContainerFile)+_Stl->FileSystem.GetFileNameNoExt(BinaryFile)+_Stl->FileSystem.GetFileExtension(ContainerFile);

  //Write application file using container buffer
  if(!_Stl->FileSystem.GetHandler(ConHnd)){ SysMessage(351).Print(AppFileName); return false; }
  if(!_Stl->FileSystem.OpenForWrite(ConHnd,AppFileName)){ SysMessage(352).Print(AppFileName,_Stl->LastError()); return false; }
  if(!_Stl->FileSystem.Write(ConHnd,ConBuffer)){ SysMessage(353).Print(AppFileName,_Stl->LastError()); return false; }
  if(!_Stl->FileSystem.CloseFile(ConHnd)){ SysMessage(354).Print(AppFileName,_Stl->LastError()); return false; }
  if(!_Stl->FileSystem.FreeHandler(ConHnd)){ SysMessage(355).Print(AppFileName,_Stl->LastError()); return false; }

  //Return success
  return true;

}

//Main program
bool CallCompiler(const String& SourceFile,const String& OutputFile,bool CompileToApp,const String& ContainerFile,
                  bool EnableAsmFile,bool StripSymbols,bool CompilerStats,bool LinterMode,int MaxErrorNr,int MaxWarningNr,bool PassOnWarnings,
                  const String& IncludePath,const String& LibraryPath,const String& DynLibPath,bool& CompileToLibrary){

  //Variables
  CompilerConfig Config;
  Compiler Comp;

  //Catch exceptions
  try{

    //Set compilerconfiguration
    Config.EnableAsmFile=EnableAsmFile;
    Config.DebugSymbols=(LinterMode?false:!StripSymbols);
    Config.MaxErrorNr=MaxErrorNr;
    Config.MaxWarningNr=MaxWarningNr;
    Config.PassOnWarnings=PassOnWarnings;
    Config.CompilerStats=CompilerStats;
    Config.LinterMode=LinterMode;
    Config.CompileToApp=CompileToApp;
    Comp.SetConfig(Config);
  
    //Open debug log
    DebugOpen(SourceFile,CMP_LOG_EXT);
  
    //Compile program
    if(!Comp.Compile(SourceFile,OutputFile,IncludePath,LibraryPath,DynLibPath,CompileToLibrary)){
      DebugClose();
      return false;
    }

    //Create aplication package
    if(CompileToApp){
      if(!Comp.CreateAppPackage(OutputFile,ContainerFile)){
        DebugClose();
        return false;
      }
    }

  }
  
  //Exception handler
  catch(BaseException& Ex){
    _Stl->Console.PrintLine(Ex.Description());
    DebugClose();
    return false;
  }

  //Close debug log
  DebugClose();

  //Return code
  return true;

}

//Main program
bool CallLibraryInfo(const String& LibraryFile){

  //Variables
  MasterData Md;

  //Catch exceptions
  try{

    //Open debug log
    DebugOpen(LibraryFile,CMP_LOG_EXT);
  
    //Compile program
    if(!Md.Bin.PrintBinary(LibraryFile,true)){
      DebugClose();
      return false;
    }

  }
  
  //Exception handler
  catch(BaseException& Ex){
    _Stl->Console.PrintLine(Ex.Description());
    DebugClose();
    return false;
  }

  //Close debug log
  DebugClose();

  //Return code
  return true;

}

//Main program
bool CallExecutableInfo(const String& ExecutableFile){

  //Variables
  MasterData Md;

  //Catch exceptions
  try{

    //Open debug log
    DebugOpen(ExecutableFile,CMP_LOG_EXT);
  
    //Compile program
    if(!Md.Bin.PrintBinary(ExecutableFile,false)){
      DebugClose();
      return false;
    }

  }
  
  //Exception handler
  catch(BaseException& Ex){
    _Stl->Console.PrintLine(Ex.Description());
    DebugClose();
    return false;
  }

  //Close debug log
  DebugClose();

  //Return code
  return true;

}