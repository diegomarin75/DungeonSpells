#font2c.py:Generate C source font data (based on: https://github.com/rogerdahl/font-to-c)
import sys
import freetype

#Constants
TAB_STR = ' ' * 2
NAMESPACE='Fnt'
FONTINDEX_TYPENAME = 'FontIndex'

#-----------------------------------------------------------------------------------------------------------------------------------------
# Get command line arguments
#-----------------------------------------------------------------------------------------------------------------------------------------
def GetCommandLineOptions(Options):

  #Default options
  FontFile=""
  PixelSize=8
  StartChar=32
  EndChar=126
  CodeName=""
  OutFile=""
  Append=False
  DefChar=32

  #No arguemnts given, show help
  if len(sys.argv)<3:
    print("Font to C-Source conversion utility - Diego Marin 2020")
    print("Usage: python font2c.py --font:<filename> --size:<pixelsize> [--start:<start>] [--end:<end>] [--codename:<codename>] [--outfile:<outfile>] [--default:<defchar>] [--append]")
    print("filename:  Font file name")
    print("pixelsize: Rendering size of characters")
    print("start:     Ascii number of first char to extract (default: "+str(StartChar)+")")
    print("end:       Ascii number of last char to extract (default: "+str(EndChar)+")")
    print("codename:  Name of font variables inside source (default: Font name used)")
    print("outfile:   Output file name (default: ./Fnt<codename>.c")
    print("defchar:   Default char number to take when glyph is missing (default: "+str(DefChar)+")")
    print("append:    Appends output to existing file (default: "+("true" if Append else "false")+")")
    return False
  
  #Get arguments
  else:
    for i in range(1,len(sys.argv)):
      item=sys.argv[i].strip()
      if item.startswith("--font:"):
        FontFile=item.replace("--font:","")
      elif item.startswith("--size:"):
        PixelSize=int(item.replace("--size:",""))
      elif item.startswith("--start:"):
        StartChar=int(item.replace("--start:",""))
      elif item.startswith("--end:"):
        EndChar=int(item.replace("--end:",""))
      elif item.startswith("--codename:"):
        CodeName=item.replace("--codename:","")
      elif item.startswith("--outfile:"):
        OutFile=item.replace("--outfile:","")
      elif item.startswith("--default:"):
        DefChar=int(item.replace("--default:",""))
      elif item=="--append":
        Append=True
      else:
        print("Invalid option: ",item)
        return False
  
  #Return arguments
  Options.append(FontFile)
  Options.append(PixelSize)
  Options.append(StartChar)
  Options.append(EndChar)
  Options.append(CodeName)
  Options.append(OutFile)
  Options.append(DefChar)
  Options.append(Append)

  #Return code
  return True

