#Imports
import os
import re
import sys
import collections
import datetime
import subprocess
import time
from dateutil.relativedelta import relativedelta

#Constants
GITCOMMITID="###commit###"
GITCOMMAND="git log <branch> --format=\"<commitid> %h% G? %ai %s {%D}\" --reverse -p --no-color --encoding=utf-8"

#Named tuple definitions
GitListCommit = collections.namedtuple("GitListCommit","chsh auth date time adds dels lcnt totl desc")
GitCumulCommit = collections.namedtuple("GitCumulCommit","date lines")

#-----------------------------------------------------------------------------------------------------------------------------------------
#Get command line arguments
#-----------------------------------------------------------------------------------------------------------------------------------------
def GetCommandLineOptions(Options):

  #Default options
  GitBranch=""
  Extensions=["*"]
  Filter=".*"
  PrintList=False
  PrintGraph=True
  Scale=250
  MarkDots=6
  MaxHeight=0
  Factor=5
  TimeScale="day"
  MinDate="00000000"
  MaxDate="99991231"
  FirstDays=0
  LastDays=0
  ScreenFit=False

  #No arguemnts given, show help
  if len(sys.argv)<2:
    print("Git history graph - Diego Marin 2019 - v0.1")
    print("") 
    print("Usage: python githist.py <gitbranch> [options]")
    print("") 
    print("options: [--ext:<extensions>] [--filter:<filter>] [--scale:<scale>]") 
    print("         [--markdots:<dots>] [--maxheight:<height>] [--round:<factor>]")
    print("         [--by:<timescale>] [--mindate:<mindate>]  [--maxdate:<maxdate>]")
    print("         [--first:<firstdays] [--last:<lastdays>] [--screenfit] [--list]")
    print("") 
    print("gitbranch:  Git branch to get commit information from")
    print("extensions: File extensions to take into account for line count separated by comma")
    print("filter:     Regular expression to filter file names for line counting")
    print("scale:      Number of lines per dot (default: "+str(Scale)+")")
    print("dots:       Number of dots per axis mark (default: "+str(MarkDots)+")")
    print("height:     Graph maximun height in dots (overrides scale specification) (default: "+str(MaxHeight)+")")
    print("factor:     Rounding factor of the scale when height is given (default: "+str(Factor)+")")
    print("timescale:  Time scale of the graph, use: day, week or month (default: "+str(TimeScale)+")")
    print("mindate:    Minimun commit date to print, use 00000000 for beginning of history (default: "+str(MinDate)+")")
    print("maxdate:    Maximun commit date to print, use 99991231 for ending of history (default: "+str(MinDate)+")")
    print("firstdays:  Display only up to first commit days (default: "+str(FirstDays)+")")
    print("lastdays:   Display only up to last commit days (default: "+str(LastDays)+")")
    print("screenfit:  Adjust graph height and lastdays automatically according to console size (default: "+("enabled" if ScreenFit else "disabled")+")")
    print("list:       Commit list is printed instead of graph (default: "+("enabled" if PrintList else "disabled")+")")
    print("") 
    print("Example: python githist.py origin/master --ext:cpp,hpp")
    return False
  
  #Get arguments
  else:
    GitBranch=sys.argv[1]
    for i in range(2,len(sys.argv)):
      item=sys.argv[i]
      if item.find("--ext:")==0:
        Extensions=list(item.replace("--ext:","").split(","))
      elif item.find("--filter:")==0:
        Filter=item.replace("--filter:","")
      elif item.find("--scale:")==0:
        Scale=int(item.replace("--scale:",""))
      elif item.find("--markdots:")==0:
        MarkDots=int(item.replace("--markdots:",""))
      elif item.find("--maxheight:")==0:
        MaxHeight=int(item.replace("--maxheight:",""))
      elif item.find("--round:")==0:
        Factor=int(item.replace("--round:",""))
      elif item.find("--by:")==0:
        TimeScale=item.replace("--by:","")
        if not TimeScale in ["day","week","month"]:
          print("Invalid value for time scale option. Valid values are: day, week or month.")
          return False
      elif item.find("--mindate:")==0:
        MinDate=item.replace("--mindate:","")
      elif item.find("--maxdate:")==0:
        MaxDate=item.replace("--maxdate:","")
      elif item.find("--first:")==0:
        FirstDays=int(item.replace("--first:",""))
      elif item.find("--last:")==0:
        LastDays=int(item.replace("--last:",""))
      elif item=="--screenfit":
        ScreenFit=True
      elif item=="--list":
        PrintList=True
        PrintGraph=False
      else:
        print("Invalid option: ",item)
        return False
  
  #Return arguments
  Options.append(GitBranch)
  Options.append(Extensions)
  Options.append(Filter)
  Options.append(PrintList)
  Options.append(PrintGraph)
  Options.append(Scale)
  Options.append(MarkDots)
  Options.append(MaxHeight)
  Options.append(Factor)
  Options.append(TimeScale)
  Options.append(MinDate)
  Options.append(MaxDate)
  Options.append(FirstDays)
  Options.append(LastDays)
  Options.append(ScreenFit)

  #Return code
  return True

