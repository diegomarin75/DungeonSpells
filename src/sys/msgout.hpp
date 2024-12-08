//msgio.hpp: Header file for message output class
#ifndef _MSGOUT_HPP
#define _MSGOUT_HPP
 
//Message dispatcher
class SysMsgDispatcher {
  
  //Public members
  public:

    //Public functions
    static void SetSourceLine(String SourceLine);
    static String GetSourceLine();
    static void SetMaximunErrorCount(int Maximun);
    static void SetMaximunWarningCount(int Maximun);
    static int GetErrorCount();
    static int GetWarningCount();
    static void SetForceOutput(bool Force);
   
};

//Message
class SysMessage {
  
  //Private members
  private:
    
    //Private members
    String _FileName;
    int _LineNr;
    int _ColumnNr;
    int _Index;
    String _MsgSourceLine;
    
    //Private methods
    int _FindMessageCode(int Code);
    String _Compose(const String& Parm1="",const String& Parm2="",const String& Parm3="",const String& Parm4="",const String& Parm5="",const String& Parm6="");
    String _GetSourcePointer(const String& SourceLine);

  //Public members
  public:
    SysMessage();
    SysMessage(int Code);
    SysMessage(int Code,const String& FileName);
    SysMessage(int Code,const String& FileName,int LineNr);
    SysMessage(int Code,const String& FileName,int LineNr,int ColumnNr);
    SysMessage(int Code,const String& FileName,int LineNr,int ColumnNr,const String& MsgSourceLine);
    ~SysMessage();
    String GetString(const String& Parm1="",const String& Parm2="",const String& Parm3="",const String& Parm4="",const String& Parm5="",const String& Parm6="");
    void Print(const String& Parm1="",const String& Parm2="",const String& Parm3="",const String& Parm4="",const String& Parm5="",const String& Parm6="",bool Flush=true);
    void Delay(const String& Parm1="",const String& Parm2="",const String& Parm3="",const String& Parm4="",const String& Parm5="",const String& Parm6="");
    int  DelayCount();
    void FlushDelayedMessages(const String& FileName,int LineNr);
    
};

//Source information
struct SourceInfo{
  String FileName;                                         //File name
  int LineNr;                                              //Line Number
  int ColNr;                                               //Column index
  SysMessage Msg(int Code) const;                          //Create message
  SysMessage Msg(int Code,const String& SourceLine) const; //Create message with source line
  SourceInfo();                                            //Constructor
  ~SourceInfo(){}                                          //Destructor
  SourceInfo(const String& Name,int Line,int Col);         //Constructor from values
  SourceInfo(const SourceInfo& Info);                      //Copy constructor
  SourceInfo& operator=(const SourceInfo& Info);           //Assignment operator
  void _Move(const SourceInfo& Info);                      //Move
};

#endif