#Run all benchmarks

#Imports
import os
import sys
import subprocess
from dataclasses import dataclass,field
from typing import Dict

#Constants
BenchmarkNr=3
HeaderWidth=5
CellWidth=7
Instructions=[60070020,85471999,85211886]
Wheel=["|","/","-","\\"]

#Benchmarking programming languages
#Categories: nt=Executed as native machine code, vr=Executed on virtual machine, in=Purely interpreted
ProgLangs={
# Lang  Categ Desc
  "co":["nt", "c++ with O3 optimization"           ],
  "cp":["nt", "c++ without optimization"           ],
  "cs":["nt", ".net/mono c#"                       ],
  "jv":["nt", "Java"                               ],
  "js":["nt", "JavaScript (node)"                  ],
  "lj":["nt", "LuaJit"                             ],
  "pp":["nt", "PyPy"                               ],
  "em":["v1", "DS Emulator"                        ],
  "lo":["v1", "LuaJit interp. mode (-joff)"        ],
  "ds":["v1", "Dungeon Spells"                     ],
  "dx":["v1", "Dungeon Spells (alt. version)"      ],
  "lu":["v1", "Lua script language"                ],
  "jo":["v1", "JS ignition interp. (node --no-opt)"],
  "ph":["v1", "Php"                                ],
  "rl":["v2", "R Language"                         ],
  "pl":["v2", "Perl"                               ],
  "py":["v2", "Python"                             ],
  "ps":["in", "PowerShell"                         ]
}