#-----------------------------------------------------------------------------------------------------------------------------------------
# Set timing start
#-----------------------------------------------------------------------------------------------------------------------------------------
def ProgBarStart(TotalItems):
  global _TotalItems
  _TotalItems=TotalItems

#-----------------------------------------------------------------------------------------------------------------------------------------
# Get progress bar message
#-----------------------------------------------------------------------------------------------------------------------------------------
def ProgBarText(Message,Counter):
  global _StartTime
  if(Counter==0):
    _StartTime=time.time()
  BarTotal=50
  Percentage=(Counter+1)/_TotalItems
  BarCount=int(BarTotal*Percentage)
  ElapsedTime=time.time()-_StartTime
  TotalTime=((ElapsedTime/(Counter+1))*_TotalItems)
  RemainingTime=TotalTime-ElapsedTime
  return Message.ljust(30)+' {0:6.2f}% [{1}{2}] {3:02d}:{4:02d} {5}' \
  .format(Percentage*100,'#'*BarCount,'.'*(BarTotal-BarCount),
  int(RemainingTime/60),int(RemainingTime%60),
  ' '*10)

#-----------------------------------------------------------------------------------------------------------------------------------------
# Print progress bar message
#-----------------------------------------------------------------------------------------------------------------------------------------
def ProgBarPrint(Message,Counter):
  print(ProgBarText(Message,Counter),end='\r',flush=True)
  if(Counter==_TotalItems-1):
    print('')

#-----------------------------------------------------------------------------------------------------------------------------------------
#Accumulate totals from file counters
#-----------------------------------------------------------------------------------------------------------------------------------------
def CalculateTotals(FileCounters,Extensions,Filter):
  
  #Init result
  Adds=0
  Dels=0
  Totl=0

  #Set regex for filter names
  Pattern=re.compile(Filter)
  
  #File loop
  for FileName in FileCounters:
    Ignore=True
    if len(FileName.split("."))>1:
      Ext=FileName.split(".")[1].replace("\n","").strip()
      if Ext in Extensions or Extensions[0]=="*":
        Ignore=False
      else:
        Ignore=True
    elif Extensions[0]=="*":
      Ignore=False
    if not Pattern.match(FileName):
      Ignore=True
    if Ignore==False:
      Adds+=FileCounters[FileName][0]
      Dels+=FileCounters[FileName][1]
      Totl+=FileCounters[FileName][2]

  #Return result
  return Adds,Dels,Totl

