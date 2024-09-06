#Run all emlator benchmarks

#Imports
import os
import sys
import subprocess
import statistics as stat
from dataclasses import dataclass

#Constants
HeaderWidth=5
CellWidth=7
DefinedEmulations=9
OptimizOptions={
  "O0N":"",   
  "O1N":"-O1",
  "O2N":"-O2",
  "O3N":"-O3",
  "O0F":"-ffast-math",
  "O1F":"-O1 -ffast-math",
  "O2F":"-O2 -ffast-math",
  "O3F":"-O3 -ffast-math",
}

#Benchmark commands
Commands={
  "WINCMP":"g++ <options> -o usr\\emulator<benchmark><optimiz>.exe usr\\emulator<benchmark>.cpp",
  "WINRUN":"usr\\emulator<benchmark><optimiz>.exe <emulation>",
  "LINCMP":"g++ <options> -o usr/emulator<benchmark><optimiz>.exe usr/emulator<benchmark>.cpp",
  "LINRUN":"usr/emulator<benchmark><optimiz>.exe <emulation>",
}

#Structures to hold benchmark results
@dataclass
class EmulatorResult:
  Emulation:int=0
  Optimiz:str=""
  Time:float=0

#Structure to hold statistics
@dataclass
class EmulatorStatistics:
  Max:float=0
  Min:float=0
  Avg:float=0
  Median:float=0
  StdDev:float=0
  Variance:float=0

#----------------------------------------------------------------------------------------------------------------------
# Show help
#----------------------------------------------------------------------------------------------------------------------
def ShowHelp():
  print("Dungeon Spells emulator benchmark utility - Diego Marin 2022")
  print("Usage: python benchmark.py <testnr> (--linux|--windows) --benchmark:<nr> [--emu:*] [--opt:*]")
  print("<testnr>:          Number of tests to complete")
  print("--benchmark:<nr>:  Number of benchmark to execute (1 or 3)")
  print("--linux:           Benchmarking for linux")
  print("--windows:         Benchmarking for windows")
  print("--emu:*:           Emulation filter (Give a string containing numbers from 1 to "+str(DefinedEmulations))
  print("--opt:*:           Optimization filter (Give a string containing optimizations to test, O0N to O3F")

#----------------------------------------------------------------------------------------------------------------------
# Get command line options
#----------------------------------------------------------------------------------------------------------------------
def GetCommandLineOptions(Options):
  
  #Init config
  TestNr=0
  Benchmark=""
  HostSystem=""
  EmuFilter=""
  OptFilter=""

  #Parse command line
  TestNr=int(sys.argv[1])
  for i in range(2,len(sys.argv)):
    Item=sys.argv[i]
    if Item=="--linux":
      HostSystem="LIN"
    elif Item=="--windows":
      HostSystem="WIN"
    elif Item.startswith("--benchmark:"):
      Benchmark=Item.replace("--benchmark:","")
    elif Item.startswith("--emu:"):
      EmuFilter=Item.replace("--emu:","")
    elif Item.startswith("--opt:"):
      OptFilter=Item.replace("--opt:","")
    else:
      print('Invalid option: '+str(Item))
      return False

  #Must provide number of tests
  if TestNr==0:
    print("Must provide number of tests")
    return False

  #Must provide host system
  if len(HostSystem)==0:
    print("Must provide host system")
    return False

  #Must provide benchmark
  if Benchmark not in ["1","3"]:
    print("Must provide benchmark 1 or 3")
    return False

  #Return configuration and training mode
  Options.append(TestNr)
  Options.append(Benchmark)
  Options.append(HostSystem)
  Options.append(EmuFilter)
  Options.append(OptFilter)
  return True

#-----------------------------------------------------------------------------------------------------------------------------------------
#Execute shell command
#-----------------------------------------------------------------------------------------------------------------------------------------
def Exec(Command,ReturnValue):
  output=subprocess.Popen(Command,shell=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT).stdout.readlines()
  if(ReturnValue==True):
    line = str(output[0]).replace("b'","").replace("'","").replace("\\n","").replace("\\r","")
  else:
    line = ""
  return line