#Benchmark commands
Commands={
  "WINCOMCO":"g++ -O3 -ffast-math -o usr\\benchmark<number>co.exe usr\\benchmark<number>.cpp",
  "WINCOMCP":"g++ -o usr\\benchmark<number>cp.exe usr\\benchmark<number>.cpp",
  "WINCOMCS":"csc -nologo -out:usr\\benchmark<number>cs.exe usr\\benchmark<number>.cs",
  "WINCOMEM":"g++ -O3 -ffast-math -o usr\\emulator<number>.exe usr\\emulator<number>.cpp",
  
  "LINCOMCO":"g++ -O3 -ffast-math -o usr/benchmark<number>co.exe usr/benchmark<number>.cpp",
  "LINCOMCP":"g++ -o usr/benchmark<number>cp.exe usr/benchmark<number>.cpp",
  "LINCOMCS":"csc -nologo -out:usr/benchmark<number>cs.exe usr/benchmark<number>.cs",
  "LINCOMEM":"g++ -O3 -ffast-math -o usr/emulator<number>.exe usr/emulator<number>.cpp",
  
  "WINRUNCO":"usr\\benchmark<number>co.exe",
  "WINRUNCP":"usr\\benchmark<number>cp.exe",
  "WINRUNCS":"usr\\benchmark<number>cs.exe",
  "WINRUNJV":"java usr\\benchmark<number>.java",
  "WINRUNJS":"node usr\\benchmark<number>.js",
  "WINRUNJO":"node --no-opt usr\\benchmark<number>.js",
  "WINRUNLJ":"luajit usr\\benchmark<number>.lua",
  "WINRUNDS":"duns usr\\benchmark<number>.ds",
  "WINRUNDX":"c:duns usr\\benchmark<number>.ds",
  "WINRUNPY":"python usr\\benchmark<number>.py",
  "WINRUNPP":"pypy usr\\benchmark<number>.py",
  "WINRUNPH":"php usr\\benchmark<number>.php",
  "WINRUNLU":"lua54 usr\\benchmark<number>.lua",
  "WINRUNEM":".\\usr\\emulator<number>.exe 8",
  "WINRUNLO":"luajit -joff usr\\benchmark<number>.lua",
  "WINRUNPL":"perl usr\\benchmark<number>.pl",
  "WINRUNRL":"rscript usr\\benchmark<number>.r",
  "WINRUNPS":"pwsh -file usr\\benchmark<number>.ps1",
  
  "WINVERCO":["g++ --version",2],
  "WINVERCP":["g++ --version",2],
  "WINVERCS":["csc /help",1],
  "WINVERJV":["java --version",1],
  "WINVERJS":["node --version",1],
  "WINVERJO":["node --version",1],
  "WINVERLJ":["luajit -v",1],
  "WINVERDS":["duns -ve",1],
  "WINVERDX":["c:duns -ve",1],
  "WINVERPY":["python --version",1],
  "WINVERPP":["pypy --version",2],
  "WINVERPH":["php --version",1],
  "WINVERLU":["lua54 -v",1],
  "WINVEREM":["echo v0.0",1],
  "WINVERLO":["luajit -v",1],
  "WINVERPL":["perl --version",1],
  "WINVERRL":["rscript --version",1],
  "WINVERPS":["pwsh -v",1],
  
  "LINRUNCO":"usr/benchmark<number>co.exe",
  "LINRUNCP":"usr/benchmark<number>cp.exe",
  "LINRUNCS":"mono usr/benchmark<number>cs.exe",
  "LINRUNJV":"java usr/benchmark<number>.java",
  "LINRUNJS":"node usr/benchmark<number>.js",
  "LINRUNJO":"node --no-opt usr/benchmark<number>.js",
  "LINRUNLJ":"luajit usr/benchmark<number>.lua",
  "LINRUNDS":"duns usr/benchmark<number>.ds",
  "LINRUNDX":"../DungeonSpells/duns usr/benchmark<number>.ds",
  "LINRUNPY":"/usr/bin/python3 usr/benchmark<number>.py",
  "LINRUNPP":"pypy usr/benchmark<number>.py",
  "LINRUNPH":"php usr/benchmark<number>.php",
  "LINRUNLU":"lua54 usr/benchmark<number>.lua",
  "LINRUNEM":"usr/emulator<number>.exe 8",
  "LINRUNLO":"luajit -joff usr/benchmark<number>.lua",
  "LINRUNPL":"perl usr/benchmark<number>.pl",
  "LINRUNRL":"Rscript usr/benchmark<number>.r",
  "LINRUNPS":"pwsh -file usr/benchmark<number>.ps1",

  "LINVERCO":["g++ --version",2],
  "LINVERCP":["g++ --version",2],
  "LINVERCS":["mono --version",1],
  "LINVERJV":["java --version",1],
  "LINVERJS":["node --version",1],
  "LINVERJO":["node --version",1],
  "LINVERLJ":["luajit -v",1],
  "LINVERDS":["duns -ve",1],
  "LINVERDX":["../DungeonSpells/duns -ve",1],
  "LINVERPY":["/usr/bin/python3 --version",1],
  "LINVERPP":["pypy --version",2],
  "LINVERPH":["php --version",1],
  "LINVERLU":["lua54 -v",1],
  "LINVEREM":["echo v0.0",1],
  "LINVERLO":["luajit -v",1],
  "LINVERPL":["perl --version",1],
  "LINVERRL":["Rscript --version",1],
  "LINVERPS":["pwsh -v",1],

}

#Structures to hold benchmark results
@dataclass
class LangTest:
  Benchmark:int=0
  Time:Dict=field(default_factory=dict)
  Mips:Dict=field(default_factory=dict)
  Rate:Dict=field(default_factory=dict)

#----------------------------------------------------------------------------------------------------------------------
# Show help
#----------------------------------------------------------------------------------------------------------------------
def ShowHelp():
  HostSystem=("WIN" if os.name=="nt" else "LIN")
  print("Dungeon Spells benchmark utility - Diego Marin 2020")
  print("Usage: python benchmark.py <testnr> (--linux|--windows) [--comp:*] [--langs:*] [--nocompile]")
  print("<testnr>:    Number of tests to complete")
  print("--linux:     Benchmarking for linux")
  print("--windows:   Benchmarking for windows")
  print("--bench:*    Benchmarking tests filter separated by comma (1,2 and/or 3)")
  print("--comp:*     Programming language for comparison (only one)")
  print("--langs:*    Programming language filter separated by comma")
  print("--skip:*     Skip programming languages, separated by comma")
  print("--categ:*    Languaje filter by categories separated by comma (nt=native machine code, v1=fast virtual machines, v2=slow virtual machines, in=purely interpreted)")
  print("--nocompile: Skip compilation of scripts")
  print("")
  print("Languages:")
  MaxLen=0
  for Lang in ProgLangs:
    MaxLen=(len(ProgLangs[Lang][1]) if len(ProgLangs[Lang][1])>MaxLen else MaxLen)
  for Lang in ProgLangs:
    print(Lang+" = "+ProgLangs[Lang][1].ljust(MaxLen)+"  ["+GetLangVersion(HostSystem,Lang)+"]")
  exit()

