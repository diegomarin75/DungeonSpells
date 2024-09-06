//systemio.cpp: Implementation of system class
#include "bas/basedefs.hpp"
#include "bas/allocator.hpp"
#include "bas/array.hpp"
#include "bas/stack.hpp"
#include "bas/buffer.hpp"
#include "bas/string.hpp"
#include "sys/sysdefs.hpp"
#include "sys/system.hpp"

//Define static variables
Stack<int> System::_GlobalProcessId;
bool System::_ExceptionFlag;
Array<SysExceptionRecord> System::_ExceptionTable;
std::ofstream DebugMessages::_Log;
bool DebugMessages::_ToConsole;
int64_t _DebugLevels;
String DebugMessages::_Append;

//Addressing mode group constants
CpuAdrMode _AmdLtVl=CpuAdrMode::LitValue;
CpuAdrMode _AmdAddr=CpuAdrMode::Address;
CpuAdrMode _AmdNull=(CpuAdrMode)0;

//Instruction table
const InstTable _Inst[_InstructionNr]={
//Mnemo   Nr InstrLen     ArgumentType1          ArgumentType2          ArgumentType3          ArgumentType4             AdrMo1   AdrMo2   AdrMo3   AdrMo4      Off1   Off2    Off3     Off4
{ "NEGc" ,2, ISIZ_IAA  , {CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Negative conversion (Char)
{ "NEGw" ,2, ISIZ_IAA  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Negative conversion (Short)
{ "NEGi" ,2, ISIZ_IAA  , {CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Negative conversion (Integer)
{ "NEGl" ,2, ISIZ_IAA  , {CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Negative conversion (Long)
{ "NEGf" ,2, ISIZ_IAA  , {CpuDataType::Float    ,CpuDataType::Float    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Negative conversion (Float)
{ "ADDc" ,3, ISIZ_IAAA , {CpuDataType::Char     ,CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Addition (Char)
{ "ADDw" ,3, ISIZ_IAAA , {CpuDataType::Short    ,CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Addition (Short)
{ "ADDi" ,3, ISIZ_IAAA , {CpuDataType::Integer  ,CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Addition (Integer)
{ "ADDl" ,3, ISIZ_IAAA , {CpuDataType::Long     ,CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Addition (Long)
{ "ADDf" ,3, ISIZ_IAAA , {CpuDataType::Float    ,CpuDataType::Float    ,CpuDataType::Float    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Addition (Float)
{ "SUBc" ,3, ISIZ_IAAA , {CpuDataType::Char     ,CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Substraction (Char)
{ "SUBw" ,3, ISIZ_IAAA , {CpuDataType::Short    ,CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Substraction (Short)
{ "SUBi" ,3, ISIZ_IAAA , {CpuDataType::Integer  ,CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Substraction (Integer)
{ "SUBl" ,3, ISIZ_IAAA , {CpuDataType::Long     ,CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Substraction (Long)
{ "SUBf" ,3, ISIZ_IAAA , {CpuDataType::Float    ,CpuDataType::Float    ,CpuDataType::Float    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Substraction (Float)
{ "MULc" ,3, ISIZ_IAAA , {CpuDataType::Char     ,CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Multiplication (Char)
{ "MULw" ,3, ISIZ_IAAA , {CpuDataType::Short    ,CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Multiplication (Integer)
{ "MULi" ,3, ISIZ_IAAA , {CpuDataType::Integer  ,CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Multiplication (Integer)
{ "MULl" ,3, ISIZ_IAAA , {CpuDataType::Long     ,CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Multiplication (Long)
{ "MULf" ,3, ISIZ_IAAA , {CpuDataType::Float    ,CpuDataType::Float    ,CpuDataType::Float    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Multiplication (Float)
{ "DIVc" ,3, ISIZ_IAAA , {CpuDataType::Char     ,CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Division (Char)
{ "DIVw" ,3, ISIZ_IAAA , {CpuDataType::Short    ,CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Division (Short)
{ "DIVi" ,3, ISIZ_IAAA , {CpuDataType::Integer  ,CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Division (Integer)
{ "DIVl" ,3, ISIZ_IAAA , {CpuDataType::Long     ,CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Division (Long)
{ "DIVf" ,3, ISIZ_IAAA , {CpuDataType::Float    ,CpuDataType::Float    ,CpuDataType::Float    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Division (Float)
{ "MODc" ,3, ISIZ_IAAA , {CpuDataType::Char     ,CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Modulus (Char)
{ "MODw" ,3, ISIZ_IAAA , {CpuDataType::Short    ,CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Modulus (Short)
{ "MODi" ,3, ISIZ_IAAA , {CpuDataType::Integer  ,CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Modulus (Integer)
{ "MODl" ,3, ISIZ_IAAA , {CpuDataType::Long     ,CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Modulus (Long)
{ "INCc" ,1, ISIZ_IA   , {CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Increment (Char)
{ "INCw" ,1, ISIZ_IA   , {CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Increment (Short)
{ "INCi" ,1, ISIZ_IA   , {CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Increment (Integer)
{ "INCl" ,1, ISIZ_IA   , {CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Increment (Long)
{ "INCf" ,1, ISIZ_IA   , {CpuDataType::Float    ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Increment (Float)
{ "DECc" ,1, ISIZ_IA   , {CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Decrement (Char)
{ "DECw" ,1, ISIZ_IA   , {CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Decrement (Short)
{ "DECi" ,1, ISIZ_IA   , {CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Decrement (Integer)
{ "DECl" ,1, ISIZ_IA   , {CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Decrement (Long)
{ "DECf" ,1, ISIZ_IA   , {CpuDataType::Float    ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Decrement (Float)
{ "PINCc",2, ISIZ_IAA  , {CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Postfix increment (Char)
{ "PINCw",2, ISIZ_IAA  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Postfix increment (Short)
{ "PINCi",2, ISIZ_IAA  , {CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Postfix increment (Integer)
{ "PINCl",2, ISIZ_IAA  , {CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Postfix increment (Long)
{ "PINCf",2, ISIZ_IAA  , {CpuDataType::Float    ,CpuDataType::Float    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Postfix increment (Float)
{ "PDECc",2, ISIZ_IAA  , {CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Postfix decrement (Char)
{ "PDECw",2, ISIZ_IAA  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Postfix decrement (Short)
{ "PDECi",2, ISIZ_IAA  , {CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Postfix decrement (Integer)
{ "PDECl",2, ISIZ_IAA  , {CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Postfix decrement (Long)
{ "PDECf",2, ISIZ_IAA  , {CpuDataType::Float    ,CpuDataType::Float    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Postfix decrement (Float)
{ "LNOT" ,2, ISIZ_IAA  , {CpuDataType::Boolean  ,CpuDataType::Boolean  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Logical not
{ "LAND" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Boolean  ,CpuDataType::Boolean  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Logical and
{ "LOR"  ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Boolean  ,CpuDataType::Boolean  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Logical or
{ "BNOTc",2, ISIZ_IAA  , {CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Bitwise not (Char)
{ "BNOTw",2, ISIZ_IAA  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Bitwise not (Short)
{ "BNOTi",2, ISIZ_IAA  , {CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Bitwise not (Integer)
{ "BNOTl",2, ISIZ_IAA  , {CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Bitwise not (Long)
{ "BANDc",3, ISIZ_IAAA , {CpuDataType::Char     ,CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Bitwise and (Char)
{ "BANDw",3, ISIZ_IAAA , {CpuDataType::Short    ,CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Bitwise and (Short)
{ "BANDi",3, ISIZ_IAAA , {CpuDataType::Integer  ,CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Bitwise and (Integer)
{ "BANDl",3, ISIZ_IAAA , {CpuDataType::Long     ,CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Bitwise and (Long)
{ "BORc" ,3, ISIZ_IAAA , {CpuDataType::Char     ,CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Bitwise or (Char)
{ "BORw" ,3, ISIZ_IAAA , {CpuDataType::Short    ,CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Bitwise or (Short)
{ "BORi" ,3, ISIZ_IAAA , {CpuDataType::Integer  ,CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Bitwise or (Integer)
{ "BORl" ,3, ISIZ_IAAA , {CpuDataType::Long     ,CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Bitwise or (Long)
{ "BXORc",3, ISIZ_IAAA , {CpuDataType::Char     ,CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Bitwise xor (Char)
{ "BXORw",3, ISIZ_IAAA , {CpuDataType::Short    ,CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Bitwise xor (Short)
{ "BXORi",3, ISIZ_IAAA , {CpuDataType::Integer  ,CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Bitwise xor (Integer)
{ "BXORl",3, ISIZ_IAAA , {CpuDataType::Long     ,CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Bitwise xor (Long)
{ "SHLc" ,3, ISIZ_IAAA , {CpuDataType::Char     ,CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Shift left (Char)
{ "SHLw" ,3, ISIZ_IAAA , {CpuDataType::Short    ,CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Shift left (Short)
{ "SHLi" ,3, ISIZ_IAAA , {CpuDataType::Integer  ,CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Shift left (Integer)
{ "SHLl" ,3, ISIZ_IAAA , {CpuDataType::Long     ,CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Shift left (Long)
{ "SHRc" ,3, ISIZ_IAAA , {CpuDataType::Char     ,CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //shift right (Char)
{ "SHRw" ,3, ISIZ_IAAA , {CpuDataType::Short    ,CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //shift right (Short)
{ "SHRi" ,3, ISIZ_IAAA , {CpuDataType::Integer  ,CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //shift right (Integer)
{ "SHRl" ,3, ISIZ_IAAA , {CpuDataType::Long     ,CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //shift right (Long)
{ "LESb" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Boolean  ,CpuDataType::Boolean  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Less (Boolean)
{ "LESc" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Less (Char)
{ "LESw" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Less (Short)
{ "LESi" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Less (Integer)
{ "LESl" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Less (Long)
{ "LESf" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Float    ,CpuDataType::Float    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Less (Float)
{ "LESs" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Less (String)
{ "LEQb" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Boolean  ,CpuDataType::Boolean  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Less or equal (Boolean)
{ "LEQc" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Less or equal (Char)
{ "LEQw" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Less or equal (Short)
{ "LEQi" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Less or equal (Integer)
{ "LEQl" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Less or equal (Long)
{ "LEQf" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Float    ,CpuDataType::Float    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Less or equal (Float)
{ "LEQs" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Less or equal (String)
{ "GREb" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Boolean  ,CpuDataType::Boolean  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Greater (Boolean)
{ "GREc" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Greater (Char)
{ "GREw" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Greater (Short)
{ "GREi" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Greater (Integer)
{ "GREl" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Greater (Long)
{ "GREf" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Float    ,CpuDataType::Float    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Greater (Float)
{ "GREs" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Greater (String)
{ "GEQb" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Boolean  ,CpuDataType::Boolean  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Greater or equal (Boolean)
{ "GEQc" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Greater or equal (Char)
{ "GEQw" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Greater or equal (short)
{ "GEQi" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Greater or equal (Integer)
{ "GEQl" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Greater or equal (Long)
{ "GEQf" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Float    ,CpuDataType::Float    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Greater or equal (Float)
{ "GEQs" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Greater or equal (String)
{ "EQUb" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Boolean  ,CpuDataType::Boolean  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Equal (Boolean)
{ "EQUc" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Equal (Char)
{ "EQUw" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Equal (Short)
{ "EQUi" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Equal (Integer)
{ "EQUl" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Equal (Long)
{ "EQUf" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Float    ,CpuDataType::Float    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Equal (Float)
{ "EQUs" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Equal (String)
{ "DISb" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Boolean  ,CpuDataType::Boolean  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Distinct (Boolean)
{ "DISc" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Distinct (Char)
{ "DISw" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Distinct (Short)
{ "DISi" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Distinct (Integer)
{ "DISl" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Distinct (Long)
{ "DISf" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::Float    ,CpuDataType::Float    ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Distinct (Float)
{ "DISs" ,3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Distinct (String)
{ "MVb"  ,2, ISIZ_IAA  , {CpuDataType::Boolean  ,CpuDataType::Boolean  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move (Boolean)
{ "MVc"  ,2, ISIZ_IAA  , {CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move (Char)
{ "MVw"  ,2, ISIZ_IAA  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move (Short)
{ "MVi"  ,2, ISIZ_IAA  , {CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move (Integer)
{ "MVl"  ,2, ISIZ_IAA  , {CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move (Long)
{ "MVf"  ,2, ISIZ_IAA  , {CpuDataType::Float    ,CpuDataType::Float    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move (Float)
{ "MVr"  ,2, ISIZ_IAA  , {CpuDataType::Undefined,CpuDataType::Undefined,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move (Reference)
{ "LOADb",2, ISIZ_IAB  , {CpuDataType::Boolean  ,CpuDataType::Boolean  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Load (Boolean)
{ "LOADc",2, ISIZ_IAC  , {CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Load (Char)
{ "LOADw",2, ISIZ_IAW  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Load (Short)
{ "LOADi",2, ISIZ_IAI  , {CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Load (Integer)
{ "LOADl",2, ISIZ_IAL  , {CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Load (Long)
{ "LOADf",2, ISIZ_IAF  , {CpuDataType::Float    ,CpuDataType::Float    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Load (Float)
{ "MVADc",2, ISIZ_IAA  , {CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and addition (Char)
{ "MVADw",2, ISIZ_IAA  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and addition (Short)
{ "MVADi",2, ISIZ_IAA  , {CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and addition (Integer)
{ "MVADl",2, ISIZ_IAA  , {CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and addition (Long)
{ "MVADf",2, ISIZ_IAA  , {CpuDataType::Float    ,CpuDataType::Float    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and addition (Float)
{ "MVSUc",2, ISIZ_IAA  , {CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and substraction (Char)
{ "MVSUw",2, ISIZ_IAA  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and substraction (Short)
{ "MVSUi",2, ISIZ_IAA  , {CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and substraction (Integer)
{ "MVSUl",2, ISIZ_IAA  , {CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and substraction (Long)
{ "MVSUf",2, ISIZ_IAA  , {CpuDataType::Float    ,CpuDataType::Float    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and substraction (Float)
{ "MVMUc",2, ISIZ_IAA  , {CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and multiplication (Char)
{ "MVMUw",2, ISIZ_IAA  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and multiplication (Short)
{ "MVMUi",2, ISIZ_IAA  , {CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and multiplication (Integer)
{ "MVMUl",2, ISIZ_IAA  , {CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and multiplication (Long)
{ "MVMUf",2, ISIZ_IAA  , {CpuDataType::Float    ,CpuDataType::Float    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and multiplication (Float)
{ "MVDIc",2, ISIZ_IAA  , {CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and division (Char)
{ "MVDIw",2, ISIZ_IAA  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and division (Short)
{ "MVDIi",2, ISIZ_IAA  , {CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and division (Integer)
{ "MVDIl",2, ISIZ_IAA  , {CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and division (Long)
{ "MVDIf",2, ISIZ_IAA  , {CpuDataType::Float    ,CpuDataType::Float    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and division (Float)
{ "MVMOc",2, ISIZ_IAA  , {CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and modulus (Char)
{ "MVMOw",2, ISIZ_IAA  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and modulus (Short)
{ "MVMOi",2, ISIZ_IAA  , {CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and modulus (Integer)
{ "MVMOl",2, ISIZ_IAA  , {CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and modulus (Long)
{ "MVSLc",2, ISIZ_IAA  , {CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and shift left (Char)
{ "MVSLw",2, ISIZ_IAA  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and shift left (Short)
{ "MVSLi",2, ISIZ_IAA  , {CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and shift left (Integer)
{ "MVSLl",2, ISIZ_IAA  , {CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and shift left (Long)
{ "MVSRc",2, ISIZ_IAA  , {CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and shift right (Char)
{ "MVSRw",2, ISIZ_IAA  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and shift right (Short)
{ "MVSRi",2, ISIZ_IAA  , {CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and shift right (Integer)
{ "MVSRl",2, ISIZ_IAA  , {CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and shift right (Long)
{ "MVANc",2, ISIZ_IAA  , {CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and bitwise and (Char)
{ "MVANw",2, ISIZ_IAA  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and bitwise and (Short)
{ "MVANi",2, ISIZ_IAA  , {CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and bitwise and (Integer)
{ "MVANl",2, ISIZ_IAA  , {CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and bitwise and (Long)
{ "MVXOc",2, ISIZ_IAA  , {CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and bitwise xor (Char)
{ "MVXOw",2, ISIZ_IAA  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and bitwise xor (Short)
{ "MVXOi",2, ISIZ_IAA  , {CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and bitwise xor (Integer)
{ "MVXOl",2, ISIZ_IAA  , {CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and bitwise xor (Long)
{ "MVORc",2, ISIZ_IAA  , {CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and bitwise or (Char)
{ "MVORw",2, ISIZ_IAA  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and bitwise or (Short)
{ "MVORi",2, ISIZ_IAA  , {CpuDataType::Integer  ,CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and bitwise or (Integer)
{ "MVORl",2, ISIZ_IAA  , {CpuDataType::Long     ,CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Move and bitwise or (Long)
{ "RPBEG",2, ISIZ_IAA  , {CpuDataType::Undefined,CpuDataType::Undefined,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Init inner block replication on structs/fix arrays
{ "RPSTR",1, ISIZ_IZ   , {(CpuDataType)-1       ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdLtVl,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Replicate string block
{ "RPARR",1, ISIZ_IZ   , {(CpuDataType)-1       ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdLtVl,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Replicate array block
{ "RPLOF",2, ISIZ_IAG  , {(CpuDataType)-1       ,CpuDataType::ArrGeom  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Add calculation loop over fixed array elements
{ "RPLOD",1, ISIZ_IA   , {(CpuDataType)-1       ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Add calculation loop over dyn array elements
{ "RPEND",0, ISIZ_I    , {(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdNull,_AmdNull,_AmdNull,_AmdNull}, {0     ,0      ,0       ,0        } }, //End calculation loop
{ "BIBEG",1, ISIZ_IA   , {CpuDataType::Undefined,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Init inner block initialization on structs/fix arrays
{ "BISTR",1, ISIZ_IZ   , {(CpuDataType)-1       ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdLtVl,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Init string block
{ "BIARR",3, ISIZ_IZCZ , {(CpuDataType)-1       ,CpuDataType::Char     ,(CpuDataType)-1       ,(CpuDataType)0        }, {_AmdLtVl,_AmdLtVl,_AmdLtVl,_AmdNull}, {AOFF_I,AOFF_IZ,AOFF_IZC,0        } }, //Init array block
{ "BILOF",2, ISIZ_IAG  , {(CpuDataType)-1       ,CpuDataType::ArrGeom  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Add calculation loop over fixed array elements
{ "BIEND",0, ISIZ_I    , {(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdNull,_AmdNull,_AmdNull,_AmdNull}, {0     ,0      ,0       ,0        } }, //End calculation loop
{ "REFOF",3, ISIZ_IAAZ , {CpuDataType::Undefined,CpuDataType::Undefined,(CpuDataType)-1       ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdLtVl,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Create reference with offset
{ "REFAD",2, ISIZ_IAZ  , {CpuDataType::Undefined,(CpuDataType)-1       ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Add offset to reference
{ "REFER",2, ISIZ_IAA  , {CpuDataType::Undefined,CpuDataType::Undefined,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Create reference
{ "COPY" ,3, ISIZ_IAAZ , {CpuDataType::Undefined,CpuDataType::Undefined,(CpuDataType)-1       ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdLtVl,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Copy bytes
{ "SCOPY",2, ISIZ_IAA  , {CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Copy string block
{ "SSWCP",2, ISIZ_IAA  , {CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Switch string memory pointers
{ "ACOPY",2, ISIZ_IAA  , {CpuDataType::ArrBlk   ,CpuDataType::ArrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Copy array block
{ "TOCA" ,3, ISIZ_IAAZ , {CpuDataType::ArrBlk   ,CpuDataType::Undefined,(CpuDataType)-1       ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdLtVl,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Undefined to char array
{ "STOCA",2, ISIZ_IAA  , {CpuDataType::ArrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //String to char array
{ "ATOCA",2, ISIZ_IAA  , {CpuDataType::ArrBlk   ,CpuDataType::ArrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Array to char array
{ "FRCA" ,3, ISIZ_IAAZ , {CpuDataType::Undefined,CpuDataType::ArrBlk   ,(CpuDataType)-1       ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdLtVl,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Undefined from char array
{ "SFRCA",2, ISIZ_IAA  , {CpuDataType::StrBlk   ,CpuDataType::ArrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //String from char array
{ "AFRCA",2, ISIZ_IAA  , {CpuDataType::ArrBlk   ,CpuDataType::ArrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Array from char array
{ "CLEAR",2, ISIZ_IAZ  , {CpuDataType::Undefined,(CpuDataType)-1       ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Set bytes to zero
{ "STACK",1, ISIZ_IL   , {CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdLtVl,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Increase used stack space
{ "AF1RF",4, ISIZ_IAAGA, {CpuDataType::Undefined,CpuDataType::Undefined,CpuDataType::ArrGeom  ,(CpuDataType)-1       }, {_AmdAddr,_AmdAddr,_AmdLtVl,_AmdAddr}, {AOFF_I,AOFF_IA,AOFF_IAA,AOFF_IAAG} }, //Create reference to 1-dim array element
{ "AF1RW",3, ISIZ_IGAA , {CpuDataType::ArrGeom  ,CpuDataType::VarAddr  ,CpuDataType::JumpAddr ,(CpuDataType)0        }, {_AmdLtVl,_AmdLtVl,_AmdLtVl,_AmdNull}, {AOFF_I,AOFF_IG,AOFF_IGA,0        } }, //Rewinds array loop pointer, sets index variable and return address
{ "AF1FO",3, ISIZ_IAAG , {CpuDataType::Undefined,CpuDataType::Undefined,CpuDataType::ArrGeom  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdLtVl,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //If for loop points to index=elements jumps, if not provides reference to index n element
{ "AF1NX",2, ISIZ_IGA  , {CpuDataType::ArrGeom  ,CpuDataType::JumpAddr ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdLtVl,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IG,0       ,0        } }, //Increases array for loop pointer then jumps to loop beginning
{ "AF1SJ",4, ISIZ_IAAGA, {CpuDataType::StrBlk   ,CpuDataType::Undefined,CpuDataType::ArrGeom  ,CpuDataType::StrBlk   }, {_AmdAddr,_AmdAddr,_AmdLtVl,_AmdAddr}, {AOFF_I,AOFF_IA,AOFF_IAA,AOFF_IAAG} }, //Join string array
{ "AF1CJ",4, ISIZ_IAAGA, {CpuDataType::StrBlk   ,CpuDataType::Undefined,CpuDataType::ArrGeom  ,CpuDataType::StrBlk   }, {_AmdAddr,_AmdAddr,_AmdLtVl,_AmdAddr}, {AOFF_I,AOFF_IA,AOFF_IAA,AOFF_IAAG} }, //Join char array
{ "AFDEF",3, ISIZ_IGCZ , {CpuDataType::ArrGeom  ,CpuDataType::Char     ,(CpuDataType)-1       ,(CpuDataType)0        }, {_AmdLtVl,_AmdLtVl,_AmdLtVl,_AmdNull}, {AOFF_I,AOFF_IG,AOFF_IGC,0        } }, //Set array dimensions and cell size
{ "AFSSZ",3, ISIZ_IGAA , {CpuDataType::ArrGeom  ,CpuDataType::Char     ,(CpuDataType)-1       ,(CpuDataType)0        }, {_AmdLtVl,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IG,AOFF_IGA,0        } }, //Set array dimension size
{ "AFGET",3, ISIZ_IGAA , {CpuDataType::ArrGeom  ,CpuDataType::Char     ,(CpuDataType)-1       ,(CpuDataType)0        }, {_AmdLtVl,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IG,AOFF_IGA,0        } }, //Get array dimension size
{ "AFIDX",3, ISIZ_IGCA , {CpuDataType::ArrGeom  ,CpuDataType::Char     ,(CpuDataType)-1       ,(CpuDataType)0        }, {_AmdLtVl,_AmdLtVl,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IG,AOFF_IGC,0        } }, //Set array dimension index
{ "AFREF",3, ISIZ_IAAG , {CpuDataType::Undefined,CpuDataType::Undefined,CpuDataType::ArrGeom  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdLtVl,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Create reference to array element (uses indexes set with AIDX)
{ "AD1EM",2, ISIZ_IAZ  , {CpuDataType::ArrBlk   ,(CpuDataType)-1       ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Define 1-dim emty array
{ "AD1DF",1, ISIZ_IA   , {CpuDataType::ArrBlk   ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Define 1-dim array and set to zero elements if it does not exist before
{ "AD1AP",3, ISIZ_IAAZ , {CpuDataType::Undefined,CpuDataType::ArrBlk   ,(CpuDataType)-1       ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdLtVl,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Resizes array to n+1 elements and gets reference to last element
{ "AD1IN",4, ISIZ_IAAAZ, {CpuDataType::Undefined,CpuDataType::ArrBlk   ,(CpuDataType)-1       ,(CpuDataType)-1       }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdLtVl}, {AOFF_I,AOFF_IA,AOFF_IAA,AOFF_IAAA} }, //Inserts position into array and gets reference to inserted element
{ "AD1DE",2, ISIZ_IAA  , {CpuDataType::ArrBlk   ,(CpuDataType)-1       ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Deletes position from array
{ "AD1RF",3, ISIZ_IAAA , {CpuDataType::Undefined,CpuDataType::ArrBlk   ,(CpuDataType)-1       ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Create reference to 1-dim array element
{ "AD1RS",1, ISIZ_IA   , {CpuDataType::ArrBlk   ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Define 1-dim array and set to zero elements
{ "AD1RW",3, ISIZ_IAAA , {CpuDataType::ArrBlk   ,CpuDataType::VarAddr  ,CpuDataType::JumpAddr ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdLtVl,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Rewinds array loop pointer, sets index variable and return address
{ "AD1FO",2, ISIZ_IAA  , {CpuDataType::Undefined,CpuDataType::ArrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //If for loop points to index=elements jumps, if not provides reference to index n element
{ "AD1NX",2, ISIZ_IAA  , {CpuDataType::ArrBlk   ,CpuDataType::JumpAddr ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Increases array for loop pointer then jumps to loop beginning
{ "AD1SJ",3, ISIZ_IAAA , {CpuDataType::StrBlk   ,CpuDataType::ArrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Join string array
{ "AD1CJ",3, ISIZ_IAAA , {CpuDataType::StrBlk   ,CpuDataType::ArrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Join char array
{ "ADEMP",3, ISIZ_IACZ , {CpuDataType::ArrBlk   ,CpuDataType::Char     ,(CpuDataType)-1       ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdLtVl,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAC,0        } }, //Define empty array
{ "ADDEF",3, ISIZ_IACZ , {CpuDataType::ArrBlk   ,CpuDataType::Char     ,(CpuDataType)-1       ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdLtVl,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAC,0        } }, //Set array dimensions and cell size
{ "ADSET",3, ISIZ_IACA , {CpuDataType::ArrBlk   ,CpuDataType::Char     ,(CpuDataType)-1       ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAC,0        } }, //Set array dimension size
{ "ADRSZ",1, ISIZ_IA   , {CpuDataType::ArrBlk   ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Set array memory buffer size
{ "ADGET",3, ISIZ_IAAA , {CpuDataType::ArrBlk   ,CpuDataType::Char     ,(CpuDataType)-1       ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Get array dimension size
{ "ADRST",1, ISIZ_IA   , {CpuDataType::ArrBlk   ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Destroy array memory
{ "ADIDX",3, ISIZ_IACA , {CpuDataType::ArrBlk   ,CpuDataType::Char     ,(CpuDataType)-1       ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAC,0        } }, //Set array dimension index
{ "ADREF",2, ISIZ_IAA  , {CpuDataType::Undefined,CpuDataType::ArrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Create reference to array element (uses indexes set with AIDX)
{ "ADSIZ",2, ISIZ_IAA  , {CpuDataType::ArrBlk   ,(CpuDataType)-1       ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Calculate array size
{ "AF2F" ,4, ISIZ_IAGAG, {CpuDataType::Undefined,CpuDataType::ArrGeom  ,CpuDataType::Undefined,CpuDataType::ArrGeom  }, {_AmdAddr,_AmdLtVl,_AmdAddr,_AmdLtVl}, {AOFF_I,AOFF_IA,AOFF_IAG,AOFF_IAGA} }, //Cast fixed array to fixed
{ "AF2D" ,3, ISIZ_IAAG , {CpuDataType::ArrBlk   ,CpuDataType::Undefined,CpuDataType::ArrGeom  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdLtVl,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //Cast fixed array to dynamic
{ "AD2F" ,3, ISIZ_IAGA , {CpuDataType::Undefined,CpuDataType::ArrGeom  ,CpuDataType::ArrBlk   ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAG,0        } }, //Cast dynamic array to fixed
{ "AD2D" ,2, ISIZ_IAA  , {CpuDataType::ArrBlk   ,CpuDataType::ArrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Cast dynamic array to dynamic
{ "PUSHb",1, ISIZ_IA   , {CpuDataType::Boolean  ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Push boolean into parameter stack
{ "PUSHc",1, ISIZ_IA   , {CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Push char into parameter stack
{ "PUSHw",1, ISIZ_IA   , {CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Push short into parameter stack
{ "PUSHi",1, ISIZ_IA   , {CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Push integer into parameter stack
{ "PUSHl",1, ISIZ_IA   , {CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Push long into parameter stack
{ "PUSHf",1, ISIZ_IA   , {CpuDataType::Float    ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Push float into parameter stack
{ "PUSHr",1, ISIZ_IA   , {CpuDataType::Undefined,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Push reference into parameter stack
{ "REFPU",1, ISIZ_IA   , {CpuDataType::Undefined,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Create and push reference into parameter stack
{ "LPUb" ,1, ISIZ_IA   , {CpuDataType::Boolean  ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //DynLib: Push boolean into parameter stack
{ "LPUc" ,1, ISIZ_IA   , {CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //DynLib: Push char into parameter stack
{ "LPUw" ,1, ISIZ_IA   , {CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //DynLib: Push short into parameter stack
{ "LPUi" ,1, ISIZ_IA   , {CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //DynLib: Push integer into parameter stack
{ "LPUl" ,1, ISIZ_IA   , {CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //DynLib: Push long into parameter stack
{ "LPUf" ,1, ISIZ_IA   , {CpuDataType::Float    ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //DynLib: Push float into parameter stack
{ "LPUr" ,1, ISIZ_IA   , {CpuDataType::Undefined,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //DynLib: Push reference into parameter stack
{ "LPUSr",2, ISIZ_IAB  , {CpuDataType::Undefined,CpuDataType::Boolean  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //DynLib: Push reference to string into parameter stack
{ "LPADr",2, ISIZ_IAB  , {CpuDataType::Undefined,CpuDataType::Boolean  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //DynLib: Push reference to dyn array into parameter stack
{ "LPAFr",3, ISIZ_IABG , {CpuDataType::Undefined,CpuDataType::Boolean  ,CpuDataType::ArrGeom  ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdLtVl,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAB,0        } }, //DynLib: Push reference to fix array into parameter stack
{ "LRPU" ,1, ISIZ_IA   , {CpuDataType::Undefined,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //DynLib: Create and push reference into parameter stack
{ "LRPUS",2, ISIZ_IAB  , {CpuDataType::Undefined,CpuDataType::Boolean  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //DynLib: Create and push reference to string into parameter stack
{ "LRPAD",2, ISIZ_IAB  , {CpuDataType::Undefined,CpuDataType::Boolean  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //DynLib: Create and push reference to dyn array into parameter stack
{ "LRPAF",3, ISIZ_IABG , {CpuDataType::Undefined,CpuDataType::Boolean  ,CpuDataType::ArrGeom  ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdLtVl,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAB,0        } }, //DynLib: Create and push reference to fix array into parameter stack
{ "CALL" ,1, ISIZ_IA   , {CpuDataType::FunAddr  ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdLtVl,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Call function
{ "RET"  ,0, ISIZ_I    , {(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdNull,_AmdNull,_AmdNull,_AmdNull}, {0     ,0      ,0       ,0        } }, //Return
{ "CALLN",1, ISIZ_IA   , {CpuDataType::FunAddr  ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdLtVl,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Call nested function
{ "RETN" ,0, ISIZ_I    , {(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdNull,_AmdNull,_AmdNull,_AmdNull}, {0     ,0      ,0       ,0        } }, //Return from nested function
{ "SCALL",1, ISIZ_II   , {CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdLtVl,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //System call
{ "LCALL",1, ISIZ_II   , {CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdLtVl,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Dynamic library call
{ "SULOK",0, ISIZ_I    , {(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdNull,_AmdNull,_AmdNull,_AmdNull}, {0     ,0      ,0       ,0        } }, //Allows changes in machine scope state
{ "CUPPR",2, ISIZ_IAA  , {CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //char .upper()
{ "CLOWR",2, ISIZ_IAA  , {CpuDataType::Char     ,CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //char .lower()
{ "SEMP" ,1, ISIZ_IA   , {CpuDataType::StrBlk   ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Creates empty string
{ "SLEN" ,2, ISIZ_IAA  , {(CpuDataType)-1       ,CpuDataType::StrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //word .length()
{ "SMID" ,4, ISIZ_IAAAA, {CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)-1       ,(CpuDataType)-1       }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdAddr}, {AOFF_I,AOFF_IA,AOFF_IAA,AOFF_IAAA} }, //string .sub(word position,word length)
{ "SINDX",3, ISIZ_IAAA , {CpuDataType::Undefined,CpuDataType::StrBlk   ,(CpuDataType)-1       ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //string indexing (subscript)
{ "SRGHT",3, ISIZ_IAAA , {CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)-1       ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //string .right(word length)
{ "SLEFT",3, ISIZ_IAAA , {CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)-1       ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //string .left(word length)
{ "SCUTR",3, ISIZ_IAAA , {CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)-1       ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //string .cutr(word length)
{ "SCUTL",3, ISIZ_IAAA , {CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)-1       ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //string .cutl(word length)
{ "SCONC",3, ISIZ_IAAA , {CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //String concatenation
{ "SMVCO",2, ISIZ_IAA  , {CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Concatenate and assign
{ "SMVRC",2, ISIZ_IAA  , {CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Reverse concatenate and assign
{ "SFIND",4, ISIZ_IAAAA, {(CpuDataType)-1       ,CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)-1       }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdAddr}, {AOFF_I,AOFF_IA,AOFF_IAA,AOFF_IAAA} }, //word .search(string substring,word start)
{ "SSUBS",4, ISIZ_IAAAA, {CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,CpuDataType::StrBlk   }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdAddr}, {AOFF_I,AOFF_IA,AOFF_IAA,AOFF_IAAA} }, //string .replace(string old, string new)
{ "STRIM",2, ISIZ_IAA  , {CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //string .trim()
{ "SUPPR",2, ISIZ_IAA  , {CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //string .upper()
{ "SLOWR",2, ISIZ_IAA  , {CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //string .lower()
{ "SLJUS",4, ISIZ_IAAAA, {CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,CpuDataType::Integer  ,CpuDataType::Char     }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdAddr}, {AOFF_I,AOFF_IA,AOFF_IAA,AOFF_IAAA} }, //string .ljust(int width,char fillchar)
{ "SRJUS",4, ISIZ_IAAAA, {CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,CpuDataType::Integer  ,CpuDataType::Char     }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdAddr}, {AOFF_I,AOFF_IA,AOFF_IAA,AOFF_IAAA} }, //string .rjust(int width,char fillchar)
{ "SMATC",3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //bool .match(string regex)
{ "SLIKE",3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //bool .like(string pattern)
{ "SREPL",3, ISIZ_IAAA , {CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,CpuDataType::Integer  ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //string .replicate(int times)
{ "SSPLI",3, ISIZ_IAAA , {CpuDataType::ArrBlk   ,CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //string[] .split(string separator)
{ "SSTWI",3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //bool .startswith(string substring)
{ "SENWI",3, ISIZ_IAAA , {CpuDataType::Boolean  ,CpuDataType::StrBlk   ,CpuDataType::StrBlk   ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //bool .endswith(string substring)
{ "SISBO",2, ISIZ_IAA  , {CpuDataType::Boolean  ,CpuDataType::StrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //bool .isbool()
{ "SISCH",2, ISIZ_IAA  , {CpuDataType::Boolean  ,CpuDataType::StrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //bool .ischar()
{ "SISSH",2, ISIZ_IAA  , {CpuDataType::Boolean  ,CpuDataType::StrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //bool .isshort()
{ "SISIN",2, ISIZ_IAA  , {CpuDataType::Boolean  ,CpuDataType::StrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //bool .isint()
{ "SISLO",2, ISIZ_IAA  , {CpuDataType::Boolean  ,CpuDataType::StrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //bool .islong()
{ "SISFL",2, ISIZ_IAA  , {CpuDataType::Boolean  ,CpuDataType::StrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //bool .isfloat()
{ "BO2CH",2, ISIZ_IAA  , {CpuDataType::Char     ,CpuDataType::Boolean  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //char .tochar()
{ "BO2SH",2, ISIZ_IAA  , {CpuDataType::Short    ,CpuDataType::Boolean  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //short .toshort()
{ "BO2IN",2, ISIZ_IAA  , {CpuDataType::Integer  ,CpuDataType::Boolean  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //int .toint()
{ "BO2LO",2, ISIZ_IAA  , {CpuDataType::Long     ,CpuDataType::Boolean  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //long .tolong()
{ "BO2FL",2, ISIZ_IAA  , {CpuDataType::Float    ,CpuDataType::Boolean  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //float .tofloat()
{ "BO2ST",2, ISIZ_IAA  , {CpuDataType::StrBlk   ,CpuDataType::Boolean  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //string .tostring()
{ "CH2BO",2, ISIZ_IAA  , {CpuDataType::Boolean  ,CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //bool .tobool()
{ "CH2SH",2, ISIZ_IAA  , {CpuDataType::Short    ,CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //short .toshort()
{ "CH2IN",2, ISIZ_IAA  , {CpuDataType::Integer  ,CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //int .toint()
{ "CH2LO",2, ISIZ_IAA  , {CpuDataType::Long     ,CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //long .tolong()
{ "CH2FL",2, ISIZ_IAA  , {CpuDataType::Float    ,CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //float .tofloat()
{ "CH2ST",2, ISIZ_IAA  , {CpuDataType::StrBlk   ,CpuDataType::Char     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //string .tostring()
{ "CHFMT",3, ISIZ_IAAA , {CpuDataType::StrBlk   ,CpuDataType::Char     ,CpuDataType::StrBlk   ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //string .format(string fmtspec)
{ "SH2BO",2, ISIZ_IAA  , {CpuDataType::Boolean  ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //bool .tobool()
{ "SH2CH",2, ISIZ_IAA  , {CpuDataType::Char     ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //char .tochar()
{ "SH2IN",2, ISIZ_IAA  , {CpuDataType::Integer  ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //int .toint()
{ "SH2LO",2, ISIZ_IAA  , {CpuDataType::Long     ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //long .tolong()
{ "SH2FL",2, ISIZ_IAA  , {CpuDataType::Float    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //float .tofloat()
{ "SH2ST",2, ISIZ_IAA  , {CpuDataType::StrBlk   ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //string .tostring()
{ "SHFMT",3, ISIZ_IAAA , {CpuDataType::StrBlk   ,CpuDataType::Short    ,CpuDataType::StrBlk   ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //string .format(string fmtspec)
{ "IN2BO",2, ISIZ_IAA  , {CpuDataType::Boolean  ,CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //bool .tobool()
{ "IN2CH",2, ISIZ_IAA  , {CpuDataType::Char     ,CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //char .tochar()
{ "IN2SH",2, ISIZ_IAA  , {CpuDataType::Short    ,CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //short .toshort()
{ "IN2LO",2, ISIZ_IAA  , {CpuDataType::Long     ,CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //long .tolong()
{ "IN2FL",2, ISIZ_IAA  , {CpuDataType::Float    ,CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //float .tofloat()
{ "IN2ST",2, ISIZ_IAA  , {CpuDataType::StrBlk   ,CpuDataType::Integer  ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //string .tostring()
{ "INFMT",3, ISIZ_IAAA , {CpuDataType::StrBlk   ,CpuDataType::Integer  ,CpuDataType::StrBlk   ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //string .format(string fmtspec)
{ "LO2BO",2, ISIZ_IAA  , {CpuDataType::Boolean  ,CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //bool .tobool()
{ "LO2CH",2, ISIZ_IAA  , {CpuDataType::Char     ,CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //char .tochar()
{ "LO2SH",2, ISIZ_IAA  , {CpuDataType::Short    ,CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //int .toint()
{ "LO2IN",2, ISIZ_IAA  , {CpuDataType::Integer  ,CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //long .tolong()
{ "LO2FL",2, ISIZ_IAA  , {CpuDataType::Float    ,CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //float .tofloat()
{ "LO2ST",2, ISIZ_IAA  , {CpuDataType::StrBlk   ,CpuDataType::Long     ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //string .tostring()
{ "LOFMT",3, ISIZ_IAAA , {CpuDataType::StrBlk   ,CpuDataType::Long     ,CpuDataType::StrBlk   ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //string .format(string fmtspec)
{ "FL2BO",2, ISIZ_IAA  , {CpuDataType::Boolean  ,CpuDataType::Float    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //bool .tobool()
{ "FL2CH",2, ISIZ_IAA  , {CpuDataType::Char     ,CpuDataType::Float    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //char .tochar()
{ "FL2SH",2, ISIZ_IAA  , {CpuDataType::Short    ,CpuDataType::Float    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //short .toshort()
{ "FL2IN",2, ISIZ_IAA  , {CpuDataType::Integer  ,CpuDataType::Float    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //int .toint()
{ "FL2LO",2, ISIZ_IAA  , {CpuDataType::Long     ,CpuDataType::Float    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //long .tolong()
{ "FL2ST",2, ISIZ_IAA  , {CpuDataType::StrBlk   ,CpuDataType::Float    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //bool .tostring()
{ "FLFMT",3, ISIZ_IAAA , {CpuDataType::StrBlk   ,CpuDataType::Float    ,CpuDataType::StrBlk   ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdAddr,_AmdNull}, {AOFF_I,AOFF_IA,AOFF_IAA,0        } }, //string .format(string fmtspec)
{ "ST2BO",2, ISIZ_IAA  , {CpuDataType::Boolean  ,CpuDataType::StrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //bool .tobool()
{ "ST2CH",2, ISIZ_IAA  , {CpuDataType::Char     ,CpuDataType::StrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //char .tochar()
{ "ST2SH",2, ISIZ_IAA  , {CpuDataType::Short    ,CpuDataType::StrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //short .toshort()
{ "ST2IN",2, ISIZ_IAA  , {CpuDataType::Integer  ,CpuDataType::StrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //int .toint()
{ "ST2LO",2, ISIZ_IAA  , {CpuDataType::Long     ,CpuDataType::StrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //long .tolong()
{ "ST2FL",2, ISIZ_IAA  , {CpuDataType::Float    ,CpuDataType::StrBlk   ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdAddr,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //float .tofloat()
{ "JMPTR",2, ISIZ_IAA  , {CpuDataType::Boolean  ,CpuDataType::JumpAddr ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Jump if true
{ "JMPFL",2, ISIZ_IAA  , {CpuDataType::Boolean  ,CpuDataType::JumpAddr ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdAddr,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IA,0       ,0        } }, //Jump if false
{ "JMP"  ,1, ISIZ_IA   , {CpuDataType::JumpAddr ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdLtVl,_AmdNull,_AmdNull,_AmdNull}, {AOFF_I,0      ,0       ,0        } }, //Absolute jump
{ "DAGV1",2, ISIZ_IWW  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdLtVl,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IW,0       ,0        } }, //Decode argument 1 for global var.
{ "DAGV2",2, ISIZ_IWW  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdLtVl,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IW,0       ,0        } }, //Decode argument 2 for global var.
{ "DAGV3",2, ISIZ_IWW  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdLtVl,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IW,0       ,0        } }, //Decode argument 3 for global var.
{ "DAGV4",2, ISIZ_IWW  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdLtVl,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IW,0       ,0        } }, //Decode argument 4 for global var.
{ "DAGI1",2, ISIZ_IWW  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdLtVl,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IW,0       ,0        } }, //Decode argument 1 for global ref. indirection
{ "DAGI2",2, ISIZ_IWW  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdLtVl,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IW,0       ,0        } }, //Decode argument 2 for global ref. Indirection
{ "DAGI3",2, ISIZ_IWW  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdLtVl,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IW,0       ,0        } }, //Decode argument 3 for global ref. Indirection
{ "DAGI4",2, ISIZ_IWW  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdLtVl,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IW,0       ,0        } }, //Decode argument 4 for global ref. Indirection
{ "DALI1",2, ISIZ_IWW  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdLtVl,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IW,0       ,0        } }, //Decode argument 1 for local ref. Indirection
{ "DALI2",2, ISIZ_IWW  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdLtVl,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IW,0       ,0        } }, //Decode argument 2 for local ref. Indirection
{ "DALI3",2, ISIZ_IWW  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdLtVl,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IW,0       ,0        } }, //Decode argument 3 for local ref. Indirection
{ "DALI4",2, ISIZ_IWW  , {CpuDataType::Short    ,CpuDataType::Short    ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdLtVl,_AmdLtVl,_AmdNull,_AmdNull}, {AOFF_I,AOFF_IW,0       ,0        } }, //Decode argument 4 for local ref. Indirection
{ "NOP"  ,0, ISIZ_I    , {(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        ,(CpuDataType)0        }, {_AmdNull,_AmdNull,_AmdNull,_AmdNull}, {0     ,0      ,0       ,0        } }  //No operation
};

//Exception message table structure
struct SysExceptionMessage{
  SysExceptionCode Code;
  String Text;
};

//Exception message table
const int _MsgNr=81;
const SysExceptionMessage _Msg[_MsgNr]={
  {SysExceptionCode::RuntimeBaseException             , "%p"},
  {SysExceptionCode::SystemPanic                      , "%p"},
  {SysExceptionCode::ConsoleLockedByOtherProcess      , "Print to console failed because it is locked by other process (id:%p)"},
  {SysExceptionCode::NullReferenceIndirection         , "Null reference indirection"},
  {SysExceptionCode::DivideByZero                     , "Division by zero exception"},
  {SysExceptionCode::InvalidInstructionCode           , "Invalid instruction code (%p)"},
  {SysExceptionCode::InvalidArrayBlock                , "Attempt to perform array operation with invalid block number (block:%p)"},
  {SysExceptionCode::InvalidStringBlock               , "Attempt to perform string operation with invalid block number (block:%p)"},
  {SysExceptionCode::StringAllocationError            , "String allocation error when trying to get %p bytes"},
  {SysExceptionCode::NullStringAllocationError        , "Null string allocation error"},
  {SysExceptionCode::InvalidCallStringMid             , "Invalid call to string sub instruction with either negative position or negative length"},
  {SysExceptionCode::InvalidCallStringRight           , "Invalid call to string right instruction with negative length"},
  {SysExceptionCode::InvalidCallStringLeft            , "Invalid call to string left instruction with negative length"},
  {SysExceptionCode::InvalidCallStringCutRight        , "Invalid call to string cut right instruction with negative length"},
  {SysExceptionCode::InvalidCallStringCutLeft         , "Invalid call to string cut left instruction with negative length"},
  {SysExceptionCode::InvalidCallStringLJust           , "Invalid call to string right justify instruction with negative length"},
  {SysExceptionCode::InvalidCallStringRJust           , "Invalid call to string left justify instruction with negative length"},
  {SysExceptionCode::InvalidCallStringFind            , "Invalid call to string find instruction with negative start position"},
  {SysExceptionCode::InvalidCallStringReplicate       , "Invalid call to string replicate with negative argument"},
  {SysExceptionCode::InvalidStringIndex               , "String subindex is negative or over string length (subindex=%p, string length=%p)"},
  {SysExceptionCode::CharToStringConvFailure          , "Char to string conversion failed"},
  {SysExceptionCode::ShortToStringConvFailure         , "Short to string conversion failed"},
  {SysExceptionCode::IntegerToStringConvFailure       , "Integer to string conversion failed"},
  {SysExceptionCode::LongToStringConvFailure          , "Long to string conversion failed"},
  {SysExceptionCode::StringToBooleanConvFailure       , "String to boolean conversion failed"},
  {SysExceptionCode::StringToCharConvFailure          , "String to char conversion failed"},
  {SysExceptionCode::StringToShortConvFailure         , "String to short conversion failed"},
  {SysExceptionCode::StringToIntegerConvFailure       , "String to integer conversion failed"},
  {SysExceptionCode::StringToLongConvFailure          , "String to integer conversion failed"},
  {SysExceptionCode::StringToFloatConvFailure         , "String to float conversion failed"},
  {SysExceptionCode::FloatToCharConvFailure           , "Float to char conversion failed"},
  {SysExceptionCode::FloatToShortConvFailure          , "Float to short conversion failed"},
  {SysExceptionCode::FloatToIntegerConvFailure        , "Float to integer conversion failed"},
  {SysExceptionCode::FloatToLongConvFailure           , "Float to long conversion failed"},
  {SysExceptionCode::FloatToStringConvFailure         , "Float to string conversion failed"},
  {SysExceptionCode::CharStringFormatFailure          , "Char string formatting failed"},
  {SysExceptionCode::ShortStringFormatFailure         , "Short string formatting failed"},
  {SysExceptionCode::IntegerStringFormatFailure       , "Integer string formatting failed"},
  {SysExceptionCode::LongStringFormatFailure          , "Long string formatting failed"},
  {SysExceptionCode::FloatStringFormatFailure         , "Float string formatting failed"},
  {SysExceptionCode::InvalidMemoryAddress             , "Attempt to access %p out of bounds at position %p when maximun position is %p"},
  {SysExceptionCode::MemoryAllocationFailure          , "Memory allocation error when allocating %p bytes"},
  {SysExceptionCode::ArrayIndexAllocationFailure      , "Memory allocation error when allocating new array dimension index record"},
  {SysExceptionCode::ArrayBlockAllocationFailure      , "Memory allocation error when reallocating array block"},
  {SysExceptionCode::CallStackUnderflow               , "Call stack underflow"},
  {SysExceptionCode::StackOverflow                    , "Stack oveflow when allocating %p bytes"},
  {SysExceptionCode::StackUnderflow                   , "Stack underflow"},
  {SysExceptionCode::InvalidArrayDimension            , "Attempt to operate with invalid array dimension (block:%p, dimension:%p)"},
  {SysExceptionCode::InvalidDimensionSize             , "Cannot resize array dimension to negative value"},
  {SysExceptionCode::ArrayWrongDimension              , "Array indexing with incorrect dimension, array has %p dimensions but requested index is for dimension %p"},
  {SysExceptionCode::ArrayIndexingOutOfBounds         , "Array indexing out of bounds on dimension %p, dimension size is %p but requested index is %p"},
  {SysExceptionCode::ReplicationRuleNegative          , "Incorrect inner block replication rule, negative offset is not valid, wrong generated binary code"},
  {SysExceptionCode::ReplicationRuleInconsistent      , "Failure when dealocating last inner block replication rule, wrong generated binary code"},
  {SysExceptionCode::InitializationRuleNegative       , "Incorrect inner block initialization rule, negative offset is not valid, wrong generated binary code"},
  {SysExceptionCode::InitializationRuleInconsistent   , "Failure when dealocating last inner block initialization rule, wrong generated binary code"},
  {SysExceptionCode::SubroutineMaxNestingLevelReached , "Max subroutine nesting level was reached (%p)"},
  {SysExceptionCode::ArrayGeometryMemoryOverflow      , "Memory overflow when resizing array geometry table up to index %p"},
  {SysExceptionCode::ArrayGeometryInvalidIndex        , "Operation with invalid array geometry index %p when table has defined up to index %p"},
  {SysExceptionCode::ArrayGeometryInvalidDimension    , "Operation with invalid dimension on array geometry index %p, requested dimension is %p but array has %p dimensions defined"},
  {SysExceptionCode::ArrayGeometryIndexingOutOfBounds , "Array indexing out of bounds on dimension %p, dimension size is %p but requested index is %p"},
  {SysExceptionCode::FromCharArrayIncorrectLength     , "Length of char array passed on .frombytes() method is %p, which does not match destination field length which is %p"},
  {SysExceptionCode::WriteCharArrayIncorrectLength    , "Length of char array (%p) is less than requested write length (%p)"},
  {SysExceptionCode::MissingDynamicLibrary            , "Unable to access dynamic library %p"},
  {SysExceptionCode::MissingLibraryFunc               , "Function %p() is not found on dynamic library %p"},
  {SysExceptionCode::DynLibArchMissmatch              , "Cannot load library %p as it is compiled for %pbit systems but runtime is in %pbit mode"},
  {SysExceptionCode::DynLibInit1Failed                , "Initialization of dynamic library %p failed"},
  {SysExceptionCode::DynLibInit2Failed                , "Init() procedure on dynamic library %p returned error: %p"},
  {SysExceptionCode::UserLibraryCopyFailed            , "File copy of user library %p into temporary path %p failed"},
  {SysExceptionCode::StaInvalidReadIndex              , "Access out of bounds for array block %p (string[]), requested index is %p but available indexes are from 0 to %p"},
  {SysExceptionCode::StaOperationAlreadyOpen          , "Cannot start operation on string[] as it is already started"},
  {SysExceptionCode::StaOperationAlreadyClosed        , "Cannot close operation on string[] as it is already closed"},
  {SysExceptionCode::StaOperationNotOpen              , "Cannot complete string[] command as operation is not open"},
  {SysExceptionCode::CannotCreatePipe                 , "Unable to create external pipe (Error code 0x%p)"},
  {SysExceptionCode::UnableToCreateProcess            , "Unable to create external process (Error code 0x%p)"},
  {SysExceptionCode::ErrorWaitingChild                , "Error happened while waiting for child process to terminate"},
  {SysExceptionCode::UnableToCheckFdStatus            , "Unable to check file descriptor status on %p"},
  {SysExceptionCode::FdStatusError                    , "File descriptor returned error status on %p"},
  {SysExceptionCode::ReadError                        , "Read error on %p "},
  {SysExceptionCode::InvalidDate                      , "Invalid date value (%p.%p.%p)"},
  {SysExceptionCode::InvalidTime                      , "Invalid time value (%p:%p:%p.%p)"}
};

//Debug level configuration table
struct DebugLevelConfig{
  char Id;              //Short code
  bool CmpEnabled;      //Enabled on compiler
  bool RunEnabled;      //Enabled on runime system
  bool GlobEnabled;     //Enabled on -dx option
  String Description;   //Description
};

//Debug levels table (free ids 9)
const DebugLevelConfig _DebugLevelConf[DEBUG_LEVELS]={
// Id  Cmp   Run   Glob  Description
  {'C',true ,true ,false,"Show system memory allocations"},                     //00: DebugLevel::SysAllocator
  {'G',true ,true ,false,"Show system memory allocator control"},               //01: DebugLevel::SysAllocControl
  {'V',true ,true ,true, "Show dynamic library activities"},                    //02: DebugLevel::SysDynLib
  {'Q',false,true ,true ,"Show internal memory pools"},                         //03: DebugLevel::VrmMemPool
  {'Z',false,true ,true ,"Show internal memory pool checking"},                 //04: DebugLevel::VrmMemPoolCheck
  {'M',false,true ,true ,"Show memory manager"},                                //05: DebugLevel::VrmMemory
  {'U',false,true ,true ,"Show auxiliary memory manager"},                      //06: DebugLevel::VrmAuxMemory
  {'T',false,true ,true ,"Show auxiliary memory manager block counts"},         //07: DebugLevel::VrmAuxMemStatus
  {'R',false,true ,true ,"Show runtime environment activity"},                  //08: DebugLevel::VrmRuntime
  {'1',false,true ,true ,"Show inner block replication"},                       //09: DebugLevel::VrmInnerBlockRpl
  {'3',false,true ,true ,"Show inner block initialization"},                    //10: DebugLevel::VrmInnerBlockInit
  {'X',false,true ,true ,"Show system exceptions"},                             //11: DebugLevel::VrmExceptions
  {'L',true ,false,true ,"Show read lines by parser"},                          //12: DebugLevel::CmpReadLine
  {'K',true ,false,true ,"Show queued lines in parser insertion buffer"},       //13: DebugLevel::CmpInsLine
  {'W',true ,false,true ,"Show queued lines in parser split buffer"},           //14: DebugLevel::CmpSplitLine
  {'P',true ,false,true ,"Show parsed sentences"},                              //15: DebugLevel::CmpParser
  {'H',true ,false,true ,"Show complex litteral value processing"},             //16: DebugLevel::CmpComplexLit
  {'D',true ,false,true ,"Show master data table records"},                     //17: DebugLevel::CmpMasterData
  {'O',true ,false,true ,"Show relocation tables"},                             //18: DebugLevel::CmpRelocation
  {'6',true ,false,true ,"Show binary file load and save activity"},            //19: DebugLevel::CmpBinary
  {'4',true ,false,true ,"Show library dependencies and undefined references"}, //20: DebugLevel::CmpLibrary
  {'Y',true ,false,true ,"Show linker symbol tables"},                          //21: DebugLevel::CmpLnkSymbol
  {'2',true ,false,true ,"Show debug symbol tables"},                           //22: DebugLevel::CmpDbgSymbol
  {'I',true ,false,true ,"Show search indexes"},                                //23: DebugLevel::CmpIndex
  {'B',true ,false,true ,"Show code block stack"},                              //24: DebugLevel::CmpCodeBlock
  {'J',true ,false,true ,"Show jump events"},                                   //25: DebugLevel::CmpJump
  {'F',true ,false,true ,"Show forward call resolution"},                       //26: DebugLevel::CmpFwdCall
  {'5',true ,false,true ,"Show litteral value replacements"},                   //27: DebugLevel::CmpLitValRepl
  {'7',true ,false,true ,"Show merging of code buffers"},                       //28: DebugLevel::CmpMergeCode
  {'S',true ,false,true ,"Show variable scope stack"},                          //29: DebugLevel::CmpScopeStk
  {'0',true ,false,true ,"Show inner block replication"},                       //30: DebugLevel::CmpInnerBlockRpl
  {'8',true ,false,true ,"Show inner block initialization"},                    //31: DebugLevel::CmpInnerBlockInit
  {'E',true ,false,true ,"Show expression compiler calculations"},              //32: DebugLevel::CmpExpression
  {'A',true ,false,true ,"Show generated assembler lines"},                     //33: DebugLevel::CmpAssembler
  {'N',true ,false,true ,"Show initilizations and start code"}                  //34: DebugLevel::CmpInit
}; 

//SysExceptionRecord copy
SysExceptionRecord::SysExceptionRecord(const SysExceptionRecord& Excp){
  _Move(Excp);
}

//SysExceptionRecord assigment
SysExceptionRecord& SysExceptionRecord::operator=(const SysExceptionRecord& Excp){
  _Move(Excp);
  return *this;
}

//SysExceptionRecord move
void SysExceptionRecord::_Move(const SysExceptionRecord& Excp){
  Code=Excp.Code;
  Message=Excp.Message;
  ProcessId=Excp.ProcessId;
}

//Constructor
System::System(){
  _GlobalProcessId.Reset();
  _ExceptionTable.Reset();
}

//Push global process id
void System::PushProcessId(int ProcessId){
  _GlobalProcessId.Push(ProcessId);
}

//Pop global process id
void System::PopProcessId(){
  _GlobalProcessId.Pop();
}

//Get current global process id
int System::CurrentProcessId(){
  return _GlobalProcessId.Top();
}

//Count
long System::ExceptionCount(){
  return _ExceptionTable.Length();
}

//Clear
void System::ExceptionReset(){
  _ExceptionTable.Reset();
  _ExceptionFlag=false;
}

//Read
bool System::ExceptionRead(int Index,SysExceptionRecord& Excp){
  if(Index>=0 && Index<_ExceptionTable.Length()){
    Excp=_ExceptionTable[Index];
    return true;
  }
  else{
    return false;
  }
}

//Throw
void System::Throw(SysExceptionCode Code,const String& Parm1,const String& Parm2,const String& Parm3,const String& Parm4,const String& Parm5){
  SysExceptionRecord Excp;
  Excp.Code=Code;
  Excp.Message=NonAsciiEscape(_Msg[(int)Code].Text.Replace("%p",Parm1,1).Replace("%p",Parm2,1).Replace("%p",Parm3,1).Replace("%p",Parm4,1).Replace("%p",Parm5,1));
  Excp.ProcessId=CurrentProcessId();
  _ExceptionTable.Add(Excp);
  _ExceptionFlag=true;
  DebugMessage(DebugLevel::VrmExceptions,"Exception: code="+ExceptionName(Excp.Code)+" message="+Excp.Message+" processid="+ToString(Excp.ProcessId));
}

//Print exception stack on console
void System::ExceptionPrint(){
  for(int i=_ExceptionTable.Length()-1;i>=0;i--){
    if(_ExceptionTable[i].Code==SysExceptionCode::RuntimeBaseException){
      std::cerr << _ExceptionTable[i].Message+" (pid="+ToString((int)_ExceptionTable[i].ProcessId)+")" << std::endl;
    }
    else{
      std::cerr << "Exception "+ExceptionName(_ExceptionTable[i].Code)+": "+_ExceptionTable[i].Message+" (pid="+ToString((int)_ExceptionTable[i].ProcessId)+")" << std::endl;
    }
  }
}

//Get last floating point exception
String System::GetLastFloException(){
  String FpExcept;
  if(     fetestexcept(FE_INVALID   )   ){ FpExcept="FE_INVALID";    }
  else if(fetestexcept(FE_DIVBYZERO )   ){ FpExcept="FE_DIVBYZERO";  }
  else if(fetestexcept(FE_OVERFLOW  )   ){ FpExcept="FE_OVERFLOW";   }
  else if(fetestexcept(FE_UNDERFLOW )   ){ FpExcept="FE_UNDERFLOW";  }
  else if(fetestexcept(FE_INEXACT   )   ){ FpExcept="FE_INEXACT";    }
  else if(fetestexcept(FE_ALL_EXCEPT)==0){ FpExcept=""; }
  return(FpExcept);
}

//Get exception name
String System::ExceptionName(SysExceptionCode Code){
  String Name;
  switch(Code){
    case SysExceptionCode::RuntimeBaseException             : Name="RuntimeBaseException";             break;
    case SysExceptionCode::SystemPanic                      : Name="SystemPanic";                      break;
    case SysExceptionCode::ConsoleLockedByOtherProcess      : Name="ConsoleLockedByOtherProcess";      break;
    case SysExceptionCode::NullReferenceIndirection         : Name="NullReferenceIndirection";         break;
    case SysExceptionCode::DivideByZero                     : Name="DivideByZero";                     break;
    case SysExceptionCode::InvalidInstructionCode           : Name="InvalidInstructionCode";           break;
    case SysExceptionCode::InvalidArrayBlock                : Name="InvalidArrayBlock";                break;
    case SysExceptionCode::InvalidStringBlock               : Name="InvalidStringBlock";               break;
    case SysExceptionCode::StringAllocationError            : Name="StringAllocationError";            break;
    case SysExceptionCode::NullStringAllocationError        : Name="NullStringAllocationError";        break;
    case SysExceptionCode::InvalidCallStringMid             : Name="InvalidCallStringMid";             break;
    case SysExceptionCode::InvalidCallStringRight           : Name="InvalidCallStringRight";           break;
    case SysExceptionCode::InvalidCallStringLeft            : Name="InvalidCallStringLeft";            break;
    case SysExceptionCode::InvalidCallStringCutRight        : Name="InvalidCallStringCutRight";        break;
    case SysExceptionCode::InvalidCallStringCutLeft         : Name="InvalidCallStringCutLeft";         break;
    case SysExceptionCode::InvalidCallStringLJust           : Name="InvalidCallStringLJust";           break;
    case SysExceptionCode::InvalidCallStringRJust           : Name="InvalidCallStringRJust";           break;
    case SysExceptionCode::InvalidCallStringFind            : Name="InvalidCallStringFind";            break;
    case SysExceptionCode::InvalidCallStringReplicate       : Name="InvalidCallStringReplicate";       break;
    case SysExceptionCode::InvalidStringIndex               : Name="InvalidStringIndex";               break;
    case SysExceptionCode::CharToStringConvFailure          : Name="CharToStringConvFailure";          break;
    case SysExceptionCode::ShortToStringConvFailure         : Name="ShortToStringConvFailure";         break;
    case SysExceptionCode::IntegerToStringConvFailure       : Name="IntegerToStringConvFailure";       break;
    case SysExceptionCode::LongToStringConvFailure          : Name="LongToStringConvFailure";          break;
    case SysExceptionCode::StringToBooleanConvFailure       : Name="StringToBooleanConvFailure";       break;
    case SysExceptionCode::StringToCharConvFailure          : Name="StringToCharConvFailure";          break;
    case SysExceptionCode::StringToShortConvFailure         : Name="StringToShortConvFailure";         break;
    case SysExceptionCode::StringToIntegerConvFailure       : Name="StringToIntegerConvFailure";       break;
    case SysExceptionCode::StringToLongConvFailure          : Name="StringToLongConvFailure";          break;
    case SysExceptionCode::StringToFloatConvFailure         : Name="StringToFloatConvFailure";         break;
    case SysExceptionCode::FloatToCharConvFailure           : Name="FloatToCharConvFailure";           break;
    case SysExceptionCode::FloatToShortConvFailure          : Name="FloatToShortConvFailure";          break;
    case SysExceptionCode::FloatToIntegerConvFailure        : Name="FloatToIntegerConvFailure";        break;
    case SysExceptionCode::FloatToLongConvFailure           : Name="FloatToLongConvFailure";           break;
    case SysExceptionCode::FloatToStringConvFailure         : Name="FloatToStringConvFailure";         break;
    case SysExceptionCode::CharStringFormatFailure          : Name="CharStringFormatFailure";          break;
    case SysExceptionCode::ShortStringFormatFailure         : Name="ShortStringFormatFailure";         break;
    case SysExceptionCode::IntegerStringFormatFailure       : Name="IntegerStringFormatFailure";       break;
    case SysExceptionCode::LongStringFormatFailure          : Name="LongStringFormatFailure";          break;
    case SysExceptionCode::FloatStringFormatFailure         : Name="FloatStringFormatFailure";         break;
    case SysExceptionCode::InvalidMemoryAddress             : Name="InvalidMemoryAddress";             break;
    case SysExceptionCode::MemoryAllocationFailure          : Name="MemoryAllocationFailure";          break;
    case SysExceptionCode::ArrayIndexAllocationFailure      : Name="ArrayIndexAllocationFailure";      break;
    case SysExceptionCode::ArrayBlockAllocationFailure      : Name="ArrayBlockAllocationFailure";      break;
    case SysExceptionCode::CallStackUnderflow               : Name="CallStackUnderflow";               break;
    case SysExceptionCode::StackOverflow                    : Name="StackOverflow";                    break;
    case SysExceptionCode::StackUnderflow                   : Name="StackUnderflow";                   break;
    case SysExceptionCode::InvalidArrayDimension            : Name="InvalidArrayDimension";            break;
    case SysExceptionCode::InvalidDimensionSize             : Name="InvalidDimensionSize";             break;
    case SysExceptionCode::ArrayWrongDimension              : Name="ArrayWrongDimension";              break;
    case SysExceptionCode::ArrayIndexingOutOfBounds         : Name="ArrayIndexingOutOfBounds";         break;
    case SysExceptionCode::ReplicationRuleNegative          : Name="ReplicationRuleNegative";          break;
    case SysExceptionCode::ReplicationRuleInconsistent      : Name="ReplicationRuleInconsistent";      break;
    case SysExceptionCode::InitializationRuleNegative       : Name="InitializationRuleNegative";       break;
    case SysExceptionCode::InitializationRuleInconsistent   : Name="InitializationRuleInconsistent";   break;
    case SysExceptionCode::SubroutineMaxNestingLevelReached : Name="SubroutineMaxNestingLevelReached"; break;
    case SysExceptionCode::ArrayGeometryMemoryOverflow      : Name="ArrayGeometryMemoryOverflow";      break;
    case SysExceptionCode::ArrayGeometryInvalidIndex        : Name="ArrayGeometryInvalidIndex";        break;
    case SysExceptionCode::ArrayGeometryInvalidDimension    : Name="ArrayGeometryInvalidDimension";    break;
    case SysExceptionCode::ArrayGeometryIndexingOutOfBounds : Name="ArrayGeometryIndexingOutOfBounds"; break;
    case SysExceptionCode::FromCharArrayIncorrectLength     : Name="FromCharArrayIncorrectLength";     break;
    case SysExceptionCode::WriteCharArrayIncorrectLength    : Name="WriteCharArrayIncorrectLength";    break;
    case SysExceptionCode::MissingDynamicLibrary            : Name="MissingDynamicLibrary";            break;
    case SysExceptionCode::MissingLibraryFunc               : Name="MissingLibraryFunc";               break;
    case SysExceptionCode::DynLibArchMissmatch              : Name="DynLibArchMissmatch";              break;
    case SysExceptionCode::DynLibInit1Failed                : Name="DynLibInit1Failed";                break;
    case SysExceptionCode::DynLibInit2Failed                : Name="DynLibInit2Failed";                break;
    case SysExceptionCode::UserLibraryCopyFailed            : Name="UserLibraryCopyFailed";            break;
    case SysExceptionCode::StaInvalidReadIndex              : Name="StaInvalidReadIndex";              break;
    case SysExceptionCode::StaOperationAlreadyOpen          : Name="StaOperationAlreadyOpen";          break;
    case SysExceptionCode::StaOperationAlreadyClosed        : Name="StaOperationAlreadyClosed";        break;
    case SysExceptionCode::StaOperationNotOpen              : Name="StaOperationNotOpen";              break;
    case SysExceptionCode::CannotCreatePipe                 : Name="CannotCreatePipe";                 break;
    case SysExceptionCode::UnableToCreateProcess            : Name="UnableToCreateProcess";            break;
    case SysExceptionCode::ErrorWaitingChild                : Name="ErrorWaitingChild";                break;
    case SysExceptionCode::UnableToCheckFdStatus            : Name="UnableToCheckFdStatus";            break;
    case SysExceptionCode::FdStatusError                    : Name="FdStatusError";                    break;
    case SysExceptionCode::ReadError                        : Name="ReadError";                        break;
    case SysExceptionCode::InvalidDate                      : Name="InvalidDate";                      break;
    case SysExceptionCode::InvalidTime                      : Name="InvalidTime";                      break;
  }
  return Name;
}

//Get symbol debug string Module
String System::GetDbgSymDebugStr(const DbgSymModule& Mod){
  return "name="+String((char *)Mod.Name)+" path="+String((char *)Mod.Name);
}

//Get symbol debug string Type
String System::GetDbgSymDebugStr(const DbgSymType& Typ){
  return "name="+String((char *)Typ.Name)+" msttype="+ToString(Typ.MstType)+" length="+ToString(Typ.Length)+" fieldlow="+ToString(Typ.FieldLow)+" fieldhigh="+ToString(Typ.FieldHigh);
}

//Get symbol debug string Variable
String System::GetDbgSymDebugStr(const DbgSymVariable& Var){
  return "name="+String((char *)Var.Name)+" modindex="+ToString(Var.ModIndex)+" glob="+String(Var.Glob?"true":"false")+" funindex="+ToString(Var.FunIndex)+" typindex="+ToString(Var.TypIndex)+" address="+HEXFORMAT(Var.Address)+" isreference="+ToString(Var.IsReference);
}

//Get symbol debug string Field
String System::GetDbgSymDebugStr(const DbgSymField& Fld){
  return "name="+String((char *)Fld.Name)+" typindex="+ToString(Fld.TypIndex)+" offset="+ToString(Fld.Offset);
}

//Get symbol debug string Function
String System::GetDbgSymDebugStr(const DbgSymFunction& Fun){
  return "kind="+ToString(Fun.Kind)+"name="+String((char *)Fun.Name)+" modindex="+ToString(Fun.ModIndex)+" begaddress="+HEXFORMAT(Fun.BegAddress)+" endaddress="+HEXFORMAT(Fun.EndAddress)+" typindex="+ToString(Fun.TypIndex)+" isvoid="+ToString(Fun.IsVoid)
  +" parmnr="+ToString(Fun.ParmNr)+" parmlow="+ToString(Fun.ParmLow)+" parmhigh="+ToString(Fun.ParmHigh);
}

//Get symbol debug string Parameter
String System::GetDbgSymDebugStr(const DbgSymParameter& Par){
  return "name="+String((char *)Par.Name)+" typindex="+ToString(Par.TypIndex)+" funindex="+ToString(Par.FunIndex)+" isreference="+ToString(Par.IsReference);
}

//Get symbol debug string Line
String System::GetDbgSymDebugStr(const DbgSymLine& Lin){
  return "modindex="+ToString(Lin.ModIndex)+" begaddress="+HEXFORMAT(Lin.BegAddress)+" endaddress="+HEXFORMAT(Lin.EndAddress)+" linenr="+ToString(Lin.LineNr);
}

//Compiler debug constructos
DebugMessages::DebugMessages(){
  _DebugLevels=0;
  _ToConsole=false;
  _Append="";
}

//Open log
bool DebugMessages::OpenLog(const String& OrigFile,const String& LogExt){

  //Variables
  bool Enabled;
  String DebugFile;

  //Detect there are debug levels enabled
  Enabled=(_DebugLevels!=0?true:false);

  //Open logfile
  if(Enabled && !_ToConsole){
    DebugFile=OrigFile.Replace(String(SOURCE_EXT),LogExt).Replace(String(EXECUTABLE_EXT),LogExt);
    _Log=std::ofstream(DebugFile.CharPnt());
    if(!_Log.good()){ return false; }
  }

  //Return code
  return true;

}

//Close log
void DebugMessages::CloseLog(){
  if(_Log.is_open()){
    _Log.close();
  }
}

//Set output log on console
void DebugMessages::SetConsoleOutput(){
  _ToConsole=true;
}

//Get output log on console
bool DebugMessages::GetConsoleOutput(){
  return _ToConsole;
}

//Set debug level
void DebugMessages::SetLevel(DebugLevel Level){
  _DebugLevels|=(0x0000000000000001LL<<(int)Level);
}

//Get debug level description
String DebugMessages::LevelText(DebugLevel Level){
  return _DebugLevelConf[(int)Level].Description;
}

//Level2Id conversion
char DebugMessages::Level2Id(DebugLevel Level){
  return _DebugLevelConf[(int)Level].Id;
}

//Id2Level conversion
DebugLevel DebugMessages::Id2Level(char Id){
  for(int i=0;i<DEBUG_LEVELS;i++){
    if(_DebugLevelConf[i].Id==Id){ return (DebugLevel)i; }
  }
  return (DebugLevel)-1;
}

//Is dunc enabled
bool DebugMessages::IsCompilerEnabled(DebugLevel Level){ 
  return _DebugLevelConf[(int)Level].CmpEnabled; 
}

//Is dunr enabled
bool DebugMessages::IsRuntimeEnabled(DebugLevel Level){ 
  return _DebugLevelConf[(int)Level].RunEnabled; 
}

//Is duns enabled
bool DebugMessages::IsGloballyEnabled(DebugLevel Level){ 
  return _DebugLevelConf[(int)Level].GlobEnabled; 
}

//Append message
void DebugMessages::Append(const String& Message){
  _Append+=NonAsciiEscape(Message);
}

//Print message
void DebugMessages::Message(DebugLevel Level,const String& Message){
  if(DebugLevelEnabled(Level)){
    if(_ToConsole){
      std::cout << "["+String(Level2Id(Level))+"] "+_Append+NonAsciiEscape(Message) << std::endl;
    }
    else if(_Log.is_open()){ 
      _Log << ("["+String(Level2Id(Level))+"] "+_Append+NonAsciiEscape(Message)) << std::endl;
    }
  }
  _Append="";
} 

//Print message without debug level
void DebugMessages::Output(const String& Message){
  if(!_ToConsole && _Log.is_open()){ 
    _Log << NonAsciiEscape(Message) << std::endl;
  }
} 

//Shpw enabled debug levels
void DebugMessages::ShowEnabledLevels(){
  for(int i=0;i<DEBUG_LEVELS;i++){
    if(DebugLevelEnabled((DebugLevel)i)){
      std::cout << Level2Id((DebugLevel)i);
    }
  }
  std::cout << std::endl;
}
