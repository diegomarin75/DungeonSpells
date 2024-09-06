#Configuration option get utility

#Imports
import sys

#Name of configuration file
CONFIG_FILE="ds.config"

#-----------------------------------------------------------------------------------------------------------------------------------------
#Get command line arguments
#-----------------------------------------------------------------------------------------------------------------------------------------
def GetCommandLineOptions(Options):

  #Default options
  ListAll=False
  Option=""

  #No arguemnts given, show help
  if len(sys.argv)<2:
    print("Dungeon Spells configuration utility - Diego Marin 2020")
    print("Usage: python getcfg.py [--list|--get:<option>]")
    print("--list: List all configuration options in file")
    print("option: Get specific configuration option value")
    return False
  
  #Get arguments
  else:
    for i in range(1,len(sys.argv)):
      item=sys.argv[i]
      if item=="--list":
        ListAll=True
      elif item.startswith("--get:"):
        Option=item.replace("--get:","")
      else:
        print("Invalid option: ",item)
        return False

  #Both options cannot be setup at same time
  if ListAll and len(Option)!=0:
    print("Cannot specify at the same time --list and --option")
    return False

  #Return arguments
  Options.append(ListAll)
  Options.append(Option)

  #Return code
  return True

#-----------------------------------------------------------------------------------------------------------------------------------------
#Get configuration file options
#-----------------------------------------------------------------------------------------------------------------------------------------
def GetConfigFileOptions(Config):

  #Open file
  CfgFile=open(CONFIG_FILE,"r")

  #Read lines
  for Line in CfgFile:
    if not Line.strip().startswith("#") and len(Line.strip())>0 and Line.strip().find("=")!=-1:
      Line=Line.replace("\n","").strip()
      Option=Line.split("=")[0].strip()
      Option=(Option[1:].strip() if Option.startswith("[") else Option)
      Option=(Option[:-1].strip() if Option.endswith("]") else Option)
      Value=Line.split("=")[1].strip()
      Value=(Value[1:].strip() if Value.startswith("\"") else Value)
      Value=(Value[:-1].strip() if Value.endswith("\"") else Value)
      Config[Option]=Value
  
  #Close file
  CfgFile.close()

#-----------------------------------------------------------------------------------------------------------------------------------------
#Print configuration file single option
#-----------------------------------------------------------------------------------------------------------------------------------------
def PrintConfigFileSingleOption(Option):
  Config={}
  GetConfigFileOptions(Config)
  print(Config[Option])

#-----------------------------------------------------------------------------------------------------------------------------------------
#Print configuration file all options
#-----------------------------------------------------------------------------------------------------------------------------------------
def PrintConfigFileAllOptions():
  Config={}
  GetConfigFileOptions(Config)
  for key in Config:
    print(key+" = "+Config[key])

#-----------------------------------------------------------------------------------------------------------------------------------------
#Main program
#-----------------------------------------------------------------------------------------------------------------------------------------
if __name__=="__main__":
  
  #Init variables
  Options=[]
  
  #Get command line arguments
  if(GetCommandLineOptions(Options)):
    ListAll=Options[0]
    Option=Options[1]
  else:
    exit()
  
  #List all configuation options
  if ListAll:
    PrintConfigFileAllOptions()
  
  #List specific option
  else:
    PrintConfigFileSingleOption(Option)

#-----------------------------------------------------------------------------------------------------------------------------------------