#-----------------------------------------------------------------------------------------------------------------------------------------
# C-Source generator class
#-----------------------------------------------------------------------------------------------------------------------------------------
class ClangGenerator(object):

    #Init
    def __init__(self,FontFile,PixelSize,StartChar,EndChar,CodeName,OutFile,DefChar,Append):
        self.PixelSize = PixelSize
        self.FirstChar = chr(int(StartChar))
        self.LastChar = chr(int(EndChar))
        self.CodeName = CodeName
        self.OutFile = OutFile
        self.DefChar = DefChar
        self.face = freetype.Face(FontFile)
        self.face.set_pixel_sizes(0, PixelSize)
        self.c_file = open(self._getSourceName(),('a' if Append else 'w'))
        self.generateCSource(Append)
        self.c_file.close()

    #Generate source file
    def generateCSource(self,Append):
        monospace,height,width = self._getFontWidthHeight()
        if(not Append):
          self.c_file.write('//Auto generated file with tool font2c.py - Do not edit manually!\n')
          self.c_file.write('\n')
        self.c_file.write('// '.ljust(85,'-')+'\n')
        self.c_file.write('// Font definition: {} (codename: {})'.format(self._getFriendlyName(),self._getSafeName()).ljust(85)+'\n')
        self.c_file.write('// '.ljust(85,'-')+'\n')
        self.c_file.write('\n')
        if(not Append):
          self.c_file.write('//Font index structure\n')
          self.c_file.write('struct '+FONTINDEX_TYPENAME+'{\n')
          self.c_file.write('{}int Offset;\n'.format(TAB_STR))
          self.c_file.write('{}int Width;\n'.format(TAB_STR))
          self.c_file.write('{}int Height;\n'.format(TAB_STR))
          self.c_file.write('{}int Left;\n'.format(TAB_STR))
          self.c_file.write('{}int Top;\n'.format(TAB_STR))
          self.c_file.write('{}int Advance;\n'.format(TAB_STR))
          self.c_file.write('};\n')
          self.c_file.write('\n')
        self.c_file.write('//Font {} constants\n'.format(self._getFriendlyName()))
        self.c_file.write('const char *_{}CodeName{} = "{}";\n'.format(NAMESPACE,self._getSafeName(),self._getSafeName()))
        self.c_file.write('const char *_{}FullName{} = "{}";\n'.format(NAMESPACE,self._getSafeName(),self._getFriendlyName()))
        self.c_file.write('const int _{}Size{} = {};\n'.format(NAMESPACE,self._getSafeName(),self.PixelSize))
        self.c_file.write('const bool _{}MonoSpace{} = {};\n'.format(NAMESPACE,self._getSafeName(),("true" if monospace else "false")))
        self.c_file.write('const int _{}Height{} = {};\n'.format(NAMESPACE,self._getSafeName(),height))
        self.c_file.write('const int _{}Width{} = {};\n'.format(NAMESPACE,self._getSafeName(),width))
        self.c_file.write('const int _{}MinChar{} = {};\n'.format(NAMESPACE,self._getSafeName(),ord(self.FirstChar)))
        self.c_file.write('const int _{}MaxChar{} = {};\n'.format(NAMESPACE,self._getSafeName(),ord(self.LastChar)))
        self.c_file.write('\n')
        self._generateLookupTable()
        self._generatePixelTable()
        print("Generated font "+self._getSafeName())

    #Calculate font height,width and check if it is monospace
    #returns: ismono, height, width
    def _getFontWidthHeight(self):
        
        #Calculate heighest char and min and max advances
        minadvance = 99999
        maxadvance = 0
        width = 0
        height = 0
        for i in range(ord(self.FirstChar), ord(self.LastChar) + 1):
            char = chr(i)
            char_bmp, buf, w, h, left, top, advance, pitch = self._getChar(char)
            width = max(width,w+left)
            height= max(height,h+top)
            #height = (top-h if top-h>height else height)
            minadvance= (advance if advance<minadvance else minadvance)
            maxadvance= (advance if advance>maxadvance else maxadvance)
        
        #Monospace fonts
        if minadvance==maxadvance:
            return True,height,width #maxadvance

        #Normal fonts
        else:
            return False,height,-1

    #Generate lookup table
    def _generateLookupTable(self):
        self.c_file.write('//Font {} index table\n'.format(self._getFriendlyName()))
        self.c_file.write('const struct '+FONTINDEX_TYPENAME+' _{}Index{}[] = {{\n'.format(NAMESPACE,self._getSafeName()))
        self.c_file.write('{}//Offset Width Height Left Top Advance\n'.format(TAB_STR))
        offset = 0
        for j in range(ord(self.FirstChar),ord(self.LastChar)+1):
            char = chr(j)
            char_bmp, buf, w, h, left, top, advance, pitch = self._getChar(char)
            self._generateLookupEntryForChar(char, offset, w, h, left, top, advance,(1 if j==ord(self.LastChar) else 0))
            offset += w * h
        self.c_file.write('};\n')
        self.c_file.write('\n')

    #Generate lookup entry for single chart
    def _generateLookupEntryForChar(self, char, offset, w, h, left, top, advance, lastone):
        self.c_file.write('{}{{{}, {}, {}, {}, {}, {}}}{} // {} [{}]\n'
        .format(TAB_STR, str(offset).rjust(7), str(w).rjust(4), str(h).rjust(5), str(left).rjust(3), 
        str(top).rjust(2), str(advance).rjust(6), (' ' if lastone==1 else ','), (char if ord(char)>=32 and ord(char)<=126 else '.'), ord(char)))

    #Generate pixel data table
    def _generatePixelTable(self):
        self.c_file.write('//Font {} character data\n'.format(self._getFriendlyName()))
        self.c_file.write('const char unsigned _{}Data{}[] = {{\n'.format(NAMESPACE,self._getSafeName()))
        for i in range(ord(self.FirstChar), ord(self.LastChar) + 1):
            char = chr(i)
            self._generatePixelTableForChar(char)
        self.c_file.write('{}//End\n'.format(TAB_STR))
        self.c_file.write('{}0x00\n'.format(TAB_STR))
        self.c_file.write('};\n')
        self.c_file.write('\n')

    #Generate pixel data for single char
    def _generatePixelTableForChar(self, char):
        char_bmp, buf, w, h, left, top, advance, pitch = self._getChar(char)
        self.c_file.write('{}// {} [{}]\n'.format(TAB_STR, (char if ord(char)>=32 and ord(char)<=126 else '.'), ord(char)))
        if not buf:
            return ''
        for y in range(char_bmp.rows):
            self.c_file.write('{}'.format(TAB_STR))
            self._hexLine(buf, y, w, pitch)
            self.c_file.write(' // ')
            self._asciiArtLine(buf, y, w, pitch)
            self.c_file.write('\n')

    #Retrieve character information from font
    def _getChar(self, char):
        char_bmp = self._renderChar(char) # Side effect: updates self.face.glyph
        assert(char_bmp.pixel_mode == 2) # 2 = FT_PIXEL_MODE_GRAY
        assert(char_bmp.num_grays == 256) # 256 = 1 byte per pixel
        w, h = char_bmp.width, char_bmp.rows
        left = self.face.glyph.bitmap_left
        top = self.PixelSize - self.face.glyph.bitmap_top
        advance = self.face.glyph.linearHoriAdvance >> 16
        buf = char_bmp.buffer # very slow (which is misleading)
        return char_bmp, buf, w, h, left, top, advance, char_bmp.pitch

    #Render char in font
    def _renderChar(self, char):
        if self.face.get_char_index(char)==0:
          self.face.load_char(self.DefChar,freetype.FT_LOAD_RENDER)  
        else:
          self.face.load_char(char,freetype.FT_LOAD_RENDER)
        return self.face.glyph.bitmap

    #Write buffer into hex line
    def _hexLine(self, buf, y, w, pitch):
        for x in range(w):
            v = buf[x + y * pitch]
            self.c_file.write('0x{:02x},'.format(v))

    #Write ascii art on C-Source
    def _asciiArtLine(self, buf, y, w, pitch):
        for x in range(w):
            v = buf[x + y * pitch]
            self.c_file.write(" .-=:+*#%@"[int(v / 26)])

    #C-Source file name
    def _getSourceName(self):
        if len(self.OutFile)!=0:
          return self.OutFile
        else:
          return '{}.c'.format(self._getSafeName())

    #Font safe name (used in variable and file names)
    def _getSafeName(self):
        if len(self.CodeName)!=0:
          return self.CodeName
        else:
          return self._getFriendlyName().replace(' ','')

    #Font friendly name (used in comments
    def _getFriendlyName(self):
        return '{} {} {}'.format(str(self.face.family_name,'utf-8'), str(self.face.style_name,'utf-8'), str(self.PixelSize))

#-----------------------------------------------------------------------------------------------------------------------------------------
# C-Source generator class
#-----------------------------------------------------------------------------------------------------------------------------------------
if __name__=="__main__":

  #Init variables
  Options=[]
  
  #Get command line arguments
  if(GetCommandLineOptions(Options)):
    FontFile=Options[0]
    PixelSize=Options[1]
    StartChar=Options[2]
    EndChar=Options[3]
    CodeName=Options[4]
    OutFile=Options[5]
    DefChar=Options[6]
    Append=Options[7]
  else:
    exit()
  
  #Generate C-Source for font
  ClangGenerator(FontFile,PixelSize,StartChar,EndChar,CodeName,OutFile,DefChar,Append)