#-----------------------------------------------------------------------------------------------------------------------------------------
#String date in format YYYY-MM-DD to datetime
#-----------------------------------------------------------------------------------------------------------------------------------------
def StrToDatetime(StrDate):
  return datetime.date(int(StrDate[0:4]),int(StrDate[5:7]),int(StrDate[8:10]))

#-----------------------------------------------------------------------------------------------------------------------------------------
#Datetime to string in format 
#-----------------------------------------------------------------------------------------------------------------------------------------
def DatetimeToStr(Date):
  return str(Date.year).zfill(4)+"-"+str(Date.month).zfill(2)+"-"+str(Date.day).zfill(2)

#-----------------------------------------------------------------------------------------------------------------------------------------
#Read commits
#-----------------------------------------------------------------------------------------------------------------------------------------
def ReadCommits(GitBranch,Extensions,Filter,FullList,DateList,MinDate,MaxDate,FirstDays,LastDays,ScreenFit):

  #Set git command
  GitCommand=GITCOMMAND.replace("<branch>",GitBranch).replace("<commitid>",GITCOMMITID)
  
  #Log loop initialization
  LineNr=0
  Totl=0
  Adds=0
  Dels=0
  FileCounters={}
  PrevCommitDate=""
  OldName=""
  NewName=""
  CommitHash=""
  CommitAuth=""
  CommitDate=""
  CommitTime=""
  CommitDesc=""
  FullCommitList=[]
  DateCommitList=[]

  #Read git log
  print("Reading git log...",end="\r",flush=True)
  result = subprocess.check_output(GitCommand,shell=True)
  LogLines=list(result.decode('utf-8','replace').split('\n'))
  
  #Start progress bar
  ProgBarStart(len(LogLines))

  #Git log processing loop
  for Line in LogLines:
    
    #Line count
    LineNr+=1

    #Detect new commit and save line counters
    if Line.startswith(GITCOMMITID):
      
      #Accumulate totals
      Adds,Dels,Totl=CalculateTotals(FileCounters,Extensions,Filter)

      #Save list commit
      if PrevCommitDate!="":
        FullCommitList.append(GitListCommit(CommitHash,CommitAuth,CommitDate,CommitTime,Adds,Dels,Adds-Dels,Totl,CommitDesc))
  
      #Get commit details
      Line=Line.replace("\n","").replace("\r","").strip()
      Fields=Line.split(" ")
      CommitHash=Fields[1]
      CommitAuth=Fields[2]
      CommitDate=Fields[3]
      CommitTime=Fields[4]
      CommitDesc=" ".join(Fields[6:len(Fields)+1])
      CommitDesc=CommitDesc.replace("{}","")
      
      #Save commit when date is different
      if CommitDate!=PrevCommitDate and PrevCommitDate!="":
        DateCommitList.append(GitCumulCommit(StrToDatetime(PrevCommitDate),Totl))
      
      #Print commits read
      ProgBarPrint("Processing commits... ",LineNr-1)
      
      #Reset fields
      for FileName in FileCounters:
        FileCounters[FileName][0]=0
        FileCounters[FileName][1]=0
      OldName=""
      NewName=""
  
      #Keep previous commit date
      PrevCommitDate=CommitDate
  
    #Get file names
    if Line.startswith("diff --git "):
      OldName=Line.replace("diff --git a/","").split(" b/")[0]
      NewName=Line.replace("diff --git a/","").split(" b/")[1]
    elif Line.startswith("rename from "):
      OldName=Line.replace("rename from ","").replace("\n","").replace("\r","").strip()
    elif Line.startswith("--- a/"):
      OldName=Line.replace("--- a/","").replace("\n","").replace("\r","").strip()
    elif Line.startswith("--- /dev/null"):
      OldName="<null>"
    elif Line.startswith("rename to "):
      NewName=Line.replace("rename to ","").replace("\n","").replace("\r","").strip()
    elif Line.startswith("+++ b/"):
      NewName=Line.replace("+++ b/","").replace("\n","").replace("\r","").strip()
    elif Line.startswith("+++ /dev/null"):
      NewName="<null>"
    
    #Detect file creations, deletions and renamings
    if Line.startswith("+++") or Line.startswith("rename to") or Line.startswith("new file mode"):
      if OldName=="<null>" and NewName!="<null>":
        if not NewName in FileCounters:
          FileCounters[NewName]=[0,0,0]
      elif OldName!="<null>" and NewName=="<null>":
        if OldName in FileCounters:
          FileCounters[OldName][1]+=FileCounters[OldName][2]
          FileCounters[OldName][2]=0
      elif OldName!=NewName:
        if OldName in FileCounters:
          Counters=FileCounters[OldName]
          del FileCounters[OldName]
          FileCounters[NewName]=Counters
      else:
        if not NewName in FileCounters:
          FileCounters[NewName]=[0,0,0]

    #Accumulate line counts
    if (Line.startswith('+') and (not Line.startswith("+++"))) \
    or (Line.startswith('-') and (not Line.startswith("---"))):
      if not NewName in FileCounters:
        if NewName!="<null>" and len(NewName)>0:
          print("Detected missing file:"+NewName+" Commit:"+CommitDesc)
          for FileName in FileCounters:
            print("File:"+FileName.ljust(30)+" Adds:"+str(FileCounters[FileName][0]).rjust(10)+"+ Dels:"+str(FileCounters[FileName][1]).rjust(10)+"- Totl:"+str(FileCounters[FileName][0]).rjust(10)+"=")
      else:
        if Line.startswith('+'):
          FileCounters[NewName][0]+=1
          FileCounters[NewName][2]+=1
        elif Line.startswith('-'):
          FileCounters[NewName][1]+=1
          FileCounters[NewName][2]-=1
  
    #Clear line
    Line=""
  
  #Accumulate totals for last commit
  Adds,Dels,Totl=CalculateTotals(FileCounters,Extensions,Filter)
  
  #Save last commit
  FullCommitList.append(GitListCommit(CommitHash,CommitAuth,CommitDate,CommitTime,Adds,Dels,Adds-Dels,Totl,CommitDesc))
  DateCommitList.append(GitCumulCommit(StrToDatetime(CommitDate),Totl))

  #Calculate numbersize (Y-Axis graph) based on max number of lines
  global NumberSize
  NumberSize=len(str(max([Commit.totl for Commit in FullCommitList])))

  #If screenfit option is set we must calculate LastDays
  if(ScreenFit):
    Size=os.get_terminal_size()
    LastDays=Size.columns-NumberSize-5

  #Filter commits according to minimun/maximun dates or last days
  if FirstDays!=0:
    Interval=datetime.timedelta(days=FirstDays)
    MinFilterDate=(FullCommitList[0].date if MinDate=="00000000" else MinDate[0:4]+"-"+MinDate[4:6]+"-"+MinDate[6:8])
    MaxFilterDate=DatetimeToStr(StrToDatetime(MinFilterDate)+Interval)
  elif LastDays!=0:
    Interval=datetime.timedelta(days=LastDays)
    MaxFilterDate=(FullCommitList[-1].date if MaxDate=="99991231" else MaxDate[0:4]+"-"+MaxDate[4:6]+"-"+MaxDate[6:8])
    MinFilterDate=DatetimeToStr(StrToDatetime(MaxFilterDate)-Interval)
  else:
    MaxFilterDate=(FullCommitList[-1].date if MaxDate=="99991231" else MaxDate[0:4]+"-"+MaxDate[4:6]+"-"+MaxDate[6:8])
    MinFilterDate=(FullCommitList[0].date if MinDate=="00000000" else MinDate[0:4]+"-"+MinDate[4:6]+"-"+MinDate[6:8])
  [FullList.append(c) for c in FullCommitList if c.date>=MinFilterDate and c.date<=MaxFilterDate]
  [DateList.append(c) for c in DateCommitList if c.date>=StrToDatetime(MinFilterDate) and c.date<=StrToDatetime(MaxFilterDate)]