#----------------------------------------------------------------------------------------------------------------------
# Get command line options
#----------------------------------------------------------------------------------------------------------------------
def GetCommandLineOptions(Options):
  
  #Init config
  TestNr=0
  HostSystem=""
  BenchFilter=""
  LangFilter=""
  CategFilter=""
  SkipFilter=""
  CompLang="co"
  Compile=True

  #Parse command line
  TestNr=int(sys.argv[1])
  for i in range(2,len(sys.argv)):
    Item=sys.argv[i]
    if Item=="--linux":
      HostSystem="LIN"
    elif Item=="--windows":
      HostSystem="WIN"
    elif Item.startswith("--bench:"):
      BenchFilter=Item.replace("--bench:","")
    elif Item.startswith("--comp:"):
      CompLang=Item.replace("--comp:","")
    elif Item.startswith("--langs:"):
      LangFilter=Item.replace("--langs:","")
    elif Item.startswith("--skip:"):
      SkipFilter=Item.replace("--skip:","")
    elif Item.startswith("--categ:"):
      CategFilter=Item.replace("--categ:","")
    elif Item=="--nocompile":
      Compile=False
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

  #Calculate lang filter from category filter
  if len(CategFilter)!=0:
    CategLangs=",".join([Lang for Lang in ProgLangs if CategFilter.find(ProgLangs[Lang][0])!=-1])
    if len(LangFilter)!=0:
      LangFilter+=","+CategLangs
      LangFilter=",".join(set(LangFilter.split(",")))
    else:
      LangFilter=CategLangs

  #Calculate selected langs by skip filter
  if len(SkipFilter)!=0:
    if len(LangFilter)!=0:
      LangFilter=",".join([Lang for Lang in LangFilter.split(",") if SkipFilter.find(Lang)==-1])
    else:
      LangFilter=",".join([Lang for Lang in ProgLangs if SkipFilter.find(Lang)==-1])
  
  #Return configuration and training mode
  Options.append(TestNr)
  Options.append(HostSystem)
  Options.append(BenchFilter)
  Options.append(LangFilter)
  Options.append(CompLang)
  Options.append(Compile)
  return True


#-----------------------------------------------------------------------------------------------------------------------------------------
#Get lang version
#-----------------------------------------------------------------------------------------------------------------------------------------
def GetLangVersion(HostSystem,Lang):
  CommandName=HostSystem+"VER"+Lang.upper()
  VersionString=""
  if CommandName in Commands:
    Command=Commands[CommandName][0]
    VersionLines=Commands[CommandName][1]
    Output=subprocess.Popen(Command,shell=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT).stdout.readlines()
    LineCount=1
    for Line in Output:
      Line = str(Line).replace("b'","").replace("'","").replace("\\n","").replace("\\r","")
      if len(Line.strip())!=0:
        VersionString+=(" " if len(VersionString)!=0 else "")+str(Line)
        if LineCount>=VersionLines:
          break
        LineCount+=1
  return VersionString    

#-----------------------------------------------------------------------------------------------------------------------------------------
#Execute shell command
#-----------------------------------------------------------------------------------------------------------------------------------------
def Exec(Command,ReturnValue):
  Output=subprocess.Popen(Command,shell=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT).stdout.readlines()
  if(ReturnValue==True):
    line = str(Output[0]).replace("b'","").replace("'","").replace("\\n","").replace("\\r","")
  else:
    line = ""
  return line