#-----------------------------------------------------------------------------------------------------------------------------------------
#Execute Benchmark command without output
#-----------------------------------------------------------------------------------------------------------------------------------------
def ExecBenchmarkCmdWithoutOutput(Command):
  result = Exec(Command,False)
  if len(result)>0:
    print(result)

#-----------------------------------------------------------------------------------------------------------------------------------------
#Execute Benchmark command with output
#-----------------------------------------------------------------------------------------------------------------------------------------
def ExecBenchmarkCmdWithOutput(Command):
  result = Exec(Command,True)
  return result

#----------------------------------------------------------------------------------------------------------------------
# PrintTable
#----------------------------------------------------------------------------------------------------------------------
def PrintTable(Heading,Formatting,Rows,MaxWidth):

  #Calculate data column widths
  Lengths=[0]*len(Rows[0])
  for Row in Rows:
    if Row!="-":
      i=0
      for Field in Row:
        Lengths[i]=max(max(Lengths[i],len(Field)),len(Heading[i]))
        i+=1

  #Calculate max column to print according to data length
  TableWidth=1
  MaxColumn=0
  i=0
  for Len in Lengths:
    TableWidth+=Len+1
    MaxColumn=i
    if(TableWidth>MaxWidth):
      MaxColumn-=1
      TableWidth-=(Len+1)
      break
    i+=1

  #Separator line
  Separator="-"*TableWidth

  #Print column headings
  print(Separator)
  print("|",end="",flush=True)
  i=0
  for Col in Heading:
    print(Col.ljust(Lengths[i])+"|",end="",flush=True)
    if(i>=MaxColumn):
      break
    i+=1
  print("")
  i=0
  print(Separator)

  #Print data
  LastRowIsSep=False
  for Row in Rows:
    if Row=="-":
      print(Separator)
      LastRowIsSep=True
    else:
      i=0
      print("|",end="",flush=True)
      for Field in Row:
        if Formatting[i]=="n":
          print(Field.rjust(Lengths[i])+"|",end="",flush=True)
        else:
          print(Field.ljust(Lengths[i])+"|",end="",flush=True)
        if(i>=MaxColumn):
          break
        i+=1
      print("")
      LastRowIsSep=False
  if LastRowIsSep==False:
    print(Separator)

  #Column count warning
  if(MaxColumn<len(Lengths)-1):
    WarnMessage="Displaying {0} columns out of {1} columns due to console width".format(str(MaxColumn+1),str(len(Lengths)))
  else:
    WarnMessage=""

  #Warning
  if(len(WarnMessage)!=0):
    print(WarnMessage)    

#-----------------------------------------------------------------------------------------------------------------------------------------
#Main procedure
#-----------------------------------------------------------------------------------------------------------------------------------------

#Show help
if len(sys.argv)==1:
  ShowHelp()
  exit()

#Get command line options
Options=[]
if GetCommandLineOptions(Options)==False:
  exit()
else:
  TestNr=Options[0]
  Benchmark=Options[1]
  HostSystem=Options[2]
  EmuFilter=Options[3]
  OptFilter=Options[4]

#Get console size
if(sys.stdout.isatty()):
  Console=os.get_terminal_size()
  ConsoleWidth=Console.columns-1
else:
  ConsoleWidth=9999

#Calculate emulations selected and maximun
MaxEmulation=0
SelEmulation=0
Emulation=1
while Emulation<=DefinedEmulations:
  if len(EmuFilter)==0 or EmuFilter.find(str(Emulation))!=-1:
    SelEmulation+=1
    MaxEmulation=(Emulation if Emulation>MaxEmulation else MaxEmulation)
  Emulation+=1