#-----------------------------------------------------------------------------------------------------------------------------------------
#Accumulate commits depending on accumulation mode
#-----------------------------------------------------------------------------------------------------------------------------------------
def AccumulateCommits(DateCommitList,TimeScale):
  
  #Accumulate by day
  if TimeScale=="day":
    CumulCommits=DateCommitList

  #Accumulate by week
  elif TimeScale=="week":
    CumulCommits=[]
    DefDict=collections.defaultdict(int)
    for CommitDate,LineCount in DateCommitList:
      DefDict[CommitDate-datetime.timedelta(days=CommitDate.weekday())]=LineCount
    for Key in DefDict:
      CumulCommits.append(GitCumulCommit(Key,DefDict[Key]))

  #Accumulate by month
  elif TimeScale=="month":
    CumulCommits=[]
    DefDict=collections.defaultdict(int)
    for CommitDate,LineCount in DateCommitList:
      DefDict[CommitDate.replace(day=1)]=LineCount
    for Key in DefDict:
      CumulCommits.append(GitCumulCommit(Key,DefDict[Key]))
  
  #Return accomulated list
  return CumulCommits

#-----------------------------------------------------------------------------------------------------------------------------------------
#Print commit list
#-----------------------------------------------------------------------------------------------------------------------------------------
def PrintCommitList(FullCommitList):
  for Commit in FullCommitList:
    WeekDayNr=StrToDatetime(Commit.date).weekday()
    WeekDayStr="MonTueWedThuFriSatSun"[WeekDayNr*3:WeekDayNr*3+3]
    print(Commit.chsh+" "+Commit.auth+" "+Commit.date+" "+WeekDayStr+" "+Commit.time+" "+str(Commit.adds).rjust(NumberSize)+"+ "+str(Commit.dels).rjust(NumberSize)+"- "+str(Commit.lcnt).rjust(NumberSize)+"= "+str(Commit.totl).rjust(NumberSize)+"# "+Commit.desc)

