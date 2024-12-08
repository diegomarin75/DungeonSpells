//compmsg.cpp: SysMessage class for compiler and machine messages
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

//SysMessage severities
enum class SysMsgSeverity:int{
  Error=0,     //Compiling error, does not stop compilation
  Warning      //Warning message, no error has occured
};

//SysMessage classes
enum class SysMsgClass{
  CmdLine,  //Command line error
  File,     //File IO error
  Syntax,   //Syntax error
  Internal, //Internal compiler error (design error)
  Runtime   //Runtime error
};

//SysMessage table structure
struct SysMsgDefinition{
  int Code;
  SysMsgSeverity Severity;
  SysMsgClass Class;
  String Text;
};

//SysMessage queue structure
struct SysMsgQueueDef{
  int Index;
  String FileName;
  int LineNr;
  int ColumnNr;
  String Parm1;
  String Parm2;
  String Parm3;
  String Parm4;
  String Parm5;
  String Parm6;
};
    
//Global variables
String _SourceLine="";              //Source line
Array<SysMsgQueueDef> _SysMsgQueue; //Message queue (for delayed messages)
int _MaxCount[2]={0,0};             //Maximun message counters (no more are printed when maximun is reached, except when maximun is zero)
int _MsgCount[2]={0,0};             //Message counters 
bool _ForceOutput=false;            //Forcemessage output regardless of maximun message counts

