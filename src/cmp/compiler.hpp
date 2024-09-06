//Compiler.hpp: Header file for compiler class
#ifndef _COMPILER_HPP
#define _COMPILER_HPP

//Compiler configuration
class CompilerConfig{
  
  //Private members
  private:
    void _Move(const CompilerConfig& Config);

  //Public members
  public:

    //Configuration constants
    const int ParserStackDefaultSize=5; //Parser stack default size
    
    //Configuration variables
    int MaxErrorNr;      //Maximun compiler errors to report before termiation
    int MaxWarningNr;    //Maximun compiler errors to report before termiation
    bool PassOnWarnings; //Ignore warnings when gnerating binaries
    bool EnableAsmFile;  //Enable assembler file generation
    bool DebugSymbols;   //Enable debug symbols on binary file
    bool CompilerStats;  //Output compiler statistics
    bool LinterMode;     //Linter mode
    bool CompileToApp;   //Compile to application package
    
    //Constructors/Destructors and assignment
    CompilerConfig(){}
    ~CompilerConfig(){}  
    CompilerConfig(const CompilerConfig& Config);
    CompilerConfig& operator=(const CompilerConfig& Config);

};

//Compiler class
class Compiler{  
  
  //Private members
  private:
    MasterData *_Md;          //Program master tables
    Stack<Parser> _PsStack;   //Parser stack
    CompilerConfig _Config;   //Compiler configuration
    int _CmplxLitValNr;       //Complex litteral value counter
    String _SourceLine;       //Current source line
    bool _DelayedInitEnded;   //Delayed init routine was ended
    CpuShr _LibMajorVers;     //Library major version
    CpuShr _LibMinorVers;     //Library minor version
    CpuShr _LibRevisionNr;    //Library revision number

    //Helper functions
    void _DelayedInitStart();
    void _DelayedInitEnd();
    String _GetNewCmplxLitValName(const String& TypeName);
    int _FindOperatorFunction(const Sentence& Stn) const;
    bool _FindIdentifier(const Sentence& Stn,int BegToken,int EndToken,const String& VarName) const;
    void _EmitFunctionSourceLine(const String& SourceLine);
    void _EmitSourceLine(const Sentence& Stn,const String& SourceLine);
    bool _DefineGrant(const Sentence& Stn,GrantKind Kind,int TypIndex,const String& FrTypName,const String& FrFunName,int ToFldIndex,int ToFunIndex,int FrLineNr,int FrColNr,int RecurLevel);
    bool _SolveLibraryUndefRefs(bool HardLinkage,const SourceInfo& SrcInfo);
    bool _ImportLibrarySingle(const String& FilePath,const String& Tracker,CpuShr Major,CpuShr Minor,CpuShr RevNr,const SourceInfo& SrcInfo);
    bool _ImportLibraryRecur(const String& ImportName,const String& ImportTracker,const String& LibraryPath,const String& DefaultPath,CpuShr Major,CpuShr Minor,CpuShr RevNr,const SourceInfo& SrcInfo,int RecurLevel);
    String _ResolveLibraryPath(const String& ImportName,const String& LibraryPath,const String& DefaultPath);
    bool _CompileLibs(Sentence& Stn);
    bool _CompilePublic(Sentence& Stn);
    bool _CompilePrivate(Sentence& Stn);
    bool _CompileImplem(Sentence& Stn);
    bool _CompileSet(Sentence& Stn,bool CompileToLibrary);
    bool _CompileImport(Sentence& Stn,const String& LibraryPath,const String& DefaultPath);
    bool _CompileInclude(Sentence& Stn,bool& AlreadyLoaded,const String& IncludePath,const String& DefaultPath);
    bool _CompileDeftype(Sentence& Stn);
    bool _CompileDlType(Sentence& Stn);
    bool _CompileConst(Sentence& Stn);
    bool _CompileVar(Sentence& Stn);
    bool _CompileClassField(Sentence& Stn);
    bool _CompilePubl(Sentence& Stn);
    bool _CompilePriv(Sentence& Stn);
    bool _CompileDefClass(Sentence& Stn);
    bool _CompileEndClass(Sentence& Stn);
    bool _CompileAllow(Sentence& Stn);
    bool _CompileDefEnum(Sentence& Stn);
    bool _CompileEnumField(Sentence& Stn);
    bool _CompileEndEnum(Sentence& Stn);
    bool _CompileFuncDecl(Sentence& Stn,int& FunIndex);
    bool _CompileFuncDefn(Sentence& Stn,int& FunIndex);
    bool _CompileEndFunction(Sentence& Stn,SentenceId LastStnId);
    bool _CompileEndMember(Sentence& Stn,SentenceId LastStnId);
    bool _CompileEndOperator(Sentence& Stn,SentenceId LastStnId);
    bool _CompileMain(Sentence& Stn,const String& ProgName,bool CompileToLibrary);
    bool _CompileEndMain(Sentence& Stn,const String& ProgName,SentenceId LastStnId);
    bool _CompileReturn(Sentence& Stn,const String& ProgName);
    bool _CompileIf(Sentence& Stn);
    bool _CompileElseIf(Sentence& Stn);
    bool _CompileElse(Sentence& Stn);
    bool _CompileEndIf(Sentence& St);
    bool _CompileWhile(Sentence& Stn);
    bool _CompileEndWhile(Sentence& Stn);
    bool _CompileDo(Sentence& Stn);
    bool _CompileLoop(Sentence& Stn);
    bool _CompileFor(Sentence& Stn,Stack<Sentence>& ForStep);
    bool _CompileEndFor(Sentence& Stn,Stack<Sentence>& ForStep);
    bool _CompileWalk(Sentence& Stn,Stack<ExprToken>& WalkArray);
    bool _CompileEndWalk(Sentence& Stn,Stack<ExprToken>& WalkArray);
    bool _CompileSwitch(Sentence& Stn,Stack<Sentence>& SwitchExpr);
    bool _CompileWhen(Sentence& Stn,Stack<Sentence>& SwitchExpr,bool FirstCase);
    bool _CompileDefault(Sentence& Stn,Stack<Sentence>& SwitchExpr);
    bool _CompileEndSwitch(Sentence& Stn,Stack<Sentence>& SwitchExpr);
    bool _CompileBreak(Sentence& Stn,bool InsideSwitch,bool InsideLoop);
    bool _CompileContinue(Sentence& Stn,bool InsideLoop);
    bool _CompileInitVar(Sentence& Stn);

  //Public members
  public:

    //Public functions
    void SetConfig(const CompilerConfig& Config);
    bool Compile(const String& SourceFile,const String& OutputFile,const String& IncludePath,const String& LibraryPath,const String& DynLibPath,bool& CompileToLibrary); 
    bool CreateAppPackage(const String& BinaryFile,const String& ContainerFile);
    
    //Constructor / Destructor
    Compiler();   //Constructor
    ~Compiler(){} //Destructor

};

//Compiler entry points
bool CallCompiler(const String& SourceFile,const String& OutputFile,bool CompileToApp,const String& ContainerFile,bool EnableAsmFile,bool StripSymbols,
                  bool CompilerStats,bool LinterMode,int MaxErrorNr,int MaxWarningNr,bool PassOnWarnings,const String& IncludePath,const String& LibraryPath,
                  const String& DynLibPath,bool& CompileToLibrary);
bool CallLibraryInfo(const String& LibraryFile);
bool CallExecutableInfo(const String& ExecutableFile);

#endif  