#-----------------------------------------------------------------------------------------------------------------------------------------
#Print commit graph
#-----------------------------------------------------------------------------------------------------------------------------------------
def PrintCommitGraph(DateCommitList,GitBranch,Extensions,Filter,Scale,MarkDots,MaxHeight,Factor,TimeScale,ScreenFit):

  #Constants
  Months=('JAN','FEB','MAR','APR','MAY','JUN','JUL','AUG','SEP','OCT','NOV','DEC')

  #Set lambda functions depending on time scale
  if TimeScale=="day":
    TimeAxis=lambda t,i:t+datetime.timedelta(days=i)
    IsMinorXMark=lambda t:t.day in (1,5,10,15,20,25)
    IsMajorXMark=lambda t:t.day in (1,15)
  elif TimeScale=="week":
    TimeAxis=lambda t,i:t+datetime.timedelta(weeks=i)
    IsMinorXMark=lambda t:t.month!=TimeAxis(t,-1).month
    IsMajorXMark=lambda t:t.month!=TimeAxis(t,-1).month and t.month in (1,3,5,7,9,11)
  elif TimeScale=="month":
    TimeAxis=lambda t,i:t+relativedelta(months=i)
    IsMinorXMark=lambda t:t.month in (1,5,9)
    IsMajorXMark=lambda t:t.month in (1,)

  #Calculate minimun/maximun
  MaxY=0
  MaxDate=DateCommitList[0].date
  MinDate=DateCommitList[0].date
  for Commit in DateCommitList:
    if(Commit.lines>MaxY):
      MaxY=Commit.lines
    if Commit.date>MaxDate:
      MaxDate=Commit.date
    if Commit.date<MinDate:
      MinDate=Commit.date
  
  #Automatic screenfit
  if(ScreenFit):
    Size=os.get_terminal_size()
    MaxHeight=Size.lines-12

  #Calculate scales
  if MaxHeight==0:
    MinorScale=Scale
  else:
    MinorScale=int(int(MaxY/MaxHeight)/Factor)*Factor+Factor
  MajorScale=MinorScale*MarkDots

  #Graph title
  Header=" "*(NumberSize+2)+"  "
  Title="Git line count history (branch:"+GitBranch+", extensions:"
  ExtStr=""
  for Ext in Extensions:
    if len(ExtStr)==0:
      ExtStr=Ext
    else:
      ExtStr+=","+Ext
  FileSet=(", fileset: "+Filter if len(Filter)!=0 else "")
  Title+=ExtStr+FileSet+", scale:"+str(MinorScale)+" lines, total:"+str(DateCommitList[len(DateCommitList)-1].lines)+" lines)"
  print(" "*150)
  print(Header+Title)
  print(" "*len(Header)+"-"*len(Title))
  print(" ")
  
  #Upper line (do not paint if first graph line has a major scale line)
  if int((int(MaxY/MinorScale)*MinorScale)%MajorScale)!=0:
    Line=" "*(NumberSize+1)+"+"
    Date=MinDate
    while Date<=MaxDate:
      Line=Line+"-"
      Date=TimeAxis(Date,1)
    print(Line+"+")
  
  #Grapth loop
  for y in reversed(range(1,int(MaxY/MinorScale)+1)):
  
    #Y-Axis
    Line=str(y*MinorScale).rjust(NumberSize)+" |"
  
    #Calculate line
    Date=MinDate
    while Date<=MaxDate:
      
      #Get number of lines on date
      Commit=[Commit for Commit in DateCommitList if Commit.date==Date]
      
      #Check there is commit on date
      if len(Commit)!=0:
        Lines=Commit[0].lines
        HasCommit=1
      else:
        HasCommit=0
  
      #Calculate dot marks
      if int(Lines/MinorScale)==y:
        Dot=("o" if HasCommit else "-")
      elif int(Lines/MinorScale)>y:
        Dot="."
      else:
        if int((y*MinorScale)%MajorScale)==0 and IsMajorXMark(Date):
          Dot="+"
        elif int((y*MinorScale)%MajorScale)==0:
          Dot="-"
        elif IsMajorXMark(Date):
          Dot="|"
        else:
          Dot=" "
  
      #Print dot on line
      Line=Line+Dot
  
      #Increase loop counter
      Date=TimeAxis(Date,1)
  
    #Output line
    print(Line+"|")
  
  #X-Axis (line)
  Line=" "*(NumberSize+1)+"+"
  Date=MinDate
  while Date<=MaxDate:
    if Date<MaxDate and IsMinorXMark(Date):
      Line=Line+"|"
    else:
      Line=Line+"-"
    Date=TimeAxis(Date,1)
  print(Line+"+")
  
  #Print axis description for day scale
  if TimeScale=="day":

    #X-Axis (days)
    Line=" "*(NumberSize+1)+" "
    Date=MinDate
    while Date<=MaxDate:
      if Date<MaxDate and IsMinorXMark(Date):
        Line=Line+str(Date.day).ljust(2)[0]
      elif IsMinorXMark(TimeAxis(Date,-1)):
        Line=Line+str(TimeAxis(Date,-1).day).ljust(2)[1] 
      else:
        Line=Line+" "
      Date=TimeAxis(Date,1)
    print(Line)
    
    #X-Axis (months)
    Line=" "*(NumberSize+1)+" "
    Date=MinDate
    while Date<=MaxDate:
      if Date.day==1 and Date<=TimeAxis(MaxDate,-2):
        Line=Line+Months[Date.month-1][0]
      elif Date.day==2 and Date<=TimeAxis(MaxDate,-1):
        Line=Line+Months[Date.month-1][1]
      elif Date.day==3 and Date<=MaxDate:
        Line=Line+Months[Date.month-1][2]
      else:
        Line=Line+" "
      Date=TimeAxis(Date,1)
    print(Line)
  
  #Print axis description for week scale
  elif TimeScale=="week":

    #X-Axis (weeks)
    Line=" "*(NumberSize+1)+" "
    Date=MinDate
    while Date<=MaxDate:
      if   IsMinorXMark(TimeAxis(Date,-0)) and TimeAxis(Date,-0)>=MinDate and TimeAxis(Date,+2)<=MaxDate:
        Line=Line+"W"
      elif IsMinorXMark(TimeAxis(Date,-1)) and TimeAxis(Date,-1)>=MinDate and TimeAxis(Date,+1)<=MaxDate:
        Line=Line+str(TimeAxis(Date,-1).isocalendar()[1]).ljust(2)[0]
      elif IsMinorXMark(TimeAxis(Date,-2)) and TimeAxis(Date,-2)>=MinDate and TimeAxis(Date,+0)<=MaxDate:
        Line=Line+str(TimeAxis(Date,-2).isocalendar()[1]).ljust(2)[1] 
      else:
        Line=Line+" "
      Date=TimeAxis(Date,1)
    print(Line)

  #Print axis description for month scale
  elif TimeScale=="month":

    #X-Axis (months)
    Line=" "*(NumberSize+1)+" "
    Date=MinDate
    while Date<=MaxDate:
      if   IsMinorXMark(TimeAxis(Date,-0)) and TimeAxis(Date,-0)>=MinDate and TimeAxis(Date,+2)<=MaxDate:
        Line=Line+Months[Date.month-1][0]
      elif IsMinorXMark(TimeAxis(Date,-1)) and TimeAxis(Date,-1)>=MinDate and TimeAxis(Date,+1)<=MaxDate:
        Line=Line+Months[TimeAxis(Date,-1).month-1][1]
      elif IsMinorXMark(TimeAxis(Date,-2)) and TimeAxis(Date,-2)>=MinDate and TimeAxis(Date,+0)<=MaxDate:
        Line=Line+Months[TimeAxis(Date,-2).month-1][2]
      else:
        Line=Line+" "
      Date=TimeAxis(Date,1)
    print(Line)


  #Legend
  print(" ")
  print("Legend: [o] "+TimeScale.capitalize()+" with commits")
  print("        [-] "+TimeScale.capitalize()+" without commits")

