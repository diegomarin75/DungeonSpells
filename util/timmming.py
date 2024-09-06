#Runtime perf annotate file analysis

#Imports
import sys
import subprocess

#Contants
ANNOTATION_FILE="perf.txt"

#Commands
Commands={
  "COMPILE_PROJECT":"python util/project.py --build --linux --rel --64 --app:dunr --perf",
  "COMPILE_SCRIPT" :"./dunc <script>.ds",
  "PERF_EXECUTE"   :"perf record -g -e cycles,instructions,cache-misses ./dunr <script>.dex",
  "PERF_ANNOTATE"  :"perf annotate --stdio --show-total-period --symbol=Runtime::_RunProgram > <annotationfile>",
  "PERF_CLEAN1"    :"rm perf.data",
  "PERF_CLEAN2"    :"rm <annotationfile>"
}

#-----------------------------------------------------------------------------------------------------------------------------------------
#Get command line arguments
#-----------------------------------------------------------------------------------------------------------------------------------------
def GetCommandLineOptions(Options):

  #Default options
  ScriptFile=""
  Recompile=False
  KeepAnnotation=False

  #No arguemnts given, show help
  if len(sys.argv)<2:
    print("Dungeon Spells runtime analysis - Diego Marin 2022")
    print("Usage: sudo python timming.py <script> [--recompile]")
    print("<script>    : DS script file (must have .ds extension")
    print("--recompile : Do recompilation of rutime")
    print("--keep      : Do not delete anotation file")
    return False
  
  #Get arguments
  else:
    ScriptFile=sys.argv[1]
    for i in range(2,len(sys.argv)):
      item=sys.argv[i]
      if item=="--recompile":
        Recompile=True
      elif item=="--keep":
        KeepAnnotation=True
      else:
        print("Invalid option: "+item)
        return False
  
  #Return arguments
  Options.append(ScriptFile)
  Options.append(Recompile)
  Options.append(KeepAnnotation)

  #Return code
  return True

#-----------------------------------------------------------------------------------------------------------------------------------------
#Execute command
#-----------------------------------------------------------------------------------------------------------------------------------------
def Exec(Command):
  subprocess.run(Command,shell=True)

#-----------------------------------------------------------------------------------------------------------------------------------------
#Execute test
#-----------------------------------------------------------------------------------------------------------------------------------------
def ExecuteTest(ScriptFile,Recompile):

  #Get script file
  Script=(ScriptFile.replace(".ds","") if ScriptFile.endswith(".ds") else ScriptFile)

  #Compile runtime
  if Recompile==True:
    Exec(Commands["COMPILE_PROJECT"])

  #Compile script
  Exec(Commands["COMPILE_SCRIPT"].replace("<script>",Script))

  #Perf execute
  Exec(Commands["PERF_EXECUTE"].replace("<script>",Script))

  #Perf annotate
  Exec(Commands["PERF_ANNOTATE"].replace("<annotationfile>",ANNOTATION_FILE))

#-----------------------------------------------------------------------------------------------------------------------------------------
#Execute clean
#-----------------------------------------------------------------------------------------------------------------------------------------
def ExecuteClean(KeepAnnotation):

  #Clean perf file
  Exec(Commands["PERF_CLEAN1"])

  #Perf annotate
  if KeepAnnotation==False:
    Exec(Commands["PERF_CLEAN2"].replace("<annotationfile>",ANNOTATION_FILE))