#Compile scripts
print("compiling emulator",end="",flush=True)
for Optimiz in OptimizOptions:
  if len(OptFilter)==0 or OptFilter.find(Optimiz)!=-1:
    print(".",end="",flush=True)
    Options=OptimizOptions[Optimiz]
    ExecBenchmarkCmdWithoutOutput(Commands[HostSystem+"CMP"].replace("<benchmark>",Benchmark).replace("<optimiz>",Optimiz).replace("<options>",Options))
print("")

#Benckmark loop
Results=[]
for i in range(TestNr):
  for Optimiz in OptimizOptions:
    if len(OptFilter)==0 or OptFilter.find(Optimiz)!=-1:
      Emulation=1
      while Emulation<=DefinedEmulations:
        if len(EmuFilter)==0 or EmuFilter.find(str(Emulation))!=-1:
          Timming=ExecBenchmarkCmdWithOutput(Commands[HostSystem+"RUN"].replace("<benchmark>",Benchmark).replace("<optimiz>",Optimiz).replace("<emulation>",str(Emulation)))
          Time=float(Timming.replace("EM Benchmark: ","").replace("s",""))
          Test=EmulatorResult()
          Test.Emulation=Emulation
          Test.Optimiz=Optimiz
          Test.Time=Time
          Results.append(Test)
          print(f"Nr={i} Emulation={Emulation}, Optimiz={Optimiz}, Time={Time:.5f}\r",end="")
        Emulation+=1
print(ConsoleWidth*" "+"\r",end="")

#Absolute maximun & minimun
AbsMax=max([x.Time for x in Results])
AbsMin=min([x.Time for x in Results])

#Calculate statistics
Statistics={}
for Optimiz in OptimizOptions:
  if len(OptFilter)==0 or OptFilter.find(Optimiz)!=-1:
    Emulation=1
    while Emulation<=DefinedEmulations:
      if len(EmuFilter)==0 or EmuFilter.find(str(Emulation))!=-1:
        Values=[x.Time for x in Results if x.Emulation==Emulation and x.Optimiz==Optimiz]
        Stats=EmulatorStatistics()
        Stats.Max=max(Values)
        Stats.Min=min(Values)
        Stats.Avg=stat.mean(Values)
        Stats.Median=stat.median(Values)
        Stats.StdDev=stat.pstdev(Values)
        Stats.Variance=stat.pvariance(Values)
        Statistics[Optimiz+"."+str(Emulation)]=Stats
      Emulation+=1

#Print statistics
Rows=[]
for Stats in Statistics:
  Optimiz=Stats.split(".")[0]
  Emulation=Stats.split(".")[1]
  Max=Statistics[Stats].Max
  Min=Statistics[Stats].Min
  Avg=Statistics[Stats].Avg
  Median=Statistics[Stats].Median
  NormMax=100*(AbsMax-Max)/(AbsMax-AbsMin)
  NormMin=100*(AbsMax-Min)/(AbsMax-AbsMin)
  NormAvg=100*(AbsMax-Avg)/(AbsMax-AbsMin)
  NormMedian=100*(AbsMax-Median)/(AbsMax-AbsMin)
  StdDev=Statistics[Stats].StdDev
  Variance=Statistics[Stats].Variance
  Line=[f"{Optimiz}",f"{Emulation}",f"{Max:.7f}",f"{Min:.7f}",f"{Avg:.7f}",f"{Median:.7f}",f"{StdDev:.7f}",f"{Variance:.7f}",f"{NormMax:11.7f}",f"{NormMin:11.7f}",f"{NormAvg:11.7f}",f"{NormMedian:11.7f}"]
  Rows.append(Line)
  if int(Emulation)==MaxEmulation and SelEmulation>1:
    Rows.append("-")
Columns=["Optimiz","Emulation","Max","Min","Avg","Median","StdDev","Variance","NormMax","NormMin","NormAvg","NormMedian"]
Formatting=["-","-","n","n","n","n","n","n","n","n","n","n"]
PrintTable(Columns,Formatting,Rows,ConsoleWidth)