#-----------------------------------------------------------------------------------------------------------------------------------------
#Main program
#-----------------------------------------------------------------------------------------------------------------------------------------
if __name__=="__main__":

  #Init variables
  Options=[]
  FullList=[]
  DateList=[]
  
  #Get command line arguments
  if(GetCommandLineOptions(Options)):
    GitBranch=Options[0]
    Extensions=Options[1]
    Filter=Options[2]
    PrintList=Options[3]
    PrintGraph=Options[4]
    Scale=Options[5]
    MarkDots=Options[6]
    MaxHeight=Options[7]
    Factor=Options[8]
    TimeScale=Options[9]
    MinDate=Options[10]
    MaxDate=Options[11]
    FirstDays=Options[12]
    LastDays=Options[13]
    ScreenFit=Options[14]
  else:
    exit()
  
  #Read commits
  ReadCommits(GitBranch,Extensions,Filter,FullList,DateList,MinDate,MaxDate,FirstDays,LastDays,ScreenFit)
  
  #Print list
  if PrintList==True:
    PrintCommitList(FullList)
  
  #Print graph
  if PrintGraph==True:
    CumulCommits=AccumulateCommits(DateList,TimeScale)
    PrintCommitGraph(CumulCommits,GitBranch,Extensions,Filter,Scale,MarkDots,MaxHeight,Factor,TimeScale,ScreenFit)

#-----------------------------------------------------------------------------------------------------------------------------------------