#-----------------------------------------------------------------------------------------------------------------------------------------
#Execute Benchmark command without output
#-----------------------------------------------------------------------------------------------------------------------------------------
def ExecBenchmarkCmdWithoutOutput(CommandId,BenchmarkNumber):
  result = Exec(Commands[CommandId].replace("<number>",str(BenchmarkNumber)),False)
  if len(result)>0:
    print(result)

#-----------------------------------------------------------------------------------------------------------------------------------------
#Execute Benchmark command with output
#-----------------------------------------------------------------------------------------------------------------------------------------
def ExecBenchmarkCmdWithOutput(CommandId,BenchmarkNumber):
  result = Exec(Commands[CommandId].replace("<number>",str(BenchmarkNumber)),True)
  result = (result.split("\n")[0] if result.find("\n")!=-1 else result)
  for Lang in ProgLangs:
    result=result.replace(Lang.upper()+" Benchmark","")
  return result.replace("s","").replace(":","").replace(" ","")

#-----------------------------------------------------------------------------------------------------------------------------------------
#Calculate rates and mips
#-----------------------------------------------------------------------------------------------------------------------------------------
def CalculateRatesMips(Test,LangFilter,CompLang):
  for Lang in ProgLangs:
    if len(LangFilter)==0 or LangFilter.find(Lang)!=-1:
      Test.Mips[Lang] =(Instructions[Test.Benchmark-1]/Test.Time[Lang])/1000000
      Test.Rate[Lang]=Test.Time[Lang]/Test.Time[CompLang]

#-----------------------------------------------------------------------------------------------------------------------------------------
#Print result
#-----------------------------------------------------------------------------------------------------------------------------------------
def PrintResult(Label,Result,CompLang):
  print()
  print(Label.ljust(8)+"Lang     | ",end="",flush=True)
  for Lang in Result.Time:
    print((" "*5)+Lang+(" "*2),end="",flush=True)
  print()
  print((" "*8)+"Time(s)  | ",end="",flush=True)
  for Lang in Result.Time:
    print(("{0:7.5f}".format(float(Result.Time[Lang])))[:7]+"  ",end="",flush=True)
  print()
  print((" "*8)+"Rate %"+CompLang+" | ",end="",flush=True)
  for Lang in Result.Rate:
    print(("{0:7.2f}".format(float(Result.Rate[Lang])))[:7]+"  ",end="",flush=True)
  print()
  print((" "*8)+"Mips     | ",end="",flush=True)
  for Lang in Result.Mips:
    print(("{0:7.2f}".format(float(Result.Mips[Lang])))[:7]+"  ",end="",flush=True)
  print()

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
  HostSystem=Options[1]
  BenchFilter=Options[2]
  LangFilter=Options[3]
  CompLang=Options[4]
  Compile=Options[5]

#Get console size
if(sys.stdout.isatty()):
  Console=os.get_terminal_size()
  ConsoleWidth=Console.columns-1
else:
  ConsoleWidth=9999

#Compile scripts
if Compile==True:
  if LangFilter.find("co")!=-1 or LangFilter.find("cp")!=-1:
    print("compiling c++ scripts",end="",flush=True)
    for Benchmark in range(1,BenchmarkNr+1):
      if len(BenchFilter)==0 or BenchFilter.find(str(Benchmark))!=-1:
        print(".",end="",flush=True)
        for Command in Commands:
          if Command.startswith(HostSystem+"COMCO"):
            ExecBenchmarkCmdWithoutOutput(Command,Benchmark)
          if Command.startswith(HostSystem+"COMCP"):
            ExecBenchmarkCmdWithoutOutput(Command,Benchmark)
    print("")
  if LangFilter.find("cs")!=-1:
    print("compiling c# scripts",end="",flush=True)
    for Benchmark in range(1,BenchmarkNr+1):
      if len(BenchFilter)==0 or BenchFilter.find(str(Benchmark))!=-1:
        print(".",end="",flush=True)
        for Command in Commands:
          if Command.startswith(HostSystem+"COMCS"):
            ExecBenchmarkCmdWithoutOutput(Command,Benchmark)
    print("")
  if LangFilter.find("em")!=-1:
    print("compiling emulator scripts",end="",flush=True)
    for Benchmark in range(1,BenchmarkNr+1):
      if len(BenchFilter)==0 or BenchFilter.find(str(Benchmark))!=-1:
        print(".",end="",flush=True)
        for Command in Commands:
          if Command.startswith(HostSystem+"COMEM"):
            ExecBenchmarkCmdWithoutOutput(Command,Benchmark)
    print("")

