import re
from SublimeLinter.lint import Linter  # or NodeLinter, PythonLinter, ComposerLinter, RubyLinter

#Regex was built using https://regexr.com/
OUTPUT_RE = re.compile(
  r'(?P<code>((?P<error>(E|F))|(?P<warning>W))\d{3})\x5B(\w|[.])+:(?P<line>\d+)(:(?P<col>\d+))?\x5D\s(?P<message>.*)\n',
  re.MULTILINE
)

class DungeonSpells(Linter):
  cmd = 'dunc ${args} ${file}'
  defaults = {
    'selector': 'source.ds',
    'args': '-er 25 -wr 25 -lm'
  }    
  regex = OUTPUT_RE
  multiline = True