//SysMessage table
const int _MsgNr=600;
const SysMsgDefinition _Msg[_MsgNr]={
  {  0,SysMsgSeverity::Error,   SysMsgClass::Internal, "Unable to determine path of executable module" },
  {  1,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Identifier %p is invalid because it cannot start by number"},
  {  2,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unable to parse \"%p\" as an integer number in %p base"},
  {  3,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Indexing of string with more than one subindex"},
  {  4,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Invalid number \"%p\" with more than one decimal dot"},
  {  5,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Invalid floating point number \"%p\", exponent is expected after e/E"},
  {  6,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Jump label overflow, code block it is very long, simplify the code using routines"},
  {  7,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Last source line ends in line joiner, cannot join anymore!"},
  {  8,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unable to parse \"%p\" as a floating point number"},
  {  9,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "String litteral exceeds permitted maximun length (%p)"},
  { 10,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unknown escape sequence in char/string literal %p"},
  { 11,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Incorrect hexadecimal escape sequence %p"},
  { 12,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Open string detected"},
  { 13,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Hexadecimal number is expected after 0x"},
  { 14,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unidentified token %p"},
  { 15,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unexpected keyword %p at beginning of sentence"},
  { 16,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Static keyword is not allowed for this sentence"},
  { 17,SysMsgSeverity::Error,   SysMsgClass::Internal, "Cannot find definition for corresponding global variable for static class field %p on class %p (%p)"},
  { 18,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "This kind of sentence (%p) is not expected in current code block (%p), or sentence is not correctly identified"},
  { 19,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Block close sentence without opening block sentence"},
  { 20,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Main sentence is not allowed when compiling libraries"},
  { 21,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Executable file does not have entry point, main sentence is missing"},
  { 22,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unexpected break sentence at current code block, it is allowed inside switch or loop blocks"},
  { 23,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Compilation aborted: Maximun permitted number of errors reached (%p)"},
  { 24,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unexpected continue sentence at current code block, it is allowed inside loop blocks"},
  { 25,SysMsgSeverity::Error,   SysMsgClass::Internal, "Type %p is identified by parser but cannot be found in master tables"},
  { 26,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Type %p is already defined"},
  { 27,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Expected %p instead of %p"},
  { 28,SysMsgSeverity::Error,   SysMsgClass::Internal, "Invalid call to parser function %p"},
  { 29,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Expected %p after position %p"},
  { 30,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unexpected token at end of %p sentence"},
  { 31,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Variable %p has already been defined"},
  { 32,SysMsgSeverity::Error,   SysMsgClass::Internal, "Tokenization of ternary operator failed, unprocessed ? operator was found"},
  { 33,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "%p does not match its previous declaracion, returning data type does not match"},
  { 34,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "%p does not match its previous declaracion, data type for argument %p does not match"},
  { 35,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Constant %p has already been defined"},
  { 36,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Function %p.%p(%p) has not been declared"},
  { 37,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Field member %p has already been defined for class %p"},
  { 38,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Function %p it is private within module %p and cannot be called from other module"},
  { 39,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Class type %p has already been defined"},
  { 40,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Cannot define member function %p.%p() as there is already defined a master method with the same name"},
  { 41,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unexpected token at position %p, comma or closing parenthesys is expected"},
  { 42,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "%p has not been defined"},
  { 43,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "%p does not match its previous declaracion, argument %p is not in function declaration"},
  { 44,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "%p does not match its previous declaracion, argument %p is not in the same position in the argment list as in function declaration"},
  { 45,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Operator function %p is defined as returning void, therefore it cannot happen in the middle of expressions"},
  { 46,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "%p does not match its previous declaration, argument %p is passed by reference in declaration but not in definition"},
  { 47,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "%p does not match its previous declaration, argument %p is passed by value in declaration but not in definition"},
  { 48,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Parameter %p is already declared in function %p"},
  { 49,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Returning type for system functions must be void"},
  { 50,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "In operator call to %p() parameter %p has data type %p but data type %p is expected"},
  { 51,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Identifier %p is not defined"},
  { 52,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Token %p is not allowed on expressions"},
  { 53,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Member operator (.) expects an identifier next to it"},
  { 54,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Comma separator, parenthesys missmatch or bracket missmatch in expression"},
  { 55,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Parenthesys missmatch in expression"},
  { 56,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Bracket missmatch in expression"},
  { 57,SysMsgSeverity::Error,   SysMsgClass::Internal, "Token %p is not expected on expression while converting to RPN form"},
  { 58,SysMsgSeverity::Error,   SysMsgClass::Internal, "Inconsistent expression token, identification is litteral value operand but master type is string,array or class"},
  { 59,SysMsgSeverity::Error,   SysMsgClass::Internal, "Tried to get assembler definition for an expression token that is not operand but %p"},
  { 60,SysMsgSeverity::Error,   SysMsgClass::Internal, "Found array geometry zero as parameter in assembler argument %p of instruction %p, this should not happen"},
  { 61,SysMsgSeverity::Error,   SysMsgClass::File,     "Unable to open assembler file %p (%p)"},
  { 62,SysMsgSeverity::Error,   SysMsgClass::Internal, "Instruction %p is called with %p arguments but it is defined with %p arguments"},
  { 63,SysMsgSeverity::Error,   SysMsgClass::Internal, "Data type is %p for %p argument in instruction %p, but it must be %p"},
  { 64,SysMsgSeverity::Error,   SysMsgClass::Internal, "Invalid addressing mode (%p) for %p argument in instruction %p"},
  { 65,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Expected lvalue on %p side of operator %p"},
  { 66,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Operator %p does not expect %p as argument"},
  { 67,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Operator %p does not expect %p and %p as arguments"},
  { 68,SysMsgSeverity::Error,   SysMsgClass::Internal, "Tried to create variable operand via master type with complex data type"},
  { 69,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Operator %p expects at least one argument"},
  { 70,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Operator %p expects at least two arguments"},
  { 71,SysMsgSeverity::Error,   SysMsgClass::Internal, "Instruction code for meta instruction %p and data type %p is not defined"},
  { 72,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Member operator expects an argument on the left side"},
  { 73,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Member operator expects a class argument on the left side"},
  { 74,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Class %p does not have a field member named %p"},
  { 75,SysMsgSeverity::Error,   SysMsgClass::Internal, "Cannot push parameter %p into stack for function call to %p.%p(), unknown case"},
  { 76,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Left side of operator %p is a constant expression that cannot be modified"},
  { 77,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Destination operand has data type %p which cannot be directly assigned from source operand data type %p"},
  { 78,SysMsgSeverity::Error,   SysMsgClass::Internal, "There is already defined a member function %p(%p) defined on class %p"},
  { 79,SysMsgSeverity::Error,   SysMsgClass::Internal, "Invalid assembler argument, Litteral value passed when datatype is not numeric"},
  { 80,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Incorrect use of subindex as it is applied to type %p that is not a string or an array"},
  { 81,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Array/string subindex is not an %p type"},
  { 82,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Array is defined with %p dimensions but subindex has %p dimensions"},
  { 83,SysMsgSeverity::Error,   SysMsgClass::Internal, "Not enough operands in evaluation stack to call function %p(), number of parameters is %p but evaluation stack has %p operands"},
  { 84,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "In function call to %p.%p() parameter %p has data type %p but data type %p is expected"},
  { 85,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "In function call to %p.%p() parameter %p passed by reference but argument is constant and cannot be modified"},
  { 86,SysMsgSeverity::Error,   SysMsgClass::Internal, "Not enough operands in evaluation stack to call function member or master method .%p(), number of parameters is %p but evaluation stack has %p operands"},
  { 87,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Cannot find function member or master method like .%p(%p) for data type %p or master type %p"},
  { 88,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "In call to .%p() parameter %p has data type %p but data type %p is expected"},
  { 89,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "In call to .%p() parameter %p passed by reference but argument is constant and cannot be modified"},
  { 90,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Missing argument in argument list"},
  { 91,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Method .name() can only be used with lvalues"},
  { 92,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Typecast to data type %p is not supported"},
  { 93,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unexpected token, %p not allowed at this place"},
  { 94,SysMsgSeverity::Error,   SysMsgClass::Internal, "Type %p is identified by parser but cannot be found in master tables"},
  { 95,SysMsgSeverity::Error,   SysMsgClass::Internal, "Type %p is identified by parser but cannot be found in master tables"},
  { 96,SysMsgSeverity::Error,   SysMsgClass::Internal, "Unable to find scope %p on scope stack"},
  { 97,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Sentence expected after keyword static"},
  { 98,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Sentence expected after keyword let"},
  { 99,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Class %p has private fields that cannot be accessed from scope %p, define a constructor or use allow statement to grant access"},
  {100,SysMsgSeverity::Error,   SysMsgClass::Internal, "Cannot resolve jump address %p"},
  {101,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Expected expression before end of sentence"},
  {102,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Expected keyword %p before end of sentence"},
  {103,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Expected expression before keyword %p"},
  {104,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Expression yields more than 1 result, check if there is a missing operator"},
  {105,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Expression does not yield any result"},
  {106,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Expression does not evaluate to boolean"},
  {107,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Expected operator %p before end of sentence"},
  {108,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Expected expression before operator %p"},
  {109,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "For condition does not evaluate to boolean value"},
  {110,SysMsgSeverity::Error,   SysMsgClass::Internal, "For step stack is empty at endfor statement"},
  {111,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "%p returns void and cannot be used as operand"},
  {112,SysMsgSeverity::Error,   SysMsgClass::Internal, "Walk array stack is empty at endwalk statement"},
  {113,SysMsgSeverity::Error,   SysMsgClass::Internal, "Switch expression stack is empty at when statement"},
  {114,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "When condition does not evaluate to boolean value"},
  {115,SysMsgSeverity::Error,   SysMsgClass::Internal, "Switch expression stack is empty at default statement"},
  {116,SysMsgSeverity::Error,   SysMsgClass::Internal, "Switch expression stack is empty at endswitch statement"},
  {117,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Void functions cannot return a value"},
  {118,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Non-void functions must return a value"},
  {119,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Function must return value before endfunction statement"},
  {120,SysMsgSeverity::Error,   SysMsgClass::Internal, "Found address zero as parameter in assembler argument %p of instruction %p, this should not happen"},
  {121,SysMsgSeverity::Error,   SysMsgClass::File,     "Write error happened when generating binary file %p (%p), at part %p index %p (position %p)"},
  {122,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unable to locate module %p from undefined reference %p"},
  {123,SysMsgSeverity::Error,   SysMsgClass::File,     "Read error happened when reading binary file %p (%p), at part %p index %p (position %p)"},
  {124,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Instruction mnemonic %p is not valid"},
  {125,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Ending quote expected"},
  {126,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Invalid data type promotion from %p to %p"},
  {127,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Initial value expression is expected"},
  {128,SysMsgSeverity::Error,   SysMsgClass::File,     "Assembler file write error (%p)"},
  {129,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Expected punctuator %p before end of sentence"},
  {130,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Expected expression before punctuator %p"},
  {131,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Operand %p is used without being initialized"},
  {132,SysMsgSeverity::Warning, SysMsgClass::Syntax,   "%p \"%p\" is declared but never used in %p"},
  {133,SysMsgSeverity::Error,   SysMsgClass::File,     "Cannot load %pbit library %p in %pbit architecture mode"},
  {134,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Main sentence is only allowed on main module"},
  {135,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Data type %p is expected to an enumerated class"},
  {136,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "End of source reached but definition of type %p is left open"},
  {137,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "End of source reached but definition of function %p is left open"},
  {138,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Ternary operator expression must be enclosed within parenthesys"},
  {139,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Incomplete ternary operator expression, : was found without matching ?"},
  {140,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Operand expected for ternary operator"},
  {141,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Boolean operand expected for ternary operator"},
  {142,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Invalid value given for configuration option %p"},
  {143,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unknown configuration option %p"},
  {144,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Source file must have %p extension"},
  {145,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Library option expects boolean value (true/false)"},
  {146,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Do not provide extension for output file"},
  {147,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Provide source file name"},
  {148,SysMsgSeverity::Error,   SysMsgClass::Internal, "Unsupported token value copy for data type %p"},
  {149,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Operation with data type %p is not supported in constant expressions"},
  {150,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Litteral value of data type %p cannot be converted to data type %p in constant expressions"},
  {151,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Token cannot be converted to data type %p in constant expressions"},
  {152,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Type cast to non-numeric data types (%p) is not allowed in constant expressions"},
  {153,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Operator %p is not allowed in constant expressions"},
  {154,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Operator cannot handle data type %p in constant expressions"},
  {155,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Token is not allowed in constant expressions"},
  {156,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Incomplete array definition, unable to find comma or end bracket after opening bracket"},
  {157,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "In operator overload call to %p() parameter %p passed by reference but argument is not lvalue"},
  {158,SysMsgSeverity::Error,   SysMsgClass::Internal, "Function %p(%p) has already been defined on module %p"},
  {159,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Invalid call to .rsize() master method, array has %p dimensions but %p arguments are passed"},
  {160,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "In operator overload call to %p() parameter %p passed by reference but argument is constant and cannot be modified"},
  {161,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "In call to .%p() parameter %p passed by reference but argument is not lvalue"},
  {162,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "In function call to %p.%p() parameter %p passed by reference but argument is not lvalue"},
  {163,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Master method .tobytes() is not available for data type %p because it has dynamic allocation objects inside"},
  {164,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Master method .frombytes() is not available for data type %p because it has dynamic allocation objects inside"},
  {165,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Corresponding ending parenthesys is not found"},
  {166,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Cannot cast from type %p to type %p"},
  {167,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Cannot cast between array types when the element type is not the same"},
  {168,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Class %p has not been defined"},
  {169,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Member function must return value before endmember statement"},
  {170,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Member field %p.%p has private scope and cannot be accessed outside its class"},
  {171,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Member function %p.%p(%p) has private scope and cannot be accessed outside its class"},
  {172,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Cannot define operator function for operator %p, it is not overloadable"},
  {173,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Operator function %p(%p) has been defined already on module %p"},
  {174,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Operator function must return value before endoperator statement"},
  {175,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Operator function cannot be defined as a class member"},
  {176,SysMsgSeverity::Error,   SysMsgClass::Internal, "Delimiter tokens are not expected in RPN form expressions, error in RPN coversion"},
  {177,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Identifier %p is not defined, a module tracker, enum type name, or variable is expected before member operator"},
  {178,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unable to find right match to call function member .%p(%p), there are several choices: %p"},
  {179,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Enumerated field name %p is not defined for enumerated class type %p"},
  {180,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Enum type %p has already been defined"},
  {181,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Field %p has already been defined for enum type %p"},
  {182,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Value expression for field %p in enum type %p is not char,short or int"},
  {183,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "For static class fields, class name and field name must be unique within the module, there is another class %p and field %p with same names defined in current module"},
  {184,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unresolved call to function %p"},
  {185,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Class/enum variable name %p is already used as %p"},
  {186,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Parameter %p is already used as %p"},
  {187,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Must define at least one member for class %p"},
  {188,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Must define at least one field for enum type %p"},
  {189,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Master method .%p() can only be used on non constant lvalues"},
  {190,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Let keyword is only allowed for on local function or operator definitions"},
  {191,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Identifier %p is not neither defined as module tracker or a data type"},
  {192,SysMsgSeverity::Error,   SysMsgClass::Internal, "Detected open scope call within sub scope, this should not happen"},
  {193,SysMsgSeverity::Error,   SysMsgClass::Internal, "Detected close scope call within sub scope, this should not happen"},
  {194,SysMsgSeverity::Error,   SysMsgClass::Internal, "Tried to exit from sub scope twice, this should not happen"},
  {195,SysMsgSeverity::Error,   SysMsgClass::Internal, "Tried close scope that does not match the current one, this should not happen"},
  {196,SysMsgSeverity::Error,   SysMsgClass::Internal, "Tried close scope when all scopes are already closed, this should not happen"},
  {197,SysMsgSeverity::Error,   SysMsgClass::Internal, "Scope stack length is zero when copying public object indexes"},
  {198,SysMsgSeverity::Error,   SysMsgClass::Internal, "Wrong scope index passed when copying public object indexes (scopeindex=%p, scopestacklength=%p)"},
  {199,SysMsgSeverity::Error,   SysMsgClass::Internal, "Upper public module scope not detected when copying public object indexes (scopeindex=%p, scopestacklength=%p)"},
  {200,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "System calls cannot be declared within local scopes"},
  {201,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "System functions cannot be declared within local scopes"},
  {202,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "endfunction statement does not close a function scope"},
  {203,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "endmember statement does not close a function member scope"},
  {204,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "endoperator statement does not close an operator function scope"},
  {205,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Class name %p refers to another module, cannot define objecs from another module on this one"},
  {206,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Data type %p is defined on inner module %p and cannot be used in module public scope, inner module data types are not reachable outside current module, use type sentence whenever necessary"},
  {207,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Module tracker %p is already used as %p"},
  {208,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Type name %p is already used as %p"},
  {209,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Class %p is not defined"},
  {210,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Function %p() is not defined in current module"},
  {211,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Function member %p() has not been defined within class %p"},
  {212,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unexpected token in allow sentece"},
  {213,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Field %p has not been defined within class %p"},
  {214,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Operator overload function %p() has not been defined"},
  {215,SysMsgSeverity::Error,   SysMsgClass::Internal, "Unexpected grant type from case %p to case %p"},
  {216,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Grant from %p to %p is already defined"},
  {217,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Module %p has been included already using same tracker"},
  {218,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Set sentence can only happen on main module"},
  {219,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Invalid debug level id %p, use -dh option for help"},
  {220,SysMsgSeverity::Error,   SysMsgClass::Internal, "Last line in insertion buffer ends in line joiner, cannot join anymore!"},
  {221,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Numeric value %p is specified with suffix %p but it does not fit within number range of data type %p"},
  {222,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Given memory handlers (block_count) is outside of range"},
  {223,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Unable to access configuration file %p"},
  {224,SysMsgSeverity::Error,   SysMsgClass::Internal, "Invalid driver argument (%p) for meta instruction %p, valid values are from 1 to 5"},
  {225,SysMsgSeverity::Error,   SysMsgClass::Internal, "Label seed %p is already enumerated when processing ternary conditional operator"},
  {226,SysMsgSeverity::Error,   SysMsgClass::Internal, "Label seed %p enumeration is not found when processing ternary sub operator"},
  {227,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Operand expected before : operator"},
  {228,SysMsgSeverity::Error,   SysMsgClass::Internal, "Label seed %p enumeration is not found when processing ternary end operator"},
  {229,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Operand expected after : operator"},
  {230,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unable to open binary file %p (%p)"},
  {231,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Octal number is expected after 0c"},
  {232,SysMsgSeverity::Error,   SysMsgClass::Internal, "Cannot find equivalent system type like one defined with name %p in module %p"},
  {233,SysMsgSeverity::Error,   SysMsgClass::Internal, "Cannot detach from search index system type %p defined in module %p"},
  {234,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "In operator overload call to %p() parameter %p is passed by reference but argument is a litteral value"},
  {235,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "In call to .%p() parameter %p passed by reference but argument is a litteral value"},
  {236,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "In function call to %p.%p() parameter %p passed by reference but argument is a litteral value"},
  {237,SysMsgSeverity::Error,   SysMsgClass::File,     "Unable to get free handler to access file %p"},
  {238,SysMsgSeverity::Error,   SysMsgClass::File,     "Error while closing file %p (%p)"},
  {239,SysMsgSeverity::Error,   SysMsgClass::File,     "Error while freeing handler for file %p (%p)"},
  {240,SysMsgSeverity::Error,   SysMsgClass::File,     "Error reading source file %p (%p)"},
  {241,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Must provide %p after %p option"},
  {242,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Valid numeric value is expected for %p option"},
  {243,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Read error hapened when parsing configuration file %p"},
  {244,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Syntax error in configuration file %p line %p: Equal operator (=) is not found in this line"},
  {245,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Unknown command line option: %p"},
  {246,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Invalid system call id (%p)"},
  {247,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Dynamic library calls cannot be declared within local scopes"},
  {248,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Cannot open dynamic library %p"},
  {249,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Dynamic library %p does not contain %p protocol functions (symbol %p is not found)"},
  {250,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Dynamic library %p load failed (initialization returned error)"},
  {251,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Function %p is not defined within dynamic library %p"},
  {252,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unable to obtain function definition for %p from dynamic library %p"},
  {253,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Dynamic function %p is defined on library %p as having %p parameters, but on declaration has %p parameters"},
  {254,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Function result does not match for dynamic function %p, it is defined on library %p as being void but it is not declared as void"},
  {255,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Function result does not match for dynamic function %p, it is defined on library %p as not being void but it is declared as void"},
  {256,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Data type on %p does not match for dynamic function %p on library %p, it is defined as %p and expected native type is %p instead of %p"},
  {257,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Detected inconsistency on %p for dynamic function %p on library %p, parameter has %p type and it is not passed by reference"},
  {258,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Syntax error for %p on dynamic function %p for library %p, only 1-dimensional arrays are supported"},
  {259,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Syntax error for %p on dynamic function %p for library %p, data type %p is not supported as it is not flat on memory, it contains inner memory blocks"},
  {260,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Compiling in %pbit mode, however library %p is compiled for %pbit systems"},
  {261,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Dynammic library name (%p) must not be larger than %p chars"},
  {262,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Dynammic library function name (%p) must not be larger than %p chars"},
  {263,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Mapped data type %p does not exist"},
  {264,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Mapped data type %p cannot be an array"},
  {265,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Dynammic library type name (%p) must not be larger than %p chars"},
  {266,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Data type %p is already mapped to data type %p on dynamic library %p"},
  {267,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Data type %p is not found on function parameters of library %p"},
  {268,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Identifier %p exceeds maximun permitted length (%p)"},
  {269,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Output file name cannot be larger than %p characters"},
  {270,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Binary file name cannot be larger than %p characters"},
  {271,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Temporary dynamic library directory (%p) does not exist"},
  {272,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Dynamic library directory (%p) does not exist"},
  {273,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Constant variable of data type %p cannot be converted to litteral value"},
  {274,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Token cannot be converted to litteral value"},
  {275,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Constant expression does not yield any result"},
  {276,SysMsgSeverity::Error,   SysMsgClass::Internal, "Unsupported token value copy"},
  {277,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Include directory (%p) does not exist"},
  {278,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Library directory (%p) does not exist"},
  {279,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unable resolve undefined reference to variable name %p metadata comming from module %p"},
  {280,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Provide library file name"},
  {281,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Provide executable file name"},
  {282,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "A file with extension %p must be given or first argument must be -lf, -xf or -ve"},
  {283,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unable resolve undefined reference to variable name %p metadata comming from module %p, object is found but it does not have a defined address"},
  {284,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unexpected end of sentence, comma or closing parenthesys is expected"},
  {285,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Cannot return %p when %p is expected"},
  {286,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Invalid identifier (%p)"},
  {287,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Type %p refers to another module, cannot grant access to objects in other modules"},
  {288,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Variable %p has already been defined"},
  {289,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unable to find right match to call function %p.%p(%p), there are several choices: %p"},
  {290,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unable to find right match to call operator function %p(%p), there are several choices: %p"},
  {291,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unable resolve undefined reference to type name %p metadata comming from module %p"},
  {292,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unable resolve undefined reference to type name %p metadata comming from module %p, object is found but it does not have a defined address"},
  {293,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unable resolve undefined reference to class field name list %p metadata comming from module %p"},
  {294,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unable resolve undefined reference to class field name list %p metadata comming from module %p, object is found but it does not have a defined address"},
  {295,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unable resolve undefined reference to class type name list %p metadata comming from module %p"},
  {296,SysMsgSeverity::Error,   SysMsgClass::Internal, "Sentence parsed as master method declaration, but this is still not implemented"},
  {297,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Constructor function %p() cannot have empty argument list"},
  {298,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "This has form of class constructor call, however type %p is not a class"},
  {299,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Contructor function refers to class %p which is not defined in current module"},
  {300,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Constructor function %p() cannot be defined without argument list"},
  {301,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Executable file must have %p extension"},
  {302,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Executable file not provided"},
  {303,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Detected invalid option in configuration file (%p): Value for option %p is expected to be true or false"},
  {304,SysMsgSeverity::Error,   SysMsgClass::Runtime,  "Unable to get free handler to load executable file (%p)"},
  {305,SysMsgSeverity::Error,   SysMsgClass::Runtime,  "Cannot open executable file %p (Error: %p)"},
  {306,SysMsgSeverity::Error,   SysMsgClass::Runtime,  "Error happened while reading executable file %p (Part: %p, Index:%p, Error: %p)"},
  {307,SysMsgSeverity::Error,   SysMsgClass::Runtime,  "Cannot close executable file %p (Error: %p)"},
  {308,SysMsgSeverity::Error,   SysMsgClass::Runtime,  "Cannot release handler for executable file %p (Error: %p)"},
  {309,SysMsgSeverity::Error,   SysMsgClass::Runtime,  "Memory allocation error when loading %p buffer for executable file %p"},
  {310,SysMsgSeverity::Error,   SysMsgClass::Runtime,  "Memory allocator initialization failure when loading executable file %p (%p)"},
  {311,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Cannot load %pbit executable file in %pbit architecture mode"},
  {312,SysMsgSeverity::Error,   SysMsgClass::Runtime,  "Memory manager initialization failure"},
  {313,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Detected invalid option in configuration file (%p): Value for option %p is expected to be an integer number"},
  {314,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Detected invalid option in configuration file (%p): Value for option %p is expected to be a string enclosed between double quotes"},
  {315,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "A file with extension %p or %p must be given or first argument must be -lf, -xf or -ve"},
  {316,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Configuration file %p has invalid option (%p)"},
  {317,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Cannot find class constructor like %p(%p)"},
  {318,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unable to find right match to call class constructor %p(%p), there are several choices: %p"},
  {319,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Class constructor %p.%p(%p) has private scope and cannot be accessed outside its class"},
  {320,SysMsgSeverity::Warning, SysMsgClass::Syntax,   "%p is declared but not defined in %p"},
  {321,SysMsgSeverity::Error,   SysMsgClass::Runtime,  "Unable to lock memory pages"},
  {322,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "In call to .%p() parameter %p has data type %p but data type %p is expected"}, 
  {323,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "In call to .%p() parameter %p passed by reference but argument is not lvalue"}, 
  {324,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "In call to .%p() parameter %p passed by reference but argument is constant and cannot be modified"}, 
  {325,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "In call to .%p() parameter %p passed by reference but argument is a litteral value"}, 
  {326,SysMsgSeverity::Error,   SysMsgClass::Runtime,  "Unable to get %p bytes memory buffer from host system"},
  {327,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Literal value of data type %p cannot be part of computable expressions"},
  {328,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "%p has already been defined"},
  {329,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Static variables can only be declared in local scope"},
  {330,SysMsgSeverity::Error,   SysMsgClass::Internal, "Last line in addition buffer ends in line joiner, cannot join anymore!"},
  {331,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Variable declaration without data type needs initial value expression"},
  {332,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Cannot declare static variables without giving explicit data type"},
  {333,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Cannot declare local constants without giving explicit data type"},
  {334,SysMsgSeverity::Error,   SysMsgClass::Runtime,  "Cannot initialize memory manager for memory unit size %p (minumun unit size is %p bytes)"},
  {335,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Line continuation is specified at end of source"},
  {336,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Raw string is left open at end of source"},
  {337,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Raw string is not closed"},
  {338,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Cannot create application package for a library"},
  {339,SysMsgSeverity::Error,   SysMsgClass::File,     "Unable to get free handler to access binary file %p"},
  {340,SysMsgSeverity::Error,   SysMsgClass::File,     "Cannot open binary file %p (%p)"},
  {341,SysMsgSeverity::Error,   SysMsgClass::File,     "Error reading binary file %p (%p)"},
  {342,SysMsgSeverity::Error,   SysMsgClass::File,     "Error while closing binary file %p (%p)"},
  {343,SysMsgSeverity::Error,   SysMsgClass::File,     "Error while freeing handler for binary file %p (%p)"},
  {344,SysMsgSeverity::Error,   SysMsgClass::File,     "Unable to get free handler to access container file %p"},
  {345,SysMsgSeverity::Error,   SysMsgClass::File,     "Cannot open container file %p (%p)"},
  {346,SysMsgSeverity::Error,   SysMsgClass::File,     "Error reading container file %p (%p)"},
  {347,SysMsgSeverity::Error,   SysMsgClass::File,     "Error while closing container file %p (%p)"},
  {348,SysMsgSeverity::Error,   SysMsgClass::File,     "Error while freeing handler for container file %p (%p)"},
  {349,SysMsgSeverity::Error,   SysMsgClass::File,     "Cannot create packaged application, binary file %p occupies %p bytes but ROM buffer in container file %p has %p bytes as maximun payload"},
  {350,SysMsgSeverity::Error,   SysMsgClass::File,     "Cannot find ROM buffer label inside container file %p"},
  {351,SysMsgSeverity::Error,   SysMsgClass::File,     "Unable to get free handler to access application file %p"},
  {352,SysMsgSeverity::Error,   SysMsgClass::File,     "Cannot open application file %p (%p)"},
  {353,SysMsgSeverity::Error,   SysMsgClass::File,     "Error reading application file %p (%p)"},
  {354,SysMsgSeverity::Error,   SysMsgClass::File,     "Error while closing application file %p (%p)"},
  {355,SysMsgSeverity::Error,   SysMsgClass::File,     "Error while freeing handler for application file %p (%p)"},
  {356,SysMsgSeverity::Error,   SysMsgClass::File,     "Rom buffer overrun when reading at position %p (buffer length is %p)"},
  {357,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Invalid benchmark mode specified, valid values are 0, 1, 2 or 3"},
  {358,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "A file with extension %p must be given or first argument must be -ve"},
  {359,SysMsgSeverity::Error,   SysMsgClass::File,     "Cannot open source file %p (%p)"},
  {360,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unknown escape sequence in char literal %p"},
  {361,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Local constant %p is defined using a local type, which is not supported"},
  {362,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Static variable %p is defined using a local type, which is not supported"},
  {363,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Given memory unit size (memory_unit) is outside of range"},
  {364,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Given start memory units (start_units) is outside of range"},
  {365,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Given increase memory units (chunk_count) is outside of range"},
  {366,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Allow sentence cannot grant permission to objects defined in other modules"},
  {367,SysMsgSeverity::Error,   SysMsgClass::Internal, "Invalid type in driver argument %p for meta instruction %p, type is %p however valid types are from 0 to 6"},
  {368,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unable resolve undefined reference to class field type list %p metadata comming from module %p, object is found but it does not have a defined address"},
  {369,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Cannot define a dynamic array based on a fixed array, this is not supported"},
  {370,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Cannot define a fixed array based on a dynamic array, this is not supported"},
  {371,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Array has more than maximun allowed dimensions (%p)"},
  {372,SysMsgSeverity::Error,   SysMsgClass::Internal, "Not enough operands to evaluate array subscript, required operands are %p but evaluation stack only has %p operands"},
  {373,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Do not pass aditional command line arguments after source file"},
  {374,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unexpected token after typecast specification"},
  {375,SysMsgSeverity::Error,   SysMsgClass::Internal, "Unable to find internal variable for function result"},
  {376,SysMsgSeverity::Error,   SysMsgClass::Internal, "Function %p is not an operator overload function"},
  {377,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Result for operator %p is first operand, overload function %p must return void"},
  {378,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Result for operator %p is first operand, first parameter in overload function %p must be declared as reference"},
  {379,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Operator %p returns a value, overload function %p must not be void"},
  {380,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Parameter in overload function %p must not be declared as reference"},
  {381,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Overflow detected in multiplication %p by %p, result does not fit in data type %p"},
  {382,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Floating point exception %p was raised in multiplication %p by %p"},
  {383,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Divide by zero error detected in division %p by %p"},
  {384,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Floating point exception %p was raised in division %p by %p"},
  {385,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Overflow detected in addition %p plus %p, result does not fit in data type %p"},
  {386,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Floating point exception %p was raised in addition %p plus %p"},
  {387,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Overflow detected in substraction %p minus %p, result does not fit in data type %p"},
  {388,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Floating point exception %p was raised in substraction %p minus %p"},
  {389,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Divide by zero error detected in operation %p modulus %p"},
  {390,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "End main statement can only happen within main function scope"},
  {391,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Compilation aborted: Maximun permitted number of warnings reached (%p)"},
  {392,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Compilation aborted: Maximun permitted number of warnings reached (%p)"},
  {393,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Compilation aborted due to warnings, use -pw option in command line to continue anyway"},
  {394,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Incorrect use of subindex as it is applied to a non-subscriptable object"},
  {395,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Opening curly brace can only happen right after a typecast operator"},
  {396,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unable resolve undefined reference to global variable name %p comming from module %p"},
  {397,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unable resolve undefined reference to global variable name %p comming from module %p, object is found but it does not have a defined address"},
  {398,SysMsgSeverity::Error,   SysMsgClass::File,     "Invalid undefined reference to function %p coming from module, parenthesys are expected in function name"},
  {399,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unable resolve undefined reference to function %p comming from module %p"},
  {400,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unable resolve undefined reference to function %p comming from module %p, object is found but it does not have a defined address"},
  {401,SysMsgSeverity::Error,   SysMsgClass::File,     "Error reasing from standard input"},
  {402,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Configuration option %p cannot happen on library modules"},
  {403,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "An opening curly brace can only happen after a typecast, comma or another opening curly brace"},
  {404,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Curly braces need a typecast before"},
  {405,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Detected extra ( or missing ) in initializer list"},
  {406,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Detected missing ( or extra ) in initializer list"},
  {407,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Detected extra [ or missing ] in initializer list"},
  {408,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Detected missing [ or extra ] in initializer list"},
  {409,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Detected extra { or missing } in initializer list"},
  {410,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Detected missing { or extra } in initializer list"},
  {411,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Number of elements at dimension %p is expected to be %p"},
  {412,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Curly braces need a typecast before"},
  {413,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Opening curly brace does not correspond to an array or class type"},
  {414,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Opening curly brace does not correspond to an array or class type"},
  {415,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Number of elements at dimension %p is expected to be %p"},
  {416,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Expected } not comma, since class %p has %p members"},
  {417,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unexpected end of initializer list"},
  {418,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Number of array dimensions on initializer list is %p but on datatype %p it is %p"},
  {419,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Dimension %p has %p elements on initializer list but it has %p elements on datatype %p"},
  {420,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Initializer list class has %p members but there are %p members on class %p"},
  {421,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Curly brace missmatch in expression"},
  {422,SysMsgSeverity::Error,   SysMsgClass::Internal, "Expected complex value token at top of operator stack"},
  {423,SysMsgSeverity::Error,   SysMsgClass::Internal, "Token is expected in operator stack"},
  {424,SysMsgSeverity::Error,   SysMsgClass::Internal, "Curly brace missmatch at end of RPN conversion"},
  {425,SysMsgSeverity::Error,   SysMsgClass::Internal, "Complex value token happens in expression which does not refers to a class or an array type"},
  {426,SysMsgSeverity::Error,   SysMsgClass::Internal, "Not enough operands in evaluation stack to calculate complex litteral value for data type %p, required values are %p but evaluation stack has %p operands"},
  {427,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Operand %p is used without being initialized"},
  {428,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Field %p in class %p expects data type %p but a value with data type %p is given"},
  {429,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Operand %p is used without being initialized"},
  {430,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Value provided has data type %p but in array data type %p elements must have data type %p"},
  {431,SysMsgSeverity::Error,   SysMsgClass::Internal, "Cannot find global variable for static class field %p on class %p (%p)"},
  {432,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "%p has not been defined, nested function definitions have to be preceded by let keyword"},
  {433,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unable resolve undefined reference to array geometry index for data type %p comming from module %p"},
  {434,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unable resolve undefined reference to array geometry index for data type %p comming from module %p, type is found but it does not have a defined dimension index"},
  {435,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unable resolve undefined reference to array geometry index for data type %p comming from module %p, type is found but it does not have a defined array geometry"},
  {436,SysMsgSeverity::Error,   SysMsgClass::File,     "Cannot load library %p because it has incompatible file format (%p), library needs to be recompiled"},
  {437,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Configuration option %p can only happen on library modules"},
  {438,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Given library major version number is outside of range"},
  {439,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Given library minor version number is outside of range"},
  {440,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Given library revision number is outside of range"},
  {441,SysMsgSeverity::Error,   SysMsgClass::File,     "Cannot import library %p as it has version %p but version %p is required"},
  {442,SysMsgSeverity::Error,   SysMsgClass::Internal, "Assembler argument %p of instruction %p, contains an invalid object id (%p)"},
  {443,SysMsgSeverity::Error,   SysMsgClass::Internal, "Found variable %p before storage but it is not hidden"},
  {444,SysMsgSeverity::Error,   SysMsgClass::Internal, "Found variable %p before storage as hidden but not in same scope"},
  {445,SysMsgSeverity::Error,   SysMsgClass::Internal, "Found hidden variable %p without local storage"},
  {446,SysMsgSeverity::Error,   SysMsgClass::Internal, "Unable to complete operation with operand %p as it is an undefined variable"},
  {447,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Identifier plus assignment operator is expected after var keyword"},
  {448,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Right side operand for operator %p cannot be an undeclared variable"},
  {449,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Left side operand for operator %p cannot be an undeclared variable"},
  {450,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Missing operator after operand"},
  {451,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Keyword if is not found within for() operator"},
  {452,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Keyword do is not found within for() operator"},
  {453,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Keyword return is not found within for() operator"},
  {454,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Corresponding ending parentheys is not found for for() operator"},
  {455,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Opening parenthesys is expected after for keyword"},
  {456,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unexpected if keyword within expression"},
  {457,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unexpected do keyword within expression"},
  {458,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unexpected return keyword within expression"},
  {459,SysMsgSeverity::Error,   SysMsgClass::Internal, "Closing parenthesys is identified as being a flow operator, but flow operator is unknown"},
  {460,SysMsgSeverity::Error,   SysMsgClass::Internal, "Detected for()/array() operator ending without a corresponing beginning"},
  {461,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Operand expected before do keyword"},
  {462,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Boolean data type is expected"},
  {463,SysMsgSeverity::Error,   SysMsgClass::Internal, "Detected for() operator end without a corresponing beginning"},
  {464,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "for() expression does not return any value"},
  {465,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Identifier %p is not defined, a module tracker, enum type name, or variable is expected before member operator"},
  {466,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Missing operator after operand"},
  {467,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Identifier %p is not defined"},
  {468,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Missing operator after operand"},
  {469,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Initializing expression cannot contain the variable/constant that is being defined"},
  {470,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Invalid call to .add() master method, array is not 1-dimensional"},
  {471,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Initializer list only possible for class or array data types"},
  {472,SysMsgSeverity::Error,   SysMsgClass::Internal, "Master method with element data type in arguments must be forcibly called for a dynamic array"},
  {473,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Invalid call to .len() master method, array is not 1-dimensional"},
  {474,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Invalid call to .len() master method, array is not 1-dimensional"},
  {475,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Invalid call to .ins() master method, array is not 1-dimensional"},
  {476,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Invalid call to .del() master method, array is not 1-dimensional"},
  {477,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Keywords if, do or return at incorrect position in for() operator, correct syntax is for(... if ... do ... return ...)"},
  {478,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Keywords on or as at incorrect position in array() operator, correct syntax is array(... on ... as ...)"},
  {479,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Keywords on, index or as at incorrect position in array() operator, correct syntax is array(... on ... index ... as ...)"},
  {480,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Keywords on, if or as at incorrect position in array() operator, correct syntax is array(... on ... if ... as ...)"},
  {481,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Keywords on, index, if or as at incorrect position in array() operator, correct syntax is array(... on ... index ... if ... as ...)"},
  {482,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Keyword on is not found within array() operator"},
  {483,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Keyword as is not found within array() operator"},
  {484,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Corresponding ending parentheys is not found for array() operator"},
  {489,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Opening parenthesys is expected after array keyword"},
  {490,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unexpected on keyword within expression"},
  {491,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unexpected index keyword within expression"},
  {492,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unexpected as keyword within expression"},
  {493,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Identifier is expected after on keyword"},
  {494,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Identifier is expected after index keyword"},
  {495,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "On keyword expects array expression on left side and variable name on right side"},
  {496,SysMsgSeverity::Error,   SysMsgClass::Internal, "Void function result token is not expected in RPN form expressions, error in RPN coversion"},
  {497,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Fixed or dynamic array is expected"},
  {498,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Array expression is expected before on keyword"},
  {499,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Variable %p is already defined"},
  {500,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Variable name %p is already used as %p"},
  {501,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Variable name is expected after on keyword"},
  {502,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Index keyword expects variable name right after"},
  {503,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Array index variable must be regular variable, lvalue not constant"},
  {504,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Array index variable must be have %p data type"},
  {505,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Variable %p is already defined"},
  {506,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Variable name is expected after index keyword"},
  {507,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Operand expected before as keyword"},
  {508,SysMsgSeverity::Error,   SysMsgClass::Internal, "Unable to complete operation with operand %p as it is an undefined variable"},
  {509,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Boolean data type is expected"},
  {510,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Operand expected before ending parenthesys"},
  {511,SysMsgSeverity::Error,   SysMsgClass::Internal, "Unable to complete operation with operand %p as it is an undefined variable"},
  {512,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Module name %p exceeds maximun permitted length (%p)"},
  {513,SysMsgSeverity::Error,   SysMsgClass::Internal, "Detected array() operator end without a corresponing beginning"},
  {514,SysMsgSeverity::Error,   SysMsgClass::Internal, "Flow label stack is empty or has incorrect kind at flow operator %p"},
  {515,SysMsgSeverity::Error,   SysMsgClass::Internal, "Flow label stack is empty or has incorrect kind at flow operator %p"},
  {516,SysMsgSeverity::Error,   SysMsgClass::Internal, "Flow label stack is empty or has incorrect kind at flow operator %p"},
  {517,SysMsgSeverity::Error,   SysMsgClass::Internal, "Flow label stack is empty or has incorrect kind at flow operator %p"},
  {518,SysMsgSeverity::Error,   SysMsgClass::Internal, "Flow label stack is empty or has incorrect kind at flow operator %p"},
  {519,SysMsgSeverity::Error,   SysMsgClass::Internal, "Flow label stack is empty or has incorrect kind at flow operator %p"},
  {520,SysMsgSeverity::Error,   SysMsgClass::Internal, "Flow label stack is empty or has incorrect kind at flow operator %p"},
  {521,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Invalid implicit type conversion from %p to %p"},
  {522,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Variable %p is already defined"},
  {523,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Walk statement expects one dimensional array"},
  {524,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Variable %p is already defined"},
  {525,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Variable name %p is already used as %p"},
  {526,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Array index variable must be regular variable, lvalue not constant"},
  {527,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Array index variable must be have %p data type"},
  {528,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Boolean data type is expected"},
  {529,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Invalid call to .join() master method, array is not 1-dimensional"},
  {530,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Invalid call to .join() master method, array of string or char is expected"},
  {531,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Invalid call to .join() master method, array is not 1-dimensional"},
  {532,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Invalid call to .join() master method, array of string or char is expected"},
  {533,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Missing colon (:) in ternary operator"},
  {534,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Module path \"%p\" exceeds maximun permitted length (%p)"},
  {535,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Cannot calculate expression value, unable to compute operand at compile time"},
  {536,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Cannot calculate expression value, result for operator %p is not determined at compile time"},
  {537,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Cannot calculate expression value, there is defined an operator overload function for operator %p and parameter list %p"},
  {538,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Cannot find function member or master method like .%p(%p) for data type %p or master type %p"},
  {539,SysMsgSeverity::Error,   SysMsgClass::Internal, "Token passed as self is not an operand"},
  {540,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Call to member function .%p() from uninitialized class object"},
  {541,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Call to master method .%p() from uninitialized object"},
  {542,SysMsgSeverity::Error,   SysMsgClass::Internal, "Variable %p has not been defined before"},
  {543,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Token %p is not allowed at end of sentence"},
  {544,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Assign or asterisk is expected after variable identifier"},
  {545,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Assign or asterisk is expected after field identifier"},
  {546,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Init keyword is only allowed on function declaration statements"},
  {547,SysMsgSeverity::Error,   SysMsgClass::Internal, "Unable to find definition for static field variable %p"},
  {548,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Parenthesys missmatch, closing parenthesys without openning one"},
  {549,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Bracket missmatch, closing bracket without openning one"},
  {550,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Curly brace missmatch, closing curly brace without openning one"},
  {551,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Parenthesys missmatch, closing parenthesys is missing"},
  {552,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Bracket missmatch, closing bracket is missing"},
  {553,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Curly brace missmatch, closing curly brace is missing"},
  {554,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Expected operand at the left side of unary operator %p"},
  {555,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Expected operand at the right side of unary operator %p"},
  {556,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Expected operand at the left side of binary operator %p"},
  {557,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Expected operand at the right side of binary operator %p"},
  {558,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Token does not have a value determined at compile time"},
  {559,SysMsgSeverity::Error,   SysMsgClass::Internal, "Sentence expected after keyword $initvar"},
  {560,SysMsgSeverity::Error,   SysMsgClass::Syntax,   "Unable to find corresonding ending bracket (])"},
  {561,SysMsgSeverity::Error,   SysMsgClass::Internal, "Unable to replace litteral value in argument %p for instruction code %p, argument does not support memory addresses"},
  {562,SysMsgSeverity::Error,   SysMsgClass::Internal, "Unable to close assembler nest id because stack is already empty!"},
  {563,SysMsgSeverity::Error,   SysMsgClass::Internal, "Unable to restore parent assembler nest id as stack is empty!"},
  {564,SysMsgSeverity::Error,   SysMsgClass::Runtime,  "Reference indirection error for global address in argument %p on instruction %p"},
  {565,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Executable file must have %p extension"},
  {566,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Provide executable file name"},
  {567,SysMsgSeverity::Error,   SysMsgClass::CmdLine,  "Executable file must have %p extension"},
  {568,SysMsgSeverity::Error,   SysMsgClass::Internal, "Unexpected decoder mode (local indirection) for litteral value argument %p on instruction %p"},
  {569,SysMsgSeverity::Error,   SysMsgClass::Internal, "Unexpected decoder mode (global indirection) for litteral value argument %p on instruction %p"},
  {570,SysMsgSeverity::Error,   SysMsgClass::Internal, "Unexpected cpu data type (%p) for litteral value argument %p on instruction %p"},
  {571,SysMsgSeverity::Error,   SysMsgClass::Internal, "Unexpected cpu addressing mode (indirection) for argument %p on instruction %p"},
  {572,SysMsgSeverity::Error,   SysMsgClass::Internal, "Unexpected cpu data type (%p) for litteral value argument %p on instruction %p"},
  {573,SysMsgSeverity::Error,   SysMsgClass::Internal, "Unexpected cpu data type (%p) for address argument %p on instruction %p"},
  {574,SysMsgSeverity::Error,   SysMsgClass::Internal, "Unexpected cpu addressing mode (indirection) for argument %p on instruction %p"},
  {575,SysMsgSeverity::Error,   SysMsgClass::Internal, "Cannot find replacement id %p in assembler body buffer to replace litteral value %p by %p"},
  {576,SysMsgSeverity::Error,   SysMsgClass::Internal, "Invalid instrucion code found at code address %p (int=%p, hex=%p)"},
  {577,SysMsgSeverity::Error,   SysMsgClass::Runtime,  ""},
  {578,SysMsgSeverity::Error,   SysMsgClass::Runtime,  "Reference indirection error for local address in argument %p on instruction %p"},
  {599,SysMsgSeverity::Error,   SysMsgClass::Syntax,   ""},
}; 

//Constructors
SysMessage::SysMessage(){
  _Index=-1;
  _FileName="";  
  _LineNr=0; 
  _ColumnNr=-1; 
  _MsgSourceLine="";
}
SysMessage::SysMessage(int Code){
  _Index=_FindMessageCode(Code);
  _FileName="";  
  _LineNr=0; 
  _ColumnNr=-1; 
  _MsgSourceLine="";
}
SysMessage::SysMessage(int Code,const String& FileName){
  _Index=_FindMessageCode(Code);
  _FileName=FileName;  
  _LineNr=0;  
  _ColumnNr=-1;  
  _MsgSourceLine="";
}
SysMessage::SysMessage(int Code,const String& FileName,int LineNr){
  _Index=_FindMessageCode(Code);
  _FileName=FileName;  
  _LineNr=LineNr;  
  _ColumnNr=-1;
  _MsgSourceLine="";
}
SysMessage::SysMessage(int Code,const String& FileName,int LineNr,int ColumnNr){
  _Index=_FindMessageCode(Code);
  _FileName=FileName;  
  _LineNr=LineNr;  
  _ColumnNr=ColumnNr;  
  _MsgSourceLine="";
}
SysMessage::SysMessage(int Code,const String& FileName,int LineNr,int ColumnNr,const String& MsgSourceLine){
  _Index=_FindMessageCode(Code);
  _FileName=FileName;  
  _LineNr=LineNr;  
  _ColumnNr=ColumnNr;  
  _MsgSourceLine=MsgSourceLine;
}
    
//Destructor
SysMessage::~SysMessage(){}

//Find message code
int SysMessage::_FindMessageCode(int Code){
  for(int i=0;i<_MsgNr;i++){
    if(_Msg[i].Code==Code){ return i; }
  }
  _Stl->Console.PrintLine("Invalid compiler message number requested: "+ToString(Code));
  return -1;
}

//SysMessage compose text
String SysMessage::_Compose(const String& Parm1,const String& Parm2,const String& Parm3,const String& Parm4,const String& Parm5,const String& Parm6){
  
  //Variables
  String Msg="";

  //Exit if message code is not valid
  if(_Index==-1){
    return Msg;
  }

  //Output severity
  switch(_Msg[_Index].Severity){
    case SysMsgSeverity::Error:   Msg += "E"; break;
    case SysMsgSeverity::Warning: Msg += "W"; break;
  }

  //Output message number
  Msg += ToString(_Msg[_Index].Code,"%03i");

  //Output file, line and column
  if(_FileName.Length()!=0 && _LineNr!=0 && _ColumnNr!=-1){
    Msg += "[" + _Stl->FileSystem.GetFileName(_FileName) + ":" + ToString(_LineNr) + ":" + ToString(_ColumnNr+1) + "]";
  }
  else if(_FileName.Length()!=0 && _LineNr!=0){
    Msg += "[" + _Stl->FileSystem.GetFileName(_FileName) + ":" + ToString(_LineNr) + "]";
  }
  else if(_FileName.Length()!=0){
    Msg += "[" + _Stl->FileSystem.GetFileName(_FileName) + "]";
  }

  //Output class description
  Msg+=" ";
  switch(_Msg[_Index].Class){
    case SysMsgClass::CmdLine:  Msg += "Command line error"; break;
    case SysMsgClass::File:     Msg += "File I/O error"; break;
    case SysMsgClass::Syntax:   Msg += "Syntax error"; break;
    case SysMsgClass::Internal: Msg += "Internal error"; break;
    case SysMsgClass::Runtime:  Msg += "Runtime error"; break;
  }
  Msg+=": ";

  //SysMessage text
  //(You would expect that below we replace using occurences from 1 to 5, 
  //but the fact is that after every replacement the passed string has 1 less "%p" 
  //substring so we always have to replace the firt occurence)
  Msg+=_Msg[_Index].Text.Replace("%p",Parm1,1).Replace("%p",Parm2,1).Replace("%p",Parm3,1).Replace("%p",Parm4,1).Replace("%p",Parm5,1).Replace("%p",Parm6,1);

  //Return message
  return Msg;

}

//Get source line pointer
String SysMessage::_GetSourcePointer(const String& SourceLine){
  int TrimmedSpaces=SourceLine.Length()-SourceLine.Trim().Length();
  if(_FileName.Length()!=0 && _LineNr!=0 && _ColumnNr!=-1){
    if(_ColumnNr==0){
      return String("^");
    }
    else{
      return String(_ColumnNr-TrimmedSpaces,' ')+"^";
    }
  }
  else{
    return "";
  }
  
}

//Set source line for reporting messages
void SysMsgDispatcher::SetSourceLine(String SourceLine){
  _SourceLine=SourceLine;
}

//Get source line for reporting messages
String SysMsgDispatcher::GetSourceLine(){
  return _SourceLine;
}

//Set maximun error count
void SysMsgDispatcher::SetMaximunErrorCount(int Maximun){
  _MaxCount[(int)SysMsgSeverity::Error]=Maximun;
}

//Set maximun warning count
void SysMsgDispatcher::SetMaximunWarningCount(int Maximun){
  _MaxCount[(int)SysMsgSeverity::Warning]=Maximun;
}

//Get error count
int SysMsgDispatcher::GetErrorCount(){
  return _MsgCount[(int)SysMsgSeverity::Error];
}

//Get warning count
int SysMsgDispatcher::GetWarningCount(){
  return _MsgCount[(int)SysMsgSeverity::Warning];
}

void SysMsgDispatcher::SetForceOutput(bool Force){
  _ForceOutput=Force;
}

//Get message string
String SysMessage::GetString(const String& Parm1,const String& Parm2,const String& Parm3,const String& Parm4,const String& Parm5,const String& Parm6){
  if(_Index==-1){ return ""; }
  return _Compose(Parm1,Parm2,Parm3,Parm4,Parm5,Parm6);
}

//Print message
void SysMessage::Print(const String& Parm1,const String& Parm2,const String& Parm3,const String& Parm4,const String& Parm5,const String& Parm6,bool Flush){
  String SourceLine;
  if(Flush){
    FlushDelayedMessages("",0);
  }
  if(_Index!=-1 && (_MsgCount[(int)_Msg[_Index].Severity]<_MaxCount[(int)_Msg[_Index].Severity] || _MaxCount[(int)_Msg[_Index].Severity]==0 || _ForceOutput)){ 
    String Message=_Compose(Parm1,Parm2,Parm3,Parm4,Parm5,Parm6);
    _Stl->Console.PrintLine(Message);
    DebugOutput(Message);
    if(_FileName.Length()!=0 && _LineNr!=0 && (_MsgSourceLine.Length()!=0 || _SourceLine.Length()!=0)){ 
      if(_MsgSourceLine.Length()!=0){ SourceLine=_MsgSourceLine; } else{ SourceLine=_SourceLine; }
      _Stl->Console.PrintLine("--> "+NonAscii2Dots(SourceLine).Trim()); 
      _Stl->Console.PrintLine("    "+_GetSourcePointer(SourceLine));
      DebugOutput("--> "+NonAscii2Dots(SourceLine).Trim()); 
      DebugOutput("    "+_GetSourcePointer(SourceLine)); 
    }
    _MsgCount[(int)_Msg[_Index].Severity]++;
  }
}

//Delay message
void SysMessage::Delay(const String& Parm1,const String& Parm2,const String& Parm3,const String& Parm4,const String& Parm5,const String& Parm6){
  if(_Index!=-1 && (_MsgCount[(int)_Msg[_Index].Severity]<_MaxCount[(int)_Msg[_Index].Severity] || _MaxCount[(int)_Msg[_Index].Severity]==0 || _ForceOutput)){
    _SysMsgQueue.Add((SysMsgQueueDef){_Index,_FileName,_LineNr,_ColumnNr,Parm1,Parm2,Parm3,Parm4,Parm5,Parm6});
  }
}

//Get delayed messages count
int SysMessage::DelayCount(){
  return _SysMsgQueue.Length();
}

//Print delayed message
void SysMessage::FlushDelayedMessages(const String& FileName,int LineNr){
  
  //Variables
  int i;
  int CurrLineNr;
  int CurrColumnNr;
  int CurrIndex;
  String CurrFileName;

  //Save current message variables
  CurrIndex=_Index;
  CurrFileName=_FileName;
  CurrLineNr=_LineNr;
  CurrColumnNr=_ColumnNr;

  //Print message loop
  for(i=0;i<_SysMsgQueue.Length();i++){ 
    _Index=_SysMsgQueue[i].Index;
    if(FileName.Length()!=0 && LineNr!=0){
      _FileName=FileName;
      _LineNr=LineNr;
      _ColumnNr=-1;
    }
    else{
      _FileName=_SysMsgQueue[i].FileName;
      _LineNr=_SysMsgQueue[i].LineNr;
      _ColumnNr=_SysMsgQueue[i].ColumnNr;
    }
    Print(_SysMsgQueue[i].Parm1,_SysMsgQueue[i].Parm2,_SysMsgQueue[i].Parm3,_SysMsgQueue[i].Parm4,_SysMsgQueue[i].Parm5,_SysMsgQueue[i].Parm6,false);
  }

  //Reset message queue
  _SysMsgQueue.Reset();

  //Restore current message
  _Index=CurrIndex;
  _FileName=CurrFileName;
  _LineNr=CurrLineNr;
  _ColumnNr=CurrColumnNr;

}

//Source information create message
SysMessage SourceInfo::Msg(int Code) const {
  return SysMessage(Code,FileName,LineNr,ColNr);
}

//Source information create message with source line
SysMessage SourceInfo::Msg(int Code,const String& SourceLine) const {
  return SysMessage(Code,FileName,LineNr,ColNr,SourceLine);
}

//Constructor from nothing
SourceInfo::SourceInfo(){
  FileName="";
  LineNr=0;
  ColNr=-1;
}

//SourceInfo copy constructor
SourceInfo::SourceInfo(const SourceInfo& Info){
  _Move(Info);
}

//Constructor from values
SourceInfo::SourceInfo(const String& Name,int Line,int Col){
  FileName=Name;
  LineNr=Line;
  ColNr=Col;
}

//SourceInfo assignment
SourceInfo& SourceInfo::operator=(const SourceInfo& Info){
  _Move(Info);
  return *this;
}

//SourceInfo move
void SourceInfo::_Move(const SourceInfo& Info){
  FileName=Info.FileName;
  LineNr=Info.LineNr;
  ColNr=Info.ColNr;
}