#Calculate total iterations
Iterations=0
for i in range(TestNr):
  for Benchmark in range(1,BenchmarkNr+1):
    if len(BenchFilter)==0 or BenchFilter.find(str(Benchmark))!=-1:
      for Lang in ProgLangs:
        if len(LangFilter)==0 or LangFilter.find(Lang)!=-1:
          Iterations+=1

#Benckmark loop
Results=[]
WheelNr=0
Iter=0
for i in range(TestNr):
  for Benchmark in range(1,BenchmarkNr+1):
    if len(BenchFilter)==0 or BenchFilter.find(str(Benchmark))!=-1:
      Test=LangTest()
      Test.Benchmark=Benchmark
      for Lang in ProgLangs:
        if len(LangFilter)==0 or LangFilter.find(Lang)!=-1:
          Percentage=f"{int(100*Iter/Iterations):02d}"
          print(Percentage+"% ["+Lang+" "+Wheel[WheelNr]+"]\r",end="",flush=True)
          Test.Time[Lang]=float(ExecBenchmarkCmdWithOutput(HostSystem+"RUN"+Lang.upper(),Benchmark))
          Iter+=1
          WheelNr+=1
          if WheelNr>3:
            WheelNr=0
      CalculateRatesMips(Test,LangFilter,CompLang)
      Timmings=""
      for Lang in ProgLangs:
        if len(LangFilter)==0 or LangFilter.find(Lang)!=-1:
          Timmings+=(" " if len(Timmings)!=0 else "")+f"{Lang}"+"="+(f"{Test.Time[Lang]:.3f}")[:6]
      print(Percentage+"% [.. "+Wheel[WheelNr]+"] "+str(i)+"/"+str(Benchmark)+": "+Timmings+"\r",end="",flush=True)
      Results.append(Test)
print(ConsoleWidth*" "+"\r",end="",flush=True)

#Calculate separator
Separator=""
for Lang in ProgLangs:
  if len(LangFilter)==0 or LangFilter.find(Lang)!=-1:
    Separator+=("-"*9)
Separator=Separator[:-2]

#Calculate minimun,maximun and average
for Benchmark in range(1,BenchmarkNr+1):
  if len(BenchFilter)==0 or BenchFilter.find(str(Benchmark))!=-1:
    print("\n[  Benchmark "+f"{Benchmark:02d}"+"  ] "+Separator)
    Max=LangTest()
    Min=LangTest()
    Avg=LangTest()
    Max.Benchmark=Benchmark
    Min.Benchmark=Benchmark
    Avg.Benchmark=Benchmark
    for Lang in ProgLangs:
      if len(LangFilter)==0 or LangFilter.find(Lang)!=-1:
        Max.Time[Lang]=max([Test.Time[Lang] for Test in Results if Test.Benchmark==Benchmark])
        Min.Time[Lang]=min([Test.Time[Lang] for Test in Results if Test.Benchmark==Benchmark])
        Avg.Time[Lang]=sum([Test.Time[Lang] for Test in Results if Test.Benchmark==Benchmark])/TestNr
    CalculateRatesMips(Max,LangFilter,CompLang)
    CalculateRatesMips(Min,LangFilter,CompLang)
    CalculateRatesMips(Avg,LangFilter,CompLang)
    PrintResult("[Worst]",Max,CompLang)
    PrintResult("[Best] ",Min,CompLang)
    PrintResult("[Avg]  ",Avg,CompLang)