#-----------------------------------------------------------------------------------------------------------------------------------------
#Analyze timming
#-----------------------------------------------------------------------------------------------------------------------------------------
def AnalyzeTimming(AnnotationFile):

  #File read
  File=open(AnnotationFile,"r")
  Lines=File.readlines()
  File.close()

  #Timming counters
  OpenLabels={}
  Labels={}
  Total=0

  #Process lines
  for Line in Lines:
    
    #Discard lines without timming
    if Line.find(":")==-1:
      continue

    #Get timming and label parts
    TimePart=Line.split(":")[0].strip()
    LabelPart=Line.split(":")[1].strip()

    #Label open
    if LabelPart.find("[$$")!=-1 and LabelPart.find("_Start]")!=-1:
      Label=LabelPart[LabelPart.find("[$$")+3:LabelPart.find("_Start]")]
      if Label.startswith("INST_"):
        for InstLabel in OpenLabels:
          if InstLabel.startswith("INST_"):
            if InstLabel not in Labels:
              Labels[InstLabel]=0
            Labels[InstLabel]+=OpenLabels[InstLabel]
        OpenLabels={Key:Val for Key,Val in OpenLabels.items() if Key.startswith("INST_")==False}
      OpenLabels[Label]=0

    #Accumulate time in mot inner label
    if len(TimePart)!=0 and int(TimePart)!=0:
      InnerLabel=list(OpenLabels)[-1]
      OpenLabels[InnerLabel]+=int(TimePart)
      Total+=int(TimePart)

    #Label close
    if LabelPart.find("[$$")!=-1 and LabelPart.find("_End]")!=-1:
      Label=LabelPart[LabelPart.find("[$$")+3:LabelPart.find("_End]")]
      if Label not in Labels:
        Labels[Label]=0
      Labels[Label]+=OpenLabels[Label]
      del OpenLabels[Label]

  #Add time for all open labels
  for Label in OpenLabels:
    print(f"Detected open label {Label} with time {OpenLabels[Label]}ns")
    if Label not in Labels:
      Labels[Label]=0
    Labels[Label]+=OpenLabels[Label]

  #Return results
  return Total,Labels

#-----------------------------------------------------------------------------------------------------------------------------------------
#Print timmings
#-----------------------------------------------------------------------------------------------------------------------------------------
def PrintTimmings(Total,Labels):
 
  #Sort labels
  LabelsSorted=dict(sorted(Labels.items(),key=lambda item:item[1],reverse=True))

  #Calculate total
  TotalLabels=0
  for Label in LabelsSorted:
    TotalLabels+=Labels[Label]

  #Print decoder labels
  print("\n-- Instruction decoding: --")
  PartialTotal=0
  for Label in LabelsSorted:
    if Labels[Label]!=0 and Label.startswith("DECODE"):
      print(Label.ljust(20)+": "+f"{Labels[Label]/1000000000:.5f}s {100*(Labels[Label]/Total):.2f}%")
      PartialTotal+=Labels[Label]
  print("Subtotal".ljust(20)+f": {PartialTotal/1000000000:.5f}s {100*(PartialTotal/Total):.2f}%")

  #Print instruction labels
  print("\n-- Instruction execution: --")
  PartialTotal=0
  for Label in LabelsSorted:
    if Labels[Label]!=0 and Label.startswith("INST_"):
      print(Label.ljust(20)+": "+f"{Labels[Label]/1000000000:.5f}s {100*(Labels[Label]/Total):.2f}%")
      PartialTotal+=Labels[Label]
  print("Subtotal".ljust(20)+f": {PartialTotal/1000000000:.5f}s {100*(PartialTotal/Total):.2f}%")

  #Print rest of labels
  print("\n-- Instruction loop: --")
  PartialTotal=0
  for Label in LabelsSorted:
    if Labels[Label]!=0 and Label.startswith("DECODE")==False and Label.startswith("INST_")==False:
      print(Label.ljust(20)+": "+f"{Labels[Label]/1000000000:.5f}s {100*(Labels[Label]/Total):.2f}%")
      PartialTotal+=Labels[Label]
  print("Subtotal".ljust(20)+f": {PartialTotal/1000000000:.5f}s {100*(PartialTotal/Total):.2f}%")

  #Print total
  print()
  print("TOTAL EXECUTION".ljust(20)+f": {Total/1000000000:.5f}s")

  #Print open labels in case totals donot match (since there must be an error comming from this)
  if Total!=TotalLabels:
    print("Calculated total for labels does not match by "+str((Total-TotalLabels))+"ns!")

#-----------------------------------------------------------------------------------------------------------------------------------------
#Main program
#-----------------------------------------------------------------------------------------------------------------------------------------

#Init variables
Options=[]

#Get command line arguments
if(GetCommandLineOptions(Options)):
  ScriptFile=Options[0]
  Recompile=Options[1]
  KeepAnnotation=Options[2]
else:
  exit()

#Execute test
ExecuteTest(ScriptFile,Recompile)

#Analyze timmings
Total,Labels=AnalyzeTimming(ANNOTATION_FILE)

#Print timmings
PrintTimmings(Total,Labels)

#Cleanup files
ExecuteClean(KeepAnnotation)

