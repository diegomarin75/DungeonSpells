//exprcomp.cpp: Expression compiler class
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
#include "cmp/parser.hpp"
#include "cmp/binary.hpp"
#include "cmp/master.hpp"
#include "cmp/exprcomp.hpp"

//Constants
const String _TernaryLabelNameSpace="CN";
const String _FlowOprLabelNameSpace="FW";

//Operator associativity
enum class ExprOpAssoc:int{ 
  Left,  //Left associativity
  Right  //Right associativity
};

//Operator class
enum class ExprOpClass:int{ 
  Unary,  //Operator takes 1 argument
  Binary  //Operator takes 2 arguments
};
  
//Operator subclass
enum class ExprOpSubClass:int{ 
  Arithmetic, //Arithmetic operators
  Logical,    //Logical operators
  Comparison, //Comparison operators
  Bitwise,    //Bitwise operators
  Member,     //Member operator
  Assignment, //Assignment operators
  Conversion, //Data conversion
  SeqOper     //Sequence operation
};

//Operator table
struct ExprOperatorProp{
  ExprOpAssoc Assoc;            //Associativity: Left or Right
  ExprOpClass Class;            //Class: Unary or Binary
  ExprOpSubClass SubClass;      //Operator SubClass
  int Prec;                     //Operator precedence
  bool IsRetTypeDeterm;         //Is result type determined ?
  bool IsResultFirst;           //Is result first operand ? (no need to create temporary variable)
  bool IsResultSecond;          //Is result second operand ? (no need to create temporary variable)
  bool CheckInitialized[2];     //Check operands are initialized
  bool LValueMandatory[2];      //Must operands be LValues ?
  bool IsOverloadable;          //Is operator overloadable ?
  String Name;                  //Operator printable string 
};

//Standard operator properties (Precedence 1 is reserved for low level operators (ternary))
const int _OprNr=45;
const int _MaxOperatorPrecedence=13;
const int _LowLevelOprPrecedence=0;
const int _FlowOprPrecedence=0;
ExprOperatorProp _Opr[_OprNr]={
// Assoc              Class                SubClass                       Pr  Deter Res1st Res2nd CIni1 Cini2   LvOp1 LvOp2   Ovrld? Name
  {ExprOpAssoc::Left ,ExprOpClass::Unary , ExprOpSubClass::Arithmetic   , 13, true ,false, false, {true ,true },{true ,false}, true , "++"      }, // ++      Postfix increment (returns original operand)
  {ExprOpAssoc::Left ,ExprOpClass::Unary , ExprOpSubClass::Arithmetic   , 13, true ,false, false, {true ,true },{true ,false}, true , "--"      }, // --      Postfix decrement (returns original operand)
  {ExprOpAssoc::Right,ExprOpClass::Unary , ExprOpSubClass::Arithmetic   , 12, true ,true,  false, {true ,true },{true ,false}, true , "(++)"    }, // ++      Prefix increment (returns modified operand)
  {ExprOpAssoc::Right,ExprOpClass::Unary , ExprOpSubClass::Arithmetic   , 12, true ,true,  false, {true ,true },{true ,false}, true , "(--)"    }, // --      Prefix decrement (returns modified operand)
  {ExprOpAssoc::Right,ExprOpClass::Unary , ExprOpSubClass::Arithmetic   , 12, true ,true,  false, {true ,true },{false,false}, true , "(+)"     }, // +       Unary plus
  {ExprOpAssoc::Right,ExprOpClass::Unary , ExprOpSubClass::Arithmetic   , 12, true ,false, false, {true ,true },{false,false}, true , "(-)"     }, // -       Unary minus
  {ExprOpAssoc::Right,ExprOpClass::Unary , ExprOpSubClass::Logical      , 12, true ,false, false, {true ,true },{false,false}, true , "!"       }, // !       Logical NOT
  {ExprOpAssoc::Right,ExprOpClass::Unary , ExprOpSubClass::Bitwise      , 12, true ,false, false, {true ,true },{false,false}, true , "~"       }, // ~       Bitwise NOT (One's Complement)
  {ExprOpAssoc::Right,ExprOpClass::Unary , ExprOpSubClass::Conversion   , 12, false,false, false, {true ,true },{false,false}, false, "(type)"  }, // (type)  Type cast operator
  {ExprOpAssoc::Left ,ExprOpClass::Binary, ExprOpSubClass::Arithmetic   , 11, true ,false, false, {true ,true },{false,false}, true , "* "      }, // *       Multiplication
  {ExprOpAssoc::Left ,ExprOpClass::Binary, ExprOpSubClass::Arithmetic   , 11, true ,false, false, {true ,true },{false,false}, true , "/ "      }, // /       Division
  {ExprOpAssoc::Left ,ExprOpClass::Binary, ExprOpSubClass::Arithmetic   , 11, true ,false, false, {true ,true },{false,false}, true , "% "      }, // %       Modulo (remainder)
  {ExprOpAssoc::Left ,ExprOpClass::Binary, ExprOpSubClass::Arithmetic   , 10, true ,false, false, {true ,true },{false,false}, true , "+ "      }, // +       Addition
  {ExprOpAssoc::Left ,ExprOpClass::Binary, ExprOpSubClass::Arithmetic   , 10, true ,false, false, {true ,true },{false,false}, true , "- "      }, // -       Subtraction
  {ExprOpAssoc::Left ,ExprOpClass::Binary, ExprOpSubClass::Bitwise      , 10, true ,false, false, {true ,true },{false,false}, true , "<<"      }, // <<      Bitwise left shift
  {ExprOpAssoc::Left ,ExprOpClass::Binary, ExprOpSubClass::Bitwise      , 10, true ,false, false, {true ,true },{false,false}, true , ">>"      }, // >>      Bitwise right shift
  {ExprOpAssoc::Left ,ExprOpClass::Binary, ExprOpSubClass::Comparison   ,  9, true ,false, false, {true ,true },{false,false}, true , "< "      }, // <       Less than
  {ExprOpAssoc::Left ,ExprOpClass::Binary, ExprOpSubClass::Comparison   ,  9, true ,false, false, {true ,true },{false,false}, true , "<="      }, // <=      Less than or equal to
  {ExprOpAssoc::Left ,ExprOpClass::Binary, ExprOpSubClass::Comparison   ,  9, true ,false, false, {true ,true },{false,false}, true , "> "      }, // >       Greater than
  {ExprOpAssoc::Left ,ExprOpClass::Binary, ExprOpSubClass::Comparison   ,  9, true ,false, false, {true ,true },{false,false}, true , ">="      }, // >=      Greater than or equal to
  {ExprOpAssoc::Left ,ExprOpClass::Binary, ExprOpSubClass::Comparison   ,  8, true ,false, false, {true ,true },{false,false}, true , "=="      }, // ==      Equal to
  {ExprOpAssoc::Left ,ExprOpClass::Binary, ExprOpSubClass::Comparison   ,  8, true ,false, false, {true ,true },{false,false}, true , "!="      }, // !=      Not equal to
  {ExprOpAssoc::Left ,ExprOpClass::Binary, ExprOpSubClass::Bitwise      ,  7, true ,false, false, {true ,true },{false,false}, true , "& "      }, // &       Bitwise AND
  {ExprOpAssoc::Left ,ExprOpClass::Binary, ExprOpSubClass::Bitwise      ,  6, true ,false, false, {true ,true },{false,false}, true , "^ "      }, // ^       Bitwise XOR (exclusive or)
  {ExprOpAssoc::Left ,ExprOpClass::Binary, ExprOpSubClass::Bitwise      ,  5, true ,false, false, {true ,true },{false,false}, true , "| "      }, // |       Bitwise OR (inclusive or)
  {ExprOpAssoc::Left ,ExprOpClass::Binary, ExprOpSubClass::Logical      ,  4, true ,false, false, {true ,true },{false,false}, true , "&&"      }, // &&      Logical AND
  {ExprOpAssoc::Left ,ExprOpClass::Binary, ExprOpSubClass::Logical      ,  3, true ,false, false, {true ,true },{false,false}, true , "||"      }, // ||      Logical OR
  {ExprOpAssoc::Right,ExprOpClass::Binary, ExprOpSubClass::Assignment   ,  2, true ,true , false, {false,true },{true ,false}, true , "= "      }, // =       Initialization of declared variable
  {ExprOpAssoc::Right,ExprOpClass::Binary, ExprOpSubClass::Assignment   ,  2, true ,true , false, {false,true },{true ,false}, true , "= "      }, // =       Direct assignment
  {ExprOpAssoc::Right,ExprOpClass::Binary, ExprOpSubClass::Assignment   ,  2, true ,true , false, {true ,true },{true ,false}, true , "+="      }, // +=      Assignment by sum
  {ExprOpAssoc::Right,ExprOpClass::Binary, ExprOpSubClass::Assignment   ,  2, true ,true , false, {true ,true },{true ,false}, true , "-="      }, // -=      Assignment by difference
  {ExprOpAssoc::Right,ExprOpClass::Binary, ExprOpSubClass::Assignment   ,  2, true ,true , false, {true ,true },{true ,false}, true , "*="      }, // *=      Assignment by product
  {ExprOpAssoc::Right,ExprOpClass::Binary, ExprOpSubClass::Assignment   ,  2, true ,true , false, {true ,true },{true ,false}, true , "/="      }, // /=      Assignment by quotient
  {ExprOpAssoc::Right,ExprOpClass::Binary, ExprOpSubClass::Assignment   ,  2, true ,true , false, {true ,true },{true ,false}, true , "%="      }, // %=      Assignment by remainder
  {ExprOpAssoc::Right,ExprOpClass::Binary, ExprOpSubClass::Assignment   ,  2, true ,true , false, {true ,true },{true ,false}, true , "<<="     }, // <<=     Assignment by bitwise left shift
  {ExprOpAssoc::Right,ExprOpClass::Binary, ExprOpSubClass::Assignment   ,  2, true ,true , false, {true ,true },{true ,false}, true , ">>="     }, // >>=     Assignment by bitwise right shift
  {ExprOpAssoc::Right,ExprOpClass::Binary, ExprOpSubClass::Assignment   ,  2, true ,true , false, {true ,true },{true ,false}, true , "&="      }, // &=      Assignment by bitwise AND
  {ExprOpAssoc::Right,ExprOpClass::Binary, ExprOpSubClass::Assignment   ,  2, true ,true , false, {true ,true },{true ,false}, true , "^="      }, // ^=      Assignment by bitwise XOR
  {ExprOpAssoc::Right,ExprOpClass::Binary, ExprOpSubClass::Assignment   ,  2, true ,true , false, {true ,true },{true ,false}, true , "|="      }, // |=      Assignment by bitwise OR
  {ExprOpAssoc::Left ,ExprOpClass::Binary, ExprOpSubClass::SeqOper      ,  1, false,false, true , {false,true },{false,false}, false, "->"      }, // ->      Sequence operator (comma operator)
};

//Cases of operands
const int _OperCaseRuleNr=185;
ExprOperCaseRule _OperCaseRule[_OperCaseRuleNr]={
// Operator                       DataPromotion  PromotionMode            PromotionType        ResultNasterType     OperandAllowedCases
  {ExprOperator::PostfixInc     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Char    , { (1<<(int)MasterType::Char) , 0 } },
  {ExprOperator::PostfixInc     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Short), 0 } },
  {ExprOperator::PostfixInc     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Integer), 0 } },
  {ExprOperator::PostfixInc     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Long), 0 } },
  {ExprOperator::PostfixInc     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Float   , { (1<<(int)MasterType::Float), 0 } },
  {ExprOperator::PostfixDec     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Char    , { (1<<(int)MasterType::Char), 0 } },
  {ExprOperator::PostfixDec     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Short), 0 } },
  {ExprOperator::PostfixDec     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Integer), 0 } },
  {ExprOperator::PostfixDec     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Long), 0 } },
  {ExprOperator::PostfixDec     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Float   , { (1<<(int)MasterType::Float), 0 } },
  {ExprOperator::PrefixInc      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Char    , { (1<<(int)MasterType::Char), 0 } },
  {ExprOperator::PrefixInc      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Short), 0 } },
  {ExprOperator::PrefixInc      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Integer), 0 } },
  {ExprOperator::PrefixInc      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Long), 0 } },
  {ExprOperator::PrefixInc      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Float   , { (1<<(int)MasterType::Float), 0 } },
  {ExprOperator::PrefixDec      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Char    , { (1<<(int)MasterType::Char), 0 } },
  {ExprOperator::PrefixDec      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Short), 0 } },
  {ExprOperator::PrefixDec      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Integer), 0 } },
  {ExprOperator::PrefixDec      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Long), 0 } },
  {ExprOperator::PrefixDec      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Float   , { (1<<(int)MasterType::Float), 0 } },
  {ExprOperator::UnaryPlus      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Char    , { (1<<(int)MasterType::Char), 0 } },
  {ExprOperator::UnaryPlus      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Short), 0 } },
  {ExprOperator::UnaryPlus      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Integer), 0 } },
  {ExprOperator::UnaryPlus      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Long), 0 } },
  {ExprOperator::UnaryPlus      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Float   , { (1<<(int)MasterType::Float), 0 } },
  {ExprOperator::UnaryMinus     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Char    , { (1<<(int)MasterType::Char), 0 } },
  {ExprOperator::UnaryMinus     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Short), 0 } },
  {ExprOperator::UnaryMinus     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Integer), 0 } },
  {ExprOperator::UnaryMinus     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Long), 0 } },
  {ExprOperator::UnaryMinus     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Float   , { (1<<(int)MasterType::Float), 0 } },
  {ExprOperator::LogicalNot     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Boolean , { (1<<(int)MasterType::Boolean), 0 } },
  {ExprOperator::BitwiseNot     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Char    , { (1<<(int)MasterType::Char), 0 } },
  {ExprOperator::BitwiseNot     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Short), 0 } },
  {ExprOperator::BitwiseNot     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Integer), 0 } },
  {ExprOperator::BitwiseNot     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Long), 0 } },
  {ExprOperator::Multiplication , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Char    , { (1<<(int)MasterType::Char), (1<<(int)MasterType::Char)  } },
  {ExprOperator::Multiplication , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Short), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short)  } },
  {ExprOperator::Multiplication , {true ,false}, ExprPromMode::ToResult , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short), (1<<(int)MasterType::Short)  } },
  {ExprOperator::Multiplication , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Integer), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer)  } },
  {ExprOperator::Multiplication , {true ,false}, ExprPromMode::ToResult , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer), (1<<(int)MasterType::Integer)  } },
  {ExprOperator::Multiplication , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Long), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long)  } },
  {ExprOperator::Multiplication , {true ,false}, ExprPromMode::ToResult , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long), (1<<(int)MasterType::Long)  } },
  {ExprOperator::Multiplication , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Float   , { (1<<(int)MasterType::Float), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float)  } },
  {ExprOperator::Multiplication , {true ,false}, ExprPromMode::ToResult , (MasterType)0      , MasterType::Float   , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float), (1<<(int)MasterType::Float)  } },
  {ExprOperator::Division       , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Char    , { (1<<(int)MasterType::Char), (1<<(int)MasterType::Char)  } },
  {ExprOperator::Division       , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Short), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short)  } },
  {ExprOperator::Division       , {true ,false}, ExprPromMode::ToResult , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short), (1<<(int)MasterType::Short)  } },
  {ExprOperator::Division       , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Integer), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer)  } },
  {ExprOperator::Division       , {true ,false}, ExprPromMode::ToResult , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer), (1<<(int)MasterType::Integer)  } },
  {ExprOperator::Division       , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Long), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long)  } },
  {ExprOperator::Division       , {true ,false}, ExprPromMode::ToResult , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long), (1<<(int)MasterType::Long)  } },
  {ExprOperator::Division       , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Float   , { (1<<(int)MasterType::Float), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float)  } },
  {ExprOperator::Division       , {true ,false}, ExprPromMode::ToResult , (MasterType)0      , MasterType::Float   , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float), (1<<(int)MasterType::Float)  } },
  {ExprOperator::Modulus        , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Char    , { (1<<(int)MasterType::Char), (1<<(int)MasterType::Char)  } },
  {ExprOperator::Modulus        , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Short), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short)  } },
  {ExprOperator::Modulus        , {true ,false}, ExprPromMode::ToResult , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short), (1<<(int)MasterType::Short)  } },
  {ExprOperator::Modulus        , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Integer), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer)  } },
  {ExprOperator::Modulus        , {true ,false}, ExprPromMode::ToResult , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer), (1<<(int)MasterType::Integer)  } },
  {ExprOperator::Modulus        , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Long), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long)  } },
  {ExprOperator::Modulus        , {true ,false}, ExprPromMode::ToResult , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long), (1<<(int)MasterType::Long)  } },
  {ExprOperator::Addition       , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Char    , { (1<<(int)MasterType::Char), (1<<(int)MasterType::Char)  } },
  {ExprOperator::Addition       , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Short), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short)  } },
  {ExprOperator::Addition       , {true ,false}, ExprPromMode::ToResult , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short), (1<<(int)MasterType::Short)  } },
  {ExprOperator::Addition       , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Integer), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer)  } },
  {ExprOperator::Addition       , {true ,false}, ExprPromMode::ToResult , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer), (1<<(int)MasterType::Integer)  } },
  {ExprOperator::Addition       , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Long), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long)  } },
  {ExprOperator::Addition       , {true ,false}, ExprPromMode::ToResult , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long), (1<<(int)MasterType::Long)  } },
  {ExprOperator::Addition       , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Float   , { (1<<(int)MasterType::Float), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float)  } },
  {ExprOperator::Addition       , {true ,false}, ExprPromMode::ToResult , (MasterType)0      , MasterType::Float   , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float), (1<<(int)MasterType::Float)  } },
  {ExprOperator::Addition       , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::String  , { (1<<(int)MasterType::String), (1<<(int)MasterType::Char) | (1<<(int)MasterType::String)  } },
  {ExprOperator::Addition       , {true ,false}, ExprPromMode::ToResult , (MasterType)0      , MasterType::String  , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::String), (1<<(int)MasterType::String)  } },
  {ExprOperator::Substraction   , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Char    , { (1<<(int)MasterType::Char), (1<<(int)MasterType::Char)  } },
  {ExprOperator::Substraction   , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Short), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short)  } },
  {ExprOperator::Substraction   , {true ,false}, ExprPromMode::ToResult , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short), (1<<(int)MasterType::Short)  } },
  {ExprOperator::Substraction   , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Integer), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer)  } },
  {ExprOperator::Substraction   , {true ,false}, ExprPromMode::ToResult , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer), (1<<(int)MasterType::Integer)  } },
  {ExprOperator::Substraction   , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Long), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long)  } },
  {ExprOperator::Substraction   , {true ,false}, ExprPromMode::ToResult , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long), (1<<(int)MasterType::Long)  } },
  {ExprOperator::Substraction   , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Float   , { (1<<(int)MasterType::Float), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float)  } },
  {ExprOperator::Substraction   , {true ,false}, ExprPromMode::ToResult , (MasterType)0      , MasterType::Float   , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float), (1<<(int)MasterType::Float)  } },
  {ExprOperator::ShiftLeft      , {false,false}, ExprPromMode::ToOther  , MasterType::Char   , MasterType::Char    , { (1<<(int)MasterType::Char), (1<<(int)MasterType::Char) } },
  {ExprOperator::ShiftLeft      , {false,true }, ExprPromMode::ToOther  , MasterType::Short  , MasterType::Short   , { (1<<(int)MasterType::Short), (1<<(int)MasterType::Char) |(1<<(int)MasterType::Short)  } },
  {ExprOperator::ShiftLeft      , {false,true }, ExprPromMode::ToOther  , MasterType::Integer, MasterType::Integer , { (1<<(int)MasterType::Integer), (1<<(int)MasterType::Char) |(1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) } },
  {ExprOperator::ShiftLeft      , {false,true }, ExprPromMode::ToOther  , MasterType::Long   , MasterType::Long    , { (1<<(int)MasterType::Long), (1<<(int)MasterType::Char) |(1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) |(1<<(int)MasterType::Long)  } },
  {ExprOperator::ShiftRight     , {false,false}, ExprPromMode::ToOther  , MasterType::Char   , MasterType::Char    , { (1<<(int)MasterType::Char), (1<<(int)MasterType::Char) } },
  {ExprOperator::ShiftRight     , {false,true }, ExprPromMode::ToOther  , MasterType::Short  , MasterType::Short   , { (1<<(int)MasterType::Short), (1<<(int)MasterType::Char) |(1<<(int)MasterType::Short)  } },
  {ExprOperator::ShiftRight     , {false,true }, ExprPromMode::ToOther  , MasterType::Integer, MasterType::Integer , { (1<<(int)MasterType::Integer), (1<<(int)MasterType::Char) |(1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) } },
  {ExprOperator::ShiftRight     , {false,true }, ExprPromMode::ToOther  , MasterType::Long   , MasterType::Long    , { (1<<(int)MasterType::Long), (1<<(int)MasterType::Char) |(1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) |(1<<(int)MasterType::Long)  } },
  {ExprOperator::Less           , {true ,true }, ExprPromMode::ToMaximun, (MasterType)0      , MasterType::Boolean , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float)  } },
  {ExprOperator::Less           , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Boolean , { (1<<(int)MasterType::String), (1<<(int)MasterType::String)  } },
  {ExprOperator::LessEqual      , {true ,true }, ExprPromMode::ToMaximun, (MasterType)0      , MasterType::Boolean , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float)  } },
  {ExprOperator::LessEqual      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Boolean , { (1<<(int)MasterType::String), (1<<(int)MasterType::String)  } },
  {ExprOperator::Greater        , {true ,true }, ExprPromMode::ToMaximun, (MasterType)0      , MasterType::Boolean , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float)  } },
  {ExprOperator::Greater        , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Boolean , { (1<<(int)MasterType::String), (1<<(int)MasterType::String)  } },
  {ExprOperator::GreaterEqual   , {true ,true }, ExprPromMode::ToMaximun, (MasterType)0      , MasterType::Boolean , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float)  } },
  {ExprOperator::GreaterEqual   , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Boolean , { (1<<(int)MasterType::String), (1<<(int)MasterType::String)  } },
  {ExprOperator::Equal          , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Boolean , { (1<<(int)MasterType::Boolean), (1<<(int)MasterType::Boolean)  } },
  {ExprOperator::Equal          , {true ,true }, ExprPromMode::ToMaximun, (MasterType)0      , MasterType::Boolean , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float)  } },
  {ExprOperator::Equal          , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Boolean , { (1<<(int)MasterType::String), (1<<(int)MasterType::String)  } },
  {ExprOperator::Equal          , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Boolean , { (1<<(int)MasterType::Enum), (1<<(int)MasterType::Enum)  } },
  {ExprOperator::Distinct       , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Boolean , { (1<<(int)MasterType::Boolean), (1<<(int)MasterType::Boolean)  } },
  {ExprOperator::Distinct       , {true ,true }, ExprPromMode::ToMaximun, (MasterType)0      , MasterType::Boolean , { (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float)  } },
  {ExprOperator::Distinct       , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Boolean , { (1<<(int)MasterType::String), (1<<(int)MasterType::String)  } },
  {ExprOperator::Distinct       , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Boolean , { (1<<(int)MasterType::Enum), (1<<(int)MasterType::Enum)  } },
  {ExprOperator::BitwiseAnd     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Char    , { (1<<(int)MasterType::Char), (1<<(int)MasterType::Char)  } },
  {ExprOperator::BitwiseAnd     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Short), (1<<(int)MasterType::Short)  } },
  {ExprOperator::BitwiseAnd     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Integer), (1<<(int)MasterType::Integer)  } },
  {ExprOperator::BitwiseAnd     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Long), (1<<(int)MasterType::Long)  } },
  {ExprOperator::BitwiseXor     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Char    , { (1<<(int)MasterType::Char), (1<<(int)MasterType::Char)  } },
  {ExprOperator::BitwiseXor     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Short), (1<<(int)MasterType::Short)  } },
  {ExprOperator::BitwiseXor     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Integer), (1<<(int)MasterType::Integer)  } },
  {ExprOperator::BitwiseXor     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Long), (1<<(int)MasterType::Long)  } },
  {ExprOperator::BitwiseOr      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Char    , { (1<<(int)MasterType::Char), (1<<(int)MasterType::Char)  } },
  {ExprOperator::BitwiseOr      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Short), (1<<(int)MasterType::Short)  } },
  {ExprOperator::BitwiseOr      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Integer), (1<<(int)MasterType::Integer)  } },
  {ExprOperator::BitwiseOr      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Long), (1<<(int)MasterType::Long)  } },
  {ExprOperator::LogicalAnd     , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Boolean , { (1<<(int)MasterType::Boolean), (1<<(int)MasterType::Boolean)  } },
  {ExprOperator::LogicalOr      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Boolean , { (1<<(int)MasterType::Boolean), (1<<(int)MasterType::Boolean)  } },
  {ExprOperator::Initializ      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Boolean , { (1<<(int)MasterType::Boolean), (1<<(int)MasterType::Boolean)  } },
  {ExprOperator::Initializ      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Char    , { (1<<(int)MasterType::Char), (1<<(int)MasterType::Char)  } },
  {ExprOperator::Initializ      , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Short), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short)  } },
  {ExprOperator::Initializ      , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Integer), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer)  } },
  {ExprOperator::Initializ      , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Long), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long)  } },
  {ExprOperator::Initializ      , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Float   , { (1<<(int)MasterType::Float), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float)  } },
  {ExprOperator::Initializ      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::String  , { (1<<(int)MasterType::String), (1<<(int)MasterType::String)  } },
  {ExprOperator::Initializ      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Enum    , { (1<<(int)MasterType::Enum), (1<<(int)MasterType::Enum)  } },
  {ExprOperator::Initializ      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Class   , { (1<<(int)MasterType::Class), (1<<(int)MasterType::Class)  } },
  {ExprOperator::Initializ      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::FixArray, { (1<<(int)MasterType::FixArray), (1<<(int)MasterType::FixArray)  } },
  {ExprOperator::Initializ      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::DynArray, { (1<<(int)MasterType::DynArray), (1<<(int)MasterType::DynArray)  } },
  {ExprOperator::Assign         , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Boolean , { (1<<(int)MasterType::Boolean), (1<<(int)MasterType::Boolean)  } },
  {ExprOperator::Assign         , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Char    , { (1<<(int)MasterType::Char), (1<<(int)MasterType::Char)  } },
  {ExprOperator::Assign         , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Short), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short)  } },
  {ExprOperator::Assign         , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Integer), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer)  } },
  {ExprOperator::Assign         , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Long), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long)  } },
  {ExprOperator::Assign         , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Float   , { (1<<(int)MasterType::Float), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float)  } },
  {ExprOperator::Assign         , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::String  , { (1<<(int)MasterType::String), (1<<(int)MasterType::String)  } },
  {ExprOperator::Assign         , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Enum    , { (1<<(int)MasterType::Enum), (1<<(int)MasterType::Enum)  } },
  {ExprOperator::Assign         , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Class   , { (1<<(int)MasterType::Class), (1<<(int)MasterType::Class)  } },
  {ExprOperator::Assign         , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::FixArray, { (1<<(int)MasterType::FixArray), (1<<(int)MasterType::FixArray)  } },
  {ExprOperator::Assign         , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::DynArray, { (1<<(int)MasterType::DynArray), (1<<(int)MasterType::DynArray)  } },
  {ExprOperator::AddAssign      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Char    , { (1<<(int)MasterType::Char), (1<<(int)MasterType::Char)  } },
  {ExprOperator::AddAssign      , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Short), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short)  } },
  {ExprOperator::AddAssign      , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Integer), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer)  } },
  {ExprOperator::AddAssign      , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Long), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long)  } },
  {ExprOperator::AddAssign      , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Float   , { (1<<(int)MasterType::Float), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float)  } },
  {ExprOperator::AddAssign      , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::String  , { (1<<(int)MasterType::String), (1<<(int)MasterType::Char) | (1<<(int)MasterType::String)  } },
  {ExprOperator::SubAssign      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Char    , { (1<<(int)MasterType::Char), (1<<(int)MasterType::Char)  } },
  {ExprOperator::SubAssign      , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Short), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short)  } },
  {ExprOperator::SubAssign      , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Integer), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer)  } },
  {ExprOperator::SubAssign      , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Long), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long)  } },
  {ExprOperator::SubAssign      , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Float   , { (1<<(int)MasterType::Float), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float)  } },
  {ExprOperator::MulAssign      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Char    , { (1<<(int)MasterType::Char), (1<<(int)MasterType::Char)  } },
  {ExprOperator::MulAssign      , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Short), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short)  } },
  {ExprOperator::MulAssign      , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Integer), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer)  } },
  {ExprOperator::MulAssign      , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Long), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long)  } },
  {ExprOperator::MulAssign      , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Float   , { (1<<(int)MasterType::Float), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float)  } },
  {ExprOperator::DivAssign      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Char    , { (1<<(int)MasterType::Char), (1<<(int)MasterType::Char)  } },
  {ExprOperator::DivAssign      , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Short), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short)  } },
  {ExprOperator::DivAssign      , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Integer), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer)  } },
  {ExprOperator::DivAssign      , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Long), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long)  } },
  {ExprOperator::DivAssign      , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Float   , { (1<<(int)MasterType::Float), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long) | (1<<(int)MasterType::Float)  } },
  {ExprOperator::ModAssign      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Char    , { (1<<(int)MasterType::Char), (1<<(int)MasterType::Char)  } },
  {ExprOperator::ModAssign      , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Short), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short)  } },
  {ExprOperator::ModAssign      , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Integer), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer)  } },
  {ExprOperator::ModAssign      , {false,true }, ExprPromMode::ToResult , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Long), (1<<(int)MasterType::Char) | (1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) | (1<<(int)MasterType::Long)  } },
  {ExprOperator::ShlAssign      , {false,false}, ExprPromMode::ToOther  , MasterType::Char   , MasterType::Char    , { (1<<(int)MasterType::Char), (1<<(int)MasterType::Char) } },
  {ExprOperator::ShlAssign      , {false,true }, ExprPromMode::ToOther  , MasterType::Short  , MasterType::Short   , { (1<<(int)MasterType::Short), (1<<(int)MasterType::Char) |(1<<(int)MasterType::Short)  } },
  {ExprOperator::ShlAssign      , {false,true }, ExprPromMode::ToOther  , MasterType::Integer, MasterType::Integer , { (1<<(int)MasterType::Integer), (1<<(int)MasterType::Char) |(1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) } },
  {ExprOperator::ShlAssign      , {false,true }, ExprPromMode::ToOther  , MasterType::Long   , MasterType::Long    , { (1<<(int)MasterType::Long), (1<<(int)MasterType::Char) |(1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) |(1<<(int)MasterType::Long)  } },
  {ExprOperator::ShrAssign      , {false,false}, ExprPromMode::ToOther  , MasterType::Char   , MasterType::Char    , { (1<<(int)MasterType::Char), (1<<(int)MasterType::Char) } },
  {ExprOperator::ShrAssign      , {false,true }, ExprPromMode::ToOther  , MasterType::Short  , MasterType::Short   , { (1<<(int)MasterType::Short), (1<<(int)MasterType::Char) |(1<<(int)MasterType::Short)  } },
  {ExprOperator::ShrAssign      , {false,true }, ExprPromMode::ToOther  , MasterType::Integer, MasterType::Integer , { (1<<(int)MasterType::Integer), (1<<(int)MasterType::Char) |(1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) } },
  {ExprOperator::ShrAssign      , {false,true }, ExprPromMode::ToOther  , MasterType::Long   , MasterType::Long    , { (1<<(int)MasterType::Long), (1<<(int)MasterType::Char) |(1<<(int)MasterType::Short) | (1<<(int)MasterType::Integer) |(1<<(int)MasterType::Long)  } },
  {ExprOperator::AndAssign      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Char    , { (1<<(int)MasterType::Char), (1<<(int)MasterType::Char)  } },
  {ExprOperator::AndAssign      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Short), (1<<(int)MasterType::Short)  } },
  {ExprOperator::AndAssign      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Integer), (1<<(int)MasterType::Integer)  } },
  {ExprOperator::AndAssign      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Long), (1<<(int)MasterType::Long)  } },
  {ExprOperator::XorAssign      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Char    , { (1<<(int)MasterType::Char), (1<<(int)MasterType::Char)  } },
  {ExprOperator::XorAssign      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Short), (1<<(int)MasterType::Short)  } },
  {ExprOperator::XorAssign      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Integer), (1<<(int)MasterType::Integer)  } },
  {ExprOperator::XorAssign      , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Long), (1<<(int)MasterType::Long)  } },
  {ExprOperator::OrAssign       , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Char    , { (1<<(int)MasterType::Char), (1<<(int)MasterType::Char)  } },
  {ExprOperator::OrAssign       , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Short   , { (1<<(int)MasterType::Short), (1<<(int)MasterType::Short)  } },
  {ExprOperator::OrAssign       , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Integer , { (1<<(int)MasterType::Integer), (1<<(int)MasterType::Integer)  } },
  {ExprOperator::OrAssign       , {false,false}, (ExprPromMode)0        , (MasterType)0      , MasterType::Long    , { (1<<(int)MasterType::Long), (1<<(int)MasterType::Long)  } },
};

//Get expression token id text
String ExprTokenIdText(ExprTokenId Id){
  String Text="undefined";
  switch(Id){
    case ExprTokenId::Operand    : Text="expression operand"; break;
    case ExprTokenId::UndefVar   : Text="undefined variable"; break;
    case ExprTokenId::Operator   : Text="operator"; break;
    case ExprTokenId::LowLevelOpr: Text="low level operator"; break;
    case ExprTokenId::FlowOpr    : Text="flow operator"; break;
    case ExprTokenId::Field      : Text="field member"; break;
    case ExprTokenId::Method     : Text="function member"; break;
    case ExprTokenId::Constructor: Text="class constructor call"; break;
    case ExprTokenId::Subscript  : Text="array subscipt"; break;
    case ExprTokenId::Function   : Text="function index"; break;
    case ExprTokenId::Complex    : Text="complex value"; break;
    case ExprTokenId::Delimiter  : Text="expression delimiter"; break;
    case ExprTokenId::VoidRes    : Text="void result"; break;
  }
  return Text;
}

//Parse type specification
bool ReadTypeSpec(MasterData *Md,Sentence& Stn,ScopeDef Scope,int& TypIndex,bool AsmCommentFixArray){

  //Variable
  int i;
  int CommaNr;
  int DimNr;
  int ModIndex;
  int BareTypIndex;
  int ArrTypIndex;
  int CommaPos;
  int EndBracketPos;
  bool IsFixedArray;
  bool OperatorFunc;
  int DimIndex;
  int DimStart;
  int DimEnd;
  bool Glob;
  bool DimLoopExit;
  bool DefineFixArray;
  CpuWrd CellSize;
  Expression Expr;
  ExprToken Result;
  ArrayIndexes DimSize;
  CpuAgx GeomIndex;
  String Id;
  String PrTypeName;
  String BareTypeName;
  String ArrTypeName;
  SubScopeDef SubScope;
  SourceInfo SrcInfo;

  //Get type name from sentence
  if(!Stn.ReadTy(PrTypeName).Ok()){ return false; }
  SrcInfo=Stn.LastSrcInfo();
  Md->DecoupleTypeName(PrTypeName,BareTypeName,ModIndex);
  if((BareTypIndex=Md->TypSearch(BareTypeName,ModIndex))==-1){
    Stn.Msg(25).Print(PrTypeName);
    return false;
  }

  //Check if we have an operator function definition at this point
  if(Stn.Is(PrPunctuator::BegBracket) && Stn.Is(PrTokenId::Operator,+1) && Stn.Is(PrPunctuator::EndBracket,+2) && Stn.Is(PrPunctuator::BegParen,+3)){
    OperatorFunc=true;
  }
  else{
    OperatorFunc=false;
  }

  //If we do not have an array type we end here
  if(!Stn.Is(PrPunctuator::BegBracket) || OperatorFunc){
    TypIndex=BareTypIndex;
    return true;
  }

  //Eat bracket for dimension specification
  if(!Stn.Get(PrPunctuator::BegBracket).Ok()){ return false; }
  
  //Dynamic array specification
  if(Stn.Is(PrPunctuator::Comma) || Stn.Is(PrPunctuator::EndBracket)){
    if(!Stn.Count(PrPunctuator::Comma,CommaNr).Get(PrPunctuator::EndBracket).Ok()){ return false; }
    DimNr=CommaNr+1;
    IsFixedArray=false;
  }

  //Fixed array specification
  else{
    DimNr=0;
    IsFixedArray=true;
    DimLoopExit=false;
    DefineFixArray=false;
    for(i=0;i<_MaxArrayDims;i++){ DimSize.n[i]=0; }
    do{
      CommaPos=Stn.ZeroFind(PrPunctuator::Comma);
      EndBracketPos=Stn.ZeroFind(PrPunctuator::EndBracket);
      if(CommaPos!=-1 && (EndBracketPos==-1 || CommaPos<EndBracketPos)){
        if(!Stn.ReadEx(PrPunctuator::Comma,DimStart,DimEnd).Get(PrPunctuator::Comma).Ok()){ return false; } else{ DimNr++; }
      }
      else if(EndBracketPos!=-1 && (CommaPos==-1 || EndBracketPos<CommaPos)){
        if(!Stn.ReadEx(PrPunctuator::EndBracket,DimStart,DimEnd).Get(PrPunctuator::EndBracket).Ok()){ return false; } else{ DimNr++; DimLoopExit=true; }
      }
      else{ Stn.Msg(156).Print(); return false; }
      if(!Expr.Compute(Md,Scope,Stn,DimStart,DimEnd,Result)){ return false; }
      if(!Result.ToWrd()){ return false; }
      DimSize.n[DimNr-1]=(GetArchitecture()==32?Result.Value.Int:Result.Value.Lon);
    }while(!DimLoopExit);
  }
  
  //Compose array type name
  if(IsFixedArray){
    ArrTypeName=BareTypeName+"[";
    for(i=0;i<DimNr;i++){ ArrTypeName+=(i!=0?",":"")+ToString(DimSize.n[i]); }
    ArrTypeName+="]";
  }
  else{
    ArrTypeName=BareTypeName+"["+String(DimNr-1,',')+"]";
  }

  //For the new type to be created we will use the scope of the bare data type
  ScopeDef WorkScope=Md->Types[BareTypIndex].Scope;

  //Check array type is already defined
  ArrTypIndex=Md->TypSearch(ArrTypeName,WorkScope.ModIndex);
  
  //If array is not defined and it is fixed we must assign geometry index and define it
  if(ArrTypIndex==-1){
    if(IsFixedArray){
      Glob=(WorkScope.Kind==ScopeKind::Local?false:true);
      GeomIndex=(Glob?Md->Bin.GetGlobGeometryIndex():Md->Bin.GetLoclGeometryIndex());
      Md->StoreDimension(DimSize,GeomIndex);
      DimIndex=Md->Dimensions.Length()-1;
      CellSize=Md->Types[BareTypIndex].Length;
      DefineFixArray=true;
    }
    else{
      DimIndex=-1;
    }
  }

  //Create type if it does not exist
  if(ArrTypIndex==-1){
    SubScope=(SubScopeDef){SubScopeKind::None,-1};
    if(IsFixedArray){
      Md->StoreType(WorkScope,SubScope,ArrTypeName,MasterType::FixArray,false,-1,false,0,DimNr,BareTypIndex,DimIndex,-1,-1,-1,-1,true,SrcInfo);
      ArrTypIndex=Md->Types.Length()-1;
      Md->Dimensions[DimIndex].TypIndex=ArrTypIndex;
      if(Md->Dimensions[DimIndex].LnkSymIndex!=-1){
        Md->Bin.UpdateLnkSymDimension(Md->Dimensions[DimIndex].LnkSymIndex,ArrTypIndex);
      }
    }
    else{
      Md->StoreType(WorkScope,SubScope,ArrTypeName,MasterType::DynArray,false,-1,false,0,DimNr,BareTypIndex,DimIndex,-1,-1,-1,-1,true,SrcInfo);
      ArrTypIndex=Md->Types.Length()-1;
    }
  }

  //Define fixed array
  if(IsFixedArray && DefineFixArray){
    if(AsmCommentFixArray){
      Md->Bin.AsmOutNewLine(AsmSection::Body);
      Md->Bin.AsmOutCommentLine(AsmSection::Body,"Fixed array geometry "+ToString(GeomIndex)+" definition for data type "+ArrTypeName+" in scope "+Md->ScopeName(WorkScope),true);
    }
    if(WorkScope.Kind==ScopeKind::Local){
      if(!Md->Bin.AsmWriteCode(CpuInstCode::AFDEF,Md->AsmAgx(ArrTypIndex),Md->Bin.AsmLitChr(DimNr),Md->Bin.AsmLitWrd(CellSize))){ return false; }
      for(i=0;i<DimNr;i++){ if(!Md->Bin.AsmWriteCode(CpuInstCode::AFSSZ,Md->AsmAgx(ArrTypIndex),Md->Bin.AsmLitChr(i+1),Md->Bin.AsmLitWrd(DimSize.n[i]))){ return false; } }
    }
    else{
      String ArrIndexes="";
      for(i=0;i<DimNr;i++){ ArrIndexes+=(ArrIndexes.Length()!=0?",":"")+ToString(DimSize.n[i]); }
      ArrIndexes="["+ArrIndexes+"]";
      Md->Bin.AsmOutLine(AsmSection::Body,"","GFDEF "+Md->AsmAgx(ArrTypIndex).ToText()+","+Md->Bin.AsmLitChr(DimNr).ToText()+","+Md->Bin.AsmLitWrd(CellSize).ToText()+","+ArrIndexes);
      Md->Bin.StoreArrFixDef((ArrayFixDef){Md->Dimensions[Md->Types[ArrTypIndex].DimIndex].GeomIndex,DimNr,CellSize,DimSize});
    }
  }

  //Return data type
  TypIndex=ArrTypIndex;
  return true;

}

//Destroy token id
void ExprToken::_DestroyId(){
  switch(_Id){
    case ExprTokenId::Operator:     Value.Operator=(ExprOperator)-1; break;
    case ExprTokenId::LowLevelOpr:  Value.LowLevelOpr=(ExprLowLevelOpr)-1; break;
    case ExprTokenId::FlowOpr:      Value.FlowOpr=(ExprFlowOpr)-1; break;
    case ExprTokenId::Subscript:    Value.DimNr=0; break;
    case ExprTokenId::Function:     Value.Function.~String(); break;
    case ExprTokenId::Delimiter:    Value.Delimiter=(ExprDelimiter)-1; break;
    case ExprTokenId::Field:        Value.Field.~String(); break;
    case ExprTokenId::Method:       Value.Method.~String(); break;
    case ExprTokenId::Constructor:  Value.CCTypIndex=-1; break;
    case ExprTokenId::Complex:      Value.ComplexTypIndex=-1; break;
    case ExprTokenId::Operand:      Value.VarIndex=-1; break;
    case ExprTokenId::UndefVar:     Value.VarName.~String(); break;
    case ExprTokenId::VoidRes:      Value.VoidFuncName.~String(); break;
  }
}

//Expression token reset 
void ExprToken::_Reset(bool ObjectExists){
  _Md=nullptr;
  _FileName="";
  _LineNr=0;
  _ColNr=-1;
  AdrMode=(CpuAdrMode)0;
  CastTypIndex=-1;
  LitNumTypIndex=-1;
  LabelSeed=-1;
  FlowLabel=-1;
  CallParmNr=0;
  DimSize=ZERO_INDEXES;
  FunModIndex=-1;
  SourceVarIndex=-1;
  IsConst=false;
  IsCalculated=false;
  HasInitialization=false;
  Meta.Case=(MetaConstCase)0;
  Meta.TypIndex=-1;
  Meta.VarIndex=-1; 
  if(ObjectExists){ Id((ExprTokenId)-1); } else{ _Id=(ExprTokenId)-1; }
} 

//ExprToken move
void ExprToken::_Move(const ExprToken& Token){
  _Md=Token._Md;
  AdrMode=Token.AdrMode;
  _FileName=Token._FileName;
  _LineNr=Token._LineNr;
  _ColNr=Token._ColNr;
  CastTypIndex=Token.CastTypIndex;
  LitNumTypIndex=Token.LitNumTypIndex;
  LabelSeed=Token.LabelSeed;
  FlowLabel=Token.FlowLabel;
  CallParmNr=Token.CallParmNr;
  DimSize=Token.DimSize;
  FunModIndex=Token.FunModIndex;
  SourceVarIndex=Token.SourceVarIndex;
  IsConst=Token.IsConst;
  IsCalculated=Token.IsCalculated;
  HasInitialization=Token.HasInitialization;
  Meta.Case=Token.Meta.Case;
  Meta.TypIndex=Token.Meta.TypIndex;
  Meta.VarIndex=Token.Meta.VarIndex;
  Id(Token._Id);
  switch(_Id){
    case ExprTokenId::Operator:    Value.Operator=Token.Value.Operator; break;
    case ExprTokenId::LowLevelOpr: Value.LowLevelOpr=Token.Value.LowLevelOpr; break;
    case ExprTokenId::FlowOpr:     Value.FlowOpr=Token.Value.FlowOpr; break;
    case ExprTokenId::Subscript:   Value.DimNr=Token.Value.DimNr; break;
    case ExprTokenId::Function:    Value.Function=Token.Value.Function; break;
    case ExprTokenId::Delimiter:   Value.Delimiter=Token.Value.Delimiter; break;
    case ExprTokenId::Field:       Value.Field=Token.Value.Field; break;
    case ExprTokenId::Method:      Value.Method=Token.Value.Method; break;
    case ExprTokenId::Constructor: Value.CCTypIndex=Token.Value.CCTypIndex; break;
    case ExprTokenId::Complex:     Value.ComplexTypIndex=Token.Value.ComplexTypIndex; break;
    case ExprTokenId::UndefVar:    Value.VarName=Token.Value.VarName; break;
    case ExprTokenId::VoidRes:     Value.VoidFuncName=Token.Value.VoidFuncName; break;
    case ExprTokenId::Operand:    
      if(Token.AdrMode==CpuAdrMode::LitValue){
        switch(_Md->Types[LitNumTypIndex].MstType){
          case MasterType::Boolean : Value.Bol=Token.Value.Bol; break;
          case MasterType::Char    : Value.Chr=Token.Value.Chr; break;
          case MasterType::Short   : Value.Shr=Token.Value.Shr; break;
          case MasterType::Integer : Value.Int=Token.Value.Int; break;
          case MasterType::Long    : Value.Lon=Token.Value.Lon; break;
          case MasterType::Float   : Value.Flo=Token.Value.Flo; break;
          case MasterType::String  : Value.Adr=Token.Value.Adr; break;
          case MasterType::DynArray: Value.Adr=Token.Value.Adr; break;
          case MasterType::Enum    : Value.Enu=Token.Value.Enu; break;
          case MasterType::FixArray: break;
          case MasterType::Class   : break;
        }
      }
      else{
        Value.VarIndex=Token.Value.VarIndex;
      }
      break;
  }
}

//Expression token clear 
void ExprToken::Clear(){
  _Reset(true);
} 

//Set token id
//Syntax for strings is weird but that is the way to invoke object constructor for existing object
void ExprToken::Id(ExprTokenId Id){
  _DestroyId();
  _Id=Id;
  switch(_Id){
    case ExprTokenId::Operator:    Value.Operator=(ExprOperator)-1; break;
    case ExprTokenId::LowLevelOpr: Value.LowLevelOpr=(ExprLowLevelOpr)-1; break;
    case ExprTokenId::FlowOpr:     Value.FlowOpr=(ExprFlowOpr)-1; break;
    case ExprTokenId::Subscript:   Value.DimNr=0; break;
    case ExprTokenId::Function:    new(&Value.Function) String(); break;
    case ExprTokenId::Delimiter:   Value.Delimiter=(ExprDelimiter)-1; break;
    case ExprTokenId::Field:       new(&Value.Field) String(); break;
    case ExprTokenId::Method:      new(&Value.Method) String(); break;
    case ExprTokenId::Constructor: Value.CCTypIndex=-1; break;
    case ExprTokenId::Complex:     Value.ComplexTypIndex=-1; break;
    case ExprTokenId::Operand:     Value.VarIndex=-1; break;
    case ExprTokenId::UndefVar:    new(&Value.VarName) String(); break;
    case ExprTokenId::VoidRes:     new(&Value.VoidFuncName) String(); break;
  }
}

//Create new operand
bool ExprToken::_New(MasterData *Md,const ScopeDef& Scope,CpuLon CodeBlockId,int TypIndex,CpuAdrMode Mode,bool IsReference,bool IsConstant,const SourceInfo& SrcInfo,TempVarKind TempKind){
  
  //Variables
  int VarIndex;
  
  //Get new temporary variable
  if(!Md->TempVarNew(Scope,CodeBlockId,TypIndex,IsReference,SrcInfo,TempKind,VarIndex)){ return false; }

  //Get new token
  Clear();
  Id(ExprTokenId::Operand);
  SetMdPointer(Md);
  AdrMode=Mode;
  Value.VarIndex=VarIndex;
  IsConst=IsConstant;
  _FileName=SrcInfo.FileName;
  _LineNr=SrcInfo.LineNr;
  _ColNr=SrcInfo.ColNr;

  //Return code
  return true;

}

//Set operand as litteral enumerated value
void ExprToken::ThisEnu(MasterData *Md,int EnumTypIndex,CpuInt EnumValue,const SourceInfo& SrcInfo){
  Clear();
  Id(ExprTokenId::Operand);
  SetMdPointer(Md);
  AdrMode=CpuAdrMode::LitValue;
  Value.Enu=EnumValue;
  IsConst=true;
  LitNumTypIndex=EnumTypIndex;
  _FileName=SrcInfo.FileName;
  _LineNr=SrcInfo.LineNr;
  _ColNr=SrcInfo.ColNr;
}

//Set operand as litteral boolean
void ExprToken::ThisBol(MasterData *Md,CpuBol Bol,const SourceInfo& SrcInfo){
  Clear();
  Id(ExprTokenId::Operand);
  SetMdPointer(Md);
  AdrMode=CpuAdrMode::LitValue;
  Value.Bol=Bol;
  IsConst=true;
  LitNumTypIndex=_Md->BolTypIndex;
  _FileName=SrcInfo.FileName;
  _LineNr=SrcInfo.LineNr;
  _ColNr=SrcInfo.ColNr;
}

//Set operand as litteral char
void ExprToken::ThisChr(MasterData *Md,CpuChr Chr,const SourceInfo& SrcInfo){
  Clear();
  Id(ExprTokenId::Operand);
  SetMdPointer(Md);
  AdrMode=CpuAdrMode::LitValue;
  Value.Chr=Chr;
  IsConst=true;
  LitNumTypIndex=_Md->ChrTypIndex;
  _FileName=SrcInfo.FileName;
  _LineNr=SrcInfo.LineNr;
  _ColNr=SrcInfo.ColNr;
}

//Set operand as litteral short
void ExprToken::ThisShr(MasterData *Md,CpuShr Shr,const SourceInfo& SrcInfo){
  Clear();
  Id(ExprTokenId::Operand);
  SetMdPointer(Md);
  AdrMode=CpuAdrMode::LitValue;
  Value.Shr=Shr;
  IsConst=true;
  LitNumTypIndex=_Md->ShrTypIndex;
  _FileName=SrcInfo.FileName;
  _LineNr=SrcInfo.LineNr;
  _ColNr=SrcInfo.ColNr;
}

//Set operand as litteral integer
void ExprToken::ThisInt(MasterData *Md,CpuInt Int,const SourceInfo& SrcInfo){
  Clear();
  Id(ExprTokenId::Operand);
  SetMdPointer(Md);
  AdrMode=CpuAdrMode::LitValue;
  Value.Int=Int;
  IsConst=true;
  LitNumTypIndex=_Md->IntTypIndex;
  _FileName=SrcInfo.FileName;
  _LineNr=SrcInfo.LineNr;
  _ColNr=SrcInfo.ColNr;
}

//Set operand as litteral long
void ExprToken::ThisLon(MasterData *Md,CpuLon Lon,const SourceInfo& SrcInfo){
  Clear();
  Id(ExprTokenId::Operand);
  SetMdPointer(Md);
  AdrMode=CpuAdrMode::LitValue;
  Value.Lon=Lon;
  IsConst=true;
  LitNumTypIndex=_Md->LonTypIndex;
  _FileName=SrcInfo.FileName;
  _LineNr=SrcInfo.LineNr;
  _ColNr=SrcInfo.ColNr;
}

//Set operand as litteral float
void ExprToken::ThisFlo(MasterData *Md,CpuFlo Flo,const SourceInfo& SrcInfo){
  Clear();
  Id(ExprTokenId::Operand);
  SetMdPointer(Md);
  AdrMode=CpuAdrMode::LitValue;
  Value.Flo=Flo;
  IsConst=true;
  LitNumTypIndex=_Md->FloTypIndex;
  _FileName=SrcInfo.FileName;
  _LineNr=SrcInfo.LineNr;
  _ColNr=SrcInfo.ColNr;
}

//Set operand as litteral cpu word
void ExprToken::ThisWrd(MasterData *Md,CpuWrd Wrd,const SourceInfo& SrcInfo){
  if(GetArchitecture()==32){
    ThisInt(Md,Wrd,SrcInfo);
  }
  else{
    ThisLon(Md,Wrd,SrcInfo);
  }
}

//Set operand as existing variable
void ExprToken::ThisVar(MasterData *Md,int VarIndex,const SourceInfo& SrcInfo){
  Clear();
  Id(ExprTokenId::Operand);
  SetMdPointer(Md);
  AdrMode=CpuAdrMode::Address;
  Value.VarIndex=VarIndex;
  _FileName=SrcInfo.FileName;
  _LineNr=SrcInfo.LineNr;
  _ColNr=SrcInfo.ColNr;
}

//Set operand as existing variable (indirection)
void ExprToken::ThisInd(MasterData *Md,int VarIndex,const SourceInfo& SrcInfo){
  Clear();
  Id(ExprTokenId::Operand);
  SetMdPointer(Md);
  AdrMode=CpuAdrMode::Indirection;
  Value.VarIndex=VarIndex;
  _FileName=SrcInfo.FileName;
  _LineNr=SrcInfo.LineNr;
  _ColNr=SrcInfo.ColNr;
}

//Set operand as meta field names
void ExprToken::AsMetaFldNames(MasterData *Md,int TypIndex,const SourceInfo& SrcInfo){
  Clear();
  Id(ExprTokenId::Operand);
  SetMdPointer(Md);
  AdrMode=CpuAdrMode::LitValue;
  Value.Adr=Md->Types[TypIndex].MetaStNames;
  IsConst=true;
  LitNumTypIndex=_Md->StaTypIndex;
  Meta.Case=MetaConstCase::FldNames;
  Meta.TypIndex=TypIndex;
  Meta.VarIndex=-1;
  _FileName=SrcInfo.FileName;
  _LineNr=SrcInfo.LineNr;
  _ColNr=SrcInfo.ColNr;
}

//Set operand as meta field types
void ExprToken::AsMetaFldTypes(MasterData *Md,int TypIndex,const SourceInfo& SrcInfo){
  Clear();
  Id(ExprTokenId::Operand);
  SetMdPointer(Md);
  AdrMode=CpuAdrMode::LitValue;
  Value.Adr=Md->Types[TypIndex].MetaStTypes;
  IsConst=true;
  LitNumTypIndex=_Md->StaTypIndex;
  Meta.Case=MetaConstCase::FldTypes;
  Meta.TypIndex=TypIndex;
  Meta.VarIndex=-1;
  _FileName=SrcInfo.FileName;
  _LineNr=SrcInfo.LineNr;
  _ColNr=SrcInfo.ColNr;
}

//Set operand as meta type name
void ExprToken::AsMetaTypName(MasterData *Md,int TypIndex,const SourceInfo& SrcInfo){
  Clear();
  Id(ExprTokenId::Operand);
  SetMdPointer(Md);
  AdrMode=CpuAdrMode::LitValue;
  Value.Adr=Md->Types[TypIndex].MetaName;
  IsConst=true;
  LitNumTypIndex=_Md->StrTypIndex;
  Meta.Case=MetaConstCase::TypName;
  Meta.TypIndex=TypIndex;
  Meta.VarIndex=-1;
  _FileName=SrcInfo.FileName;
  _LineNr=SrcInfo.LineNr;
  _ColNr=SrcInfo.ColNr;
}

//Set operand as meta variable name
void ExprToken::AsMetaVarName(MasterData *Md,int VarIndex,const SourceInfo& SrcInfo){
  Clear();
  Id(ExprTokenId::Operand);
  SetMdPointer(Md);
  AdrMode=CpuAdrMode::LitValue;
  Value.Adr=Md->Variables[VarIndex].MetaName;
  IsConst=true;
  LitNumTypIndex=_Md->StrTypIndex;
  Meta.Case=MetaConstCase::VarName;
  Meta.TypIndex=-1;
  Meta.VarIndex=VarIndex;
  _FileName=SrcInfo.FileName;
  _LineNr=SrcInfo.LineNr;
  _ColNr=SrcInfo.ColNr;
}

//New constant operand
bool ExprToken::NewConst(MasterData *Md,MasterType MstType,const SourceInfo& SrcInfo){
  switch(MstType){
    case MasterType::Boolean : ThisBol(Md,0,SrcInfo); break;
    case MasterType::Char    : ThisChr(Md,0,SrcInfo); break;
    case MasterType::Short   : ThisShr(Md,0,SrcInfo); break;
    case MasterType::Integer : ThisInt(Md,0,SrcInfo); break;
    case MasterType::Long    : ThisLon(Md,0,SrcInfo); break;
    case MasterType::Float   : ThisFlo(Md,0,SrcInfo); break;
    case MasterType::String  :
    case MasterType::Enum    :
    case MasterType::Class   :
    case MasterType::FixArray:
    case MasterType::DynArray:
      SysMessage(149).Print(MasterTypeName(MstType));
      return false;
      break;
  }
  return true;
}

//New constant operand
bool ExprToken::NewConst(MasterData *Md,const ScopeDef& Scope,CpuLon CodeBlockId,int TypIndex,const SourceInfo& SrcInfo){
  if(!_New(Md,Scope,CodeBlockId,TypIndex,CpuAdrMode::Address,false,true,SrcInfo,TempVarKind::Regular)){ return false; }
  return true;
}

//New variable operand (through master type)
bool ExprToken::NewVar(MasterData *Md,const ScopeDef& Scope,CpuLon CodeBlockId,MasterType MstType,const SourceInfo& SrcInfo){
  int TypIndex;
  switch(MstType){
    case MasterType::Boolean : TypIndex=Md->BolTypIndex; break;
    case MasterType::Char    : TypIndex=Md->ChrTypIndex; break;
    case MasterType::Short   : TypIndex=Md->ShrTypIndex; break;
    case MasterType::Integer : TypIndex=Md->IntTypIndex; break;
    case MasterType::Long    : TypIndex=Md->LonTypIndex; break;
    case MasterType::Float   : TypIndex=Md->FloTypIndex; break;
    case MasterType::String  : TypIndex=Md->StrTypIndex; break;
    case MasterType::Enum    :
    case MasterType::Class   :
    case MasterType::FixArray:
    case MasterType::DynArray:
      SrcInfo.Msg(68).Print();
      return false;
  }
  if(!_New(Md,Scope,CodeBlockId,TypIndex,CpuAdrMode::Address,false,false,SrcInfo,TempVarKind::Regular)){ return false; }
  return true;
}

//New variable operand (through master type)
bool ExprToken::NewVar(MasterData *Md,const ScopeDef& Scope,CpuLon CodeBlockId,MasterType MstType,const SourceInfo& SrcInfo,TempVarKind TempKind){
  int TypIndex;
  switch(MstType){
    case MasterType::Boolean : TypIndex=Md->BolTypIndex; break;
    case MasterType::Char    : TypIndex=Md->ChrTypIndex; break;
    case MasterType::Short   : TypIndex=Md->ShrTypIndex; break;
    case MasterType::Integer : TypIndex=Md->IntTypIndex; break;
    case MasterType::Long    : TypIndex=Md->LonTypIndex; break;
    case MasterType::Float   : TypIndex=Md->FloTypIndex; break;
    case MasterType::String  : TypIndex=Md->StrTypIndex; break;
    case MasterType::Enum    :
    case MasterType::Class   :
    case MasterType::FixArray:
    case MasterType::DynArray:
      SrcInfo.Msg(68).Print();
      return false;
  }
  if(!_New(Md,Scope,CodeBlockId,TypIndex,CpuAdrMode::Address,false,false,SrcInfo,TempKind)){ return false; }
  return true;
}

//New variable operand
bool ExprToken::NewVar(MasterData *Md,const ScopeDef& Scope,CpuLon CodeBlockId,int TypIndex,const SourceInfo& SrcInfo){
  if(!_New(Md,Scope,CodeBlockId,TypIndex,CpuAdrMode::Address,false,false,SrcInfo,TempVarKind::Regular)){ return false; }
  return true;
}

//New variable operand (returns if temp variable was reused)
bool ExprToken::NewVar(MasterData *Md,const ScopeDef& Scope,CpuLon CodeBlockId,int TypIndex,const SourceInfo& SrcInfo,bool& Reused){
  int VarCount=Md->Variables.Length();
  if(!_New(Md,Scope,CodeBlockId,TypIndex,CpuAdrMode::Address,false,false,SrcInfo,TempVarKind::Regular)){ return false; }
  if(VarCount==Md->Variables.Length()){ Reused=true; } else{ Reused=false; }
  return true;
}

//New reference operand
bool ExprToken::NewInd(MasterData *Md,const ScopeDef& Scope,CpuLon CodeBlockId,int TypIndex,bool IsConstant,const SourceInfo& SrcInfo,TempVarKind TempKind){
  if(!_New(Md,Scope,CodeBlockId,TypIndex,CpuAdrMode::Indirection,true,IsConstant,SrcInfo,TempKind)){ return false; }
  return true;
}

//New pointer operand
bool ExprToken::NewPtr(MasterData *Md,const ScopeDef& Scope,CpuLon CodeBlockId,int TypIndex,bool IsConstant,const SourceInfo& SrcInfo){
  if(!_New(Md,Scope,CodeBlockId,TypIndex,CpuAdrMode::Address,true,IsConstant,SrcInfo,TempVarKind::Regular)){ return false; }
  return true;
}

//On-the-fly data conversion to Bol (supported only for litteral numeric types)
bool ExprToken::ToBol(){
  if(_Id==ExprTokenId::Operand && AdrMode==CpuAdrMode::LitValue){
    switch(_Md->Types[LitNumTypIndex].MstType){
      case MasterType::Boolean : Value.Bol=(Value.Bol!=0?true:false); LitNumTypIndex=_Md->BolTypIndex; break;
      case MasterType::Char    : Value.Bol=(Value.Chr!=0?true:false); LitNumTypIndex=_Md->BolTypIndex; break;
      case MasterType::Short   : Value.Bol=(Value.Shr!=0?true:false); LitNumTypIndex=_Md->BolTypIndex; break;
      case MasterType::Integer : Value.Bol=(Value.Int!=0?true:false); LitNumTypIndex=_Md->BolTypIndex; break;
      case MasterType::Long    : Value.Bol=(Value.Lon!=0?true:false); LitNumTypIndex=_Md->BolTypIndex; break;
      case MasterType::Float   : Value.Bol=(Value.Flo!=0?true:false); LitNumTypIndex=_Md->BolTypIndex; break;
      case MasterType::String  : 
      case MasterType::Class   :
      case MasterType::Enum    :
      case MasterType::FixArray: 
      case MasterType::DynArray: 
        Msg(150).Print(MasterTypeName(_Md->Types[LitNumTypIndex].MstType),MasterTypeName(MasterType::Boolean));
        return false;
        break;
    }
  }
  else{
    Msg(151).Print(MasterTypeName(_Md->Types[_Md->BolTypIndex].MstType));
    return false;
  }
  return true;
}

//On-the-fly data conversion to Chr (supported only for numeric types)
bool ExprToken::ToChr(){
  if(_Id==ExprTokenId::Operand && AdrMode==CpuAdrMode::LitValue){
    switch(_Md->Types[LitNumTypIndex].MstType){
      case MasterType::Boolean : Value.Chr=(CpuChr)Value.Bol; LitNumTypIndex=_Md->ChrTypIndex; break;
      case MasterType::Char    : Value.Chr=(CpuChr)Value.Chr; LitNumTypIndex=_Md->ChrTypIndex; break;
      case MasterType::Short   : Value.Chr=(CpuChr)Value.Shr; LitNumTypIndex=_Md->ChrTypIndex; break;
      case MasterType::Integer : Value.Chr=(CpuChr)Value.Int; LitNumTypIndex=_Md->ChrTypIndex; break;
      case MasterType::Long    : Value.Chr=(CpuChr)Value.Lon; LitNumTypIndex=_Md->ChrTypIndex; break;
      case MasterType::Float   : Value.Chr=(CpuChr)Value.Flo; LitNumTypIndex=_Md->ChrTypIndex; break;
      case MasterType::String  : 
      case MasterType::Class   :
      case MasterType::Enum    :
      case MasterType::FixArray: 
      case MasterType::DynArray: 
        Msg(150).Print(MasterTypeName(_Md->Types[LitNumTypIndex].MstType),MasterTypeName(MasterType::Char));
        return false;
        break;
    }
  }
  else{
    Msg(151).Print(MasterTypeName(_Md->Types[_Md->ChrTypIndex].MstType));
    return false;
  }
  return true;
}

//On-the-fly data conversion to Shr (supported only for numeric types)
bool ExprToken::ToShr(){
  if(_Id==ExprTokenId::Operand && AdrMode==CpuAdrMode::LitValue){
    switch(_Md->Types[LitNumTypIndex].MstType){
      case MasterType::Boolean : Value.Shr=(CpuShr)Value.Bol; LitNumTypIndex=_Md->ShrTypIndex; break;
      case MasterType::Char    : Value.Shr=(CpuShr)Value.Chr; LitNumTypIndex=_Md->ShrTypIndex; break;
      case MasterType::Short   : Value.Shr=(CpuShr)Value.Shr; LitNumTypIndex=_Md->ShrTypIndex; break;
      case MasterType::Integer : Value.Shr=(CpuShr)Value.Int; LitNumTypIndex=_Md->ShrTypIndex; break;
      case MasterType::Long    : Value.Shr=(CpuShr)Value.Lon; LitNumTypIndex=_Md->ShrTypIndex; break;
      case MasterType::Float   : Value.Shr=(CpuShr)Value.Flo; LitNumTypIndex=_Md->ShrTypIndex; break;
      case MasterType::String  : 
      case MasterType::Class   :
      case MasterType::Enum    :
      case MasterType::FixArray: 
      case MasterType::DynArray: 
        Msg(150).Print(MasterTypeName(_Md->Types[LitNumTypIndex].MstType),MasterTypeName(MasterType::Short));
        return false;
        break;
    }
  }
  else{
    Msg(151).Print(MasterTypeName(_Md->Types[_Md->ShrTypIndex].MstType));
    return false;
  }
  return true;
}

//On-the-fly data conversion to Int (supported only for numeric types)
bool ExprToken::ToInt(){
  if(_Id==ExprTokenId::Operand && AdrMode==CpuAdrMode::LitValue){
    switch(_Md->Types[LitNumTypIndex].MstType){
      case MasterType::Boolean : Value.Int=(CpuInt)Value.Bol; LitNumTypIndex=_Md->IntTypIndex; break;
      case MasterType::Char    : Value.Int=(CpuInt)Value.Chr; LitNumTypIndex=_Md->IntTypIndex; break;
      case MasterType::Short   : Value.Int=(CpuInt)Value.Shr; LitNumTypIndex=_Md->IntTypIndex; break;
      case MasterType::Integer : Value.Int=(CpuInt)Value.Int; LitNumTypIndex=_Md->IntTypIndex; break;
      case MasterType::Long    : Value.Int=(CpuInt)Value.Lon; LitNumTypIndex=_Md->IntTypIndex; break;
      case MasterType::Float   : Value.Int=(CpuInt)Value.Flo; LitNumTypIndex=_Md->IntTypIndex; break;
      case MasterType::String  : 
      case MasterType::Class   :
      case MasterType::Enum    :
      case MasterType::FixArray: 
      case MasterType::DynArray: 
        Msg(150).Print(MasterTypeName(_Md->Types[LitNumTypIndex].MstType),MasterTypeName(MasterType::Integer));
        return false;
        break;
    }
  }
  else{
    Msg(151).Print(MasterTypeName(_Md->Types[_Md->IntTypIndex].MstType));
    return false;
  }
  return true;
}

//On-the-fly data conversion to Lon (supported only for numeric types)
bool ExprToken::ToLon(){
  if(_Id==ExprTokenId::Operand && AdrMode==CpuAdrMode::LitValue){
    switch(_Md->Types[LitNumTypIndex].MstType){
      case MasterType::Boolean : Value.Lon=(CpuLon)Value.Bol; LitNumTypIndex=_Md->LonTypIndex; break;
      case MasterType::Char    : Value.Lon=(CpuLon)Value.Chr; LitNumTypIndex=_Md->LonTypIndex; break;
      case MasterType::Short   : Value.Lon=(CpuLon)Value.Shr; LitNumTypIndex=_Md->LonTypIndex; break;
      case MasterType::Integer : Value.Lon=(CpuLon)Value.Int; LitNumTypIndex=_Md->LonTypIndex; break;
      case MasterType::Long    : Value.Lon=(CpuLon)Value.Lon; LitNumTypIndex=_Md->LonTypIndex; break;
      case MasterType::Float   : Value.Lon=(CpuLon)Value.Flo; LitNumTypIndex=_Md->LonTypIndex; break;
      case MasterType::String  : 
      case MasterType::Class   :
      case MasterType::Enum    :
      case MasterType::FixArray: 
      case MasterType::DynArray: 
        Msg(150).Print(MasterTypeName(_Md->Types[LitNumTypIndex].MstType),MasterTypeName(MasterType::Long));
        return false;
        break;
    }
  }
  else{
    Msg(151).Print(MasterTypeName(_Md->Types[_Md->LonTypIndex].MstType));
    return false;
  }
  return true;
}

//On-the-fly data conversion to Flo (supported only for numeric types)
bool ExprToken::ToFlo(){
  if(_Id==ExprTokenId::Operand && AdrMode==CpuAdrMode::LitValue){
    switch(_Md->Types[LitNumTypIndex].MstType){
      case MasterType::Boolean : Value.Flo=(CpuFlo)Value.Bol; LitNumTypIndex=_Md->FloTypIndex; break;
      case MasterType::Char    : Value.Flo=(CpuFlo)Value.Chr; LitNumTypIndex=_Md->FloTypIndex; break;
      case MasterType::Short   : Value.Flo=(CpuFlo)Value.Shr; LitNumTypIndex=_Md->FloTypIndex; break;
      case MasterType::Integer : Value.Flo=(CpuFlo)Value.Int; LitNumTypIndex=_Md->FloTypIndex; break;
      case MasterType::Long    : Value.Flo=(CpuFlo)Value.Lon; LitNumTypIndex=_Md->FloTypIndex; break;
      case MasterType::Float   : Value.Flo=(CpuFlo)Value.Flo; LitNumTypIndex=_Md->FloTypIndex; break;
      case MasterType::String  : 
      case MasterType::Class   :
      case MasterType::Enum    :
      case MasterType::FixArray: 
      case MasterType::DynArray: 
        Msg(150).Print(MasterTypeName(_Md->Types[LitNumTypIndex].MstType),MasterTypeName(MasterType::Float));
        return false;
        break;
    }
  }
  else{
    Msg(151).Print(MasterTypeName(_Md->Types[_Md->FloTypIndex].MstType));
    return false;
  }
  return true;
}

//On-the-fly data conversion to Word (supported only for numeric types)
bool ExprToken::ToWrd(){
  if(_Id==ExprTokenId::Operand && AdrMode==CpuAdrMode::LitValue){
    switch(_Md->Types[LitNumTypIndex].MstType){
      case MasterType::Boolean : if(GetArchitecture()==32){ Value.Int=(CpuWrd)Value.Bol; LitNumTypIndex=_Md->IntTypIndex; } else{ Value.Lon=(CpuWrd)Value.Bol; LitNumTypIndex=_Md->LonTypIndex; } break;
      case MasterType::Char    : if(GetArchitecture()==32){ Value.Int=(CpuWrd)Value.Chr; LitNumTypIndex=_Md->IntTypIndex; } else{ Value.Lon=(CpuWrd)Value.Chr; LitNumTypIndex=_Md->LonTypIndex; } break;
      case MasterType::Short   : if(GetArchitecture()==32){ Value.Int=(CpuWrd)Value.Shr; LitNumTypIndex=_Md->IntTypIndex; } else{ Value.Lon=(CpuWrd)Value.Shr; LitNumTypIndex=_Md->LonTypIndex; } break;
      case MasterType::Integer : if(GetArchitecture()==32){ Value.Int=(CpuWrd)Value.Int; LitNumTypIndex=_Md->IntTypIndex; } else{ Value.Lon=(CpuWrd)Value.Int; LitNumTypIndex=_Md->LonTypIndex; } break;
      case MasterType::Long    : if(GetArchitecture()==32){ Value.Int=(CpuWrd)Value.Lon; LitNumTypIndex=_Md->IntTypIndex; } else{ Value.Lon=(CpuWrd)Value.Lon; LitNumTypIndex=_Md->LonTypIndex; } break;
      case MasterType::Float   : if(GetArchitecture()==32){ Value.Int=(CpuWrd)Value.Flo; LitNumTypIndex=_Md->IntTypIndex; } else{ Value.Lon=(CpuWrd)Value.Flo; LitNumTypIndex=_Md->LonTypIndex; } break;
      case MasterType::String  : 
      case MasterType::Class   :
      case MasterType::Enum    :
      case MasterType::FixArray: 
      case MasterType::DynArray: 
        Msg(150).Print(MasterTypeName(_Md->Types[LitNumTypIndex].MstType),MasterTypeName(MasterType::Long));
        return false;
        break;
    }
  }
  else{
    Msg(151).Print(MasterTypeName(_Md->Types[_Md->WrdTypIndex].MstType));
    return false;
  }
  return true;
}

//Turn constant variable token into litteral token
bool ExprToken::ToLitValue(bool SendError){
  char *Ptr;
  if(!IsConst){
    if(SendError){ Msg(274).Print(); }
    return false;
  }
  if(_Id==ExprTokenId::Operand){
    if(AdrMode==CpuAdrMode::LitValue){
      switch(_Md->Types[LitNumTypIndex].MstType){
        case MasterType::Boolean : break;
        case MasterType::Char    : break;
        case MasterType::Short   : break;
        case MasterType::Integer : break;
        case MasterType::Long    : break;
        case MasterType::Float   : break;
        case MasterType::String  : 
        case MasterType::Class   :
        case MasterType::Enum    :
        case MasterType::FixArray: 
        case MasterType::DynArray: 
          if(SendError){ Msg(327).Print(MasterTypeName(_Md->Types[LitNumTypIndex].MstType)); }
          return false;
          break;
      }
      return true;
    }
    else if(AdrMode==CpuAdrMode::Address && _Md->Variables[Value.VarIndex].Scope.Kind!=ScopeKind::Local){
      if(!_Md->Variables[Value.VarIndex].IsComputed){
        if(SendError){ Msg(558).Print(); }
        return false;
      }
      switch(_Md->Types[_Md->Variables[Value.VarIndex].TypIndex].MstType){
        case MasterType::Boolean : Ptr=_Md->Bin.GlobValuePointer(_Md->Variables[Value.VarIndex].Address); Value.Bol=*(CpuBol *)Ptr; LitNumTypIndex=_Md->BolTypIndex; break;
        case MasterType::Char    : Ptr=_Md->Bin.GlobValuePointer(_Md->Variables[Value.VarIndex].Address); Value.Chr=*(CpuChr *)Ptr; LitNumTypIndex=_Md->ChrTypIndex; break;
        case MasterType::Short   : Ptr=_Md->Bin.GlobValuePointer(_Md->Variables[Value.VarIndex].Address); Value.Shr=*(CpuShr *)Ptr; LitNumTypIndex=_Md->ShrTypIndex; break;
        case MasterType::Integer : Ptr=_Md->Bin.GlobValuePointer(_Md->Variables[Value.VarIndex].Address); Value.Int=*(CpuInt *)Ptr; LitNumTypIndex=_Md->IntTypIndex; break;
        case MasterType::Long    : Ptr=_Md->Bin.GlobValuePointer(_Md->Variables[Value.VarIndex].Address); Value.Lon=*(CpuLon *)Ptr; LitNumTypIndex=_Md->LonTypIndex; break;
        case MasterType::Float   : Ptr=_Md->Bin.GlobValuePointer(_Md->Variables[Value.VarIndex].Address); Value.Flo=*(CpuFlo *)Ptr; LitNumTypIndex=_Md->FloTypIndex; break;
        case MasterType::String  : 
        case MasterType::Class   :
        case MasterType::Enum    :
        case MasterType::FixArray: 
        case MasterType::DynArray: 
          if(SendError){ Msg(273).Print(MasterTypeName(_Md->Types[_Md->Variables[Value.VarIndex].TypIndex].MstType)); }
          return false;
          break;
      }
      AdrMode=CpuAdrMode::LitValue;
      IsConst=true;
      return true;
    }
    else{
      if(SendError){ Msg(274).Print(); }
      return false;
    }
  }
  else{
    if(SendError){ Msg(274).Print(); }
    return false;
  }
}

//Check expresion token is computable
bool ExprToken::IsComputableOperand(){
  return ToLitValue(false);
}

//Check operator is computable
bool ExprToken::IsComputableOperator(){

  //Variables
  bool IsComputable=false;

  //Not computable if not an operator
  if(_Id!=ExprTokenId::Operator){ return false; }

  //Check operator
  switch(Value.Operator){
    
    //Numeric operators
    case ExprOperator::UnaryPlus:      
    case ExprOperator::UnaryMinus:     
    case ExprOperator::Multiplication: 
    case ExprOperator::Division:       
    case ExprOperator::Addition:       
    case ExprOperator::Substraction:   
    case ExprOperator::Less:           
    case ExprOperator::LessEqual:      
    case ExprOperator::Greater:        
    case ExprOperator::GreaterEqual:   
    case ExprOperator::Equal:          
    case ExprOperator::Distinct:       
    case ExprOperator::BitwiseNot:     
    case ExprOperator::Modulus:        
    case ExprOperator::ShiftLeft:      
    case ExprOperator::ShiftRight:     
    case ExprOperator::BitwiseAnd:     
    case ExprOperator::BitwiseXor:     
    case ExprOperator::BitwiseOr:      
    case ExprOperator::LogicalNot:     
    case ExprOperator::LogicalAnd:     
    case ExprOperator::LogicalOr:      
      IsComputable=true;
      break;

    //Type cast
    case ExprOperator::TypeCast:
      switch(_Md->Types[CastTypIndex].MstType){
        case MasterType::Boolean : 
        case MasterType::Char    : 
        case MasterType::Short   : 
        case MasterType::Integer : 
        case MasterType::Long    : 
        case MasterType::Float   : 
          IsComputable=true;
          break;
        case MasterType::String  : 
        case MasterType::Class   : 
        case MasterType::Enum    : 
        case MasterType::FixArray:
        case MasterType::DynArray:
          IsComputable=false;
          break;
      }
      break;

    //Other operators not supported
    case ExprOperator::PostfixInc:
    case ExprOperator::PostfixDec:
    case ExprOperator::PrefixInc :
    case ExprOperator::PrefixDec :
    case ExprOperator::Initializ :
    case ExprOperator::Assign    :
    case ExprOperator::AddAssign :
    case ExprOperator::SubAssign :
    case ExprOperator::MulAssign :
    case ExprOperator::DivAssign :
    case ExprOperator::ModAssign :
    case ExprOperator::ShlAssign :
    case ExprOperator::ShrAssign :
    case ExprOperator::AndAssign :
    case ExprOperator::XorAssign :
    case ExprOperator::OrAssign  :
    case ExprOperator::SeqOper  :
      IsComputable=false;
      break;
    
  }

  //Return code
  return IsComputable;

}

//Copy token value to memory address
bool ExprToken::CopyValue(char *Ptr){
  if(_Id==ExprTokenId::Operand && AdrMode==CpuAdrMode::LitValue){
    switch(_Md->Types[LitNumTypIndex].MstType){
      case MasterType::Boolean : *(CpuBol *)Ptr=Value.Bol; break;
      case MasterType::Char    : *(CpuChr *)Ptr=Value.Chr; break;
      case MasterType::Short   : *(CpuShr *)Ptr=Value.Shr; break;
      case MasterType::Integer : *(CpuInt *)Ptr=Value.Int; break;
      case MasterType::Long    : *(CpuLon *)Ptr=Value.Lon; break;
      case MasterType::Float   : *(CpuFlo *)Ptr=Value.Flo; break;
      case MasterType::String  : 
      case MasterType::Class   :
      case MasterType::Enum    :
      case MasterType::FixArray: 
      case MasterType::DynArray: 
        Msg(148).Print(MasterTypeName(_Md->Types[LitNumTypIndex].MstType));
        return false;
        break;
    }
    return true;
  }
  else{
    Msg(276).Print();
    return false;
  }
}

//Release operand
void ExprToken::Release(){
  if(_Id==ExprTokenId::Operand && (AdrMode==CpuAdrMode::Address || AdrMode==CpuAdrMode::Indirection)){
    if(_Md->Variables[Value.VarIndex].IsTempVar){ _Md->TempVarUnlock(Value.VarIndex); }
  }
}

//Lock operand
void ExprToken::Lock(){
  if(_Id==ExprTokenId::Operand && (AdrMode==CpuAdrMode::Address || AdrMode==CpuAdrMode::Indirection)){
    if(_Md->Variables[Value.VarIndex].IsTempVar){ _Md->TempVarLock(Value.VarIndex); }
  }
}

//Set variable as used in source
void ExprToken::SetSourceUsed(const ScopeDef& Scope,bool Forced){
  bool SetUsed=false;
  if(AdrMode==CpuAdrMode::Address || AdrMode==CpuAdrMode::Indirection){
    if(_Md->Variables[Value.VarIndex].IsParameter && _Md->Variables[Value.VarIndex].IsReference && !_Md->Variables[Value.VarIndex].IsConst){
      SetUsed=true;
    }
    else if(_Md->Variables[Value.VarIndex].Scope!=Scope){
      SetUsed=true;
    }
    else if(Forced){
      SetUsed=true;
    }
  }
  else if(SourceVarIndex!=-1){
    SetUsed=true;
  }
  if(SetUsed){
    int VarIndex=(SourceVarIndex!=-1?SourceVarIndex:Value.VarIndex);
    _Md->Variables[VarIndex].IsSourceUsed=true;
    DebugMessage(DebugLevel::CmpExpression,"Source used flag set on variable "+_Md->Variables[VarIndex].Name+" in scope "+_Md->ScopeName(_Md->Variables[VarIndex].Scope));
  }

}

//Set master data pointer inexpression token
void ExprToken::SetMdPointer(MasterData *Md){
  _Md=Md;
}

//Set position
void ExprToken::SetPosition(const SourceInfo& SrcInfo){
  _FileName=SrcInfo.FileName;
  _LineNr=SrcInfo.LineNr;
  _ColNr=SrcInfo.ColNr;
}

//Get lvalue-ness of token
bool ExprToken::IsLValue() const {
  if(_Id==ExprTokenId::Operand){
    if(AdrMode==CpuAdrMode::Address){
      if(_Md->Variables[Value.VarIndex].IsTempVar){
        return false;
      }
      else{
        return true;
      }
    }
    else if(AdrMode==CpuAdrMode::Indirection){
      return true;  
    }
    else{
      return false;
    }
  }
  else{
    return false;
  }
}

//Get if operand is initialized
bool ExprToken::IsInitialized() const {
  bool Initialized;
  if(_Id==ExprTokenId::Operand){
    switch(AdrMode){
      case CpuAdrMode::Address: 
      case CpuAdrMode::Indirection:
        if(_Md->Variables[Value.VarIndex].IsInitialized){
          Initialized=true;
        }
        else if(SourceVarIndex!=-1 && _Md->Variables[SourceVarIndex].IsInitialized){
          Initialized=true;
        }
        else{
          Initialized=false;
        }
        break;
      case CpuAdrMode::LitValue: 
        Initialized=true;
        break;
    }
  }
  else{
    Initialized=false;
  }
  return Initialized;
}

//Check operand is function result
bool ExprToken::IsFuncResult() const {
  if(_Id==ExprTokenId::Operand && AdrMode==CpuAdrMode::Indirection && _Md->Variables[Value.VarIndex].Name==_Md->GetFuncResultName()){
    return true;
  }
  else{
    return false;
  }
}

//Get type index for token
int ExprToken::TypIndex() const{
  if(AdrMode==CpuAdrMode::LitValue){
    return LitNumTypIndex;
  }
  else{
    return _Md->Variables[Value.VarIndex].TypIndex;
  }
}

//Get variable index for token
int ExprToken::VarIndex() const{
  if(AdrMode==CpuAdrMode::LitValue){
    return -1;
  }
  else{
    return Value.VarIndex;
  }
}

//Get master type for token
MasterType ExprToken::MstType() const{
  if(AdrMode==CpuAdrMode::LitValue){
    return _Md->Types[LitNumTypIndex].MstType;
  }
  else{
    return _Md->Types[_Md->Variables[Value.VarIndex].TypIndex].MstType;
  }
}

//Checkmaster type is atomic (bool,integers,float and string)
bool ExprToken::IsMasterAtomic() const{
  if(AdrMode==CpuAdrMode::LitValue){
    return _Md->IsMasterAtomic(LitNumTypIndex);
  }
  else{
    return _Md->IsMasterAtomic(_Md->Variables[Value.VarIndex].TypIndex);
  }
}

//Expression token conversion to assembler argument
//KeepReferences: Needs to be passed as true when argument is already a reference but we do not want to make any indirection
AsmArg ExprToken::Asm(bool KeepReferences) const {

  //Variables
  AsmArg Arg;
  
  //Error if argument is not an operand
  if(_Id!=ExprTokenId::Operand){
    Msg(59).Delay(ExprTokenIdText(_Id));
    return _Md->AsmErr();
  }

  //Switch on addressing mode
  switch(AdrMode){

    //Litteral values
    case CpuAdrMode::LitValue:    
      if((int)Meta.Case!=0){
        switch(Meta.Case){
          case MetaConstCase::VarName:  Arg=_Md->AsmMta(Meta.Case,Meta.VarIndex); break;
          case MetaConstCase::TypName:  Arg=_Md->AsmMta(Meta.Case,Meta.TypIndex); break;
          case MetaConstCase::FldNames: Arg=_Md->AsmMta(Meta.Case,Meta.TypIndex); break;
          case MetaConstCase::FldTypes: Arg=_Md->AsmMta(Meta.Case,Meta.TypIndex); break;
        }
      }
      else{
        switch(_Md->Types[LitNumTypIndex].MstType){
          case MasterType::Boolean : Arg=_Md->Bin.AsmLitBol(Value.Bol); break;
          case MasterType::Char    : Arg=_Md->Bin.AsmLitChr(Value.Chr); break;
          case MasterType::Short   : Arg=_Md->Bin.AsmLitShr(Value.Shr); break;
          case MasterType::Integer : Arg=_Md->Bin.AsmLitInt(Value.Int); break;
          case MasterType::Long    : Arg=_Md->Bin.AsmLitLon(Value.Lon); break;
          case MasterType::Float   : Arg=_Md->Bin.AsmLitFlo(Value.Flo); break;
          case MasterType::Enum    : Arg=_Md->Bin.AsmLitInt(Value.Enu); break;
          case MasterType::String  : Arg=_Md->Bin.AsmLitStr(Value.Adr); break;
          case MasterType::Class   :
          case MasterType::FixArray:
          case MasterType::DynArray: 
            Msg(58).Delay();
            Arg=_Md->AsmErr();
            break;
        }
      }
      break;
    
    //Addresses
    case CpuAdrMode::Address:     
      Arg=_Md->AsmVar(Value.VarIndex); 
      break;

    //Indirections
    case CpuAdrMode::Indirection: 
      if(KeepReferences){
        Arg=_Md->AsmVar(Value.VarIndex); 
      }
      else{
        Arg=_Md->AsmInd(Value.VarIndex); 
      }
      break;

  }

  //Return value
  return Arg;

}

//Expression token conversion to assembler argument
String ExprToken::Print() const {
  String Output;
  switch(_Id){
    case ExprTokenId::Operator:    
      switch(Value.Operator){
        case ExprOperator::TypeCast:
          Output="opr("+_Md->Types[CastTypIndex].Name.Trim()+")";
          break;
        default:
         Output="opr"+_Opr[(int)Value.Operator].Name.Trim(); 
          break;
      }
      break;
    case ExprTokenId::LowLevelOpr: 
      switch(Value.LowLevelOpr){
        case ExprLowLevelOpr::TernaryCond: Output="low(?,"+ToString(LabelSeed)+")"; break;
        case ExprLowLevelOpr::TernaryMid : Output="low(:,"+ToString(LabelSeed)+")"; break;
        case ExprLowLevelOpr::TernaryEnd : Output="low(end,"+ToString(LabelSeed)+")"; break;
      }
      break;
    case ExprTokenId::FlowOpr: 
      switch(Value.FlowOpr){
        case ExprFlowOpr::ForBeg  : Output="flow(for.beg,"+ToString(FlowLabel)+")"; break;
        case ExprFlowOpr::ForIf   : Output="flow(for.if,"+ToString(FlowLabel)+")"; break;
        case ExprFlowOpr::ForDo   : Output="flow(for.do,"+ToString(FlowLabel)+")"; break;
        case ExprFlowOpr::ForRet  : Output="flow(for.ret,"+ToString(FlowLabel)+")"; break;
        case ExprFlowOpr::ForEnd  : Output="flow(for.end,"+ToString(FlowLabel)+")"; break;
        case ExprFlowOpr::ArrBeg  : Output="flow(arr.beg,"+ToString(FlowLabel)+")"; break;
        case ExprFlowOpr::ArrOnvar: Output="flow(arr.onvar,"+ToString(FlowLabel)+")"; break;
        case ExprFlowOpr::ArrIxvar: Output="flow(arr.ixvar,"+ToString(FlowLabel)+")"; break;
        case ExprFlowOpr::ArrInit : Output="flow(arr.init,"+ToString(FlowLabel)+")"; break;
        case ExprFlowOpr::ArrAsif : Output="flow(arr.asif,"+ToString(FlowLabel)+")"; break;
        case ExprFlowOpr::ArrEnd  : Output="flow(arr.end,"+ToString(FlowLabel)+")"; break;
      }
      break;
    case ExprTokenId::Subscript:   
      Output="["+ToString(Value.DimNr)+"]"; 
      break;
    case ExprTokenId::Function:    
      Output="func("+_Md->Modules[FunModIndex].Name+"."+Value.Function+","+ToString(CallParmNr)+")"; 
      break;
    case ExprTokenId::Delimiter:   
      switch(Value.Delimiter){
        case ExprDelimiter::BegParen  : Output="("; break;
        case ExprDelimiter::EndParen  : Output=")"; break;
        case ExprDelimiter::BegBracket: Output="["; break;
        case ExprDelimiter::EndBracket: Output="]"; break;
        case ExprDelimiter::BegCurly  : Output="{"; break;
        case ExprDelimiter::EndCurly  : Output="}"; break;
        case ExprDelimiter::Comma     : Output=","; break;
      }
      break;
    case ExprTokenId::Field:       
      Output="."+Value.Field; 
      break;
    case ExprTokenId::Method:      
      Output="."+Value.Method+"("+ToString(CallParmNr)+")"; 
      break;
    case ExprTokenId::Constructor:      
      Output=_Md->CannonicalTypeName(Value.CCTypIndex)+"."+_Md->Types[Value.CCTypIndex].Name+"("+ToString(CallParmNr)+")"; 
      break;
    case ExprTokenId::Complex:      
      Output="Cpx:"+_Md->CannonicalTypeName(Value.ComplexTypIndex); 
      if(_Md->Types[Value.ComplexTypIndex].MstType==MasterType::DynArray){
        Output+=":[";
        for(int i=0;i<_Md->Types[Value.ComplexTypIndex].DimNr;i++){
          Output+=(i==0?"":",")+ToString(DimSize.n[i]);
        }
        Output+="]";
      }
      break;
    case ExprTokenId::UndefVar:       
      Output="undef("+Value.VarName+")"; 
      break;
    case ExprTokenId::VoidRes:       
      Output="void("+Value.VoidFuncName+")"; 
      break;
    case ExprTokenId::Operand:    
      switch(AdrMode){
        case CpuAdrMode::LitValue:
          if((int)Meta.Case!=0){
            switch(Meta.Case){
              case MetaConstCase::VarName:  Output="Meta(VN:"+_Md->Variables[Meta.VarIndex].Name+")"; break;
              case MetaConstCase::TypName:  Output="Meta(TN:"+_Md->Types[Meta.TypIndex].Name+")"; break;
              case MetaConstCase::FldNames: Output="Meta(FN:"+_Md->Types[Meta.TypIndex].Name+")"; break;
              case MetaConstCase::FldTypes: Output="Meta(FT:"+_Md->Types[Meta.TypIndex].Name+")"; break;
            }
          }
          else{
            switch(_Md->Types[LitNumTypIndex].MstType){
              case MasterType::Boolean : Output=ToString(Value.Bol)+"B"; break;
              case MasterType::Char    : Output=ToString(Value.Chr)+"C"; break;
              case MasterType::Short   : Output=ToString(Value.Shr)+"S"; break;
              case MasterType::Integer : Output=ToString(Value.Int)+"I"; break;
              case MasterType::Long    : Output=ToString(Value.Lon)+"L"; break;
              case MasterType::Float   : Output=ToString(Value.Flo)+"F"; break;
              case MasterType::Enum    : Output="Enu:"+_Md->Types[LitNumTypIndex].Name+":"+ToString(Value.Enu); break;
              case MasterType::String  : Output="Str:"+ToString(Value.Adr); break;
              case MasterType::DynArray: Output="Ard:"+ToString(Value.Adr); break;
              case MasterType::FixArray: Output="Arf:error"; break;
              case MasterType::Class   : Output="Cla:error"; break;
            }
          }
          break;
        case CpuAdrMode::Address:
          Output="var:("+_Md->CannonicalTypeName(_Md->Variables[Value.VarIndex].TypIndex)+")"+_Md->Variables[Value.VarIndex].Name;
          break;
        case CpuAdrMode::Indirection:
          Output="ind:("+_Md->CannonicalTypeName(_Md->Variables[Value.VarIndex].TypIndex)+")"+_Md->Variables[Value.VarIndex].Name;
          break;
      }
      break;
  }
  if(_Id==(ExprTokenId)-1){ Output="init"; }
  Output=(IsConst?"{K}":"")+Output;
  Output+=(SourceVarIndex!=-1?"{src="+_Md->Variables[SourceVarIndex].Name+"}":"");
  Output+=(IsInitialized()?"*":"");
  Output+=(HasInitialization?"{HasInit}":"");
  return Output;
}

//Expression token message
SysMessage ExprToken::Msg(int Code) const {
  return SysMessage(Code,_FileName,_LineNr,_ColNr);
}

//Get operand readable name
String ExprToken::Name() const{
  String Name;
  if(_Id==ExprTokenId::Operand){
    switch(AdrMode){
      case CpuAdrMode::Address: 
        Name=_Md->Variables[Value.VarIndex].Name;
        break;
      case CpuAdrMode::Indirection:
        if(SourceVarIndex!=-1){
          Name=_Md->Variables[SourceVarIndex].Name;
        }
        else{
          Name=_Md->Variables[Value.VarIndex].Name;
        }
        break;
      case CpuAdrMode::LitValue: 
        Name=Print();
        break;
    }
  }
  else{
    Name=Print();
  }
  return Name;
}

//Get file name
String ExprToken::FileName() const{
  return _FileName;
}

//Get line number
int ExprToken::LineNr() const {
  return _LineNr;
} 

//Get column number   
int ExprToken::ColNr() const {
  return _ColNr;
}     

//Source information
SourceInfo ExprToken::SrcInfo() const {
  return SourceInfo(_FileName,_LineNr,_ColNr);
}

//Expression tokenconstructor
ExprToken::ExprToken(){
  _Reset(false);
}

//Expression token destructor
ExprToken::~ExprToken(){
  _DestroyId();
}

//Expression token copy
ExprToken::ExprToken(const ExprToken& Token){
  _Reset(false);
  _Move(Token);
}

//Expression token assignment
ExprToken& ExprToken::operator=(const ExprToken& Token){
  _Move(Token);
  return *this;
}

//Expression copy
Expression::Expression(const Expression& Expr){
  *this=Expr;
}

//Expression assignment
Expression& Expression::operator=(const Expression& Expr){
  _Md=Expr._Md;
  _FileName=Expr._FileName;
  _LineNr=Expr._LineNr;
  _Tokens=Expr._Tokens;
  _Origin=Expr._Origin;
  return *this;
}

//Check token has operand on the right
bool Expression::_HasOperandOnRight(int Index) const {

  //No operand if index is last token
  if(Index==_Tokens.Length()-1){ return false; }

  //Left token is an operand in any if it is any of these:
  //Prefix unary operator (++ --), Openning parenthesys, Openning bracket, Function, Constructor, Complex value, Undefined variable or Operand
  if((_Tokens[Index+1].Id()==ExprTokenId::Operator 
  && _Opr[(int)_Tokens[Index+1].Value.Operator].Assoc==ExprOpAssoc::Right 
  && _Opr[(int)_Tokens[Index+1].Value.Operator].Class==ExprOpClass::Unary)
  ||(_Tokens[Index+1].Id()==ExprTokenId::Delimiter && _Tokens[Index+1].Value.Delimiter==ExprDelimiter::BegParen)
  ||(_Tokens[Index+1].Id()==ExprTokenId::Delimiter && _Tokens[Index+1].Value.Delimiter==ExprDelimiter::BegBracket)
  ||(_Tokens[Index+1].Id()==ExprTokenId::Function)
  ||(_Tokens[Index+1].Id()==ExprTokenId::Constructor)
  ||(_Tokens[Index+1].Id()==ExprTokenId::Complex)
  ||(_Tokens[Index+1].Id()==ExprTokenId::UndefVar)
  ||(_Tokens[Index+1].Id()==ExprTokenId::Operand)){
    return true;
  }

  //Return not found
  return false;

}

//Check token has operand on the left
bool Expression::_HasOperandOnLeft(int Index) const {

  //No operand if index is first token
  if(Index==0){ return false; }

  //Left token is an operand in any if it is any of these:
  //Postfix unary operator (++ --), Closing parenthesys, Closing bracket, Closing curly brace, Member or just operand
  if((_Tokens[Index-1].Id()==ExprTokenId::Operator 
  && _Opr[(int)_Tokens[Index-1].Value.Operator].Assoc==ExprOpAssoc::Left 
  && _Opr[(int)_Tokens[Index-1].Value.Operator].Class==ExprOpClass::Unary)
  ||(_Tokens[Index-1].Id()==ExprTokenId::Delimiter && _Tokens[Index-1].Value.Delimiter==ExprDelimiter::EndParen)
  ||(_Tokens[Index-1].Id()==ExprTokenId::Delimiter && _Tokens[Index-1].Value.Delimiter==ExprDelimiter::EndBracket)
  ||(_Tokens[Index-1].Id()==ExprTokenId::Delimiter && _Tokens[Index-1].Value.Delimiter==ExprDelimiter::EndCurly)
  ||(_Tokens[Index-1].Id()==ExprTokenId::Field)
  ||(_Tokens[Index-1].Id()==ExprTokenId::UndefVar)
  ||(_Tokens[Index-1].Id()==ExprTokenId::Operand)){
    return true;
  }

  //Return not found
  return false;

}

//Check last is an operand
bool Expression::_IsLastTokenOperand() const {
  return _HasOperandOnLeft(_Tokens.Length());
}

//Check two expression tokens reffer to the same operand (only for lvalues without idirection)
bool Expression::_SameOperand(ExprToken Opnd1,ExprToken Opnd2) const {
  if(Opnd1.Id()==ExprTokenId::Operand && Opnd2.Id()==ExprTokenId::Operand
  && Opnd1.AdrMode==CpuAdrMode::Address && Opnd2.AdrMode==CpuAdrMode::Address 
  && Opnd1.Value.VarIndex==Opnd2.Value.VarIndex){
      return true;
  }
  else{
    return false;
  }
}

//Automatic data type promotion
bool Expression::_IsDataTypePromotionAutomatic(MasterType FrType,MasterType ToType) const {
  if((FrType==MasterType::Char    && ToType==MasterType::Integer)
  || (FrType==MasterType::Char    && ToType==MasterType::Short  )
  || (FrType==MasterType::Char    && ToType==MasterType::Long   )
  || (FrType==MasterType::Char    && ToType==MasterType::Float  )
  || (FrType==MasterType::Short   && ToType==MasterType::Integer)
  || (FrType==MasterType::Short   && ToType==MasterType::Long   )
  || (FrType==MasterType::Short   && ToType==MasterType::Float  )
  || (FrType==MasterType::Integer && ToType==MasterType::Long   )
  || (FrType==MasterType::Integer && ToType==MasterType::Float  )
  || (FrType==MasterType::Long    && ToType==MasterType::Float  )
  || (FrType==MasterType::Char    && ToType==MasterType::String )){
    return true;
  }
  else{
    return false;
  }
}

//Count parameters in function/method call
bool Expression::_CountParameters(const Sentence& Stn,int StartToken,int& ParmCount) const {

  //Variables
  int i;
  int ParmNr;
  int ParLevel;
  int BraLevel;
  int ClyLevel;
  ExprToken Token;

  //Count number of parameters in function/method call
  ParmNr=0;
  ParLevel=-1;
  BraLevel=0;
  ClyLevel=0;
  for(i=StartToken;i<Stn.Tokens.Length();i++){
    if(Stn.Tokens[i].Id()==PrTokenId::Punctuator){
      if(Stn.Tokens[i].Value.Pnc==PrPunctuator::BegParen  ){ ParLevel++; }
      if(Stn.Tokens[i].Value.Pnc==PrPunctuator::BegBracket){ BraLevel++; }
      if(Stn.Tokens[i].Value.Pnc==PrPunctuator::BegCurly  ){ ClyLevel++; }
    }
    if(Stn.Tokens[i].Id()==PrTokenId::Punctuator && Stn.Tokens[i].Value.Pnc==PrPunctuator::Comma && ParLevel==0 && BraLevel==0 && ClyLevel==0){
      if(i-1>=StartToken && Stn.Tokens[i-1].Id()==PrTokenId::Punctuator && (Stn.Tokens[i-1].Value.Pnc==PrPunctuator::BegParen || Stn.Tokens[i-1].Value.Pnc==PrPunctuator::Comma)){
        Stn.Tokens[i].Msg(90).Print();
        return false;
      }
      ParmNr++;
    }
    else if(Stn.Tokens[i].Id()==PrTokenId::Punctuator && Stn.Tokens[i].Value.Pnc==PrPunctuator::EndParen && ParLevel==0 && BraLevel==0 && ClyLevel==0){
      if(i-1>=StartToken && Stn.Tokens[i-1].Id()==PrTokenId::Punctuator && Stn.Tokens[i-1].Value.Pnc==PrPunctuator::BegParen){
        ParmNr=0;
      }
      else if(i-1>=StartToken && Stn.Tokens[i-1].Id()==PrTokenId::Punctuator && Stn.Tokens[i-1].Value.Pnc==PrPunctuator::Comma){
        Stn.Tokens[i].Msg(90).Print();
        return false;
      }
      else{
        ParmNr++; 
      }
      break;
    }
    if(Stn.Tokens[i].Id()==PrTokenId::Punctuator){
      if(Stn.Tokens[i].Value.Pnc==PrPunctuator::EndParen  ){ ParLevel--; }
      if(Stn.Tokens[i].Value.Pnc==PrPunctuator::EndBracket){ BraLevel--; }
      if(Stn.Tokens[i].Value.Pnc==PrPunctuator::EndCurly  ){ ClyLevel--; }
    }
  }

  //Return parameter count
  ParmCount=ParmNr;
  return true;

}

//Tokenize complex litteral value and get extended attributes
bool Expression::_ComplexLitValueTokenize(const Sentence& Stn,int TypIndex,int BegToken,Array<ComplexAttr>& CmplxAttr,int& ReadTokens,int RecurLevel) const {

  //Variables
  int i,j;
  int ParLevel;
  int BraLevel;
  int ClyLevel;
  int FldIndex;
  int BraceLevel;
  int ElementNr;
  int CurrSize[_MaxArrayDims];
  bool DimNrSet;
  bool DimSizeSet[_MaxArrayDims];
  int DimNr;           
  ArrayIndexes DimSize;
  int EndToken;
  int ProcTokens;

  //Debug message
  DebugMessage(DebugLevel::CmpComplexLit,String(2*RecurLevel,' ')+"Beg complex tokenizer: token="+ToString(BegToken)+":"+Stn.Tokens[BegToken].Print()+" type="+_Md->CannonicalTypeName(TypIndex));

  //Init variables
  DimNr=0;
  DimNrSet=false;
  BraceLevel=-1;
  ElementNr=0;
  EndToken=-1;
  for(i=0;i<_MaxArrayDims;i++){ CurrSize[i]=0; DimSize.n[i]=0; DimSizeSet[i]=false; }
  if(RecurLevel==0){
    CmplxAttr.Resize(Stn.Tokens.Length());
    for(i=0;i<CmplxAttr.Length();i++){ CmplxAttr[i].Used=false; }
  }
  ReadTokens=0;
  ProcTokens=0;
  
  //Token processing loop
  i=BegToken;
  while(i<Stn.Tokens.Length()){
    
    //Openning curly brace
    if(Stn.Tokens[i].Id()==PrTokenId::Punctuator && Stn.Tokens[i].Value.Pnc==PrPunctuator::BegCurly){
      
      //Init processed tokens
      ProcTokens=0;

      //Opening curly brace on array
      if(_Md->Types[TypIndex].MstType==MasterType::FixArray
      || _Md->Types[TypIndex].MstType==MasterType::DynArray){
        if(BraceLevel+1<_Md->Types[TypIndex].DimNr){
          BraceLevel++;
          if(!DimNrSet){ DimNr++; }
        }
        else if(_Md->Types[_Md->Types[TypIndex].ElemTypIndex].MstType==MasterType::FixArray
        ||      _Md->Types[_Md->Types[TypIndex].ElemTypIndex].MstType==MasterType::DynArray
        ||      _Md->Types[_Md->Types[TypIndex].ElemTypIndex].MstType==MasterType::Class){
          if(!_ComplexLitValueTokenize(Stn,_Md->Types[TypIndex].ElemTypIndex,i,CmplxAttr,ProcTokens,RecurLevel+1)){ return false; }
        }
        else{
          Stn.Tokens[i].Msg(413).Print();
          return false;
        }
      }

      //Opening curly brace on class
      else if(_Md->Types[TypIndex].MstType==MasterType::Class){
        BraceLevel++;
        if(BraceLevel==0){
          FldIndex=_Md->FieldLoop(TypIndex,-1);
          ElementNr=1;
        }
        else if(_Md->Types[_Md->Fields[FldIndex].TypIndex].MstType==MasterType::FixArray
        ||      _Md->Types[_Md->Fields[FldIndex].TypIndex].MstType==MasterType::DynArray
        ||      _Md->Types[_Md->Fields[FldIndex].TypIndex].MstType==MasterType::Class){
          if(!_ComplexLitValueTokenize(Stn,_Md->Fields[FldIndex].TypIndex,i,CmplxAttr,ProcTokens,RecurLevel+1)){ return false; }
        }
        else{
          Stn.Tokens[i].Msg(414).Print();
          return false;
        }
      }

      //Set attributes (we set here all curly as inner, at end we set outer braces)
      if(!CmplxAttr[i].Used){
        CmplxAttr[i].Used=true;
        CmplxAttr[i].TypIndex=-1;
        CmplxAttr[i].DimSize=ZERO_INDEXES;
        CmplxAttr[i].CurlyClass=CurlyBraceClass::Inner;
      }

      //Jump processed tokens by recursive call
      i+=ProcTokens;

    }

    //Comma
    else if(Stn.Tokens[i].Id()==PrTokenId::Punctuator && Stn.Tokens[i].Value.Pnc==PrPunctuator::Comma){
      if(_Md->Types[TypIndex].MstType==MasterType::FixArray
      || _Md->Types[TypIndex].MstType==MasterType::DynArray){
        if(!DimSizeSet[BraceLevel]){ DimSize.n[BraceLevel]++; }
        CurrSize[BraceLevel]++;
        if(DimSize.n[BraceLevel]<CurrSize[BraceLevel]){
          Stn.Tokens[i].Msg(411).Print(ToString(BraceLevel+1),ToString(DimSize.n[BraceLevel]));
          return false;
        }
      }
      else if(_Md->Types[TypIndex].MstType==MasterType::Class){
        FldIndex=_Md->FieldLoop(TypIndex,FldIndex);
        if(FldIndex!=-1){
          ElementNr++;
        }
        else{
          Stn.Tokens[i].Msg(416).Print(_Md->CannonicalTypeName(TypIndex),ToString(_Md->FieldCount(TypIndex)));
          return false;
        }
      }
    }

    //Closing curly brace
    else if(Stn.Tokens[i].Id()==PrTokenId::Punctuator && Stn.Tokens[i].Value.Pnc==PrPunctuator::EndCurly){
      
      //Clossing curly brace on array
      if(_Md->Types[TypIndex].MstType==MasterType::FixArray
      || _Md->Types[TypIndex].MstType==MasterType::DynArray){
        if(i-1<BegToken || Stn.Tokens[i-1].Id()!=PrTokenId::Punctuator || Stn.Tokens[i-1].Value.Pnc!=PrPunctuator::BegCurly){
          if(!DimSizeSet[BraceLevel]){ DimSize.n[BraceLevel]++; }
          CurrSize[BraceLevel]++;
        }
        if(DimSize.n[BraceLevel]!=CurrSize[BraceLevel]){
          Stn.Tokens[i].Msg(415).Print(ToString(BraceLevel+1),ToString(DimSize.n[BraceLevel]));
          return false;
        }
        DimNrSet=true;
        DimSizeSet[BraceLevel]=true; 
        CurrSize[BraceLevel]=0;
        BraceLevel--;
        if(BraceLevel<0){ EndToken=i; }
      }

      //Clossing curly brace on class
      else if(_Md->Types[TypIndex].MstType==MasterType::Class){
        EndToken=i;
      }

      //Set attributes (we set here all curly as inner, at end we set outer braces)
      if(!CmplxAttr[i].Used){
        CmplxAttr[i].Used=true;
        CmplxAttr[i].TypIndex=-1;
        CmplxAttr[i].DimSize=ZERO_INDEXES;
        CmplxAttr[i].CurlyClass=CurlyBraceClass::Inner;
      }

      //Finish loop if end token is detected
      if(EndToken!=-1){ break; }

    }

    //Others
    else{

      //Jump to next punctuator at level zero
      ParLevel=0;
      BraLevel=0;
      ClyLevel=0;
      for(j=i;j<Stn.Tokens.Length();j++){
        if(Stn.Tokens[j].Id()==PrTokenId::Punctuator){
          if(Stn.Tokens[j].Value.Pnc==PrPunctuator::BegCurly){
            if(ParLevel==0 && BraLevel==0 && ClyLevel==0){ i=j-1; break; } 
            ClyLevel++;
          }
          else if(Stn.Tokens[j].Value.Pnc==PrPunctuator::BegParen){
            ParLevel++;
          }
          else if(Stn.Tokens[j].Value.Pnc==PrPunctuator::EndParen){
            ParLevel--;
          }
          else if(Stn.Tokens[j].Value.Pnc==PrPunctuator::BegBracket){
            BraLevel++;
          }
          else if(Stn.Tokens[j].Value.Pnc==PrPunctuator::EndBracket){
            BraLevel--;
          }
          else if(Stn.Tokens[j].Value.Pnc==PrPunctuator::Comma){
            if(ParLevel==0 && BraLevel==0 && ClyLevel==0){ i=j-1; break; }
          }
          else if(Stn.Tokens[j].Value.Pnc==PrPunctuator::EndCurly){
            if(ParLevel==0 && BraLevel==0 && ClyLevel==0){ i=j-1; break; }
            ClyLevel--;
          }
        }
      }

      //Detect missing closing punctuator
      if(ParLevel>0){ Stn.Tokens.Last().Msg(405).Print(); return false; }
      if(ParLevel<0){ Stn.Tokens.Last().Msg(406).Print(); return false; }
      if(BraLevel>0){ Stn.Tokens.Last().Msg(407).Print(); return false; }
      if(BraLevel<0){ Stn.Tokens.Last().Msg(408).Print(); return false; }
      if(ClyLevel>0){ Stn.Tokens.Last().Msg(409).Print(); return false; }
      if(ClyLevel<0){ Stn.Tokens.Last().Msg(410).Print(); return false; }

    }

    //Next token
    i++;

  }

  //Check end token was reached
  if(EndToken==-1){
    Stn.Tokens[BegToken].Msg(417).Print();
    return false;
  }

  //Final checks
  if(_Md->Types[TypIndex].MstType==MasterType::FixArray
  || _Md->Types[TypIndex].MstType==MasterType::DynArray){
    if(DimNr!=_Md->Types[TypIndex].DimNr){
      Stn.Tokens[BegToken].Msg(418).Print(ToString(DimNr),_Md->CannonicalTypeName(TypIndex),ToString(_Md->Types[TypIndex].DimNr));
      return false;
    }
    if(_Md->Types[TypIndex].MstType==MasterType::FixArray){
      for(i=0;i<_MaxArrayDims;i++){
        if(_Md->Dimensions[_Md->Types[TypIndex].DimIndex].DimSize.n[i]!=DimSize.n[i]){
          Stn.Tokens[BegToken].Msg(419).Print(ToString(i+1),ToString(DimSize.n[i]),
          ToString(_Md->Dimensions[_Md->Types[TypIndex].DimIndex].DimSize.n[i]),_Md->CannonicalTypeName(TypIndex));
          return false;
        }
      }
    }
  }
  else if(_Md->Types[TypIndex].MstType==MasterType::Class){
    if(_Md->FieldLoop(TypIndex,FldIndex)!=-1){
      Stn.Tokens[BegToken].Msg(420).Print(ToString(ElementNr),ToString(_Md->FieldCount(TypIndex)),_Md->CannonicalTypeName(TypIndex));
      return false;
    }
  }

  //Setup outer brace attributes
  if(_Md->Types[TypIndex].MstType==MasterType::FixArray
  || _Md->Types[TypIndex].MstType==MasterType::DynArray){
    CmplxAttr[BegToken].Used=true;
    CmplxAttr[BegToken].TypIndex=TypIndex;
    CmplxAttr[BegToken].DimSize=DimSize;
    CmplxAttr[BegToken].CurlyClass=CurlyBraceClass::Outer;
    CmplxAttr[EndToken].Used=true;
    CmplxAttr[EndToken].TypIndex=-1;
    CmplxAttr[EndToken].DimSize=ZERO_INDEXES;
    CmplxAttr[EndToken].CurlyClass=CurlyBraceClass::Outer;
  }
  else if(_Md->Types[TypIndex].MstType==MasterType::Class){
    CmplxAttr[BegToken].Used=true;
    CmplxAttr[BegToken].TypIndex=TypIndex;
    CmplxAttr[BegToken].DimSize=ZERO_INDEXES;
    CmplxAttr[BegToken].CurlyClass=CurlyBraceClass::Outer;
    CmplxAttr[EndToken].Used=true;
    CmplxAttr[EndToken].TypIndex=-1;
    CmplxAttr[EndToken].DimSize=ZERO_INDEXES;
    CmplxAttr[EndToken].CurlyClass=CurlyBraceClass::Outer;
  }

  //Return read tokens
  ReadTokens=EndToken-BegToken;

  //Show tokenizer output
  #ifdef __DEV__
  String Output="";
  for(i=BegToken;i<=EndToken;i++){
    Output+=ToString(i)+":"+Stn.Tokens[i].Print();
    if(CmplxAttr[i].Used){
      Output+=String(":")+(CmplxAttr[i].CurlyClass==CurlyBraceClass::Inner?"i":"o");
      if(Stn.Tokens[i].Id()==PrTokenId::Punctuator 
      && Stn.Tokens[i].Value.Pnc==PrPunctuator::BegCurly 
      && CmplxAttr[i].CurlyClass==CurlyBraceClass::Outer 
      && (_Md->Types[CmplxAttr[i].TypIndex].MstType==MasterType::FixArray
      || _Md->Types[CmplxAttr[i].TypIndex].MstType==MasterType::DynArray)){
        Output+="[";
        for(j=0;j<_Md->Types[CmplxAttr[i].TypIndex].DimNr;j++){
          Output+=(j>0?",":"")+ToString(CmplxAttr[i].DimSize.n[j]);
        }
        Output+="]";
      }
    }
    Output+=" ";
  }
  #endif
  DebugMessage(DebugLevel::CmpComplexLit,String(2*RecurLevel,' ')+"Complex tokenizer out: "+Output);  

  //Debug message
  DebugMessage(DebugLevel::CmpComplexLit,String(2*RecurLevel,' ')+"End complex tokenizer: token="+ToString(EndToken)+":"+Stn.Tokens[EndToken].Print()+" type="+_Md->CannonicalTypeName(TypIndex)+" readtokens="+ToString(ReadTokens));
  
  //Return success
  return true;

}

//Print expression
String Expression::Print() const {
  String Output;
  if(_Tokens.Length()==0){
    Output="(empty)";
  }
  else{
    for(int i=0;i<_Tokens.Length();i++){
      Output+=(Output.Length()!=0?" ":"")+_Tokens[i].Print();
    }
  }
  return Output;
}

//Return if operator is overloadable
bool Expression::IsOverloadableOperator(PrOperator Opr){
  ExprOperator ExOpr=_TranslateOperator(Opr);
  if((int)ExOpr==-1){ return false; }
  return _Opr[(int)ExOpr].IsOverloadable;
}

//Translate parser operator into expression compiler operator
ExprOperator Expression::_TranslateOperator(PrOperator Opr){
  ExprOperator RetOpr;
  switch(Opr){
    case PrOperator::PostfixIncrement: RetOpr=ExprOperator::PostfixInc;     break;
    case PrOperator::PostfixDecrement: RetOpr=ExprOperator::PostfixDec;     break;
    case PrOperator::PrefixIncrement:  RetOpr=ExprOperator::PrefixInc;      break;
    case PrOperator::PrefixDecrement:  RetOpr=ExprOperator::PrefixDec;      break;
    case PrOperator::Plus:             RetOpr=ExprOperator::UnaryPlus;      break;
    case PrOperator::Minus:            RetOpr=ExprOperator::UnaryMinus;     break;
    case PrOperator::Add:              RetOpr=ExprOperator::UnaryPlus;      break;
    case PrOperator::Sub:              RetOpr=ExprOperator::UnaryMinus;     break;
    case PrOperator::LogicalNot:       RetOpr=ExprOperator::LogicalNot;     break;
    case PrOperator::BitwiseNot:       RetOpr=ExprOperator::BitwiseNot;     break;
    case PrOperator::Asterisk:         RetOpr=ExprOperator::Multiplication; break;
    case PrOperator::Division:         RetOpr=ExprOperator::Division;       break;
    case PrOperator::Modulus:          RetOpr=ExprOperator::Modulus;        break;
    case PrOperator::ShiftLeft:        RetOpr=ExprOperator::ShiftLeft;      break;
    case PrOperator::ShiftRight:       RetOpr=ExprOperator::ShiftRight;     break;
    case PrOperator::Less:             RetOpr=ExprOperator::Less;           break;
    case PrOperator::LessEqual:        RetOpr=ExprOperator::LessEqual;      break;
    case PrOperator::Greater:          RetOpr=ExprOperator::Greater;        break;
    case PrOperator::GreaterEqual:     RetOpr=ExprOperator::GreaterEqual;   break;
    case PrOperator::Equal:            RetOpr=ExprOperator::Equal;          break;
    case PrOperator::Distinct:         RetOpr=ExprOperator::Distinct;       break;
    case PrOperator::BitwiseAnd:       RetOpr=ExprOperator::BitwiseAnd;     break;
    case PrOperator::BitwiseXor:       RetOpr=ExprOperator::BitwiseXor;     break;
    case PrOperator::BitwiseOr:        RetOpr=ExprOperator::BitwiseOr;      break;
    case PrOperator::LogicalAnd:       RetOpr=ExprOperator::LogicalAnd;     break;
    case PrOperator::LogicalOr:        RetOpr=ExprOperator::LogicalOr;      break;
    case PrOperator::Assign:           RetOpr=ExprOperator::Assign;         break;
    case PrOperator::AddAssign:        RetOpr=ExprOperator::AddAssign;      break;
    case PrOperator::SubAssign:        RetOpr=ExprOperator::SubAssign;      break;
    case PrOperator::MulAssign:        RetOpr=ExprOperator::MulAssign;      break;
    case PrOperator::DivAssign:        RetOpr=ExprOperator::DivAssign;      break;
    case PrOperator::ModAssign:        RetOpr=ExprOperator::ModAssign;      break;
    case PrOperator::ShlAssign:        RetOpr=ExprOperator::ShlAssign;      break;
    case PrOperator::ShrAssign:        RetOpr=ExprOperator::ShrAssign;      break;
    case PrOperator::AndAssign:        RetOpr=ExprOperator::AndAssign;      break;
    case PrOperator::XorAssign:        RetOpr=ExprOperator::XorAssign;      break;
    case PrOperator::OrAssign:         RetOpr=ExprOperator::OrAssign;       break;
    case PrOperator::SeqOper:          RetOpr=ExprOperator::SeqOper;        break;
    case PrOperator::TernaryIf:        RetOpr=(ExprOperator)-1;             break; //No direct translation
    case PrOperator::Member:           RetOpr=(ExprOperator)-1;             break; //No direct translation
  }
  return RetOpr;
}

//Expression tokenizer
bool Expression::_Tokenize(const ScopeDef& Scope,const Sentence& Stn,int BegToken,int EndToken){

  //Variables
  int i,j;
  int EndParToken;
  int ReadTokens;
  int MemberCase;
  int IdentifierCase;
  int TypeCase;
  int VarIndex;
  int TokenModIndex;
  int NextTokenModIndex;
  int ModIndex;
  int TrkIndex;
  int TypIndex;
  int FldIndex;
  int FoundIndex;
  int ParmCount;
  bool TypeCastLegal;
  bool SetInitOpr;
  bool HasInitialization;
  String TypeName;
  String VarName;
  String FoundObject;
  ExprToken Token;
  Sentence SubStn;
  Array<ComplexAttr> CmplxAttr;
  Array<FlowOprAttr> FlowOpr;
  CpuLon Label;
  Stack<CpuLon> FlowLabel;
  SourceInfo SrcInfo;

  //Clear set initialization operator flag
  SetInitOpr=false;

  //Initialize token modindex
  TokenModIndex=Scope.ModIndex;
  NextTokenModIndex=-1;

  //Initialize flow operator array
  FlowOpr.Resize(Stn.Tokens.Length());
  for(i=0;i<FlowOpr.Length();i++){ FlowOpr[i]=(FlowOprAttr){(ExprFlowOpr)-1,-1}; }

  //Parser token loop
  for(i=BegToken;i<=EndToken;i++){

    //Common fields for all tokens
    Token.Clear();
    Token.SetMdPointer(_Md);
    Token.SetPosition(Stn.Tokens[i].SrcInfo());

    //Set module index for token
    if(NextTokenModIndex!=-1){
      TokenModIndex=NextTokenModIndex;
      NextTokenModIndex=-1;
    }
    else{
      TokenModIndex=Scope.ModIndex;
    }

    //Switch on token type
    switch(Stn.Tokens[i].Id()){
      
      //Operator
      case PrTokenId::Operator:
      
        //Ternary if
        if(Stn.Tokens[i].Value.Opr==PrOperator::TernaryIf){
          Token.Id(ExprTokenId::LowLevelOpr); 
          Token.Value.LowLevelOpr=ExprLowLevelOpr::TernaryCond;
        }
          
        //Member operator
        else if(Stn.Tokens[i].Value.Opr==PrOperator::Member){
          
          //Error if next token is not an identifier
          if(i<EndToken && Stn.Tokens[i+1].Id()!=PrTokenId::Identifier){
            Stn.Tokens[i].Msg(53).Print();
            return false;
          }

          //Get member case to test (1=Field,2=Method)
          if(i+1<EndToken){
            if(Stn.Tokens[i+2].Id()==PrTokenId::Punctuator && Stn.Tokens[i+2].Value.Pnc==PrPunctuator::BegParen){ MemberCase=2; }
            else{ MemberCase=1; }
          }
          else if(i+1==EndToken){
            MemberCase=1; 
          }

          //Switch on member case
          switch(MemberCase){
            
            //Field
            case 1:
              Token.Id(ExprTokenId::Field);
              Token.Value.Field=Stn.Tokens[i+1].Value.Idn;
              Token.SetPosition(Stn.Tokens[i+1].SrcInfo());
              break;

            //Method
            case 2:
              if(!_CountParameters(Stn,i+2,ParmCount)){ return false; }
              Token.Id(ExprTokenId::Method);
              Token.Value.Method=Stn.Tokens[i+1].Value.Idn;
              Token.CallParmNr=ParmCount;
              Token.SetPosition(Stn.Tokens[i+1].SrcInfo());
              break;

          }

          //Increase index since we have processed the identifier next to member operator
          i++;

          //Reset token module index
          TokenModIndex=Scope.ModIndex;

        }

        //PostfixIncrement operator (re-interpreted depending on context)
        else if(Stn.Tokens[i].Value.Opr==PrOperator::PostfixIncrement){
          Token.Id(ExprTokenId::Operator);
          Token.Value.Operator=(_IsLastTokenOperand()?ExprOperator::PostfixInc:ExprOperator::PrefixInc);
        }
        
        //PostfixDecrement operator (re-interpreted depending on context)
        else if(Stn.Tokens[i].Value.Opr==PrOperator::PostfixDecrement){
          Token.Id(ExprTokenId::Operator);
          Token.Value.Operator=(_IsLastTokenOperand()?ExprOperator::PostfixDec:ExprOperator::PrefixDec);
        }
        
        //Add operator (re-interpreted depending on context)
        else if(Stn.Tokens[i].Value.Opr==PrOperator::Add){
          Token.Id(ExprTokenId::Operator);
          Token.Value.Operator=(_IsLastTokenOperand()?ExprOperator::Addition:ExprOperator::UnaryPlus);
        }

        //Sub operator (re-interpreted depending on context)
        else if(Stn.Tokens[i].Value.Opr==PrOperator::Sub){
          Token.Id(ExprTokenId::Operator);
          Token.Value.Operator=(_IsLastTokenOperand()?ExprOperator::Substraction:ExprOperator::UnaryMinus);
        }

        //Initialization operator
        else if(Stn.Tokens[i].Value.Opr==PrOperator::Assign && SetInitOpr){
          Token.Id(ExprTokenId::Operator);
          Token.Value.Operator=ExprOperator::Initializ;
          SetInitOpr=false;
        }

        //Rest of operators
        else{
          Token.Id(ExprTokenId::Operator);
          Token.Value.Operator=_TranslateOperator(Stn.Tokens[i].Value.Opr);
        }
        break;
      
      //Punctuator
      case PrTokenId::Punctuator:
        
        //Switch on punctuator
        switch(Stn.Tokens[i].Value.Pnc){
          
          //Beginning parenthesys
          case PrPunctuator::BegParen:   
            
            //Type cast
            //We check that previous token is not an identifier, because that is a function call
            //Also we check that after type there is not an operator because if we have a member operator it can be the field of an enum type
            //Also we check that after type there is not a opening parenthesyss because that is a consructor function call
            if((i==0 && i+2<=EndToken && Stn.Tokens[i+1].Id()==PrTokenId::TypeName && Stn.Tokens[i+2].Id()!=PrTokenId::Operator && (Stn.Tokens[i+2].Id()!=PrTokenId::Punctuator || Stn.Tokens[i+2].Value.Pnc!=PrPunctuator::BegParen))
            || (i>0  && i+2<=EndToken && Stn.Tokens[i-1].Id()!=PrTokenId::Identifier && Stn.Tokens[i+1].Id()==PrTokenId::TypeName && Stn.Tokens[i+2].Id()!=PrTokenId::Operator && (Stn.Tokens[i+2].Id()!=PrTokenId::Punctuator || Stn.Tokens[i+2].Value.Pnc!=PrPunctuator::BegParen))){
              
              //Get ending parenthesys
              ReadTokens=0;
              EndParToken=-1;
              for(j=i;j<=EndToken;j++){
                ReadTokens++;
                if(Stn.Tokens[j].Id()==PrTokenId::Punctuator && Stn.Tokens[j].Value.Pnc==PrPunctuator::EndParen){
                  EndParToken=j;
                  break;
                }
              }
              if(EndParToken==-1){
                Stn.Tokens[i].Msg(165).Print();
                return false;
              }

              //Parse type specification
              SubStn=Stn.SubSentence(i+1,EndParToken-1);
              if(!ReadTypeSpec(_Md,SubStn,Scope,TypIndex,false)){ return false; }

              //If we have an identifier after type specification we are not in a typecast, but it could be a variable declaration so we parse as parenthesys
              if(SubStn.TokensLeft()!=0 && SubStn.CurrentToken().Id()==PrTokenId::Identifier){
                Token.Id(ExprTokenId::Delimiter);
                Token.Value.Delimiter=ExprDelimiter::BegParen; 
              }
          
              //If not we assume we have a type cast
              else{

                //Check there are not unexpected tokens after type specification and before ending parenthesys
                if(SubStn.TokensLeft()!=0){
                  SubStn.CurrentToken().Msg(374).Print();
                  return false;
                }

                //Module public objects cannot use datatypes of inner modules since they are not exported
                if(Scope.Kind==ScopeKind::Public && _Md->Types[TypIndex].Scope.ModIndex!=Scope.ModIndex && !_Md->Types[TypIndex].IsSystemDef){
                  Stn.Msg(206).Print(_Md->CannonicalTypeName(TypIndex),_Md->Modules[_Md->Types[TypIndex].Scope.ModIndex].Name);
                  return false;
                }

                //Check type cast is legal
                switch(_Md->Types[TypIndex].MstType){
                  case MasterType::Boolean :
                  case MasterType::Char    :
                  case MasterType::Short   :
                  case MasterType::Integer :
                  case MasterType::Long    :
                  case MasterType::Float   :
                  case MasterType::String  :
                  case MasterType::FixArray:
                  case MasterType::DynArray:
                    TypeCastLegal=true;
                    break;
                  case MasterType::Class:
                    if(i+ReadTokens<=EndToken 
                    && Stn.Tokens[i+ReadTokens].Id()==PrTokenId::Punctuator 
                    && Stn.Tokens[i+ReadTokens].Value.Pnc==PrPunctuator::BegCurly){
                      TypeCastLegal=true;   
                    }
                    else{
                      TypeCastLegal=false;
                    }
                    break;
                  case MasterType::Enum :
                    TypeCastLegal=false;
                    break;
                }
                if(!TypeCastLegal){
                  Stn.Tokens[i].Msg(92).Print(_Md->CannonicalTypeName(TypIndex));
                  return false;
                }

                //Tokenize typecast operator
                Token.Id(ExprTokenId::Operator);
                Token.Value.Operator=ExprOperator::TypeCast;
                Token.CastTypIndex=TypIndex;
                Token.SetPosition(Stn.Tokens[i+1].SrcInfo());

                //Advance processing position
                i+=(ReadTokens>1?ReadTokens-1:0);
              }

            }
            
            //Just a parenthesys
            else{
              Token.Id(ExprTokenId::Delimiter);
              Token.Value.Delimiter=ExprDelimiter::BegParen; 
            }
            break;
          
          //Curly brace open
          case PrPunctuator::BegCurly:

            //If token before is a parenthesys we have the first curly brace after the type
            if(i>BegToken && Stn.Tokens[i-1].Id()==PrTokenId::Punctuator && Stn.Tokens[i-1].Value.Pnc==PrPunctuator::EndParen){
  
              //Previous tokenized token must be a typecast operator, then take complex type
              if(_Tokens.Length()>0 && _Tokens[_Tokens.Length()-1].Id()==ExprTokenId::Operator && _Tokens[_Tokens.Length()-1].Value.Operator==ExprOperator::TypeCast){
                TypIndex=_Tokens[_Tokens.Length()-1].CastTypIndex;
              }
              else{
                Stn.Tokens[i].Msg(395).Print();
                return false;
              }

              //Data type must be one that admits initializer list (class or array)
              if(_Md->Types[TypIndex].MstType!=MasterType::Class 
              && _Md->Types[TypIndex].MstType!=MasterType::DynArray 
              && _Md->Types[TypIndex].MstType!=MasterType::FixArray){
                _Tokens[_Tokens.Length()-1].Msg(471).Print();
                return false;
              }

              //Tokenize complex lit value
              if(!_ComplexLitValueTokenize(Stn,TypIndex,i,CmplxAttr,ReadTokens)){ return false; }

              //Replace previous tokenized typecast operator by a complex token
              _Tokens[_Tokens.Length()-1].Clear();
              _Tokens[_Tokens.Length()-1].Id(ExprTokenId::Complex);
              _Tokens[_Tokens.Length()-1].SetMdPointer(_Md);
              _Tokens[_Tokens.Length()-1].SetPosition(Stn.Tokens[i].SrcInfo());
              _Tokens[_Tokens.Length()-1].Value.ComplexTypIndex=TypIndex;
              _Tokens[_Tokens.Length()-1].DimSize=CmplxAttr[i].DimSize;

              //Set token
              Token.Id(ExprTokenId::Delimiter);
              Token.Value.Delimiter=ExprDelimiter::BegCurly; 

            }
            
            //Then we have a curly brace inside another
            else{

              //Then token before must be forcibly another beginning curly or a comma
              if(i==BegToken || Stn.Tokens[i-1].Id()!=PrTokenId::Punctuator || (Stn.Tokens[i-1].Value.Pnc!=PrPunctuator::BegCurly && Stn.Tokens[i-1].Value.Pnc!=PrPunctuator::Comma)){
                Stn.Tokens[i].Msg(403).Print();
                return false;
              }

              //The complex value has to be already tokenized before
              if(i>CmplxAttr.Length()-1 || CmplxAttr[i].Used==false){
                Stn.Tokens[i].Msg(404).Print();
                return false;
              }

              //Set token
              switch(CmplxAttr[i].CurlyClass){
                
                //Outer curly braces (insert complex token and opening curly brace)
                case CurlyBraceClass::Outer:
                  
                  //Complex token
                  Token.Id(ExprTokenId::Complex);
                  Token.Value.ComplexTypIndex=CmplxAttr[i].TypIndex;
                  Token.DimSize=CmplxAttr[i].DimSize;
                  _Tokens.Add(Token);

                  //Opening curly brace
                  Token.Clear();
                  Token.Id(ExprTokenId::Delimiter);
                  Token.SetMdPointer(_Md);
                  Token.SetPosition(Stn.Tokens[i].SrcInfo());
                  Token.Value.Delimiter=ExprDelimiter::BegCurly; 
                  break;

                //Inner curly brace (just ignored)
                case CurlyBraceClass::Inner:
                  Token.Clear(); 
                  break;
              }

            }
            break;
          
          //Ending curly brace
          case PrPunctuator::EndCurly:   

            //The complex value has to be already tokenized before
            if(i>CmplxAttr.Length()-1 || CmplxAttr[i].Used==false){
              Stn.Tokens[i].Msg(412).Print();
              return false;
            }

            //Set token
            switch(CmplxAttr[i].CurlyClass){
              
              //Outer curly braces
              case CurlyBraceClass::Outer:
                Token.Id(ExprTokenId::Delimiter);
                Token.Value.Delimiter=ExprDelimiter::EndCurly; 
                break;

              //Inner curly brace (just ignored)
              case CurlyBraceClass::Inner:
                Token.Clear(); 
                break;
            }
            break;
          
          //Colon, parsed as ternary else operator when not at end of sentence
          case PrPunctuator::Colon:
            if(i<EndToken){
              Token.Id(ExprTokenId::LowLevelOpr); 
              Token.Value.LowLevelOpr=ExprLowLevelOpr::TernaryMid; 
            }
            else{
              Stn.Tokens[i].Msg(543).Print(Stn.Tokens[i].Description());
              return false;
            }
            break;

          //End parenthesys
          case PrPunctuator::EndParen:   
            
            //Normal parenthesys
            if(FlowOpr[i].Opr==(ExprFlowOpr)-1){
              Token.Id(ExprTokenId::Delimiter); 
              Token.Value.Delimiter=ExprDelimiter::EndParen; 
            }

            //Parenthesys is a flow operator
            else{
              
              //Error if a flow operator is identified but it is unknown
              if(FlowOpr[i].Opr!=ExprFlowOpr::ForEnd && FlowOpr[i].Opr!=ExprFlowOpr::ArrEnd){
                Stn.Tokens[i].Msg(459).Print();
                return false;
              }
            
              //Output flow operator
              Token.Id(ExprTokenId::FlowOpr); 
              Token.Value.FlowOpr=FlowOpr[i].Opr; 
              Token.FlowLabel=FlowOpr[i].Label; 
              _Tokens.Add(Token);

              //Output parenthesys
              Token.Clear();
              Token.Id(ExprTokenId::Delimiter);
              Token.SetMdPointer(_Md);
              Token.SetPosition(Stn.Tokens[i].SrcInfo());
              Token.Value.Delimiter=ExprDelimiter::EndParen; 

              //Pop flow label from stack
              if(FlowLabel.Length()==0){
                Stn.Tokens[i].Msg(460).Print();
                return false;
              }
              FlowLabel.Pop();
            
            }
            
            break;

          //Rest of punctuators
          case PrPunctuator::BegBracket: Token.Id(ExprTokenId::Delimiter); Token.Value.Delimiter=ExprDelimiter::BegBracket; break;
          case PrPunctuator::EndBracket: Token.Id(ExprTokenId::Delimiter); Token.Value.Delimiter=ExprDelimiter::EndBracket; break;
          case PrPunctuator::Comma:      Token.Id(ExprTokenId::Delimiter); Token.Value.Delimiter=ExprDelimiter::Comma; break;

          //Others not allowed
          case PrPunctuator::Splitter:
            Stn.Tokens[i].Msg(93).Print(Stn.Tokens[i].Description());
            return false;
            break;

        }
        break;

      
      //Identifier
      case PrTokenId::Identifier:

        //Get identifier case to test
        //1=Function
        //2=ModuleTracker/ClassVariable
        //3=Variable
        if(i+1<=EndToken){
          if(Stn.Tokens[i+1].Id()==PrTokenId::Punctuator && Stn.Tokens[i+1].Value.Pnc==PrPunctuator::BegParen){ IdentifierCase=1; }
          else if(Stn.Tokens[i+1].Id()==PrTokenId::Operator && Stn.Tokens[i+1].Value.Opr==PrOperator::Member){ IdentifierCase=2; }
          else{ IdentifierCase=3; }
        }
        else if(i==EndToken){
          IdentifierCase=3; 
        }

        //Switch on identifier case
        switch(IdentifierCase){
          
          //Function name
          case 1:

            //Set token
            if(!_CountParameters(Stn,i+1,ParmCount)){ return false; }
            Token.Id(ExprTokenId::Function);
            Token.Value.Function=Stn.Tokens[i].Value.Idn;
            Token.CallParmNr=ParmCount;
            Token.FunModIndex=TokenModIndex;

            //Reset token module index
            TokenModIndex=Scope.ModIndex;
            break;
          
          //Module tracker / Class variable / Regular variable
          case 2:

            //Check identifier as module tracker
            if((TrkIndex=_Md->TrkSearch(Stn.Tokens[i].Value.Idn))!=-1){
              
              //Set module index for next token
              NextTokenModIndex=_Md->Trackers[TrkIndex].ModIndex;    
              
              //Increase index to process member operator
              i++;
            
            }

            //Check identifier class variable
            else if((VarIndex=_Md->VarSearch(Stn.Tokens[i].Value.Idn,TokenModIndex))!=-1 
            && _Md->Types[_Md->Variables[VarIndex].TypIndex].MstType==MasterType::Class){
  
              //Set token
              Token.Id(ExprTokenId::Operand);
              Token.AdrMode=(_Md->Variables[VarIndex].IsReference?CpuAdrMode::Indirection:CpuAdrMode::Address);
              Token.Value.VarIndex=VarIndex;
              Token.IsConst=_Md->Variables[VarIndex].IsConst;
            
            }

            //Parse identifier as a regular variable since member operator can be used to access a master method
            else if((VarIndex=_Md->VarSearch(Stn.Tokens[i].Value.Idn,TokenModIndex))!=-1){
  
              //Set token
              Token.Id(ExprTokenId::Operand);
              Token.AdrMode=(_Md->Variables[VarIndex].IsReference?CpuAdrMode::Indirection:CpuAdrMode::Address);
              Token.Value.VarIndex=VarIndex;
              Token.IsConst=_Md->Variables[VarIndex].IsConst;

              //Convert variable to litteral value whenever possible
              if(Token.IsComputableOperand()){
                if(!Token.ToLitValue()){ return false; }
                Token.SourceVarIndex=VarIndex;
              }

              //Reset token module index
              TokenModIndex=Scope.ModIndex;
            
            }

            //Check if variable has been parsed previously as undefined variable
            else if(TokenModIndex==Scope.ModIndex){

              //Find identifier as undefined variable in parsed expression
              FoundIndex=-1;
              for(j=_Tokens.Length()-1;j>=0;j--){
                if(_Tokens[j].Id()==ExprTokenId::UndefVar && _Tokens[j].Value.VarName==Stn.Tokens[i].Value.Idn){
                  FoundIndex=j; break;
                }
              }

              //Found as undefined variable
              if(FoundIndex!=-1){
                Token.Id(ExprTokenId::UndefVar);
                Token.Value.VarName=Stn.Tokens[i].Value.Idn;
              }

              //Not found: error
              else{
                Stn.Tokens[i].Msg(465).Print(Stn.Tokens[i].Value.Idn);
                return false;
              }
            
            }
            
            //Rest of cases is error
            else{
              Stn.Tokens[i].Msg(177).Print(Stn.Tokens[i].Value.Idn);
              return false;
            }

            //Case end
            break;

          //Variable
          case 3:

            //Check variable is defined
            VarIndex=_Md->VarSearch(Stn.Tokens[i].Value.Idn,TokenModIndex);
            
            //Variable is defined
            if(VarIndex!=-1){

              //Set token
              Token.Id(ExprTokenId::Operand);
              Token.AdrMode=(_Md->Variables[VarIndex].IsReference?CpuAdrMode::Indirection:CpuAdrMode::Address);
              Token.Value.VarIndex=VarIndex;
              Token.IsConst=_Md->Variables[VarIndex].IsConst;

              //Convert variable to litteral value whenever possible
              if(Token.IsComputableOperand()){
                if(!Token.ToLitValue()){ return false; }
                Token.SourceVarIndex=VarIndex;
              }

              //Reset token module index
              TokenModIndex=Scope.ModIndex;

            }

            //Variable is not defined
            else if(TokenModIndex==Scope.ModIndex){

              //Find identifier as undefined variable in parsed expression
              FoundIndex=-1;
              for(j=_Tokens.Length()-1;j>=0;j--){
                if(_Tokens[j].Id()==ExprTokenId::UndefVar && _Tokens[j].Value.VarName==Stn.Tokens[i].Value.Idn){
                  FoundIndex=j; break;
                }
              }

              //Found as undefined variable
              if(FoundIndex!=-1){
                Token.Id(ExprTokenId::UndefVar);
                Token.Value.VarName=Stn.Tokens[i].Value.Idn;
              }

              //Not found: error
              else{
                Stn.Tokens[i].Msg(467).Print(Stn.Tokens[i].Value.Idn);
                return false;
              }
            
            }

            //Variable not identified
            else{
              Stn.Tokens[i].Msg(51).Print(Stn.Tokens[i].Value.Idn);
              return false;
            }
            break;

        }
        break;
      
      //Type name: Enum fields / class constructor call
      case PrTokenId::TypeName:

        //Get type case
        //1=Enum field
        //2=Class constructor call
        //3=Local variable declaration
        if(i+2<=EndToken && Stn.Tokens[i+1].Id()==PrTokenId::Operator && Stn.Tokens[i+1].Value.Opr==PrOperator::Member && Stn.Tokens[i+2].Id()==PrTokenId::Identifier){ TypeCase=1; }
        else if(i+1<=EndToken && Stn.Tokens[i+1].Id()==PrTokenId::Punctuator && Stn.Tokens[i+1].Value.Pnc==PrPunctuator::BegParen){ TypeCase=2; }
        else{ TypeCase=3; }

        //Swich on type case
        switch(TypeCase){

          //Enum field
          case 1:

            //Decouple type from parser
            _Md->DecoupleTypeName(Stn.Tokens[i].Value.Idn,TypeName,ModIndex);

            //Get type index
            if((TypIndex=_Md->TypSearch(TypeName,ModIndex))==-1){
              Stn.Tokens[i].Msg(94).Print(Stn.Tokens[i].Value.Typ);
              return false;
            }

            //Check type is enumerated class
            if(_Md->Types[TypIndex].MstType!=MasterType::Enum){
              Stn.Tokens[i].Msg(135).Print(Stn.Tokens[i].Value.Typ);
              return false;
            }
            
            //Search enum field name
            if((FldIndex=_Md->FldSearch(Stn.Tokens[i+2].Value.Idn,TypIndex))==-1){
              Stn.Tokens[i].Msg(179).Print(Stn.Tokens[i+2].Value.Idn,Stn.Tokens[i].Value.Idn);
              return false;
            }

            //Set token
            Token.Id(ExprTokenId::Operand);
            Token.LitNumTypIndex=TypIndex;
            Token.AdrMode=CpuAdrMode::LitValue;
            Token.Value.Enu=_Md->Fields[FldIndex].EnumValue;
            Token.IsConst=true;
            Token.SetPosition(Stn.Tokens[i+2].SrcInfo());

            //Increase index to process member operator and enum field name
            i+=2;
            break;

          //Class constructor call
          case 2:

            //Decouple type from parser
            _Md->DecoupleTypeName(Stn.Tokens[i].Value.Idn,TypeName,ModIndex);

            //Get type index
            if((TypIndex=_Md->TypSearch(TypeName,ModIndex))==-1){
              Stn.Tokens[i].Msg(95).Print(Stn.Tokens[i].Value.Typ);
              return false;
            }

            //Tye must be a class
            if(_Md->Types[TypIndex].MstType!=MasterType::Class){
              Stn.Tokens[i].Msg(298).Print(_Md->CannonicalTypeName(TypIndex));
              return false;
            }

            //Count parameters
            if(!_CountParameters(Stn,i+1,ParmCount)){ return false; }

            //Set token
            Token.Id(ExprTokenId::Constructor);
            Token.Value.CCTypIndex=TypIndex;
            Token.CallParmNr=ParmCount;

            //Reset token module index
            TokenModIndex=Scope.ModIndex;
            break;

          //Local variable declaration
          case 3:
            
            //Read type specification
            SubStn=Stn.SubSentence(i,Stn.Tokens.Length()-1);
            if(!ReadTypeSpec(_Md,SubStn,Scope,TypIndex,false)){ return false; }
            
            //Read variable type
            if(!SubStn.ReadId(VarName).Ok()){ return false; }
            SrcInfo=Stn.Tokens[i+SubStn.GetProcIndex()-1].SrcInfo();

            //Variable must not be already defined
            if(_Md->VarSearch(VarName,Scope.ModIndex)!=-1){
              SubStn.Tokens[SubStn.GetProcIndex()-1].Msg(288).Print(VarName);
              return false;
            }

            //Dot collission check
            if(_Md->Types[TypIndex].MstType==MasterType::Class || _Md->Types[TypIndex].MstType==MasterType::Enum){
              if(!_Md->DotCollissionCheck(VarName,FoundObject)){
                SubStn.Tokens[SubStn.GetProcIndex()-1].Msg(185).Print(VarName,FoundObject);
                return false;
              }
            }

            //Store new variable
            Label=(FlowLabel.Length()==0?-1:FlowLabel.Top());
            if(!_Md->ReuseVariable(Scope,Stn.GetCodeBlockId(),Label,VarName,TypIndex,false,false,false,SrcInfo,SysMsgDispatcher::GetSourceLine(),VarIndex)){ return false; }
            if(VarIndex==-1){
              _Md->CleanHidden(Scope,VarName);
              _Md->StoreVariable(Scope,Stn.GetCodeBlockId(),Label,VarName,TypIndex,false,false,false,false,false,false,false,true,true,SrcInfo,SysMsgDispatcher::GetSourceLine());
              VarIndex=_Md->Variables.Length()-1;
              _Md->Bin.AsmOutNewLine(AsmSection::Decl);
              _Md->Bin.AsmOutCommentLine(AsmSection::Decl,"Declared from expression",true);
              _Md->Bin.AsmOutVarDecl(AsmSection::Decl,(Scope.Kind!=ScopeKind::Local?true:false),false,false,false,false,VarName,
              _Md->CpuDataTypeFromMstType(_Md->Types[TypIndex].MstType),_Md->VarLength(VarIndex),_Md->Variables[VarIndex].Address,"",false,false,"","");
            }

            //Variable is declared with initialization
            if(SubStn.Is(PrOperator::Asterisk) || _Md->InitializeVars==true){
              if(SubStn.Is(PrOperator::Asterisk)){
                if(!SubStn.Get(PrOperator::Asterisk).Ok()){ return false; }
              }
              HasInitialization=true;
            }
            else{
              HasInitialization=false;
            }

            //Set token
            Token.Id(ExprTokenId::Operand);
            Token.AdrMode=CpuAdrMode::Address;
            Token.Value.VarIndex=VarIndex;
            Token.IsConst=false;
            Token.HasInitialization=HasInitialization;
            Token.SetPosition(SrcInfo);

            //Advance processed tokens
            i+=SubStn.GetProcIndex()-1;

            //If next token is assignmnt operator change it to initialization operator (if variable is not declared with initialization)
            if(i+1<=EndToken && Stn.Tokens[i+1].Id()==PrTokenId::Operator && Stn.Tokens[i+1].Value.Opr==PrOperator::Assign && HasInitialization==false){
              SetInitOpr=true;
            }
            break;
          
        }
        break;
      
      //Boolean
      case PrTokenId::Boolean:
        Token.Id(ExprTokenId::Operand);
        Token.LitNumTypIndex=_Md->BolTypIndex;
        Token.AdrMode=CpuAdrMode::LitValue;
        Token.Value.Bol=Stn.Tokens[i].Value.Bol;
        Token.IsConst=true;
        break;
      
      //char
      case PrTokenId::Char:
        Token.Id(ExprTokenId::Operand);
        Token.LitNumTypIndex=_Md->ChrTypIndex;
        Token.AdrMode=CpuAdrMode::LitValue;
        Token.Value.Chr=Stn.Tokens[i].Value.Chr;
        Token.IsConst=true;
        break;
      
      //Short
      case PrTokenId::Short:
        Token.Id(ExprTokenId::Operand);
        Token.LitNumTypIndex=_Md->ShrTypIndex;
        Token.AdrMode=CpuAdrMode::LitValue;
        Token.Value.Shr=Stn.Tokens[i].Value.Shr;
        Token.IsConst=true;
        break;
      
      //Integer
      case PrTokenId::Integer:
        Token.Id(ExprTokenId::Operand);
        Token.LitNumTypIndex=_Md->IntTypIndex;
        Token.AdrMode=CpuAdrMode::LitValue;
        Token.Value.Int=Stn.Tokens[i].Value.Int;
        Token.IsConst=true;
        break;
      
      //Long
      case PrTokenId::Long:
        Token.Id(ExprTokenId::Operand);
        Token.LitNumTypIndex=_Md->LonTypIndex;
        Token.AdrMode=CpuAdrMode::LitValue;
        Token.Value.Lon=Stn.Tokens[i].Value.Lon;
        Token.IsConst=true;
        break;
      
      //Float
      case PrTokenId::Float:
        Token.Id(ExprTokenId::Operand);
        Token.LitNumTypIndex=_Md->FloTypIndex;
        Token.AdrMode=CpuAdrMode::LitValue;
        Token.Value.Flo=Stn.Tokens[i].Value.Flo;
        Token.IsConst=true;
        break;
      
      //String
      case PrTokenId::String:
        Token.Id(ExprTokenId::Operand);
        Token.LitNumTypIndex=_Md->StrTypIndex;
        Token.AdrMode=CpuAdrMode::LitValue;
        Token.Value.Adr=_Md->StoreLitString(Stn.Tokens[i].Value.Str);
        Token.IsConst=true;
        break;

      //Rest of tokens not permitted on expression
      case PrTokenId::Keyword: 

        //Var keyword (send undefined variable)
        if(Stn.Tokens[i].Value.Kwd==PrKeyword::Var){

          //Check if we have identifier and assignment operator next to it
          if(i+2<=EndToken 
          && Stn.Tokens[i+1].Id()==PrTokenId::Identifier 
          && Stn.Tokens[i+2].Id()==PrTokenId::Operator && Stn.Tokens[i+2].Value.Opr==PrOperator::Assign){

            //Send token
            Token.Id(ExprTokenId::UndefVar);
            Token.Value.VarName=Stn.Tokens[i+1].Value.Idn;
            Token.SetPosition(Stn.Tokens[i+1].SrcInfo());

            //Next assignment is initalization operator
            SetInitOpr=true;

            //Skip identifier
            i++;

          }

          //Identifier plus assignment is expected after var keyword
          else{
            Stn.Tokens[i].Msg(447).Print();
            return false;
          }

        }

        //For expression
        else if(Stn.Tokens[i].Value.Kwd==PrKeyword::For){

          //Next token must be an openning parenthesys
          if(!(i+1<=EndToken && Stn.Tokens[i+1].Id()==PrTokenId::Punctuator && Stn.Tokens[i+1].Value.Pnc==PrPunctuator::BegParen)){
            Stn.Tokens[i].Msg(455).Print();
            return false;
          }

          //Parse for expression
          if(!_FlowOperatorParse(Stn,i,EndToken,FlowOpr)){ return false; }

          //Output parenthesys
          Token.Id(ExprTokenId::Delimiter);
          Token.Value.Delimiter=ExprDelimiter::BegParen; 
          _Tokens.Add(Token);

          //Output flow operator
          Token.Clear();
          Token.Id(ExprTokenId::FlowOpr); 
          Token.SetMdPointer(_Md);
          Token.SetPosition(Stn.Tokens[i].SrcInfo());
          Token.Value.FlowOpr=FlowOpr[i].Opr; 
          Token.FlowLabel=FlowOpr[i].Label; 

          //Push flow label into stack
          FlowLabel.Push(FlowOpr[i].Label);

          //Skip openning parenthesys
          i++;

        }

        //Array expression
        else if(Stn.Tokens[i].Value.Kwd==PrKeyword::Array){

          //Next token must be an openning parenthesys
          if(!(i+1<=EndToken && Stn.Tokens[i+1].Id()==PrTokenId::Punctuator && Stn.Tokens[i+1].Value.Pnc==PrPunctuator::BegParen)){
            Stn.Tokens[i].Msg(489).Print();
            return false;
          }

          //Parse for expression
          if(!_FlowOperatorParse(Stn,i,EndToken,FlowOpr)){ return false; }

          //Output parenthesys
          Token.Id(ExprTokenId::Delimiter);
          Token.Value.Delimiter=ExprDelimiter::BegParen; 
          _Tokens.Add(Token);

          //Output flow operator
          Token.Clear();
          Token.Id(ExprTokenId::FlowOpr); 
          Token.SetMdPointer(_Md);
          Token.SetPosition(Stn.Tokens[i].SrcInfo());
          Token.Value.FlowOpr=FlowOpr[i].Opr; 
          Token.FlowLabel=FlowOpr[i].Label; 

          //Push flow label into stack
          FlowLabel.Push(FlowOpr[i].Label);

          //Skip openning parenthesys
          i++;

        }

        //If (for/array expressions)
        else if(Stn.Tokens[i].Value.Kwd==PrKeyword::If){
          if(FlowOpr[i].Opr==ExprFlowOpr::ForIf){
            Token.Id(ExprTokenId::FlowOpr); 
            Token.Value.FlowOpr=FlowOpr[i].Opr; 
            Token.FlowLabel=FlowOpr[i].Label; 
          }
          else if(FlowOpr[i].Opr==(ExprFlowOpr)FLOW_ARR_IF){
            Token.Clear(); 
          }
          else{
            Stn.Tokens[i].Msg(456).Print();
            return false;
          }
        }

        //Do (for expressions)
        else if(Stn.Tokens[i].Value.Kwd==PrKeyword::Do){
          if(FlowOpr[i].Opr==ExprFlowOpr::ForDo){
            Token.Id(ExprTokenId::FlowOpr); 
            Token.Value.FlowOpr=FlowOpr[i].Opr; 
            Token.FlowLabel=FlowOpr[i].Label; 
          }
          else{
            Stn.Tokens[i].Msg(457).Print();
            return false;
          }
        }

        //Return (for expressions)
        else if(Stn.Tokens[i].Value.Kwd==PrKeyword::Return){
          if(FlowOpr[i].Opr==ExprFlowOpr::ForRet){
            Token.Id(ExprTokenId::FlowOpr); 
            Token.Value.FlowOpr=FlowOpr[i].Opr; 
            Token.FlowLabel=FlowOpr[i].Label; 
          }
          else{
            Stn.Tokens[i].Msg(458).Print();
            return false;
          }
        }

        //On (array expressions)
        else if(Stn.Tokens[i].Value.Kwd==PrKeyword::On){
          
          //Check flow operator
          if(FlowOpr[i].Opr!=(ExprFlowOpr)FLOW_ARR_ON
          && FlowOpr[i].Opr!=(ExprFlowOpr)FLOW_ARR_OX){
            Stn.Tokens[i].Msg(490).Print();
            return false;
          }

          //Next token must be an identifier
          if(!(i+1<=EndToken && Stn.Tokens[i+1].Id()==PrTokenId::Identifier)){
            Stn.Tokens[i].Msg(493).Print();
            return false;
          }

          //Get identifier
          VarName=Stn.Tokens[i+1].Value.Idn;

          //Send variable operand if variable is defined
          if((VarIndex=_Md->VarSearch(VarName,Scope.ModIndex))!=-1){
            Stn.Tokens[i+1].Msg(522).Print(VarName);
            return false;
          }

          //Send undefined on variable
          Token.Id(ExprTokenId::UndefVar);
          Token.Value.VarName=VarName;
          Token.SetPosition(Stn.Tokens[i+1].SrcInfo());
          _Tokens.Add(Token);

          //Send flow operator on variable
          Token.Clear();
          Token.SetMdPointer(_Md);
          Token.Id(ExprTokenId::FlowOpr); 
          Token.Value.FlowOpr=ExprFlowOpr::ArrOnvar; 
          Token.FlowLabel=FlowOpr[i].Label; 
          Token.SetPosition(Stn.Tokens[i].SrcInfo());

          //Send array loop init operator if we do not have index
          if(FlowOpr[i].Opr==(ExprFlowOpr)FLOW_ARR_ON){
            _Tokens.Add(Token);
            Token.Clear();
            Token.SetMdPointer(_Md);
            Token.Id(ExprTokenId::FlowOpr); 
            Token.Value.FlowOpr=ExprFlowOpr::ArrInit; 
            Token.FlowLabel=FlowOpr[i].Label; 
            Token.SetPosition(Stn.Tokens[i].SrcInfo());
          }

          //Skip identifier
          i++;

        }

        //Index (array expressions)
        else if(Stn.Tokens[i].Value.Kwd==PrKeyword::Index){

          //Check flow operator
          if(FlowOpr[i].Opr!=(ExprFlowOpr)FLOW_ARR_IX){
            Stn.Tokens[i].Msg(491).Print();
            return false;
          }

          //Next token must be an identifier
          if(!(i+1<=EndToken && Stn.Tokens[i+1].Id()==PrTokenId::Identifier)){
            Stn.Tokens[i].Msg(494).Print();
            return false;
          }

          //Get identifier
          VarName=Stn.Tokens[i+1].Value.Idn;

          //Send variable operand if variable is defined
          if((VarIndex=_Md->VarSearch(VarName,Scope.ModIndex))!=-1){
            Token.Id(ExprTokenId::Operand);
            Token.AdrMode=(_Md->Variables[VarIndex].IsReference?CpuAdrMode::Indirection:CpuAdrMode::Address);
            Token.Value.VarIndex=VarIndex;
            Token.IsConst=_Md->Variables[VarIndex].IsConst;
            Token.SetPosition(Stn.Tokens[i+1].SrcInfo());
            _Tokens.Add(Token);
          }

          //Send undefined variable if not defined
          else{
            Token.Id(ExprTokenId::UndefVar);
            Token.Value.VarName=VarName;
            Token.SetPosition(Stn.Tokens[i+1].SrcInfo());
            _Tokens.Add(Token);
          }

          //Send flow operator index variable
          Token.Clear();
          Token.SetMdPointer(_Md);
          Token.Id(ExprTokenId::FlowOpr); 
          Token.Value.FlowOpr=ExprFlowOpr::ArrIxvar; 
          Token.FlowLabel=FlowOpr[i].Label; 
          Token.SetPosition(Stn.Tokens[i].SrcInfo());
          _Tokens.Add(Token);

          //Send array loop init
          Token.Clear();
          Token.SetMdPointer(_Md);
          Token.Id(ExprTokenId::FlowOpr); 
          Token.Value.FlowOpr=ExprFlowOpr::ArrInit; 
          Token.FlowLabel=FlowOpr[i].Label; 
          Token.SetPosition(Stn.Tokens[i].SrcInfo());

          //Skip identifier
          i++;

        }

        //As (array expressions)
        else if(Stn.Tokens[i].Value.Kwd==PrKeyword::As){
          if(FlowOpr[i].Opr==(ExprFlowOpr)FLOW_ARR_AS){
            Token.Clear();
          }
          else if(FlowOpr[i].Opr==ExprFlowOpr::ArrAsif){
            Token.Id(ExprTokenId::FlowOpr); 
            Token.Value.FlowOpr=FlowOpr[i].Opr; 
            Token.FlowLabel=FlowOpr[i].Label; 
          }
          else{
            Stn.Tokens[i].Msg(492).Print();
            return false;
          }
        }

        //Rest of keywords not allowed in expressions
        else{
          Stn.Tokens[i].Msg(52).Print(Stn.Tokens[i].Description());
          return false;
        }
        break;

    }

    //Add token
    if(Token.Id()!=(ExprTokenId)-1){ 
      _Tokens.Add(Token);
    }

  }

  //Tokenize ternary operators in expression
  if(!_TernaryOperatorTokenize(Scope)){ return false; }
  
  //Check consystency of expression
  if(!_CheckConsystency()){ return false; }

  //Debug message
  DebugMessage(DebugLevel::CmpExpression,"Tokenizer ouput: "+Print());

  //Return code
  return true;

}

//Check expression consistency
//- Parenhesys, brackets and curly brace missmatches
//- Operators have operands
bool Expression::_CheckConsystency() const {

  //Variables
  int i;
  int ParLevel;
  int BraLevel;
  int ClyLevel;

  //Check parenthesys, bracket and curly brace missmatches
  ParLevel=0; BraLevel=0; ClyLevel=0;
  for(i=0;i<_Tokens.Length();i++){
    
    //Follow parenthesys, bracket and curly brace levels
    if(_Tokens[i].Id()==ExprTokenId::Delimiter && _Tokens[i].Value.Delimiter==ExprDelimiter::BegParen){ ParLevel++; }
    if(_Tokens[i].Id()==ExprTokenId::Delimiter && _Tokens[i].Value.Delimiter==ExprDelimiter::BegBracket){ BraLevel++; }
    if(_Tokens[i].Id()==ExprTokenId::Delimiter && _Tokens[i].Value.Delimiter==ExprDelimiter::BegCurly){ ClyLevel++; }
    if(_Tokens[i].Id()==ExprTokenId::Delimiter && _Tokens[i].Value.Delimiter==ExprDelimiter::EndParen){ ParLevel--; }
    if(_Tokens[i].Id()==ExprTokenId::Delimiter && _Tokens[i].Value.Delimiter==ExprDelimiter::EndBracket){ BraLevel--; }
    if(_Tokens[i].Id()==ExprTokenId::Delimiter && _Tokens[i].Value.Delimiter==ExprDelimiter::EndCurly){ ClyLevel--; }

    //There is a missmatch if any level becomes negative
    if(ParLevel<0){ _Tokens[i].Msg(548).Print(); return false; }
    if(BraLevel<0){ _Tokens[i].Msg(549).Print(); return false; }
    if(ClyLevel<0){ _Tokens[i].Msg(550).Print(); return false; }

  }

  //There is a missmatch if any level is not zero at end of expression
  if(ParLevel!=0){ _Tokens[_Tokens.Length()-1].Msg(551).Print(); return false; }
  if(BraLevel!=0){ _Tokens[_Tokens.Length()-1].Msg(552).Print(); return false; }
  if(ClyLevel!=0){ _Tokens[_Tokens.Length()-1].Msg(553).Print(); return false; }

  //Check operators have operands
  for(i=0;i<_Tokens.Length();i++){
    if(_Tokens[i].Id()==ExprTokenId::Operator){
      switch(_Opr[(int)_Tokens[i].Value.Operator].Class){
        case ExprOpClass::Unary:
          switch(_Opr[(int)_Tokens[i].Value.Operator].Assoc){
            case ExprOpAssoc::Left:
              if(!_HasOperandOnLeft(i)){ _Tokens[i].Msg(554).Print(_Opr[(int)_Tokens[i].Value.Operator].Name); return false; }
              break;
            case ExprOpAssoc::Right:
              if(!_HasOperandOnRight(i)){ _Tokens[i].Msg(555).Print(_Opr[(int)_Tokens[i].Value.Operator].Name); return false; }
              break;
          }
          break;
        case ExprOpClass::Binary:
          if(!_HasOperandOnLeft(i)){ _Tokens[i].Msg(556).Print(_Opr[(int)_Tokens[i].Value.Operator].Name); return false; }
          if(!_HasOperandOnRight(i)){ _Tokens[i].Msg(557).Print(_Opr[(int)_Tokens[i].Value.Operator].Name); return false; }
          break;
      }
    }
  }

  //Return result
  return true;

}

//Tokenize flow operators in expression
bool Expression::_FlowOperatorParse(const Sentence& Stn,int CurrToken,int EndToken,Array<FlowOprAttr>& FlowOpr) const {

  //Variables
  int i;
  int ParLevel;
  int BraLevel;
  int ClyLevel;
  int FoundForIf;
  int FoundForDo;
  int FoundForRet;
  int FoundForEnd;
  int FoundArrOn;
  int FoundArrIdx;
  int FoundArrIf;
  int FoundArrAs;
  int FoundArrEnd;
  CpuLon Label;
  String KwOrder;

  //For expression
  if(Stn.Tokens[CurrToken].Id()==PrTokenId::Keyword && Stn.Tokens[CurrToken].Value.Kwd==PrKeyword::For){

    //Get flow label
    Label=_Md->GetFlowLabelGenerator(); 

    //Current token
    FlowOpr[CurrToken].Opr=ExprFlowOpr::ForBeg;
    FlowOpr[CurrToken].Label=Label;
    
    //Init levels and flags
    ParLevel=0;
    BraLevel=0;
    ClyLevel=0;
    FoundForIf=-1;
    FoundForDo=-1;
    FoundForRet=-1;
    FoundForEnd=-1;
    KwOrder="(";

    //Token loop
    for(i=CurrToken;i<=EndToken;i++){
      
      //Calculate levels (increase)
      if(Stn.Tokens[i].Id()==PrTokenId::Punctuator && Stn.Tokens[i].Value.Pnc==PrPunctuator::BegParen){ ParLevel++; }
      if(Stn.Tokens[i].Id()==PrTokenId::Punctuator && Stn.Tokens[i].Value.Pnc==PrPunctuator::BegBracket){ BraLevel++; }
      if(Stn.Tokens[i].Id()==PrTokenId::Punctuator && Stn.Tokens[i].Value.Pnc==PrPunctuator::BegCurly){ ClyLevel++; }

      //Detect keywords
      if(Stn.Tokens[i].Id()==PrTokenId::Keyword && Stn.Tokens[i].Value.Kwd==PrKeyword::If && ParLevel==1 && BraLevel==0 && ClyLevel==0){
        FlowOpr[i].Opr=ExprFlowOpr::ForIf;
        FlowOpr[i].Label=Label;
        FoundForIf=i;
        KwOrder+=" "+Stn.Tokens[i].Text();
      }
      else if(Stn.Tokens[i].Id()==PrTokenId::Keyword && Stn.Tokens[i].Value.Kwd==PrKeyword::Do && ParLevel==1 && BraLevel==0 && ClyLevel==0){
        FlowOpr[i].Opr=ExprFlowOpr::ForDo;
        FlowOpr[i].Label=Label;
        FoundForDo=i;
        KwOrder+=" "+Stn.Tokens[i].Text();
      }
      else if(Stn.Tokens[i].Id()==PrTokenId::Keyword && Stn.Tokens[i].Value.Kwd==PrKeyword::Return && ParLevel==1 && BraLevel==0 && ClyLevel==0){
        FlowOpr[i].Opr=ExprFlowOpr::ForRet;
        FlowOpr[i].Label=Label;
        FoundForRet=i;
        KwOrder+=" "+Stn.Tokens[i].Text();
      }
      else if(Stn.Tokens[i].Id()==PrTokenId::Punctuator && Stn.Tokens[i].Value.Pnc==PrPunctuator::EndParen && ParLevel==1 && BraLevel==0 && ClyLevel==0){
        FlowOpr[i].Opr=ExprFlowOpr::ForEnd;
        FlowOpr[i].Label=Label;
        FoundForEnd=i;
        KwOrder+=" "+Stn.Tokens[i].Text();
        break;
      }

      //Calculate levels (decrease)
      if(Stn.Tokens[i].Id()==PrTokenId::Punctuator && Stn.Tokens[i].Value.Pnc==PrPunctuator::EndParen){ ParLevel--; }
      if(Stn.Tokens[i].Id()==PrTokenId::Punctuator && Stn.Tokens[i].Value.Pnc==PrPunctuator::EndBracket){ BraLevel--; }
      if(Stn.Tokens[i].Id()==PrTokenId::Punctuator && Stn.Tokens[i].Value.Pnc==PrPunctuator::EndCurly){ ClyLevel--; }

    }

    //Syntax checks
    if(FoundForIf==-1){
      Stn.Tokens[CurrToken].Msg(451).Print();
      return false;
    }
    if(FoundForDo==-1){
      Stn.Tokens[CurrToken].Msg(452).Print();
      return false;
    }
    if(FoundForRet==-1){
      Stn.Tokens[CurrToken].Msg(453).Print();
      return false;
    }
    if(FoundForEnd==-1){
      Stn.Tokens[CurrToken].Msg(454).Print();
      return false;
    }

    //Check order of keywords
    if(KwOrder!="( if do return )"){
      Stn.Tokens[CurrToken].Msg(477).Print();
      return false;
    }

    //Debug message
    DebugMessage(DebugLevel::CmpExpression,"for() operator parsed:"
    " beg="+Stn.Tokens[CurrToken].Print()+":"+ToString(Stn.Tokens[CurrToken].ColNr+1)+
    " if="+Stn.Tokens[FoundForIf].Print()+":"+ToString(Stn.Tokens[FoundForIf].ColNr+1)+
    " do="+Stn.Tokens[FoundForDo].Print()+":"+ToString(Stn.Tokens[FoundForDo].ColNr+1)+
    " ret="+Stn.Tokens[FoundForRet].Print()+":"+ToString(Stn.Tokens[FoundForRet].ColNr+1)+
    " end="+Stn.Tokens[FoundForEnd].Print()+":"+ToString(Stn.Tokens[FoundForEnd].ColNr+1));

    //Increase flow label generator for next flow operator
    _Md->IncreaseFlowLabelGenerator(); 
  
  }

  //Array expression
  else if(Stn.Tokens[CurrToken].Id()==PrTokenId::Keyword && Stn.Tokens[CurrToken].Value.Kwd==PrKeyword::Array){

    //Get flow label
    Label=_Md->GetFlowLabelGenerator(); 

    //Current token
    FlowOpr[CurrToken].Opr=ExprFlowOpr::ArrBeg;
    FlowOpr[CurrToken].Label=Label;
    
    //Init levels and flags
    ParLevel=0;
    BraLevel=0;
    ClyLevel=0;
    FoundArrOn=-1;
    FoundArrIdx=-1;
    FoundArrIf=-1;
    FoundArrAs=-1;
    FoundArrEnd=-1;
    KwOrder="(";

    //Token loop
    for(i=CurrToken;i<=EndToken;i++){
      
      //Calculate levels (increase)
      if(Stn.Tokens[i].Id()==PrTokenId::Punctuator && Stn.Tokens[i].Value.Pnc==PrPunctuator::BegParen){ ParLevel++; }
      if(Stn.Tokens[i].Id()==PrTokenId::Punctuator && Stn.Tokens[i].Value.Pnc==PrPunctuator::BegBracket){ BraLevel++; }
      if(Stn.Tokens[i].Id()==PrTokenId::Punctuator && Stn.Tokens[i].Value.Pnc==PrPunctuator::BegCurly){ ClyLevel++; }

      //Detect keywords
      if(Stn.Tokens[i].Id()==PrTokenId::Keyword && Stn.Tokens[i].Value.Kwd==PrKeyword::On && ParLevel==1 && BraLevel==0 && ClyLevel==0){
        FlowOpr[i].Opr=(ExprFlowOpr)FLOW_ARR_ON;
        FlowOpr[i].Label=Label;
        FoundArrOn=i;
        KwOrder+=" "+Stn.Tokens[i].Text();
      }
      else if(Stn.Tokens[i].Id()==PrTokenId::Keyword && Stn.Tokens[i].Value.Kwd==PrKeyword::Index && ParLevel==1 && BraLevel==0 && ClyLevel==0){
        FlowOpr[i].Opr=(ExprFlowOpr)FLOW_ARR_IX;
        FlowOpr[i].Label=Label;
        FoundArrIdx=i;
        KwOrder+=" "+Stn.Tokens[i].Text();
      }
      else if(Stn.Tokens[i].Id()==PrTokenId::Keyword && Stn.Tokens[i].Value.Kwd==PrKeyword::If && ParLevel==1 && BraLevel==0 && ClyLevel==0){
        FlowOpr[i].Opr=(ExprFlowOpr)FLOW_ARR_IF;
        FlowOpr[i].Label=Label;
        FoundArrIf=i;
        KwOrder+=" "+Stn.Tokens[i].Text();
      }
      else if(Stn.Tokens[i].Id()==PrTokenId::Keyword && Stn.Tokens[i].Value.Kwd==PrKeyword::As && ParLevel==1 && BraLevel==0 && ClyLevel==0){
        FlowOpr[i].Opr=(ExprFlowOpr)FLOW_ARR_AS;
        FlowOpr[i].Label=Label;
        FoundArrAs=i;
        KwOrder+=" "+Stn.Tokens[i].Text();
      }
      else if(Stn.Tokens[i].Id()==PrTokenId::Punctuator && Stn.Tokens[i].Value.Pnc==PrPunctuator::EndParen && ParLevel==1 && BraLevel==0 && ClyLevel==0){
        FlowOpr[i].Opr=ExprFlowOpr::ArrEnd;
        FlowOpr[i].Label=Label;
        FoundArrEnd=i;
        KwOrder+=" "+Stn.Tokens[i].Text();
        break;
      }

      //Calculate levels (decrease)
      if(Stn.Tokens[i].Id()==PrTokenId::Punctuator && Stn.Tokens[i].Value.Pnc==PrPunctuator::EndParen){ ParLevel--; }
      if(Stn.Tokens[i].Id()==PrTokenId::Punctuator && Stn.Tokens[i].Value.Pnc==PrPunctuator::EndBracket){ BraLevel--; }
      if(Stn.Tokens[i].Id()==PrTokenId::Punctuator && Stn.Tokens[i].Value.Pnc==PrPunctuator::EndCurly){ ClyLevel--; }

    }

    //Syntax checks
    if(FoundArrOn==-1){
      Stn.Tokens[CurrToken].Msg(482).Print();
      return false;
    }
    if(FoundArrAs==-1){
      Stn.Tokens[CurrToken].Msg(483).Print();
      return false;
    }
    if(FoundArrEnd==-1){
      Stn.Tokens[CurrToken].Msg(484).Print();
      return false;
    }

    //Check order of keywords
    if(FoundArrIdx==-1 && FoundArrIf==-1){
      if(KwOrder!="( on as )"){
        Stn.Tokens[CurrToken].Msg(478).Print();
        return false;
      }
    }
    else if(FoundArrIdx!=-1 && FoundArrIf==-1){
      if(KwOrder!="( on index as )"){
        Stn.Tokens[CurrToken].Msg(479).Print();
        return false;
      }
    }
    else if(FoundArrIdx==-1 && FoundArrIf!=-1){
      if(KwOrder!="( on if as )"){
        Stn.Tokens[CurrToken].Msg(480).Print();
        return false;
      }
    }
    else if(FoundArrIdx!=-1 && FoundArrIf!=-1){
      if(KwOrder!="( on index if as )"){
        Stn.Tokens[CurrToken].Msg(481).Print();
        return false;
      }
    }

    //On becomes Ox if we have index variable
    if(FoundArrIdx!=-1){
      FlowOpr[FoundArrOn].Opr=(ExprFlowOpr)FLOW_ARR_OX;
    }

    //As becomes Asif if we have if condition
    if(FoundArrIf!=-1){
      FlowOpr[FoundArrAs].Opr=ExprFlowOpr::ArrAsif;
    }

    //Debug message
    DebugMessage(DebugLevel::CmpExpression,"array() operator parsed:"
    " beg="+Stn.Tokens[CurrToken].Print()+":"+ToString(Stn.Tokens[CurrToken].ColNr+1)+
    " on="+Stn.Tokens[FoundArrOn].Print()+":"+ToString(Stn.Tokens[FoundArrOn].ColNr+1)+
    (FoundArrIdx!=-1?" idx="+Stn.Tokens[FoundArrIdx].Print()+":"+ToString(Stn.Tokens[FoundArrIdx].ColNr+1):"")+
    (FoundArrIf!=-1?" if="+Stn.Tokens[FoundArrIf].Print()+":"+ToString(Stn.Tokens[FoundArrIf].ColNr+1):"")+
    " as="+Stn.Tokens[FoundArrAs].Print()+":"+ToString(Stn.Tokens[FoundArrAs].ColNr+1)+
    " end="+Stn.Tokens[FoundArrEnd].Print()+":"+ToString(Stn.Tokens[FoundArrEnd].ColNr+1));

    //Increase flow label generator for next flow operator
    _Md->IncreaseFlowLabelGenerator(); 
  
  }

  //Return value
  return true;

}

//Tokenize ternary operators in expression
bool Expression::_TernaryOperatorTokenize(const ScopeDef& Scope){

  //Variables
  int i;
  int ParLevel;
  int BraLevel;
  int ClyLevel;
  int FoundParLevel;
  int FoundBraLevel;
  int FoundClyLevel;
  bool Found;
  ExprToken Token;

  //Operator tokenization loop
  do{

    //Expression token loop
    ParLevel=0;
    BraLevel=0;
    ClyLevel=0;
    Found=false;
    FoundParLevel=0;
    FoundBraLevel=0;
    FoundClyLevel=0;
    for(i=0;i<_Tokens.Length();i++){
      
      //Calculate levels (increase)
      if(_Tokens[i].Id()==ExprTokenId::Delimiter && _Tokens[i].Value.Delimiter==ExprDelimiter::BegParen){ ParLevel++; }
      if(_Tokens[i].Id()==ExprTokenId::Delimiter && _Tokens[i].Value.Delimiter==ExprDelimiter::BegBracket){ BraLevel++; }
      if(_Tokens[i].Id()==ExprTokenId::Delimiter && _Tokens[i].Value.Delimiter==ExprDelimiter::BegCurly){ ClyLevel++; }

      //Ternary condition
      if(_Tokens[i].Id()==ExprTokenId::LowLevelOpr 
      && _Tokens[i].Value.LowLevelOpr==ExprLowLevelOpr::TernaryCond 
      && _Tokens[i].LabelSeed==-1
      && Found==false){
        
        //Check parethesys level is 1 at least
        if(ParLevel==0){
          _Tokens[i].Msg(138).Print();
          return false;
        }
        
        //Add label seed
        _Tokens[i].LabelSeed=_Md->GetLabelGenerator(); 
        Found=true;
        FoundParLevel=ParLevel;
        FoundBraLevel=BraLevel;
        FoundClyLevel=ClyLevel;

      }

      //Ternary mid
      else if(_Tokens[i].Id()==ExprTokenId::LowLevelOpr 
      && _Tokens[i].Value.LowLevelOpr==ExprLowLevelOpr::TernaryMid 
      && _Tokens[i].LabelSeed==-1 
      && Found==true 
      && ParLevel==FoundParLevel 
      && BraLevel==FoundBraLevel
      && ClyLevel==FoundClyLevel){
        _Tokens[i].LabelSeed=_Md->GetLabelGenerator(); 
      }

      //Ternary end
      else if(_Tokens[i].Id()==ExprTokenId::Delimiter 
      && _Tokens[i].Value.Delimiter==ExprDelimiter::EndParen 
      && Found==true 
      && ParLevel==FoundParLevel 
      && BraLevel==FoundBraLevel
      && ClyLevel==FoundClyLevel){
        Token=_Tokens[i];
        Token.Id(ExprTokenId::LowLevelOpr);
        Token.Value.LowLevelOpr=ExprLowLevelOpr::TernaryEnd;
        Token.LabelSeed=_Md->GetLabelGenerator(); 
        _Tokens.Insert(i,Token);
        break;
      }

      //Calculate levels (decrease)
      if(_Tokens[i].Id()==ExprTokenId::Delimiter && _Tokens[i].Value.Delimiter==ExprDelimiter::EndParen){ ParLevel--; }
      if(_Tokens[i].Id()==ExprTokenId::Delimiter && _Tokens[i].Value.Delimiter==ExprDelimiter::EndBracket){ BraLevel--; }
      if(_Tokens[i].Id()==ExprTokenId::Delimiter && _Tokens[i].Value.Delimiter==ExprDelimiter::EndCurly){ ClyLevel--; }

    }

    //Increase label generator
    if(Found){
      _Md->IncreaseLabelGenerator(); 
    }

    //Exit loop
    else{
      break;
    }

  }while(true);

  //Check syntax errors
  for(i=0;i<_Tokens.Length();i++){
    if(_Tokens[i].Id()==ExprTokenId::LowLevelOpr && _Tokens[i].Value.LowLevelOpr==ExprLowLevelOpr::TernaryCond && _Tokens[i].LabelSeed==-1){
      _Tokens[i].Msg(32).Print();
      return false;
    }
    if(_Tokens[i].Id()==ExprTokenId::LowLevelOpr && _Tokens[i].Value.LowLevelOpr==ExprLowLevelOpr::TernaryMid && _Tokens[i].LabelSeed==-1){
      _Tokens[i].Msg(139).Print();
      return false;
    }
  }

  //Return value
  return true;

}

//Infix notation to RPN notation conversion (Shunting yard algorithm)
bool Expression::_Infix2RPN(){

  //Variables
  int i;                                       //Input token index (i)
  int j;                                       //Secondary index
  ExprToken Token;                             //Expresion token (c)
  Stack<ExprToken> OprStack;                   //Shunting yart operator stack (Stack())
  ExprToken OprToken;                          //Used to record token into operator stack (Sc)
  bool Found;                                  //Found flag (pe)
  int BracketLevel;                            //Bracket level
  int ArrDimNr;                                //Number of array indexes
  Array<ExprToken> RPN;                        //Result

  //Token loop
  for(i=0;i<_Tokens.Length();i++){
       
    //Read one token expression
    Token=_Tokens[i];
       
    //Token is a function argument separator (comma)
    if(Token.Id()==ExprTokenId::Delimiter && Token.Value.Delimiter==ExprDelimiter::Comma){

        //Until the token at the top of the operator stack is a left parenthesis / bracket / curly brace,
        //pop operators off the operator stack onto the output queue.
        Found=false; 
        while(OprStack.Length()>0){
          OprToken=OprStack.Top();
          if((OprToken.Id()==ExprTokenId::Delimiter && OprToken.Value.Delimiter==ExprDelimiter::BegParen) 
          || (OprToken.Id()==ExprTokenId::Delimiter && OprToken.Value.Delimiter==ExprDelimiter::BegBracket)
          || (OprToken.Id()==ExprTokenId::Delimiter && OprToken.Value.Delimiter==ExprDelimiter::BegCurly)){ 
            Found=true;
            break; 
          }
          else{ 
            RPN.Add(OprStack.Pop());
          }
        
        }

        //If no left parentheses are encountered, either the separator was misplacedor parentheses were mismatched.
        if(!Found){ 
          Token.Msg(54).Print();
          return false;
        }

    }
   
    //If the token is an operator, op1, then:
    else if(Token.Id()==ExprTokenId::Operator){
        
      //While there is an operator token, o2, at the top of the stack
      //op1 is left-associative and its precedence is less than or equal to that of op2,
      //or op1 is right-associative and its precedence is less than that of op2,
      //Then, Pop o2 off the stack, onto the output queue;
      while(OprStack.Length()>0){ 
        OprToken=OprStack.Top();
        if(OprToken.Id()==ExprTokenId::Operator && (
        (_Opr[(int)Token.Value.Operator].Assoc==ExprOpAssoc::Left  && _Opr[(int)Token.Value.Operator].Prec<=_Opr[(int)OprToken.Value.Operator].Prec) || 
        (_Opr[(int)Token.Value.Operator].Assoc==ExprOpAssoc::Right && _Opr[(int)Token.Value.Operator].Prec< _Opr[(int)OprToken.Value.Operator].Prec))){ 
          RPN.Add(OprStack.Pop());
        }
        else if(OprToken.Id()==ExprTokenId::LowLevelOpr && (
        (_Opr[(int)Token.Value.Operator].Assoc==ExprOpAssoc::Left  && _Opr[(int)Token.Value.Operator].Prec<=_LowLevelOprPrecedence) || 
        (_Opr[(int)Token.Value.Operator].Assoc==ExprOpAssoc::Right && _Opr[(int)Token.Value.Operator].Prec< _LowLevelOprPrecedence))){ 
          RPN.Add(OprStack.Pop());
        }
        else if(OprToken.Id()==ExprTokenId::FlowOpr && (
        (_Opr[(int)Token.Value.Operator].Assoc==ExprOpAssoc::Left  && _Opr[(int)Token.Value.Operator].Prec<=_FlowOprPrecedence) || 
        (_Opr[(int)Token.Value.Operator].Assoc==ExprOpAssoc::Right && _Opr[(int)Token.Value.Operator].Prec< _FlowOprPrecedence))){ 
          RPN.Add(OprStack.Pop());
        }
        else{
          break;
        }
      }
     
      //Push operator onto the stack
      OprStack.Push(Token); 

    }
   
    //If the token is a low level operator (ternary operator) push to operator stack
    else if(Token.Id()==ExprTokenId::LowLevelOpr){
      while(OprStack.Length()>0){ 
        OprToken=OprStack.Top();
        if(OprToken.Id()==ExprTokenId::Operator && _LowLevelOprPrecedence<_Opr[(int)OprToken.Value.Operator].Prec){ 
          RPN.Add(OprStack.Pop());
        }
        else if(OprToken.Id()==ExprTokenId::LowLevelOpr && (int)OprToken.Value.LowLevelOpr<(int)OprToken.Value.LowLevelOpr){ 
          RPN.Add(OprStack.Pop());
        }
        else{
          break;
        }
      }
      RPN.Add(Token); 
    }
   
    //If the token is a flow operator push to operator stack
    else if(Token.Id()==ExprTokenId::FlowOpr){
      while(OprStack.Length()>0){ 
        OprToken=OprStack.Top();
        if(OprToken.Id()==ExprTokenId::Operator && _FlowOprPrecedence<_Opr[(int)OprToken.Value.Operator].Prec){ 
          RPN.Add(OprStack.Pop());
        }
        else if(OprToken.Id()==ExprTokenId::FlowOpr && (int)OprToken.Value.FlowOpr<(int)OprToken.Value.FlowOpr){ 
          RPN.Add(OprStack.Pop());
        }
        else{
          break;
        }
      }
      RPN.Add(Token); 
    }
   
    //If the token is a function/method or opening parenthesys, or complex value or opening curly brace then push it onto the stack
    else if((Token.Id()==ExprTokenId::Function) 
    ||      (Token.Id()==ExprTokenId::Method) 
    ||      (Token.Id()==ExprTokenId::Constructor) 
    ||      (Token.Id()==ExprTokenId::Complex) 
    ||      (Token.Id()==ExprTokenId::Delimiter && Token.Value.Delimiter==ExprDelimiter::BegParen) 
    ||      (Token.Id()==ExprTokenId::Delimiter && Token.Value.Delimiter==ExprDelimiter::BegCurly)){ 
      OprStack.Push(Token);
    }

    //If the token is a opening bracket, then push it onto the stack
    //Here we must do the same as for rest of operators considering that an opening bracket hast highest operator precedence and that it is left assoc.
    else if(Token.Id()==ExprTokenId::Delimiter && Token.Value.Delimiter==ExprDelimiter::BegBracket){ 

      //While there is an operator token, o2, at the top of the stack
      //op1 is left-associative and its precedence is less than or equal to that of op2,
      //or op1 is right-associative and its precedence is less than that of op2,
      //Then, Pop o2 off the stack, onto the output queue;
      while(OprStack.Length()>0){ 
        OprToken=OprStack.Top();
        if(OprToken.Id()==ExprTokenId::Operator && _MaxOperatorPrecedence<=_Opr[(int)OprToken.Value.Operator].Prec){
          RPN.Add(OprStack.Pop());
        }
        else{
          break;
        }
      }
     
      //Push operator onto the stack
      OprStack.Push(Token);

    }

    //If the token is a litteral value or a variable, then add it to the output queue
    else if(Token.Id()==ExprTokenId::Operand || Token.Id()==ExprTokenId::UndefVar){ 
      RPN.Add(Token);
    }

    //If the token is a field output to queue
    else if(Token.Id()==ExprTokenId::Field){
      RPN.Add(Token); 
    }
   
    //If the token is a right parenthesis:
    //Until the token at the top of the stack is a left parenthesis,
    //pop operators off the stack onto the output queue
    else if(Token.Id()==ExprTokenId::Delimiter && Token.Value.Delimiter==ExprDelimiter::EndParen){
      Found=false;
      while(OprStack.Length()>0){
        OprToken=OprStack.Top();
        if(OprToken.Id()==ExprTokenId::Delimiter && OprToken.Value.Delimiter==ExprDelimiter::BegParen){ 
          Found=true;
          break; 
        }
        else
        {
          RPN.Add(OprStack.Pop());
        }
      }
       
      //If the stack runs out without finding a left parenthesis, then there are mismatched parentheses.
      if(!Found){
        Token.Msg(55).Print();
        return false;
      }
     
      //Pop the left parenthesis from the stack, but not onto the output queue.
      OprStack.Pop();
     
      //If the token at the top of the stack is a function/member token, pop it onto the output queue.
      if(OprStack.Length()>0){
        OprToken=OprStack.Top();
        if(OprToken.Id()==ExprTokenId::Function || OprToken.Id()==ExprTokenId::Method || OprToken.Id()==ExprTokenId::Constructor){
          RPN.Add(OprStack.Pop());
        }
      }
    
    }

    //If the token is a right curly brace
    //Until the token at the top of the stack is a left curly brace,
    //pop operators off the stack onto the output queue
    else if(Token.Id()==ExprTokenId::Delimiter && Token.Value.Delimiter==ExprDelimiter::EndCurly){
      Found=false;
      while(OprStack.Length()>0){
        OprToken=OprStack.Top();
        if(OprToken.Id()==ExprTokenId::Delimiter && OprToken.Value.Delimiter==ExprDelimiter::BegCurly){ 
          Found=true;
          break; 
        }
        else
        {
          RPN.Add(OprStack.Pop());
        }
      }
       
      //If the stack runs out without finding a left parenthesis, then there are mismatched parentheses.
      if(!Found){
        Token.Msg(421).Print();
        return false;
      }
     
      //Pop the left parenthesis from the stack, but not onto the output queue.
      OprStack.Pop();
     
      //If the token at the top of the stack is a complex token, pop it onto the output queue.
      if(OprStack.Length()>0){
        OprToken=OprStack.Top();
        if(OprToken.Id()==ExprTokenId::Complex){
          RPN.Add(OprStack.Pop());
        }
        else{
          Token.Msg(422).Print();
          return false;
        }
      }
      else{
        Token.Msg(423).Print();
        return false;
      }
    
    }

    //If the token is a right bracket:
    //Until the token at the top of the stack is a left bracket,
    //pop operators off the stack onto the output queue
    else if(Token.Id()==ExprTokenId::Delimiter && Token.Value.Delimiter==ExprDelimiter::EndBracket){
      Found=false;
      while(OprStack.Length()>0){
        OprToken=OprStack.Top();
        if(OprToken.Id()==ExprTokenId::Delimiter && OprToken.Value.Delimiter==ExprDelimiter::BegBracket){
          Found=true;
          break;
        }
        else
        {
          RPN.Add(OprStack.Pop());
        }
      }
       
      //If the stack runs out without finding a left bracktet, then there are mismatched brackets.
      if(!Found){
        Token.Msg(56).Print();
        return false;
      }
     
      //Pop the left bracket from the stack, but not onto the output queue.
      OprStack.Pop();
     
      //Get number of subscripts
      ArrDimNr=1;
      BracketLevel=1;
      for(j=i;j>=0;j--){
        if(_Tokens[j].Id()==ExprTokenId::Delimiter && _Tokens[j].Value.Delimiter==ExprDelimiter::BegBracket && BracketLevel==0){ break; }
        if(     _Tokens[j].Id()==ExprTokenId::Delimiter && _Tokens[j].Value.Delimiter==ExprDelimiter::BegBracket){ BracketLevel++; }
        else if(_Tokens[j].Id()==ExprTokenId::Delimiter && _Tokens[j].Value.Delimiter==ExprDelimiter::EndBracket){ BracketLevel--; }
        if(_Tokens[j].Id()==ExprTokenId::Delimiter && _Tokens[j].Value.Delimiter==ExprDelimiter::Comma && BracketLevel==0){ ArrDimNr++; }
      }

      //Insert array subscript token
      Token.SetMdPointer(_Md);
      Token.Id(ExprTokenId::Subscript);
      Token.Value.DimNr=ArrDimNr;
      Token.SetPosition(_Tokens[i].SrcInfo());
      RPN.Add(Token);

    }
    
    //Unknown token error
    else{
      Token.Msg(57).Print(Token.Print());
      return false;
    }

  }
 
  //When there are no more tokens to read:
  //While there are still operator tokens in the stack:
  //Push the operators and check parenthesys / bracket missmatch
  while(OprStack.Length()>0){
    OprToken=OprStack.Top(); 
    if((OprToken.Id()==ExprTokenId::Delimiter && OprToken.Value.Delimiter==ExprDelimiter::BegParen)
    || (OprToken.Id()==ExprTokenId::Delimiter && OprToken.Value.Delimiter==ExprDelimiter::EndParen)){ 
      OprToken.Msg(55).Print();
      return false;
    }
    else if((OprToken.Id()==ExprTokenId::Delimiter && OprToken.Value.Delimiter==ExprDelimiter::BegBracket)
    ||      (OprToken.Id()==ExprTokenId::Delimiter && OprToken.Value.Delimiter==ExprDelimiter::EndBracket)){ 
      OprToken.Msg(56).Print();
      return false;
    }
    else if((OprToken.Id()==ExprTokenId::Delimiter && OprToken.Value.Delimiter==ExprDelimiter::BegCurly)
    ||      (OprToken.Id()==ExprTokenId::Delimiter && OprToken.Value.Delimiter==ExprDelimiter::EndCurly)){ 
      OprToken.Msg(424).Print();
      return false;
    }
    RPN.Add(OprStack.Pop());
  }

  //Set result
  _Tokens=RPN;

  //Debug message
  DebugMessage(DebugLevel::CmpExpression,"RPN ouput: "+Print());

  //Return code
  return(true);

}

//Inner block replication:
//Phase 1: Replicate all string and dyn array blocks
//Phase 2: Insert calculation rules for arrays and replicate recursively
bool Expression::_InnerBlockReplication(MasterData *Md,const ExprToken& Destin,const ExprToken& Source) const {

  //Debug message
  DebugMessage(DebugLevel::CmpInnerBlockRpl,"Started inner block replication (src="+Source.Print()+" dst="+Destin.Print()+")");
  
  //Init inner block replication mechanism: RPBEG <dest>,<source>
  if(!Md->Bin.AsmWriteCode(CpuInstCode::RPBEG,Destin.Asm(),Source.Asm())){ return false; }

  //Phase 1
  if(!_InnerBlockReplicationRecur(Md,1,0,0,Source.TypIndex(),Destin.ColNr())){ Md->Bin.SetAsmDefaultIndentation(); return(false); }
  
  //Phase 2
  if(!_InnerBlockReplicationRecur(Md,2,0,0,Source.TypIndex(),Destin.ColNr())){ Md->Bin.SetAsmDefaultIndentation(); return(false); }

  //Restore asm indentation
  Md->Bin.SetAsmDefaultIndentation();

  //Debug message
  DebugMessage(DebugLevel::CmpInnerBlockRpl,"Finished inner block replication");
  
  //Return code
  return(true);

}
  
//Inner block replication (recursive procedure):
//Phase 1: Replicate all string and dyn array blocks
//Phase 2: Insert calculation rules for arrays and replicate recursively
bool Expression::_InnerBlockReplicationRecur(MasterData *Md,int Phase,int RecurLevel,CpuWrd CumulOffset,int TypIndex,int ColNr) const{

  //Variables
  int i;

  //Debug message
  DebugMessage(DebugLevel::CmpInnerBlockRpl,"Entered recursion (phase="+ToString(Phase)+" level="+ToString(RecurLevel)+" cumuloffset="+ToString(CumulOffset)+" type="+Md->CannonicalTypeName(TypIndex)+")");

  //Phase 1: Block replication
  if(Phase==1){

    //Classes
    if(Md->Types[TypIndex].MstType==MasterType::Class && Md->Types[TypIndex].FieldLow!=-1 && Md->Types[TypIndex].FieldHigh!=-1){

      //Loop over members to execute replication on string and dyn array blocks at fixed positions
      i=-1;
      while((i=Md->FieldLoop(TypIndex,i))!=-1){
        
        //Avoid static fields since they are not inside the class
        if(!Md->Fields[i].IsStatic){

          //Execute replication for string blocks
          if(Md->Types[Md->Fields[i].TypIndex].MstType==MasterType::String){
            if(!Md->Bin.AsmWriteCode(CpuInstCode::RPSTR,Md->Bin.AsmLitWrd(Md->Fields[i].Offset+CumulOffset))){ return false; }
          }
    
          //Execute replication for dyn array blocks
          else if(Md->Types[Md->Fields[i].TypIndex].MstType==MasterType::DynArray){
            if(!Md->Bin.AsmWriteCode(CpuInstCode::RPARR,Md->Bin.AsmLitWrd(Md->Fields[i].Offset+CumulOffset))){ return false; }
          }
          
          //Execute replication for strings and arrays inside classes
          else if(Md->Types[Md->Fields[i].TypIndex].MstType==MasterType::Class){
            if(Md->HasInnerBlocks(Md->Fields[i].TypIndex)){
              if(!_InnerBlockReplicationRecur(Md,1,RecurLevel,Md->Fields[i].Offset+CumulOffset,Md->Fields[i].TypIndex,ColNr)){ return(false); }
            }
          }

        }

      }
    
    }

    //string blocks
    else if(Md->Types[TypIndex].MstType==MasterType::String){
      if(!Md->Bin.AsmWriteCode(CpuInstCode::RPSTR,Md->Bin.AsmLitWrd(CumulOffset))){ return false; }
    }

    //Dyn array blocks
    else if(Md->Types[TypIndex].MstType==MasterType::DynArray){
      if(!Md->Bin.AsmWriteCode(CpuInstCode::RPARR,Md->Bin.AsmLitWrd(CumulOffset))){ return false; }
    }
        
  }

  //Phase 2:Emit loops
  else if(Phase==2){

    //Class fields
    if(Md->Types[TypIndex].MstType==MasterType::Class && Md->Types[TypIndex].FieldLow!=-1 && Md->Types[TypIndex].FieldHigh!=-1){
      
      //Loop over members to execute replication
      i=-1;
      while((i=Md->FieldLoop(TypIndex,i))!=-1){
        
        //Do inner block replication for items with blocks inside, avoid static fields as they are instantiated only once
        if(!Md->Fields[i].IsStatic && Md->HasInnerBlocks(Md->Fields[i].TypIndex)){
  
          //Execute replication for fixed arrays
          if(Md->Types[Md->Fields[i].TypIndex].MstType==MasterType::FixArray){
            if(!Md->Bin.AsmWriteCode(CpuInstCode::RPLOF,Md->Bin.AsmLitWrd(Md->Fields[i].Offset+CumulOffset),Md->AsmAgx(Md->Fields[i].TypIndex))){ return false; }
            Md->Bin.SetAsmIndentation(Md->Bin.GetAsmDefaultIndentation()+(RecurLevel+1)*2);
            if(!_InnerBlockReplicationRecur(Md,1,RecurLevel+1,0,Md->Types[Md->Fields[i].TypIndex].ElemTypIndex,ColNr)){ return(false); }
            if(!_InnerBlockReplicationRecur(Md,2,RecurLevel+1,0,Md->Types[Md->Fields[i].TypIndex].ElemTypIndex,ColNr)){ return(false); }
            Md->Bin.SetAsmIndentation(Md->Bin.GetAsmDefaultIndentation()+(RecurLevel)*2);
            if(!Md->Bin.AsmWriteCode(CpuInstCode::RPEND)){ return false; }
          }
          
          //Execute replication for dynamic arrays
          else if(Md->Types[Md->Fields[i].TypIndex].MstType==MasterType::DynArray){
            if(!Md->Bin.AsmWriteCode(CpuInstCode::RPLOD,Md->Bin.AsmLitWrd(Md->Fields[i].Offset+CumulOffset))){ return false; }
            Md->Bin.SetAsmIndentation(Md->Bin.GetAsmDefaultIndentation()+(RecurLevel+1)*2);
            if(!_InnerBlockReplicationRecur(Md,1,RecurLevel+1,0,Md->Types[Md->Fields[i].TypIndex].ElemTypIndex,ColNr)){ return(false); }
            if(!_InnerBlockReplicationRecur(Md,2,RecurLevel+1,0,Md->Types[Md->Fields[i].TypIndex].ElemTypIndex,ColNr)){ return(false); }
            Md->Bin.SetAsmIndentation(Md->Bin.GetAsmDefaultIndentation()+(RecurLevel)*2);
            if(!Md->Bin.AsmWriteCode(CpuInstCode::RPEND)){ return false; }
          }
        }

      }
    
    }

    //Fixed arrays
    else if(Md->Types[TypIndex].MstType==MasterType::FixArray){
      if(Md->HasInnerBlocks(TypIndex)){
        if(!Md->Bin.AsmWriteCode(CpuInstCode::RPLOF,Md->Bin.AsmLitWrd(CumulOffset),Md->AsmAgx(TypIndex))){ return false; }
        Md->Bin.SetAsmIndentation(Md->Bin.GetAsmDefaultIndentation()+(RecurLevel+1)*2);
        if(!_InnerBlockReplicationRecur(Md,1,RecurLevel+1,0,Md->Types[TypIndex].ElemTypIndex,ColNr)){ return(false); }
        if(!_InnerBlockReplicationRecur(Md,2,RecurLevel+1,0,Md->Types[TypIndex].ElemTypIndex,ColNr)){ return(false); }
        Md->Bin.SetAsmIndentation(Md->Bin.GetAsmDefaultIndentation()+(RecurLevel)*2);
        if(!Md->Bin.AsmWriteCode(CpuInstCode::RPEND)){ return false; }
      }
    }
      
    //Dynamic arrays
    else if(Md->Types[TypIndex].MstType==MasterType::DynArray){
      if(Md->HasInnerBlocks(TypIndex)){
        if(!Md->Bin.AsmWriteCode(CpuInstCode::RPLOD,Md->Bin.AsmLitWrd(CumulOffset))){ return false; }
        Md->Bin.SetAsmIndentation(Md->Bin.GetAsmDefaultIndentation()+(RecurLevel+1)*2);
        if(!_InnerBlockReplicationRecur(Md,1,RecurLevel+1,0,Md->Types[TypIndex].ElemTypIndex,ColNr)){ return(false); }
        if(!_InnerBlockReplicationRecur(Md,2,RecurLevel+1,0,Md->Types[TypIndex].ElemTypIndex,ColNr)){ return(false); }
        Md->Bin.SetAsmIndentation(Md->Bin.GetAsmDefaultIndentation()+(RecurLevel)*2);
        if(!Md->Bin.AsmWriteCode(CpuInstCode::RPEND)){ return false; }
      }
      
    }

  }

  //Debug message
  DebugMessage(DebugLevel::CmpInnerBlockRpl,"Exited recursion (phase="+ToString(Phase)+" level="+ToString(RecurLevel)+")");

  //Return code
  return(true);

}

//Inner block initialization:
//Phase 1: Replicate all string and dyn array blocks
//Phase 2: Insert calculation rules for arrays and replicate recursively
bool Expression::_InnerBlockInitialization(MasterData *Md,const ExprToken& Destin) const {

  //Debug message
  DebugMessage(DebugLevel::CmpInnerBlockInit,"Started inner block initialization (dst="+Destin.Print()+")");
  
  //Init inner block initialization mechanism: BIBEG <dest>
  if(!Md->Bin.AsmWriteCode(CpuInstCode::BIBEG,Destin.Asm())){ return false; }

  //Phase 1
  if(!_InnerBlockInitializationRecur(Md,1,0,0,Destin.TypIndex(),Destin.ColNr())){ Md->Bin.SetAsmDefaultIndentation(); return(false); }
  
  //Phase 2
  if(!_InnerBlockInitializationRecur(Md,2,0,0,Destin.TypIndex(),Destin.ColNr())){ Md->Bin.SetAsmDefaultIndentation(); return(false); }

  //Restore asm indentation
  Md->Bin.SetAsmDefaultIndentation();

  //Debug message
  DebugMessage(DebugLevel::CmpInnerBlockInit,"Finished inner block initialization");
  
  //Return code
  return(true);

}
  
//Inner block initialization (recursive procedure):
//Phase 1: Init all string and dyn array blocks
//Phase 2: Insert calculation rules for arrays and init recursively
bool Expression::_InnerBlockInitializationRecur(MasterData *Md,int Phase,int RecurLevel,CpuWrd CumulOffset,int TypIndex,int ColNr) const{

  //Variables
  int i;
  AsmArg DimNrArg;
  AsmArg CellSizeArg;

  //Debug message
  DebugMessage(DebugLevel::CmpInnerBlockInit,"Entered recursion (phase="+ToString(Phase)+" level="+ToString(RecurLevel)+" cumuloffset="+ToString(CumulOffset)+" type="+Md->CannonicalTypeName(TypIndex)+")");

  //Phase 1: Block initialization
  if(Phase==1){

    //Classes
    if(Md->Types[TypIndex].MstType==MasterType::Class && Md->Types[TypIndex].FieldLow!=-1 && Md->Types[TypIndex].FieldHigh!=-1){

      //Loop over members to execute initialization on string and dyn array blocks at fixed positions
      i=-1;
      while((i=Md->FieldLoop(TypIndex,i))!=-1){
        
        //Skip static fields
        if(!Md->Fields[i].IsStatic){

          //Execute initialization for string blocks
          if(Md->Types[Md->Fields[i].TypIndex].MstType==MasterType::String){
            if(!Md->Bin.AsmWriteCode(CpuInstCode::BISTR,Md->Bin.AsmLitWrd(Md->Fields[i].Offset+CumulOffset))){ return false; }
          }
    
          //Execute initialization for dyn array blocks
          else if(Md->Types[Md->Fields[i].TypIndex].MstType==MasterType::DynArray){
            DimNrArg=Md->Bin.AsmLitChr(Md->Types[Md->Fields[i].TypIndex].DimNr);
            CellSizeArg=Md->Bin.AsmLitWrd(Md->Types[Md->Types[Md->Fields[i].TypIndex].ElemTypIndex].Length);
            if(!Md->Bin.AsmWriteCode(CpuInstCode::BIARR,Md->Bin.AsmLitWrd(Md->Fields[i].Offset+CumulOffset),DimNrArg,CellSizeArg)){ return false; }
          }
          
          //Execute initialization for strings and arrays inside classes
          else if(Md->Types[Md->Fields[i].TypIndex].MstType==MasterType::Class){
            if(Md->HasInnerBlocks(Md->Fields[i].TypIndex)){
              if(!_InnerBlockInitializationRecur(Md,1,RecurLevel,Md->Fields[i].Offset+CumulOffset,Md->Fields[i].TypIndex,ColNr)){ return(false); }
            }
          }

        }   

      }
    
    }

    //string blocks
    else if(Md->Types[TypIndex].MstType==MasterType::String){
      if(!Md->Bin.AsmWriteCode(CpuInstCode::BISTR,Md->Bin.AsmLitWrd(CumulOffset))){ return false; }
    }

    //Dyn array blocks
    else if(Md->Types[TypIndex].MstType==MasterType::DynArray){
      DimNrArg=Md->Bin.AsmLitChr(Md->Types[TypIndex].DimNr);
      CellSizeArg=Md->Bin.AsmLitWrd(Md->Types[Md->Types[TypIndex].ElemTypIndex].Length);
      if(!Md->Bin.AsmWriteCode(CpuInstCode::BIARR,Md->Bin.AsmLitWrd(CumulOffset),DimNrArg,CellSizeArg)){ return false; }
    }
        
  }

  //Phase 2:Emit loops
  else if(Phase==2){

    //Class fields
    if(Md->Types[TypIndex].MstType==MasterType::Class && Md->Types[TypIndex].FieldLow!=-1 && Md->Types[TypIndex].FieldHigh!=-1){
      
      //Loop over members to execute initialization
      i=-1;
      while((i=Md->FieldLoop(TypIndex,i))!=-1){
        
        //Do inner block initialization for items with blocks inside
        if(!Md->Fields[i].IsStatic && Md->HasInnerBlocks(Md->Fields[i].TypIndex)){
  
          //Execute initialization for fixed arrays
          if(Md->Types[Md->Fields[i].TypIndex].MstType==MasterType::FixArray){
            if(!Md->Bin.AsmWriteCode(CpuInstCode::BILOF,Md->Bin.AsmLitWrd(Md->Fields[i].Offset+CumulOffset),Md->AsmAgx(Md->Fields[i].TypIndex))){ return false; }
            Md->Bin.SetAsmIndentation(Md->Bin.GetAsmDefaultIndentation()+(RecurLevel+1)*2);
            if(!_InnerBlockInitializationRecur(Md,1,RecurLevel+1,0,Md->Types[Md->Fields[i].TypIndex].ElemTypIndex,ColNr)){ return(false); }
            if(!_InnerBlockInitializationRecur(Md,2,RecurLevel+1,0,Md->Types[Md->Fields[i].TypIndex].ElemTypIndex,ColNr)){ return(false); }
            Md->Bin.SetAsmIndentation(Md->Bin.GetAsmDefaultIndentation()+(RecurLevel)*2);
            if(!Md->Bin.AsmWriteCode(CpuInstCode::BIEND)){ return false; }
          }
          
        }

      }
    
    }

    //Fixed arrays
    else if(Md->Types[TypIndex].MstType==MasterType::FixArray){
      if(Md->HasInnerBlocks(TypIndex)){
        if(!Md->Bin.AsmWriteCode(CpuInstCode::BILOF,Md->Bin.AsmLitWrd(CumulOffset),Md->AsmAgx(TypIndex))){ return false; }
        Md->Bin.SetAsmIndentation(Md->Bin.GetAsmDefaultIndentation()+(RecurLevel+1)*2);
        if(!_InnerBlockInitializationRecur(Md,1,RecurLevel+1,0,Md->Types[TypIndex].ElemTypIndex,ColNr)){ return(false); }
        if(!_InnerBlockInitializationRecur(Md,2,RecurLevel+1,0,Md->Types[TypIndex].ElemTypIndex,ColNr)){ return(false); }
        Md->Bin.SetAsmIndentation(Md->Bin.GetAsmDefaultIndentation()+(RecurLevel)*2);
        if(!Md->Bin.AsmWriteCode(CpuInstCode::BIEND)){ return false; }
      }
    }
      
  }

  //Debug message
  DebugMessage(DebugLevel::CmpInnerBlockInit,"Exited recursion (phase="+ToString(Phase)+" level="+ToString(RecurLevel)+")");

  //Return code
  return(true);

}

//Get all static field indexes of class and inner classes
Array<int> Expression::_GetStaticFields(MasterData *Md,int TypIndex) const {

  //Variables
  int i,j;
  int WorkTypIndex;
  Array<int> StaticFields;
  Array<int> InnerStaticFields;

  //Check if we are a classor arrays of classes
  if(Md->Types[TypIndex].MstType==MasterType::Class 
  && Md->Types[TypIndex].FieldLow!=-1 
  && Md->Types[TypIndex].FieldHigh!=-1){
    WorkTypIndex=TypIndex;
  }
  else if(Md->Types[TypIndex].MstType==MasterType::FixArray
  && Md->Types[Md->Types[TypIndex].ElemTypIndex].MstType==MasterType::Class 
  && Md->Types[Md->Types[TypIndex].ElemTypIndex].FieldLow!=-1 
  && Md->Types[Md->Types[TypIndex].ElemTypIndex].FieldHigh!=-1){
    WorkTypIndex=Md->Types[TypIndex].ElemTypIndex;
  }
  else if(Md->Types[TypIndex].MstType==MasterType::DynArray
  && Md->Types[Md->Types[TypIndex].ElemTypIndex].MstType==MasterType::Class 
  && Md->Types[Md->Types[TypIndex].ElemTypIndex].FieldLow!=-1 
  && Md->Types[Md->Types[TypIndex].ElemTypIndex].FieldHigh!=-1){
    WorkTypIndex=Md->Types[TypIndex].ElemTypIndex;
  }
  else{
    return StaticFields;
  }

  //Loop over members to find static fields
  i=-1;
  while((i=Md->FieldLoop(WorkTypIndex,i))!=-1){
    if(Md->Fields[i].IsStatic){
      if(StaticFields.Search(i)==-1){
        StaticFields.Add(i);
      }
    }
    else if(Md->Types[Md->Fields[i].TypIndex].MstType==MasterType::Class
    || Md->Types[Md->Fields[i].TypIndex].MstType==MasterType::FixArray
    || Md->Types[Md->Fields[i].TypIndex].MstType==MasterType::DynArray){
      InnerStaticFields=_GetStaticFields(Md,Md->Fields[i].TypIndex);
      for(j=0;j<InnerStaticFields.Length();j++){
        if(StaticFields.Search(InnerStaticFields[j])==-1){
          StaticFields.Add(InnerStaticFields[j]);
        }
      }
    }
  }

  //Return static fields
  return StaticFields;

}

//Data type promotion wrapper for simplified call and computed flag
bool Expression::CompileDataTypePromotion(MasterData *Md,const ScopeDef& Scope,CpuLon CodeBlockId,ExprToken& Opnd,MasterType ToMstType,bool& Computed){
  _Md=Md;
  ExprOperCaseRule CaseRule;
  CaseRule.PromMode=ExprPromMode::ToResult;
  CaseRule.MstResult=ToMstType;
  return _CompileDataTypePromotion(Scope,CodeBlockId,Opnd,CaseRule,(MasterType)0,Computed);
}

//Data type promotion wrapper for simplified call
bool Expression::_CompileDataTypePromotion(const ScopeDef& Scope,CpuLon CodeBlockId,ExprToken& Opnd,MasterType ToMstType) const {
  bool Computed;
  ExprOperCaseRule CaseRule;
  CaseRule.PromMode=ExprPromMode::ToResult;
  CaseRule.MstResult=ToMstType;
  return _CompileDataTypePromotion(Scope,CodeBlockId,Opnd,CaseRule,(MasterType)0,Computed);
}

//Promote operand
bool Expression::_CompileDataTypePromotion(const ScopeDef& Scope,CpuLon CodeBlockId,ExprToken& Opnd,ExprOperCaseRule& CaseRule,MasterType MstMaxType) const {
  bool Computed;
  return _CompileDataTypePromotion(Scope,CodeBlockId,Opnd,CaseRule,MstMaxType,Computed);
}

//Promote operand
bool Expression::_CompileDataTypePromotion(const ScopeDef& Scope,CpuLon CodeBlockId,ExprToken& Opnd,ExprOperCaseRule& CaseRule,MasterType MstMaxType,bool& Computed) const {

  //Variables
  MasterType ToMstType;
  ExprToken Token;

  //Switch on promotion mode
  switch(CaseRule.PromMode){
    case ExprPromMode::ToResult:  ToMstType=CaseRule.MstResult; break;
    case ExprPromMode::ToMaximun: ToMstType=MstMaxType; break;
    case ExprPromMode::ToOther:   ToMstType=CaseRule.MstPromType; break;
  }

  //Compute data type promotion whenever possible
  if(Opnd.IsComputableOperand() 
  && (ToMstType==MasterType::Boolean || ToMstType==MasterType::Char || ToMstType==MasterType::Short 
  ||  ToMstType==MasterType::Integer || ToMstType==MasterType::Long || ToMstType==MasterType::Float)){
    if(!_ComputeDataTypePromotion(Scope,Opnd,CaseRule,MstMaxType)){ return false; }
    Computed=true;
  }

  //Compile data type promotion
  else{

    //Not computed operation
    Computed=false;
    
    //No promotion needed if data type is the same
    if(Opnd.MstType()==ToMstType){ return true; }

    //Create new token
    if(_Md->IsMasterAtomic(ToMstType)){
      if(!Token.NewVar(_Md,Scope,CodeBlockId,ToMstType,Opnd.SrcInfo(),TempVarKind::Promotion)){ return false; }
    }
    else{
      Opnd.Msg(521).Print(MasterTypeName(Opnd.MstType()),MasterTypeName(ToMstType));
      return false;
    }

    //Emit conversion asm instruction
    if(     Opnd.MstType()==MasterType::Char    && ToMstType==MasterType::Short   ){ _Md->Bin.AsmWriteCode(CpuInstCode::CH2SH,Token.Asm(),Opnd.Asm()); }
    else if(Opnd.MstType()==MasterType::Char    && ToMstType==MasterType::Integer ){ _Md->Bin.AsmWriteCode(CpuInstCode::CH2IN,Token.Asm(),Opnd.Asm()); }
    else if(Opnd.MstType()==MasterType::Char    && ToMstType==MasterType::Long    ){ _Md->Bin.AsmWriteCode(CpuInstCode::CH2LO,Token.Asm(),Opnd.Asm()); }
    else if(Opnd.MstType()==MasterType::Char    && ToMstType==MasterType::Float   ){ _Md->Bin.AsmWriteCode(CpuInstCode::CH2FL,Token.Asm(),Opnd.Asm()); }
    else if(Opnd.MstType()==MasterType::Char    && ToMstType==MasterType::String  ){ _Md->Bin.AsmWriteCode(CpuInstCode::CH2ST,Token.Asm(),Opnd.Asm()); }
    else if(Opnd.MstType()==MasterType::Short   && ToMstType==MasterType::Integer ){ _Md->Bin.AsmWriteCode(CpuInstCode::SH2IN,Token.Asm(),Opnd.Asm()); }
    else if(Opnd.MstType()==MasterType::Short   && ToMstType==MasterType::Long    ){ _Md->Bin.AsmWriteCode(CpuInstCode::SH2LO,Token.Asm(),Opnd.Asm()); }
    else if(Opnd.MstType()==MasterType::Short   && ToMstType==MasterType::Float   ){ _Md->Bin.AsmWriteCode(CpuInstCode::SH2FL,Token.Asm(),Opnd.Asm()); }
    else if(Opnd.MstType()==MasterType::Integer && ToMstType==MasterType::Long    ){ _Md->Bin.AsmWriteCode(CpuInstCode::IN2LO,Token.Asm(),Opnd.Asm()); }
    else if(Opnd.MstType()==MasterType::Integer && ToMstType==MasterType::Float   ){ _Md->Bin.AsmWriteCode(CpuInstCode::IN2FL,Token.Asm(),Opnd.Asm()); }
    else if(Opnd.MstType()==MasterType::Long    && ToMstType==MasterType::Float   ){ _Md->Bin.AsmWriteCode(CpuInstCode::LO2FL,Token.Asm(),Opnd.Asm()); }
    else{
      Opnd.Msg(126).Print(MasterTypeName(Opnd.MstType()),MasterTypeName(ToMstType));
      return false;
    }

    //Release previous token and take new one
    //Token.Release(); 
    Opnd.Release(); 
    Opnd=Token;
  
  }
  
  //Return code
  return true;

}

//Pre-Calculation
//- Check operand lvalue ness
//- Check operand master types
//- Promote operands
//- Get result òperand
bool Expression::_PrepareCompileOperation(const ScopeDef& Scope,CpuLon CodeBlockId,const ExprToken& Opr,ExprToken& Opnd1,ExprToken& Opnd2, ExprToken& Result,bool ForceIsResultFirst) const {

  //Variables
  int i;
  int Opx;
  int CaseRule;
  bool Match[2];
  MasterType MstMaxType;
  
  //Get operand index
  Opx=(int)Opr.Value.Operator;

  //Check lvalue-ness of operands
  if(_Opr[Opx].LValueMandatory[0] && !Opnd1.IsLValue()){
    Opr.Msg(65).Print("left",_Opr[Opx].Name.Trim());
    return false;
  }
  if(_Opr[Opx].Class==ExprOpClass::Binary){
    if(_Opr[Opx].LValueMandatory[1] && !Opnd2.IsLValue()){
      Opr.Msg(65).Print("right",_Opr[Opx].Name.Trim());
      return false;
    }
  }

  //Check modification of constant operand
  //(We avoid this check when we are inside the delayed init routine, as this routine can contain inialization of local constants)
  if(_Opr[Opx].IsResultFirst && Opnd1.IsConst 
  && ((Scope.Kind!=ScopeKind::Local)
  ||  (Scope.Kind==ScopeKind::Local && _Md->Functions[Scope.FunIndex].Name!=_Md->GetDelayedInitFunctionName(_Md->Modules[Scope.ModIndex].Name)))){
    Opr.Msg(76).Print(_Opr[Opx].Name.Trim());
    return false;
  }

  //Check usage of non initialized operands
  //Temporary variables are not checked, we assume compiler does the right thing with them
  if(_Opr[Opx].CheckInitialized[0] && !Opnd1.IsInitialized()){
    Opnd1.Msg(131).Print(Opnd1.Name());
    return false;
  }
  if(_Opr[Opx].Class==ExprOpClass::Binary){
    if(_Opr[Opx].CheckInitialized[1] && !Opnd2.IsInitialized()){
      Opnd2.Msg(131).Print(Opnd2.Name());
      return false;
    }
  }

  //Check and find operand case rule
  //We skip the typecast operator here since it does not have return type determined
  //We also check if operator is binary in case we add in future a binary operator without determined return type
  //It will fail since case rule is necessary for data promotions
  if(_Opr[Opx].IsRetTypeDeterm){ 
    CaseRule=-1;
    for(i=0;i<_OperCaseRuleNr;i++){
      if(Opx==(int)_OperCaseRule[i].Operator){
        Match[0]=((1<<(int)Opnd1.MstType())&_OperCaseRule[i].OperandCases[0]?true:false);
        if(_Opr[Opx].Class==ExprOpClass::Binary){
          Match[1]=((1<<(int)Opnd2.MstType())&_OperCaseRule[i].OperandCases[1]?true:false);
        }
        switch(_Opr[Opx].Class){
          case ExprOpClass::Unary : if(Match[0]){ CaseRule=i; break; } break;
          case ExprOpClass::Binary: if(Match[0] && Match[1]){ CaseRule=i; break; } break;
        }
      }
      if(CaseRule!=-1){ break; }
    }
    if(CaseRule==-1){
      switch(_Opr[Opx].Class){
        case ExprOpClass::Unary : 
          Opr.Msg(66).Print(_Opr[Opx].Name.Trim(),MasterTypeName(Opnd1.MstType())); 
          break;
        case ExprOpClass::Binary: 
          if((ExprOperator)Opx==ExprOperator::Assign && Opnd1.IsFuncResult()){
            Opnd2.Msg(285).Print(MasterTypeName(Opnd2.MstType()),MasterTypeName(Opnd1.MstType())); 
          }
          else{
            Opr.Msg(67).Print(_Opr[Opx].Name.Trim(),MasterTypeName(Opnd1.MstType()),MasterTypeName(Opnd2.MstType())); 
          }
          break;
      }
      return false;
    }
  }

  //Get result operand
  //On typecasts, when they cast to array a constant is returned (as no inner block is performed)
  if(!_Opr[Opx].IsResultFirst && !_Opr[Opx].IsResultSecond && !ForceIsResultFirst){
    if(_Opr[Opx].IsRetTypeDeterm){
      if(!Result.NewVar(_Md,Scope,CodeBlockId,_OperCaseRule[CaseRule].MstResult,Opr.SrcInfo())){ return false; }
    }
    else{
      if(_Md->Types[Opr.CastTypIndex].MstType==MasterType::FixArray
      || _Md->Types[Opr.CastTypIndex].MstType==MasterType::DynArray){
        if(!Result.NewConst(_Md,Scope,CodeBlockId,Opr.CastTypIndex,Opr.SrcInfo())){ return false; } 
      }
      else{
        if(!Result.NewVar(_Md,Scope,CodeBlockId,Opr.CastTypIndex,Opr.SrcInfo())){ return false; } 
      }
    }
  }
  
  //Complete data type promotions
  if(_Opr[Opx].Class==ExprOpClass::Binary && _Opr[Opx].IsRetTypeDeterm){
    MstMaxType=(Opnd1.MstType()>Opnd2.MstType()?Opnd1.MstType():Opnd2.MstType());
    if(_OperCaseRule[CaseRule].Promotion[0]){ 
      if(!_CompileDataTypePromotion(Scope,CodeBlockId,Opnd1,_OperCaseRule[CaseRule],MstMaxType)){ return false; }
    }
    if(_OperCaseRule[CaseRule].Promotion[1]){
      if(!_CompileDataTypePromotion(Scope,CodeBlockId,Opnd2,_OperCaseRule[CaseRule],MstMaxType)){ return false; }
    }
  }

  //Return code
  return true;

}

//Complex value call
bool Expression::_ComplexValueCall(const ScopeDef& Scope,CpuLon CodeBlockId,int CurrToken,Stack<ExprToken>& OpndStack,ExprToken& Result){

  //Vaariables
  int i,j;
  int OperandNr;
  int ElemTypIndex;
  int VarIndex;
  int FirstNonStatic;
  int LastNonStatic;
  bool EmptyClass;
  String VarName;
  CpuChr DimNr;
  CpuWrd CellSize;
  ExprToken CmpxToken;
  ExprToken OpndToken;
  ExprToken Reference;
  ExprToken StaticFld;
  AsmArg GeomIndex;
  Array<ExprToken> OpndTokens;

  //Get function token
  CmpxToken=_Tokens[CurrToken];

  //Calculate number of operands to get from stack, field indexes and element type index
  if(_Md->Types[CmpxToken.Value.ComplexTypIndex].MstType==MasterType::Class){
    if(_Md->Types[CmpxToken.Value.ComplexTypIndex].FieldLow!=-1 && _Md->Types[CmpxToken.Value.ComplexTypIndex].FieldHigh!=-1){
      OperandNr=_Md->FieldCount(CmpxToken.Value.ComplexTypIndex);
      EmptyClass=false;
    }
    else{
      OperandNr=0;
      EmptyClass=true;
    }
  }
  else if(_Md->Types[CmpxToken.Value.ComplexTypIndex].MstType==MasterType::FixArray){
    ElemTypIndex=_Md->Types[CmpxToken.Value.ComplexTypIndex].ElemTypIndex;
    OperandNr=_Md->ArrayElements(_Md->Types[CmpxToken.Value.ComplexTypIndex].DimNr,_Md->Dimensions[_Md->Types[CmpxToken.Value.ComplexTypIndex].DimIndex].DimSize);
  }
  else if(_Md->Types[CmpxToken.Value.ComplexTypIndex].MstType==MasterType::DynArray){
    ElemTypIndex=_Md->Types[CmpxToken.Value.ComplexTypIndex].ElemTypIndex;
    OperandNr=_Md->ArrayElements(_Md->Types[CmpxToken.Value.ComplexTypIndex].DimNr,CmpxToken.DimSize);
  }
  else{
    CmpxToken.Msg(425).Print();
    return false;
  }

  //Only for classes check all fields are visible and calculate first and last non-static class fields
  if(_Md->Types[CmpxToken.Value.ComplexTypIndex].MstType==MasterType::Class && !EmptyClass){
    
    //Check all class fields are visible from scope
    if(Scope.Kind==ScopeKind::Local && !_Md->AreAllFieldsVisible(Scope,CmpxToken.Value.ComplexTypIndex)){
      CmpxToken.Msg(99).Print(_Md->CannonicalTypeName(CmpxToken.Value.ComplexTypIndex),_Md->ScopeDescription(Scope));
      return false;
    }

    //Calculate first and last non-static class fields
    FirstNonStatic=-1;
    LastNonStatic=-1;
    i=-1;
    while((i=_Md->FieldLoop(CmpxToken.Value.ComplexTypIndex,i))!=-1){
      if(!_Md->Fields[i].IsStatic){
        if(FirstNonStatic==-1){ FirstNonStatic=i; }
        LastNonStatic=i;
      }
    }
  }

  //Check stack has enough operands
  if(OpndStack.Length()<OperandNr){
    CmpxToken.Msg(426).Print(_Md->CannonicalTypeName(CmpxToken.Value.ComplexTypIndex),ToString(OperandNr),ToString(OpndStack.Length()));
    return false;
  }

  //Get parameters from stack
  for(i=0;i<OperandNr;i++){
    OpndToken=OpndStack[OpndStack.Length()-OperandNr+i];
    OpndTokens.Add(OpndToken);
  }
  OpndStack.Pop(OperandNr);

  //Check we do not have undefined variables or void results as operands
  for(i=0;i<OpndTokens.Length();i++){
    if(OpndTokens[i].Id()==ExprTokenId::UndefVar){
      OpndTokens[i].Msg(446).Print(OpndTokens[i].Value.VarName);
      return false;
    }
    else if(OpndTokens[i].Id()==ExprTokenId::VoidRes){
      OpndTokens[i].Msg(111).Print(OpndTokens[i].Value.VoidFuncName);
      return false;
    }
  }

  //Create result token
  if(!Result.NewVar(_Md,Scope,CodeBlockId,CmpxToken.Value.ComplexTypIndex,CmpxToken.SrcInfo())){ return false; }

  //Create work reference for element assignments
  //On classes reference is not created (as it will not be used) if class only contains static fields
  if(_Md->Types[CmpxToken.Value.ComplexTypIndex].MstType==MasterType::Class){
    if(!EmptyClass && FirstNonStatic!=-1 && LastNonStatic!=-1){
      if(!Reference.NewInd(_Md,Scope,CodeBlockId,CmpxToken.Value.ComplexTypIndex,false,CmpxToken.SrcInfo())){ return false; }
    }
  }
  else if(_Md->Types[CmpxToken.Value.ComplexTypIndex].MstType==MasterType::FixArray
  ||      _Md->Types[CmpxToken.Value.ComplexTypIndex].MstType==MasterType::DynArray){
    if(OperandNr!=0){
      if(!Reference.NewInd(_Md,Scope,CodeBlockId,ElemTypIndex,false,CmpxToken.SrcInfo())){ return false; }
    }
  }

  //Release parameter tokens here to avoid reusing a parameter as function result
  for(i=0;i<OpndTokens.Length();i++){
    OpndTokens[i].Release();
  }
  
  //Set operands as used in source
  for(i=0;i<OpndTokens.Length();i++){
    OpndTokens[i].SetSourceUsed(Scope,true);
  }

  //Operand check loop for classes
  if(_Md->Types[CmpxToken.Value.ComplexTypIndex].MstType==MasterType::Class){

    //Do nothing if class is empty
    if(!EmptyClass){

      //Field loop
      i=-1; j=0;
      while((i=_Md->FieldLoop(CmpxToken.Value.ComplexTypIndex,i))!=-1){
        
        //Check parameter is initialized
        if(!OpndTokens[j].IsInitialized()){
          OpndTokens[j].Msg(427).Print(_Md->Variables[OpndTokens[j].Value.VarIndex].Name);
          return false;
        } 

        //Automatic data type promotions
        if(_IsDataTypePromotionAutomatic(OpndTokens[j].MstType(),_Md->Types[_Md->Fields[i].TypIndex].MstType)){
          if(!_CompileDataTypePromotion(Scope,CodeBlockId,OpndTokens[j],_Md->Types[_Md->Fields[i].TypIndex].MstType)){ return false; }
        }
    
        //Check data type of operand matches field definition
        //(We only send error when master types do not match and they are not atomic or they are not equivalent arrays)
        if(_Md->WordTypeFilter(OpndTokens[j].TypIndex(),true)!=_Md->WordTypeFilter(_Md->Fields[i].TypIndex,true)
        && _Md->ConvertibleTypeName(_Md->WordTypeFilter(OpndTokens[j].TypIndex(),true))!=_Md->ConvertibleTypeName(_Md->WordTypeFilter(_Md->Fields[i].TypIndex,true))){
          if(!((OpndTokens[j].MstType()==_Md->Types[_Md->Fields[i].TypIndex].MstType 
          && OpndTokens[j].IsMasterAtomic() && _Md->IsMasterAtomic(_Md->Fields[i].TypIndex)) 
          || _Md->EquivalentArrays(OpndTokens[j].TypIndex(),_Md->Fields[i].TypIndex))){
            OpndTokens[j].Msg(428).Print(_Md->Fields[i].Name,_Md->CannonicalTypeName(CmpxToken.Value.ComplexTypIndex),_Md->CannonicalTypeName(_Md->Fields[i].TypIndex),_Md->CannonicalTypeName(OpndTokens[j].TypIndex()));
            return false;
          }
        }

        //Increase token index
        j++;

      }

    }

  }
  
  //Operand check loop for arrays
  else if(_Md->Types[CmpxToken.Value.ComplexTypIndex].MstType==MasterType::FixArray
  ||      _Md->Types[CmpxToken.Value.ComplexTypIndex].MstType==MasterType::DynArray){

    //Operand loop
    for(j=0;j<OpndTokens.Length();j++){
      
      //Check parameter is initialized
      if(!OpndTokens[j].IsInitialized()){
        OpndTokens[j].Msg(429).Print(_Md->Variables[OpndTokens[j].Value.VarIndex].Name);
        return false;
      } 

      //Automatic data type promotions
      if(_IsDataTypePromotionAutomatic(OpndTokens[j].MstType(),_Md->Types[ElemTypIndex].MstType)){
        if(!_CompileDataTypePromotion(Scope,CodeBlockId,OpndTokens[j],_Md->Types[ElemTypIndex].MstType)){ return false; }
      }
  
      //Check data type of operand matches field definition
      //(We only send error when master types do not match and they are not atomic and they are not equivalent arrays)
      if(_Md->WordTypeFilter(OpndTokens[j].TypIndex(),true)!=_Md->WordTypeFilter(ElemTypIndex,true)
      && _Md->ConvertibleTypeName(_Md->WordTypeFilter(OpndTokens[j].TypIndex(),true))!=_Md->ConvertibleTypeName(_Md->WordTypeFilter(ElemTypIndex,true))){
        if(!((OpndTokens[j].MstType()==_Md->Types[ElemTypIndex].MstType && OpndTokens[j].IsMasterAtomic() && _Md->IsMasterAtomic(ElemTypIndex)) || _Md->EquivalentArrays(OpndTokens[j].TypIndex(),ElemTypIndex))){
          OpndTokens[j].Msg(430).Print(_Md->CannonicalTypeName(OpndTokens[j].TypIndex()),_Md->CannonicalTypeName(CmpxToken.Value.ComplexTypIndex),_Md->CannonicalTypeName(ElemTypIndex));
          return false;
        }
      }

    }

  }
    
  //Assignment of operands into result for classes
  if(_Md->Types[CmpxToken.Value.ComplexTypIndex].MstType==MasterType::Class){

    //Do nothing if class is empty
    if(!EmptyClass){

      //Field loop
      i=-1; j=0;
      while((i=_Md->FieldLoop(CmpxToken.Value.ComplexTypIndex,i))!=-1){
        
        //Static class fields
        if(_Md->Fields[i].IsStatic){
          VarName=_Md->GetStaticFldName(i);
          if((VarIndex=_Md->VarSearch(VarName,Scope.ModIndex))==-1){
            OpndTokens[j].Msg(431).Print(_Md->Fields[i].Name,_Md->CannonicalTypeName(_Md->Fields[i].SubScope.TypIndex),VarName);
            return false; 
          }
          StaticFld.ThisVar(_Md,VarIndex,OpndTokens[j].SrcInfo());
          if(!CopyOperand(_Md,StaticFld,OpndTokens[j])){ return false; }
          _Md->Variables[VarIndex].IsInitialized=true;
          DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[VarIndex].Name+" in scope "+_Md->ScopeName(_Md->Variables[VarIndex].Scope));
        }
        
        //Non-static class fields
        //(for every field we change the pointing reference type to the corresponding field type (for CopyOperand() inner checkings)
        //The first non-static field initializes pointing reference
        else{
          if(i==FirstNonStatic){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::REFOF,Reference.Asm(true),Result.Asm(),_Md->Bin.AsmLitWrd(0))){ return false; } 
          }
          _Md->Variables[Reference.Value.VarIndex].TypIndex=_Md->Fields[i].TypIndex;
          if(!CopyOperand(_Md,Reference,OpndTokens[j])){ return false; }
          if(i<LastNonStatic){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::REFAD,Reference.Asm(true),_Md->Bin.AsmLitWrd(_Md->Types[_Md->Fields[i].TypIndex].Length))){ return false; } 
          }
        }
        
        //Increase token index
        j++;

      }

    }

  }
  
  //Assignment of operands into result for arrays
  else if(_Md->Types[CmpxToken.Value.ComplexTypIndex].MstType==MasterType::FixArray
  ||      _Md->Types[CmpxToken.Value.ComplexTypIndex].MstType==MasterType::DynArray){

    //Initialize dynamic arrays
    if(_Md->Types[CmpxToken.Value.ComplexTypIndex].MstType==MasterType::DynArray){
      DimNr=_Md->Types[CmpxToken.Value.ComplexTypIndex].DimNr;
      CellSize=_Md->Types[_Md->Types[CmpxToken.Value.ComplexTypIndex].ElemTypIndex].Length;
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADDEF,Result.Asm(),_Md->Bin.AsmLitChr(DimNr),_Md->Bin.AsmLitWrd(CellSize))){ return false; }
      for(i=0;i<DimNr;i++){
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADSET,Result.Asm(),_Md->Bin.AsmLitChr(i+1),_Md->Bin.AsmLitWrd(CmpxToken.DimSize.n[i]))){ return false; } 
      }
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADRSZ,Result.Asm())){ return false; } 
    }

    //Create reference to first array element
    if(OpndTokens.Length()!=0){
      if(_Md->Types[CmpxToken.Value.ComplexTypIndex].MstType==MasterType::FixArray){
        if(_Md->Types[CmpxToken.Value.ComplexTypIndex].DimNr==1){
          GeomIndex=_Md->AsmAgx(CmpxToken.Value.ComplexTypIndex);
          if(!_Md->Bin.AsmWriteCode(CpuInstCode::AF1RF,Reference.Asm(true),Result.Asm(),GeomIndex,_Md->Bin.AsmLitWrd(0))){ return false; }
        }
        else{
          GeomIndex=_Md->AsmAgx(CmpxToken.Value.ComplexTypIndex);
          for(i=0;i<_Md->Types[CmpxToken.Value.ComplexTypIndex].DimNr;i++){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::AFIDX,GeomIndex,_Md->Bin.AsmLitChr(i+1),_Md->Bin.AsmLitWrd(0))){ return false; }
          }
          if(!_Md->Bin.AsmWriteCode(CpuInstCode::AFREF,Reference.Asm(true),Result.Asm(),GeomIndex)){ return false; }
        }
      }
      else{
        if(_Md->Types[CmpxToken.Value.ComplexTypIndex].DimNr==1){
          if(!_Md->Bin.AsmWriteCode(CpuInstCode::AD1RF,Reference.Asm(true),Result.Asm(),_Md->Bin.AsmLitWrd(0))){ return false; }
        }
        else{
          for(i=0;i<_Md->Types[CmpxToken.Value.ComplexTypIndex].DimNr;i++){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADIDX,Result.Asm(),_Md->Bin.AsmLitChr(i+1),_Md->Bin.AsmLitWrd(0))){ return false; }
          }
          if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADREF,Reference.Asm(true),Result.Asm())){ return false; }
        }
      }
    }

    //Field loop
    for(j=0;j<OpndTokens.Length();j++){
      if(!CopyOperand(_Md,Reference,OpndTokens[j])){ return false; }
      if(j!=OpndTokens.Length()-1){
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::REFAD,Reference.Asm(true),_Md->Bin.AsmLitWrd(_Md->Types[ElemTypIndex].Length))){ return false; } 
      }
    }

  }

  //Release working reference
  if(_Md->Types[CmpxToken.Value.ComplexTypIndex].MstType==MasterType::Class){
    if(!EmptyClass && FirstNonStatic!=-1 && LastNonStatic!=-1){
      Reference.Release();
    }
  }
  else if(_Md->Types[CmpxToken.Value.ComplexTypIndex].MstType==MasterType::FixArray
  ||      _Md->Types[CmpxToken.Value.ComplexTypIndex].MstType==MasterType::DynArray){
    if(OperandNr!=0){
      Reference.Release();
    }
  }
      
  //Set result variable as initialized since it is a litteral value
  _Md->Variables[Result.VarIndex()].IsInitialized=true;
  DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[Result.VarIndex()].Name+" in scope "+_Md->ScopeName(_Md->Variables[Result.VarIndex()].Scope));

  //Push result into stack
  Result.IsCalculated=true;
  OpndStack.Push(Result);

  //Return code
  return true;

}

//Compile array subscript / string subscript
bool Expression::_SubscriptCall(const ScopeDef& Scope,CpuLon CodeBlockId,int CurrToken,Stack<ExprToken>& OpndStack,ExprToken& Result){

  //Vaariables
  int i;
  ExprToken ObjToken;
  ExprToken SubToken;
  ExprToken Token;
  Array<ExprToken> IdxTokens;
  AsmArg GeomIndex;

  //Get subscript token
  SubToken=_Tokens[CurrToken];

  //Check number of tokens in stack
  if(OpndStack.Length()<SubToken.Value.DimNr+1){
    SubToken.Msg(372).Print(ToString(SubToken.Value.DimNr+1),ToString(OpndStack.Length())); 
    return false;
  }

  //Get sub index tokens and array/string token
  for(i=0;i<SubToken.Value.DimNr;i++){
    Token=OpndStack[OpndStack.Length()-SubToken.Value.DimNr+i];
    IdxTokens.Add(Token);
  }
  OpndStack.Pop(SubToken.Value.DimNr);
  ObjToken=OpndStack.Pop();
  
  //Check we do not have undefined variables or void results as operands
  for(i=0;i<IdxTokens.Length();i++){
    if(IdxTokens[i].Id()==ExprTokenId::UndefVar){
      IdxTokens[i].Msg(446).Print(IdxTokens[i].Value.VarName);
      return false;
    }
    else if(IdxTokens[i].Id()==ExprTokenId::VoidRes){
      IdxTokens[i].Msg(111).Print(IdxTokens[i].Value.VoidFuncName);
      return false;
    }
  }
  if(ObjToken.Id()==ExprTokenId::UndefVar){
    ObjToken.Msg(446).Print(ObjToken.Value.VarName);
    return false;
  }
  else if(ObjToken.Id()==ExprTokenId::VoidRes){
    ObjToken.Msg(111).Print(ObjToken.Value.VoidFuncName);
    return false;
  }

  //Message
  #ifdef __DEV__
  String Tokens="";
  for(i=0;i<IdxTokens.Length();i++){ Tokens+=IdxTokens[i].Print()+" "; }
  DebugMessage(DebugLevel::CmpExpression,"Subscript call on token "+ObjToken.Print()+" using: "+Tokens);
  #endif

  //Check subindexes are initialized variables
  for(i=0;i<IdxTokens.Length();i++){
    if(!IdxTokens[i].IsInitialized()){
      IdxTokens[i].Msg(131).Print(IdxTokens[i].Name());
      return false;
    } 
  }

  //Check subscript is applied to scriptable type
  if(_Md->Types[ObjToken.TypIndex()].MstType!=MasterType::FixArray
  &&  _Md->Types[ObjToken.TypIndex()].MstType!=MasterType::DynArray
  &&  _Md->Types[ObjToken.TypIndex()].MstType!=MasterType::String){ 
    SubToken.Msg(80).Print(_Md->CannonicalTypeName(ObjToken.TypIndex())); 
    return false;
  }

  //Check subscript is applied to scriptable object
  if(ObjToken.AdrMode!=CpuAdrMode::Address && ObjToken.AdrMode!=CpuAdrMode::Indirection){ 
    SubToken.Msg(394).Print(); 
    return false;
  }

  //Get result operand (this goes before data type promotions, to avoid reusing temp vars still not used)
  if(_Md->Types[ObjToken.TypIndex()].MstType==MasterType::FixArray){ 
    if(!Result.NewInd(_Md,Scope,CodeBlockId,_Md->Types[ObjToken.TypIndex()].ElemTypIndex,ObjToken.IsConst,ObjToken.SrcInfo())){
      return false;
    }
  }
  else if(_Md->Types[ObjToken.TypIndex()].MstType==MasterType::DynArray){ 
    if(!Result.NewInd(_Md,Scope,CodeBlockId,_Md->Types[ObjToken.TypIndex()].ElemTypIndex,ObjToken.IsConst,ObjToken.SrcInfo())){
      return false;
    }
  }
  else if(_Md->Types[ObjToken.TypIndex()].MstType==MasterType::String){ 
    if(!Result.NewInd(_Md,Scope,CodeBlockId,_Md->ChrTypIndex,ObjToken.IsConst,ObjToken.SrcInfo())){
      return false;
    }
  }

  //Set related source variable in result token
  Result.SourceVarIndex=(ObjToken.SourceVarIndex!=-1?ObjToken.SourceVarIndex:ObjToken.Value.VarIndex);
  DebugMessage(DebugLevel::CmpExpression,"Source variable set for token "+Result.Print()+" pointing to variable "+_Md->Variables[Result.SourceVarIndex].Name+" in scope "+_Md->ScopeName(_Md->Variables[Result.SourceVarIndex].Scope));

  //Release all tokens here as we do not want reference for result to be reused
  for(i=0;i<IdxTokens.Length();i++){
    IdxTokens[i].Release();
  }
  ObjToken.Release();

  //Set operands as used in source
  for(i=0;i<IdxTokens.Length();i++){
    IdxTokens[i].SetSourceUsed(Scope,true);
  }
  ObjToken.SetSourceUsed(Scope,false);

  //Check all subindexes are integers or promote them to integers if possible
  for(i=0;i<IdxTokens.Length();i++){
    if(IdxTokens[i].MstType()!=WordMasterType()){ 
      if(_IsDataTypePromotionAutomatic(IdxTokens[i].MstType(),WordMasterType())){
        if(!_CompileDataTypePromotion(Scope,CodeBlockId,IdxTokens[i],WordMasterType())){ return false; }
      }
      else{
        IdxTokens[i].Msg(81).Print(MasterTypeName(WordMasterType())); 
        return false; 
      }
    }
  }

  //Object token is a fixed array
  if(_Md->Types[ObjToken.TypIndex()].MstType==MasterType::FixArray){ 
  
    //Check subindexes matches array dimension
    if(_Md->Types[ObjToken.TypIndex()].DimNr!=SubToken.Value.DimNr){ 
      SubToken.Msg(82).Print(ToString(_Md->Types[ObjToken.TypIndex()].DimNr),ToString(SubToken.Value.DimNr)); 
      return false; 
    }
  
    //Emit instructions
    if(_Md->Types[ObjToken.TypIndex()].DimNr==1){
      GeomIndex=_Md->AsmAgx(ObjToken.TypIndex());
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::AF1RF,Result.Asm(true),ObjToken.Asm(),GeomIndex,IdxTokens[0].Asm())){ return false; }
    }
    else{
      GeomIndex=_Md->AsmAgx(ObjToken.TypIndex());
      for(i=0;i<_Md->Types[ObjToken.TypIndex()].DimNr;i++){
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::AFIDX,GeomIndex,_Md->Bin.AsmLitChr(i+1),IdxTokens[i].Asm())){ return false; }
      }
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::AFREF,Result.Asm(true),ObjToken.Asm(),GeomIndex)){ return false; }
    }
  
  }

  //Object token is a dynamic array
  else if(_Md->Types[ObjToken.TypIndex()].MstType==MasterType::DynArray){ 
  
    //Check subindexes matches array dimension
    if(_Md->Types[ObjToken.TypIndex()].DimNr!=SubToken.Value.DimNr){ 
      SubToken.Msg(82).Print(ToString(_Md->Types[ObjToken.TypIndex()].DimNr),ToString(SubToken.Value.DimNr)); 
      return false; 
    }
  
    //Emit instructions
    if(_Md->Types[ObjToken.TypIndex()].DimNr==1){
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::AD1RF,Result.Asm(true),ObjToken.Asm(),IdxTokens[0].Asm())){ return false; }
    }
    else{
      for(i=0;i<_Md->Types[ObjToken.TypIndex()].DimNr;i++){
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADIDX,ObjToken.Asm(),_Md->Bin.AsmLitChr(i+1),IdxTokens[i].Asm())){ return false; }
      }
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADREF,Result.Asm(true),ObjToken.Asm())){ return false; }
    }
  
  }

  //Object token is a string
  else if(_Md->Types[ObjToken.TypIndex()].MstType==MasterType::String){ 
  
    //Check number of subindexes is only 1
    if(SubToken.Value.DimNr!=1){ 
      SubToken.Msg(3).Print(); 
      return false; 
    }
  
    //Emit instructions
    if(!_Md->Bin.AsmWriteCode(CpuInstCode::SINDX,Result.Asm(true),ObjToken.Asm(),IdxTokens[0].Asm())){ return false; }
  
  }

  //Set Result as initialized variable
  _Md->Variables[Result.VarIndex()].IsInitialized=true;
  DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[Result.VarIndex()].Name+" in scope "+_Md->ScopeName(_Md->Variables[Result.VarIndex()].Scope));

  //Push result to operand stack
  Result.IsCalculated=true;
  OpndStack.Push(Result);
  
  //Return code
  return true;

}

//Compile operator overload call
bool Expression::_OperatorCall(const ScopeDef& Scope,CpuLon CodeBlockId,int FunIndex,const ExprToken& OprToken,ExprToken& Opnd1,const ExprToken& Opnd2,bool IsOprStackEmpty,ExprToken& Result){

  //Vaariables
  int i,j;
  int ParmLow;
  int ParmHigh;
  ExprToken ParmToken;
  Array<ExprToken> ParmTokens;

  //Check function kind is operator
  if(_Md->Functions[FunIndex].Kind!=FunctionKind::Operator){
    OprToken.Msg(376).Print(_Md->Functions[FunIndex].Name);
    return false;
  }

  //Get parameter array
  ParmTokens.Add(Opnd1);
  if(_Opr[(int)OprToken.Value.Operator].Class==ExprOpClass::Binary){
    ParmTokens.Add(Opnd2);
  }

  //When function is void it cannot happen in middle of expressions
  if(_Md->Functions[FunIndex].IsVoid && !IsOprStackEmpty && !_Opr[(int)OprToken.Value.Operator].IsResultFirst){
    OprToken.Msg(45).Print(_Md->Functions[FunIndex].Name);
    return false;
  }

  //Create result token
  if(!_Md->Functions[FunIndex].IsVoid){
    if(!Result.NewVar(_Md,Scope,CodeBlockId,_Md->Functions[FunIndex].TypIndex,OprToken.SrcInfo())){ return false; }
    _Md->Variables[Result.VarIndex()].IsInitialized=true;
    DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[Result.VarIndex()].Name+" in scope "+_Md->ScopeName(_Md->Variables[Result.VarIndex()].Scope));
  }

  //Parameter check loop
  if(_Md->Functions[FunIndex].ParmNr!=0){

    //Get parameter indexes and ignore result parameter
    ParmLow=_Md->Functions[FunIndex].ParmLow;
    ParmHigh=_Md->Functions[FunIndex].ParmHigh;
    if(!_Md->Functions[FunIndex].IsVoid){ ParmLow++; }

    //When result of operator is first operand, function must be void
    if(_Opr[(int)OprToken.Value.Operator].IsResultFirst){
      if(!_Md->Functions[FunIndex].IsVoid){
        OprToken.Msg(377).Print(_Opr[(int)OprToken.Value.Operator].Name,_Md->Functions[FunIndex].FullName);
        return false;
      }
    }
    else{
      if(_Md->Functions[FunIndex].IsVoid){
        OprToken.Msg(379).Print(_Opr[(int)OprToken.Value.Operator].Name,_Md->Functions[FunIndex].FullName);
        return false;
      }
    }

    //Parameter loop
    for(i=ParmLow,j=0;i<=ParmHigh;i++,j++){
      
      //When result of operator is first operand, first parameter must be passed by reference
      //If not arguments are passed by value or constant references
      if(i==ParmLow && _Opr[(int)OprToken.Value.Operator].IsResultFirst){
        if(!_Md->Parameters[i].IsReference || _Md->Parameters[i].IsConst){
          OprToken.Msg(378).Print(_Opr[(int)OprToken.Value.Operator].Name,_Md->Functions[FunIndex].FullName);
          return false;
        }
      }
      else{
        if(_Md->Parameters[i].IsReference && !_Md->Parameters[i].IsConst){
          OprToken.Msg(380).Print(_Md->Functions[FunIndex].FullName);
          return false;
        }
      }

      //Check parameter is initialized
      if(!ParmTokens[j].IsInitialized()){
        ParmTokens[j].Msg(131).Print(ParmTokens[j].Name());
        return false;
      } 

      //Automatic data type promotions
      if(_IsDataTypePromotionAutomatic(ParmTokens[j].MstType(),_Md->Types[_Md->Parameters[i].TypIndex].MstType)){
        if(!_CompileDataTypePromotion(Scope,CodeBlockId,ParmTokens[j],_Md->Types[_Md->Parameters[i].TypIndex].MstType)){ return false; }
      }
  
      //Check data type of argument matches parameter
      if(_Md->WordTypeFilter(ParmTokens[j].TypIndex(),true)!=_Md->WordTypeFilter(_Md->Parameters[i].TypIndex,true)){
        ParmTokens[j].Msg(50).Print(_Md->Functions[FunIndex].Name,_Md->Parameters[i].Name,_Md->CannonicalTypeName(ParmTokens[j].TypIndex()),_Md->CannonicalTypeName(_Md->Parameters[i].TypIndex));
        return false;
      }
  
      //Check that if parameter passed by reference it is an lvalue
      if(_Md->Parameters[i].IsReference && !_Md->Parameters[i].IsConst && !ParmTokens[j].IsLValue()){
        ParmTokens[j].Msg(157).Print(_Md->Functions[FunIndex].Name,_Md->Parameters[i].Name);
        return false;
      }
  
      //Check constants are not passed by reference
      if(_Md->Parameters[i].IsReference && !_Md->Parameters[i].IsConst && ParmTokens[j].IsConst){
        ParmTokens[j].Msg(160).Print(_Md->Functions[FunIndex].Name,_Md->Parameters[i].Name);
        return false;
      }

      //Litteral values can be passed by referenceonly when they are strings or arrays (since they usememory addressing)
      if(_Md->Parameters[i].IsReference && ParmTokens[j].AdrMode==CpuAdrMode::LitValue){
        if(_Md->Types[_Md->Parameters[i].TypIndex].MstType!=MasterType::String && _Md->Types[_Md->Parameters[i].TypIndex].MstType!=MasterType::DynArray){
          ParmTokens[j].Msg(234).Print(_Md->Functions[FunIndex].Name,_Md->Parameters[i].Name);
          return false;
        }
      }
    
    }

  }

  //Get parameter indexes
  ParmLow=_Md->Functions[FunIndex].ParmLow;
  ParmHigh=_Md->Functions[FunIndex].ParmHigh;

  //Push reference to result in stack
  if(!_Md->Functions[FunIndex].IsVoid){
    if(_Md->TypLength(_Md->Functions[FunIndex].TypIndex)!=0){
      if(_Md->Functions[FunIndex].Scope.Kind==ScopeKind::Local){
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::REFER,_Md->AsmPar(ParmLow),Result.Asm())){ return false; }
      }
      else{
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::REFPU,Result.Asm())){ return false; }
      }
    }
    ParmLow++;
  }

  //Push parameters into parameter stack
  if(_Md->Functions[FunIndex].ParmNr!=0){

    //Parameter loop
    for(i=ParmLow,j=0;i<=ParmHigh;i++,j++){
      
      //Skip parameters with zero size (empty classes)
      if(_Md->TypLength(_Md->Parameters[i].TypIndex)==0){ continue; }

      //Parameter passing for local functions
      if(_Md->Functions[FunIndex].Scope.Kind==ScopeKind::Local){
        if(_Md->Parameters[i].IsReference){
          if(ParmTokens[j].AdrMode!=CpuAdrMode::LitValue && _Md->Variables[ParmTokens[j].Value.VarIndex].IsReference){          
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::MVr,_Md->AsmPar(i),ParmTokens[j].Asm(true))){ return false; }
          }
          else{
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::REFER,_Md->AsmPar(i),ParmTokens[j].Asm())){ return false; }
          }
        }
        else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Boolean){
          if(!_Md->Bin.AsmWriteCode(CpuInstCode::MVb,_Md->AsmPar(i),ParmTokens[j].Asm())){ return false; }
        }
        else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Char){
          if(!_Md->Bin.AsmWriteCode(CpuInstCode::MVc,_Md->AsmPar(i),ParmTokens[j].Asm())){ return false; }
        }
        else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Short){
          if(!_Md->Bin.AsmWriteCode(CpuInstCode::MVw,_Md->AsmPar(i),ParmTokens[j].Asm())){ return false; }
        }
        else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Integer){
          if(!_Md->Bin.AsmWriteCode(CpuInstCode::MVi,_Md->AsmPar(i),ParmTokens[j].Asm())){ return false; }
        }
        else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Long){
          if(!_Md->Bin.AsmWriteCode(CpuInstCode::MVl,_Md->AsmPar(i),ParmTokens[j].Asm())){ return false; }
        }
        else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Float){
          if(!_Md->Bin.AsmWriteCode(CpuInstCode::MVf,_Md->AsmPar(i),ParmTokens[j].Asm())){ return false; }
        }
        else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Enum){
          if(!_Md->Bin.AsmWriteCode(CpuInstCode::MVi,_Md->AsmPar(i),ParmTokens[j].Asm())){ return false; }
        }
        else{
          ParmTokens[j].Msg(75).Print(_Md->Parameters[i].Name,_Md->Modules[OprToken.FunModIndex].Name,_Md->Functions[FunIndex].Name);
          return false;
        }
      }

      //Parameter passing for public/private functions
      else{
        if(_Md->Parameters[i].IsReference){
          if(ParmTokens[j].AdrMode!=CpuAdrMode::LitValue && _Md->Variables[ParmTokens[j].Value.VarIndex].IsReference){          
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::PUSHr,ParmTokens[j].Asm(true))){ return false; }
          }
          else{
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::REFPU,ParmTokens[j].Asm())){ return false; }
          }
        }
        else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Boolean){
          if(!_Md->Bin.AsmWriteCode(CpuInstCode::PUSHb,ParmTokens[j].Asm())){ return false; }
        }
        else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Char){
          if(!_Md->Bin.AsmWriteCode(CpuInstCode::PUSHc,ParmTokens[j].Asm())){ return false; }
        }
        else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Short){
          if(!_Md->Bin.AsmWriteCode(CpuInstCode::PUSHw,ParmTokens[j].Asm())){ return false; }
        }
        else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Integer){
          if(!_Md->Bin.AsmWriteCode(CpuInstCode::PUSHi,ParmTokens[j].Asm())){ return false; }
        }
        else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Long){
          if(!_Md->Bin.AsmWriteCode(CpuInstCode::PUSHl,ParmTokens[j].Asm())){ return false; }
        }
        else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Float){
          if(!_Md->Bin.AsmWriteCode(CpuInstCode::PUSHf,ParmTokens[j].Asm())){ return false; }
        }
        else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Enum){
          if(!_Md->Bin.AsmWriteCode(CpuInstCode::PUSHi,ParmTokens[j].Asm())){ return false; }
        }
        else{
          ParmTokens[j].Msg(75).Print(_Md->Parameters[i].Name,_Md->Modules[OprToken.FunModIndex].Name,_Md->Functions[FunIndex].Name);
          return false;
        }
      }

    }
    
  }
    
  //Call operator function
  if(_Md->Functions[FunIndex].IsNested){
    if(!_Md->Bin.AsmWriteCode(CpuInstCode::CALLN,_Md->AsmFun(FunIndex))){ return false; }
  }
  else{
    if(!_Md->Bin.AsmWriteCode(CpuInstCode::CALL,_Md->AsmFun(FunIndex))){ return false; }
  }

  //Return code
  return true;

}

//Compile function/method call
bool Expression::_FunctionMethodCall(const ScopeDef& Scope,CpuLon CodeBlockId,int CurrToken,Stack<ExprToken>& OpndStack,ExprCallType CallType,bool IsOprStackEmpty,ExprToken& Result){

  //Vaariables
  int i,j;
  int FunIndex;
  int ParmLow;
  int ParmHigh;
  String ParmStr1;
  String ParmStr2;
  String Matches;
  ExprToken FunToken;
  ExprToken ParmToken;
  ExprToken OpdToken;
  Array<ExprToken> ParmTokens;
  int DlCallId;

  //Get function token
  FunToken=_Tokens[CurrToken];

  //Check stack has enough operands
  if(CallType==ExprCallType::Method){
    if(OpndStack.Length()<FunToken.CallParmNr+1){
      FunToken.Msg(86).Print(FunToken.Value.Method,ToString(FunToken.CallParmNr+1),ToString(OpndStack.Length()));
      return false;
    }
  }
  else{
    if(OpndStack.Length()<FunToken.CallParmNr){
      FunToken.Msg(83).Print(FunToken.Value.Function,ToString(FunToken.CallParmNr),ToString(OpndStack.Length()));
      return false;
    }
  }

  //Get parameters from stack
  for(i=0;i<FunToken.CallParmNr;i++){
    ParmToken=OpndStack[OpndStack.Length()-FunToken.CallParmNr+i];
    ParmTokens.Add(ParmToken);
  }
  OpndStack.Pop(FunToken.CallParmNr);

  //Check we do not have undefined variables or void results as operands
  for(i=0;i<ParmTokens.Length();i++){
    if(ParmTokens[i].Id()==ExprTokenId::UndefVar){
      ParmTokens[i].Msg(446).Print(ParmTokens[i].Value.VarName);
      return false;
    }
    else if(ParmTokens[i].Id()==ExprTokenId::VoidRes){
      ParmTokens[i].Msg(111).Print(ParmTokens[i].Value.VoidFuncName);
      return false;
    }
  }

  //Get class instance operand from stack
  if(CallType==ExprCallType::Method){
    OpdToken=OpndStack.Pop(); 
    if(OpdToken.Id()==ExprTokenId::UndefVar){
      OpdToken.Msg(446).Print(OpdToken.Value.VarName);
      return false;
    }
    else if(OpdToken.Id()==ExprTokenId::VoidRes){
      OpdToken.Msg(111).Print(OpdToken.Value.VoidFuncName);
      return false;
    }
    if(OpdToken.Id()!=ExprTokenId::Operand){
      OpdToken.Msg(539).Print();
      return false;
    }
  }

  //Compose function name with parameter string (ParmStr1 is for current architecture, ParmStr2 is for the other one)
  ParmStr1="";
  ParmStr2="";
  for(i=0;i<ParmTokens.Length();i++){
    ParmStr1+=(ParmStr1.Length()!=0?",":"")+_Md->Types[_Md->WordTypeFilter(ParmTokens[i].TypIndex(),true)].Name;
    ParmStr2+=(ParmStr2.Length()!=0?",":"")+_Md->ConvertibleTypeName(_Md->WordTypeFilter(ParmTokens[i].TypIndex(),true));
  }

  //Find function/method
  switch(CallType){
    
    //Function call
    case ExprCallType::Function:
      if((FunIndex=_Md->FunSearch(FunToken.Value.Function,FunToken.FunModIndex,ParmStr1,ParmStr2,&Matches))==-1){
        if(Matches.Length()==0){
          FunToken.Msg(36).Print(_Md->Modules[FunToken.FunModIndex].Name,FunToken.Value.Function,ParmStr1);
        }
        else{
          FunToken.Msg(289).Print(_Md->Modules[FunToken.FunModIndex].Name,FunToken.Value.Function,ParmStr1,Matches);
        }
        return false;
      }
      break;

    //Method call
    //(the second search by name on master methods is needed when argument is array element
    //as number of passed parameters is not checked we must add an additional check)
    case ExprCallType::Method:
      if((FunIndex=_Md->FmbSearch(FunToken.Value.Method,_Md->Types[OpdToken.TypIndex()].Scope.ModIndex,OpdToken.TypIndex(),ParmStr1,ParmStr2,&Matches))==-1){
        if(Matches.Length()!=0){
          FunToken.Msg(178).Print(FunToken.Value.Method,ParmStr1,Matches);
          return false;
        }
        else{
          if((FunIndex=_Md->MmtSearch(FunToken.Value.Method,OpdToken.MstType(),ParmStr1,ParmStr2,&Matches))==-1){
            if(Matches.Length()!=0){
              FunToken.Msg(178).Print(FunToken.Value.Method,ParmStr1,Matches);
              return false;
            }
            else{
              if((FunIndex=_Md->MmtSearch(FunToken.Value.Method,OpdToken.MstType(),ParmStr1,ParmStr2,&Matches,true))==-1){
                if(Matches.Length()!=0){
                  FunToken.Msg(178).Print(FunToken.Value.Method,ParmStr1,Matches);
                  return false;
                }
                else{
                  FunToken.Msg(87).Print(FunToken.Value.Method,ParmStr1,_Md->CannonicalTypeName(OpdToken.TypIndex()),MasterTypeName(OpdToken.MstType()));
                  return false;
                }
              }
              if(_Md->Functions[FunIndex].ParmNr!=ParmTokens.Length()){
                FunToken.Msg(538).Print(FunToken.Value.Method,ParmStr1,_Md->CannonicalTypeName(OpdToken.TypIndex()),MasterTypeName(OpdToken.MstType()));
                return false;
              }
            }
          }
        }
      }
      break;

    //Constructor call
    case ExprCallType::Constructor:
      if((FunIndex=_Md->FmbSearch(_Md->Types[FunToken.Value.CCTypIndex].Name,_Md->Types[FunToken.Value.CCTypIndex].Scope.ModIndex,FunToken.Value.CCTypIndex,ParmStr1,ParmStr2,&Matches))==-1){
        if(Matches.Length()==0){
          FunToken.Msg(317).Print(_Md->Types[FunToken.Value.CCTypIndex].Name,ParmStr1);
        }
        else{
          FunToken.Msg(318).Print(_Md->Types[FunToken.Value.CCTypIndex].Name,ParmStr1,Matches);
        }
        return false; 
      }
      break;

  }

  //Check call to member function / master method from unitialized class object
  if(CallType==ExprCallType::Method){
    if(_Md->Functions[FunIndex].Kind==FunctionKind::Member && !_Md->Functions[FunIndex].IsInitializer && !_Md->IsEmptyClass(OpdToken.TypIndex())){
      if(!OpdToken.IsInitialized()){  
        FunToken.Msg(540).Print(_Md->Functions[FunIndex].Name);
        return false;
      }
    }
    else if(_Md->Functions[FunIndex].Kind==FunctionKind::MasterMth && !_Md->Functions[FunIndex].IsInitializer && !_Md->Functions[FunIndex].IsMetaMethod){
      if(!OpdToken.IsInitialized()){
        FunToken.Msg(541).Print(_Md->Functions[FunIndex].Name);
        return false;
      }
    }
  }

  //Check function access in relation to its scope
  switch(CallType){
    case ExprCallType::Function:
      if(_Md->Functions[FunIndex].Scope.Kind==ScopeKind::Private && _Md->Functions[FunIndex].Scope.ModIndex!=Scope.ModIndex){
        FunToken.Msg(38).Print(_Md->Functions[FunIndex].Name,_Md->Modules[_Md->Functions[FunIndex].Scope.ModIndex].Name);
        return false;
      }
      break;
    case ExprCallType::Method:
      if(_Origin!=OrigBuffer::Insertion && _Origin!=OrigBuffer::Addition && _Md->Functions[FunIndex].Kind!=FunctionKind::MasterMth){
        if(!_Md->IsMemberVisible(Scope,OpdToken.TypIndex(),-1,FunIndex)){
          FunToken.Msg(171).Print(_Md->CannonicalTypeName(OpdToken.TypIndex()),FunToken.Value.Method,ParmStr1);
          return false;
        }
      }
      break;
    case ExprCallType::Constructor:
      if(!_Md->IsMemberVisible(Scope,FunToken.Value.CCTypIndex,-1,FunIndex)){
        FunToken.Msg(319).Print(_Md->CannonicalTypeName(FunToken.Value.CCTypIndex),_Md->Types[FunToken.Value.CCTypIndex].Name,ParmStr1);
        return false;
      }
      break;
  }

  //Create result token
  if(!_Md->Functions[FunIndex].IsVoid){
    if(CallType==ExprCallType::Method && _Md->Functions[FunIndex].Kind==FunctionKind::MasterMth){
      if(_Md->Functions[FunIndex].TypIndex!=-1){
        if(!Result.NewVar(_Md,Scope,CodeBlockId,_Md->Functions[FunIndex].TypIndex,FunToken.SrcInfo())){ return false; }
      }
      else{
        if(!Result.NewVar(_Md,Scope,CodeBlockId,OpdToken.TypIndex(),FunToken.SrcInfo())){ return false; }
      }
    }
    else{
      if(!Result.NewVar(_Md,Scope,CodeBlockId,_Md->Functions[FunIndex].TypIndex,FunToken.SrcInfo())){ return false; }
    }
    _Md->Variables[Result.VarIndex()].IsInitialized=true;
    DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[Result.VarIndex()].Name+" in scope "+_Md->ScopeName(_Md->Variables[Result.VarIndex()].Scope));
  }
  else{
    if(!IsOprStackEmpty){
      Result.Clear();
      Result.Id(ExprTokenId::VoidRes);
      Result.SetMdPointer(_Md);
      Result.SetPosition(FunToken.SrcInfo());
      switch(CallType){
        case ExprCallType::Function:    Result.Value.VoidFuncName="Function "+_Md->Modules[FunToken.FunModIndex].Name+"."+FunToken.Value.Function+"()"; break;
        case ExprCallType::Method:      Result.Value.VoidFuncName="Method ."+FunToken.Value.Method+"()"; break;
        case ExprCallType::Constructor: Result.Value.VoidFuncName="Class constructor "+_Md->Types[FunToken.Value.CCTypIndex].Name+"("+ParmStr1+")"; break;
      }
    }
  }

  //Create temporary class variable for reference to self in constructors as they do not have it
  if(CallType==ExprCallType::Constructor){
    if(!OpdToken.NewVar(_Md,Scope,CodeBlockId,FunToken.Value.CCTypIndex,FunToken.SrcInfo())){ return false; }
  }

  //Release parameter tokens here to avoid reusing a parameter as function result
  for(i=0;i<ParmTokens.Length();i++){
    ParmTokens[i].Release();
  }
  if(CallType==ExprCallType::Method){
    OpdToken.Release();
  }
  
  //Set operands as used in source
  for(i=0;i<ParmTokens.Length();i++){
    ParmTokens[i].SetSourceUsed(Scope,true);
  }
  if(CallType==ExprCallType::Method){
    OpdToken.SetSourceUsed(Scope,true);
  }

  //Parameter check loop
  if(_Md->Functions[FunIndex].ParmNr!=0){

    //Get parameter indexes and ignore result and reference to self parameters
    ParmLow=_Md->Functions[FunIndex].ParmLow;
    ParmHigh=_Md->Functions[FunIndex].ParmHigh;
    if(!_Md->Functions[FunIndex].IsVoid){ ParmLow++; }
    if(_Md->Functions[FunIndex].Kind==FunctionKind::Member){ ParmLow++; }

    //Parameter loop
    for(i=ParmLow,j=0;i<=ParmHigh;i++,j++){
      
      //When we have identified a master method call that uses array element parameter we must update data type using operand token
      if(_Md->Functions[FunIndex].Kind==FunctionKind::MasterMth && _Md->Parameters[i].Name.StartsWith(_Md->GetElementTypePrefix())){
        if(_Md->Types[OpdToken.TypIndex()].MstType!=MasterType::DynArray){
          FunToken.Msg(472).Print();
          return false;
        }
        _Md->Parameters[i].TypIndex=_Md->Types[OpdToken.TypIndex()].ElemTypIndex;
        if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::String
        || _Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::DynArray
        || _Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::FixArray){
          _Md->Parameters[i].IsReference=true;
          _Md->Parameters[i].IsConst=true;
        }
        else{
          _Md->Parameters[i].IsReference=false;
          _Md->Parameters[i].IsConst=false;
        }
      }

      //Check parameter is initialized
      if(!ParmTokens[j].IsInitialized()){
        ParmTokens[j].Msg(131).Print(ParmTokens[j].Name());
        return false;
      } 

      //Automatic data type promotions
      if(_IsDataTypePromotionAutomatic(ParmTokens[j].MstType(),_Md->Types[_Md->Parameters[i].TypIndex].MstType)){
        if(!_CompileDataTypePromotion(Scope,CodeBlockId,ParmTokens[j],_Md->Types[_Md->Parameters[i].TypIndex].MstType)){ return false; }
      }
  
      //Check data type of argument matches parameter
      if(_Md->WordTypeFilter(ParmTokens[j].TypIndex(),true)!=_Md->WordTypeFilter(_Md->Parameters[i].TypIndex,true)
      && _Md->ConvertibleTypeName(_Md->WordTypeFilter(ParmTokens[j].TypIndex(),true))!=_Md->ConvertibleTypeName(_Md->WordTypeFilter(_Md->Parameters[i].TypIndex,true))){
        switch(CallType){
          case ExprCallType::Function:    ParmTokens[j].Msg(84).Print(_Md->Modules[FunToken.FunModIndex].Name,_Md->Functions[FunIndex].Name,_Md->Parameters[i].Name,_Md->CannonicalTypeName(ParmTokens[j].TypIndex()),_Md->CannonicalTypeName(_Md->Parameters[i].TypIndex)); break;
          case ExprCallType::Method:      ParmTokens[j].Msg(88).Print(_Md->Functions[FunIndex].Name,_Md->Parameters[i].Name,_Md->CannonicalTypeName(ParmTokens[j].TypIndex()),_Md->CannonicalTypeName(_Md->Parameters[i].TypIndex)); break;
          case ExprCallType::Constructor: ParmTokens[j].Msg(322).Print(_Md->Functions[FunIndex].Name,_Md->Parameters[i].Name,_Md->CannonicalTypeName(ParmTokens[j].TypIndex()),_Md->CannonicalTypeName(_Md->Parameters[i].TypIndex)); break;
        }
        return false;
      }
  
      //Check that if parameter passed by reference it is an lvalue
      if(_Md->Parameters[i].IsReference && !_Md->Parameters[i].IsConst && !ParmTokens[j].IsLValue()){
        switch(CallType){
          case ExprCallType::Function:    ParmTokens[j].Msg(162).Print(_Md->Modules[FunToken.FunModIndex].Name,_Md->Functions[FunIndex].Name,_Md->Parameters[i].Name); break;
          case ExprCallType::Method:      ParmTokens[j].Msg(161).Print(_Md->Functions[FunIndex].Name,_Md->Parameters[i].Name); break;
          case ExprCallType::Constructor: ParmTokens[j].Msg(323).Print(_Md->Functions[FunIndex].Name,_Md->Parameters[i].Name); break;
        }
        return false;
      }
  
      //Check constants are not passed by reference
      if(_Md->Parameters[i].IsReference && !_Md->Parameters[i].IsConst && ParmTokens[j].IsConst){
        switch(CallType){
          case ExprCallType::Function:    ParmTokens[j].Msg(85).Print(_Md->Modules[FunToken.FunModIndex].Name,_Md->Functions[FunIndex].Name,_Md->Parameters[i].Name); break;
          case ExprCallType::Method:      ParmTokens[j].Msg(89).Print(_Md->Functions[FunIndex].Name,_Md->Parameters[i].Name); break;
          case ExprCallType::Constructor: ParmTokens[j].Msg(324).Print(_Md->Functions[FunIndex].Name,_Md->Parameters[i].Name); break;
        }
        return false;
      }

      //Litteral values can be passed by reference only when they are strings or arrays (since they use memory addressing)
      if(_Md->Parameters[i].IsReference && ParmTokens[j].AdrMode==CpuAdrMode::LitValue){
        if(_Md->Types[_Md->Parameters[i].TypIndex].MstType!=MasterType::String && _Md->Types[_Md->Parameters[i].TypIndex].MstType!=MasterType::DynArray){
          switch(CallType){
            case ExprCallType::Function:    ParmTokens[j].Msg(236).Print(_Md->Modules[FunToken.FunModIndex].Name,_Md->Functions[FunIndex].Name,_Md->Parameters[i].Name); break;
            case ExprCallType::Method:      ParmTokens[j].Msg(235).Print(_Md->Functions[FunIndex].Name,_Md->Parameters[i].Name); break;
            case ExprCallType::Constructor: ParmTokens[j].Msg(325).Print(_Md->Functions[FunIndex].Name,_Md->Parameters[i].Name); break;
          }
          return false;
        }
      }

    }

  }
  
  //Push parameters into stack (only for user functions,member functions, system calls, dynamic library calls and operators)
  if(_Md->Functions[FunIndex].Kind==FunctionKind::Function || _Md->Functions[FunIndex].Kind==FunctionKind::Member 
  || _Md->Functions[FunIndex].Kind==FunctionKind::SysCall  || _Md->Functions[FunIndex].Kind==FunctionKind::DlFunc){

    //Get parameter indexes
    ParmLow=_Md->Functions[FunIndex].ParmLow;
    ParmHigh=_Md->Functions[FunIndex].ParmHigh;

    //Push reference to result in stack (skip parameters with length==0, empty classes!)
    if(!_Md->Functions[FunIndex].IsVoid){
      if(_Md->TypLength(_Md->Functions[FunIndex].TypIndex)!=0){
        if(_Md->Functions[FunIndex].Kind==FunctionKind::DlFunc){
          if(_Md->Types[Result.TypIndex()].MstType==MasterType::String){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::LRPUS,Result.Asm(),_Md->Bin.AsmLitBol((CpuBol)false))){ return false; }
          }
          else if(_Md->Types[Result.TypIndex()].MstType==MasterType::DynArray){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::LRPAD,Result.Asm(),_Md->Bin.AsmLitBol((CpuBol)false))){ return false; }
          }
          else if(_Md->Types[Result.TypIndex()].MstType==MasterType::FixArray){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::LRPAF,Result.Asm(),_Md->Bin.AsmLitBol((CpuBol)false),_Md->AsmAgx(Result.TypIndex()))){ return false; }
          }
          else{
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::LRPU,Result.Asm())){ return false; }
          }
        }
        else if(_Md->Functions[FunIndex].Scope.Kind==ScopeKind::Local){
          if(!_Md->Bin.AsmWriteCode(CpuInstCode::REFER,_Md->AsmPar(ParmLow),Result.Asm())){ return false; }
        }
        else{
          if(!_Md->Bin.AsmWriteCode(CpuInstCode::REFPU,Result.Asm())){ return false; }
        }
      }
      ParmLow++;
    }

    //Push reference to self (skip for empty classes)
    if(_Md->Functions[FunIndex].Kind==FunctionKind::Member){
      if(_Md->TypLength(OpdToken.TypIndex())!=0){
        if(OpdToken.AdrMode==CpuAdrMode::Indirection){
          if(!_Md->Bin.AsmWriteCode(CpuInstCode::PUSHr,OpdToken.Asm(true))){ return false; }
        }
        else if(_Md->Functions[FunIndex].Scope.Kind==ScopeKind::Local){
          if(!_Md->Bin.AsmWriteCode(CpuInstCode::REFER,_Md->AsmPar(ParmLow),OpdToken.Asm())){ return false; }
        }
        else{
          if(!_Md->Bin.AsmWriteCode(CpuInstCode::REFPU,OpdToken.Asm())){ return false; }
        }
      }
      ParmLow++;
    }

    //Push parameters into parameter stack
    if(_Md->Functions[FunIndex].ParmNr!=0){

      //Parameter loop
      for(i=ParmLow,j=0;i<=ParmHigh;i++,j++){

        //Skip zero length parameters (empty classes)
        if(_Md->TypLength(_Md->Parameters[i].TypIndex)==0){ continue; }

        //Parameters for dynamic library functions
        if(_Md->Functions[FunIndex].Kind==FunctionKind::DlFunc){
          if(_Md->Parameters[i].IsReference){
            if(ParmTokens[j].AdrMode!=CpuAdrMode::LitValue && _Md->Variables[ParmTokens[j].Value.VarIndex].IsReference){          
              if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::String){
                if(!_Md->Bin.AsmWriteCode(CpuInstCode::LPUSr,ParmTokens[j].Asm(true),_Md->Bin.AsmLitBol((CpuBol)_Md->Parameters[i].IsConst))){ return false; }
              }
              else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::DynArray){
                if(!_Md->Bin.AsmWriteCode(CpuInstCode::LPADr,ParmTokens[j].Asm(true),_Md->Bin.AsmLitBol((CpuBol)_Md->Parameters[i].IsConst))){ return false; }
              }
              else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::FixArray){
                if(!_Md->Bin.AsmWriteCode(CpuInstCode::LPAFr,ParmTokens[j].Asm(true),_Md->Bin.AsmLitBol((CpuBol)_Md->Parameters[i].IsConst),_Md->AsmAgx(_Md->Parameters[i].TypIndex))){ return false; }
              }
              else{
                if(!_Md->Bin.AsmWriteCode(CpuInstCode::LPUr,ParmTokens[j].Asm(true))){ return false; }
              }
            }
            else{
              if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::String){
                if(!_Md->Bin.AsmWriteCode(CpuInstCode::LRPUS,ParmTokens[j].Asm(),_Md->Bin.AsmLitBol((CpuBol)_Md->Parameters[i].IsConst))){ return false; }
              }
              else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::DynArray){
                if(!_Md->Bin.AsmWriteCode(CpuInstCode::LRPAD,ParmTokens[j].Asm(),_Md->Bin.AsmLitBol((CpuBol)_Md->Parameters[i].IsConst))){ return false; }
              }
              else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::FixArray){
                if(!_Md->Bin.AsmWriteCode(CpuInstCode::LRPAF,ParmTokens[j].Asm(),_Md->Bin.AsmLitBol((CpuBol)_Md->Parameters[i].IsConst),_Md->AsmAgx(_Md->Parameters[i].TypIndex))){ return false; }
              }
              else{
                if(!_Md->Bin.AsmWriteCode(CpuInstCode::LRPU,ParmTokens[j].Asm())){ return false; }
              }
            }
          }
          else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Boolean){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::LPUb,ParmTokens[j].Asm())){ return false; }
          }
          else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Char){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::LPUc,ParmTokens[j].Asm())){ return false; }
          }
          else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Short){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::LPUw,ParmTokens[j].Asm())){ return false; }
          }
          else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Integer){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::LPUi,ParmTokens[j].Asm())){ return false; }
          }
          else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Long){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::LPUl,ParmTokens[j].Asm())){ return false; }
          }
          else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Float){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::LPUf,ParmTokens[j].Asm())){ return false; }
          }
          else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Enum){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::LPUi,ParmTokens[j].Asm())){ return false; }
          }
          else{
            ParmTokens[j].Msg(75).Print(_Md->Parameters[i].Name,_Md->Modules[FunToken.FunModIndex].Name,_Md->Functions[FunIndex].Name);
            return false;
          }
        }

        //Parameters for local functions
        else if(_Md->Functions[FunIndex].Scope.Kind==ScopeKind::Local){
          if(_Md->Parameters[i].IsReference){
            if(ParmTokens[j].AdrMode!=CpuAdrMode::LitValue && _Md->Variables[ParmTokens[j].Value.VarIndex].IsReference){          
              if(!_Md->Bin.AsmWriteCode(CpuInstCode::MVr,_Md->AsmPar(i),ParmTokens[j].Asm(true))){ return false; }
            }
            else{
              if(!_Md->Bin.AsmWriteCode(CpuInstCode::REFER,_Md->AsmPar(i),ParmTokens[j].Asm())){ return false; }
            }
          }
          else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Boolean){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::MVb,_Md->AsmPar(i),ParmTokens[j].Asm())){ return false; }
          }
          else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Char){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::MVc,_Md->AsmPar(i),ParmTokens[j].Asm())){ return false; }
          }
          else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Short){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::MVw,_Md->AsmPar(i),ParmTokens[j].Asm())){ return false; }
          }
          else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Integer){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::MVi,_Md->AsmPar(i),ParmTokens[j].Asm())){ return false; }
          }
          else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Long){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::MVl,_Md->AsmPar(i),ParmTokens[j].Asm())){ return false; }
          }
          else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Float){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::MVf,_Md->AsmPar(i),ParmTokens[j].Asm())){ return false; }
          }
          else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Enum){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::MVi,_Md->AsmPar(i),ParmTokens[j].Asm())){ return false; }
          }
          else{
            ParmTokens[j].Msg(75).Print(_Md->Parameters[i].Name,_Md->Modules[FunToken.FunModIndex].Name,_Md->Functions[FunIndex].Name);
            return false;
          }
        }

        //Parameters for other functions
        else{
          if(_Md->Parameters[i].IsReference){
            if(ParmTokens[j].AdrMode!=CpuAdrMode::LitValue && _Md->Variables[ParmTokens[j].Value.VarIndex].IsReference){          
              if(!_Md->Bin.AsmWriteCode(CpuInstCode::PUSHr,ParmTokens[j].Asm(true))){ return false; }
            }
            else{
              if(!_Md->Bin.AsmWriteCode(CpuInstCode::REFPU,ParmTokens[j].Asm())){ return false; }
            }
          }
          else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Boolean){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::PUSHb,ParmTokens[j].Asm())){ return false; }
          }
          else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Char){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::PUSHc,ParmTokens[j].Asm())){ return false; }
          }
          else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Short){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::PUSHw,ParmTokens[j].Asm())){ return false; }
          }
          else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Integer){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::PUSHi,ParmTokens[j].Asm())){ return false; }
          }
          else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Long){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::PUSHl,ParmTokens[j].Asm())){ return false; }
          }
          else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Float){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::PUSHf,ParmTokens[j].Asm())){ return false; }
          }
          else if(_Md->Types[_Md->Parameters[i].TypIndex].MstType==MasterType::Enum){
            if(!_Md->Bin.AsmWriteCode(CpuInstCode::PUSHi,ParmTokens[j].Asm())){ return false; }
          }
          else{
            ParmTokens[j].Msg(75).Print(_Md->Parameters[i].Name,_Md->Modules[FunToken.FunModIndex].Name,_Md->Functions[FunIndex].Name);
            return false;
          }
        }

      }
      
    }
    
  }
    
  //Switch on function kind
  switch(_Md->Functions[FunIndex].Kind){
    
    //Regular function
    case FunctionKind::Function:
      if(_Md->Functions[FunIndex].IsNested){
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::CALLN,_Md->AsmFun(FunIndex))){ return false; }
      }
      else{
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::CALL,_Md->AsmFun(FunIndex))){ return false; }
      }
      if(!_Md->Functions[FunIndex].IsVoid){
        Result.IsCalculated=true;
        OpndStack.Push(Result);
      }
      else if(!IsOprStackEmpty){
        Result.IsCalculated=true;
        OpndStack.Push(Result);
      }
      break;
    
    //Member function
    case FunctionKind::Member:
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::CALL,_Md->AsmFun(FunIndex))){ return false; }
      if(!_Md->Functions[FunIndex].IsVoid){
        Result.IsCalculated=true;
        OpndStack.Push(Result);
      }
      else if(!IsOprStackEmpty){
        Result.IsCalculated=true;
        OpndStack.Push(Result);
      }
      if(_Md->Functions[FunIndex].IsInitializer && OpdToken.VarIndex()!=-1){
        _Md->Variables[OpdToken.VarIndex()].IsInitialized=true;
        DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[OpdToken.VarIndex()].Name+" in scope "+_Md->ScopeName(_Md->Variables[OpdToken.VarIndex()].Scope));
      }
      break;
    
    //Master method
    case FunctionKind::MasterMth:
      if(!_MasterMethodExecute(Scope,CodeBlockId,FunIndex,FunToken,OpdToken,ParmTokens,Result)){ return false; }
      if(!_Md->Functions[FunIndex].IsVoid){
        Result.IsCalculated=true;
        OpndStack.Push(Result);
      }
      else if(!IsOprStackEmpty){
        Result.IsCalculated=true;
        OpndStack.Push(Result);
      }
      if(_Md->Functions[FunIndex].IsInitializer && OpdToken.VarIndex()!=-1){
        _Md->Variables[OpdToken.VarIndex()].IsInitialized=true;
        DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[OpdToken.VarIndex()].Name+" in scope "+_Md->ScopeName(_Md->Variables[OpdToken.VarIndex()].Scope));
      }
      break;

    //System call
    case FunctionKind::SysCall:
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::SCALL,_Md->Bin.AsmLitInt(_Md->Functions[FunIndex].SysCallNr))){ return false; }
      if(!_Md->Functions[FunIndex].IsVoid){
        Result.IsCalculated=true;
        OpndStack.Push(Result);
      }
      else if(!IsOprStackEmpty){
        Result.IsCalculated=true;
        OpndStack.Push(Result);
      }
      break;
    
    //System function
    case FunctionKind::SysFunc:
      switch(_Md->Bin.InstArgNr(_Md->Functions[FunIndex].InstCode)){
        case 0: if(!_Md->Bin.AsmWriteCode(_Md->Functions[FunIndex].InstCode)){ return false; } break;
        case 1: if(!_Md->Bin.AsmWriteCode(_Md->Functions[FunIndex].InstCode,ParmTokens[0].Asm())){ return false; } break;
        case 2: if(!_Md->Bin.AsmWriteCode(_Md->Functions[FunIndex].InstCode,ParmTokens[0].Asm(),ParmTokens[1].Asm())){ return false; } break;
        case 3: if(!_Md->Bin.AsmWriteCode(_Md->Functions[FunIndex].InstCode,ParmTokens[0].Asm(),ParmTokens[1].Asm(),ParmTokens[2].Asm())){ return false; } break;
        case 4: if(!_Md->Bin.AsmWriteCode(_Md->Functions[FunIndex].InstCode,ParmTokens[0].Asm(),ParmTokens[1].Asm(),ParmTokens[2].Asm(),ParmTokens[3].Asm())){ return false; } break;
      }
      if(!IsOprStackEmpty){
        Result.IsCalculated=true;
        OpndStack.Push(Result);
      }
      break;

    //Dynamic library call
    case FunctionKind::DlFunc:
      DlCallId=_Md->Bin.StoreDlCall(_Md->Functions[FunIndex].DlName,_Md->Functions[FunIndex].DlFunction);
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::LCALL,_Md->Bin.AsmLitInt(DlCallId))){ return false; }
      if(!_Md->Functions[FunIndex].IsVoid){
        Result.IsCalculated=true;
        OpndStack.Push(Result);
      }
      else if(!IsOprStackEmpty){
        Result.IsCalculated=true;
        OpndStack.Push(Result);
      }
      break;
    
    //Operator functions not handled here
    case FunctionKind::Operator:
      break;
  }

  //Return code
  return true;

}

//Compile method call
bool Expression::_MasterMethodExecute(const ScopeDef& Scope,CpuLon CodeBlockId,int FunIndex,const ExprToken& FunToken,ExprToken& SelfToken,Array<ExprToken>& ParmTokens,ExprToken& Result){

  //Variables
  int FieldCount;
  AsmArg DimNrArg; 
  AsmArg CellSizeArg; 
  AsmArg GeomIndexArg;
  ExprToken Temp;
  ExprToken Last;

  //Switch on method
  switch(_Md->Functions[FunIndex].MstMethod){
    
    //Fixed array methods
    case MasterMethod::ArfDsize:      
      GeomIndexArg=_Md->AsmAgx(SelfToken.TypIndex());
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::AFGET,GeomIndexArg,ParmTokens[0].Asm(),Result.Asm())){ return false; } 
      break; 
    
    //1-dim fixed array methods
    case MasterMethod::ArfLen:      
      if(_Md->Types[SelfToken.TypIndex()].DimNr!=1){ FunToken.Msg(474).Print(); return false; }
      GeomIndexArg=_Md->AsmAgx(SelfToken.TypIndex());
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::AFGET,GeomIndexArg,_Md->Bin.AsmLitChr(1),Result.Asm())){ return false; } 
      break; 
    case MasterMethod::ArfJoin:      
      if(_Md->Types[SelfToken.TypIndex()].DimNr!=1){ FunToken.Msg(531).Print(); return false; }
      GeomIndexArg=_Md->AsmAgx(SelfToken.TypIndex());
      if(_Md->Types[_Md->Types[SelfToken.TypIndex()].ElemTypIndex].MstType==MasterType::String){ 
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::AF1SJ,Result.Asm(),SelfToken.Asm(),GeomIndexArg,ParmTokens[0].Asm())){ return false; }
      }
      else if(_Md->Types[_Md->Types[SelfToken.TypIndex()].ElemTypIndex].MstType==MasterType::Char){ 
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::AF1CJ,Result.Asm(),SelfToken.Asm(),GeomIndexArg,ParmTokens[0].Asm())){ return false; }
      }
      else{ FunToken.Msg(532).Print(); return false; }
      break;
    
    //Dynamic array methods
    case MasterMethod::ArdRsize1:      
      if(_Md->Types[SelfToken.TypIndex()].DimNr!=1){ FunToken.Msg(159).Print(ToString(_Md->Types[SelfToken.TypIndex()].DimNr),"1"); return false; }
      DimNrArg=_Md->Bin.AsmLitChr(_Md->Types[SelfToken.TypIndex()].DimNr);
      CellSizeArg=_Md->Bin.AsmLitWrd(_Md->Types[_Md->Types[SelfToken.TypIndex()].ElemTypIndex].Length);
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADDEF,SelfToken.Asm(),DimNrArg,CellSizeArg)){ return false; }
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADSET,SelfToken.Asm(),_Md->Bin.AsmLitChr(1),ParmTokens[0].Asm())){ return false; } 
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADRSZ,SelfToken.Asm())){ return false; } 
      break; 
    case MasterMethod::ArdRsize2:      
      if(_Md->Types[SelfToken.TypIndex()].DimNr!=2){ FunToken.Msg(159).Print(ToString(_Md->Types[SelfToken.TypIndex()].DimNr),"2"); return false; }
      DimNrArg=_Md->Bin.AsmLitChr(_Md->Types[SelfToken.TypIndex()].DimNr);
      CellSizeArg=_Md->Bin.AsmLitWrd(_Md->Types[_Md->Types[SelfToken.TypIndex()].ElemTypIndex].Length);
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADDEF,SelfToken.Asm(),DimNrArg,CellSizeArg)){ return false; }
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADSET,SelfToken.Asm(),_Md->Bin.AsmLitChr(1),ParmTokens[0].Asm())){ return false; } 
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADSET,SelfToken.Asm(),_Md->Bin.AsmLitChr(2),ParmTokens[1].Asm())){ return false; } 
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADRSZ,SelfToken.Asm())){ return false; } 
      break; 
    case MasterMethod::ArdRsize3:      
      if(_Md->Types[SelfToken.TypIndex()].DimNr!=3){ FunToken.Msg(159).Print(ToString(_Md->Types[SelfToken.TypIndex()].DimNr),"3"); return false; }
      DimNrArg=_Md->Bin.AsmLitChr(_Md->Types[SelfToken.TypIndex()].DimNr);
      CellSizeArg=_Md->Bin.AsmLitWrd(_Md->Types[_Md->Types[SelfToken.TypIndex()].ElemTypIndex].Length);
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADDEF,SelfToken.Asm(),DimNrArg,CellSizeArg)){ return false; }
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADSET,SelfToken.Asm(),_Md->Bin.AsmLitChr(1),ParmTokens[0].Asm())){ return false; } 
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADSET,SelfToken.Asm(),_Md->Bin.AsmLitChr(2),ParmTokens[1].Asm())){ return false; } 
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADSET,SelfToken.Asm(),_Md->Bin.AsmLitChr(3),ParmTokens[2].Asm())){ return false; } 
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADRSZ,SelfToken.Asm())){ return false; } 
      break; 
    case MasterMethod::ArdRsize4:      
      if(_Md->Types[SelfToken.TypIndex()].DimNr!=4){ FunToken.Msg(159).Print(ToString(_Md->Types[SelfToken.TypIndex()].DimNr),"4"); return false; }
      DimNrArg=_Md->Bin.AsmLitChr(_Md->Types[SelfToken.TypIndex()].DimNr);
      CellSizeArg=_Md->Bin.AsmLitWrd(_Md->Types[_Md->Types[SelfToken.TypIndex()].ElemTypIndex].Length);
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADDEF,SelfToken.Asm(),DimNrArg,CellSizeArg)){ return false; }
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADSET,SelfToken.Asm(),_Md->Bin.AsmLitChr(1),ParmTokens[0].Asm())){ return false; } 
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADSET,SelfToken.Asm(),_Md->Bin.AsmLitChr(2),ParmTokens[1].Asm())){ return false; } 
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADSET,SelfToken.Asm(),_Md->Bin.AsmLitChr(3),ParmTokens[2].Asm())){ return false; } 
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADSET,SelfToken.Asm(),_Md->Bin.AsmLitChr(4),ParmTokens[3].Asm())){ return false; } 
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADRSZ,SelfToken.Asm())){ return false; } 
      break; 
    case MasterMethod::ArdRsize5:      
      if(_Md->Types[SelfToken.TypIndex()].DimNr!=5){ FunToken.Msg(159).Print(ToString(_Md->Types[SelfToken.TypIndex()].DimNr),"5"); return false; }
      DimNrArg=_Md->Bin.AsmLitChr(_Md->Types[SelfToken.TypIndex()].DimNr);
      CellSizeArg=_Md->Bin.AsmLitWrd(_Md->Types[_Md->Types[SelfToken.TypIndex()].ElemTypIndex].Length);
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADDEF,SelfToken.Asm(),DimNrArg,CellSizeArg)){ return false; }
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADSET,SelfToken.Asm(),_Md->Bin.AsmLitChr(1),ParmTokens[0].Asm())){ return false; } 
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADSET,SelfToken.Asm(),_Md->Bin.AsmLitChr(2),ParmTokens[1].Asm())){ return false; } 
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADSET,SelfToken.Asm(),_Md->Bin.AsmLitChr(3),ParmTokens[2].Asm())){ return false; } 
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADSET,SelfToken.Asm(),_Md->Bin.AsmLitChr(4),ParmTokens[3].Asm())){ return false; } 
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADSET,SelfToken.Asm(),_Md->Bin.AsmLitChr(5),ParmTokens[4].Asm())){ return false; } 
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADRSZ,SelfToken.Asm())){ return false; } 
      break; 
    case MasterMethod::ArdDsize:      
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADGET,SelfToken.Asm(),ParmTokens[0].Asm(),Result.Asm())){ return false; } 
      break; 
    case MasterMethod::ArdReset:      
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADRST,SelfToken.Asm())){ return false; } 
      break; 
    
    //1-dim dyn array methods
    case MasterMethod::ArdLen:      
      if(_Md->Types[SelfToken.TypIndex()].DimNr!=1){ FunToken.Msg(473).Print(); return false; }
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADGET,SelfToken.Asm(),_Md->Bin.AsmLitChr(1),Result.Asm())){ return false; } 
      break;
    case MasterMethod::ArdAdd:      
      if(_Md->Types[SelfToken.TypIndex()].DimNr!=1){ FunToken.Msg(470).Print(); return false; }
      CellSizeArg=_Md->Bin.AsmLitWrd(_Md->Types[_Md->Types[SelfToken.TypIndex()].ElemTypIndex].Length);
      if(!Last.NewInd(_Md,Scope,CodeBlockId,_Md->Types[SelfToken.TypIndex()].ElemTypIndex,false,SelfToken.SrcInfo(),TempVarKind::Master)){ return false; }
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::AD1DF,SelfToken.Asm())){ return false; }
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::AD1AP,Last.Asm(true),SelfToken.Asm(),CellSizeArg)){ return false; }
      if(!CopyOperand(_Md,Last,ParmTokens[0])){ return false; }
      Last.Release();
      break;
    case MasterMethod::ArdIns:      
      if(_Md->Types[SelfToken.TypIndex()].DimNr!=1){ FunToken.Msg(475).Print(); return false; }
      CellSizeArg=_Md->Bin.AsmLitWrd(_Md->Types[_Md->Types[SelfToken.TypIndex()].ElemTypIndex].Length);
      if(!Temp.NewInd(_Md,Scope,CodeBlockId,_Md->Types[SelfToken.TypIndex()].ElemTypIndex,false,SelfToken.SrcInfo(),TempVarKind::Master)){ return false; }
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::AD1DF,SelfToken.Asm())){ return false; }
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::AD1IN,Temp.Asm(true),SelfToken.Asm(),ParmTokens[0].Asm(),CellSizeArg)){ return false; }
      if(!CopyOperand(_Md,Temp,ParmTokens[1])){ return false; }
      Temp.Release();
      break;
    case MasterMethod::ArdDel:      
      if(_Md->Types[SelfToken.TypIndex()].DimNr!=1){ FunToken.Msg(476).Print(); return false; }
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::AD1DE,SelfToken.Asm(),ParmTokens[0].Asm())){ return false; }
      break;
    case MasterMethod::ArdJoin:      
      if(_Md->Types[SelfToken.TypIndex()].DimNr!=1){ FunToken.Msg(529).Print(); return false; }
      if(_Md->Types[_Md->Types[SelfToken.TypIndex()].ElemTypIndex].MstType==MasterType::String){ 
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::AD1SJ,Result.Asm(),SelfToken.Asm(),ParmTokens[0].Asm())){ return false; }
      }
      else if(_Md->Types[_Md->Types[SelfToken.TypIndex()].ElemTypIndex].MstType==MasterType::Char){ 
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::AD1CJ,Result.Asm(),SelfToken.Asm(),ParmTokens[0].Asm())){ return false; }
      }
      else{ FunToken.Msg(530).Print(); return false; }
      break;

    //Char methods
    case MasterMethod::ChrUpper:      if(!_Md->Bin.AsmWriteCode(CpuInstCode::CUPPR,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::ChrLower:      if(!_Md->Bin.AsmWriteCode(CpuInstCode::CLOWR,Result.Asm(),SelfToken.Asm())){ return false; } break; 

    //String methods
    case MasterMethod::StrLen:        if(!_Md->Bin.AsmWriteCode(CpuInstCode::SLEN,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::StrSub:        if(!_Md->Bin.AsmWriteCode(CpuInstCode::SMID,Result.Asm(),SelfToken.Asm(),ParmTokens[0].Asm(),ParmTokens[1].Asm())){ return false; } break; 
    case MasterMethod::StrRight:      if(!_Md->Bin.AsmWriteCode(CpuInstCode::SRGHT,Result.Asm(),SelfToken.Asm(),ParmTokens[0].Asm())){ return false; } break; 
    case MasterMethod::StrLeft:       if(!_Md->Bin.AsmWriteCode(CpuInstCode::SLEFT,Result.Asm(),SelfToken.Asm(),ParmTokens[0].Asm())){ return false; } break; 
    case MasterMethod::StrCutRight:   if(!_Md->Bin.AsmWriteCode(CpuInstCode::SCUTR,Result.Asm(),SelfToken.Asm(),ParmTokens[0].Asm())){ return false; } break; 
    case MasterMethod::StrCutLeft:    if(!_Md->Bin.AsmWriteCode(CpuInstCode::SCUTL,Result.Asm(),SelfToken.Asm(),ParmTokens[0].Asm())){ return false; } break; 
    case MasterMethod::StrSearch:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::SFIND,Result.Asm(),SelfToken.Asm(),ParmTokens[0].Asm(),ParmTokens[1].Asm())){ return false; } break; 
    case MasterMethod::StrReplace:    if(!_Md->Bin.AsmWriteCode(CpuInstCode::SSUBS,Result.Asm(),SelfToken.Asm(),ParmTokens[0].Asm(),ParmTokens[1].Asm())){ return false; } break; 
    case MasterMethod::StrTrim:       if(!_Md->Bin.AsmWriteCode(CpuInstCode::STRIM,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::StrUpper:      if(!_Md->Bin.AsmWriteCode(CpuInstCode::SUPPR,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::StrLower:      if(!_Md->Bin.AsmWriteCode(CpuInstCode::SLOWR,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::StrLJust1:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::SLJUS,Result.Asm(),SelfToken.Asm(),ParmTokens[0].Asm(),_Md->Bin.AsmLitChr(' '))){ return false; } break; 
    case MasterMethod::StrRJust1:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::SRJUS,Result.Asm(),SelfToken.Asm(),ParmTokens[0].Asm(),_Md->Bin.AsmLitChr(' '))){ return false; } break; 
    case MasterMethod::StrLJust2:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::SLJUS,Result.Asm(),SelfToken.Asm(),ParmTokens[0].Asm(),ParmTokens[1].Asm())){ return false; } break; 
    case MasterMethod::StrRJust2:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::SRJUS,Result.Asm(),SelfToken.Asm(),ParmTokens[0].Asm(),ParmTokens[1].Asm())){ return false; } break; 
    case MasterMethod::StrMatch:      if(!_Md->Bin.AsmWriteCode(CpuInstCode::SMATC,Result.Asm(),SelfToken.Asm(),ParmTokens[0].Asm())){ return false; } break; 
    case MasterMethod::StrLike:       if(!_Md->Bin.AsmWriteCode(CpuInstCode::SLIKE,Result.Asm(),SelfToken.Asm(),ParmTokens[0].Asm())){ return false; } break; 
    case MasterMethod::StrReplicate:  if(!_Md->Bin.AsmWriteCode(CpuInstCode::SREPL,Result.Asm(),SelfToken.Asm(),ParmTokens[0].Asm())){ return false; } break; 
    case MasterMethod::StrSplit:      if(!_Md->Bin.AsmWriteCode(CpuInstCode::SSPLI,Result.Asm(),SelfToken.Asm(),ParmTokens[0].Asm())){ return false; } break; 
    case MasterMethod::StrStartswith: if(!_Md->Bin.AsmWriteCode(CpuInstCode::SSTWI,Result.Asm(),SelfToken.Asm(),ParmTokens[0].Asm())){ return false; } break; 
    case MasterMethod::StrEndswith:   if(!_Md->Bin.AsmWriteCode(CpuInstCode::SENWI,Result.Asm(),SelfToken.Asm(),ParmTokens[0].Asm())){ return false; } break; 
    case MasterMethod::StrIsbool:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::SISBO,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::StrIschar:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::SISCH,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::StrIsshort:    if(!_Md->Bin.AsmWriteCode(CpuInstCode::SISSH,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::StrIsint:      if(!_Md->Bin.AsmWriteCode(CpuInstCode::SISIN,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::StrIslong:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::SISLO,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::StrIsfloat:    if(!_Md->Bin.AsmWriteCode(CpuInstCode::SISFL,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    
    //Data conversion methods
    case MasterMethod::BolToshort:    if(!_Md->Bin.AsmWriteCode(CpuInstCode::BO2SH,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::BolToint:      if(!_Md->Bin.AsmWriteCode(CpuInstCode::BO2IN,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::BolTolong:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::BO2LO,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::BolTochar:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::BO2CH,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::BolTofloat:    if(!_Md->Bin.AsmWriteCode(CpuInstCode::BO2FL,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::BolTostring:   if(!_Md->Bin.AsmWriteCode(CpuInstCode::BO2ST,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::ChrTobool:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::CH2BO,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::ChrToshort:    if(!_Md->Bin.AsmWriteCode(CpuInstCode::CH2SH,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::ChrToint:      if(!_Md->Bin.AsmWriteCode(CpuInstCode::CH2IN,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::ChrTolong:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::CH2LO,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::ChrTofloat:    if(!_Md->Bin.AsmWriteCode(CpuInstCode::CH2FL,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::ChrTostring:   if(!_Md->Bin.AsmWriteCode(CpuInstCode::CH2ST,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::ChrFormat:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::CHFMT,Result.Asm(),SelfToken.Asm(),ParmTokens[0].Asm())){ return false; } break; 
    case MasterMethod::ShrTobool:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::SH2BO,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::ShrTochar:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::SH2CH,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::ShrToint:      if(!_Md->Bin.AsmWriteCode(CpuInstCode::SH2IN,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::ShrTolong:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::SH2LO,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::ShrTofloat:    if(!_Md->Bin.AsmWriteCode(CpuInstCode::SH2FL,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::ShrTostring:   if(!_Md->Bin.AsmWriteCode(CpuInstCode::SH2ST,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::ShrFormat:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::SHFMT,Result.Asm(),SelfToken.Asm(),ParmTokens[0].Asm())){ return false; } break; 
    case MasterMethod::IntTobool:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::IN2BO,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::IntTochar:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::IN2CH,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::IntToshort:    if(!_Md->Bin.AsmWriteCode(CpuInstCode::IN2SH,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::IntTolong:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::IN2LO,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::IntTofloat:    if(!_Md->Bin.AsmWriteCode(CpuInstCode::IN2FL,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::IntTostring:   if(!_Md->Bin.AsmWriteCode(CpuInstCode::IN2ST,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::IntFormat:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::INFMT,Result.Asm(),SelfToken.Asm(),ParmTokens[0].Asm())){ return false; } break; 
    case MasterMethod::LonTobool:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::LO2BO,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::LonTochar:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::LO2CH,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::LonToshort:    if(!_Md->Bin.AsmWriteCode(CpuInstCode::LO2SH,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::LonToint:      if(!_Md->Bin.AsmWriteCode(CpuInstCode::LO2IN,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::LonTofloat:    if(!_Md->Bin.AsmWriteCode(CpuInstCode::LO2FL,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::LonTostring:   if(!_Md->Bin.AsmWriteCode(CpuInstCode::LO2ST,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::LonFormat:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::LOFMT,Result.Asm(),SelfToken.Asm(),ParmTokens[0].Asm())){ return false; } break; 
    case MasterMethod::FloTobool:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::FL2BO,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::FloTochar:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::FL2CH,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::FloToshort:    if(!_Md->Bin.AsmWriteCode(CpuInstCode::FL2SH,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::FloToint:      if(!_Md->Bin.AsmWriteCode(CpuInstCode::FL2IN,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::FloTolong:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::FL2LO,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::FloTostring:   if(!_Md->Bin.AsmWriteCode(CpuInstCode::FL2ST,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::FloFormat:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::FLFMT,Result.Asm(),SelfToken.Asm(),ParmTokens[0].Asm())){ return false; } break; 
    case MasterMethod::StrTobool:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::ST2BO,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::StrTochar:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::ST2CH,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::StrToshort:    if(!_Md->Bin.AsmWriteCode(CpuInstCode::ST2SH,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::StrToint:      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ST2IN,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::StrTolong:     if(!_Md->Bin.AsmWriteCode(CpuInstCode::ST2LO,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::StrTofloat:    if(!_Md->Bin.AsmWriteCode(CpuInstCode::ST2FL,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    
    //Class methods
    case MasterMethod::ClaFieldCount: 
      if(_Md->Types[SelfToken.TypIndex()].FieldLow!=-1 && _Md->Types[SelfToken.TypIndex()].FieldHigh!=-1){
        FieldCount=_Md->FieldCount(SelfToken.TypIndex());
      }
      else{
        FieldCount=0;
      }
      Result.ThisInt(_Md,FieldCount,FunToken.SrcInfo());
      break; 
    case MasterMethod::ClaFieldNames:  
      Result.AsMetaFldNames(_Md,SelfToken.TypIndex(),FunToken.SrcInfo());
      break;
    case MasterMethod::ClaFieldTypes:
      Result.AsMetaFldTypes(_Md,SelfToken.TypIndex(),FunToken.SrcInfo());
      break;
    
    //Enum methods
    case MasterMethod::EnuTochar:     
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::IN2CH,Result.Asm(),SelfToken.Asm())){ return false; } 
      break; 
    case MasterMethod::EnuToshort:     
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::IN2SH,Result.Asm(),SelfToken.Asm())){ return false; } 
      break; 
    case MasterMethod::EnuToint:
      if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MV,Result.Asm(),SelfToken.Asm())){ return false; } 
      break; 
    case MasterMethod::EnuTolong:     
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::IN2LO,Result.Asm(),SelfToken.Asm())){ return false; } 
      break; 
    case MasterMethod::EnuTofloat:     
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::IN2FL,Result.Asm(),SelfToken.Asm())){ return false; } 
      break; 
    case MasterMethod::EnuTostring: 
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::IN2ST,Result.Asm(),SelfToken.Asm())){ return false; } 
      break; 
    case MasterMethod::EnuFieldCount: 
      FieldCount=_Md->FieldCount(SelfToken.TypIndex());
      Result.ThisInt(_Md,FieldCount,FunToken.SrcInfo());
      break; 
    case MasterMethod::EnuFieldNames:  
      Result.AsMetaFldNames(_Md,SelfToken.TypIndex(),FunToken.SrcInfo());
      break;
    case MasterMethod::EnuFieldTypes:
      Result.AsMetaFldTypes(_Md,SelfToken.TypIndex(),FunToken.SrcInfo());
      break;
    
    //Generic method .name()
    case MasterMethod::BolName:       
    case MasterMethod::ChrName:
    case MasterMethod::ShrName:
    case MasterMethod::IntName:
    case MasterMethod::LonName:
    case MasterMethod::FloName:
    case MasterMethod::StrName:
    case MasterMethod::ClaName:
    case MasterMethod::EnuName:
    case MasterMethod::ArfName:
    case MasterMethod::ArdName:
      if(SelfToken.IsLValue()){ 
        Result.AsMetaVarName(_Md,SelfToken.Value.VarIndex,FunToken.SrcInfo());
      } 
      else{ 
        SelfToken.Msg(91).Print(); 
        return false; 
      } 
      break;     
    
    //Generic method .type()
    case MasterMethod::BolType:       
    case MasterMethod::ChrType:       
    case MasterMethod::ShrType:       
    case MasterMethod::IntType:       
    case MasterMethod::LonType:       
    case MasterMethod::FloType:       
    case MasterMethod::StrType:       
    case MasterMethod::ClaType:       
    case MasterMethod::EnuType:       
    case MasterMethod::ArfType:       
    case MasterMethod::ArdType:       
      Result.AsMetaTypName(_Md,SelfToken.TypIndex(),FunToken.SrcInfo());
      break; 
    
    //Generic method .sizeof()
    case MasterMethod::BolSizeof: Result.ThisWrd(_Md,sizeof(CpuBol),FunToken.SrcInfo()); break; 
    case MasterMethod::ChrSizeof: Result.ThisWrd(_Md,sizeof(CpuChr),FunToken.SrcInfo()); break; 
    case MasterMethod::ShrSizeof: Result.ThisWrd(_Md,sizeof(CpuShr),FunToken.SrcInfo()); break; 
    case MasterMethod::IntSizeof: Result.ThisWrd(_Md,sizeof(CpuInt),FunToken.SrcInfo()); break; 
    case MasterMethod::LonSizeof: Result.ThisWrd(_Md,sizeof(CpuLon),FunToken.SrcInfo()); break; 
    case MasterMethod::FloSizeof: Result.ThisWrd(_Md,sizeof(CpuFlo),FunToken.SrcInfo()); break; 
    case MasterMethod::StrSizeof: if(!_Md->Bin.AsmWriteCode(CpuInstCode::SLEN,Result.Asm(),SelfToken.Asm())){ return false; } break; 
    case MasterMethod::EnuSizeof: Result.ThisWrd(_Md,sizeof(CpuInt),FunToken.SrcInfo()); break; 
    case MasterMethod::ArfSizeof: Result.ThisWrd(_Md,_Md->Types[SelfToken.TypIndex()].Length,FunToken.SrcInfo()); break; 
    case MasterMethod::ArdSizeof: if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADSIZ,SelfToken.Asm(),Result.Asm())){ return false; } break; 
  
    //Generic method .tobytes();
    case MasterMethod::BolToBytes: 
    case MasterMethod::ChrToBytes:
    case MasterMethod::ShrToBytes:
    case MasterMethod::IntToBytes:
    case MasterMethod::LonToBytes:
    case MasterMethod::FloToBytes:
    case MasterMethod::EnuToBytes: 
      if(SelfToken.AdrMode==CpuAdrMode::LitValue){
        if(!Temp.NewVar(_Md,Scope,CodeBlockId,SelfToken.MstType(),SelfToken.SrcInfo(),TempVarKind::Master)){ return false; }
        if(!CopyOperand(_Md,Temp,SelfToken)){ return false; }
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::TOCA,Result.Asm(),SelfToken.Asm(),_Md->Bin.AsmLitWrd(_Md->Types[SelfToken.TypIndex()].Length))){ return false; }
        Temp.Release();
      }
      else{
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::TOCA,Result.Asm(),SelfToken.Asm(),_Md->Bin.AsmLitWrd(_Md->Types[SelfToken.TypIndex()].Length))){ return false; }
      }
      break;
    case MasterMethod::StrToBytes: 
      if(SelfToken.AdrMode==CpuAdrMode::LitValue){
        if(!Temp.NewVar(_Md,Scope,CodeBlockId,SelfToken.MstType(),SelfToken.SrcInfo(),TempVarKind::Master)){ return false; }
        if(!CopyOperand(_Md,Temp,SelfToken)){ return false; }
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::STOCA,Result.Asm(),SelfToken.Asm())){ return false; }
        Temp.Release();
      }
      else{
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::STOCA,Result.Asm(),SelfToken.Asm())){ return false; } 
      }
      break;
    case MasterMethod::ClaToBytes: 
      if(_Md->HasInnerBlocks(SelfToken.TypIndex())){
        SelfToken.Msg(163).Print(_Md->CannonicalTypeName(SelfToken.TypIndex()));
        return false;
      }
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::TOCA,Result.Asm(),SelfToken.Asm(),_Md->Bin.AsmLitWrd(_Md->Types[SelfToken.TypIndex()].Length))){ return false; } 
      break; 
    case MasterMethod::ArfToBytes: 
      if(_Md->HasInnerBlocks(SelfToken.TypIndex())){
        SelfToken.Msg(163).Print(_Md->CannonicalTypeName(SelfToken.TypIndex()));
        return false;
      }
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::TOCA,Result.Asm(),SelfToken.Asm(),_Md->Bin.AsmLitWrd(_Md->Types[SelfToken.TypIndex()].Length))){ return false; } 
      break; 
    case MasterMethod::ArdToBytes: 
      if(_Md->HasInnerBlocks(SelfToken.TypIndex())){
        SelfToken.Msg(163).Print(_Md->CannonicalTypeName(SelfToken.TypIndex()));
        return false;
      }
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ATOCA,Result.Asm(),SelfToken.Asm())){ return false; } 
      break; 

    //Generic method .frombytes();
    case MasterMethod::BolFromBytes: 
    case MasterMethod::ChrFromBytes: 
    case MasterMethod::ShrFromBytes: 
    case MasterMethod::IntFromBytes: 
    case MasterMethod::LonFromBytes: 
    case MasterMethod::FloFromBytes: 
      if(!SelfToken.IsLValue() || SelfToken.IsConst){
        SelfToken.Msg(189).Print(_Md->Functions[FunIndex].Name);
        return false;
      }
      else{
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::FRCA,Result.Asm(),ParmTokens[0].Asm(),_Md->Bin.AsmLitWrd(_Md->Types[_Md->Functions[FunIndex].TypIndex].Length))){ return false; }
        if(!CopyOperand(_Md,SelfToken,Result)){return false; }
      }
      break;
    case MasterMethod::StrFromBytes: 
      if(!SelfToken.IsLValue() || SelfToken.IsConst){
        SelfToken.Msg(189).Print(_Md->Functions[FunIndex].Name);
        return false;
      }
      else{
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::SFRCA,Result.Asm(),ParmTokens[0].Asm())){ return false; }
        if(!CopyOperand(_Md,SelfToken,Result)){return false; }
      }
      break;
    case MasterMethod::EnuFromBytes: 
      if(!SelfToken.IsLValue() || SelfToken.IsConst){
        SelfToken.Msg(189).Print(_Md->Functions[FunIndex].Name);
        return false;
      }
      else{
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::FRCA,Result.Asm(),ParmTokens[0].Asm(),_Md->Bin.AsmLitWrd(_Md->Types[SelfToken.TypIndex()].Length))){ return false; }
        if(!CopyOperand(_Md,SelfToken,Result)){return false; }
      }
      break;
    case MasterMethod::ClaFromBytes: 
      if(!SelfToken.IsLValue() || SelfToken.IsConst){
        SelfToken.Msg(189).Print(_Md->Functions[FunIndex].Name);
        return false;
      }
      else if(_Md->HasInnerBlocks(SelfToken.TypIndex())){
        SelfToken.Msg(164).Print(_Md->CannonicalTypeName(SelfToken.TypIndex()));
        return false;
      }
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::FRCA,Result.Asm(),ParmTokens[0].Asm(),_Md->Bin.AsmLitWrd(_Md->Types[SelfToken.TypIndex()].Length))){ return false; }
      if(!CopyOperand(_Md,SelfToken,Result)){return false; }
      break;
    case MasterMethod::ArfFromBytes: 
      if(!SelfToken.IsLValue() || SelfToken.IsConst){
        SelfToken.Msg(189).Print(_Md->Functions[FunIndex].Name);
        return false;
      }
      else if(_Md->HasInnerBlocks(SelfToken.TypIndex())){
        SelfToken.Msg(164).Print(_Md->CannonicalTypeName(SelfToken.TypIndex()));
        return false;
      }
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::FRCA,Result.Asm(),ParmTokens[0].Asm(),_Md->Bin.AsmLitWrd(_Md->Types[SelfToken.TypIndex()].Length))){ return false; }
      if(!CopyOperand(_Md,SelfToken,Result)){return false; }
      break;
    case MasterMethod::ArdFromBytes: 
      if(!SelfToken.IsLValue() || SelfToken.IsConst){
        SelfToken.Msg(189).Print(_Md->Functions[FunIndex].Name);
        return false;
      }
      else if(_Md->HasInnerBlocks(SelfToken.TypIndex())){
        SelfToken.Msg(164).Print(_Md->CannonicalTypeName(SelfToken.TypIndex()));
        return false;
      }
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::AFRCA,Result.Asm(),ParmTokens[0].Asm())){ return false; }
      if(!CopyOperand(_Md,SelfToken,Result)){return false; }
      break;
  
  }

  //Return code
  return true;

}

//Low level operator processing
bool Expression::_LowLevelOperatorCall(const ScopeDef& Scope,CpuLon CodeBlockId,int CurrToken,Stack<ExprToken>& OpndStack,Array<TernarySeed>& Seed,ExprToken& Result){

  //Variables
  int i;
  int SeedIndex;
  String Label;
  ExprToken TernaryResult;
  ExprToken OpdToken;
  ExprToken OprToken;

  //Get operator token
  OprToken=_Tokens[CurrToken];

  //Switch on operator
  switch(OprToken.Value.LowLevelOpr){
    
    //Ternary operator question mark
    case ExprLowLevelOpr::TernaryCond:
      
      //Check stack has at least one operand
      if(OpndStack.Length()<1){ 
        OprToken.Msg(140).Print( ); 
        return false; 
      }
      
      //Get operand
      OpdToken=OpndStack.Pop(); 
      OpdToken.Release();
      
      //Check we do not have undefined variables or void results as operands
      if(OpdToken.Id()==ExprTokenId::UndefVar){
        OpdToken.Msg(446).Print(OpdToken.Value.VarName);
        return false;
      }
      else if(OpdToken.Id()==ExprTokenId::VoidRes){
        OpdToken.Msg(111).Print(OpdToken.Value.VoidFuncName);
        return false;
      }

      //Check operand is boolean
      if(OpdToken.MstType()!=MasterType::Boolean){ 
        OpdToken.Msg(141).Print( ); 
        return false; 
      }

      //Check operand is initialized
      if(!OpdToken.IsInitialized()){
        OpdToken.Msg(131).Print(OpdToken.Name());
        return false;
      }

      //Set operand as used in source
      OpdToken.SetSourceUsed(Scope,true);

      //Output jump
      Label=_TernaryLabelNameSpace+ToString(OprToken.LabelSeed,"%010i")+"FAL";
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::JMPFL,OpdToken.Asm(),_Md->AsmJmp(Label))){
        return false;
      }

      //Check label seed is not already enumerated
      SeedIndex=-1;
      for(i=0;i<Seed.Length();i++){
        if(Seed[i].LabelSeed==OprToken.LabelSeed){
          SeedIndex=i;
          break;
        }
      }
      if(SeedIndex!=-1){
        OprToken.Msg(225).Print(ToString(OprToken.LabelSeed)); 
        return false; 
      }

      //Enumerate label seed
      Seed.Add((TernarySeed){OprToken.LabelSeed,-1,false});
      break;
    
    //Ternary operator semicolon
    case ExprLowLevelOpr::TernaryMid:

      //Find label seed enumeration
      SeedIndex=-1;
      for(i=0;i<Seed.Length();i++){
        if(Seed[i].LabelSeed==OprToken.LabelSeed){
          SeedIndex=i;
          break;
        }
      }
      if(SeedIndex==-1){
        OprToken.Msg(226).Print(ToString(OprToken.LabelSeed)); 
        return false; 
      }

      //Check stack has at least one operand
      if(OpndStack.Length()<1){ 
        OprToken.Msg(227).Print(); 
        return false; 
      }
      
      //Get operand
      OpdToken=OpndStack.Pop(); 
      OpdToken.Release();

      //Check we do not have undefined variables or void results as operands
      if(OpdToken.Id()==ExprTokenId::UndefVar){
        OpdToken.Msg(446).Print(OpdToken.Value.VarName);
        return false;
      }
      else if(OpdToken.Id()==ExprTokenId::VoidRes){
        OpdToken.Msg(111).Print(OpdToken.Value.VoidFuncName);
        return false;
      }

      //Check operand is initialized
      if(!OpdToken.IsInitialized()){
        OpdToken.Msg(131).Print(OpdToken.Name());
        return false;
      }

      //Set operand as used in source
      OpdToken.SetSourceUsed(Scope,true);

      //Create new operand as ternary result
      if(!TernaryResult.NewVar(_Md,Scope,CodeBlockId,OpdToken.TypIndex(),OpdToken.SrcInfo(),Seed[SeedIndex].Reused)){ return false; }
      if(TernaryResult.VarIndex()!=OpdToken.VarIndex()){
        if(!CopyOperand(_Md,TernaryResult,OpdToken)){ return false; } 
      }
      _Md->Variables[TernaryResult.VarIndex()].IsInitialized=true;
      DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[TernaryResult.VarIndex()].Name+" in scope "+_Md->ScopeName(_Md->Variables[TernaryResult.VarIndex()].Scope));

      //Keep result temp variable varindex
      Seed[SeedIndex].VarIndex=TernaryResult.Value.VarIndex;

      //Jump to end
      Label=_TernaryLabelNameSpace+ToString(OprToken.LabelSeed,"%010i")+"END";
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::JMP,_Md->AsmJmp(Label))){
        return false;
      }

      //Set next instruction as destination for false label
      Label=_TernaryLabelNameSpace+ToString(OprToken.LabelSeed,"%010i")+"FAL";
      _Md->Bin.StoreJumpDestination(Label,_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());
      break;
    
    //Ternary operator end parenthesys
    case ExprLowLevelOpr::TernaryEnd:

      //Find label seed enumeration
      SeedIndex=-1;
      for(i=0;i<Seed.Length();i++){
        if(Seed[i].LabelSeed==OprToken.LabelSeed){
          SeedIndex=i;
          break;
        }
      }
      if(SeedIndex==-1){
        OprToken.Msg(228).Print(ToString(OprToken.LabelSeed)); 
        return false; 
      }

      //Missing ternary mid operator
      if(Seed[SeedIndex].VarIndex==-1){
        OprToken.Msg(533).Print(); 
        return false; 
      }

      //Check stack has at least one operand
      if(OpndStack.Length()<1){ 
        OprToken.Msg(229).Print( ); 
        return false; 
      }
      
      //Get operand
      OpdToken=OpndStack.Pop(); 
      OpdToken.Release();

      //Check we do not have undefined variables or void results as operands
      if(OpdToken.Id()==ExprTokenId::UndefVar){
        OpdToken.Msg(446).Print(OpdToken.Value.VarName);
        return false;
      }
      else if(OpdToken.Id()==ExprTokenId::VoidRes){
        OpdToken.Msg(111).Print(OpdToken.Value.VoidFuncName);
        return false;
      }

      //Check operand is initialized
      if(!OpdToken.IsInitialized()){
        OpdToken.Msg(131).Print(OpdToken.Name());
        return false;
      }

      //Set operand as used in source
      OpdToken.SetSourceUsed(Scope,true);

      //Check that ternary operator true and false parts return same data type
      if(OpdToken.TypIndex()!=_Md->Variables[Seed[SeedIndex].VarIndex].TypIndex){
        if(!_CompileDataTypePromotion(_Md->CurrentScope(),CodeBlockId,OpdToken,_Md->Types[_Md->Variables[Seed[SeedIndex].VarIndex].TypIndex].MstType)){ return false; };
      }

      //Reuse ternary varindex to create ternary operator result and copy
      TernaryResult.ThisVar(_Md,Seed[SeedIndex].VarIndex,OpdToken.SrcInfo());
      if(TernaryResult.VarIndex()!=OpdToken.VarIndex()){
        if(!CopyOperand(_Md,TernaryResult,OpdToken)){ return false; } 
      }

      //Set next instruction as destination for end label
      Label=_TernaryLabelNameSpace+ToString(OprToken.LabelSeed,"%010i")+"END";
      _Md->Bin.StoreJumpDestination(Label,_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());

      //Push ternary operator result into stack
      TernaryResult.IsCalculated=true;
      OpndStack.Push(TernaryResult);

      //Delete entry from seed table
      Seed.Delete(SeedIndex);
      break;

  }

  //Return code
  return true;

}

//Flow operator processing
bool Expression::_FlowOperatorCall(const ScopeDef& Scope,CpuLon CodeBlockId,int CurrToken,Stack<ExprToken>& OpndStack,Stack<FlowLabelStack>& FlowLabel){

  //Variables
  int TypIndex;
  int VarIndex;
  int ElemTypIndex;
  int ResTypIndex;
  CpuLon Label;
  String LabelStr;
  String VarName;
  String FoundObject;
  String ArrTypeName;
  ExprToken ArrToken;
  ExprToken ResArray;
  ExprToken OprToken;
  ExprToken OpdToken;
  ExprToken RefLast;
  ExprToken Result;

  //Get operator token
  OprToken=_Tokens[CurrToken];

  //Switch on operator
  switch(OprToken.Value.FlowOpr){
    
    //Flow operator ForBeg
    case ExprFlowOpr::ForBeg:

      //Create new flow label
      FlowLabel.Push((FlowLabelStack){ExprFlowLabelKind::For,OprToken.FlowLabel,SourceInfo(),-1,-1,ExprToken(),ExprToken()});
      break;

    //Flow operator ForIf
    case ExprFlowOpr::ForIf:
      
      //Check flow label stack is not empty and has correct kind
      if(FlowLabel.Length()==0 || FlowLabel.Top().Kind!=ExprFlowLabelKind::For){
        OprToken.Msg(518).Print(OprToken.Print());
        return false;
      }

      //Error if there is one result but it does not come from a calculation
      if(OpndStack.Length()==1 && _Tokens.Length()>1 && !OpndStack[0].IsCalculated){
        OpndStack[0].Msg(466).Print();
        return false;
      }
      
      //Pop from operand stack
      Result=OpndStack.Pop();
      Result.SetSourceUsed(Scope,false);

      //Set next instruction as jump destination for if label
      LabelStr=_FlowOprLabelNameSpace+ToString(OprToken.FlowLabel,"%010i")+"IF";
      _Md->Bin.StoreJumpDestination(LabelStr,_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());
      break;

    //Flow operator ForDo
    case ExprFlowOpr::ForDo:
      
      //Check flow label stack is not empty and has correct kind
      if(FlowLabel.Length()==0 || FlowLabel.Top().Kind!=ExprFlowLabelKind::For){
        OprToken.Msg(519).Print(OprToken.Print());
        return false;
      }

      //Check stack has at least one operand
      if(OpndStack.Length()<1){ 
        OprToken.Msg(461).Print(); 
        return false; 
      }
      
      //Get operand
      OpdToken=OpndStack.Pop(); 
      OpdToken.Release();

      //Check we do not have undefined variables or void results as operands
      if(OpdToken.Id()==ExprTokenId::UndefVar){
        OpdToken.Msg(446).Print(OpdToken.Value.VarName);
        return false;
      }
      else if(OpdToken.Id()==ExprTokenId::VoidRes){
        OpdToken.Msg(111).Print(OpdToken.Value.VoidFuncName);
        return false;
      }

      //Check operand is boolean
      if(OpdToken.MstType()!=MasterType::Boolean){
        OpdToken.Msg(462).Print();
        return false;
      }

      //Output conditional to return label
      LabelStr=_FlowOprLabelNameSpace+ToString(OprToken.FlowLabel,"%010i")+"RET";
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::JMPFL,OpdToken.Asm(),_Md->AsmJmp(LabelStr))){
        return false;
      }
      break;

    //Flow operator ForRet
    case ExprFlowOpr::ForRet:
      
      //Check flow label stack is not empty and has correct kind
      if(FlowLabel.Length()==0 || FlowLabel.Top().Kind!=ExprFlowLabelKind::For){
        OprToken.Msg(520).Print(OprToken.Print());
        return false;
      }

      //Error if there is one result but it does not come from a calculation
      if(OpndStack.Length()==1 && _Tokens.Length()>1 && !OpndStack[0].IsCalculated){
        OpndStack[0].Msg(468).Print();
        return false;
      }
      
      //Pop from operand stack
      Result=OpndStack.Pop();
      Result.SetSourceUsed(Scope,false);

      //Output absolute jump to if label
      LabelStr=_FlowOprLabelNameSpace+ToString(OprToken.FlowLabel,"%010i")+"IF";
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::JMP,_Md->AsmJmp(LabelStr))){
        return false;
      }

      //Set next instruction as jump destination for return label
      LabelStr=_FlowOprLabelNameSpace+ToString(OprToken.FlowLabel,"%010i")+"RET";
      _Md->Bin.StoreJumpDestination(LabelStr,_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());
      break;

    //Flow operator ForEnd
    case ExprFlowOpr::ForEnd:
      
      //Check flow label stack is not empty and has correct kind
      if(FlowLabel.Length()==0 || FlowLabel.Top().Kind!=ExprFlowLabelKind::For){
        OprToken.Msg(463).Print();
        return false;
      }

      //Hide local variables created within flow operator
      _Md->HideLocalVariables(Scope,(const Array<CpuLon>&){},FlowLabel.Top().Label,OprToken.SrcInfo());

      //Pop flow label from stack
      FlowLabel.Pop();
      
      //Check operand stack has at least one operand
      if(OpndStack.Length()==0){
        OprToken.Msg(464).Print();
        return false;
      }

      //Set result as calculated
      OpndStack[0].IsCalculated=true;
      break;

    //Flow operator ArrBeg
    case ExprFlowOpr::ArrBeg:
      
      //Push new flow label
      FlowLabel.Push((FlowLabelStack){ExprFlowLabelKind::Array,OprToken.FlowLabel,SourceInfo(),-1,-1,ExprToken(),ExprToken()});
      FlowLabel.Top().ArraySrcInfo=OprToken.SrcInfo();
      break;

    //Flow operator ArrOnvar (set on variable)
    case ExprFlowOpr::ArrOnvar:

      //Check flow label stack is not empty and has correct type
      if(FlowLabel.Length()==0 || FlowLabel.Top().Kind!=ExprFlowLabelKind::Array){
        OprToken.Msg(514).Print(OprToken.Print());
        return false;
      }

      //Check stack has at least two operands
      if(OpndStack.Length()<2){ 
        OprToken.Msg(495).Print(); 
        return false; 
      }
      
      //Get on variable operand
      OpdToken=OpndStack.Pop(); 
      OpdToken.Release();

      //Get array token
      ArrToken=OpndStack.Pop();

      //Check we have an array
      if(ArrToken.MstType()!=MasterType::FixArray && ArrToken.MstType()!=MasterType::DynArray){
        ArrToken.Msg(498).Print(); 
        return false; 
      }

      //Check we have an undeclared variable token
      if(OpdToken.Id()!=ExprTokenId::UndefVar){
        OpdToken.Msg(501).Print(); 
        return false; 
      }
      
      //Get on variable name and data type
      VarName=OpdToken.Value.VarName; 
      TypIndex=_Md->Types[ArrToken.TypIndex()].ElemTypIndex;

      //Variable must not be already defined
      if(_Md->VarSearch(VarName,Scope.ModIndex)!=-1){
        OpdToken.Msg(499).Print(VarName);
        return false;
      }

      //Dot collission check
      if(_Md->Types[TypIndex].MstType==MasterType::Class || _Md->Types[TypIndex].MstType==MasterType::Enum){
        if(!_Md->DotCollissionCheck(VarName,FoundObject)){
          OpdToken.Msg(500).Print(VarName,FoundObject);
          return false;
        }
      }

      //Store new variable
      Label=(FlowLabel.Length()==0?-1:FlowLabel.Top().Label);
      if(!_Md->ReuseVariable(Scope,CodeBlockId,Label,VarName,TypIndex,false,false,true,OpdToken.SrcInfo(),SysMsgDispatcher::GetSourceLine(),VarIndex)){ return false; }
      if(VarIndex==-1){
        _Md->CleanHidden(Scope,VarName);
        _Md->StoreVariable(Scope,CodeBlockId,Label,VarName,TypIndex,false,false,false,true,false,false,false,true,true,OpdToken.SrcInfo(),SysMsgDispatcher::GetSourceLine());
        VarIndex=_Md->Variables.Length()-1;
        _Md->Bin.AsmOutNewLine(AsmSection::Decl);
        _Md->Bin.AsmOutCommentLine(AsmSection::Decl,"Declared from expression",true);
        _Md->Bin.AsmOutVarDecl(AsmSection::Decl,(Scope.Kind!=ScopeKind::Local?true:false),false,true,false,false,VarName,
        _Md->CpuDataTypeFromMstType(_Md->Types[TypIndex].MstType),_Md->VarLength(VarIndex),_Md->Variables[VarIndex].Address,"",false,false,"","");
      }

      //Set on variable as initialized (as no initialization is required)
      _Md->Variables[VarIndex].IsInitialized=true;
      DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[VarIndex].Name+" in scope "+_Md->ScopeName(_Md->Variables[VarIndex].Scope));

      //Set array token as used
      ArrToken.SetSourceUsed(Scope,true);
      
      //Set array element data type and element variable index
      FlowLabel.Top().OrigArray=ArrToken;
      FlowLabel.Top().OnVarIndex=VarIndex;
      break;

    //Flow operator ArrIxdecl (set index variable)
    case ExprFlowOpr::ArrIxvar:

      //Check flow label stack is not empty and has correct type
      if(FlowLabel.Length()==0 || FlowLabel.Top().Kind!=ExprFlowLabelKind::Array){
        OprToken.Msg(515).Print(OprToken.Print());
        return false;
      }

      //Check stack has at least one operand
      if(OpndStack.Length()<1){ 
        OprToken.Msg(502).Print(); 
        return false; 
      }
      
      //Get on variable operand
      OpdToken=OpndStack.Pop(); 
      OpdToken.Release();

      //Use variable if it is declared
      if(OpdToken.Id()==ExprTokenId::Operand){

        //Variable must be regular variable not constant
        if(OpdToken.AdrMode!=CpuAdrMode::Address || _Md->Variables[OpdToken.VarIndex()].IsConst || _Md->Variables[OpdToken.VarIndex()].IsTempVar){
          OpdToken.Msg(503).Print(); 
          return false; 
        }

        //Variable must be word master type
        if(GetArchitecture()==64 && OpdToken.MstType()!=MasterType::Long){
          OpdToken.Msg(504).Print(_Md->CannonicalTypeName(_Md->LonTypIndex)); 
          return false; 
        } 
        if(GetArchitecture()==32 && OpdToken.MstType()!=MasterType::Integer){
          OpdToken.Msg(504).Print(_Md->CannonicalTypeName(_Md->IntTypIndex)); 
          return false; 
        } 

        //Get variable index
        VarIndex=OpdToken.VarIndex();

      }
      
      //Define variable if it is undeclared
      else if(OpdToken.Id()==ExprTokenId::UndefVar){

        //Get on variable name and data type
        VarName=OpdToken.Value.VarName; 
        TypIndex=_Md->WrdTypIndex;

        //Variable must not be already defined
        if(_Md->VarSearch(VarName,Scope.ModIndex)!=-1){
          OpdToken.Msg(505).Print(VarName);
          return false;
        }

        //Store new variable
        Label=(FlowLabel.Length()==0?-1:FlowLabel.Top().Label);
        if(!_Md->ReuseVariable(Scope,CodeBlockId,Label,VarName,TypIndex,false,false,false,OpdToken.SrcInfo(),SysMsgDispatcher::GetSourceLine(),VarIndex)){ return false; }
        if(VarIndex==-1){
          _Md->CleanHidden(Scope,VarName);
          _Md->StoreVariable(Scope,CodeBlockId,Label,VarName,TypIndex,false,false,false,false,false,false,false,true,true,OpdToken.SrcInfo(),SysMsgDispatcher::GetSourceLine());
          VarIndex=_Md->Variables.Length()-1;
          _Md->Bin.AsmOutNewLine(AsmSection::Decl);
          _Md->Bin.AsmOutCommentLine(AsmSection::Decl,"Declared from expression",true);
          _Md->Bin.AsmOutVarDecl(AsmSection::Decl,(Scope.Kind!=ScopeKind::Local?true:false),false,false,false,false,VarName,
          _Md->CpuDataTypeFromMstType(_Md->Types[TypIndex].MstType),_Md->VarLength(VarIndex),_Md->Variables[VarIndex].Address,"",false,false,"","");
        }

      }

      //Unexpected token as on variable
      else{
        OpdToken.Msg(506).Print(); 
        return false; 
      }

      //Set index variable as initialized (as no initialization is required)
      _Md->Variables[VarIndex].IsInitialized=true;
      DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[VarIndex].Name+" in scope "+_Md->ScopeName(_Md->Variables[VarIndex].Scope));

      //Set array index variable index
      FlowLabel.Top().IxVarIndex=VarIndex;
      break;

    //Flow operator ArrInit
    case ExprFlowOpr::ArrInit:

      //Check flow label stack is not empty and has correct type
      if(FlowLabel.Length()==0 || FlowLabel.Top().Kind!=ExprFlowLabelKind::Array){
        OprToken.Msg(516).Print(OprToken.Print());
        return false;
      }

      //Define data type for result array
      ElemTypIndex=_Md->Types[FlowLabel.Top().OrigArray.TypIndex()].ElemTypIndex;
      ArrTypeName=_Md->Types[ElemTypIndex].Name+"[]";
      if((ResTypIndex=_Md->TypSearch(ArrTypeName,Scope.ModIndex))==-1){
        _Md->StoreType(Scope,(SubScopeDef){SubScopeKind::None,-1},ArrTypeName,MasterType::DynArray,false,-1,false,0,1,ElemTypIndex,-1,-1,-1,-1,-1,true,FlowLabel.Top().OrigArray.SrcInfo());
        ResTypIndex=_Md->Types.Length()-1;
      }

      //Create result variable
      if(!ResArray.NewVar(_Md,Scope,CodeBlockId,ResTypIndex,FlowLabel.Top().ArraySrcInfo)){ return false; }
      FlowLabel.Top().ResArray=ResArray;

      //Send initialization instructions for fixed arrays
      if(_Md->Types[FlowLabel.Top().OrigArray.TypIndex()].MstType==MasterType::FixArray){
        LabelStr=_FlowOprLabelNameSpace+ToString(OprToken.FlowLabel,"%010i")+"EXIT";
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::AD1RS,ResArray.Asm())){ return false; }
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::AF1RW,_Md->AsmAgx(FlowLabel.Top().OrigArray.TypIndex()),(FlowLabel.Top().IxVarIndex==-1?_Md->AsmNva():_Md->AsmVad(FlowLabel.Top().IxVarIndex)),_Md->AsmJmp(LabelStr))){ return false; }
        LabelStr=_FlowOprLabelNameSpace+ToString(OprToken.FlowLabel,"%010i")+"FOR";
        _Md->Bin.StoreJumpDestination(LabelStr,_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::AF1FO,_Md->AsmVar(FlowLabel.Top().OnVarIndex),FlowLabel.Top().OrigArray.Asm(),_Md->AsmAgx(FlowLabel.Top().OrigArray.TypIndex()))){ return false; }
      }

      //Send initialization instructions for dynamic arrays
      else if(_Md->Types[FlowLabel.Top().OrigArray.TypIndex()].MstType==MasterType::DynArray){
        LabelStr=_FlowOprLabelNameSpace+ToString(OprToken.FlowLabel,"%010i")+"EXIT";
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::AD1RS,ResArray.Asm())){ return false; }
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::AD1RW,FlowLabel.Top().OrigArray.Asm(),(FlowLabel.Top().IxVarIndex==-1?_Md->AsmNva():_Md->AsmVad(FlowLabel.Top().IxVarIndex)),_Md->AsmJmp(LabelStr))){ return false; }
        LabelStr=_FlowOprLabelNameSpace+ToString(OprToken.FlowLabel,"%010i")+"FOR";
        _Md->Bin.StoreJumpDestination(LabelStr,_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::AD1FO,_Md->AsmVar(FlowLabel.Top().OnVarIndex),FlowLabel.Top().OrigArray.Asm())){ return false; }
      }

      //Resulting array is initialized
      _Md->Variables[ResArray.VarIndex()].IsInitialized=true;
      DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[ResArray.VarIndex()].Name+" in scope "+_Md->ScopeName(_Md->Variables[ResArray.VarIndex()].Scope));
      break;

    //Flow operator ArrAsif
    case ExprFlowOpr::ArrAsif:

      //Check flow label stack is not empty and has correct type
      if(FlowLabel.Length()==0 || FlowLabel.Top().Kind!=ExprFlowLabelKind::Array){
        OprToken.Msg(517).Print(OprToken.Print());
        return false;
      }

      //Check stack has at least one operand
      if(OpndStack.Length()<1){ 
        OprToken.Msg(507).Print(); 
        return false; 
      }
      
      //Get operand
      OpdToken=OpndStack.Pop(); 
      OpdToken.Release();
      OpdToken.SetSourceUsed(Scope,true);

      //Check we do not have undefined variables or void results as operands
      if(OpdToken.Id()==ExprTokenId::UndefVar){
        OpdToken.Msg(508).Print(OpdToken.Value.VarName);
        return false;
      }
      else if(OpdToken.Id()==ExprTokenId::VoidRes){
        OpdToken.Msg(111).Print(OpdToken.Value.VoidFuncName);
        return false;
      }

      //Check operand is boolean
      if(OpdToken.MstType()!=MasterType::Boolean){
        OpdToken.Msg(509).Print();
        return false;
      }

      //Output conditional instruction
      LabelStr=_FlowOprLabelNameSpace+ToString(OprToken.FlowLabel,"%010i")+"NEXT";
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::JMPFL,OpdToken.Asm(),_Md->AsmJmp(LabelStr))){
        return false;
      }
      break;

    //Flow operator ArrEnd
    case ExprFlowOpr::ArrEnd:

      //Check flow label stack is not empty and has correct type
      if(FlowLabel.Length()==0 || FlowLabel.Top().Kind!=ExprFlowLabelKind::Array){
        OprToken.Msg(513).Print();
        return false;
      }
      
      //Get result array
      ResArray=FlowLabel.Top().ResArray;

      //Check stack has at least one operand
      if(OpndStack.Length()<1){ 
        OprToken.Msg(510).Print(); 
        return false; 
      }
      
      //Get operand
      OpdToken=OpndStack.Pop(); 
      OpdToken.Release();
      OpdToken.SetSourceUsed(Scope,true);

      //Check we do not have undefined variables as operands
      if(OpdToken.Id()==ExprTokenId::UndefVar){
        OpdToken.Msg(511).Print(OpdToken.Value.VarName);
        return false;
      }
      else if(OpdToken.Id()==ExprTokenId::VoidRes){
        OpdToken.Msg(111).Print(OpdToken.Value.VoidFuncName);
        return false;
      }

      //If operand has different data type as array element we need to redefine array type
      if(OpdToken.TypIndex()!=_Md->Types[ResArray.TypIndex()].ElemTypIndex){
        ElemTypIndex=OpdToken.TypIndex();
        ArrTypeName=_Md->Types[ElemTypIndex].Name+"[]";
        if((ResTypIndex=_Md->TypSearch(ArrTypeName,Scope.ModIndex))==-1){
          _Md->StoreType(Scope,(SubScopeDef){SubScopeKind::None,-1},ArrTypeName,MasterType::DynArray,false,-1,false,0,1,ElemTypIndex,-1,-1,-1,-1,-1,true,OpdToken.SrcInfo());
          ResTypIndex=_Md->Types.Length()-1;
        }
        _Md->Variables[ResArray.VarIndex()].TypIndex=ResTypIndex;
        FlowLabel.Top().ResArray=ResArray;
      }
      
      //Output instructions
      if(!RefLast.NewInd(_Md,Scope,CodeBlockId,_Md->Types[ResArray.TypIndex()].ElemTypIndex,false,ResArray.SrcInfo())){ return false; }
      if(!_Md->Bin.AsmWriteCode(CpuInstCode::AD1AP,RefLast.Asm(true),ResArray.Asm(),_Md->Bin.AsmLitWrd(_Md->TypLength(_Md->Types[ResArray.TypIndex()].ElemTypIndex)))){ return false; }
      if(!CopyOperand(_Md,RefLast,OpdToken)){ return false; }
      LabelStr=_FlowOprLabelNameSpace+ToString(OprToken.FlowLabel,"%010i")+"NEXT";
      _Md->Bin.StoreJumpDestination(LabelStr,_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());
      LabelStr=_FlowOprLabelNameSpace+ToString(OprToken.FlowLabel,"%010i")+"FOR";
      if(_Md->Types[FlowLabel.Top().OrigArray.TypIndex()].MstType==MasterType::FixArray){
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::AF1NX,_Md->AsmAgx(FlowLabel.Top().OrigArray.TypIndex()),_Md->AsmJmp(LabelStr))){ return false; }
      }
      else{
        if(!_Md->Bin.AsmWriteCode(CpuInstCode::AD1NX,FlowLabel.Top().OrigArray.Asm(),_Md->AsmJmp(LabelStr))){ return false; }
      }
      RefLast.Release();

      //Set loop exit jump
      LabelStr=_FlowOprLabelNameSpace+ToString(OprToken.FlowLabel,"%010i")+"EXIT";
      _Md->Bin.StoreJumpDestination(LabelStr,_Md->CurrentScope().Depth,_Md->Bin.CurrentCodeAddress());

      //Release original array
      FlowLabel.Top().OrigArray.Release();
      
      //Hide local variables created within flow operator
      _Md->HideLocalVariables(Scope,(const Array<CpuLon>&){},FlowLabel.Top().Label,OprToken.SrcInfo());

      //Push result array into stack and set as calculated
      OpndStack.Push(FlowLabel.Top().ResArray);
      OpndStack[0].IsCalculated=true;

      //Pop flow label from stack
      FlowLabel.Pop();
      break;

  }

  //Return code
  return true;

}

//Expression compiler
bool Expression::_Compile(const ScopeDef& Scope,CpuLon CodeBlockId,bool ResultIsMandatory,ExprToken& Result){

  //Variables
  int i,j;
  int TypIndex;
  int VarIndex;
  int FldIndex;
  int FunIndex;
  bool Error;
  String VarName;
  String ParmStr1;
  String ParmStr2;
  String Matches;
  String FoundObject;
  Array<TernarySeed> Seed;
  Stack<ExprToken> OpndStack;
  Stack<FlowLabelStack> FlowLabel;
  ExprToken Opnd;
  ExprToken Opnd1;
  ExprToken Opnd2;
  ExprToken OpndAux;
  ExprToken RefToken;
  int DimNr;
  CpuWrd CellSize;
  bool ForceIsResultFirst;
  bool IsComputable;
  bool ReplaceResult;
  bool SkipNext;
  bool CodeGenerated;
  CpuLon Label;

  //Token loop
  for(i=0;i<_Tokens.Length();i++){
    
    //Message
    DebugMessage(DebugLevel::CmpExpression,".......... Processing token "+_Tokens[i].Print()+" ..........");
    
    //Set source on binary class
    _Md->Bin.SetSource(_Tokens[i].SrcInfo());

    //Init flags
    Error=false;
    SkipNext=false;

    //Switch on token id
    switch(_Tokens[i].Id()){
      
      //Operand
      case ExprTokenId::Operand:
        
        //Push token into stack
        OpndStack.Push(_Tokens[i]);

        //If token is a variable declared with initialization do initialization here
        if(_Tokens[i].HasInitialization){
          Opnd1.ThisVar(_Md,_Tokens[i].VarIndex(),_Tokens[i].SrcInfo());
          if(!InitOperand(_Md,Opnd1,CodeGenerated)){ Error=true; break; } 
          _Md->Variables[Opnd1.Value.VarIndex].IsInitialized=true; 
          DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[Opnd1.Value.VarIndex].Name+" in scope "+_Md->ScopeName(_Md->Variables[Opnd1.Value.VarIndex].Scope));
        }
        break;
      
      //Undefined variable
      case ExprTokenId::UndefVar:
        if((VarIndex=_Md->VarSearch(_Tokens[i].Value.VarName,_Md->CurrentScope().ModIndex))!=-1){
          Opnd=_Tokens[i];
          Opnd.Id(ExprTokenId::Operand);
          Opnd.AdrMode=CpuAdrMode::Address;
          Opnd.Value.VarIndex=VarIndex;
          OpndStack.Push(Opnd);
        }
        else{
          OpndStack.Push(_Tokens[i]);
        }
        break;
      
      //Low level operator
      case ExprTokenId::LowLevelOpr:
        if(!_LowLevelOperatorCall(Scope,CodeBlockId,i,OpndStack,Seed,Result)){ Error=true; break; }
        break;
        
      //Flow operator
      case ExprTokenId::FlowOpr:
        
        //Compile flow operator
        if(!_FlowOperatorCall(Scope,CodeBlockId,i,OpndStack,FlowLabel)){ Error=true; break; }

        //In case flow operator defined some variables we must define the same undefined variable in subsequent tokens
        if(_Tokens[i].Value.FlowOpr==ExprFlowOpr::ArrOnvar 
        || _Tokens[i].Value.FlowOpr==ExprFlowOpr::ArrIxvar){
          for(j=i;j<_Tokens.Length();j++){
            if(_Tokens[j].Id()==ExprTokenId::UndefVar){
              if(_Tokens[i].Value.FlowOpr==ExprFlowOpr::ArrOnvar && _Tokens[j].Value.VarName==_Md->Variables[FlowLabel.Top().OnVarIndex].Name){
                VarIndex=FlowLabel.Top().OnVarIndex;
                _Tokens[j].Id(ExprTokenId::Operand);
                _Tokens[j].AdrMode=(_Md->Variables[VarIndex].IsReference?CpuAdrMode::Indirection:CpuAdrMode::Address);
                _Tokens[j].Value.VarIndex=VarIndex;
                _Tokens[j].IsConst=_Md->Variables[VarIndex].IsConst;
                DebugMessage(DebugLevel::CmpExpression,"Undefined variable for array element "+_Md->Variables[VarIndex].Name
                +" becomes defined, position "+ToString(_Tokens[j].LineNr())+":"+ToString(_Tokens[j].ColNr()+1));
              }
              if(_Tokens[i].Value.FlowOpr==ExprFlowOpr::ArrIxvar 
              && FlowLabel.Top().IxVarIndex!=-1 
              && _Tokens[j].Value.VarName==_Md->Variables[FlowLabel.Top().IxVarIndex].Name){
                VarIndex=FlowLabel.Top().IxVarIndex;
                _Tokens[j].Id(ExprTokenId::Operand);
                _Tokens[j].AdrMode=(_Md->Variables[VarIndex].IsReference?CpuAdrMode::Indirection:CpuAdrMode::Address);
                _Tokens[j].Value.VarIndex=VarIndex;
                _Tokens[j].IsConst=_Md->Variables[VarIndex].IsConst;
                DebugMessage(DebugLevel::CmpExpression,"Undefined variable for array index "+_Md->Variables[VarIndex].Name
                +" becomes defined, position "+ToString(_Tokens[j].LineNr())+":"+ToString(_Tokens[j].ColNr()+1));
              }
            }
          }

        }
        break;
        
      //Operator
      case ExprTokenId::Operator:
        
        //Get operands from stack and prepare argument strings for operator overloads
        //(on operator overloads we do not use convertible index to get more control on when overload is called)
        switch(_Opr[(int)_Tokens[i].Value.Operator].Class){
          case ExprOpClass::Unary:  
            if(OpndStack.Length()<1){ _Tokens[i].Msg(69).Print(_Opr[(int)_Tokens[i].Value.Operator].Name.Trim()); return false; } 
            Opnd1=OpndStack.Pop();
            break;
          case ExprOpClass::Binary: 
            if(OpndStack.Length()<2){ _Tokens[i].Msg(70).Print(_Opr[(int)_Tokens[i].Value.Operator].Name.Trim()); return false; } 
            Opnd2=OpndStack.Pop();
            Opnd1=OpndStack.Pop();
            break;
        }

        //Error if second operand is undefined variable
        if(_Opr[(int)_Tokens[i].Value.Operator].Class==ExprOpClass::Binary && Opnd2.Id()==ExprTokenId::UndefVar){
          Opnd2.Msg(448).Print(_Opr[(int)_Tokens[i].Value.Operator].Name);
          return false;
        }

        //Declare undefined variables
        if(_Opr[(int)_Tokens[i].Value.Operator].Class==ExprOpClass::Binary 
        && _Tokens[i].Value.Operator==ExprOperator::Initializ
        && Opnd1.Id()==ExprTokenId::UndefVar){
             
            //Get variable name and data type
            VarName=Opnd1.Value.VarName; 
            TypIndex=Opnd2.TypIndex();

            //Variable must not be already defined
            if(_Md->VarSearch(VarName,Scope.ModIndex)!=-1){
              Opnd1.Msg(288).Print(VarName);
              return false;
            }

            //Dot collission check
            if(_Md->Types[TypIndex].MstType==MasterType::Class || _Md->Types[TypIndex].MstType==MasterType::Enum){
              if(!_Md->DotCollissionCheck(VarName,FoundObject)){
                Opnd1.Msg(185).Print(VarName,FoundObject);
                return false;
              }
            }

            //Store new variable
            Label=(FlowLabel.Length()==0?-1:FlowLabel.Top().Label);
            if(!_Md->ReuseVariable(Scope,CodeBlockId,Label,VarName,TypIndex,false,false,false,Opnd1.SrcInfo(),SysMsgDispatcher::GetSourceLine(),VarIndex)){ return false; }
            if(VarIndex==-1){
              _Md->CleanHidden(Scope,VarName);
              _Md->StoreVariable(Scope,CodeBlockId,Label,VarName,TypIndex,false,false,false,false,false,false,false,true,true,Opnd1.SrcInfo(),SysMsgDispatcher::GetSourceLine());
              VarIndex=_Md->Variables.Length()-1;
              _Md->Bin.AsmOutNewLine(AsmSection::Decl);
              _Md->Bin.AsmOutCommentLine(AsmSection::Decl,"Declared from expression",true);
              _Md->Bin.AsmOutVarDecl(AsmSection::Decl,(Scope.Kind!=ScopeKind::Local?true:false),false,false,false,false,VarName,
              _Md->CpuDataTypeFromMstType(_Md->Types[TypIndex].MstType),_Md->VarLength(VarIndex),_Md->Variables[VarIndex].Address,"",false,false,"","");
            }

            //Change operand
            OpndAux=Opnd1;
            Opnd1.ThisVar(_Md,VarIndex,OpndAux.SrcInfo());

        }

        //Error if first operand is undefined variable
        if(Opnd1.Id()==ExprTokenId::UndefVar){
          Opnd1.Msg(449).Print(_Opr[(int)_Tokens[i].Value.Operator].Name);
          return false;
        }

        //Error if first operand is void result when operator is not the sequence operator or second operand is void result
        if(Opnd1.Id()==ExprTokenId::VoidRes && _Tokens[i].Value.Operator!=ExprOperator::SeqOper){
          Opnd1.Msg(111).Print(Opnd1.Value.VoidFuncName);
          return false;
        }
        if(_Opr[(int)_Tokens[i].Value.Operator].Class==ExprOpClass::Binary && Opnd2.Id()==ExprTokenId::VoidRes){
          Opnd2.Msg(111).Print(Opnd2.Value.VoidFuncName);
          return false;
        }

        //Set operands as released (cannot do just after Pop since unused vars have VarIndex==-1)
        Opnd1.Release();
        if(_Opr[(int)_Tokens[i].Value.Operator].Class==ExprOpClass::Binary){ 
          Opnd2.Release(); 
        }

        //Set operands as used in source
        //For first operand we do this only when operator is not last one, since in that case result is not further used, or when expression has mandatory result
        //For second operand we add also the special case for operators on which result is first operand, a+=b is the same as a=a+b, this implies that b is always used in a+=b
        if(_Opr[(int)_Tokens[i].Value.Operator].Class==ExprOpClass::Unary || _Opr[(int)_Tokens[i].Value.Operator].Class==ExprOpClass::Binary){
          if(_Tokens[i].Value.Operator!=ExprOperator::Initializ && _Tokens[i].Value.Operator!=ExprOperator::SeqOper){
            Opnd1.SetSourceUsed(Scope,((i<_Tokens.Length()-1?true:false)|ResultIsMandatory) && !_Opr[(int)_Tokens[i].Value.Operator].IsResultSecond);
          }
        }
        if(_Opr[(int)_Tokens[i].Value.Operator].Class==ExprOpClass::Binary){
          Opnd2.SetSourceUsed(Scope,(i<_Tokens.Length()-1?true:false)|ResultIsMandatory|_Opr[(int)_Tokens[i].Value.Operator].IsResultFirst);
        }

        //Find operator overload function
        switch(_Opr[(int)_Tokens[i].Value.Operator].Class){
          case ExprOpClass::Unary:  
            ParmStr1=(Opnd1.Id()==ExprTokenId::VoidRes?"void":_Md->Types[_Md->WordTypeFilter(Opnd1.TypIndex(),true)].Name); 
            break;
          case ExprOpClass::Binary: 
            ParmStr1=
            (Opnd1.Id()==ExprTokenId::VoidRes?"void":_Md->Types[_Md->WordTypeFilter(Opnd1.TypIndex(),true)].Name)+","+
            (Opnd2.Id()==ExprTokenId::VoidRes?"void":_Md->Types[_Md->WordTypeFilter(Opnd2.TypIndex(),true)].Name); 
            break;
        }
        FunIndex=_Md->OprSearch(_Md->GetOperatorFuncName(_Opr[(int)_Tokens[i].Value.Operator].Name),ParmStr1,"",&Matches);
        
        //Call operator overload function if there is one defined
        if(FunIndex!=-1){
          DebugMessage(DebugLevel::CmpExpression,"Operator overload found: "+_Md->FunctionSearchName(FunIndex)+"("+ParmStr1+")");
          if(!_OperatorCall(Scope,CodeBlockId,FunIndex,_Tokens[i],Opnd1,Opnd2,(i==_Tokens.Length()-1?true:false),Result)){ Error=true; break; }
        }

        //Send error if there is ambiguity in operator overload functions
        else if(FunIndex==-1 && Matches.Length()!=0){
          _Tokens[i].Msg(290).Print(_Md->GetOperatorFuncName(_Opr[(int)_Tokens[i].Value.Operator].Name),ParmStr1,Matches);
          return false;
        }

        //Call normal operator
        else{

          //Determine operation is computable
          IsComputable=false;
          if(_Tokens[i].IsComputableOperator()){
            switch(_Opr[(int)_Tokens[i].Value.Operator].Class){
              case ExprOpClass::Unary : if(Opnd1.IsComputableOperand()){ IsComputable=true; } break;
              case ExprOpClass::Binary: if(Opnd1.IsComputableOperand() && Opnd2.IsComputableOperand()){ IsComputable=true; } break;
            }
          }

          //Compute operation on-the-fly insteadof generaring any instructions
          if(IsComputable){
            if(!_ComputeOperation(Scope,_Tokens[i],Opnd1,Opnd2,Result)){ Error=true; break; }
          }

          //Compile operation
          else{

            //Special case for type cast operator: Check if master type changes, if not result is first operand
            if(_Tokens[i].Value.Operator==ExprOperator::TypeCast && _Md->Types[_Tokens[i].CastTypIndex].MstType==Opnd1.MstType()
            && (Opnd1.MstType()==MasterType::Boolean || Opnd1.MstType()==MasterType::Char || Opnd1.MstType()==MasterType::Short || Opnd1.MstType()==MasterType::Integer
            ||  Opnd1.MstType()==MasterType::Long || Opnd1.MstType()==MasterType::Float || Opnd1.MstType()==MasterType::String)){
              ForceIsResultFirst=true;
            }
            else{
              ForceIsResultFirst=false;
            }

            //Prepare operation
            if(!_PrepareCompileOperation(Scope,CodeBlockId,_Tokens[i],Opnd1,Opnd2,Result,ForceIsResultFirst)){ Error=true; break; }

            //Result operand is replaced by next operand in the stack when next operator is assign
            //For simple master types, master type of result is checked against next operand, if not type index
            if(!_Opr[(int)_Tokens[i].Value.Operator].IsResultFirst && !_Opr[(int)_Tokens[i].Value.Operator].IsResultSecond && !ForceIsResultFirst 
            && i+1<=_Tokens.Length()-1 && _Tokens[i+1].Id()==ExprTokenId::Operator && _Tokens[i+1].Value.Operator==ExprOperator::Assign 
            && OpndStack.Length()>=1 && OpndStack.Top().IsLValue()){
              ReplaceResult=false;
              OpndAux=OpndStack.Top();
              if((OpndAux.MstType()==MasterType::Boolean || OpndAux.MstType()==MasterType::Char || OpndAux.MstType()==MasterType::Short || OpndAux.MstType()==MasterType::Integer
              ||  OpndAux.MstType()==MasterType::Long || OpndAux.MstType()==MasterType::Float || OpndAux.MstType()==MasterType::String)){
                if(Result.MstType()==OpndAux.MstType()){ ReplaceResult=true; }
              }
              else{
                if(Result.TypIndex()==OpndAux.TypIndex()){ ReplaceResult=true; }
              }
              if(ReplaceResult){
                Result.Release();
                OpndAux.Release();
                Result=OpndAux;
                OpndStack.Pop();
                SkipNext=true;
                if(OpndAux.AdrMode==CpuAdrMode::Address){ 
                  _Md->Variables[OpndAux.Value.VarIndex].IsInitialized=true; 
                  DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[OpndAux.Value.VarIndex].Name+" in scope "+_Md->ScopeName(_Md->Variables[OpndAux.Value.VarIndex].Scope));
                }
                DebugMessage(DebugLevel::CmpExpression,"Operand "+OpndAux.Print()+" is re-used as result for operator "+_Opr[(int)_Tokens[i].Value.Operator].Name);
              }
            }
            
            //Replace litteral values for variables (only wen we do not have an asignment, since there we can use LOAD* instructions)
            if(_Tokens[i].Value.Operator!=ExprOperator::Initializ && _Tokens[i].Value.Operator!=ExprOperator::Assign){
              
            }

            //Complete operation
            switch(_Tokens[i].Value.Operator){
              
              //Unary operators
              case ExprOperator::UnaryMinus: if(!_Md->Bin.AsmWriteCode(CpuMetaInst::NEG, Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
              case ExprOperator::PostfixInc: if(!_Md->Bin.AsmWriteCode(CpuMetaInst::PINC,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
              case ExprOperator::PostfixDec: if(!_Md->Bin.AsmWriteCode(CpuMetaInst::PDEC,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
              case ExprOperator::PrefixInc:  if(!_Md->Bin.AsmWriteCode(CpuMetaInst::INC, Opnd1.Asm())){ Error=true; break; } break;
              case ExprOperator::PrefixDec:  if(!_Md->Bin.AsmWriteCode(CpuMetaInst::DEC, Opnd1.Asm())){ Error=true; break; } break;
              case ExprOperator::BitwiseNot: if(!_Md->Bin.AsmWriteCode(CpuMetaInst::BNOT,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
              
              //Binary operators (replaced by corresponding assignment operators when one operant matches result)
              case ExprOperator::Multiplication: 
                if(     _SameOperand(Result,Opnd1)){ if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVMU,Result.Asm(),Opnd2.Asm())){ Error=true; break; } }
                else if(_SameOperand(Result,Opnd2)){ if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVMU,Result.Asm(),Opnd1.Asm())){ Error=true; break; } }
                else{                                if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MUL ,Result.Asm(),Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } }
                break;
              case ExprOperator::Division:       
                if(     _SameOperand(Result,Opnd1)){ if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVDI,Result.Asm(),Opnd2.Asm())){ Error=true; break; } }
                else{                                if(!_Md->Bin.AsmWriteCode(CpuMetaInst::DIV ,Result.Asm(),Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } }
                break;
              case ExprOperator::Addition:       
                if(Result.MstType()==MasterType::String){
                  if(     _SameOperand(Result,Opnd1)){ if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVAD, Result.Asm(),Opnd2.Asm())){ Error=true; break; } }
                  else if(_SameOperand(Result,Opnd2)){ if(!_Md->Bin.AsmWriteCode(CpuInstCode::SMVRC,Result.Asm(),Opnd1.Asm())){ Error=true; break; } }
                  else{                                if(!_Md->Bin.AsmWriteCode(CpuMetaInst::ADD , Result.Asm(),Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } }
                }
                else{
                  if(     _SameOperand(Result,Opnd1)){ if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVAD,Result.Asm(),Opnd2.Asm())){ Error=true; break; } }
                  else if(_SameOperand(Result,Opnd2)){ if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVAD,Result.Asm(),Opnd1.Asm())){ Error=true; break; } }
                  else{                                if(!_Md->Bin.AsmWriteCode(CpuMetaInst::ADD ,Result.Asm(),Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } }
                }
                break;
              case ExprOperator::Substraction:   
                if(     _SameOperand(Result,Opnd1)){ if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVSU,Result.Asm(),Opnd2.Asm())){ Error=true; break; } }
                else{                                if(!_Md->Bin.AsmWriteCode(CpuMetaInst::SUB ,Result.Asm(),Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } }
                break;
              case ExprOperator::Modulus:        
                if(     _SameOperand(Result,Opnd1)){ if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVMO,Result.Asm(),Opnd2.Asm())){ Error=true; break; } }
                else{                                if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MOD ,Result.Asm(),Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } }
                break;
              case ExprOperator::ShiftLeft:      
                if(     _SameOperand(Result,Opnd1)){ if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVSL,Result.Asm(),Opnd2.Asm())){ Error=true; break; } }
                else{                                if(!_Md->Bin.AsmWriteCode(CpuMetaInst::SHL ,Result.Asm(),Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } }
                break;
              case ExprOperator::ShiftRight:     
                if(     _SameOperand(Result,Opnd1)){ if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVSR,Result.Asm(),Opnd2.Asm())){ Error=true; break; } }
                else{                                if(!_Md->Bin.AsmWriteCode(CpuMetaInst::SHR ,Result.Asm(),Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } }
                break;
              case ExprOperator::BitwiseAnd:     
                if(     _SameOperand(Result,Opnd1)){ if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVAN,Result.Asm(),Opnd2.Asm())){ Error=true; break; } }
                else if(_SameOperand(Result,Opnd2)){ if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVAN,Result.Asm(),Opnd1.Asm())){ Error=true; break; } }
                else{                                if(!_Md->Bin.AsmWriteCode(CpuMetaInst::BAND,Result.Asm(),Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } }
                break;
              case ExprOperator::BitwiseXor:     
                if(     _SameOperand(Result,Opnd1)){ if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVXO,Result.Asm(),Opnd2.Asm())){ Error=true; break; } }
                else if(_SameOperand(Result,Opnd2)){ if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVXO,Result.Asm(),Opnd1.Asm())){ Error=true; break; } }
                else{                                if(!_Md->Bin.AsmWriteCode(CpuMetaInst::BXOR,Result.Asm(),Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } }
                break;
              case ExprOperator::BitwiseOr:      
                if(     _SameOperand(Result,Opnd1)){ if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVOR,Result.Asm(),Opnd2.Asm())){ Error=true; break; } }
                else if(_SameOperand(Result,Opnd2)){ if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVOR,Result.Asm(),Opnd1.Asm())){ Error=true; break; } }
                else{                                if(!_Md->Bin.AsmWriteCode(CpuMetaInst::BOR ,Result.Asm(),Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } }
                break;
              
              //Operate and assign operators
              case ExprOperator::AddAssign: if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVAD,Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } break;
              case ExprOperator::SubAssign: if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVSU,Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } break;
              case ExprOperator::MulAssign: if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVMU,Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } break;
              case ExprOperator::DivAssign: if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVDI,Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } break;
              case ExprOperator::ModAssign: if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVMO,Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } break;
              case ExprOperator::ShlAssign: if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVSL,Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } break;
              case ExprOperator::ShrAssign: if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVSR,Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } break;
              case ExprOperator::AndAssign: if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVAN,Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } break;
              case ExprOperator::XorAssign: if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVXO,Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } break;
              case ExprOperator::OrAssign:  if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MVOR,Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } break;

              //Comparison operators
              case ExprOperator::Less:         if(!_Md->Bin.AsmWriteCode(CpuMetaInst::LES ,2,Result.Asm(),Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } break;
              case ExprOperator::LessEqual:    if(!_Md->Bin.AsmWriteCode(CpuMetaInst::LEQ ,2,Result.Asm(),Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } break;
              case ExprOperator::Greater:      if(!_Md->Bin.AsmWriteCode(CpuMetaInst::GRE ,2,Result.Asm(),Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } break;
              case ExprOperator::GreaterEqual: if(!_Md->Bin.AsmWriteCode(CpuMetaInst::GEQ ,2,Result.Asm(),Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } break;
              case ExprOperator::Equal:        if(!_Md->Bin.AsmWriteCode(CpuMetaInst::EQU ,2,Result.Asm(),Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } break;
              case ExprOperator::Distinct:     if(!_Md->Bin.AsmWriteCode(CpuMetaInst::DIS ,2,Result.Asm(),Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } break;

              //Logical operators
              case ExprOperator::LogicalNot: if(!_Md->Bin.AsmWriteCode(CpuInstCode::LNOT,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
              case ExprOperator::LogicalAnd: if(!_Md->Bin.AsmWriteCode(CpuInstCode::LAND,Result.Asm(),Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } break;
              case ExprOperator::LogicalOr:  if(!_Md->Bin.AsmWriteCode(CpuInstCode::LOR ,Result.Asm(),Opnd1.Asm(),Opnd2.Asm())){ Error=true; break; } break;

              //Next operation (comma operator)
              case ExprOperator::SeqOper: Result=Opnd2; Result.Lock(); break;

              //Type cast
              case ExprOperator::TypeCast:
                switch(_Md->Types[_Tokens[i].CastTypIndex].MstType){
                  case MasterType::Boolean: 
                    switch(Opnd1.MstType()){
                      case MasterType::Boolean : Result=Opnd1; Result.Lock(); break;
                      case MasterType::Char    : if(!_Md->Bin.AsmWriteCode(CpuInstCode::CH2BO,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Short   : if(!_Md->Bin.AsmWriteCode(CpuInstCode::SH2BO,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Integer : if(!_Md->Bin.AsmWriteCode(CpuInstCode::IN2BO,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Long    : if(!_Md->Bin.AsmWriteCode(CpuInstCode::LO2BO,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Float   : if(!_Md->Bin.AsmWriteCode(CpuInstCode::FL2BO,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::String  : if(!_Md->Bin.AsmWriteCode(CpuInstCode::ST2BO,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Enum    :
                      case MasterType::Class   :
                      case MasterType::FixArray:
                      case MasterType::DynArray:
                        Opnd1.Msg(166).Print(_Md->CannonicalTypeName(Opnd1.TypIndex()),_Md->CannonicalTypeName(_Tokens[i].CastTypIndex));
                        Error=true;
                        break;
                    }
                    break;
                  case MasterType::Char: 
                    switch(Opnd1.MstType()){
                      case MasterType::Boolean : if(!_Md->Bin.AsmWriteCode(CpuInstCode::BO2CH,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Char    : Result=Opnd1; Result.Lock(); break;
                      case MasterType::Short   : if(!_Md->Bin.AsmWriteCode(CpuInstCode::SH2CH,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Integer : if(!_Md->Bin.AsmWriteCode(CpuInstCode::IN2CH,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Long    : if(!_Md->Bin.AsmWriteCode(CpuInstCode::LO2CH,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Float   : if(!_Md->Bin.AsmWriteCode(CpuInstCode::FL2CH,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::String  : if(!_Md->Bin.AsmWriteCode(CpuInstCode::ST2CH,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Enum    :
                      case MasterType::Class   :
                      case MasterType::FixArray:
                      case MasterType::DynArray:
                        Opnd1.Msg(166).Print(_Md->CannonicalTypeName(Opnd1.TypIndex()),_Md->CannonicalTypeName(_Tokens[i].CastTypIndex));
                        Error=true;
                        break;
                    }
                    break;
                  case MasterType::Short: 
                    switch(Opnd1.MstType()){
                      case MasterType::Boolean : if(!_Md->Bin.AsmWriteCode(CpuInstCode::BO2SH,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Char    : if(!_Md->Bin.AsmWriteCode(CpuInstCode::CH2SH,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Short   : Result=Opnd1; Result.Lock(); break;
                      case MasterType::Integer : if(!_Md->Bin.AsmWriteCode(CpuInstCode::IN2SH,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Long    : if(!_Md->Bin.AsmWriteCode(CpuInstCode::LO2SH,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Float   : if(!_Md->Bin.AsmWriteCode(CpuInstCode::FL2SH,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::String  : if(!_Md->Bin.AsmWriteCode(CpuInstCode::ST2SH,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Enum    :
                      case MasterType::Class   :
                      case MasterType::FixArray:
                      case MasterType::DynArray:
                        Opnd1.Msg(166).Print(_Md->CannonicalTypeName(Opnd1.TypIndex()),_Md->CannonicalTypeName(_Tokens[i].CastTypIndex));
                        Error=true;
                        break;
                    }
                    break;
                  case MasterType::Integer: 
                    switch(Opnd1.MstType()){
                      case MasterType::Boolean : if(!_Md->Bin.AsmWriteCode(CpuInstCode::BO2IN,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Char    : if(!_Md->Bin.AsmWriteCode(CpuInstCode::CH2IN,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Short   : if(!_Md->Bin.AsmWriteCode(CpuInstCode::SH2IN,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Integer : Result=Opnd1; Result.Lock(); break;
                      case MasterType::Long    : if(!_Md->Bin.AsmWriteCode(CpuInstCode::LO2IN,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Float   : if(!_Md->Bin.AsmWriteCode(CpuInstCode::FL2IN,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::String  : if(!_Md->Bin.AsmWriteCode(CpuInstCode::ST2IN,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Enum    : if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MV,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Class   :
                      case MasterType::FixArray:
                      case MasterType::DynArray:
                        Opnd1.Msg(166).Print(_Md->CannonicalTypeName(Opnd1.TypIndex()),_Md->CannonicalTypeName(_Tokens[i].CastTypIndex));
                        Error=true;
                        break;
                    }
                    break;
                  case MasterType::Long: 
                    switch(Opnd1.MstType()){
                      case MasterType::Boolean : if(!_Md->Bin.AsmWriteCode(CpuInstCode::BO2LO,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Char    : if(!_Md->Bin.AsmWriteCode(CpuInstCode::CH2LO,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Short   : if(!_Md->Bin.AsmWriteCode(CpuInstCode::SH2LO,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Integer : if(!_Md->Bin.AsmWriteCode(CpuInstCode::IN2LO,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Long    : Result=Opnd1; Result.Lock(); break;
                      case MasterType::Float   : if(!_Md->Bin.AsmWriteCode(CpuInstCode::FL2LO,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::String  : if(!_Md->Bin.AsmWriteCode(CpuInstCode::ST2LO,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Enum    :
                      case MasterType::Class   :
                      case MasterType::FixArray:
                      case MasterType::DynArray:
                        Opnd1.Msg(166).Print(_Md->CannonicalTypeName(Opnd1.TypIndex()),_Md->CannonicalTypeName(_Tokens[i].CastTypIndex));
                        Error=true;
                        break;
                    }
                    break;
                  case MasterType::Float  : 
                    switch(Opnd1.MstType()){
                      case MasterType::Boolean : if(!_Md->Bin.AsmWriteCode(CpuInstCode::BO2FL,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Char    : if(!_Md->Bin.AsmWriteCode(CpuInstCode::CH2FL,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Short   : if(!_Md->Bin.AsmWriteCode(CpuInstCode::SH2FL,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Integer : if(!_Md->Bin.AsmWriteCode(CpuInstCode::IN2FL,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Long    : if(!_Md->Bin.AsmWriteCode(CpuInstCode::LO2FL,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Float   : Result=Opnd1; Result.Lock(); break;
                      case MasterType::String  : if(!_Md->Bin.AsmWriteCode(CpuInstCode::ST2FL,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Enum    :
                      case MasterType::Class   :
                      case MasterType::FixArray:
                      case MasterType::DynArray:
                        Opnd1.Msg(166).Print(_Md->CannonicalTypeName(Opnd1.TypIndex()),_Md->CannonicalTypeName(_Tokens[i].CastTypIndex));
                        Error=true;
                        break;
                    }
                    break;
                  case MasterType::String : 
                    switch(Opnd1.MstType()){
                      case MasterType::Boolean : if(!_Md->Bin.AsmWriteCode(CpuInstCode::BO2ST,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Char    : if(!_Md->Bin.AsmWriteCode(CpuInstCode::CH2ST,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Short   : if(!_Md->Bin.AsmWriteCode(CpuInstCode::SH2ST,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Integer : if(!_Md->Bin.AsmWriteCode(CpuInstCode::IN2ST,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Long    : if(!_Md->Bin.AsmWriteCode(CpuInstCode::LO2ST,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Float   : if(!_Md->Bin.AsmWriteCode(CpuInstCode::FL2ST,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::String  : Result=Opnd1; Result.Lock(); break;
                      case MasterType::Enum    :
                      case MasterType::Class   :
                      case MasterType::FixArray:
                      case MasterType::DynArray:
                        Opnd1.Msg(166).Print(_Md->CannonicalTypeName(Opnd1.TypIndex()),_Md->CannonicalTypeName(_Tokens[i].CastTypIndex));
                        Error=true;
                        break;
                    }
                    break;
                  case MasterType::FixArray : 
                    if(Opnd1.MstType()!=MasterType::FixArray && Opnd1.MstType()!=MasterType::DynArray){
                      Opnd1.Msg(166).Print(_Md->CannonicalTypeName(Opnd1.TypIndex()),_Md->CannonicalTypeName(_Tokens[i].CastTypIndex));
                      Error=true;
                      break;
                    }
                    if(_Md->Types[_Tokens[i].CastTypIndex].ElemTypIndex!=_Md->Types[Opnd1.TypIndex()].ElemTypIndex){
                      _Tokens[i].Msg(167).Print();
                      Error=true; 
                      break; 
                    }
                    if(Opnd1.MstType()==MasterType::FixArray){
                      if(!_Md->Bin.AsmWriteCode(CpuInstCode::AF2F,Result.Asm(),_Md->AsmAgx(_Tokens[i].CastTypIndex),Opnd1.Asm(),_Md->AsmAgx(Opnd1.TypIndex()))){ Error=true; break; }
                    }
                    else if(Opnd1.MstType()==MasterType::DynArray){
                      if(!_Md->Bin.AsmWriteCode(CpuInstCode::AD2F,Result.Asm(),_Md->AsmAgx(_Tokens[i].CastTypIndex),Opnd1.Asm())){ Error=true; break; }
                    }
                    break;
                  case MasterType::DynArray : 
                    if(Opnd1.MstType()!=MasterType::FixArray && Opnd1.MstType()!=MasterType::DynArray){
                      Opnd1.Msg(166).Print(_Md->CannonicalTypeName(Opnd1.TypIndex()),_Md->CannonicalTypeName(_Tokens[i].CastTypIndex));
                      Error=true;
                      break;
                    }
                    if(_Md->Types[_Tokens[i].CastTypIndex].ElemTypIndex!=_Md->Types[Opnd1.TypIndex()].ElemTypIndex){
                      _Tokens[i].Msg(167).Print();
                      Error=true; 
                      break; 
                    }
                    if(Opnd1.MstType()==MasterType::FixArray){
                      DimNr=_Md->Types[_Tokens[i].CastTypIndex].DimNr;
                      CellSize=_Md->Types[_Md->Types[_Tokens[i].CastTypIndex].ElemTypIndex].Length;
                      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADDEF,Result.Asm(),_Md->Bin.AsmLitChr(DimNr),_Md->Bin.AsmLitWrd(CellSize))){ Error=true; break; }
                      if(!_Md->Bin.AsmWriteCode(CpuInstCode::AF2D,Result.Asm(),Opnd1.Asm(),_Md->AsmAgx(Opnd1.TypIndex()))){ Error=true; break; }
                    }
                    else if(Opnd1.MstType()==MasterType::DynArray){
                      DimNr=_Md->Types[_Tokens[i].CastTypIndex].DimNr;
                      CellSize=_Md->Types[_Md->Types[_Tokens[i].CastTypIndex].ElemTypIndex].Length;
                      if(!_Md->Bin.AsmWriteCode(CpuInstCode::ADDEF,Result.Asm(),_Md->Bin.AsmLitChr(DimNr),_Md->Bin.AsmLitWrd(CellSize))){ Error=true; break; }
                      if(!_Md->Bin.AsmWriteCode(CpuInstCode::AD2D,Result.Asm(),Opnd1.Asm())){ Error=true; break; }
                    }
                    break;
                  case MasterType::Enum: 
                    switch(Opnd1.MstType()){
                      case MasterType::Integer : if(!_Md->Bin.AsmWriteCode(CpuMetaInst::MV,Result.Asm(),Opnd1.Asm())){ Error=true; break; } break;
                      case MasterType::Boolean : 
                      case MasterType::Char    : 
                      case MasterType::Short   : 
                      case MasterType::Long    : 
                      case MasterType::Float   : 
                      case MasterType::String  : 
                      case MasterType::Enum    : 
                      case MasterType::Class   :
                      case MasterType::FixArray:
                      case MasterType::DynArray:
                        Opnd1.Msg(166).Print(_Md->CannonicalTypeName(Opnd1.TypIndex()),_Md->CannonicalTypeName(_Tokens[i].CastTypIndex));
                        Error=true;
                        break;
                    }
                    break;
                  case MasterType::Class: 
                    break;
                }
                break;

              //Unary plus (nothing to do)
              case ExprOperator::UnaryPlus:
                break;
              
              //Initialization && Assignment
              case ExprOperator::Initializ:
              case ExprOperator::Assign:
                if(!CopyOperand(_Md,Opnd1,Opnd2)){ Error=true; break; } 
                if(Opnd1.AdrMode==CpuAdrMode::Address){ 
                  _Md->Variables[Opnd1.Value.VarIndex].IsInitialized=true; 
                  DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[Opnd1.Value.VarIndex].Name+" in scope "+_Md->ScopeName(_Md->Variables[Opnd1.Value.VarIndex].Scope));
                }
                break;
            
            }

          }

        }

        //Error exit
        if(Error){ break; }

        //Set initialization of variables in case of assignment operator
        if(_Opr[(int)_Tokens[i].Value.Operator].SubClass==ExprOpSubClass::Assignment
        && Opnd1.Id()==ExprTokenId::Operand && Opnd1.AdrMode!=CpuAdrMode::LitValue){
          _Md->Variables[Opnd1.Value.VarIndex].IsInitialized=true;
          DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[Opnd1.Value.VarIndex].Name+" in scope "+_Md->ScopeName(_Md->Variables[Opnd1.Value.VarIndex].Scope));
        }

        //Result of operation is initialized also
        if(_Opr[(int)_Tokens[i].Value.Operator].IsResultFirst){
          if(Opnd1.VarIndex()!=-1){ 
            _Md->Variables[Opnd1.VarIndex()].IsInitialized=true; 
            DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[Opnd1.Value.VarIndex].Name+" in scope "+_Md->ScopeName(_Md->Variables[Opnd1.Value.VarIndex].Scope));
          }
        }
        else if(_Opr[(int)_Tokens[i].Value.Operator].IsResultSecond){
          if(Opnd2.VarIndex()!=-1){ 
            _Md->Variables[Opnd2.VarIndex()].IsInitialized=true; 
            DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[Opnd2.Value.VarIndex].Name+" in scope "+_Md->ScopeName(_Md->Variables[Opnd2.Value.VarIndex].Scope));
          }
        }
        else{
          if(Result.VarIndex()!=-1){ 
            _Md->Variables[Result.VarIndex()].IsInitialized=true; 
            DebugMessage(DebugLevel::CmpExpression,"Initialized flag set for variable "+_Md->Variables[Result.VarIndex()].Name+" in scope "+_Md->ScopeName(_Md->Variables[Result.VarIndex()].Scope));
          }
        }

        //Push result to operand stack
        if(_Opr[(int)_Tokens[i].Value.Operator].IsResultFirst){
          Opnd1.Lock();
          Opnd1.IsCalculated=true;
          OpndStack.Push(Opnd1);
        }
        else if(_Opr[(int)_Tokens[i].Value.Operator].IsResultSecond){
          Opnd2.Lock();
          Opnd2.IsCalculated=true;
          OpndStack.Push(Opnd2);
        }
        else{
          Result.IsCalculated=true;
          OpndStack.Push(Result);
        }
        break;
      
      //Field access
      case ExprTokenId::Field:

        //Get operand from stack
        if(OpndStack.Length()<1){ 
          _Tokens[i].Msg(72).Print(); 
          Error=true; 
          break; 
        } 
        Opnd1=OpndStack.Pop(); 

        //Check we do not have undefined variables or void results as operands
        if(Opnd1.Id()==ExprTokenId::UndefVar){
          Opnd1.Msg(446).Print(Opnd1.Value.VarName);
          return false;
        }
        else if(Opnd1.Id()==ExprTokenId::VoidRes){
          Opnd1.Msg(111).Print(Opnd1.Value.VoidFuncName);
          return false;
        }

        //Check operand is a class variable
        if(Opnd1.MstType()!=MasterType::Class){ 
          _Tokens[i].Msg(73).Print(); 
          Error=true; 
          break; 
        } 

        //Find member within class
        if((FldIndex=_Md->FldSearch(_Tokens[i].Value.Field,_Md->Variables[Opnd1.Value.VarIndex].TypIndex))==-1){
          _Tokens[i].Msg(74).Print(_Md->CannonicalTypeName(Opnd1.TypIndex()),_Tokens[i].Value.Field);
          Error=true; 
          break;
        }

        //Check field access in relation to its scope
        if(!_Md->IsMemberVisible(Scope,Opnd1.TypIndex(),FldIndex,-1) && _Origin!=OrigBuffer::Insertion && _Origin!=OrigBuffer::Addition){
          _Tokens[i].Msg(170).Print(_Md->CannonicalTypeName(Opnd1.TypIndex()),_Tokens[i].Value.Field);
          Error=true; 
          break;
        }

        //Get result operand
        if(_Md->Fields[FldIndex].IsStatic){
          VarName=_Md->GetStaticFldName(FldIndex);
          if((VarIndex=_Md->VarSearch(VarName,Scope.ModIndex))==-1){
            _Tokens[i].Msg(17).Print(_Md->Fields[FldIndex].Name,_Md->CannonicalTypeName(_Md->Fields[FldIndex].SubScope.TypIndex),VarName);
            Error=true; 
            break;
          }
          Result.ThisVar(_Md,VarIndex,_Tokens[i].SrcInfo());
          Result.SourceVarIndex=(Opnd1.SourceVarIndex!=-1?Opnd1.SourceVarIndex:Opnd1.Value.VarIndex);
          DebugMessage(DebugLevel::CmpExpression,"Source variable set for token "+Result.Print()+" pointing to variable "+_Md->Variables[Result.SourceVarIndex].Name+" in scope "+_Md->ScopeName(_Md->Variables[Result.SourceVarIndex].Scope));
          Result.SetSourceUsed(Scope,false);
        }
        else{
          if(!Result.NewInd(_Md,Scope,CodeBlockId,_Md->Fields[FldIndex].TypIndex,false,_Tokens[i].SrcInfo())){ Error=true; break; }
          Result.SourceVarIndex=(Opnd1.SourceVarIndex!=-1?Opnd1.SourceVarIndex:Opnd1.Value.VarIndex);
          DebugMessage(DebugLevel::CmpExpression,"Source variable set for token "+Result.Print()+" pointing to variable "+_Md->Variables[Result.SourceVarIndex].Name+" in scope "+_Md->ScopeName(_Md->Variables[Result.SourceVarIndex].Scope));
          if(!_Md->Bin.AsmWriteCode(CpuInstCode::REFOF,Result.Asm(true),Opnd1.Asm(),_Md->Bin.AsmLitWrd(_Md->Fields[FldIndex].Offset))){ Error=true; break; } 
        }

        //Release operand
        Opnd1.Release();

        //Set result as calculated
        Result.IsCalculated=true;

        //Push result to operand stack
        OpndStack.Push(Result);
        break;        
      
      //Array subscript / String subscript
      case ExprTokenId::Subscript:
        if(!_SubscriptCall(Scope,CodeBlockId,i,OpndStack,Result)){ Error=true; break; }
        break;
      
      //Function
      case ExprTokenId::Function:
        if(!_FunctionMethodCall(Scope,CodeBlockId,i,OpndStack,ExprCallType::Function,(i==_Tokens.Length()-1?true:false),Result)){ Error=true; break; }
        break;

      //Method call
      case ExprTokenId::Method:
        if(!_FunctionMethodCall(Scope,CodeBlockId,i,OpndStack,ExprCallType::Method,(i==_Tokens.Length()-1?true:false),Result)){ Error=true; break; }
        break;

      //Class constructor call
      case ExprTokenId::Constructor:
        if(!_FunctionMethodCall(Scope,CodeBlockId,i,OpndStack,ExprCallType::Constructor,(i==_Tokens.Length()-1?true:false),Result)){ Error=true; break; }
        break;

      //Complex value call
      case ExprTokenId::Complex:
        if(!_ComplexValueCall(Scope,CodeBlockId,i,OpndStack,Result)){ Error=true; break; }
        break;

      //Delimiter tokens
      case ExprTokenId::Delimiter:
        _Tokens[i].Msg(176).Print();
        Error=true;
        break;

      //Void result
      case ExprTokenId::VoidRes:
        _Tokens[i].Msg(496).Print();
        Error=true;
        break;

    }

    //Error exit
    if(Error){ break; }

    //Debug message
    #ifdef __DEV__
      String OpndStackStr;
      for(int j=0;j<OpndStack.Length();j++){ OpndStackStr+=(OpndStackStr.Length()!=0?" ":"")+OpndStack[j].Print(); }
      DebugMessage(DebugLevel::CmpExpression,"Token "+_Tokens[i].Print()+" --> Operand stack {"+OpndStackStr+"}");
    #endif

    //Skip next token (when leftside of assignment is re-used as result operand)
    if(SkipNext){ i++; }

  }

  //Error happened during expression evaluation
  if(Error){
    return false;
  }

  //Check there are delayed error messages before exitting
  if(SysMessage().DelayCount()!=0){
    return false;
  }

  //Error if operand stack has more than 1 result
  if(OpndStack.Length()>1){
    OpndStack[1].Msg(104).Print();
    return false;
  }
  
  //Operand stack does not have any operands
  else if(OpndStack.Length()==0){
    if(ResultIsMandatory){
      _Tokens[0].Msg(105).Print();
      return false;
    }
    else{
      Result=ExprToken();
    }
  }
  
  //Operand stack has exactly one operand
  else if(OpndStack.Length()==1){
    if(_Tokens.Length()>1 && !OpndStack[0].IsCalculated){
      OpndStack[0].Msg(450).Print();
      return false;
    }
    Result=OpndStack[0];
    Result.SetSourceUsed(Scope,ResultIsMandatory);
  }

  //Print result
  #ifdef __DEV__
    if(ResultIsMandatory){
      DebugMessage(DebugLevel::CmpExpression,"Result token: "+Result.Print());
    }
  #endif

  //Return code
  return true;

}

//Copy operand
bool Expression::CopyOperand(MasterData *Md,ExprToken& Dest,const ExprToken& Sour) const {
  
  //Check data type of source and destination is the same
  //Error is sent only when master types do not match or they are not atomic
  if(Dest.TypIndex()!=Sour.TypIndex()){
    if(!((Dest.MstType()==Sour.MstType() && Dest.IsMasterAtomic() && Sour.IsMasterAtomic()) || Md->EquivalentArrays(Sour.TypIndex(),Dest.TypIndex()))){
      Sour.Msg(77).Print(Md->CannonicalTypeName(Dest.TypIndex()),Md->CannonicalTypeName(Sour.TypIndex()));
      return false;
    }
  }

  //Switch on master type
  switch(Sour.MstType()){
    
    //Simple data types    
    case MasterType::Boolean: 
    case MasterType::Char: 
    case MasterType::Short: 
    case MasterType::Integer: 
    case MasterType::Long: 
    case MasterType::Float: 
    case MasterType::Enum: 
      if(!Md->Bin.AsmWriteCode(CpuMetaInst::MV,Dest.Asm(),Sour.Asm())){ return false; }
      break;

    //Strings
    case MasterType::String: 
      if(Sour.AdrMode==CpuAdrMode::Address && Md->Variables[Sour.Value.VarIndex].IsTempVar==true && Md->Variables[Sour.Value.VarIndex].IsTempLocked==false
      && Dest.AdrMode==CpuAdrMode::Address && Md->Variables[Dest.Value.VarIndex].IsTempVar==false){
        if(!Md->Bin.AsmWriteCode(CpuInstCode::SSWCP,Dest.Asm(),Sour.Asm())){ return false; }
      }
      else{
        if(!Md->Bin.AsmWriteCode(CpuMetaInst::MV,Dest.Asm(),Sour.Asm())){ return false; }
      }
      break;

    //Classes
    //(does nothing if class is static or empty as there is no data to copy)
    case MasterType::Class:
      if(!Md->IsEmptyClass(Sour.TypIndex()) && !Md->IsStaticClass(Sour.TypIndex())){
        if(!Md->Bin.AsmWriteCode(CpuInstCode::COPY,Dest.Asm(),Sour.Asm(),Md->Bin.AsmLitWrd(Md->Types[Sour.TypIndex()].Length))){ return false; }
        if(Md->HasInnerBlocks(Md->Variables[Sour.Value.VarIndex].TypIndex)){
          if(!_InnerBlockReplication(Md,Dest,Sour)){ return false; }
        }
      }
      break;

    //Fixed arrays
    case MasterType::FixArray: 
      if(!Md->Bin.AsmWriteCode(CpuInstCode::COPY,Dest.Asm(),Sour.Asm(),Md->Bin.AsmLitWrd(Md->Types[Sour.TypIndex()].Length))){ return false; }
      if(Md->HasInnerBlocks(Md->Variables[Sour.Value.VarIndex].TypIndex)){
        if(!_InnerBlockReplication(Md,Dest,Sour)){ return false; }
      }
      break;

    //Dynamic arrays
    case MasterType::DynArray:
      if(!Md->Bin.AsmWriteCode(CpuInstCode::ACOPY,Dest.Asm(),Sour.Asm())){ return false; }
      if(Md->HasInnerBlocks(Md->Variables[Sour.Value.VarIndex].TypIndex)){
        if(!_InnerBlockReplication(Md,Dest,Sour)){ return false; }
      }
      break;
  }

  //Return code
  return true;

}

//Initialize operand
//This routine does not generate code for items with atomic master types, since they are zeroanyway
//Globals are stored with zero value by default by compiler, and locals have zero value since stack frames are initialized to zero by runtime
bool Expression::InitOperand(MasterData *Md,ExprToken& Dest,bool& CodeGenerated) const {
  
  //Variables
  int i;
  int VarIndex;
  int DimNr;
  CpuWrd CellSize;
  Array<int> StaticFields;
  ExprToken StaticVar;


  //Switch on master type
  switch(Dest.MstType()){
    
    //Simple data types    
    case MasterType::Boolean: 
    case MasterType::Char:    
    case MasterType::Short:   
    case MasterType::Integer: 
    case MasterType::Long:    
    case MasterType::Float:   
    case MasterType::Enum:    
      CodeGenerated=false;
      break;

    //Strings
    case MasterType::String: 
      if(!Md->Bin.AsmWriteCode(CpuInstCode::SEMP,Dest.Asm())){ return false; }
      CodeGenerated=true;
      break;

    //Classes
    //(does nothing if class is static or empty as there is no data to copy)
    case MasterType::Class:
      if(!Md->IsEmptyClass(Dest.TypIndex()) && !Md->IsStaticClass(Dest.TypIndex())){
        if(Md->HasInnerBlocks(Md->Variables[Dest.Value.VarIndex].TypIndex)){
          if(!_InnerBlockInitialization(Md,Dest)){ return false; }
          CodeGenerated=true;
        }
      }
      break;

    //Fixed arrays
    case MasterType::FixArray: 
      if(Md->HasInnerBlocks(Md->Variables[Dest.Value.VarIndex].TypIndex)){
        if(!_InnerBlockInitialization(Md,Dest)){ return false; }
        CodeGenerated=true;
      }
      break;

    //Dynamic arrays
    case MasterType::DynArray:
      DimNr=Md->Types[Dest.TypIndex()].DimNr;
      CellSize=Md->Types[Md->Types[Dest.TypIndex()].ElemTypIndex].Length;
      if(DimNr==1){
        if(!Md->Bin.AsmWriteCode(CpuInstCode::AD1EM,Dest.Asm(),Md->Bin.AsmLitWrd(CellSize))){ return false; }
      }
      else{
        if(!Md->Bin.AsmWriteCode(CpuInstCode::ADEMP,Dest.Asm(),Md->Bin.AsmLitChr(DimNr),Md->Bin.AsmLitWrd(CellSize))){ return false; }
      }
      CodeGenerated=true;
      break;
  }

  //Initialize static fields with blocks for classes and arrays of classes
  StaticFields=_GetStaticFields(Md,Dest.TypIndex());
  if(StaticFields.Length()!=0){
    for(i=0;i<StaticFields.Length();i++){
      DebugMessage(DebugLevel::CmpInnerBlockInit,"Initializing static field "+Md->GetStaticFldName(StaticFields[i]));
      if((VarIndex=Md->VarSearch(Md->GetStaticFldName(StaticFields[i]),Md->CurrentScope().ModIndex))==-1){
        Dest.Msg(547).Print(Md->GetStaticFldName(StaticFields[i]));
        return false;
      }
      StaticVar.ThisVar(Md,VarIndex,Dest.SrcInfo());
      if(!InitOperand(Md,StaticVar,CodeGenerated)){ return false; }
    }
  }

  //Return code
  return true;

}

//Compile expression when result is expected
bool Expression::Compile(MasterData *Md,const ScopeDef& Scope,Sentence& Stn,int BegToken,int EndToken,ExprToken& Result){
  bool IsComp;
  _Md=Md;
  _FileName=Stn.FileName();
  _LineNr=Stn.LineNr();
  _Tokens.Reset();
  _Origin=Stn.Origin();
  DebugMessage(DebugLevel::CmpExpression,"ExprCompiler input: "+Stn.Print(BegToken,EndToken));
  Md->TempVarUnlockAll();
  if(!_Tokenize(Scope,Stn,BegToken,EndToken)){ return false; }
  if(!_Infix2RPN()){ return false; }
  if(!_IsComputable(Scope,IsComp)){ return false; }
  if(IsComp){
    DebugMessage(DebugLevel::CmpExpression,"Expression is computed, not compiled");
    if(!_Compute(Scope,Result)){ return false;}
  }
  else{
    if(!_Compile(Scope,Stn.GetCodeBlockId(),true,Result)){ return false;}
  }
  return true;
}

//Compile expression when result is not 
bool Expression::Compile(MasterData *Md,const ScopeDef& Scope,Sentence& Stn,int BegToken,int EndToken){
  _Md=Md;
  _FileName=Stn.FileName();
  _LineNr=Stn.LineNr();
  _Tokens.Reset();
  _Origin=Stn.Origin();
  ExprToken Result;
  DebugMessage(DebugLevel::CmpExpression,"ExprCompiler input: "+Stn.Print(BegToken,EndToken));
  Md->TempVarUnlockAll();
  if(!_Tokenize(Scope,Stn,BegToken,EndToken)){ return false; }
  if(!_Infix2RPN()){ return false; }
  if(!_Compile(Scope,Stn.GetCodeBlockId(),false,Result)){ return false;}
  return true;
}

//Compile full expression when result is expected
bool Expression::Compile(MasterData *Md,const ScopeDef& Scope,Sentence& Stn,ExprToken& Result){
  bool IsComp;
  _Md=Md;
  _FileName=Stn.FileName();
  _LineNr=Stn.LineNr();
  _Tokens.Reset();
  _Origin=Stn.Origin();
  DebugMessage(DebugLevel::CmpExpression,"ExprCompiler input: "+Stn.Print());
  Md->TempVarUnlockAll();
  if(!_Tokenize(Scope,Stn,0,Stn.Tokens.Length()-1)){ return false; }
  if(!_Infix2RPN()){ return false; }
  if(!_IsComputable(Scope,IsComp)){ return false; }
  if(IsComp){
    DebugMessage(DebugLevel::CmpExpression,"Expression is computed, not compiled");
    if(!_Compute(Scope,Result)){ return false;}
  }
  else{
    if(!_Compile(Scope,Stn.GetCodeBlockId(),true,Result)){ return false;}
  }
  return true;
}

//Compile full expression when result is not 
bool Expression::Compile(MasterData *Md,const ScopeDef& Scope,Sentence& Stn){
  _Md=Md;
  _FileName=Stn.FileName();
  _LineNr=Stn.LineNr();
  _Tokens.Reset();
  _Origin=Stn.Origin();
  ExprToken Result;
  DebugMessage(DebugLevel::CmpExpression,"ExprCompiler input: "+Stn.Print());
  Md->TempVarUnlockAll();
  if(!_Tokenize(Scope,Stn,0,Stn.Tokens.Length()-1)){ return false; }
  if(!_Infix2RPN()){ return false; }
  if(!_Compile(Scope,Stn.GetCodeBlockId(),false,Result)){ return false;}
  return true;
}

//Compute expression when result is expected
bool Expression::Compile(MasterData *Md,const ScopeDef& Scope,Sentence& Stn,int BegToken,int EndToken,ExprToken& Result,bool& Computed){
  bool IsComp;
  _Md=Md;
  _FileName=Stn.FileName();
  _LineNr=Stn.LineNr();
  _Tokens.Reset();
  _Origin=Stn.Origin();
  DebugMessage(DebugLevel::CmpExpression,"ExprCompiler input: "+Stn.Print(BegToken,EndToken));
  Md->TempVarUnlockAll();
  if(!_Tokenize(Scope,Stn,BegToken,EndToken)){ return false; }
  if(!_Infix2RPN()){ return false; }
  if(!_IsComputable(Scope,IsComp)){ return false; }
  if(IsComp){
    DebugMessage(DebugLevel::CmpExpression,"Expression is computed, not compiled");
    if(!_Compute(Scope,Result)){ return false;}
    Computed=true;
  }
  else{
    if(!_Compile(Scope,Stn.GetCodeBlockId(),true,Result)){ return false;}
    Computed=false;
  }
  return true;
}

//Operation ExprOperator::UnaryPlus
bool OprUnaryPlus(ExprToken& Result,const ExprToken& a,const SourceInfo& SrcInfo){
  bool Error=false;
  switch(a.MstType()){
    case MasterType::Boolean : Error=true; break;
    case MasterType::Char    : Result=a; break;
    case MasterType::Short   : Result=a; break;
    case MasterType::Integer : Result=a; break;
    case MasterType::Long    : Result=a; break;
    case MasterType::Float   : Result=a; break;
    case MasterType::String  : Error=true; break;
    case MasterType::Enum    : Error=true; break;
    case MasterType::Class   : Error=true; break;
    case MasterType::FixArray: Error=true; break;
    case MasterType::DynArray: Error=true; break;
  }
  if(Error){
    SrcInfo.Msg(154).Print(MasterTypeName(a.MstType()));
    return false;
  }
  return true;
} 

//Operation ExprOperator::UnaryMinus
bool OprUnaryMinus(ExprToken& Result,const ExprToken& a,const SourceInfo& SrcInfo){
  bool Error=false;
  switch(a.MstType()){
    case MasterType::Boolean : Error=true; break;
    case MasterType::Char    : Result.Value.Chr=-a.Value.Chr; break;
    case MasterType::Short   : Result.Value.Shr=-a.Value.Shr; break;
    case MasterType::Integer : Result.Value.Int=-a.Value.Int; break;
    case MasterType::Long    : Result.Value.Lon=-a.Value.Lon; break;
    case MasterType::Float   : Result.Value.Flo=-a.Value.Flo; break;
    case MasterType::String  : Error=true; break;
    case MasterType::Enum    : Error=true; break;
    case MasterType::Class   : Error=true; break;
    case MasterType::FixArray: Error=true; break;
    case MasterType::DynArray: Error=true; break;
  }
  if(Error){
    SrcInfo.Msg(154).Print(MasterTypeName(a.MstType()));
    return false;
  }
  return true;
} 

//Operation ExprOperator::Multiplication
bool OprMultiplication(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo){
  bool Error=false;
  bool Overflow=false;
  bool FlExcept=false;
  String Val1;
  String Val2;
  switch(a.MstType()){
    case MasterType::Boolean : Error=true; break;
    case MasterType::Char    : if(!System::IsChrMulSafe(a.Value.Chr,b.Value.Chr)){ Overflow=true; Val1=ToString(a.Value.Chr); Val2=ToString(b.Value.Chr); } else{ Result.Value.Chr=a.Value.Chr * b.Value.Chr; } break;
    case MasterType::Short   : if(!System::IsShrMulSafe(a.Value.Shr,b.Value.Shr)){ Overflow=true; Val1=ToString(a.Value.Shr); Val2=ToString(b.Value.Shr); } else{ Result.Value.Shr=a.Value.Shr * b.Value.Shr; } break;
    case MasterType::Integer : if(!System::IsIntMulSafe(a.Value.Int,b.Value.Int)){ Overflow=true; Val1=ToString(a.Value.Int); Val2=ToString(b.Value.Int); } else{ Result.Value.Int=a.Value.Int * b.Value.Int; } break;
    case MasterType::Long    : if(!System::IsLonMulSafe(a.Value.Lon,b.Value.Lon)){ Overflow=true; Val1=ToString(a.Value.Lon); Val2=ToString(b.Value.Lon); } else{ Result.Value.Lon=a.Value.Lon * b.Value.Lon; } break;
    case MasterType::Float   : System::ClearFloException(); Result.Value.Flo=a.Value.Flo * b.Value.Flo; if(System::CheckFloException()){ FlExcept=true; Val1=ToString(a.Value.Flo); Val2=ToString(b.Value.Flo); }; break;
    case MasterType::String  : Error=true; break;
    case MasterType::Enum    : Error=true; break;
    case MasterType::Class   : Error=true; break;
    case MasterType::FixArray: Error=true; break;
    case MasterType::DynArray: Error=true; break;
  }
  if(Error){
    SrcInfo.Msg(154).Print(MasterTypeName(a.MstType()));
    return false;
  }
  if(Overflow){
    SrcInfo.Msg(381).Print(Val1,Val2,MasterTypeName(a.MstType()));
    return false;
  }
  if(FlExcept){
    SrcInfo.Msg(382).Print(System::GetLastFloException(),Val1,Val2);
    return false;
  }
  return true;
} 

//Operation ExprOperator::Division
bool OprDivision(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo){
  bool Error=false;
  bool DivError=false;
  bool FlExcept=false;
  String Val1;
  String Val2;
  switch(a.MstType()){
    case MasterType::Boolean : Error=true; break;
    case MasterType::Char    : if(b.Value.Chr==0){ DivError=true; Val1=ToString(a.Value.Chr); Val2=ToString(b.Value.Chr); } else{ Result.Value.Chr=a.Value.Chr / b.Value.Chr; } break;
    case MasterType::Short   : if(b.Value.Shr==0){ DivError=true; Val1=ToString(a.Value.Shr); Val2=ToString(b.Value.Shr); } else{ Result.Value.Shr=a.Value.Shr / b.Value.Shr; } break;
    case MasterType::Integer : if(b.Value.Int==0){ DivError=true; Val1=ToString(a.Value.Int); Val2=ToString(b.Value.Int); } else{ Result.Value.Int=a.Value.Int / b.Value.Int; } break;
    case MasterType::Long    : if(b.Value.Lon==0){ DivError=true; Val1=ToString(a.Value.Lon); Val2=ToString(b.Value.Lon); } else{ Result.Value.Lon=a.Value.Lon / b.Value.Lon; } break;
    case MasterType::Float   : if(b.Value.Flo==0){ DivError=true; Val1=ToString(a.Value.Flo); Val2=ToString(b.Value.Flo); } else{ System::ClearFloException(); Result.Value.Flo=a.Value.Flo * b.Value.Flo; if(System::CheckFloException()){ FlExcept=true; Val1=ToString(a.Value.Flo); Val2=ToString(b.Value.Flo); } } break;
    case MasterType::String  : Error=true; break;
    case MasterType::Enum    : Error=true; break;
    case MasterType::Class   : Error=true; break;
    case MasterType::FixArray: Error=true; break;
    case MasterType::DynArray: Error=true; break;
  }
  if(Error){
    SrcInfo.Msg(154).Print(MasterTypeName(a.MstType()));
    return false;
  }
  if(DivError){
    SrcInfo.Msg(383).Print(Val1,Val2);
    return false;
  }
  if(FlExcept){
    SrcInfo.Msg(384).Print(System::GetLastFloException(),Val1,Val2);
    return false;
  }
  return true;
} 

//Operation ExprOperator::Addition
bool OprAddition(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo){
  bool Error=false;
  bool Overflow=false;
  bool FlExcept=false;
  String Val1;
  String Val2;
  switch(a.MstType()){
    case MasterType::Boolean : Error=true; break;
    case MasterType::Char    : if(!System::IsChrAddSafe(a.Value.Chr,b.Value.Chr)){ Overflow=true; Val1=ToString(a.Value.Chr); Val2=ToString(b.Value.Chr); } else{ Result.Value.Chr=a.Value.Chr + b.Value.Chr; } break;
    case MasterType::Short   : if(!System::IsShrAddSafe(a.Value.Shr,b.Value.Shr)){ Overflow=true; Val1=ToString(a.Value.Shr); Val2=ToString(b.Value.Shr); } else{ Result.Value.Shr=a.Value.Shr + b.Value.Shr; } break;
    case MasterType::Integer : if(!System::IsIntAddSafe(a.Value.Int,b.Value.Int)){ Overflow=true; Val1=ToString(a.Value.Int); Val2=ToString(b.Value.Int); } else{ Result.Value.Int=a.Value.Int + b.Value.Int; } break;
    case MasterType::Long    : if(!System::IsLonAddSafe(a.Value.Lon,b.Value.Lon)){ Overflow=true; Val1=ToString(a.Value.Lon); Val2=ToString(b.Value.Lon); } else{ Result.Value.Lon=a.Value.Lon + b.Value.Lon; } break;
    case MasterType::Float   : System::ClearFloException(); Result.Value.Flo=a.Value.Flo + b.Value.Flo; if(System::CheckFloException()){ FlExcept=true; Val1=ToString(a.Value.Flo); Val2=ToString(b.Value.Flo); }; break;
    case MasterType::String  : Error=true; break;
    case MasterType::Enum    : Error=true; break;
    case MasterType::Class   : Error=true; break;
    case MasterType::FixArray: Error=true; break;
    case MasterType::DynArray: Error=true; break;
  }
  if(Error){
    SrcInfo.Msg(154).Print(MasterTypeName(a.MstType()));
    return false;
  }
  if(Overflow){
    SrcInfo.Msg(385).Print(Val1,Val2,MasterTypeName(a.MstType()));
    return false;
  }
  if(FlExcept){
    SrcInfo.Msg(386).Print(System::GetLastFloException(),Val1,Val2);
    return false;
  }
  return true;
} 

//Operation ExprOperator::Substraction
bool OprSubstraction(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo){
  bool Error=false;
  bool Overflow=false;
  bool FlExcept=false;
  String Val1;
  String Val2;
  switch(a.MstType()){
    case MasterType::Boolean : Error=true; break;
    case MasterType::Char    : if(!System::IsChrSubSafe(a.Value.Chr,b.Value.Chr)){ Overflow=true; Val1=ToString(a.Value.Chr); Val2=ToString(b.Value.Chr); } else{ Result.Value.Chr=a.Value.Chr - b.Value.Chr; } break;
    case MasterType::Short   : if(!System::IsShrSubSafe(a.Value.Shr,b.Value.Shr)){ Overflow=true; Val1=ToString(a.Value.Shr); Val2=ToString(b.Value.Shr); } else{ Result.Value.Shr=a.Value.Shr - b.Value.Shr; } break;
    case MasterType::Integer : if(!System::IsIntSubSafe(a.Value.Int,b.Value.Int)){ Overflow=true; Val1=ToString(a.Value.Int); Val2=ToString(b.Value.Int); } else{ Result.Value.Int=a.Value.Int - b.Value.Int; } break;
    case MasterType::Long    : if(!System::IsLonSubSafe(a.Value.Lon,b.Value.Lon)){ Overflow=true; Val1=ToString(a.Value.Lon); Val2=ToString(b.Value.Lon); } else{ Result.Value.Lon=a.Value.Lon - b.Value.Lon; } break;
    case MasterType::Float   : System::ClearFloException(); Result.Value.Flo=a.Value.Flo - b.Value.Flo; if(System::CheckFloException()){ FlExcept=true; Val1=ToString(a.Value.Flo); Val2=ToString(b.Value.Flo); }; break;
    case MasterType::String  : Error=true; break;
    case MasterType::Enum    : Error=true; break;
    case MasterType::Class   : Error=true; break;
    case MasterType::FixArray: Error=true; break;
    case MasterType::DynArray: Error=true; break;
  }
  if(Error){
    SrcInfo.Msg(154).Print(MasterTypeName(a.MstType()));
    return false;
  }
  if(Overflow){
    SrcInfo.Msg(387).Print(Val2,Val2,MasterTypeName(a.MstType()));
    return false;
  }
  if(FlExcept){
    SrcInfo.Msg(388).Print(System::GetLastFloException(),Val2,Val2);
    return false;
  }
  return true;
} 

//Operation ExprOperator::Less
bool OprLess(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo){
  bool Error=false;
  Result.LitNumTypIndex=a._Md->BolTypIndex;
  switch(a.MstType()){
    case MasterType::Boolean : Error=true; break;
    case MasterType::Char    : Result.Value.Bol=(a.Value.Chr < b.Value.Chr?true:false); break;
    case MasterType::Short   : Result.Value.Bol=(a.Value.Shr < b.Value.Shr?true:false); break;
    case MasterType::Integer : Result.Value.Bol=(a.Value.Int < b.Value.Int?true:false); break;
    case MasterType::Long    : Result.Value.Bol=(a.Value.Lon < b.Value.Lon?true:false); break;
    case MasterType::Float   : Result.Value.Bol=(a.Value.Flo < b.Value.Flo?true:false); break;
    case MasterType::String  : Error=true; break;
    case MasterType::Enum    : Error=true; break;
    case MasterType::Class   : Error=true; break;
    case MasterType::FixArray: Error=true; break;
    case MasterType::DynArray: Error=true; break;
  }
  if(Error){
    SrcInfo.Msg(154).Print(MasterTypeName(a.MstType()));
    return false;
  }
  return true;
} 

//Operation ExprOperator::LessEqual
bool OprLessEqual(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo){
  bool Error=false;
  Result.LitNumTypIndex=a._Md->BolTypIndex;
  switch(a.MstType()){
    case MasterType::Boolean : Error=true; break;
    case MasterType::Char    : Result.Value.Bol=(a.Value.Chr <= b.Value.Chr?true:false); break;
    case MasterType::Short   : Result.Value.Bol=(a.Value.Shr <= b.Value.Shr?true:false); break;
    case MasterType::Integer : Result.Value.Bol=(a.Value.Int <= b.Value.Int?true:false); break;
    case MasterType::Long    : Result.Value.Bol=(a.Value.Lon <= b.Value.Lon?true:false); break;
    case MasterType::Float   : Result.Value.Bol=(a.Value.Flo <= b.Value.Flo?true:false); break;
    case MasterType::String  : Error=true; break;
    case MasterType::Enum    : Error=true; break;
    case MasterType::Class   : Error=true; break;
    case MasterType::FixArray: Error=true; break;
    case MasterType::DynArray: Error=true; break;
  }
  if(Error){
    SrcInfo.Msg(154).Print(MasterTypeName(a.MstType()));
    return false;
  }
  return true;
} 

//Operation ExprOperator::Greater
bool OprGreater(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo){
  bool Error=false;
  Result.LitNumTypIndex=a._Md->BolTypIndex;
  switch(a.MstType()){
    case MasterType::Boolean : Error=true; break;
    case MasterType::Char    : Result.Value.Bol=(a.Value.Chr >  b.Value.Chr?true:false); break;
    case MasterType::Short   : Result.Value.Bol=(a.Value.Shr >  b.Value.Shr?true:false); break;
    case MasterType::Integer : Result.Value.Bol=(a.Value.Int >  b.Value.Int?true:false); break;
    case MasterType::Long    : Result.Value.Bol=(a.Value.Lon >  b.Value.Lon?true:false); break;
    case MasterType::Float   : Result.Value.Bol=(a.Value.Flo >  b.Value.Flo?true:false); break;
    case MasterType::String  : Error=true; break;
    case MasterType::Enum    : Error=true; break;
    case MasterType::Class   : Error=true; break;
    case MasterType::FixArray: Error=true; break;
    case MasterType::DynArray: Error=true; break;
  }
  if(Error){
    SrcInfo.Msg(154).Print(MasterTypeName(a.MstType()));
    return false;
  }
  return true;
} 

//Operation ExprOperator::GreaterEqual
bool OprGreaterEqual(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo){
  bool Error=false;
  Result.LitNumTypIndex=a._Md->BolTypIndex;
  switch(a.MstType()){
    case MasterType::Boolean : Error=true; break;
    case MasterType::Char    : Result.Value.Bol=(a.Value.Chr >= b.Value.Chr?true:false); break;
    case MasterType::Short   : Result.Value.Bol=(a.Value.Shr >= b.Value.Shr?true:false); break;
    case MasterType::Integer : Result.Value.Bol=(a.Value.Int >= b.Value.Int?true:false); break;
    case MasterType::Long    : Result.Value.Bol=(a.Value.Lon >= b.Value.Lon?true:false); break;
    case MasterType::Float   : Result.Value.Bol=(a.Value.Flo >= b.Value.Flo?true:false); break;
    case MasterType::String  : Error=true; break;
    case MasterType::Enum    : Error=true; break;
    case MasterType::Class   : Error=true; break;
    case MasterType::FixArray: Error=true; break;
    case MasterType::DynArray: Error=true; break;
  }
  if(Error){
    SrcInfo.Msg(154).Print(MasterTypeName(a.MstType()));
    return false;
  }
  return true;
} 

//Operation ExprOperator::Equal
bool OprEqual(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo){
  bool Error=false;
  Result.LitNumTypIndex=a._Md->BolTypIndex;
  switch(a.MstType()){
    case MasterType::Boolean : Result.Value.Bol=(a.Value.Chr == b.Value.Chr?true:false); break;
    case MasterType::Char    : Result.Value.Bol=(a.Value.Chr == b.Value.Chr?true:false); break;
    case MasterType::Short   : Result.Value.Bol=(a.Value.Shr == b.Value.Shr?true:false); break;
    case MasterType::Integer : Result.Value.Bol=(a.Value.Int == b.Value.Int?true:false); break;
    case MasterType::Long    : Result.Value.Bol=(a.Value.Lon == b.Value.Lon?true:false); break;
    case MasterType::Float   : Result.Value.Bol=(a.Value.Flo == b.Value.Flo?true:false); break;
    case MasterType::String  : Error=true; break;
    case MasterType::Enum    : Error=true; break;
    case MasterType::Class   : Error=true; break;
    case MasterType::FixArray: Error=true; break;
    case MasterType::DynArray: Error=true; break;
  }
  if(Error){
    SrcInfo.Msg(154).Print(MasterTypeName(a.MstType()));
    return false;
  }
  return true;
} 

//Operation ExprOperator::Distinct
bool OprDistinct(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo){
  bool Error=false;
  Result.LitNumTypIndex=a._Md->BolTypIndex;
  switch(a.MstType()){
    case MasterType::Boolean : Result.Value.Bol=(a.Value.Chr != b.Value.Chr?true:false); break;
    case MasterType::Char    : Result.Value.Bol=(a.Value.Chr != b.Value.Chr?true:false); break;
    case MasterType::Short   : Result.Value.Bol=(a.Value.Shr != b.Value.Shr?true:false); break;
    case MasterType::Integer : Result.Value.Bol=(a.Value.Int != b.Value.Int?true:false); break;
    case MasterType::Long    : Result.Value.Bol=(a.Value.Lon != b.Value.Lon?true:false); break;
    case MasterType::Float   : Result.Value.Bol=(a.Value.Flo != b.Value.Flo?true:false); break;
    case MasterType::String  : Error=true; break;
    case MasterType::Enum    : Error=true; break;
    case MasterType::Class   : Error=true; break;
    case MasterType::FixArray: Error=true; break;
    case MasterType::DynArray: Error=true; break;
  }
  if(Error){
    SrcInfo.Msg(154).Print(MasterTypeName(a.MstType()));
    return false;
  }
  return true;
} 

//Operation ExprOperator::BitwiseNot
bool OprBitwiseNot(ExprToken& Result,const ExprToken& a,const SourceInfo& SrcInfo){
  bool Error=false;
  switch(a.MstType()){
    case MasterType::Boolean : Error=true; break;
    case MasterType::Char    : Result.Value.Chr= ~a.Value.Chr; break;
    case MasterType::Short   : Result.Value.Shr= ~a.Value.Shr; break;
    case MasterType::Integer : Result.Value.Int= ~a.Value.Int; break;
    case MasterType::Long    : Result.Value.Lon= ~a.Value.Lon; break;
    case MasterType::Float   : Error=true; break;
    case MasterType::String  : Error=true; break;
    case MasterType::Enum    : Error=true; break;
    case MasterType::Class   : Error=true; break;
    case MasterType::FixArray: Error=true; break;
    case MasterType::DynArray: Error=true; break;
  }
  if(Error){
    SrcInfo.Msg(154).Print(MasterTypeName(a.MstType()));
    return false;
  }
  return true;
} 

//Operation ExprOperator::Modulus
bool OprModulus(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo){
  bool Error=false;
  bool DivError=false;
  String Val1;
  String Val2;
  switch(a.MstType()){
    case MasterType::Boolean : Error=true; break;
    case MasterType::Char    : if(b.Value.Chr==0){ DivError=true; Val1=ToString(a.Value.Chr); Val2=ToString(b.Value.Chr); } else{ Result.Value.Chr=MOD(a.Value.Chr,b.Value.Chr); } break;
    case MasterType::Short   : if(b.Value.Shr==0){ DivError=true; Val1=ToString(a.Value.Shr); Val2=ToString(b.Value.Shr); } else{ Result.Value.Shr=MOD(a.Value.Shr,b.Value.Shr); } break;
    case MasterType::Integer : if(b.Value.Int==0){ DivError=true; Val1=ToString(a.Value.Int); Val2=ToString(b.Value.Int); } else{ Result.Value.Int=MOD(a.Value.Int,b.Value.Int); } break;
    case MasterType::Long    : if(b.Value.Lon==0){ DivError=true; Val1=ToString(a.Value.Lon); Val2=ToString(b.Value.Lon); } else{ Result.Value.Lon=MOD(a.Value.Lon,b.Value.Lon); } break;
    case MasterType::Float   : Error=true; break;
    case MasterType::String  : Error=true; break;
    case MasterType::Enum    : Error=true; break;
    case MasterType::Class   : Error=true; break;
    case MasterType::FixArray: Error=true; break;
    case MasterType::DynArray: Error=true; break;
  }
  if(Error){
    SrcInfo.Msg(154).Print(MasterTypeName(a.MstType()));
    return false;
  }
  if(DivError){
    SrcInfo.Msg(389).Print(Val1,Val2);
    return false;
  }
  return true;
} 

//Operation ExprOperator::ShiftLeft
bool OprShiftLeft(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo){
  bool Error=false;
  switch(a.MstType()){
    case MasterType::Boolean : Error=true; break;
    case MasterType::Char    : Result.Value.Chr=a.Value.Chr << b.Value.Chr; break;
    case MasterType::Short   : Result.Value.Shr=a.Value.Shr << b.Value.Shr; break;
    case MasterType::Integer : Result.Value.Int=a.Value.Int << b.Value.Int; break;
    case MasterType::Long    : Result.Value.Lon=a.Value.Lon << b.Value.Lon; break;
    case MasterType::Float   : Error=true; break;
    case MasterType::String  : Error=true; break;
    case MasterType::Enum    : Error=true; break;
    case MasterType::Class   : Error=true; break;
    case MasterType::FixArray: Error=true; break;
    case MasterType::DynArray: Error=true; break;
  }
  if(Error){
    SrcInfo.Msg(154).Print(MasterTypeName(a.MstType()));
    return false;
  }
  return true;
}

//Operation ExprOperator::ShiftRight
bool OprShiftRight(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo){
  bool Error=false;
  switch(a.MstType()){
    case MasterType::Boolean : Error=true; break;
    case MasterType::Char    : Result.Value.Chr=a.Value.Chr >> b.Value.Chr; break;
    case MasterType::Short   : Result.Value.Shr=a.Value.Shr >> b.Value.Shr; break;
    case MasterType::Integer : Result.Value.Int=a.Value.Int >> b.Value.Int; break;
    case MasterType::Long    : Result.Value.Lon=a.Value.Lon >> b.Value.Lon; break;
    case MasterType::Float   : Error=true; break;
    case MasterType::String  : Error=true; break;
    case MasterType::Enum    : Error=true; break;
    case MasterType::Class   : Error=true; break;
    case MasterType::FixArray: Error=true; break;
    case MasterType::DynArray: Error=true; break;
  }
  if(Error){
    SrcInfo.Msg(154).Print(MasterTypeName(a.MstType()));
    return false;
  }
  return true;
}

//Operation ExprOperator::BitwiseAnd
bool OprBitwiseAnd(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo){
  bool Error=false;
  switch(a.MstType()){
    case MasterType::Boolean : Error=true; break;
    case MasterType::Char    : Result.Value.Chr=a.Value.Chr & b.Value.Chr; break;
    case MasterType::Short   : Result.Value.Shr=a.Value.Shr & b.Value.Shr; break;
    case MasterType::Integer : Result.Value.Int=a.Value.Int & b.Value.Int; break;
    case MasterType::Long    : Result.Value.Lon=a.Value.Lon & b.Value.Lon; break;
    case MasterType::Float   : Error=true; break;
    case MasterType::String  : Error=true; break;
    case MasterType::Enum    : Error=true; break;
    case MasterType::Class   : Error=true; break;
    case MasterType::FixArray: Error=true; break;
    case MasterType::DynArray: Error=true; break;
  }
  if(Error){
    SrcInfo.Msg(154).Print(MasterTypeName(a.MstType()));
    return false;
  }
  return true;
} 

//Operation ExprOperator::BitwiseXor
bool OprBitwiseXor(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo){
  bool Error=false;
  switch(a.MstType()){
    case MasterType::Boolean : Error=true; break;
    case MasterType::Char    : Result.Value.Chr=a.Value.Chr ^ b.Value.Chr; break;
    case MasterType::Short   : Result.Value.Shr=a.Value.Shr ^ b.Value.Shr; break;
    case MasterType::Integer : Result.Value.Int=a.Value.Int ^ b.Value.Int; break;
    case MasterType::Long    : Result.Value.Lon=a.Value.Lon ^ b.Value.Lon; break;
    case MasterType::Float   : Error=true; break;
    case MasterType::String  : Error=true; break;
    case MasterType::Enum    : Error=true; break;
    case MasterType::Class   : Error=true; break;
    case MasterType::FixArray: Error=true; break;
    case MasterType::DynArray: Error=true; break;
  }
  if(Error){
    SrcInfo.Msg(154).Print(MasterTypeName(a.MstType()));
    return false;
  }
  return true;
} 

//Operation ExprOperator::BitwiseOr
bool OprBitwiseOr(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo){
  bool Error=false;
  switch(a.MstType()){
    case MasterType::Boolean : Error=true; break;
    case MasterType::Char    : Result.Value.Chr=a.Value.Chr | b.Value.Chr; break;
    case MasterType::Short   : Result.Value.Shr=a.Value.Shr | b.Value.Shr; break;
    case MasterType::Integer : Result.Value.Int=a.Value.Int | b.Value.Int; break;
    case MasterType::Long    : Result.Value.Lon=a.Value.Lon | b.Value.Lon; break;
    case MasterType::Float   : Error=true; break;
    case MasterType::String  : Error=true; break;
    case MasterType::Enum    : Error=true; break;
    case MasterType::Class   : Error=true; break;
    case MasterType::FixArray: Error=true; break;
    case MasterType::DynArray: Error=true; break;
  }
  if(Error){
    SrcInfo.Msg(154).Print(MasterTypeName(a.MstType()));
    return false;
  }
  return true;
} 

//Operation ExprOperator::LogicalNot
bool OprLogicalNot(ExprToken& Result,const ExprToken& a,const SourceInfo& SrcInfo){
  bool Error=false;
  Result.LitNumTypIndex=a._Md->BolTypIndex;
  switch(a.MstType()){
    case MasterType::Boolean : Result.Value.Bol=(!a.Value.Chr?true:false); break;
    case MasterType::Char    : Result.Value.Bol=(!a.Value.Chr?true:false); break;
    case MasterType::Short   : Result.Value.Bol=(!a.Value.Shr?true:false); break;
    case MasterType::Integer : Result.Value.Bol=(!a.Value.Int?true:false); break;
    case MasterType::Long    : Result.Value.Bol=(!a.Value.Lon?true:false); break;
    case MasterType::Float   : Result.Value.Bol=(!a.Value.Flo?true:false); break;
    case MasterType::String  : Error=true; break;
    case MasterType::Enum    : Error=true; break;
    case MasterType::Class   : Error=true; break;
    case MasterType::FixArray: Error=true; break;
    case MasterType::DynArray: Error=true; break;
  }
  if(Error){
    SrcInfo.Msg(154).Print(MasterTypeName(a.MstType()));
    return false;
  }
  return true;
} 

//Operation ExprOperator::LogicalAnd
bool OprLogicalAnd(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo){
  bool Error=false;
  Result.LitNumTypIndex=a._Md->BolTypIndex;
  switch(a.MstType()){
    case MasterType::Boolean : Result.Value.Bol=(a.Value.Chr && b.Value.Chr?true:false); break;
    case MasterType::Char    : Result.Value.Bol=(a.Value.Chr && b.Value.Chr?true:false); break;
    case MasterType::Short   : Result.Value.Bol=(a.Value.Shr && b.Value.Shr?true:false); break;
    case MasterType::Integer : Result.Value.Bol=(a.Value.Int && b.Value.Int?true:false); break;
    case MasterType::Long    : Result.Value.Bol=(a.Value.Lon && b.Value.Lon?true:false); break;
    case MasterType::Float   : Result.Value.Bol=(a.Value.Flo && b.Value.Flo?true:false); break;
    case MasterType::String  : Error=true; break;
    case MasterType::Enum    : Error=true; break;
    case MasterType::Class   : Error=true; break;
    case MasterType::FixArray: Error=true; break;
    case MasterType::DynArray: Error=true; break;
  }
  if(Error){
    SrcInfo.Msg(154).Print(MasterTypeName(a.MstType()));
    return false;
  }
  return true;
} 

//Operation ExprOperator::LogicalOr
bool OprLogicalOr(ExprToken& Result,const ExprToken& a,const ExprToken& b,const SourceInfo& SrcInfo){
  bool Error=false;
  Result.LitNumTypIndex=a._Md->BolTypIndex;
  switch(a.MstType()){
    case MasterType::Boolean : Result.Value.Bol=(a.Value.Chr || b.Value.Chr?true:false); break;
    case MasterType::Char    : Result.Value.Bol=(a.Value.Chr || b.Value.Chr?true:false); break;
    case MasterType::Short   : Result.Value.Bol=(a.Value.Shr || b.Value.Shr?true:false); break;
    case MasterType::Integer : Result.Value.Bol=(a.Value.Int || b.Value.Int?true:false); break;
    case MasterType::Long    : Result.Value.Bol=(a.Value.Lon || b.Value.Lon?true:false); break;
    case MasterType::Float   : Result.Value.Bol=(a.Value.Flo || b.Value.Flo?true:false); break;
    case MasterType::String  : Error=true; break;
    case MasterType::Enum    : Error=true; break;
    case MasterType::Class   : Error=true; break;
    case MasterType::FixArray: Error=true; break;
    case MasterType::DynArray: Error=true; break;
  }
  if(Error){
    SrcInfo.Msg(154).Print(MasterTypeName(a.MstType()));
    return false;
  }
  return true;
} 

//Promote operand
//(the only data promotion supported is from integer to float)
bool Expression::_ComputeDataTypePromotion(const ScopeDef& Scope,ExprToken& Opnd,ExprOperCaseRule& CaseRule,MasterType MstMaxType) const {

  //Variables
  MasterType ToMstType;

  //Switch on promotion mode
  switch(CaseRule.PromMode){
    case ExprPromMode::ToResult:  ToMstType=CaseRule.MstResult; break;
    case ExprPromMode::ToMaximun: ToMstType=MstMaxType; break;
    case ExprPromMode::ToOther:   ToMstType=CaseRule.MstPromType; break;

  }

  //No promotion needed if data type is the same
  if(Opnd.MstType()==ToMstType){ return true; }

  //Emit conversion asm instruction
  if(     Opnd.MstType()==MasterType::Char    &&ToMstType==MasterType::Short   ){ if(!Opnd.ToShr()){ return false; } }
  else if(Opnd.MstType()==MasterType::Char    &&ToMstType==MasterType::Integer ){ if(!Opnd.ToInt()){ return false; } }
  else if(Opnd.MstType()==MasterType::Char    &&ToMstType==MasterType::Long    ){ if(!Opnd.ToLon()){ return false; } }
  else if(Opnd.MstType()==MasterType::Char    &&ToMstType==MasterType::Float   ){ if(!Opnd.ToFlo()){ return false; } }
  else if(Opnd.MstType()==MasterType::Short   &&ToMstType==MasterType::Integer ){ if(!Opnd.ToInt()){ return false; } }
  else if(Opnd.MstType()==MasterType::Short   &&ToMstType==MasterType::Long    ){ if(!Opnd.ToLon()){ return false; } }
  else if(Opnd.MstType()==MasterType::Short   &&ToMstType==MasterType::Float   ){ if(!Opnd.ToFlo()){ return false; } }
  else if(Opnd.MstType()==MasterType::Integer &&ToMstType==MasterType::Long    ){ if(!Opnd.ToLon()){ return false; } }
  else if(Opnd.MstType()==MasterType::Integer &&ToMstType==MasterType::Float   ){ if(!Opnd.ToFlo()){ return false; } }
  else if(Opnd.MstType()==MasterType::Long    &&ToMstType==MasterType::Float   ){ if(!Opnd.ToFlo()){ return false; } }
  else{
    Opnd.Msg(126).Print(MasterTypeName(Opnd.MstType()),MasterTypeName(ToMstType));
    return false;
  }

  //Return code
  return true;

}

//Prepare compute operation
bool Expression::_PrepareComputeOperation(const ScopeDef& Scope,const ExprToken& Opr,ExprToken& Opnd1,ExprToken& Opnd2,ExprToken& Result) const {

  //Variables
  int i;
  int Opx;
  int CaseRule;
  bool Match[2];
  MasterType MstMaxType;
  MasterType MstResult;
  
  //Get operand index
  Opx=(int)Opr.Value.Operator;

  //Check and find operand case rule
  CaseRule=-1;
  for(i=0;i<_OperCaseRuleNr;i++){
    Match[0]=((1<<(int)Opnd1.MstType())&_OperCaseRule[i].OperandCases[0]?true:false);
    if(_Opr[Opx].Class==ExprOpClass::Binary){
      Match[1]=((1<<(int)Opnd2.MstType())&_OperCaseRule[i].OperandCases[1]?true:false);
    }
    switch(_Opr[Opx].Class){
      case ExprOpClass::Unary : if(Match[0]){ CaseRule=i; } break;
      case ExprOpClass::Binary: if(Match[0] && Match[1]){ CaseRule=i; } break;
    }
    if(CaseRule!=-1){ break; }
  }
  if(CaseRule==-1){
    switch(_Opr[Opx].Class){
      case ExprOpClass::Unary : Opr.Msg(66).Print(_Opr[Opx].Name.Trim(),MasterTypeName(Opnd1.MstType())); break;
      case ExprOpClass::Binary: Opr.Msg(67).Print(_Opr[Opx].Name.Trim(),MasterTypeName(Opnd1.MstType()),MasterTypeName(Opnd2.MstType())); break;
    }
    return false;
  }

  //Complete data type promotions
  if(_Opr[Opx].Class==ExprOpClass::Binary){
    MstMaxType=(Opnd1.MstType()>Opnd2.MstType()?Opnd1.MstType():Opnd2.MstType());
    if(_OperCaseRule[CaseRule].Promotion[0] && Opnd1.MstType()!=_OperCaseRule[CaseRule].MstResult){ 
      if(!_ComputeDataTypePromotion(Scope,Opnd1,_OperCaseRule[CaseRule],MstMaxType)){ return false; }
    }
    if(_OperCaseRule[CaseRule].Promotion[1] && Opnd2.MstType()!=_OperCaseRule[CaseRule].MstResult && _Opr[Opx].Class==ExprOpClass::Binary){
      if(!_ComputeDataTypePromotion(Scope,Opnd2,_OperCaseRule[CaseRule],MstMaxType)){ return false; }
    }
  }

  //Get result operand
  MstResult=(_Opr[Opx].IsRetTypeDeterm?_OperCaseRule[CaseRule].MstResult:_Md->Types[Opr.CastTypIndex].MstType);
  if(!Result.NewConst(_Md,MstResult,Opr.SrcInfo())){ return false; }
  
  //Return code
  return true;

}

//Compute single operation
bool Expression::_ComputeOperation(const ScopeDef& Scope,const ExprToken& Oper,const ExprToken& Opnd1,const ExprToken& Opnd2,ExprToken& Result) const {

  //Variables
  ExprToken Arg1=Opnd1;
  ExprToken Arg2=Opnd2;

  //Prepare operation
  if(!_PrepareComputeOperation(Scope,Oper,Arg1,Arg2,Result)){ return false; }
  
  //Complete operation
  switch(Oper.Value.Operator){
    
    //NUmeric operators
    case ExprOperator::UnaryPlus:      if(!OprUnaryPlus     (Result,Arg1,     Oper.SrcInfo())){ return false; } break;
    case ExprOperator::UnaryMinus:     if(!OprUnaryMinus    (Result,Arg1,     Oper.SrcInfo())){ return false; } break;
    case ExprOperator::Multiplication: if(!OprMultiplication(Result,Arg1,Arg2,Oper.SrcInfo())){ return false; } break;
    case ExprOperator::Division:       if(!OprDivision      (Result,Arg1,Arg2,Oper.SrcInfo())){ return false; } break;
    case ExprOperator::Addition:       if(!OprAddition      (Result,Arg1,Arg2,Oper.SrcInfo())){ return false; } break;
    case ExprOperator::Substraction:   if(!OprSubstraction  (Result,Arg1,Arg2,Oper.SrcInfo())){ return false; } break;
    case ExprOperator::Less:           if(!OprLess          (Result,Arg1,Arg2,Oper.SrcInfo())){ return false; } break;
    case ExprOperator::LessEqual:      if(!OprLessEqual     (Result,Arg1,Arg2,Oper.SrcInfo())){ return false; } break;
    case ExprOperator::Greater:        if(!OprGreater       (Result,Arg1,Arg2,Oper.SrcInfo())){ return false; } break;
    case ExprOperator::GreaterEqual:   if(!OprGreaterEqual  (Result,Arg1,Arg2,Oper.SrcInfo())){ return false; } break;
    case ExprOperator::Equal:          if(!OprEqual         (Result,Arg1,Arg2,Oper.SrcInfo())){ return false; } break;
    case ExprOperator::Distinct:       if(!OprDistinct      (Result,Arg1,Arg2,Oper.SrcInfo())){ return false; } break;
    case ExprOperator::BitwiseNot:     if(!OprBitwiseNot    (Result,Arg1,     Oper.SrcInfo())){ return false; } break;
    case ExprOperator::Modulus:        if(!OprModulus       (Result,Arg1,Arg2,Oper.SrcInfo())){ return false; } break;
    case ExprOperator::ShiftLeft:      if(!OprShiftLeft     (Result,Arg1,Arg2,Oper.SrcInfo())){ return false; } break;
    case ExprOperator::ShiftRight:     if(!OprShiftRight    (Result,Arg1,Arg2,Oper.SrcInfo())){ return false; } break;
    case ExprOperator::BitwiseAnd:     if(!OprBitwiseAnd    (Result,Arg1,Arg2,Oper.SrcInfo())){ return false; } break;
    case ExprOperator::BitwiseXor:     if(!OprBitwiseXor    (Result,Arg1,Arg2,Oper.SrcInfo())){ return false; } break;
    case ExprOperator::BitwiseOr:      if(!OprBitwiseOr     (Result,Arg1,Arg2,Oper.SrcInfo())){ return false; } break;
    case ExprOperator::LogicalNot:     if(!OprLogicalNot    (Result,Arg1,     Oper.SrcInfo())){ return false; } break;
    case ExprOperator::LogicalAnd:     if(!OprLogicalAnd    (Result,Arg1,Arg2,Oper.SrcInfo())){ return false; } break;
    case ExprOperator::LogicalOr:      if(!OprLogicalOr     (Result,Arg1,Arg2,Oper.SrcInfo())){ return false; } break;

    //Type cast
    case ExprOperator::TypeCast:
      switch(_Md->Types[Oper.CastTypIndex].MstType){
        case MasterType::Boolean : Result=Arg1; if(!Result.ToBol()){ return false; } break;
        case MasterType::Char    : Result=Arg1; if(!Result.ToChr()){ return false; } break;
        case MasterType::Short   : Result=Arg1; if(!Result.ToShr()){ return false; } break;
        case MasterType::Integer : Result=Arg1; if(!Result.ToInt()){ return false; } break;
        case MasterType::Long    : Result=Arg1; if(!Result.ToLon()){ return false; } break;
        case MasterType::Float   : Result=Arg1; if(!Result.ToFlo()){ return false; } break;
        case MasterType::String  : 
        case MasterType::Class   : 
        case MasterType::Enum    : 
        case MasterType::FixArray:
        case MasterType::DynArray:
          Oper.Msg(152).Print(MasterTypeName(_Md->Types[Oper.CastTypIndex].MstType));
          return false;
          break;
      }
      Result.LitNumTypIndex=Oper.CastTypIndex;
      break;

    //Other operators not supported
    case ExprOperator::PostfixInc:
    case ExprOperator::PostfixDec:
    case ExprOperator::PrefixInc :
    case ExprOperator::PrefixDec :
    case ExprOperator::Initializ :
    case ExprOperator::Assign    :
    case ExprOperator::AddAssign :
    case ExprOperator::SubAssign :
    case ExprOperator::MulAssign :
    case ExprOperator::DivAssign :
    case ExprOperator::ModAssign :
    case ExprOperator::ShlAssign :
    case ExprOperator::ShrAssign :
    case ExprOperator::AndAssign :
    case ExprOperator::XorAssign :
    case ExprOperator::OrAssign  :
    case ExprOperator::SeqOper   :
      Oper.Msg(153).Print(_Opr[(int)Oper.Value.Operator].Name.Trim());
      return false;
      break;
  
  }

  //Return code
  return true;

}

//Expression computer
bool Expression::_Compute(const ScopeDef& Scope,ExprToken& Result) const {
  bool IsComp;
  return _Compute(Scope,Result,false,IsComp);
}

//Expression computer
//TestMode: When enabled the goal is to detect is expression is computable
//IsComputable: Returns if expression is computable when TestMode flag is true
bool Expression::_Compute(const ScopeDef& Scope,ExprToken& Result,bool TestMode,bool& IsComputable) const {

  //Variables
  int i;
  int FunIndex;
  String Label;
  Stack<ExprToken> OpndStack;
  ExprToken Opnd0;
  ExprToken Opnd1;
  ExprToken Opnd2;
  String ParmStr;
  String Matches;

  //Token loop
  for(i=0;i<_Tokens.Length();i++){

    //Switch on token id
    switch(_Tokens[i].Id()){
      
      //Operand
      case ExprTokenId::Operand:
        Opnd0=_Tokens[i];
        if(!Opnd0.IsComputableOperand()){ 
          if(TestMode){ IsComputable=false; return true; } 
          else{ Opnd0.Msg(535).Print(); return false; }
        }
        if(!Opnd0.ToLitValue()){ return false; }
        OpndStack.Push(Opnd0);
        break;
      
      //Operator
      case ExprTokenId::Operator:
        
        //Check operator is computable in test mode
        if(!_Tokens[i].IsComputableOperator()){ 
          if(TestMode){ IsComputable=false; return true; }
          else{ Opnd0.Msg(536).Print(_Opr[(int)_Tokens[i].Value.Operator].Name.Trim()); return false; }
        }

        //Get operands from stack
        switch(_Opr[(int)_Tokens[i].Value.Operator].Class){
          case ExprOpClass::Unary:  
            if(OpndStack.Length()<1){ _Tokens[i].Msg(69).Print(_Opr[(int)_Tokens[i].Value.Operator].Name.Trim()); return false; } 
            Opnd1=OpndStack.Pop();
            break;
          case ExprOpClass::Binary: 
            if(OpndStack.Length()<2){ _Tokens[i].Msg(70).Print(_Opr[(int)_Tokens[i].Value.Operator].Name.Trim()); return false; } 
            Opnd2=OpndStack.Pop();
            Opnd1=OpndStack.Pop();
            break;
        }
        
        //Set operands as used in source
        //For first operand we do this only when operator is not last one, since in that case result is not further used, or when expression has mandatory result
        if(_Opr[(int)_Tokens[i].Value.Operator].Class==ExprOpClass::Unary || _Opr[(int)_Tokens[i].Value.Operator].Class==ExprOpClass::Binary){
          if(_Tokens[i].Value.Operator!=ExprOperator::Initializ && _Tokens[i].Value.Operator!=ExprOperator::SeqOper){
            Opnd1.SetSourceUsed(Scope,!_Opr[(int)_Tokens[i].Value.Operator].IsResultSecond);
          }
        }
        if(_Opr[(int)_Tokens[i].Value.Operator].Class==ExprOpClass::Binary){
          Opnd2.SetSourceUsed(Scope,true);
        }

        //Find operator overload function
        switch(_Opr[(int)_Tokens[i].Value.Operator].Class){
          case ExprOpClass::Unary:  
            ParmStr=(Opnd1.Id()==ExprTokenId::VoidRes?"void":_Md->Types[_Md->WordTypeFilter(Opnd1.TypIndex(),true)].Name); 
            break;
          case ExprOpClass::Binary: 
            ParmStr=
            (Opnd1.Id()==ExprTokenId::VoidRes?"void":_Md->Types[_Md->WordTypeFilter(Opnd1.TypIndex(),true)].Name)+","+
            (Opnd2.Id()==ExprTokenId::VoidRes?"void":_Md->Types[_Md->WordTypeFilter(Opnd2.TypIndex(),true)].Name); 
            break;
        }
        FunIndex=_Md->OprSearch(_Md->GetOperatorFuncName(_Opr[(int)_Tokens[i].Value.Operator].Name),ParmStr,"",&Matches);
        if(FunIndex!=-1){ 
          if(TestMode){ IsComputable=false; return true; }
          else{ Opnd0.Msg(537).Print(_Opr[(int)_Tokens[i].Value.Operator].Name.Trim(),ParmStr); return false; }
        }

        //Compute operation
        if(!_ComputeOperation(Scope,_Tokens[i],Opnd1,Opnd2,Result)){ return false; }

        //Push result to operand stack
        Result.IsCalculated=true;
        OpndStack.Push(Result);
        break;
      
      //Other tokens not supported
      case ExprTokenId::UndefVar   :
      case ExprTokenId::VoidRes    :
      case ExprTokenId::LowLevelOpr:
      case ExprTokenId::FlowOpr:
      case ExprTokenId::Field      :
      case ExprTokenId::Method     :
      case ExprTokenId::Constructor:
      case ExprTokenId::Complex    :
      case ExprTokenId::Subscript  :
      case ExprTokenId::Function   :
      case ExprTokenId::Delimiter  :
        if(TestMode){ IsComputable=false; return true; }
        else{ _Tokens[i].Msg(155).Print(); return false; }
        break;
    }

    //Debug message
    #ifdef __DEV__
      String OpndStackStr;
      for(int j=0;j<OpndStack.Length();j++){ OpndStackStr+=(OpndStackStr.Length()!=0?" ":"")+OpndStack[j].Print(); }
      DebugMessage(DebugLevel::CmpExpression,"Token "+_Tokens[i].Print()+" --> Operand stack {"+OpndStackStr+"}");
    #endif

  }

  //Check there are delayed error messages before exitting
  if(SysMessage().DelayCount()!=0){
    return false;
  }

  //At end of evaluation operand stack must contain the result to be returned
  if(OpndStack.Length()>1){
    _Tokens[0].Msg(104).Print();
    return false;
  }
  else if(OpndStack.Length()==0){
    _Tokens[0].Msg(275).Print();
    return false;
  }
  else if(OpndStack.Length()==1){
    Result=OpndStack[0];
  }

  //Print result
  #ifdef __DEV__
    DebugMessage(DebugLevel::CmpExpression,"Result token: "+Result.Print());
  #endif

  //Return computable expresion in test mode
  if(TestMode){ IsComputable=true; }

  //Return code
  return true;

}

//Compute expression when result is expected
bool Expression::Compute(MasterData *Md,const ScopeDef& Scope,Sentence& Stn,int BegToken,int EndToken,ExprToken& Result){
  _Md=Md;
  _FileName=Stn.FileName();
  _LineNr=Stn.LineNr();
  _Tokens.Reset();
  _Origin=Stn.Origin();
  DebugMessage(DebugLevel::CmpExpression,"ExprComputer input: "+Stn.Print(BegToken,EndToken));
  if(!_Tokenize(Scope,Stn,BegToken,EndToken)){ return false; }
  if(!_Infix2RPN()){ return false; }
  if(!_Compute(Scope,Result)){ return false;}
  return true;
}

//Check expression is computable
bool Expression::_IsComputable(const ScopeDef& Scope,bool& IsComp) const {
  ExprToken Result;
  return _Compute(Scope,Result,true,IsComp);
}