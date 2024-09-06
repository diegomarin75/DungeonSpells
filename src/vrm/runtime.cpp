//runtime.cpp: runtime class
#include "bas/basedefs.hpp"
#include "bas/allocator.hpp"
#include "bas/array.hpp"
#include "bas/sortedarray.hpp"
#include "bas/stack.hpp"
#include "bas/buffer.hpp"
#include "bas/string.hpp"
#include "sys/sysdefs.hpp"
#include "sys/system.hpp"
#include "sys/stl.hpp"
#include "sys/msgout.hpp"
#include "vrm/pool.hpp"
#include "vrm/memman.hpp"
#include "vrm/auxmem.hpp"
#include "vrm/strcomp.hpp"
#include "vrm/arrcomp.hpp"
#include "vrm/runtime.hpp"

//Default memory buffers data chunk sizes
#define DEFAULT_CHUNKSIZE_GLOB      65536L   //Default program buffer chunk sizes for Glob buffer
#define DEFAULT_CHUNKSIZE_STACK     65536L   //Default program buffer chunk sizes for Stack buffer
#define DEFAULT_CHUNKSIZE_CODE      65536L   //Default program buffer chunk sizes for Code buffer
#define DEFAULT_CHUNKSIZE_CALLST    64L      //Default program buffer chunk sizes for CallSt buffer
#define DEFAULT_CHUNKSIZE_PARMST    16384L   //Default program buffer chunk sizes for ParmSt buffer
#define DEFAULT_CHUNKSIZE_PARAM     256L     //Default program buffer chunk sizes for Param buffer
#define DEFAULT_CHUNKSIZE_PARMPTR   256L     //Default program buffer chunk sizes for ParmPtr buffer
#define DEFAULT_CHUNKSIZE_ARRMETA   64L      //Default program buffer chunk sizes for ArrMeta buffer
#define DEFAULT_CHUNKSIZE_ARRGEOM   64L      //Default program buffer chunk sizes for ArrGeom buffer
#define DEFAULT_CHUNKSIZE_RPRULE    64L      //Default program buffer chunk sizes for RpRule buffer 
#define DEFAULT_CHUNKSIZE_DLCALL    64L      //Default program buffer chunk sizes for DlCall buffer 
#define DEFAULT_CHUNKSIZE_DYNLIB    64L      //Default program buffer chunk sizes for DynLib buffer 
#define DEFAULT_CHUNKSIZE_DYNFUN    64L      //Default program buffer chunk sizes for DynFun buffer 
#define DEFAULT_CHUNKSIZE_DBGSYMMOD 64L      //Default program buffer chunk size for debug symbol modules
#define DEFAULT_CHUNKSIZE_DBGSYMTYP 64L      //Default program buffer chunk size for debug symbol types
#define DEFAULT_CHUNKSIZE_DBGSYMVAR 64L      //Default program buffer chunk size for debug symbol variables
#define DEFAULT_CHUNKSIZE_DBGSYMFLD 64L      //Default program buffer chunk size for debug symbol fields
#define DEFAULT_CHUNKSIZE_DBGSYMFUN 64L      //Default program buffer chunk size for debug symbol functions
#define DEFAULT_CHUNKSIZE_DBGSYMPAR 64L      //Default program buffer chunk size for debug symbol parameters
#define DEFAULT_CHUNKSIZE_DBGSYMLIN 64L      //Default program buffer chunk size for debug symbol source code lines

//Maximun string printout length in debug messages
#define MAX_STRING_DEBUG_PRINT 9999 //255

//Global scope id identifier
const CpuMbl _GlobalScopeId=BLOCKMASK7F;    

//Pointer to current runtime instance
Runtime *_Rt=nullptr;

// Runtime local variables, only used in _RunProgram() -----------------------------
// (they are here to avoid optimization, as they do not belong to fast path) 

//Pointers
char *GlobPnt;  //Pointer to glob buffer
char *StackPnt; //Pointer to stack buffer

//Instruction tables
const void *FakeInstAddress[_InstructionNr];    //Fake instruction table
const void **InstAddressConstPtr;               //Instruction table pointer (constant)

//Control registers
CpuAdr PST;              //Parameter stack pointer
char *TPTR;          //Temporary pointer
CpuWrd *WRDP;        //Auxiliar word pointer
CpuIcd ICODE;        //Current instruction code
CpuMbl TRYBLK;       //Trial block (CHECK_SAFE_ADDRESSING)
CpuAdr TRYADR;       //Trial address (CHECK_SAFE_ADDRESSING)
CpuAdr MAXADR;       //Maximun address (CHECK_SAFE_ADDRESSING)
CpuWrd OFF;          //Used for offset calculations
CpuAdr INDXADDR;     //Index variable address on array loop
CpuDecMode INDXDMOD; //Index variable decoder mode on array loop
CpuAdr EXITADR;      //Used array loop exit jump
CpuInt SCNR;         //Last system call executed
CpuInt LCNR;         //Last dynamic library call executed
CallStack RETADR;    //Return address
CpuLon CUMULSC;      //Acumulated scope number (never decreases, ensures ScopeNr is always different)
CpuMbl DSOZ;         //Necessary just to avoid compiler error, not used
String STR;          //Temporary string
void *VPTR;          //Void pointer used in library push instructions
CpuMbl *BLK;         //Used in library push instructions
CpuWrd *HPTR;        //Handler pointer to the instruction to be restored
CpuIcd HICD;         //Instruction code related to the instruction to be restored
CpuAdr PREVIP;       //Previous IP
CpuAdr PREVBP;       //Previous BP
int CFIX;            //Current function index

//Reference indirections
CpuMbl DSO1,DSO2,DSO3,DSO4,DSO5,DSO6; //Decoder argument scope
CpuRef DRF1,DRF2,DRF3,DRF4,DRF5,DRF6; //Decoder reference

//Cpu decoding
CpuDecMode DMOD1,DMOD2,DMOD3,DMOD4; //Argument decoder modes
CpuAdr ADV1,ADV2,ADV3,ADV4;         //Argument address value (used by decoder argument instructions)
CpuAdr *ADP1,*ADP2,*ADP3,*ADP4;     //Argument address pointer (used by decoder argument instructions)

// ---------------------------------------------------------------------------------

//Internal functions
void SetCurrentRuntime(Runtime *Rt);
int _GetCurrentProcess();

// Windows only specific -----------------------------------------------------------
#ifdef __WIN__
String _GetLastError();

//Linux only specific --------------------------------------------------------------
#else
enum class FdStatus{ Readable, Closed, Timeout, Error };
bool _GetFdStatus(int Fd,int MaxTimeInSeconds,FdStatus &Status);
#endif

//Define instruction labels
#define INST_LABEL_TABLE \
  const void *InstAddress[_InstructionNr]={ \
  &&InstLabelNEGc, \
  &&InstLabelNEGw, \
  &&InstLabelNEGi, \
  &&InstLabelNEGl, \
  &&InstLabelNEGf, \
  &&InstLabelADDc, \
  &&InstLabelADDw, \
  &&InstLabelADDi, \
  &&InstLabelADDl, \
  &&InstLabelADDf, \
  &&InstLabelSUBc, \
  &&InstLabelSUBw, \
  &&InstLabelSUBi, \
  &&InstLabelSUBl, \
  &&InstLabelSUBf, \
  &&InstLabelMULc, \
  &&InstLabelMULw, \
  &&InstLabelMULi, \
  &&InstLabelMULl, \
  &&InstLabelMULf, \
  &&InstLabelDIVc, \
  &&InstLabelDIVw, \
  &&InstLabelDIVi, \
  &&InstLabelDIVl, \
  &&InstLabelDIVf, \
  &&InstLabelMODc, \
  &&InstLabelMODw, \
  &&InstLabelMODi, \
  &&InstLabelMODl, \
  &&InstLabelINCc, \
  &&InstLabelINCw, \
  &&InstLabelINCi, \
  &&InstLabelINCl, \
  &&InstLabelINCf, \
  &&InstLabelDECc, \
  &&InstLabelDECw, \
  &&InstLabelDECi, \
  &&InstLabelDECl, \
  &&InstLabelDECf, \
  &&InstLabelPINCc, \
  &&InstLabelPINCw, \
  &&InstLabelPINCi, \
  &&InstLabelPINCl, \
  &&InstLabelPINCf, \
  &&InstLabelPDECc, \
  &&InstLabelPDECw, \
  &&InstLabelPDECi, \
  &&InstLabelPDECl, \
  &&InstLabelPDECf, \
  &&InstLabelLNOT, \
  &&InstLabelLAND, \
  &&InstLabelLOR, \
  &&InstLabelBNOTc, \
  &&InstLabelBNOTw, \
  &&InstLabelBNOTi, \
  &&InstLabelBNOTl, \
  &&InstLabelBANDc, \
  &&InstLabelBANDw, \
  &&InstLabelBANDi, \
  &&InstLabelBANDl, \
  &&InstLabelBORc, \
  &&InstLabelBORw, \
  &&InstLabelBORi, \
  &&InstLabelBORl, \
  &&InstLabelBXORc, \
  &&InstLabelBXORw, \
  &&InstLabelBXORi, \
  &&InstLabelBXORl, \
  &&InstLabelSHLc, \
  &&InstLabelSHLw, \
  &&InstLabelSHLi, \
  &&InstLabelSHLl, \
  &&InstLabelSHRc, \
  &&InstLabelSHRw, \
  &&InstLabelSHRi, \
  &&InstLabelSHRl, \
  &&InstLabelLESb, \
  &&InstLabelLESc, \
  &&InstLabelLESw, \
  &&InstLabelLESi, \
  &&InstLabelLESl, \
  &&InstLabelLESf, \
  &&InstLabelLESs, \
  &&InstLabelLEQb, \
  &&InstLabelLEQc, \
  &&InstLabelLEQw, \
  &&InstLabelLEQi, \
  &&InstLabelLEQl, \
  &&InstLabelLEQf, \
  &&InstLabelLEQs, \
  &&InstLabelGREb, \
  &&InstLabelGREc, \
  &&InstLabelGREw, \
  &&InstLabelGREi, \
  &&InstLabelGREl, \
  &&InstLabelGREf, \
  &&InstLabelGREs, \
  &&InstLabelGEQb, \
  &&InstLabelGEQc, \
  &&InstLabelGEQw, \
  &&InstLabelGEQi, \
  &&InstLabelGEQl, \
  &&InstLabelGEQf, \
  &&InstLabelGEQs, \
  &&InstLabelEQUb, \
  &&InstLabelEQUc, \
  &&InstLabelEQUw, \
  &&InstLabelEQUi, \
  &&InstLabelEQUl, \
  &&InstLabelEQUf, \
  &&InstLabelEQUs, \
  &&InstLabelDISb, \
  &&InstLabelDISc, \
  &&InstLabelDISw, \
  &&InstLabelDISi, \
  &&InstLabelDISl, \
  &&InstLabelDISf, \
  &&InstLabelDISs, \
  &&InstLabelMVb, \
  &&InstLabelMVc, \
  &&InstLabelMVw, \
  &&InstLabelMVi, \
  &&InstLabelMVl, \
  &&InstLabelMVf, \
  &&InstLabelMVr, \
  &&InstLabelLOADb, \
  &&InstLabelLOADc, \
  &&InstLabelLOADw, \
  &&InstLabelLOADi, \
  &&InstLabelLOADl, \
  &&InstLabelLOADf, \
  &&InstLabelMVADc, \
  &&InstLabelMVADw, \
  &&InstLabelMVADi, \
  &&InstLabelMVADl, \
  &&InstLabelMVADf, \
  &&InstLabelMVSUc, \
  &&InstLabelMVSUw, \
  &&InstLabelMVSUi, \
  &&InstLabelMVSUl, \
  &&InstLabelMVSUf, \
  &&InstLabelMVMUc, \
  &&InstLabelMVMUw, \
  &&InstLabelMVMUi, \
  &&InstLabelMVMUl, \
  &&InstLabelMVMUf, \
  &&InstLabelMVDIc, \
  &&InstLabelMVDIw, \
  &&InstLabelMVDIi, \
  &&InstLabelMVDIl, \
  &&InstLabelMVDIf, \
  &&InstLabelMVMOc, \
  &&InstLabelMVMOw, \
  &&InstLabelMVMOi, \
  &&InstLabelMVMOl, \
  &&InstLabelMVSLc, \
  &&InstLabelMVSLw, \
  &&InstLabelMVSLi, \
  &&InstLabelMVSLl, \
  &&InstLabelMVSRc, \
  &&InstLabelMVSRw, \
  &&InstLabelMVSRi, \
  &&InstLabelMVSRl, \
  &&InstLabelMVANc, \
  &&InstLabelMVANw, \
  &&InstLabelMVANi, \
  &&InstLabelMVANl, \
  &&InstLabelMVXOc, \
  &&InstLabelMVXOw, \
  &&InstLabelMVXOi, \
  &&InstLabelMVXOl, \
  &&InstLabelMVORc, \
  &&InstLabelMVORw, \
  &&InstLabelMVORi, \
  &&InstLabelMVORl, \
  &&InstLabelRPBEG, \
  &&InstLabelRPSTR, \
  &&InstLabelRPARR, \
  &&InstLabelRPLOF, \
  &&InstLabelRPLOD, \
  &&InstLabelRPEND, \
  &&InstLabelBIBEG, \
  &&InstLabelBISTR, \
  &&InstLabelBIARR, \
  &&InstLabelBILOF, \
  &&InstLabelBIEND, \
  &&InstLabelREFOF, \
  &&InstLabelREFAD, \
  &&InstLabelREFER, \
  &&InstLabelCOPY, \
  &&InstLabelSCOPY, \
  &&InstLabelSSWCP, \
  &&InstLabelACOPY, \
  &&InstLabelTOCA, \
  &&InstLabelSTOCA, \
  &&InstLabelATOCA, \
  &&InstLabelFRCA, \
  &&InstLabelSFRCA, \
  &&InstLabelAFRCA, \
  &&InstLabelCLEAR, \
  &&InstLabelSTACK, \
  &&InstLabelAF1RF, \
  &&InstLabelAF1RW, \
  &&InstLabelAF1FO, \
  &&InstLabelAF1NX, \
  &&InstLabelAF1SJ, \
  &&InstLabelAF1CJ, \
  &&InstLabelAFDEF, \
  &&InstLabelAFSSZ, \
  &&InstLabelAFGET, \
  &&InstLabelAFIDX, \
  &&InstLabelAFREF, \
  &&InstLabelAD1EM, \
  &&InstLabelAD1DF, \
  &&InstLabelAD1AP, \
  &&InstLabelAD1IN, \
  &&InstLabelAD1DE, \
  &&InstLabelAD1RF, \
  &&InstLabelAD1RS, \
  &&InstLabelAD1RW, \
  &&InstLabelAD1FO, \
  &&InstLabelAD1NX, \
  &&InstLabelAD1SJ, \
  &&InstLabelAD1CJ, \
  &&InstLabelADEMP, \
  &&InstLabelADDEF, \
  &&InstLabelADSET, \
  &&InstLabelADRSZ, \
  &&InstLabelADGET, \
  &&InstLabelADRST, \
  &&InstLabelADIDX, \
  &&InstLabelADREF, \
  &&InstLabelADSIZ, \
  &&InstLabelAF2F, \
  &&InstLabelAF2D, \
  &&InstLabelAD2F, \
  &&InstLabelAD2D, \
  &&InstLabelPUSHb, \
  &&InstLabelPUSHc, \
  &&InstLabelPUSHw, \
  &&InstLabelPUSHi, \
  &&InstLabelPUSHl, \
  &&InstLabelPUSHf, \
  &&InstLabelPUSHr, \
  &&InstLabelREFPU, \
  &&InstLabelLPUb, \
  &&InstLabelLPUc, \
  &&InstLabelLPUw, \
  &&InstLabelLPUi, \
  &&InstLabelLPUl, \
  &&InstLabelLPUf, \
  &&InstLabelLPUr, \
  &&InstLabelLPUSr, \
  &&InstLabelLPADr, \
  &&InstLabelLPAFr, \
  &&InstLabelLRPU, \
  &&InstLabelLRPUS, \
  &&InstLabelLRPAD, \
  &&InstLabelLRPAF, \
  &&InstLabelCALL, \
  &&InstLabelRET, \
  &&InstLabelCALLN, \
  &&InstLabelRETN, \
  &&InstLabelSCALL, \
  &&InstLabelLCALL, \
  &&InstLabelSULOK, \
  &&InstLabelCUPPR, \
  &&InstLabelCLOWR, \
  &&InstLabelSEMP, \
  &&InstLabelSLEN, \
  &&InstLabelSMID, \
  &&InstLabelSINDX, \
  &&InstLabelSRGHT, \
  &&InstLabelSLEFT, \
  &&InstLabelSCUTR, \
  &&InstLabelSCUTL, \
  &&InstLabelSCONC, \
  &&InstLabelSMVCO, \
  &&InstLabelSMVRC, \
  &&InstLabelSFIND, \
  &&InstLabelSSUBS, \
  &&InstLabelSTRIM, \
  &&InstLabelSUPPR, \
  &&InstLabelSLOWR, \
  &&InstLabelSLJUS, \
  &&InstLabelSRJUS, \
  &&InstLabelSMATC, \
  &&InstLabelSLIKE, \
  &&InstLabelSREPL, \
  &&InstLabelSSPLI, \
  &&InstLabelSSTWI, \
  &&InstLabelSENWI, \
  &&InstLabelSISBO, \
  &&InstLabelSISCH, \
  &&InstLabelSISSH, \
  &&InstLabelSISIN, \
  &&InstLabelSISLO, \
  &&InstLabelSISFL, \
  &&InstLabelBO2CH, \
  &&InstLabelBO2SH, \
  &&InstLabelBO2IN, \
  &&InstLabelBO2LO, \
  &&InstLabelBO2FL, \
  &&InstLabelBO2ST, \
  &&InstLabelCH2BO, \
  &&InstLabelCH2SH, \
  &&InstLabelCH2IN, \
  &&InstLabelCH2LO, \
  &&InstLabelCH2FL, \
  &&InstLabelCH2ST, \
  &&InstLabelCHFMT, \
  &&InstLabelSH2BO, \
  &&InstLabelSH2CH, \
  &&InstLabelSH2IN, \
  &&InstLabelSH2LO, \
  &&InstLabelSH2FL, \
  &&InstLabelSH2ST, \
  &&InstLabelSHFMT, \
  &&InstLabelIN2BO, \
  &&InstLabelIN2CH, \
  &&InstLabelIN2SH, \
  &&InstLabelIN2LO, \
  &&InstLabelIN2FL, \
  &&InstLabelIN2ST, \
  &&InstLabelINFMT, \
  &&InstLabelLO2BO, \
  &&InstLabelLO2CH, \
  &&InstLabelLO2SH, \
  &&InstLabelLO2IN, \
  &&InstLabelLO2FL, \
  &&InstLabelLO2ST, \
  &&InstLabelLOFMT, \
  &&InstLabelFL2BO, \
  &&InstLabelFL2CH, \
  &&InstLabelFL2SH, \
  &&InstLabelFL2IN, \
  &&InstLabelFL2LO, \
  &&InstLabelFL2ST, \
  &&InstLabelFLFMT, \
  &&InstLabelST2BO, \
  &&InstLabelST2CH, \
  &&InstLabelST2SH, \
  &&InstLabelST2IN, \
  &&InstLabelST2LO, \
  &&InstLabelST2FL, \
  &&InstLabelJMPTR, \
  &&InstLabelJMPFL, \
  &&InstLabelJMP, \
  &&InstLabelDAGV1, \
  &&InstLabelDAGV2, \
  &&InstLabelDAGV3, \
  &&InstLabelDAGV4, \
  &&InstLabelDAGI1, \
  &&InstLabelDAGI2, \
  &&InstLabelDAGI3, \
  &&InstLabelDAGI4, \
  &&InstLabelDALI1, \
  &&InstLabelDALI2, \
  &&InstLabelDALI3, \
  &&InstLabelDALI4, \
  &&InstLabelNOP \
  }; \

//Instruction switcher block
#define INST_SWITCHER \
INST_NEGc; \
INST_NEGw; \
INST_NEGi; \
INST_NEGl; \
INST_NEGf; \
INST_ADDc; \
INST_ADDw; \
INST_ADDi; \
INST_ADDl; \
INST_ADDf; \
INST_SUBc; \
INST_SUBw; \
INST_SUBi; \
INST_SUBl; \
INST_SUBf; \
INST_MULc; \
INST_MULw; \
INST_MULi; \
INST_MULl; \
INST_MULf; \
INST_DIVc; \
INST_DIVw; \
INST_DIVi; \
INST_DIVl; \
INST_DIVf; \
INST_MODc; \
INST_MODw; \
INST_MODi; \
INST_MODl; \
INST_INCc; \
INST_INCw; \
INST_INCi; \
INST_INCl; \
INST_INCf; \
INST_DECc; \
INST_DECw; \
INST_DECi; \
INST_DECl; \
INST_DECf; \
INST_PINCc; \
INST_PINCw; \
INST_PINCi; \
INST_PINCl; \
INST_PINCf; \
INST_PDECc; \
INST_PDECw; \
INST_PDECi; \
INST_PDECl; \
INST_PDECf; \
INST_LNOT; \
INST_LAND; \
INST_LOR; \
INST_BNOTc; \
INST_BNOTw; \
INST_BNOTi; \
INST_BNOTl; \
INST_BANDc; \
INST_BANDw; \
INST_BANDi; \
INST_BANDl; \
INST_BORc; \
INST_BORw; \
INST_BORi; \
INST_BORl; \
INST_BXORc; \
INST_BXORw; \
INST_BXORi; \
INST_BXORl; \
INST_SHLc; \
INST_SHLw; \
INST_SHLi; \
INST_SHLl; \
INST_SHRc; \
INST_SHRw; \
INST_SHRi; \
INST_SHRl; \
INST_LESb; \
INST_LESc; \
INST_LESw; \
INST_LESi; \
INST_LESl; \
INST_LESf; \
INST_LESs; \
INST_LEQb; \
INST_LEQc; \
INST_LEQw; \
INST_LEQi; \
INST_LEQl; \
INST_LEQf; \
INST_LEQs; \
INST_GREb; \
INST_GREc; \
INST_GREw; \
INST_GREi; \
INST_GREl; \
INST_GREf; \
INST_GREs; \
INST_GEQb; \
INST_GEQc; \
INST_GEQw; \
INST_GEQi; \
INST_GEQl; \
INST_GEQf; \
INST_GEQs; \
INST_EQUb; \
INST_EQUc; \
INST_EQUw; \
INST_EQUi; \
INST_EQUl; \
INST_EQUf; \
INST_EQUs; \
INST_DISb; \
INST_DISc; \
INST_DISw; \
INST_DISi; \
INST_DISl; \
INST_DISf; \
INST_DISs; \
INST_MVb; \
INST_MVc; \
INST_MVw; \
INST_MVi; \
INST_MVl; \
INST_MVf; \
INST_MVr; \
INST_LOADb; \
INST_LOADc; \
INST_LOADw; \
INST_LOADi; \
INST_LOADl; \
INST_LOADf; \
INST_MVADc; \
INST_MVADw; \
INST_MVADi; \
INST_MVADl; \
INST_MVADf; \
INST_MVSUc; \
INST_MVSUw; \
INST_MVSUi; \
INST_MVSUl; \
INST_MVSUf; \
INST_MVMUc; \
INST_MVMUw; \
INST_MVMUi; \
INST_MVMUl; \
INST_MVMUf; \
INST_MVDIc; \
INST_MVDIw; \
INST_MVDIi; \
INST_MVDIl; \
INST_MVDIf; \
INST_MVMOc; \
INST_MVMOw; \
INST_MVMOi; \
INST_MVMOl; \
INST_MVSLc; \
INST_MVSLw; \
INST_MVSLi; \
INST_MVSLl; \
INST_MVSRc; \
INST_MVSRw; \
INST_MVSRi; \
INST_MVSRl; \
INST_MVANc; \
INST_MVANw; \
INST_MVANi; \
INST_MVANl; \
INST_MVXOc; \
INST_MVXOw; \
INST_MVXOi; \
INST_MVXOl; \
INST_MVORc; \
INST_MVORw; \
INST_MVORi; \
INST_MVORl; \
INST_RPBEG; \
INST_RPSTR; \
INST_RPARR; \
INST_RPLOF; \
INST_RPLOD; \
INST_RPEND; \
INST_BIBEG; \
INST_BISTR; \
INST_BIARR; \
INST_BILOF; \
INST_BIEND; \
INST_REFOF; \
INST_REFAD; \
INST_REFER; \
INST_COPY; \
INST_SCOPY; \
INST_SSWCP; \
INST_ACOPY; \
INST_TOCA; \
INST_STOCA; \
INST_ATOCA; \
INST_FRCA; \
INST_SFRCA; \
INST_AFRCA; \
INST_CLEAR; \
INST_STACK; \
INST_AF1RF; \
INST_AF1RW; \
INST_AF1FO; \
INST_AF1NX; \
INST_AF1SJ; \
INST_AF1CJ; \
INST_AFDEF; \
INST_AFSSZ; \
INST_AFGET; \
INST_AFIDX; \
INST_AFREF; \
INST_AD1EM; \
INST_AD1DF; \
INST_AD1AP; \
INST_AD1IN; \
INST_AD1DE; \
INST_AD1RF; \
INST_AD1RS; \
INST_AD1RW; \
INST_AD1FO; \
INST_AD1NX; \
INST_AD1SJ; \
INST_AD1CJ; \
INST_ADEMP; \
INST_ADDEF; \
INST_ADSET; \
INST_ADRSZ; \
INST_ADGET; \
INST_ADRST; \
INST_ADIDX; \
INST_ADREF; \
INST_ADSIZ; \
INST_AF2F; \
INST_AF2D; \
INST_AD2F; \
INST_AD2D; \
INST_PUSHb; \
INST_PUSHc; \
INST_PUSHw; \
INST_PUSHi; \
INST_PUSHl; \
INST_PUSHf; \
INST_PUSHr; \
INST_REFPU; \
INST_LPUb; \
INST_LPUc; \
INST_LPUw; \
INST_LPUi; \
INST_LPUl; \
INST_LPUf; \
INST_LPUr; \
INST_LPUSr; \
INST_LPADr; \
INST_LPAFr; \
INST_LRPU; \
INST_LRPUS; \
INST_LRPAD; \
INST_LRPAF; \
INST_CALL; \
INST_RET; \
INST_CALLN; \
INST_RETN; \
INST_SCALL; \
INST_LCALL; \
INST_SULOK; \
INST_CUPPR; \
INST_CLOWR; \
INST_SEMP; \
INST_SLEN; \
INST_SMID; \
INST_SINDX; \
INST_SRGHT; \
INST_SLEFT; \
INST_SCUTR; \
INST_SCUTL; \
INST_SCONC; \
INST_SMVCO; \
INST_SMVRC; \
INST_SFIND; \
INST_SSUBS; \
INST_STRIM; \
INST_SUPPR; \
INST_SLOWR; \
INST_SLJUS; \
INST_SRJUS; \
INST_SMATC; \
INST_SLIKE; \
INST_SREPL; \
INST_SSPLI; \
INST_SSTWI; \
INST_SENWI; \
INST_SISBO; \
INST_SISCH; \
INST_SISSH; \
INST_SISIN; \
INST_SISLO; \
INST_SISFL; \
INST_BO2CH; \
INST_BO2SH; \
INST_BO2IN; \
INST_BO2LO; \
INST_BO2FL; \
INST_BO2ST; \
INST_CH2BO; \
INST_CH2SH; \
INST_CH2IN; \
INST_CH2LO; \
INST_CH2FL; \
INST_CH2ST; \
INST_CHFMT; \
INST_SH2BO; \
INST_SH2CH; \
INST_SH2IN; \
INST_SH2LO; \
INST_SH2FL; \
INST_SH2ST; \
INST_SHFMT; \
INST_IN2BO; \
INST_IN2CH; \
INST_IN2SH; \
INST_IN2LO; \
INST_IN2FL; \
INST_IN2ST; \
INST_INFMT; \
INST_LO2BO; \
INST_LO2CH; \
INST_LO2SH; \
INST_LO2IN; \
INST_LO2FL; \
INST_LO2ST; \
INST_LOFMT; \
INST_FL2BO; \
INST_FL2CH; \
INST_FL2SH; \
INST_FL2IN; \
INST_FL2LO; \
INST_FL2ST; \
INST_FLFMT; \
INST_ST2BO; \
INST_ST2CH; \
INST_ST2SH; \
INST_ST2IN; \
INST_ST2LO; \
INST_ST2FL; \
INST_JMPTR; \
INST_JMPFL; \
INST_JMP; \
INST_DAGV1; \
INST_DAGV2; \
INST_DAGV3; \
INST_DAGV4; \
INST_DAGI1; \
INST_DAGI2; \
INST_DAGI3; \
INST_DAGI4; \
INST_DALI1; \
INST_DALI2; \
INST_DALI3; \
INST_DALI4; \
INST_NOP; \

//Define instruction labels
#define SYSTEMCALL_LABEL_TABLE \
  const void *SysCallLabelPtr[_SystemCallNr]={ \
  &&SystemCallLabelProgramExit, \
  &&SystemCallLabelPanic, \
  &&SystemCallLabelDelay, \
  &&SystemCallLabelExecute1, \
  &&SystemCallLabelExecute2, \
  &&SystemCallLabelError, \
  &&SystemCallLabelErrorText, \
  &&SystemCallLabelLastError, \
  &&SystemCallLabelGetArg, \
  &&SystemCallLabelHostSystem, \
  &&SystemCallLabelConsolePrint, \
  &&SystemCallLabelConsolePrintLine, \
  &&SystemCallLabelConsolePrintError, \
  &&SystemCallLabelConsolePrintErrorLine, \
  &&SystemCallLabelGetFileName, \
  &&SystemCallLabelGetFileNameNoExt, \
  &&SystemCallLabelGetFileExtension, \
  &&SystemCallLabelGetDirName, \
  &&SystemCallLabelFileExists, \
  &&SystemCallLabelDirExists, \
  &&SystemCallLabelGetHandler, \
  &&SystemCallLabelFreeHandler, \
  &&SystemCallLabelGetFileSize, \
  &&SystemCallLabelOpenForRead, \
  &&SystemCallLabelOpenForWrite, \
  &&SystemCallLabelOpenForAppend, \
  &&SystemCallLabelRead, \
  &&SystemCallLabelWrite, \
  &&SystemCallLabelReadAll, \
  &&SystemCallLabelWriteAll, \
  &&SystemCallLabelReadStr, \
  &&SystemCallLabelWriteStr, \
  &&SystemCallLabelReadStrAll, \
  &&SystemCallLabelWriteStrAll, \
  &&SystemCallLabelCloseFile, \
  &&SystemCallLabelHnd2File, \
  &&SystemCallLabelFile2Hnd, \
  &&SystemCallLabelAbsChr, \
  &&SystemCallLabelAbsShr, \
  &&SystemCallLabelAbsInt, \
  &&SystemCallLabelAbsLon, \
  &&SystemCallLabelAbsFlo, \
  &&SystemCallLabelMinChr, \
  &&SystemCallLabelMinShr, \
  &&SystemCallLabelMinInt, \
  &&SystemCallLabelMinLon, \
  &&SystemCallLabelMinFlo, \
  &&SystemCallLabelMaxChr, \
  &&SystemCallLabelMaxShr, \
  &&SystemCallLabelMaxInt, \
  &&SystemCallLabelMaxLon, \
  &&SystemCallLabelMaxFlo, \
  &&SystemCallLabelExp, \
  &&SystemCallLabelLn, \
  &&SystemCallLabelLog, \
  &&SystemCallLabelLogn, \
  &&SystemCallLabelPow, \
  &&SystemCallLabelSqrt, \
  &&SystemCallLabelCbrt, \
  &&SystemCallLabelSin, \
  &&SystemCallLabelCos, \
  &&SystemCallLabelTan, \
  &&SystemCallLabelAsin, \
  &&SystemCallLabelAcos, \
  &&SystemCallLabelAtan, \
  &&SystemCallLabelSinh, \
  &&SystemCallLabelCosh, \
  &&SystemCallLabelTanh, \
  &&SystemCallLabelAsinh, \
  &&SystemCallLabelAcosh, \
  &&SystemCallLabelAtanh, \
  &&SystemCallLabelCeil, \
  &&SystemCallLabelFloor, \
  &&SystemCallLabelRound, \
  &&SystemCallLabelSeed, \
  &&SystemCallLabelRand, \
  &&SystemCallLabelDateValid, \
  &&SystemCallLabelDateValue, \
  &&SystemCallLabelBegOfMonth, \
  &&SystemCallLabelEndOfMonth, \
  &&SystemCallLabelDatePart, \
  &&SystemCallLabelDateAdd, \
  &&SystemCallLabelTimeValid, \
  &&SystemCallLabelTimeValue, \
  &&SystemCallLabelTimePart, \
  &&SystemCallLabelTimeAdd, \
  &&SystemCallLabelNanoSecAdd, \
  &&SystemCallLabelGetDate, \
  &&SystemCallLabelGetTime, \
  &&SystemCallLabelDateDiff, \
  &&SystemCallLabelTimeDiff \
}; \

#define SYSTEMCALL_SWITCHER \
SYSCALL_PROGRAMEXIT; \
SYSCALL_PANIC; \
SYSCALL_DELAY; \
SYSCALL_EXECUTE1; \
SYSCALL_EXECUTE2; \
SYSCALL_ERROR; \
SYSCALL_ERRORTEXT; \
SYSCALL_LASTERROR; \
SYSCALL_GETARG; \
SYSCALL_HOSTSYSTEM; \
SYSCALL_CONSOLEPRINT; \
SYSCALL_CONSOLEPRINTLINE; \
SYSCALL_CONSOLEPRINTERROR; \
SYSCALL_CONSOLEPRINTERRORLINE; \
SYSCALL_GETFILENAME; \
SYSCALL_GETFILENAMENOEXT; \
SYSCALL_GETFILEEXTENSION; \
SYSCALL_GETDIRNAME; \
SYSCALL_FILEEXISTS; \
SYSCALL_DIREXISTS; \
SYSCALL_GETHANDLER; \
SYSCALL_FREEHANDLER; \
SYSCALL_GETFILESIZE; \
SYSCALL_OPENFORREAD; \
SYSCALL_OPENFORWRITE; \
SYSCALL_OPENFORAPPEND; \
SYSCALL_READ; \
SYSCALL_WRITE; \
SYSCALL_READALL; \
SYSCALL_WRITEALL; \
SYSCALL_READSTR; \
SYSCALL_WRITESTR; \
SYSCALL_READSTRALL; \
SYSCALL_WRITESTRALL; \
SYSCALL_CLOSEFILE; \
SYSCALL_HND2FILE; \
SYSCALL_FILE2HND; \
SYSCALL_ABSCHR; \
SYSCALL_ABSSHR; \
SYSCALL_ABSINT; \
SYSCALL_ABSLON; \
SYSCALL_ABSFLO; \
SYSCALL_MINCHR; \
SYSCALL_MINSHR; \
SYSCALL_MININT; \
SYSCALL_MINLON; \
SYSCALL_MINFLO; \
SYSCALL_MAXCHR; \
SYSCALL_MAXSHR; \
SYSCALL_MAXINT; \
SYSCALL_MAXLON; \
SYSCALL_MAXFLO; \
SYSCALL_EXP; \
SYSCALL_LN; \
SYSCALL_LOG; \
SYSCALL_LOGN; \
SYSCALL_POW; \
SYSCALL_SQRT; \
SYSCALL_CBRT; \
SYSCALL_SIN; \
SYSCALL_COS; \
SYSCALL_TAN; \
SYSCALL_ASIN; \
SYSCALL_ACOS; \
SYSCALL_ATAN; \
SYSCALL_SINH; \
SYSCALL_COSH; \
SYSCALL_TANH; \
SYSCALL_ASINH; \
SYSCALL_ACOSH; \
SYSCALL_ATANH; \
SYSCALL_CEIL; \
SYSCALL_FLOOR; \
SYSCALL_ROUND; \
SYSCALL_SEED; \
SYSCALL_RAND; \
SYSCALL_DATEVALID; \
SYSCALL_DATEVALUE; \
SYSCALL_BEGOFMONTH; \
SYSCALL_ENDOFMONTH; \
SYSCALL_DATEPART; \
SYSCALL_DATEADD; \
SYSCALL_TIMEVALID; \
SYSCALL_TIMEVALUE; \
SYSCALL_TIMEPART; \
SYSCALL_TIMEADD; \
SYSCALL_NANOSECADD; \
SYSCALL_GETDATE; \
SYSCALL_GETTIME; \
SYSCALL_DATEDIFF; \
SYSCALL_TIMEDIFF; \

//Argument to string functions
String Runtime::_ToStringCpuBol(CpuBol Arg){ return (Arg==0?"false":(Arg==1?"true":NZHEXFORMAT(Arg))); }
String Runtime::_ToStringCpuChr(CpuChr Arg){ return NZHEXFORMAT(Arg); }
String Runtime::_ToStringCpuShr(CpuShr Arg){ return ToString(Arg); }
String Runtime::_ToStringCpuInt(CpuInt Arg){ return ToString(Arg); }
String Runtime::_ToStringCpuLon(CpuLon Arg){ return ToString(Arg); }
String Runtime::_ToStringCpuFlo(CpuFlo Arg){ return ToString(Arg); }
String Runtime::_ToStringCpuAgx(CpuAgx Arg){ return "{"+NZHEXFORMAT(Arg)+"} "+_ArC.FixPrint(Arg); }
String Runtime::_ToStringCpuAdr(CpuAdr Arg){ return NZHEXFORMAT(Arg); }
String Runtime::_ToStringCpuWrd(CpuWrd Arg){ return ToString(Arg); }
String Runtime::_ToStringCpuRef(CpuRef Arg){ return "{id="+NZHEXFORMAT(Arg.Id)+", offset="+NZHEXFORMAT(Arg.Offset)+"}"; }
String Runtime::_ToStringCpuDat(CpuDat Arg){ return NZHEXFORMAT(Arg); }
String Runtime::_ToStringCpuMbl(CpuMbl Arg){
  int ArrIndex;
  String Result=NZHEXFORMAT(Arg)+":";
  if(Arg==0){
    Result+="null";
  }
  else if(!_Aux.IsValid(Arg)){
    Result+=_Aux.IsValidText(Arg);
  }
  else if((ArrIndex=_Aux.GetArrIndex(Arg))!=-1){
    Result+="{"+ToString(_Aux.ScopeId(Arg))+"} "+_ArC.DynPrint(Arg);
  }
  else{
    Result+="{"+ToString(_Aux.ScopeId(Arg))+"} string["+ToString(_Aux.GetLen(Arg))+"] = \""+
    NonAsciiEscape(_Aux.GetLen(Arg)>MAX_STRING_DEBUG_PRINT?String(_Aux.CharPtr(Arg),MAX_STRING_DEBUG_PRINT)+"...":String(_Aux.CharPtr(Arg)))+"\"";
  }
  return Result;
}


//Register pointer macros
#define BOL1 (reinterpret_cast<CpuBol*>(REG1))
#define CHR1 (reinterpret_cast<CpuChr*>(REG1))
#define SHR1 (reinterpret_cast<CpuShr*>(REG1))
#define INT1 (reinterpret_cast<CpuInt*>(REG1))
#define LON1 (reinterpret_cast<CpuLon*>(REG1))
#define FLO1 (reinterpret_cast<CpuFlo*>(REG1))
#define MBL1 (reinterpret_cast<CpuMbl*>(REG1))
#define AGX1 (reinterpret_cast<CpuAgx*>(REG1))
#define ADR1 (reinterpret_cast<CpuAdr*>(REG1))
#define WRD1 (reinterpret_cast<CpuWrd*>(REG1))
#define REF1 (reinterpret_cast<CpuRef*>(REG1))
#define DAT1 (reinterpret_cast<CpuDat*>(REG1))
#define BOL2 (reinterpret_cast<CpuBol*>(REG2))
#define CHR2 (reinterpret_cast<CpuChr*>(REG2))
#define SHR2 (reinterpret_cast<CpuShr*>(REG2))
#define INT2 (reinterpret_cast<CpuInt*>(REG2))
#define LON2 (reinterpret_cast<CpuLon*>(REG2))
#define FLO2 (reinterpret_cast<CpuFlo*>(REG2))
#define MBL2 (reinterpret_cast<CpuMbl*>(REG2))
#define AGX2 (reinterpret_cast<CpuAgx*>(REG2))
#define ADR2 (reinterpret_cast<CpuAdr*>(REG2))
#define WRD2 (reinterpret_cast<CpuWrd*>(REG2))
#define REF2 (reinterpret_cast<CpuRef*>(REG2))
#define DAT2 (reinterpret_cast<CpuDat*>(REG2))
#define BOL3 (reinterpret_cast<CpuBol*>(REG3))
#define CHR3 (reinterpret_cast<CpuChr*>(REG3))
#define SHR3 (reinterpret_cast<CpuShr*>(REG3))
#define INT3 (reinterpret_cast<CpuInt*>(REG3))
#define LON3 (reinterpret_cast<CpuLon*>(REG3))
#define FLO3 (reinterpret_cast<CpuFlo*>(REG3))
#define MBL3 (reinterpret_cast<CpuMbl*>(REG3))
#define AGX3 (reinterpret_cast<CpuAgx*>(REG3))
#define ADR3 (reinterpret_cast<CpuAdr*>(REG3))
#define WRD3 (reinterpret_cast<CpuWrd*>(REG3))
#define REF3 (reinterpret_cast<CpuRef*>(REG3))
#define DAT3 (reinterpret_cast<CpuDat*>(REG3))
#define BOL4 (reinterpret_cast<CpuBol*>(REG4))
#define CHR4 (reinterpret_cast<CpuChr*>(REG4))
#define SHR4 (reinterpret_cast<CpuShr*>(REG4))
#define INT4 (reinterpret_cast<CpuInt*>(REG4))
#define LON4 (reinterpret_cast<CpuLon*>(REG4))
#define FLO4 (reinterpret_cast<CpuFlo*>(REG4))
#define MBL4 (reinterpret_cast<CpuMbl*>(REG4))
#define AGX4 (reinterpret_cast<CpuAgx*>(REG4))
#define ADR4 (reinterpret_cast<CpuAdr*>(REG4))
#define WRD4 (reinterpret_cast<CpuWrd*>(REG4))
#define REF4 (reinterpret_cast<CpuRef*>(REG4))
#define DAT4 (reinterpret_cast<CpuDat*>(REG4))
#define BOL5 (reinterpret_cast<CpuBol*>(REG5))
#define CHR5 (reinterpret_cast<CpuChr*>(REG5))
#define SHR5 (reinterpret_cast<CpuShr*>(REG5))
#define INT5 (reinterpret_cast<CpuInt*>(REG5))
#define LON5 (reinterpret_cast<CpuLon*>(REG5))
#define FLO5 (reinterpret_cast<CpuFlo*>(REG5))
#define MBL5 (reinterpret_cast<CpuMbl*>(REG5))
#define AGX5 (reinterpret_cast<CpuAgx*>(REG5))
#define ADR5 (reinterpret_cast<CpuAdr*>(REG5))
#define WRD5 (reinterpret_cast<CpuWrd*>(REG5))
#define REF5 (reinterpret_cast<CpuRef*>(REG5))
#define DAT5 (reinterpret_cast<CpuDat*>(REG5))
#define BOL6 (reinterpret_cast<CpuBol*>(REG6))
#define CHR6 (reinterpret_cast<CpuChr*>(REG6))
#define SHR6 (reinterpret_cast<CpuShr*>(REG6))
#define INT6 (reinterpret_cast<CpuInt*>(REG6))
#define LON6 (reinterpret_cast<CpuLon*>(REG6))
#define FLO6 (reinterpret_cast<CpuFlo*>(REG6))
#define MBL6 (reinterpret_cast<CpuMbl*>(REG6))
#define AGX6 (reinterpret_cast<CpuAgx*>(REG6))
#define ADR6 (reinterpret_cast<CpuAdr*>(REG6))
#define WRD6 (reinterpret_cast<CpuWrd*>(REG6))
#define REF6 (reinterpret_cast<CpuRef*>(REG6))
#define DAT6 (reinterpret_cast<CpuDat*>(REG6))

//Exception exit on instruction handlers
#define EXCP_EXIT goto RunProgInstrExceptionHandler;


//Instruction decoders
#define INSTDECODE_0                
#define INSTDECODE_1_A_V            DECODE_LIT(1,ADR,CpuAdr,AOFF_I); 
#define INSTDECODE_1_B_A            DECODE_ADR(1,BOL,CpuBol,AOFF_I); 
#define INSTDECODE_1_C_A            DECODE_ADR(1,CHR,CpuChr,AOFF_I); 
#define INSTDECODE_1_D_A            DECODE_ADR(1,DAT,CpuDat,AOFF_I); 
#define INSTDECODE_1_F_A            DECODE_ADR(1,FLO,CpuFlo,AOFF_I); 
#define INSTDECODE_1_I_A            DECODE_ADR(1,INT,CpuInt,AOFF_I); 
#define INSTDECODE_1_I_V            DECODE_LIT(1,INT,CpuInt,AOFF_I); 
#define INSTDECODE_1_L_A            DECODE_ADR(1,LON,CpuLon,AOFF_I); 
#define INSTDECODE_1_L_V            DECODE_LIT(1,LON,CpuLon,AOFF_I); 
#define INSTDECODE_1_M_A            DECODE_ADR(1,MBL,CpuMbl,AOFF_I); 
#define INSTDECODE_1_R_A            DECODE_ADR(1,REF,CpuRef,AOFF_I); 
#define INSTDECODE_1_W_A            DECODE_ADR(1,SHR,CpuShr,AOFF_I); 
#define INSTDECODE_1_Z_A            DECODE_ADR(1,WRD,CpuWrd,AOFF_I); 
#define INSTDECODE_1_Z_V            DECODE_LIT(1,WRD,CpuWrd,AOFF_I); 
#define INSTDECODE_2_BA_AV          DECODE_ADR(1,BOL,CpuBol,AOFF_I); DECODE_LIT(2,ADR,CpuAdr,AOFF_IA); 
#define INSTDECODE_2_BB_AA          DECODE_ADR(1,BOL,CpuBol,AOFF_I); DECODE_ADR(2,BOL,CpuBol,AOFF_IA); 
#define INSTDECODE_2_BB_AV          DECODE_ADR(1,BOL,CpuBol,AOFF_I); DECODE_LIT(2,BOL,CpuBol,AOFF_IA); 
#define INSTDECODE_2_BC_AA          DECODE_ADR(1,BOL,CpuBol,AOFF_I); DECODE_ADR(2,CHR,CpuChr,AOFF_IA); 
#define INSTDECODE_2_BF_AA          DECODE_ADR(1,BOL,CpuBol,AOFF_I); DECODE_ADR(2,FLO,CpuFlo,AOFF_IA); 
#define INSTDECODE_2_BI_AA          DECODE_ADR(1,BOL,CpuBol,AOFF_I); DECODE_ADR(2,INT,CpuInt,AOFF_IA); 
#define INSTDECODE_2_BL_AA          DECODE_ADR(1,BOL,CpuBol,AOFF_I); DECODE_ADR(2,LON,CpuLon,AOFF_IA); 
#define INSTDECODE_2_BM_AA          DECODE_ADR(1,BOL,CpuBol,AOFF_I); DECODE_ADR(2,MBL,CpuMbl,AOFF_IA); 
#define INSTDECODE_2_BW_AA          DECODE_ADR(1,BOL,CpuBol,AOFF_I); DECODE_ADR(2,SHR,CpuShr,AOFF_IA); 
#define INSTDECODE_2_CB_AA          DECODE_ADR(1,CHR,CpuChr,AOFF_I); DECODE_ADR(2,BOL,CpuBol,AOFF_IA); 
#define INSTDECODE_2_CC_AA          DECODE_ADR(1,CHR,CpuChr,AOFF_I); DECODE_ADR(2,CHR,CpuChr,AOFF_IA); 
#define INSTDECODE_2_CC_AV          DECODE_ADR(1,CHR,CpuChr,AOFF_I); DECODE_LIT(2,CHR,CpuChr,AOFF_IA); 
#define INSTDECODE_2_CF_AA          DECODE_ADR(1,CHR,CpuChr,AOFF_I); DECODE_ADR(2,FLO,CpuFlo,AOFF_IA); 
#define INSTDECODE_2_CI_AA          DECODE_ADR(1,CHR,CpuChr,AOFF_I); DECODE_ADR(2,INT,CpuInt,AOFF_IA); 
#define INSTDECODE_2_CL_AA          DECODE_ADR(1,CHR,CpuChr,AOFF_I); DECODE_ADR(2,LON,CpuLon,AOFF_IA); 
#define INSTDECODE_2_CM_AA          DECODE_ADR(1,CHR,CpuChr,AOFF_I); DECODE_ADR(2,MBL,CpuMbl,AOFF_IA); 
#define INSTDECODE_2_CW_AA          DECODE_ADR(1,CHR,CpuChr,AOFF_I); DECODE_ADR(2,SHR,CpuShr,AOFF_IA); 
#define INSTDECODE_2_DB_AV          DECODE_ADR(1,DAT,CpuDat,AOFF_I); DECODE_LIT(2,BOL,CpuBol,AOFF_IA); 
#define INSTDECODE_2_DD_AA          DECODE_ADR(1,DAT,CpuDat,AOFF_I); DECODE_ADR(2,DAT,CpuDat,AOFF_IA); 
#define INSTDECODE_2_DZ_AV          DECODE_ADR(1,DAT,CpuDat,AOFF_I); DECODE_LIT(2,WRD,CpuWrd,AOFF_IA); 
#define INSTDECODE_2_FB_AA          DECODE_ADR(1,FLO,CpuFlo,AOFF_I); DECODE_ADR(2,BOL,CpuBol,AOFF_IA); 
#define INSTDECODE_2_FC_AA          DECODE_ADR(1,FLO,CpuFlo,AOFF_I); DECODE_ADR(2,CHR,CpuChr,AOFF_IA); 
#define INSTDECODE_2_FF_AA          DECODE_ADR(1,FLO,CpuFlo,AOFF_I); DECODE_ADR(2,FLO,CpuFlo,AOFF_IA); 
#define INSTDECODE_2_FF_AV          DECODE_ADR(1,FLO,CpuFlo,AOFF_I); DECODE_LIT(2,FLO,CpuFlo,AOFF_IA); 
#define INSTDECODE_2_FI_AA          DECODE_ADR(1,FLO,CpuFlo,AOFF_I); DECODE_ADR(2,INT,CpuInt,AOFF_IA); 
#define INSTDECODE_2_FL_AA          DECODE_ADR(1,FLO,CpuFlo,AOFF_I); DECODE_ADR(2,LON,CpuLon,AOFF_IA); 
#define INSTDECODE_2_FM_AA          DECODE_ADR(1,FLO,CpuFlo,AOFF_I); DECODE_ADR(2,MBL,CpuMbl,AOFF_IA); 
#define INSTDECODE_2_FW_AA          DECODE_ADR(1,FLO,CpuFlo,AOFF_I); DECODE_ADR(2,SHR,CpuShr,AOFF_IA); 
#define INSTDECODE_2_GA_VV          DECODE_LIT(1,AGX,CpuAgx,AOFF_I); DECODE_LIT(2,ADR,CpuAdr,AOFF_IG); 
#define INSTDECODE_2_IB_AA          DECODE_ADR(1,INT,CpuInt,AOFF_I); DECODE_ADR(2,BOL,CpuBol,AOFF_IA); 
#define INSTDECODE_2_IC_AA          DECODE_ADR(1,INT,CpuInt,AOFF_I); DECODE_ADR(2,CHR,CpuChr,AOFF_IA); 
#define INSTDECODE_2_IF_AA          DECODE_ADR(1,INT,CpuInt,AOFF_I); DECODE_ADR(2,FLO,CpuFlo,AOFF_IA); 
#define INSTDECODE_2_II_AA          DECODE_ADR(1,INT,CpuInt,AOFF_I); DECODE_ADR(2,INT,CpuInt,AOFF_IA); 
#define INSTDECODE_2_II_AV          DECODE_ADR(1,INT,CpuInt,AOFF_I); DECODE_LIT(2,INT,CpuInt,AOFF_IA); 
#define INSTDECODE_2_IL_AA          DECODE_ADR(1,INT,CpuInt,AOFF_I); DECODE_ADR(2,LON,CpuLon,AOFF_IA); 
#define INSTDECODE_2_IM_AA          DECODE_ADR(1,INT,CpuInt,AOFF_I); DECODE_ADR(2,MBL,CpuMbl,AOFF_IA); 
#define INSTDECODE_2_IW_AA          DECODE_ADR(1,INT,CpuInt,AOFF_I); DECODE_ADR(2,SHR,CpuShr,AOFF_IA); 
#define INSTDECODE_2_LB_AA          DECODE_ADR(1,LON,CpuLon,AOFF_I); DECODE_ADR(2,BOL,CpuBol,AOFF_IA); 
#define INSTDECODE_2_LC_AA          DECODE_ADR(1,LON,CpuLon,AOFF_I); DECODE_ADR(2,CHR,CpuChr,AOFF_IA); 
#define INSTDECODE_2_LF_AA          DECODE_ADR(1,LON,CpuLon,AOFF_I); DECODE_ADR(2,FLO,CpuFlo,AOFF_IA); 
#define INSTDECODE_2_LI_AA          DECODE_ADR(1,LON,CpuLon,AOFF_I); DECODE_ADR(2,INT,CpuInt,AOFF_IA); 
#define INSTDECODE_2_LL_AA          DECODE_ADR(1,LON,CpuLon,AOFF_I); DECODE_ADR(2,LON,CpuLon,AOFF_IA); 
#define INSTDECODE_2_LL_AV          DECODE_ADR(1,LON,CpuLon,AOFF_I); DECODE_LIT(2,LON,CpuLon,AOFF_IA); 
#define INSTDECODE_2_LM_AA          DECODE_ADR(1,LON,CpuLon,AOFF_I); DECODE_ADR(2,MBL,CpuMbl,AOFF_IA); 
#define INSTDECODE_2_LW_AA          DECODE_ADR(1,LON,CpuLon,AOFF_I); DECODE_ADR(2,SHR,CpuShr,AOFF_IA); 
#define INSTDECODE_2_MA_AV          DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_LIT(2,ADR,CpuAdr,AOFF_IA); 
#define INSTDECODE_2_MB_AA          DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_ADR(2,BOL,CpuBol,AOFF_IA); 
#define INSTDECODE_2_MC_AA          DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_ADR(2,CHR,CpuChr,AOFF_IA); 
#define INSTDECODE_2_MF_AA          DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_ADR(2,FLO,CpuFlo,AOFF_IA); 
#define INSTDECODE_2_MI_AA          DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_ADR(2,INT,CpuInt,AOFF_IA); 
#define INSTDECODE_2_ML_AA          DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_ADR(2,LON,CpuLon,AOFF_IA); 
#define INSTDECODE_2_MM_AA          DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_ADR(2,MBL,CpuMbl,AOFF_IA); 
#define INSTDECODE_2_MW_AA          DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_ADR(2,SHR,CpuShr,AOFF_IA); 
#define INSTDECODE_2_MZ_AA          DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_ADR(2,WRD,CpuWrd,AOFF_IA); 
#define INSTDECODE_2_MZ_AV          DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_LIT(2,WRD,CpuWrd,AOFF_IA); 
#define INSTDECODE_2_RB_AV          DECODE_ADR(1,REF,CpuRef,AOFF_I); DECODE_LIT(2,BOL,CpuBol,AOFF_IA); 
#define INSTDECODE_2_RD_AA          DECODE_ADR(1,REF,CpuRef,AOFF_I); DECODE_ADR(2,DAT,CpuDat,AOFF_IA); 
#define INSTDECODE_2_RM_AA          DECODE_ADR(1,REF,CpuRef,AOFF_I); DECODE_ADR(2,MBL,CpuMbl,AOFF_IA); 
#define INSTDECODE_2_RR_AA          DECODE_ADR(1,REF,CpuRef,AOFF_I); DECODE_ADR(2,REF,CpuRef,AOFF_IA); 
#define INSTDECODE_2_RZ_AV          DECODE_ADR(1,REF,CpuRef,AOFF_I); DECODE_LIT(2,WRD,CpuWrd,AOFF_IA); 
#define INSTDECODE_2_WB_AA          DECODE_ADR(1,SHR,CpuShr,AOFF_I); DECODE_ADR(2,BOL,CpuBol,AOFF_IA); 
#define INSTDECODE_2_WC_AA          DECODE_ADR(1,SHR,CpuShr,AOFF_I); DECODE_ADR(2,CHR,CpuChr,AOFF_IA); 
#define INSTDECODE_2_WF_AA          DECODE_ADR(1,SHR,CpuShr,AOFF_I); DECODE_ADR(2,FLO,CpuFlo,AOFF_IA); 
#define INSTDECODE_2_WI_AA          DECODE_ADR(1,SHR,CpuShr,AOFF_I); DECODE_ADR(2,INT,CpuInt,AOFF_IA); 
#define INSTDECODE_2_WL_AA          DECODE_ADR(1,SHR,CpuShr,AOFF_I); DECODE_ADR(2,LON,CpuLon,AOFF_IA); 
#define INSTDECODE_2_WM_AA          DECODE_ADR(1,SHR,CpuShr,AOFF_I); DECODE_ADR(2,MBL,CpuMbl,AOFF_IA); 
#define INSTDECODE_2_WW_AA          DECODE_ADR(1,SHR,CpuShr,AOFF_I); DECODE_ADR(2,SHR,CpuShr,AOFF_IA); 
#define INSTDECODE_2_WW_AV          DECODE_ADR(1,SHR,CpuShr,AOFF_I); DECODE_LIT(2,SHR,CpuShr,AOFF_IA); 
#define INSTDECODE_2_WW_VV          DECODE_LIT(1,SHR,CpuShr,AOFF_I); DECODE_LIT(2,SHR,CpuShr,AOFF_IW); 
#define INSTDECODE_2_ZG_AV          DECODE_ADR(1,WRD,CpuWrd,AOFF_I); DECODE_LIT(2,AGX,CpuAgx,AOFF_IA); 
#define INSTDECODE_2_ZM_AA          DECODE_ADR(1,WRD,CpuWrd,AOFF_I); DECODE_ADR(2,MBL,CpuMbl,AOFF_IA); 
#define INSTDECODE_3_BBB_AAA        DECODE_ADR(1,BOL,CpuBol,AOFF_I); DECODE_ADR(2,BOL,CpuBol,AOFF_IA); DECODE_ADR(3,BOL,CpuBol,AOFF_IAA); 
#define INSTDECODE_3_BCC_AAA        DECODE_ADR(1,BOL,CpuBol,AOFF_I); DECODE_ADR(2,CHR,CpuChr,AOFF_IA); DECODE_ADR(3,CHR,CpuChr,AOFF_IAA); 
#define INSTDECODE_3_BFF_AAA        DECODE_ADR(1,BOL,CpuBol,AOFF_I); DECODE_ADR(2,FLO,CpuFlo,AOFF_IA); DECODE_ADR(3,FLO,CpuFlo,AOFF_IAA); 
#define INSTDECODE_3_BII_AAA        DECODE_ADR(1,BOL,CpuBol,AOFF_I); DECODE_ADR(2,INT,CpuInt,AOFF_IA); DECODE_ADR(3,INT,CpuInt,AOFF_IAA); 
#define INSTDECODE_3_BLL_AAA        DECODE_ADR(1,BOL,CpuBol,AOFF_I); DECODE_ADR(2,LON,CpuLon,AOFF_IA); DECODE_ADR(3,LON,CpuLon,AOFF_IAA); 
#define INSTDECODE_3_BMM_AAA        DECODE_ADR(1,BOL,CpuBol,AOFF_I); DECODE_ADR(2,MBL,CpuMbl,AOFF_IA); DECODE_ADR(3,MBL,CpuMbl,AOFF_IAA); 
#define INSTDECODE_3_BWW_AAA        DECODE_ADR(1,BOL,CpuBol,AOFF_I); DECODE_ADR(2,SHR,CpuShr,AOFF_IA); DECODE_ADR(3,SHR,CpuShr,AOFF_IAA); 
#define INSTDECODE_3_CCC_AAA        DECODE_ADR(1,CHR,CpuChr,AOFF_I); DECODE_ADR(2,CHR,CpuChr,AOFF_IA); DECODE_ADR(3,CHR,CpuChr,AOFF_IAA); 
#define INSTDECODE_3_DBG_AVV        DECODE_ADR(1,DAT,CpuDat,AOFF_I); DECODE_LIT(2,BOL,CpuBol,AOFF_IA); DECODE_LIT(3,AGX,CpuAgx,AOFF_IAB); 
#define INSTDECODE_3_DDZ_AAV        DECODE_ADR(1,DAT,CpuDat,AOFF_I); DECODE_ADR(2,DAT,CpuDat,AOFF_IA); DECODE_LIT(3,WRD,CpuWrd,AOFF_IAA); 
#define INSTDECODE_3_DGM_AVA        DECODE_ADR(1,DAT,CpuDat,AOFF_I); DECODE_LIT(2,AGX,CpuAgx,AOFF_IA); DECODE_ADR(3,MBL,CpuMbl,AOFF_IAG); 
#define INSTDECODE_3_DMZ_AAV        DECODE_ADR(1,DAT,CpuDat,AOFF_I); DECODE_ADR(2,MBL,CpuMbl,AOFF_IA); DECODE_LIT(3,WRD,CpuWrd,AOFF_IAA); 
#define INSTDECODE_3_FFF_AAA        DECODE_ADR(1,FLO,CpuFlo,AOFF_I); DECODE_ADR(2,FLO,CpuFlo,AOFF_IA); DECODE_ADR(3,FLO,CpuFlo,AOFF_IAA); 
#define INSTDECODE_3_GAA_VVV        DECODE_LIT(1,AGX,CpuAgx,AOFF_I); DECODE_LIT(2,ADR,CpuAdr,AOFF_IG); DECODE_LIT(3,ADR,CpuAdr,AOFF_IGA); 
#define INSTDECODE_3_GCZ_VAA        DECODE_LIT(1,AGX,CpuAgx,AOFF_I); DECODE_ADR(2,CHR,CpuChr,AOFF_IG); DECODE_ADR(3,WRD,CpuWrd,AOFF_IGA); 
#define INSTDECODE_3_GCZ_VVA        DECODE_LIT(1,AGX,CpuAgx,AOFF_I); DECODE_LIT(2,CHR,CpuChr,AOFF_IG); DECODE_ADR(3,WRD,CpuWrd,AOFF_IGC); 
#define INSTDECODE_3_GCZ_VVV        DECODE_LIT(1,AGX,CpuAgx,AOFF_I); DECODE_LIT(2,CHR,CpuChr,AOFF_IG); DECODE_LIT(3,WRD,CpuWrd,AOFF_IGC); 
#define INSTDECODE_3_III_AAA        DECODE_ADR(1,INT,CpuInt,AOFF_I); DECODE_ADR(2,INT,CpuInt,AOFF_IA); DECODE_ADR(3,INT,CpuInt,AOFF_IAA); 
#define INSTDECODE_3_LLL_AAA        DECODE_ADR(1,LON,CpuLon,AOFF_I); DECODE_ADR(2,LON,CpuLon,AOFF_IA); DECODE_ADR(3,LON,CpuLon,AOFF_IAA); 
#define INSTDECODE_3_MAA_AVV        DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_LIT(2,ADR,CpuAdr,AOFF_IA); DECODE_LIT(3,ADR,CpuAdr,AOFF_IAA); 
#define INSTDECODE_3_MBM_AAA        DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_ADR(2,BOL,CpuBol,AOFF_IA); DECODE_ADR(3,MBL,CpuMbl,AOFF_IAA); 
#define INSTDECODE_3_MCM_AAA        DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_ADR(2,CHR,CpuChr,AOFF_IA); DECODE_ADR(3,MBL,CpuMbl,AOFF_IAA); 
#define INSTDECODE_3_MCZ_AAA        DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_ADR(2,CHR,CpuChr,AOFF_IA); DECODE_ADR(3,WRD,CpuWrd,AOFF_IAA); 
#define INSTDECODE_3_MCZ_AVA        DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_LIT(2,CHR,CpuChr,AOFF_IA); DECODE_ADR(3,WRD,CpuWrd,AOFF_IAC); 
#define INSTDECODE_3_MCZ_AVV        DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_LIT(2,CHR,CpuChr,AOFF_IA); DECODE_LIT(3,WRD,CpuWrd,AOFF_IAC); 
#define INSTDECODE_3_MDG_AAV        DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_ADR(2,DAT,CpuDat,AOFF_IA); DECODE_LIT(3,AGX,CpuAgx,AOFF_IAA); 
#define INSTDECODE_3_MDZ_AAV        DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_ADR(2,DAT,CpuDat,AOFF_IA); DECODE_LIT(3,WRD,CpuWrd,AOFF_IAA); 
#define INSTDECODE_3_MFM_AAA        DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_ADR(2,FLO,CpuFlo,AOFF_IA); DECODE_ADR(3,MBL,CpuMbl,AOFF_IAA); 
#define INSTDECODE_3_MIM_AAA        DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_ADR(2,INT,CpuInt,AOFF_IA); DECODE_ADR(3,MBL,CpuMbl,AOFF_IAA); 
#define INSTDECODE_3_MLM_AAA        DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_ADR(2,LON,CpuLon,AOFF_IA); DECODE_ADR(3,MBL,CpuMbl,AOFF_IAA); 
#define INSTDECODE_3_MMI_AAA        DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_ADR(2,MBL,CpuMbl,AOFF_IA); DECODE_ADR(3,INT,CpuInt,AOFF_IAA); 
#define INSTDECODE_3_MMM_AAA        DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_ADR(2,MBL,CpuMbl,AOFF_IA); DECODE_ADR(3,MBL,CpuMbl,AOFF_IAA); 
#define INSTDECODE_3_MMZ_AAA        DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_ADR(2,MBL,CpuMbl,AOFF_IA); DECODE_ADR(3,WRD,CpuWrd,AOFF_IAA); 
#define INSTDECODE_3_MWM_AAA        DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_ADR(2,SHR,CpuShr,AOFF_IA); DECODE_ADR(3,MBL,CpuMbl,AOFF_IAA); 
#define INSTDECODE_3_RBG_AVV        DECODE_ADR(1,REF,CpuRef,AOFF_I); DECODE_LIT(2,BOL,CpuBol,AOFF_IA); DECODE_LIT(3,AGX,CpuAgx,AOFF_IAB); 
#define INSTDECODE_3_RDG_AAV        DECODE_ADR(1,REF,CpuRef,AOFF_I); DECODE_ADR(2,DAT,CpuDat,AOFF_IA); DECODE_LIT(3,AGX,CpuAgx,AOFF_IAA); 
#define INSTDECODE_3_RDZ_AAV        DECODE_ADR(1,REF,CpuRef,AOFF_I); DECODE_ADR(2,DAT,CpuDat,AOFF_IA); DECODE_LIT(3,WRD,CpuWrd,AOFF_IAA); 
#define INSTDECODE_3_RMZ_AAA        DECODE_ADR(1,REF,CpuRef,AOFF_I); DECODE_ADR(2,MBL,CpuMbl,AOFF_IA); DECODE_ADR(3,WRD,CpuWrd,AOFF_IAA); 
#define INSTDECODE_3_RMZ_AAV        DECODE_ADR(1,REF,CpuRef,AOFF_I); DECODE_ADR(2,MBL,CpuMbl,AOFF_IA); DECODE_LIT(3,WRD,CpuWrd,AOFF_IAA); 
#define INSTDECODE_3_WWW_AAA        DECODE_ADR(1,SHR,CpuShr,AOFF_I); DECODE_ADR(2,SHR,CpuShr,AOFF_IA); DECODE_ADR(3,SHR,CpuShr,AOFF_IAA); 
#define INSTDECODE_3_ZCZ_VVV        DECODE_LIT(1,WRD,CpuWrd,AOFF_I); DECODE_LIT(2,CHR,CpuChr,AOFF_IZ); DECODE_LIT(3,WRD,CpuWrd,AOFF_IZC); 
#define INSTDECODE_4_DGDG_AVAV      DECODE_ADR(1,DAT,CpuDat,AOFF_I); DECODE_LIT(2,AGX,CpuAgx,AOFF_IA); DECODE_ADR(3,DAT,CpuDat,AOFF_IAG); DECODE_LIT(4,AGX,CpuAgx,AOFF_IAGA); 
#define INSTDECODE_4_MDGM_AAVA      DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_ADR(2,DAT,CpuDat,AOFF_IA); DECODE_LIT(3,AGX,CpuAgx,AOFF_IAA); DECODE_ADR(4,MBL,CpuMbl,AOFF_IAAG); 
#define INSTDECODE_4_MMIC_AAAA      DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_ADR(2,MBL,CpuMbl,AOFF_IA); DECODE_ADR(3,INT,CpuInt,AOFF_IAA); DECODE_ADR(4,CHR,CpuChr,AOFF_IAAA); 
#define INSTDECODE_4_MMMM_AAAA      DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_ADR(2,MBL,CpuMbl,AOFF_IA); DECODE_ADR(3,MBL,CpuMbl,AOFF_IAA); DECODE_ADR(4,MBL,CpuMbl,AOFF_IAAA); 
#define INSTDECODE_4_MMZZ_AAAA      DECODE_ADR(1,MBL,CpuMbl,AOFF_I); DECODE_ADR(2,MBL,CpuMbl,AOFF_IA); DECODE_ADR(3,WRD,CpuWrd,AOFF_IAA); DECODE_ADR(4,WRD,CpuWrd,AOFF_IAAA); 
#define INSTDECODE_4_RDGZ_AAVA      DECODE_ADR(1,REF,CpuRef,AOFF_I); DECODE_ADR(2,DAT,CpuDat,AOFF_IA); DECODE_LIT(3,AGX,CpuAgx,AOFF_IAA); DECODE_ADR(4,WRD,CpuWrd,AOFF_IAAG); 
#define INSTDECODE_4_RMZZ_AAAV      DECODE_ADR(1,REF,CpuRef,AOFF_I); DECODE_ADR(2,MBL,CpuMbl,AOFF_IA); DECODE_ADR(3,WRD,CpuWrd,AOFF_IAA); DECODE_LIT(4,WRD,CpuWrd,AOFF_IAAA); 
#define INSTDECODE_4_ZMMZ_AAAA      DECODE_ADR(1,WRD,CpuWrd,AOFF_I); DECODE_ADR(2,MBL,CpuMbl,AOFF_IA); DECODE_ADR(3,MBL,CpuMbl,AOFF_IAA); DECODE_ADR(4,WRD,CpuWrd,AOFF_IAAA); 

//Instruction ends
#define CONST_INSTEND_2_WW_VV   IP+=ISIZ_IWW;   CONST_INST_DISPATCH;
#define INSTEND_0               IP+=ISIZ_I;     PROG_INST_DISPATCH;
#define INSTEND_1_B_A           IP+=ISIZ_IA;    PROG_INST_DISPATCH;
#define INSTEND_1_C_A           IP+=ISIZ_IA;    PROG_INST_DISPATCH;
#define INSTEND_1_D_A           IP+=ISIZ_IA;    PROG_INST_DISPATCH;
#define INSTEND_1_F_A           IP+=ISIZ_IA;    PROG_INST_DISPATCH;
#define INSTEND_1_I_A           IP+=ISIZ_IA;    PROG_INST_DISPATCH;
#define INSTEND_1_I_V           IP+=ISIZ_II;    PROG_INST_DISPATCH;
#define INSTEND_1_L_A           IP+=ISIZ_IA;    PROG_INST_DISPATCH;
#define INSTEND_1_L_V           IP+=ISIZ_IL;    PROG_INST_DISPATCH;
#define INSTEND_1_M_A           IP+=ISIZ_IA;    PROG_INST_DISPATCH;
#define INSTEND_1_R_A           IP+=ISIZ_IA;    PROG_INST_DISPATCH;
#define INSTEND_1_W_A           IP+=ISIZ_IA;    PROG_INST_DISPATCH;
#define INSTEND_1_Z_A           IP+=ISIZ_IA;    PROG_INST_DISPATCH;
#define INSTEND_1_Z_V           IP+=ISIZ_IZ;    PROG_INST_DISPATCH;
#define INSTEND_2_BA_AV         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_BB_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_BB_AV         IP+=ISIZ_IAB;   PROG_INST_DISPATCH;
#define INSTEND_2_BC_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_BF_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_BI_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_BL_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_BM_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_BW_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_CB_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_CC_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_CC_AV         IP+=ISIZ_IAC;   PROG_INST_DISPATCH;
#define INSTEND_2_CF_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_CI_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_CL_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_CM_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_CW_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_DB_AV         IP+=ISIZ_IAB;   PROG_INST_DISPATCH;
#define INSTEND_2_DD_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_DZ_AV         IP+=ISIZ_IAZ;   PROG_INST_DISPATCH;
#define INSTEND_2_FB_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_FC_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_FF_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_FF_AV         IP+=ISIZ_IAF;   PROG_INST_DISPATCH;
#define INSTEND_2_FI_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_FL_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_FM_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_FW_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_IB_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_IC_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_IF_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_II_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_II_AV         IP+=ISIZ_IAI;   PROG_INST_DISPATCH;
#define INSTEND_2_IL_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_IM_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_IW_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_LB_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_LC_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_LF_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_LI_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_LL_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_LL_AV         IP+=ISIZ_IAL;   PROG_INST_DISPATCH;
#define INSTEND_2_LM_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_LW_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_MB_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_MC_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_MF_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_MI_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_ML_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_MM_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_MW_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_MZ_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_MZ_AV         IP+=ISIZ_IAZ;   PROG_INST_DISPATCH;
#define INSTEND_2_RB_AV         IP+=ISIZ_IAB;   PROG_INST_DISPATCH;
#define INSTEND_2_RD_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_RM_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_RR_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_RZ_AV         IP+=ISIZ_IAZ;   PROG_INST_DISPATCH;
#define INSTEND_2_WB_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_WC_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_WF_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_WI_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_WL_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_WM_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_WW_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_2_WW_AV         IP+=ISIZ_IAW;   PROG_INST_DISPATCH;
#define INSTEND_2_ZG_AV         IP+=ISIZ_IAG;   PROG_INST_DISPATCH;
#define INSTEND_2_ZM_AA         IP+=ISIZ_IAA;   PROG_INST_DISPATCH;
#define INSTEND_3_BBB_AAA       IP+=ISIZ_IAAA;  PROG_INST_DISPATCH;
#define INSTEND_3_BCC_AAA       IP+=ISIZ_IAAA;  PROG_INST_DISPATCH;
#define INSTEND_3_BFF_AAA       IP+=ISIZ_IAAA;  PROG_INST_DISPATCH;
#define INSTEND_3_BII_AAA       IP+=ISIZ_IAAA;  PROG_INST_DISPATCH;
#define INSTEND_3_BLL_AAA       IP+=ISIZ_IAAA;  PROG_INST_DISPATCH;
#define INSTEND_3_BMM_AAA       IP+=ISIZ_IAAA;  PROG_INST_DISPATCH;
#define INSTEND_3_BWW_AAA       IP+=ISIZ_IAAA;  PROG_INST_DISPATCH;
#define INSTEND_3_CCC_AAA       IP+=ISIZ_IAAA;  PROG_INST_DISPATCH;
#define INSTEND_3_DBG_AVV       IP+=ISIZ_IABG;  PROG_INST_DISPATCH;
#define INSTEND_3_DDZ_AAV       IP+=ISIZ_IAAZ;  PROG_INST_DISPATCH;
#define INSTEND_3_DGM_AVA       IP+=ISIZ_IAGA;  PROG_INST_DISPATCH;
#define INSTEND_3_DMZ_AAV       IP+=ISIZ_IAAZ;  PROG_INST_DISPATCH;
#define INSTEND_3_FFF_AAA       IP+=ISIZ_IAAA;  PROG_INST_DISPATCH;
#define INSTEND_3_GAA_VVV       IP+=ISIZ_IGAA;  PROG_INST_DISPATCH;
#define INSTEND_3_GCZ_VAA       IP+=ISIZ_IGAA;  PROG_INST_DISPATCH;
#define INSTEND_3_GCZ_VVA       IP+=ISIZ_IGCA;  PROG_INST_DISPATCH;
#define INSTEND_3_GCZ_VVV       IP+=ISIZ_IGCZ;  PROG_INST_DISPATCH;
#define INSTEND_3_III_AAA       IP+=ISIZ_IAAA;  PROG_INST_DISPATCH;
#define INSTEND_3_LLL_AAA       IP+=ISIZ_IAAA;  PROG_INST_DISPATCH;
#define INSTEND_3_MAA_AVV       IP+=ISIZ_IAAA;  PROG_INST_DISPATCH;
#define INSTEND_3_MBM_AAA       IP+=ISIZ_IAAA;  PROG_INST_DISPATCH;
#define INSTEND_3_MCM_AAA       IP+=ISIZ_IAAA;  PROG_INST_DISPATCH;
#define INSTEND_3_MCZ_AAA       IP+=ISIZ_IAAA;  PROG_INST_DISPATCH;
#define INSTEND_3_MCZ_AVA       IP+=ISIZ_IACA;  PROG_INST_DISPATCH;
#define INSTEND_3_MCZ_AVV       IP+=ISIZ_IACZ;  PROG_INST_DISPATCH;
#define INSTEND_3_MDG_AAV       IP+=ISIZ_IAAG;  PROG_INST_DISPATCH;
#define INSTEND_3_MDZ_AAV       IP+=ISIZ_IAAZ;  PROG_INST_DISPATCH;
#define INSTEND_3_MFM_AAA       IP+=ISIZ_IAAA;  PROG_INST_DISPATCH;
#define INSTEND_3_MIM_AAA       IP+=ISIZ_IAAA;  PROG_INST_DISPATCH;
#define INSTEND_3_MLM_AAA       IP+=ISIZ_IAAA;  PROG_INST_DISPATCH;
#define INSTEND_3_MMI_AAA       IP+=ISIZ_IAAA;  PROG_INST_DISPATCH;
#define INSTEND_3_MMM_AAA       IP+=ISIZ_IAAA;  PROG_INST_DISPATCH;
#define INSTEND_3_MMZ_AAA       IP+=ISIZ_IAAA;  PROG_INST_DISPATCH;
#define INSTEND_3_MWM_AAA       IP+=ISIZ_IAAA;  PROG_INST_DISPATCH;
#define INSTEND_3_RBG_AVV       IP+=ISIZ_IABG;  PROG_INST_DISPATCH;
#define INSTEND_3_RDG_AAV       IP+=ISIZ_IAAG;  PROG_INST_DISPATCH;
#define INSTEND_3_RDZ_AAV       IP+=ISIZ_IAAZ;  PROG_INST_DISPATCH;
#define INSTEND_3_RMZ_AAA       IP+=ISIZ_IAAA;  PROG_INST_DISPATCH;
#define INSTEND_3_RMZ_AAV       IP+=ISIZ_IAAZ;  PROG_INST_DISPATCH;
#define INSTEND_3_WWW_AAA       IP+=ISIZ_IAAA;  PROG_INST_DISPATCH;
#define INSTEND_3_ZCZ_VVV       IP+=ISIZ_IZCZ;  PROG_INST_DISPATCH;
#define INSTEND_4_DGDG_AVAV     IP+=ISIZ_IAGAG; PROG_INST_DISPATCH;
#define INSTEND_4_MDGM_AAVA     IP+=ISIZ_IAAGA; PROG_INST_DISPATCH;
#define INSTEND_4_MMIC_AAAA     IP+=ISIZ_IAAAA; PROG_INST_DISPATCH;
#define INSTEND_4_MMMM_AAAA     IP+=ISIZ_IAAAA; PROG_INST_DISPATCH;
#define INSTEND_4_MMZZ_AAAA     IP+=ISIZ_IAAAA; PROG_INST_DISPATCH;
#define INSTEND_4_RDGZ_AAVA     IP+=ISIZ_IAAGA; PROG_INST_DISPATCH;
#define INSTEND_4_RMZZ_AAAV     IP+=ISIZ_IAAAZ; PROG_INST_DISPATCH;
#define INSTEND_4_ZMMZ_AAAA     IP+=ISIZ_IAAAA; PROG_INST_DISPATCH;
#define JMP_INSTEND_0           PROG_INST_DISPATCH;
#define JMP_INSTEND_1_A_V       PROG_INST_DISPATCH;
#define JMP_INSTEND_2_GA_VV     PROG_INST_DISPATCH;
#define JMP_INSTEND_2_MA_AV     PROG_INST_DISPATCH;

//Additional instruction endings that do not come from design sheet
#define INSTEND_1_A_V           IP+=ISIZ_IA; PROG_INST_DISPATCH;
#define JMP_INSTEND_2_BA_AV     PROG_INST_DISPATCH;
#define JMP_INSTEND_3_RDG_AAV   PROG_INST_DISPATCH;
#define JMP_INSTEND_2_RM_AA     PROG_INST_DISPATCH;


//Check division by zero
#define CHECK_ZERODIVIDE(x) { \
  if((x)==0){ \
    System::Throw(SysExceptionCode::DivideByZero); \
    return; \
  } \
}

//Check float to charr conversion
#define CHECK_FLO2CHR_CONVERSION(argnr) { \
  if(*FLO##argnr<MIN_CHR || *FLO##argnr>MAX_CHR){ \
    System::Throw(SysExceptionCode::FloatToCharConvFailure); \
    EXCP_EXIT; \
  } \
}

//Check float to short conversion
#define CHECK_FLO2SHR_CONVERSION(argnr) { \
  if(*FLO##argnr<MIN_SHR || *FLO##argnr>MAX_SHR){ \
    System::Throw(SysExceptionCode::FloatToShortConvFailure); \
    EXCP_EXIT; \
  } \
}

//Check float to integer conversion
#define CHECK_FLO2INT_CONVERSION(argnr) { \
  if(*FLO##argnr<MIN_INT || *FLO##argnr>MAX_INT){ \
    System::Throw(SysExceptionCode::FloatToIntegerConvFailure); \
    EXCP_EXIT; \
  } \
}

//Check float to long conversion
#define CHECK_FLO2LON_CONVERSION(argnr) { \
  if(*FLO##argnr<MIN_LON || *FLO##argnr>MAX_LON){ \
    System::Throw(SysExceptionCode::FloatToLongConvFailure); \
    EXCP_EXIT; \
  } \
}

//Check memory addressing within bounds
#define CHECK_SAFE_ADDRESSING(argnr,argoffset,memoffset) { \
  if(DMOD##argnr==CpuDecMode::LoclVar){ \
    if(((reinterpret_cast<char*>(*(CpuAdr *)(CodePtr+IP+argoffset))+BP)-StackPnt)+(memoffset)>_Stack.Length()-1){ \
      MAXADR=_Stack.Length()-1-BP; \
      TRYADR=((reinterpret_cast<char*>(*(CpuAdr *)(CodePtr+IP+argoffset))+BP)-StackPnt)+(memoffset); \
      System::Throw(SysExceptionCode::InvalidMemoryAddress,"stack memory","BP+"+HEXFORMAT(TRYADR),"BP+"+HEXFORMAT(MAXADR)); \
      EXCP_EXIT; \
    } \
  } \
  else if(DMOD##argnr==CpuDecMode::GlobVar){ \
    if(((reinterpret_cast<char*>(*(CpuAdr *)(CodePtr+IP+argoffset))+BP)-GlobPnt)+(memoffset)>_Glob.Length()-1){ \
      MAXADR=_Glob.Length()-1; \
      TRYADR=((reinterpret_cast<char*>(*(CpuAdr *)(CodePtr+IP+argoffset))+BP)-GlobPnt)+(memoffset); \
      System::Throw(SysExceptionCode::InvalidMemoryAddress,"global memory",HEXFORMAT(TRYADR),HEXFORMAT(MAXADR)); \
      EXCP_EXIT; \
    } \
  } \
  else{ \
    if(DRF##argnr.Id==0){ \
      System::Throw(SysExceptionCode::NullReferenceIndirection); \
      EXCP_EXIT; \
    } \
    else if(DRF##argnr.Id==_GlobalScopeId){ \
      if(DRF##argnr.Offset+(memoffset)>_Glob.Length()-1){ \
        MAXADR=_Glob.Length()-1; \
        TRYADR=DRF##argnr.Offset+(memoffset); \
        System::Throw(SysExceptionCode::InvalidMemoryAddress,"global memory",HEXFORMAT(TRYADR),HEXFORMAT(MAXADR)); \
        EXCP_EXIT; \
      } \
    } \
    else if((DRF##argnr.Id&BLOCKMASK80)==0){ \
      if(DRF##argnr.Offset+(memoffset)>_Stack.Length()-1){ \
        MAXADR=_Stack.Length()-1; \
        TRYADR=DRF##argnr.Offset+(memoffset); \
        System::Throw(SysExceptionCode::InvalidMemoryAddress,"stack memory",HEXFORMAT(TRYADR),HEXFORMAT(MAXADR)); \
        EXCP_EXIT; \
      } \
    } \
    else{ \
      if(DRF##argnr.Offset+(memoffset)>_Aux.GetSize(DRF##argnr.Id&BLOCKMASK7F)-1){ \
        TRYBLK=DRF##argnr.Id&BLOCKMASK7F; \
        MAXADR=_Aux.GetSize(TRYBLK)-1; \
        TRYADR=DRF##argnr.Offset+(memoffset); \
        System::Throw(SysExceptionCode::InvalidMemoryAddress,"block "+HEXFORMAT(TRYBLK),HEXFORMAT(TRYADR),HEXFORMAT(MAXADR)); \
        EXCP_EXIT; \
      } \
    } \
  } \
} 

//Get stack parameter argument
#define SCALLGETPARAMETER(argnr,argptr,datatype) { \
  DebugAppend(DebugLevel::VrmRuntime,"SysCall: Input ParmSt["+HEXFORMAT(PST)+"] Value " #argptr #argnr); \
  REG##argnr=(datatype *)&_ParmSt[PST]; \
  PST+=sizeof(datatype); \
  DebugMessage(DebugLevel::VrmRuntime," = "+_ToString##datatype(*reinterpret_cast<datatype*>(REG##argnr))); \
}

//Get stack parameter argument through reference indirection
#define SCALLGETREFRINDIR(argnr,argptr,datatype) { \
  DebugAppend(DebugLevel::VrmRuntime,"SysCall: Input ParmSt["+HEXFORMAT(PST)+"] Indir " #argptr #argnr); \
  DRF##argnr=*(CpuRef *)&_ParmSt[PST]; \
  DebugAppend(DebugLevel::VrmRuntime," "+_ToStringCpuRef(DRF##argnr)); \
  REFINDIRECTION(DRF##argnr,TPTR,DSO##argnr); \
  REG##argnr=(datatype *)TPTR; \
  PST+=sizeof(CpuRef); \
  DebugMessage(DebugLevel::VrmRuntime," = "+_ToString##datatype(*reinterpret_cast<datatype*>(REG##argnr))); \
}

//Print previously fetched parameter
#define SCALLOUTPARAMETER(argnr,argptr,datatype) { \
  DebugMessage(DebugLevel::VrmRuntime,"SysCall: Output " #argptr #argnr " = "+_ToString##datatype(*reinterpret_cast<datatype*>(REG##argnr))); \
}

//Instruction macros for arithmethic operations
#define INST_NEGc  InstLabelNEGc :; INSTDECODE_2_CC_AA;   (*CHR1)=-(*CHR2);           INSTEND_2_CC_AA;  
#define INST_NEGw  InstLabelNEGw :; INSTDECODE_2_WW_AA;   (*SHR1)=-(*SHR2);           INSTEND_2_WW_AA;  
#define INST_NEGi  InstLabelNEGi :; INSTDECODE_2_II_AA;   (*INT1)=-(*INT2);           INSTEND_2_II_AA;  
#define INST_NEGl  InstLabelNEGl :; INSTDECODE_2_LL_AA;   (*LON1)=-(*LON2);           INSTEND_2_LL_AA;  
#define INST_NEGf  InstLabelNEGf :; INSTDECODE_2_FF_AA;   (*FLO1)=-(*FLO2);           INSTEND_2_FF_AA;  
#define INST_ADDc  InstLabelADDc :; INSTDECODE_3_CCC_AAA; (*CHR1)=(*CHR2)+(*CHR3);    INSTEND_3_CCC_AAA;
#define INST_ADDw  InstLabelADDw :; INSTDECODE_3_WWW_AAA; (*SHR1)=(*SHR2)+(*SHR3);    INSTEND_3_WWW_AAA;
#define INST_ADDi  InstLabelADDi :; INSTDECODE_3_III_AAA; (*INT1)=(*INT2)+(*INT3);    INSTEND_3_III_AAA;
#define INST_ADDl  InstLabelADDl :; INSTDECODE_3_LLL_AAA; (*LON1)=(*LON2)+(*LON3);    INSTEND_3_LLL_AAA;
#define INST_ADDf  InstLabelADDf :; INSTDECODE_3_FFF_AAA; (*FLO1)=(*FLO2)+(*FLO3);    INSTEND_3_FFF_AAA;
#define INST_SUBc  InstLabelSUBc :; INSTDECODE_3_CCC_AAA; (*CHR1)=(*CHR2)-(*CHR3);    INSTEND_3_CCC_AAA;
#define INST_SUBw  InstLabelSUBw :; INSTDECODE_3_WWW_AAA; (*SHR1)=(*SHR2)-(*SHR3);    INSTEND_3_WWW_AAA;
#define INST_SUBi  InstLabelSUBi :; INSTDECODE_3_III_AAA; (*INT1)=(*INT2)-(*INT3);    INSTEND_3_III_AAA;
#define INST_SUBl  InstLabelSUBl :; INSTDECODE_3_LLL_AAA; (*LON1)=(*LON2)-(*LON3);    INSTEND_3_LLL_AAA;
#define INST_SUBf  InstLabelSUBf :; INSTDECODE_3_FFF_AAA; (*FLO1)=(*FLO2)-(*FLO3);    INSTEND_3_FFF_AAA;
#define INST_MULc  InstLabelMULc :; INSTDECODE_3_CCC_AAA; (*CHR1)=(*CHR2)*(*CHR3);    INSTEND_3_CCC_AAA;
#define INST_MULw  InstLabelMULw :; INSTDECODE_3_WWW_AAA; (*SHR1)=(*SHR2)*(*SHR3);    INSTEND_3_WWW_AAA;
#define INST_MULi  InstLabelMULi :; INSTDECODE_3_III_AAA; (*INT1)=(*INT2)*(*INT3);    INSTEND_3_III_AAA;
#define INST_MULl  InstLabelMULl :; INSTDECODE_3_LLL_AAA; (*LON1)=(*LON2)*(*LON3);    INSTEND_3_LLL_AAA;
#define INST_MULf  InstLabelMULf :; INSTDECODE_3_FFF_AAA; (*FLO1)=(*FLO2)*(*FLO3);    INSTEND_3_FFF_AAA;
#define INST_INCc  InstLabelINCc :; INSTDECODE_1_C_A;     (*CHR1)++;                  INSTEND_1_C_A;    
#define INST_INCw  InstLabelINCw :; INSTDECODE_1_W_A;     (*SHR1)++;                  INSTEND_1_W_A;    
#define INST_INCi  InstLabelINCi :; INSTDECODE_1_I_A;     (*INT1)++;                  INSTEND_1_I_A;    
#define INST_INCl  InstLabelINCl :; INSTDECODE_1_L_A;     (*LON1)++;                  INSTEND_1_L_A;    
#define INST_INCf  InstLabelINCf :; INSTDECODE_1_F_A;     (*FLO1)++;                  INSTEND_1_F_A;    
#define INST_DECc  InstLabelDECc :; INSTDECODE_1_C_A;     (*CHR1)--;                  INSTEND_1_C_A;    
#define INST_DECw  InstLabelDECw :; INSTDECODE_1_W_A;     (*SHR1)--;                  INSTEND_1_W_A;    
#define INST_DECi  InstLabelDECi :; INSTDECODE_1_I_A;     (*INT1)--;                  INSTEND_1_I_A;    
#define INST_DECl  InstLabelDECl :; INSTDECODE_1_L_A;     (*LON1)--;                  INSTEND_1_L_A;    
#define INST_DECf  InstLabelDECf :; INSTDECODE_1_F_A;     (*FLO1)--;                  INSTEND_1_F_A;    
#define INST_PINCc InstLabelPINCc:; INSTDECODE_2_CC_AA;   (*CHR1)=(*CHR2); (*CHR2)++; INSTEND_2_CC_AA;  
#define INST_PINCw InstLabelPINCw:; INSTDECODE_2_WW_AA;   (*SHR1)=(*SHR2); (*SHR2)++; INSTEND_2_WW_AA;  
#define INST_PINCi InstLabelPINCi:; INSTDECODE_2_II_AA;   (*INT1)=(*INT2); (*INT2)++; INSTEND_2_II_AA;  
#define INST_PINCl InstLabelPINCl:; INSTDECODE_2_LL_AA;   (*LON1)=(*LON2); (*LON2)++; INSTEND_2_LL_AA;  
#define INST_PINCf InstLabelPINCf:; INSTDECODE_2_FF_AA;   (*FLO1)=(*FLO2); (*FLO2)++; INSTEND_2_FF_AA;  
#define INST_PDECc InstLabelPDECc:; INSTDECODE_2_CC_AA;   (*CHR1)=(*CHR2); (*CHR2)--; INSTEND_2_CC_AA;  
#define INST_PDECw InstLabelPDECw:; INSTDECODE_2_WW_AA;   (*SHR1)=(*SHR2); (*SHR2)--; INSTEND_2_WW_AA;  
#define INST_PDECi InstLabelPDECi:; INSTDECODE_2_II_AA;   (*INT1)=(*INT2); (*INT2)--; INSTEND_2_II_AA;  
#define INST_PDECl InstLabelPDECl:; INSTDECODE_2_LL_AA;   (*LON1)=(*LON2); (*LON2)--; INSTEND_2_LL_AA;  
#define INST_PDECf InstLabelPDECf:; INSTDECODE_2_FF_AA;   (*FLO1)=(*FLO2); (*FLO2)--; INSTEND_2_FF_AA;  

//Instruction macros for arithmethic operations with exception checking
#define INST_DIVc  InstLabelDIVc :; INSTDECODE_3_CCC_AAA; CHECK_ZERODIVIDE((*CHR3)); (*CHR1)=(*CHR2)/(*CHR3);      INSTEND_3_CCC_AAA;
#define INST_DIVw  InstLabelDIVw :; INSTDECODE_3_WWW_AAA; CHECK_ZERODIVIDE((*SHR3)); (*SHR1)=(*SHR2)/(*SHR3);      INSTEND_3_WWW_AAA;
#define INST_DIVi  InstLabelDIVi :; INSTDECODE_3_III_AAA; CHECK_ZERODIVIDE((*INT3)); (*INT1)=(*INT2)/(*INT3);      INSTEND_3_III_AAA;
#define INST_DIVl  InstLabelDIVl :; INSTDECODE_3_LLL_AAA; CHECK_ZERODIVIDE((*LON3)); (*LON1)=(*LON2)/(*LON3);      INSTEND_3_LLL_AAA;
#define INST_DIVf  InstLabelDIVf :; INSTDECODE_3_FFF_AAA; CHECK_ZERODIVIDE((*FLO3)); (*FLO1)=(*FLO2)/(*FLO3);      INSTEND_3_FFF_AAA;
#define INST_MODc  InstLabelMODc :; INSTDECODE_3_CCC_AAA; CHECK_ZERODIVIDE((*CHR3)); (*CHR1)=MOD((*CHR2),(*CHR3)); INSTEND_3_CCC_AAA;
#define INST_MODw  InstLabelMODw :; INSTDECODE_3_WWW_AAA; CHECK_ZERODIVIDE((*SHR3)); (*SHR1)=MOD((*SHR2),(*SHR3)); INSTEND_3_WWW_AAA;
#define INST_MODi  InstLabelMODi :; INSTDECODE_3_III_AAA; CHECK_ZERODIVIDE((*INT3)); (*INT1)=MOD((*INT2),(*INT3)); INSTEND_3_III_AAA;
#define INST_MODl  InstLabelMODl :; INSTDECODE_3_LLL_AAA; CHECK_ZERODIVIDE((*LON3)); (*LON1)=MOD((*LON2),(*LON3)); INSTEND_3_LLL_AAA;
#define INST_MVDIc InstLabelMVDIc:; INSTDECODE_2_CC_AA;   CHECK_ZERODIVIDE((*CHR2)); (*CHR1)/=(*CHR2);             INSTEND_2_CC_AA;
#define INST_MVDIw InstLabelMVDIw:; INSTDECODE_2_WW_AA;   CHECK_ZERODIVIDE((*SHR2)); (*SHR1)/=(*SHR2);             INSTEND_2_WW_AA;
#define INST_MVDIi InstLabelMVDIi:; INSTDECODE_2_II_AA;   CHECK_ZERODIVIDE((*INT2)); (*INT1)/=(*INT2);             INSTEND_2_II_AA;
#define INST_MVDIl InstLabelMVDIl:; INSTDECODE_2_LL_AA;   CHECK_ZERODIVIDE((*LON2)); (*LON1)/=(*LON2);             INSTEND_2_LL_AA;
#define INST_MVDIf InstLabelMVDIf:; INSTDECODE_2_FF_AA;   CHECK_ZERODIVIDE((*FLO2)); (*FLO1)/=(*FLO2);             INSTEND_2_FF_AA;
#define INST_MVMOc InstLabelMVMOc:; INSTDECODE_2_CC_AA;   CHECK_ZERODIVIDE((*CHR2)); (*CHR1)=MOD((*CHR1),(*CHR2)); INSTEND_2_CC_AA;
#define INST_MVMOw InstLabelMVMOw:; INSTDECODE_2_WW_AA;   CHECK_ZERODIVIDE((*SHR2)); (*SHR1)=MOD((*SHR1),(*SHR2)); INSTEND_2_WW_AA;
#define INST_MVMOi InstLabelMVMOi:; INSTDECODE_2_II_AA;   CHECK_ZERODIVIDE((*INT2)); (*INT1)=MOD((*INT1),(*INT2)); INSTEND_2_II_AA;
#define INST_MVMOl InstLabelMVMOl:; INSTDECODE_2_LL_AA;   CHECK_ZERODIVIDE((*LON2)); (*LON1)=MOD((*LON1),(*LON2)); INSTEND_2_LL_AA;

//Instruction macros for logical operations
#define INST_LNOT  InstLabelLNOT :; INSTDECODE_2_BB_AA;   (*BOL1)=!(*BOL2);         INSTEND_2_BB_AA;
#define INST_LAND  InstLabelLAND :; INSTDECODE_3_BBB_AAA; (*BOL1)=(*BOL2)&&(*BOL3); INSTEND_3_BBB_AAA;
#define INST_LOR   InstLabelLOR  :; INSTDECODE_3_BBB_AAA; (*BOL1)=(*BOL2)||(*BOL3); INSTEND_3_BBB_AAA;

//Instruction macros for bitwise operations
#define INST_BNOTc InstLabelBNOTc:; INSTDECODE_2_CC_AA;   (*CHR1)=~(*CHR2);         INSTEND_2_CC_AA;
#define INST_BNOTw InstLabelBNOTw:; INSTDECODE_2_WW_AA;   (*SHR1)=~(*SHR2);         INSTEND_2_WW_AA;
#define INST_BNOTi InstLabelBNOTi:; INSTDECODE_2_II_AA;   (*INT1)=~(*INT2);         INSTEND_2_II_AA;
#define INST_BNOTl InstLabelBNOTl:; INSTDECODE_2_LL_AA;   (*LON1)=~(*LON2);         INSTEND_2_LL_AA;
#define INST_BANDc InstLabelBANDc:; INSTDECODE_3_CCC_AAA; (*CHR1)=(*CHR2)&(*CHR3);  INSTEND_3_CCC_AAA;
#define INST_BANDw InstLabelBANDw:; INSTDECODE_3_WWW_AAA; (*SHR1)=(*SHR2)&(*SHR3);  INSTEND_3_WWW_AAA;
#define INST_BANDi InstLabelBANDi:; INSTDECODE_3_III_AAA; (*INT1)=(*INT2)&(*INT3);  INSTEND_3_III_AAA;
#define INST_BANDl InstLabelBANDl:; INSTDECODE_3_LLL_AAA; (*LON1)=(*LON2)&(*LON3);  INSTEND_3_LLL_AAA;
#define INST_BORc  InstLabelBORc :; INSTDECODE_3_CCC_AAA; (*CHR1)=(*CHR2)|(*CHR3);  INSTEND_3_CCC_AAA;
#define INST_BORw  InstLabelBORw :; INSTDECODE_3_WWW_AAA; (*SHR1)=(*SHR2)|(*SHR3);  INSTEND_3_WWW_AAA;
#define INST_BORi  InstLabelBORi :; INSTDECODE_3_III_AAA; (*INT1)=(*INT2)|(*INT3);  INSTEND_3_III_AAA;
#define INST_BORl  InstLabelBORl :; INSTDECODE_3_LLL_AAA; (*LON1)=(*LON2)|(*LON3);  INSTEND_3_LLL_AAA;
#define INST_BXORc InstLabelBXORc:; INSTDECODE_3_CCC_AAA; (*CHR1)=(*CHR2)^(*CHR3);  INSTEND_3_CCC_AAA;
#define INST_BXORw InstLabelBXORw:; INSTDECODE_3_WWW_AAA; (*SHR1)=(*SHR2)^(*SHR3);  INSTEND_3_WWW_AAA;
#define INST_BXORi InstLabelBXORi:; INSTDECODE_3_III_AAA; (*INT1)=(*INT2)^(*INT3);  INSTEND_3_III_AAA;
#define INST_BXORl InstLabelBXORl:; INSTDECODE_3_LLL_AAA; (*LON1)=(*LON2)^(*LON3);  INSTEND_3_LLL_AAA;
#define INST_SHLc  InstLabelSHLc :; INSTDECODE_3_CCC_AAA; (*CHR1)=(*CHR2)<<(*CHR3); INSTEND_3_CCC_AAA;
#define INST_SHLw  InstLabelSHLw :; INSTDECODE_3_WWW_AAA; (*SHR1)=(*SHR2)<<(*SHR3); INSTEND_3_WWW_AAA;
#define INST_SHLi  InstLabelSHLi :; INSTDECODE_3_III_AAA; (*INT1)=(*INT2)<<(*INT3); INSTEND_3_III_AAA;
#define INST_SHLl  InstLabelSHLl :; INSTDECODE_3_LLL_AAA; (*LON1)=(*LON2)<<(*LON3); INSTEND_3_LLL_AAA;
#define INST_SHRc  InstLabelSHRc :; INSTDECODE_3_CCC_AAA; (*CHR1)=(*CHR2)>>(*CHR3); INSTEND_3_CCC_AAA;
#define INST_SHRw  InstLabelSHRw :; INSTDECODE_3_WWW_AAA; (*SHR1)=(*SHR2)>>(*SHR3); INSTEND_3_WWW_AAA;
#define INST_SHRi  InstLabelSHRi :; INSTDECODE_3_III_AAA; (*INT1)=(*INT2)>>(*INT3); INSTEND_3_III_AAA;
#define INST_SHRl  InstLabelSHRl :; INSTDECODE_3_LLL_AAA; (*LON1)=(*LON2)>>(*LON3); INSTEND_3_LLL_AAA;

//Instruction macros for comparison operators (except strings)
#define INST_LESb  InstLabelLESb :; INSTDECODE_3_BBB_AAA; (*BOL1)=(*BOL2)< (*BOL3); INSTEND_3_BBB_AAA;
#define INST_LESc  InstLabelLESc :; INSTDECODE_3_BCC_AAA; (*BOL1)=(*CHR2)< (*CHR3); INSTEND_3_BCC_AAA;
#define INST_LESw  InstLabelLESw :; INSTDECODE_3_BWW_AAA; (*BOL1)=(*SHR2)< (*SHR3); INSTEND_3_BWW_AAA;
#define INST_LESi  InstLabelLESi :; INSTDECODE_3_BII_AAA; (*BOL1)=(*INT2)< (*INT3); INSTEND_3_BII_AAA;
#define INST_LESl  InstLabelLESl :; INSTDECODE_3_BLL_AAA; (*BOL1)=(*LON2)< (*LON3); INSTEND_3_BLL_AAA;
#define INST_LESf  InstLabelLESf :; INSTDECODE_3_BFF_AAA; (*BOL1)=(*FLO2)< (*FLO3); INSTEND_3_BFF_AAA;
#define INST_LEQb  InstLabelLEQb :; INSTDECODE_3_BBB_AAA; (*BOL1)=(*BOL2)<=(*BOL3); INSTEND_3_BBB_AAA;
#define INST_LEQc  InstLabelLEQc :; INSTDECODE_3_BCC_AAA; (*BOL1)=(*CHR2)<=(*CHR3); INSTEND_3_BCC_AAA;
#define INST_LEQw  InstLabelLEQw :; INSTDECODE_3_BWW_AAA; (*BOL1)=(*SHR2)<=(*SHR3); INSTEND_3_BWW_AAA;
#define INST_LEQi  InstLabelLEQi :; INSTDECODE_3_BII_AAA; (*BOL1)=(*INT2)<=(*INT3); INSTEND_3_BII_AAA;
#define INST_LEQl  InstLabelLEQl :; INSTDECODE_3_BLL_AAA; (*BOL1)=(*LON2)<=(*LON3); INSTEND_3_BLL_AAA;
#define INST_LEQf  InstLabelLEQf :; INSTDECODE_3_BFF_AAA; (*BOL1)=(*FLO2)<=(*FLO3); INSTEND_3_BFF_AAA;
#define INST_GREb  InstLabelGREb :; INSTDECODE_3_BBB_AAA; (*BOL1)=(*BOL2)> (*BOL3); INSTEND_3_BBB_AAA;
#define INST_GREc  InstLabelGREc :; INSTDECODE_3_BCC_AAA; (*BOL1)=(*CHR2)> (*CHR3); INSTEND_3_BCC_AAA;
#define INST_GREw  InstLabelGREw :; INSTDECODE_3_BWW_AAA; (*BOL1)=(*SHR2)> (*SHR3); INSTEND_3_BWW_AAA;
#define INST_GREi  InstLabelGREi :; INSTDECODE_3_BII_AAA; (*BOL1)=(*INT2)> (*INT3); INSTEND_3_BII_AAA;
#define INST_GREl  InstLabelGREl :; INSTDECODE_3_BLL_AAA; (*BOL1)=(*LON2)> (*LON3); INSTEND_3_BLL_AAA;
#define INST_GREf  InstLabelGREf :; INSTDECODE_3_BFF_AAA; (*BOL1)=(*FLO2)> (*FLO3); INSTEND_3_BFF_AAA;
#define INST_GEQb  InstLabelGEQb :; INSTDECODE_3_BBB_AAA; (*BOL1)=(*BOL2)>=(*BOL3); INSTEND_3_BBB_AAA;
#define INST_GEQc  InstLabelGEQc :; INSTDECODE_3_BCC_AAA; (*BOL1)=(*CHR2)>=(*CHR3); INSTEND_3_BCC_AAA;
#define INST_GEQw  InstLabelGEQw :; INSTDECODE_3_BWW_AAA; (*BOL1)=(*SHR2)>=(*SHR3); INSTEND_3_BWW_AAA;
#define INST_GEQi  InstLabelGEQi :; INSTDECODE_3_BII_AAA; (*BOL1)=(*INT2)>=(*INT3); INSTEND_3_BII_AAA;
#define INST_GEQl  InstLabelGEQl :; INSTDECODE_3_BLL_AAA; (*BOL1)=(*LON2)>=(*LON3); INSTEND_3_BLL_AAA;
#define INST_GEQf  InstLabelGEQf :; INSTDECODE_3_BFF_AAA; (*BOL1)=(*FLO2)>=(*FLO3); INSTEND_3_BFF_AAA;
#define INST_EQUb  InstLabelEQUb :; INSTDECODE_3_BBB_AAA; (*BOL1)=(*BOL2)==(*BOL3); INSTEND_3_BBB_AAA;
#define INST_EQUc  InstLabelEQUc :; INSTDECODE_3_BCC_AAA; (*BOL1)=(*CHR2)==(*CHR3); INSTEND_3_BCC_AAA;
#define INST_EQUw  InstLabelEQUw :; INSTDECODE_3_BWW_AAA; (*BOL1)=(*SHR2)==(*SHR3); INSTEND_3_BWW_AAA;
#define INST_EQUi  InstLabelEQUi :; INSTDECODE_3_BII_AAA; (*BOL1)=(*INT2)==(*INT3); INSTEND_3_BII_AAA;
#define INST_EQUl  InstLabelEQUl :; INSTDECODE_3_BLL_AAA; (*BOL1)=(*LON2)==(*LON3); INSTEND_3_BLL_AAA;
#define INST_EQUf  InstLabelEQUf :; INSTDECODE_3_BFF_AAA; (*BOL1)=(*FLO2)==(*FLO3); INSTEND_3_BFF_AAA;
#define INST_DISb  InstLabelDISb :; INSTDECODE_3_BBB_AAA; (*BOL1)=(*BOL2)!=(*BOL3); INSTEND_3_BBB_AAA;
#define INST_DISc  InstLabelDISc :; INSTDECODE_3_BCC_AAA; (*BOL1)=(*CHR2)!=(*CHR3); INSTEND_3_BCC_AAA;
#define INST_DISw  InstLabelDISw :; INSTDECODE_3_BWW_AAA; (*BOL1)=(*SHR2)!=(*SHR3); INSTEND_3_BWW_AAA;
#define INST_DISi  InstLabelDISi :; INSTDECODE_3_BII_AAA; (*BOL1)=(*INT2)!=(*INT3); INSTEND_3_BII_AAA;
#define INST_DISl  InstLabelDISl :; INSTDECODE_3_BLL_AAA; (*BOL1)=(*LON2)!=(*LON3); INSTEND_3_BLL_AAA;
#define INST_DISf  InstLabelDISf :; INSTDECODE_3_BFF_AAA; (*BOL1)=(*FLO2)!=(*FLO3); INSTEND_3_BFF_AAA;

//Instruction macros for assignment operations
#define INST_MVb   InstLabelMVb  :; INSTDECODE_2_BB_AA; (*BOL1)=  (*BOL2); INSTEND_2_BB_AA;
#define INST_MVc   InstLabelMVc  :; INSTDECODE_2_CC_AA; (*CHR1)=  (*CHR2); INSTEND_2_CC_AA;
#define INST_MVw   InstLabelMVw  :; INSTDECODE_2_WW_AA; (*SHR1)=  (*SHR2); INSTEND_2_WW_AA;
#define INST_MVi   InstLabelMVi  :; INSTDECODE_2_II_AA; (*INT1)=  (*INT2); INSTEND_2_II_AA;
#define INST_MVl   InstLabelMVl  :; INSTDECODE_2_LL_AA; (*LON1)=  (*LON2); INSTEND_2_LL_AA;
#define INST_MVf   InstLabelMVf  :; INSTDECODE_2_FF_AA; (*FLO1)=  (*FLO2); INSTEND_2_FF_AA;
#define INST_MVr   InstLabelMVr  :; INSTDECODE_2_RR_AA; (*REF1)=  (*REF2); INSTEND_2_RR_AA;
#define INST_LOADb InstLabelLOADb:; INSTDECODE_2_BB_AV; (*BOL1)=  (*BOL2); INSTEND_2_BB_AV;
#define INST_LOADc InstLabelLOADc:; INSTDECODE_2_CC_AV; (*CHR1)=  (*CHR2); INSTEND_2_CC_AV;
#define INST_LOADw InstLabelLOADw:; INSTDECODE_2_WW_AV; (*SHR1)=  (*SHR2); INSTEND_2_WW_AV;
#define INST_LOADi InstLabelLOADi:; INSTDECODE_2_II_AV; (*INT1)=  (*INT2); INSTEND_2_II_AV;
#define INST_LOADl InstLabelLOADl:; INSTDECODE_2_LL_AV; (*LON1)=  (*LON2); INSTEND_2_LL_AV;
#define INST_LOADf InstLabelLOADf:; INSTDECODE_2_FF_AV; (*FLO1)=  (*FLO2); INSTEND_2_FF_AV;
#define INST_MVADc InstLabelMVADc:; INSTDECODE_2_CC_AA; (*CHR1)+= (*CHR2); INSTEND_2_CC_AA;
#define INST_MVADw InstLabelMVADw:; INSTDECODE_2_WW_AA; (*SHR1)+= (*SHR2); INSTEND_2_WW_AA;
#define INST_MVADi InstLabelMVADi:; INSTDECODE_2_II_AA; (*INT1)+= (*INT2); INSTEND_2_II_AA;
#define INST_MVADl InstLabelMVADl:; INSTDECODE_2_LL_AA; (*LON1)+= (*LON2); INSTEND_2_LL_AA;
#define INST_MVADf InstLabelMVADf:; INSTDECODE_2_FF_AA; (*FLO1)+= (*FLO2); INSTEND_2_FF_AA;
#define INST_MVSUc InstLabelMVSUc:; INSTDECODE_2_CC_AA; (*CHR1)-= (*CHR2); INSTEND_2_CC_AA;
#define INST_MVSUw InstLabelMVSUw:; INSTDECODE_2_WW_AA; (*SHR1)-= (*SHR2); INSTEND_2_WW_AA;
#define INST_MVSUi InstLabelMVSUi:; INSTDECODE_2_II_AA; (*INT1)-= (*INT2); INSTEND_2_II_AA;
#define INST_MVSUl InstLabelMVSUl:; INSTDECODE_2_LL_AA; (*LON1)-= (*LON2); INSTEND_2_LL_AA;
#define INST_MVSUf InstLabelMVSUf:; INSTDECODE_2_FF_AA; (*FLO1)-= (*FLO2); INSTEND_2_FF_AA;
#define INST_MVMUc InstLabelMVMUc:; INSTDECODE_2_CC_AA; (*CHR1)*= (*CHR2); INSTEND_2_CC_AA;
#define INST_MVMUw InstLabelMVMUw:; INSTDECODE_2_WW_AA; (*SHR1)*= (*SHR2); INSTEND_2_WW_AA;
#define INST_MVMUi InstLabelMVMUi:; INSTDECODE_2_II_AA; (*INT1)*= (*INT2); INSTEND_2_II_AA;
#define INST_MVMUl InstLabelMVMUl:; INSTDECODE_2_LL_AA; (*LON1)*= (*LON2); INSTEND_2_LL_AA;
#define INST_MVMUf InstLabelMVMUf:; INSTDECODE_2_FF_AA; (*FLO1)*= (*FLO2); INSTEND_2_FF_AA;
#define INST_MVSLc InstLabelMVSLc:; INSTDECODE_2_CC_AA; (*CHR1)<<=(*CHR2); INSTEND_2_CC_AA;
#define INST_MVSLw InstLabelMVSLw:; INSTDECODE_2_WW_AA; (*SHR1)<<=(*SHR2); INSTEND_2_WW_AA;
#define INST_MVSLi InstLabelMVSLi:; INSTDECODE_2_II_AA; (*INT1)<<=(*INT2); INSTEND_2_II_AA;
#define INST_MVSLl InstLabelMVSLl:; INSTDECODE_2_LL_AA; (*LON1)<<=(*LON2); INSTEND_2_LL_AA;
#define INST_MVSRc InstLabelMVSRc:; INSTDECODE_2_CC_AA; (*CHR1)>>=(*CHR2); INSTEND_2_CC_AA;
#define INST_MVSRw InstLabelMVSRw:; INSTDECODE_2_WW_AA; (*SHR1)>>=(*SHR2); INSTEND_2_WW_AA;
#define INST_MVSRi InstLabelMVSRi:; INSTDECODE_2_II_AA; (*INT1)>>=(*INT2); INSTEND_2_II_AA;
#define INST_MVSRl InstLabelMVSRl:; INSTDECODE_2_LL_AA; (*LON1)>>=(*LON2); INSTEND_2_LL_AA;
#define INST_MVANc InstLabelMVANc:; INSTDECODE_2_CC_AA; (*CHR1)&= (*CHR2); INSTEND_2_CC_AA;
#define INST_MVANw InstLabelMVANw:; INSTDECODE_2_WW_AA; (*SHR1)&= (*SHR2); INSTEND_2_WW_AA;
#define INST_MVANi InstLabelMVANi:; INSTDECODE_2_II_AA; (*INT1)&= (*INT2); INSTEND_2_II_AA;
#define INST_MVANl InstLabelMVANl:; INSTDECODE_2_LL_AA; (*LON1)&= (*LON2); INSTEND_2_LL_AA;
#define INST_MVORc InstLabelMVORc:; INSTDECODE_2_CC_AA; (*CHR1)|= (*CHR2); INSTEND_2_CC_AA;
#define INST_MVORw InstLabelMVORw:; INSTDECODE_2_WW_AA; (*SHR1)|= (*SHR2); INSTEND_2_WW_AA;
#define INST_MVORi InstLabelMVORi:; INSTDECODE_2_II_AA; (*INT1)|= (*INT2); INSTEND_2_II_AA;
#define INST_MVORl InstLabelMVORl:; INSTDECODE_2_LL_AA; (*LON1)|= (*LON2); INSTEND_2_LL_AA;
#define INST_MVXOc InstLabelMVXOc:; INSTDECODE_2_CC_AA; (*CHR1)^= (*CHR2); INSTEND_2_CC_AA;
#define INST_MVXOw InstLabelMVXOw:; INSTDECODE_2_WW_AA; (*SHR1)^= (*SHR2); INSTEND_2_WW_AA;
#define INST_MVXOi InstLabelMVXOi:; INSTDECODE_2_II_AA; (*INT1)^= (*INT2); INSTEND_2_II_AA;
#define INST_MVXOl InstLabelMVXOl:; INSTDECODE_2_LL_AA; (*LON1)^= (*LON2); INSTEND_2_LL_AA;

//Instruction macros for char operations
#define INST_CUPPR InstLabelCUPPR:; INSTDECODE_2_CC_AA; *CHR1=toupper(*CHR2); INSTEND_2_CC_AA;
#define INST_CLOWR InstLabelCLOWR:; INSTDECODE_2_CC_AA; *CHR1=tolower(*CHR2); INSTEND_2_CC_AA;

//Instruction macros for string operations
#define INST_LESs  InstLabelLESs :; INSTDECODE_3_BMM_AAA;   if(!_StC.SLES(BOL1,*MBL2,*MBL3)){ EXCP_EXIT; };        INSTEND_3_BMM_AAA;
#define INST_LEQs  InstLabelLEQs :; INSTDECODE_3_BMM_AAA;   if(!_StC.SLEQ(BOL1,*MBL2,*MBL3)){ EXCP_EXIT; };        INSTEND_3_BMM_AAA;
#define INST_GREs  InstLabelGREs :; INSTDECODE_3_BMM_AAA;   if(!_StC.SGRE(BOL1,*MBL2,*MBL3)){ EXCP_EXIT; };        INSTEND_3_BMM_AAA;
#define INST_GEQs  InstLabelGEQs :; INSTDECODE_3_BMM_AAA;   if(!_StC.SGEQ(BOL1,*MBL2,*MBL3)){ EXCP_EXIT; };        INSTEND_3_BMM_AAA;
#define INST_EQUs  InstLabelEQUs :; INSTDECODE_3_BMM_AAA;   if(!_StC.SEQU(BOL1,*MBL2,*MBL3)){ EXCP_EXIT; };        INSTEND_3_BMM_AAA;
#define INST_DISs  InstLabelDISs :; INSTDECODE_3_BMM_AAA;   if(!_StC.SDIS(BOL1,*MBL2,*MBL3)){ EXCP_EXIT; };        INSTEND_3_BMM_AAA;
#define INST_SEMP  InstLabelSEMP :; INSTDECODE_1_M_A;       if(!_StC.SEMP(MBL1)){ EXCP_EXIT; };                    INSTEND_1_M_A;
#define INST_SLEN  InstLabelSLEN :; INSTDECODE_2_ZM_AA;     if(!_StC.SLEN(WRD1,*MBL2)){ EXCP_EXIT; };              INSTEND_2_ZM_AA;
#define INST_STRIM InstLabelSTRIM:; INSTDECODE_2_MM_AA;     if(!_StC.STRIM(MBL1,*MBL2)){ EXCP_EXIT; };             INSTEND_2_MM_AA;
#define INST_SUPPR InstLabelSUPPR:; INSTDECODE_2_MM_AA;     if(!_StC.SUPPR(MBL1,*MBL2)){ EXCP_EXIT; };             INSTEND_2_MM_AA;
#define INST_SLOWR InstLabelSLOWR:; INSTDECODE_2_MM_AA;     if(!_StC.SLOWR(MBL1,*MBL2)){ EXCP_EXIT; };             INSTEND_2_MM_AA;
#define INST_SISBO InstLabelSISBO:; INSTDECODE_2_BM_AA;     if(!_StC.SISBO(BOL1,*MBL2)){ EXCP_EXIT; };             INSTEND_2_BM_AA;
#define INST_SISCH InstLabelSISCH:; INSTDECODE_2_BM_AA;     if(!_StC.SISCH(BOL1,*MBL2)){ EXCP_EXIT; };             INSTEND_2_BM_AA;
#define INST_SISSH InstLabelSISSH:; INSTDECODE_2_BM_AA;     if(!_StC.SISSH(BOL1,*MBL2)){ EXCP_EXIT; };             INSTEND_2_BM_AA;
#define INST_SISIN InstLabelSISIN:; INSTDECODE_2_BM_AA;     if(!_StC.SISIN(BOL1,*MBL2)){ EXCP_EXIT; };             INSTEND_2_BM_AA;
#define INST_SISLO InstLabelSISLO:; INSTDECODE_2_BM_AA;     if(!_StC.SISLO(BOL1,*MBL2)){ EXCP_EXIT; };             INSTEND_2_BM_AA;
#define INST_SISFL InstLabelSISFL:; INSTDECODE_2_BM_AA;     if(!_StC.SISFL(BOL1,*MBL2)){ EXCP_EXIT; };             INSTEND_2_BM_AA;
#define INST_SRGHT InstLabelSRGHT:; INSTDECODE_3_MMZ_AAA;   if(!_StC.SRGHT(MBL1,*MBL2,*WRD3)){ EXCP_EXIT; };       INSTEND_3_MMZ_AAA;
#define INST_SLEFT InstLabelSLEFT:; INSTDECODE_3_MMZ_AAA;   if(!_StC.SLEFT(MBL1,*MBL2,*WRD3)){ EXCP_EXIT; };       INSTEND_3_MMZ_AAA;
#define INST_SCUTR InstLabelSCUTR:; INSTDECODE_3_MMZ_AAA;   if(!_StC.SCUTR(MBL1,*MBL2,*WRD3)){ EXCP_EXIT; };       INSTEND_3_MMZ_AAA;
#define INST_SCUTL InstLabelSCUTL:; INSTDECODE_3_MMZ_AAA;   if(!_StC.SCUTL(MBL1,*MBL2,*WRD3)){ EXCP_EXIT; };       INSTEND_3_MMZ_AAA;
#define INST_SCONC InstLabelSCONC:; INSTDECODE_3_MMM_AAA;   if(!_StC.SCONC(MBL1,*MBL2,*MBL3)){ EXCP_EXIT; };       INSTEND_3_MMM_AAA;
#define INST_SMVCO InstLabelSMVCO:; INSTDECODE_2_MM_AA;     if(!_StC.SMVCO(MBL1,*MBL2)){ EXCP_EXIT; };             INSTEND_2_MM_AA;
#define INST_SMVRC InstLabelSMVRC:; INSTDECODE_2_MM_AA;     if(!_StC.SMVRC(MBL1,*MBL2)){ EXCP_EXIT; };             INSTEND_2_MM_AA;
#define INST_SFIND InstLabelSFIND:; INSTDECODE_4_ZMMZ_AAAA; if(!_StC.SFIND(WRD1,*MBL2,*MBL3,*WRD4)){ EXCP_EXIT; }; INSTEND_4_ZMMZ_AAAA;
#define INST_SLJUS InstLabelSLJUS:; INSTDECODE_4_MMIC_AAAA; if(!_StC.SLJUS(MBL1,*MBL2,*INT3,*CHR4)){ EXCP_EXIT; }; INSTEND_4_MMIC_AAAA;
#define INST_SRJUS InstLabelSRJUS:; INSTDECODE_4_MMIC_AAAA; if(!_StC.SRJUS(MBL1,*MBL2,*INT3,*CHR4)){ EXCP_EXIT; }; INSTEND_4_MMIC_AAAA;
#define INST_SMATC InstLabelSMATC:; INSTDECODE_3_BMM_AAA;   if(!_StC.SMATC(BOL1,*MBL2,*MBL3)){ EXCP_EXIT; };       INSTEND_3_BMM_AAA;
#define INST_SLIKE InstLabelSLIKE:; INSTDECODE_3_BMM_AAA;   if(!_StC.SLIKE(BOL1,*MBL2,*MBL3)){ EXCP_EXIT; };       INSTEND_3_BMM_AAA;
#define INST_SREPL InstLabelSREPL:; INSTDECODE_3_MMI_AAA;   if(!_StC.SREPL(MBL1,*MBL2,*INT3)){ EXCP_EXIT; };       INSTEND_3_MMI_AAA;
#define INST_SSTWI InstLabelSSTWI:; INSTDECODE_3_BMM_AAA;   if(!_StC.SSTWI(BOL1,*MBL2,*MBL3)){ EXCP_EXIT; };       INSTEND_3_BMM_AAA;
#define INST_SENWI InstLabelSENWI:; INSTDECODE_3_BMM_AAA;   if(!_StC.SENWI(BOL1,*MBL2,*MBL3)){ EXCP_EXIT; };       INSTEND_3_BMM_AAA;
#define INST_SMID  InstLabelSMID :; INSTDECODE_4_MMZZ_AAAA; if(!_StC.SMID(MBL1,*MBL2,*WRD3,*WRD4)){ EXCP_EXIT; };  INSTEND_4_MMZZ_AAAA;
#define INST_SINDX InstLabelSINDX:; INSTDECODE_3_RMZ_AAA;   if(!_StC.SINDX(REF1,*MBL2,*WRD3)){ EXCP_EXIT; };       INSTEND_3_RMZ_AAA;
#define INST_SSUBS InstLabelSSUBS:; INSTDECODE_4_MMMM_AAAA; if(!_StC.SSUBS(MBL1,*MBL2,*MBL3,*MBL4)){ EXCP_EXIT; }; INSTEND_4_MMMM_AAAA;
#define INST_SCOPY InstLabelSCOPY:; INSTDECODE_2_MM_AA;     if(!_StC.SCOPY(MBL1,*MBL2)){ EXCP_EXIT; };             INSTEND_2_MM_AA;
#define INST_SSWCP InstLabelSSWCP:; INSTDECODE_2_MM_AA;     if(!_StC.SSWCP(MBL1,*MBL2)){ EXCP_EXIT; };             INSTEND_2_MM_AA;
#define INST_SSPLI InstLabelSSPLI:; INSTDECODE_3_MMM_AAA;   if(!_ArC.SSPL(MBL1,*MBL2,*MBL3)){ EXCP_EXIT; };        INSTEND_3_MMM_AAA;
#define INST_ACOPY InstLabelACOPY:; INSTDECODE_2_MM_AA;     if(!_ArC.ACOPY(MBL1,*MBL2)){ EXCP_EXIT; };             INSTEND_2_MM_AA;

//Instruction macros for jump instructions
#define INST_JMP   InstLabelJMP  :; INSTDECODE_1_A_V;   IP+=(*ADR1);                                                                           JMP_INSTEND_1_A_V;
#define INST_JMPTR InstLabelJMPTR:; INSTDECODE_2_BA_AV; if(*BOL1){ IP+=(*ADR2); RESTORE_HANDLER; RESTORE_DECODER(1); JMP_INSTEND_2_BA_AV; }    INSTEND_2_BA_AV;
#define INST_JMPFL InstLabelJMPFL:; INSTDECODE_2_BA_AV; if(!(*BOL1)){ IP+=(*ADR2); RESTORE_HANDLER; RESTORE_DECODER(1); JMP_INSTEND_2_BA_AV; } INSTEND_2_BA_AV;

//Instruction macros for data conversions
#define INST_BO2CH InstLabelBO2CH:; INSTDECODE_2_CB_AA;   if(*BOL2){ (*CHR1)=1; } else{ (*CHR1)=0; }            INSTEND_2_CB_AA;
#define INST_BO2SH InstLabelBO2SH:; INSTDECODE_2_WB_AA;   if(*BOL2){ (*SHR1)=1; } else{ (*SHR1)=0; }            INSTEND_2_WB_AA;
#define INST_BO2IN InstLabelBO2IN:; INSTDECODE_2_IB_AA;   if(*BOL2){ (*INT1)=1; } else{ (*INT1)=0; }            INSTEND_2_IB_AA;
#define INST_BO2LO InstLabelBO2LO:; INSTDECODE_2_LB_AA;   if(*BOL2){ (*LON1)=1; } else{ (*LON1)=0; }            INSTEND_2_LB_AA;
#define INST_BO2FL InstLabelBO2FL:; INSTDECODE_2_FB_AA;   if(*BOL2){ (*FLO1)=1; } else{ (*FLO1)=0; }            INSTEND_2_FB_AA;
#define INST_BO2ST InstLabelBO2ST:; INSTDECODE_2_MB_AA;   if(!_StC.SBO2S(MBL1,*BOL2)){ EXCP_EXIT; }             INSTEND_2_MB_AA;
#define INST_CH2BO InstLabelCH2BO:; INSTDECODE_2_BC_AA;   if(*CHR2!=0){ (*BOL1)=true; } else{ (*BOL1)=false; }  INSTEND_2_BC_AA;
#define INST_CH2SH InstLabelCH2SH:; INSTDECODE_2_WC_AA;   (*SHR1)=(CpuShr)(*CHR2);                              INSTEND_2_WC_AA;
#define INST_CH2IN InstLabelCH2IN:; INSTDECODE_2_IC_AA;   (*INT1)=(CpuInt)(*CHR2);                              INSTEND_2_IC_AA;
#define INST_CH2LO InstLabelCH2LO:; INSTDECODE_2_LC_AA;   (*LON1)=(CpuLon)(*CHR2);                              INSTEND_2_LC_AA;
#define INST_CH2FL InstLabelCH2FL:; INSTDECODE_2_FC_AA;   (*FLO1)=(CpuFlo)(*CHR2);                              INSTEND_2_FC_AA;
#define INST_CH2ST InstLabelCH2ST:; INSTDECODE_2_MC_AA;   if(!_StC.SCH2S(MBL1,*CHR2)){ EXCP_EXIT; }             INSTEND_2_MC_AA;
#define INST_CHFMT InstLabelCHFMT:; INSTDECODE_3_MCM_AAA; if(!_StC.SCHFM(MBL1,*CHR2,*MBL3)){ EXCP_EXIT; }       INSTEND_3_MCM_AAA;
#define INST_SH2BO InstLabelSH2BO:; INSTDECODE_2_BW_AA;   if(*SHR2!=0){ (*BOL1)=true; } else{ (*BOL1)=false; }  INSTEND_2_BW_AA;
#define INST_SH2CH InstLabelSH2CH:; INSTDECODE_2_CW_AA;   (*CHR1)=(CpuChr)(*SHR2);                              INSTEND_2_CW_AA;
#define INST_SH2IN InstLabelSH2IN:; INSTDECODE_2_IW_AA;   (*INT1)=(CpuInt)(*SHR2);                              INSTEND_2_IW_AA;
#define INST_SH2LO InstLabelSH2LO:; INSTDECODE_2_LW_AA;   (*LON1)=(CpuLon)(*SHR2);                              INSTEND_2_LW_AA;
#define INST_SH2FL InstLabelSH2FL:; INSTDECODE_2_FW_AA;   (*FLO1)=(CpuFlo)(*SHR2);                              INSTEND_2_FW_AA;
#define INST_SH2ST InstLabelSH2ST:; INSTDECODE_2_MW_AA;   if(!_StC.SSH2S(MBL1,*SHR2)){ EXCP_EXIT; }             INSTEND_2_MW_AA;
#define INST_SHFMT InstLabelSHFMT:; INSTDECODE_3_MWM_AAA; if(!_StC.SSHFM(MBL1,*SHR2,*MBL3)){ EXCP_EXIT; }       INSTEND_3_MWM_AAA;
#define INST_IN2BO InstLabelIN2BO:; INSTDECODE_2_BI_AA;   if(*INT2!=0){ (*BOL1)=true; } else{ (*BOL1)=false; }  INSTEND_2_BI_AA;
#define INST_IN2CH InstLabelIN2CH:; INSTDECODE_2_CI_AA;   (*CHR1)=(CpuChr)(*INT2);                              INSTEND_2_CI_AA;
#define INST_IN2SH InstLabelIN2SH:; INSTDECODE_2_WI_AA;   (*SHR1)=(CpuShr)(*INT2);                              INSTEND_2_WI_AA;
#define INST_IN2LO InstLabelIN2LO:; INSTDECODE_2_LI_AA;   (*LON1)=(CpuLon)(*INT2);                              INSTEND_2_LI_AA;
#define INST_IN2FL InstLabelIN2FL:; INSTDECODE_2_FI_AA;   (*FLO1)=(CpuFlo)(*INT2);                              INSTEND_2_FI_AA;
#define INST_IN2ST InstLabelIN2ST:; INSTDECODE_2_MI_AA;   if(!_StC.SIN2S(MBL1,*INT2)){ EXCP_EXIT; }             INSTEND_2_MI_AA;
#define INST_INFMT InstLabelINFMT:; INSTDECODE_3_MIM_AAA; if(!_StC.SINFM(MBL1,*INT2,*MBL3)){ EXCP_EXIT; }       INSTEND_3_MIM_AAA;
#define INST_LO2BO InstLabelLO2BO:; INSTDECODE_2_BL_AA;   if(*LON2!=0){ (*BOL1)=true; } else{ (*BOL1)=false; }  INSTEND_2_BL_AA;
#define INST_LO2CH InstLabelLO2CH:; INSTDECODE_2_CL_AA;   (*CHR1)=(CpuChr)(*LON2);                              INSTEND_2_CL_AA;
#define INST_LO2SH InstLabelLO2SH:; INSTDECODE_2_WL_AA;   (*SHR1)=(CpuShr)(*LON2);                              INSTEND_2_WL_AA;
#define INST_LO2IN InstLabelLO2IN:; INSTDECODE_2_IL_AA;   (*INT1)=(CpuInt)(*LON2);                              INSTEND_2_IL_AA;
#define INST_LO2FL InstLabelLO2FL:; INSTDECODE_2_FL_AA;   (*FLO1)=(CpuFlo)(*LON2);                              INSTEND_2_FL_AA;
#define INST_LO2ST InstLabelLO2ST:; INSTDECODE_2_ML_AA;   if(!_StC.SLO2S(MBL1,*LON2)){ EXCP_EXIT; }             INSTEND_2_ML_AA;
#define INST_LOFMT InstLabelLOFMT:; INSTDECODE_3_MLM_AAA; if(!_StC.SLOFM(MBL1,*LON2,*MBL3)){ EXCP_EXIT; }       INSTEND_3_MLM_AAA;
#define INST_FL2BO InstLabelFL2BO:; INSTDECODE_2_BF_AA;   if(*FLO2!=0){ (*BOL1)=true; } else{ (*BOL1)=false; }  INSTEND_2_BF_AA;
#define INST_FL2CH InstLabelFL2CH:; INSTDECODE_2_CF_AA;   CHECK_FLO2CHR_CONVERSION(2); (*CHR1)=(CpuChr)(*FLO2); INSTEND_2_CF_AA;
#define INST_FL2SH InstLabelFL2SH:; INSTDECODE_2_WF_AA;   CHECK_FLO2SHR_CONVERSION(2); (*SHR1)=(CpuShr)(*FLO2); INSTEND_2_WF_AA;
#define INST_FL2IN InstLabelFL2IN:; INSTDECODE_2_IF_AA;   CHECK_FLO2INT_CONVERSION(2); (*INT1)=(CpuInt)(*FLO2); INSTEND_2_IF_AA;
#define INST_FL2LO InstLabelFL2LO:; INSTDECODE_2_LF_AA;   CHECK_FLO2LON_CONVERSION(2); (*LON1)=(CpuLon)(*FLO2); INSTEND_2_LF_AA;
#define INST_FL2ST InstLabelFL2ST:; INSTDECODE_2_MF_AA;   if(!_StC.SFL2S(MBL1,*FLO2)){ EXCP_EXIT; }             INSTEND_2_MF_AA;
#define INST_FLFMT InstLabelFLFMT:; INSTDECODE_3_MFM_AAA; if(!_StC.SFLFM(MBL1,*FLO2,*MBL3)){ EXCP_EXIT; }       INSTEND_3_MFM_AAA;
#define INST_ST2BO InstLabelST2BO:; INSTDECODE_2_BM_AA;   if(!_StC.SST2B(BOL1,*MBL2)){ EXCP_EXIT; }             INSTEND_2_BM_AA;
#define INST_ST2CH InstLabelST2CH:; INSTDECODE_2_CM_AA;   if(!_StC.SST2C(CHR1,*MBL2)){ EXCP_EXIT; }             INSTEND_2_CM_AA;
#define INST_ST2SH InstLabelST2SH:; INSTDECODE_2_WM_AA;   if(!_StC.SST2W(SHR1,*MBL2)){ EXCP_EXIT; }             INSTEND_2_WM_AA;
#define INST_ST2IN InstLabelST2IN:; INSTDECODE_2_IM_AA;   if(!_StC.SST2I(INT1,*MBL2)){ EXCP_EXIT; }             INSTEND_2_IM_AA;
#define INST_ST2LO InstLabelST2LO:; INSTDECODE_2_LM_AA;   if(!_StC.SST2L(LON1,*MBL2)){ EXCP_EXIT; }             INSTEND_2_LM_AA;
#define INST_ST2FL InstLabelST2FL:; INSTDECODE_2_FM_AA;   if(!_StC.SST2F(FLO1,*MBL2)){ EXCP_EXIT; }             INSTEND_2_FM_AA;

//Instruction macros for memory instructions
#define INST_REFOF InstLabelREFOF:; INSTDECODE_3_RDZ_AAV; GET_ARG_AS_REFERENCE(2,*REF1,AOFF_IA); (*REF1).Offset+=(*WRD3);                                                   INSTEND_3_RDZ_AAV;
#define INST_REFAD InstLabelREFAD:; INSTDECODE_2_RZ_AV;   (*REF1).Offset+=(*WRD2);                                                                                          INSTEND_2_RZ_AV;
#define INST_REFER InstLabelREFER:; INSTDECODE_2_RD_AA;   GET_ARG_AS_REFERENCE(2,*REF1,AOFF_IA);                                                                            INSTEND_2_RD_AA;
#define INST_COPY  InstLabelCOPY :; INSTDECODE_3_DDZ_AAV; CHECK_SAFE_ADDRESSING(1,AOFF_I,(*WRD3)-1); CHECK_SAFE_ADDRESSING(2,AOFF_IA,(*WRD3)-1); MemCpy(DAT1,DAT2,(*WRD3)); INSTEND_3_DDZ_AAV;
#define INST_CLEAR InstLabelCLEAR:; INSTDECODE_2_DZ_AV;   CHECK_SAFE_ADDRESSING(1,AOFF_I,(*WRD2)-1); memset(DAT1,0,(*WRD2));                                                INSTEND_2_DZ_AV;
#define INST_TOCA  InstLabelTOCA :; INSTDECODE_3_MDZ_AAV; if(!_ArC.TOCA(MBL1,DAT2,*WRD3)){ EXCP_EXIT; }                                                                     INSTEND_3_MDZ_AAV;
#define INST_STOCA InstLabelSTOCA:; INSTDECODE_2_MM_AA;   if(!_ArC.STOCA(MBL1,*MBL2)){ EXCP_EXIT; }                                                                         INSTEND_2_MM_AA;
#define INST_ATOCA InstLabelATOCA:; INSTDECODE_2_MM_AA;   if(!_ArC.ATOCA(MBL1,*MBL2)){ EXCP_EXIT; }                                                                         INSTEND_2_MM_AA;
#define INST_FRCA  InstLabelFRCA :; INSTDECODE_3_DMZ_AAV; if(!_ArC.FRCA(DAT1,*MBL2,*WRD3)){ EXCP_EXIT; }                                                                    INSTEND_3_DMZ_AAV;
#define INST_SFRCA InstLabelSFRCA:; INSTDECODE_2_MM_AA;   if(!_ArC.SFRCA(MBL1,*MBL2)){ EXCP_EXIT; }                                                                         INSTEND_2_MM_AA;
#define INST_AFRCA InstLabelAFRCA:; INSTDECODE_2_MM_AA;   if(!_ArC.AFRCA(*MBL1,*MBL2)){ EXCP_EXIT; }                                                                        INSTEND_2_MM_AA;
#define INST_STACK InstLabelSTACK:; INSTDECODE_1_L_V;     if(!_Stack.Reserve(*LON1)){ System::Throw(SysExceptionCode::StackOverflow,ToString(*LON1)); EXCP_EXIT; } \
                                                          if(StackPnt!=_Stack.Pnt()){ \
                                                            if(!_DecodeLocalVariables(false,CodePtr,StackPnt,_Stack.Pnt())){ EXCP_EXIT; } \
                                                            StackPnt=_Stack.Pnt(); \
                                                          } \
                                                          INSTEND_1_L_V;

//Instruction macros for 1-dimensional fixed array operations
#define INST_AF1RF InstLabelAF1RF:; INSTDECODE_4_RDGZ_AAVA; if(!_ArC.AF1OF(*AGX3,*WRD4,&OFF)){ EXCP_EXIT; } GET_ARG_AS_REFERENCE(2,*REF1,AOFF_IA); (*REF1).Offset+=OFF;                INSTEND_4_RDGZ_AAVA;
#define INST_AF1RW InstLabelAF1RW:; INSTDECODE_3_GAA_VVV;   if(!_ArC.AF1RW(*AGX1,*ADR2,DMOD2,IP+*ADR3)){ EXCP_EXIT; } if(*ADR2!=0){ GET_POINTER(*ADR2,DMOD2,CpuWrd,WRDP); (*WRDP)=0; } INSTEND_3_GAA_VVV;
#define INST_AF1SJ InstLabelAF1SJ:; INSTDECODE_4_MDGM_AAVA; if(!_ArC.AF1SJ(MBL1,(char *)DAT2,*AGX3,*MBL4)){ EXCP_EXIT; }                                                               INSTEND_4_MDGM_AAVA;
#define INST_AF1CJ InstLabelAF1CJ:; INSTDECODE_4_MDGM_AAVA; if(!_ArC.AF1CJ(MBL1,(char *)DAT2,*AGX3,*MBL4)){ EXCP_EXIT; }                                                               INSTEND_4_MDGM_AAVA;
#define INST_AF1NX InstLabelAF1NX:; INSTDECODE_2_GA_VV;     if(!_ArC.AF1NX(*AGX1,&INDXADDR,&INDXDMOD)){ EXCP_EXIT; } \
                                                            if(INDXADDR!=0){ GET_POINTER(INDXADDR,INDXDMOD,CpuWrd,WRDP); (*WRDP)++; } IP+=(*ADR2); RESTORE_HANDLER;                    JMP_INSTEND_2_GA_VV;
#define INST_AF1FO InstLabelAF1FO:; INSTDECODE_3_RDG_AAV;   if(!_ArC.AF1FO(*AGX3,&OFF,&EXITADR)){ EXCP_EXIT; } \
                                                            if(EXITADR!=0){ IP=EXITADR; RESTORE_HANDLER; RESTORE_DECODER(1); RESTORE_DECODER(2);                                       JMP_INSTEND_3_RDG_AAV; } \
                                                            else{ GET_ARG_AS_REFERENCE(2,*REF1,AOFF_IA); (*REF1).Offset+=OFF; }                                                        INSTEND_3_RDG_AAV;

//Instruction macros for fixed array operations
#define INST_AFDEF InstLabelAFDEF:; INSTDECODE_3_GCZ_VVV; if(!_ArC.AFDEF(*AGX1,*CHR2,*WRD3)){ EXCP_EXIT; }                                                      INSTEND_3_GCZ_VVV;
#define INST_AFSSZ InstLabelAFSSZ:; INSTDECODE_3_GCZ_VAA; if(!_ArC.AFSSZ(*AGX1,*CHR2,*WRD3)){ EXCP_EXIT; }                                                      INSTEND_3_GCZ_VAA;
#define INST_AFGET InstLabelAFGET:; INSTDECODE_3_GCZ_VAA; if(!_ArC.AFGET(*AGX1,*CHR2,WRD3)){ EXCP_EXIT; }                                                       INSTEND_3_GCZ_VAA;
#define INST_AFIDX InstLabelAFIDX:; INSTDECODE_3_GCZ_VVA; if(!_ArC.AFIDX(*AGX1,*CHR2,*WRD3)){ EXCP_EXIT; }                                                      INSTEND_3_GCZ_VVA;
#define INST_AFREF InstLabelAFREF:; INSTDECODE_3_RDG_AAV; if(!_ArC.AFOFN(*AGX3,&OFF)){ EXCP_EXIT; } GET_ARG_AS_REFERENCE(2,*REF1,AOFF_IA); (*REF1).Offset+=OFF; INSTEND_3_RDG_AAV;

//Instruction macros for 1-dimensional dynamic array operations
#define INST_AD1EM InstLabelAD1EM:; INSTDECODE_2_MZ_AV;     if(!_ArC.AD1EM(MBL1,*WRD2)){ EXCP_EXIT; }                                                                                   INSTEND_2_MZ_AV;
#define INST_AD1DF InstLabelAD1DF:; INSTDECODE_1_M_A;       if(!_ArC.AD1DF(MBL1)){ EXCP_EXIT; }                                                                                         INSTEND_1_M_A;
#define INST_AD1AP InstLabelAD1AP:; INSTDECODE_3_RMZ_AAV;   if(!_ArC.AD1AP(*MBL2,&OFF,*WRD3)){ EXCP_EXIT; } (*REF1)=(CpuRef){ (CpuMbl)(BLOCKMASK80|(*MBL2)),OFF};                       INSTEND_3_RMZ_AAV;
#define INST_AD1IN InstLabelAD1IN:; INSTDECODE_4_RMZZ_AAAV; if(!_ArC.AD1IN(*MBL2,*WRD3,&OFF,*WRD4)){ EXCP_EXIT; } (*REF1)=(CpuRef){ (CpuMbl)(BLOCKMASK80|(*MBL2)),OFF };                INSTEND_4_RMZZ_AAAV;
#define INST_AD1DE InstLabelAD1DE:; INSTDECODE_2_MZ_AA;     if(!_ArC.AD1DE(*MBL1,*WRD2)){ EXCP_EXIT; }                                                                                  INSTEND_2_MZ_AA;
#define INST_AD1RF InstLabelAD1RF:; INSTDECODE_3_RMZ_AAA;   if(!_ArC.AD1OF(*MBL2,*WRD3,&OFF)){ EXCP_EXIT; } (*REF1)=(CpuRef){ (CpuMbl)(BLOCKMASK80|(*MBL2)),OFF };                      INSTEND_3_RMZ_AAA;
#define INST_AD1RS InstLabelAD1RS:; INSTDECODE_1_M_A;       if(!_ArC.AD1RS(MBL1)){ EXCP_EXIT; }                                                                                         INSTEND_1_M_A;
#define INST_AD1RW InstLabelAD1RW:; INSTDECODE_3_MAA_AVV;   if(!_ArC.AD1RW(*MBL1,*ADR2,DMOD2,IP+*ADR3)){ EXCP_EXIT; } if(*ADR2!=0){ GET_POINTER(*ADR2,DMOD2,CpuWrd,WRDP); (*WRDP)=0; }  INSTEND_3_MAA_AVV;
#define INST_AD1SJ InstLabelAD1SJ:; INSTDECODE_3_MMM_AAA;   if(!_ArC.AD1SJ(MBL1,*MBL2,*MBL3)){ EXCP_EXIT; }                                                                             INSTEND_3_MMM_AAA;
#define INST_AD1CJ InstLabelAD1CJ:; INSTDECODE_3_MMM_AAA;   if(!_ArC.AD1CJ(MBL1,*MBL2,*MBL3)){ EXCP_EXIT; }                                                                             INSTEND_3_MMM_AAA;
#define INST_AD1NX InstLabelAD1NX:; INSTDECODE_2_MA_AV;     if(!_ArC.AD1NX(*MBL1,&INDXADDR,&INDXDMOD)){ EXCP_EXIT; } \
                                                            if(INDXADDR!=0){ GET_POINTER(INDXADDR,INDXDMOD,CpuWrd,WRDP); (*WRDP)++; } IP+=(*ADR2); RESTORE_HANDLER; RESTORE_DECODER(1); JMP_INSTEND_2_MA_AV;
#define INST_AD1FO InstLabelAD1FO:; INSTDECODE_2_RM_AA;     if(!_ArC.AD1FO(*MBL2,&OFF,&EXITADR)){ EXCP_EXIT; } \
                                                            if(EXITADR!=0){ IP=EXITADR; RESTORE_HANDLER; RESTORE_DECODER(1); RESTORE_DECODER(2);                                        JMP_INSTEND_2_RM_AA; } \
                                                            else{ (*REF1)=(CpuRef){ (CpuMbl)(BLOCKMASK80|(*MBL2)),OFF }; }                                                              INSTEND_2_RM_AA;

//Instruction macros for dynamic array operations
#define INST_ADEMP InstLabelADEMP:; INSTDECODE_3_MCZ_AVV;   if(!_ArC.ADEMP(MBL1,*CHR2,*WRD3)){ EXCP_EXIT; }                                                  INSTEND_3_MCZ_AVV;
#define INST_ADDEF InstLabelADDEF:; INSTDECODE_3_MCZ_AVV;   if(!_ArC.ADDEF(MBL1,*CHR2,*WRD3)){ EXCP_EXIT; }                                                  INSTEND_3_MCZ_AVV;
#define INST_ADSET InstLabelADSET:; INSTDECODE_3_MCZ_AVA;   if(!_ArC.ADSET(MBL1,*CHR2,*WRD3)){ EXCP_EXIT; }                                                  INSTEND_3_MCZ_AVA;
#define INST_ADRSZ InstLabelADRSZ:; INSTDECODE_1_M_A;       if(!_ArC.ADRSZ(MBL1)){ EXCP_EXIT; }                                                              INSTEND_1_M_A;
#define INST_ADGET InstLabelADGET:; INSTDECODE_3_MCZ_AAA;   if(!_ArC.ADGET(*MBL1,*CHR2,WRD3)){ EXCP_EXIT; }                                                  INSTEND_3_MCZ_AAA;
#define INST_ADRST InstLabelADRST:; INSTDECODE_1_M_A;       if(!_ArC.ADRST(MBL1)){ EXCP_EXIT; }                                                              INSTEND_1_M_A;
#define INST_ADIDX InstLabelADIDX:; INSTDECODE_3_MCZ_AVA;   if(!_ArC.ADIDX(*MBL1,*CHR2,*WRD3)){ EXCP_EXIT; }                                                 INSTEND_3_MCZ_AVA;
#define INST_ADSIZ InstLabelADSIZ:; INSTDECODE_2_MZ_AA;     if(!_ArC.ADSIZ(*MBL1,WRD2)){ EXCP_EXIT; }                                                        INSTEND_2_MZ_AA;
#define INST_ADREF InstLabelADREF:; INSTDECODE_2_RM_AA;     if(!_ArC.ADOFN(*MBL2,&OFF)){ EXCP_EXIT; } (*REF1)=(CpuRef){ (CpuMbl)(BLOCKMASK80|(*MBL2)),OFF }; INSTEND_2_RM_AA;
#define INST_AF2F  InstLabelAF2F :; INSTDECODE_4_DGDG_AVAV; if(!_ArC.AF2F(DAT1,*AGX2,DAT3,*AGX4)){ EXCP_EXIT; }                                              INSTEND_4_DGDG_AVAV;
#define INST_AF2D  InstLabelAF2D :; INSTDECODE_3_MDG_AAV;   if(!_ArC.AF2D(MBL1,DAT2,*AGX3)){ EXCP_EXIT; }                                                    INSTEND_3_MDG_AAV;
#define INST_AD2F  InstLabelAD2F :; INSTDECODE_3_DGM_AVA;   if(!_ArC.AD2F(DAT1,*AGX2,*MBL3)){ EXCP_EXIT; }                                                   INSTEND_3_DGM_AVA;
#define INST_AD2D  InstLabelAD2D :; INSTDECODE_2_MM_AA;     if(!_ArC.AD2D(MBL1,*MBL2)){ EXCP_EXIT; }                                                         INSTEND_2_MM_AA;

//No operation
#define INST_NOP InstLabelNOP:; INSTDECODE_0; INSTEND_0;

//Instruction macro INST_DAGV1
#define INST_DAGV1 \
InstLabelDAGV1:; \
INSTDECODE_2_WW_VV; \
HPTR=(CpuWrd *)(CodePtr+IP+(*SHR1)); \
HICD=*(CpuIcd *)(CodePtr+IP+(*SHR1)+sizeof(CpuWrd)); \
(*HPTR)=(CpuWrd)&&RunProgInstEndRestore; \
ADP1=(CpuAdr *)(CodePtr+IP+(*SHR2)); \
ADV1=*ADP1; \
if(ADV1<0 || ADV1>=_Glob.Length()-1){ MAXADR=_Glob.Length()-1; System::Throw(SysExceptionCode::InvalidMemoryAddress,"global memory",HEXFORMAT(ADV1),HEXFORMAT(MAXADR)); return; } \
(*ADP1)=(CpuAdr)reinterpret_cast<CpuAdr>(GlobPnt+ADV1-BP); \
DMOD1=CpuDecMode::GlobVar; \
DebugMessage(DebugLevel::VrmRuntime,"Decode argument 1 as "+CpuDecModeName(DMOD1)+" (orig="+HEXFORMAT(ADV1)+" stored="+HEXFORMAT(*ADP1)+")"); \
CONST_INSTEND_2_WW_VV;

//Instruction macro INST_DAGV2
#define INST_DAGV2 \
InstLabelDAGV2:; \
INSTDECODE_2_WW_VV; \
HPTR=(CpuWrd *)(CodePtr+IP+(*SHR1)); \
HICD=*(CpuIcd *)(CodePtr+IP+(*SHR1)+sizeof(CpuWrd)); \
(*HPTR)=(CpuWrd)&&RunProgInstEndRestore; \
ADP2=(CpuAdr *)(CodePtr+IP+(*SHR2)); \
ADV2=*ADP2; \
if(ADV2<0 || ADV2>=_Glob.Length()-1){ MAXADR=_Glob.Length()-1; System::Throw(SysExceptionCode::InvalidMemoryAddress,"global memory",HEXFORMAT(ADV2),HEXFORMAT(MAXADR)); return; } \
(*ADP2)=(CpuAdr)reinterpret_cast<CpuAdr>(GlobPnt+ADV2-BP); \
DMOD2=CpuDecMode::GlobVar; \
DebugMessage(DebugLevel::VrmRuntime,"Decode argument 2 as "+CpuDecModeName(DMOD2)+" (orig="+HEXFORMAT(ADV2)+" stored="+HEXFORMAT(*ADP2)+")"); \
CONST_INSTEND_2_WW_VV;

//Instruction macro INST_DAGV3
#define INST_DAGV3 \
InstLabelDAGV3:; \
INSTDECODE_2_WW_VV; \
HPTR=(CpuWrd *)(CodePtr+IP+(*SHR1)); \
HICD=*(CpuIcd *)(CodePtr+IP+(*SHR1)+sizeof(CpuWrd)); \
(*HPTR)=(CpuWrd)&&RunProgInstEndRestore; \
ADP3=(CpuAdr *)(CodePtr+IP+(*SHR2)); \
ADV3=*ADP3; \
if(ADV3<0 || ADV3>=_Glob.Length()-1){ MAXADR=_Glob.Length()-1; System::Throw(SysExceptionCode::InvalidMemoryAddress,"global memory",HEXFORMAT(ADV3),HEXFORMAT(MAXADR)); return; } \
(*ADP3)=(CpuAdr)reinterpret_cast<CpuAdr>(GlobPnt+ADV3-BP); \
DMOD3=CpuDecMode::GlobVar; \
DebugMessage(DebugLevel::VrmRuntime,"Decode argument 3 as "+CpuDecModeName(DMOD3)+" (orig="+HEXFORMAT(ADV3)+" stored="+HEXFORMAT(*ADP3)+")"); \
CONST_INSTEND_2_WW_VV;

//Instruction macro INST_DAGV4
#define INST_DAGV4 \
InstLabelDAGV4:; \
INSTDECODE_2_WW_VV; \
HPTR=(CpuWrd *)(CodePtr+IP+(*SHR1)); \
HICD=*(CpuIcd *)(CodePtr+IP+(*SHR1)+sizeof(CpuWrd)); \
(*HPTR)=(CpuWrd)&&RunProgInstEndRestore; \
ADP4=(CpuAdr *)(CodePtr+IP+(*SHR2)); \
ADV4=*ADP4; \
if(ADV4<0 || ADV4>=_Glob.Length()-1){ MAXADR=_Glob.Length()-1; System::Throw(SysExceptionCode::InvalidMemoryAddress,"global memory",HEXFORMAT(ADV4),HEXFORMAT(MAXADR)); return; } \
(*ADP4)=(CpuAdr)reinterpret_cast<CpuAdr>(GlobPnt+ADV4-BP); \
DMOD4=CpuDecMode::GlobVar; \
DebugMessage(DebugLevel::VrmRuntime,"Decode argument 4 as "+CpuDecModeName(DMOD4)+" (orig="+HEXFORMAT(ADV4)+" stored="+HEXFORMAT(*ADP4)+")"); \
CONST_INSTEND_2_WW_VV;

//Instruction macro DAGI1
#define INST_DAGI1 \
InstLabelDAGI1:; \
INSTDECODE_2_WW_VV; \
HPTR=(CpuWrd *)(CodePtr+IP+(*SHR1)); \
HICD=*(CpuIcd *)(CodePtr+IP+(*SHR1)+sizeof(CpuWrd)); \
(*HPTR)=(CpuWrd)&&RunProgInstEndRestore; \
ADP1=(CpuAdr *)(CodePtr+IP+(*SHR2)); \
ADV1=*ADP1; \
if(ADV1<0 || ADV1>=_Glob.Length()-1){ MAXADR=_Glob.Length()-1; System::Throw(SysExceptionCode::InvalidMemoryAddress,"global memory",HEXFORMAT(ADV1),HEXFORMAT(MAXADR)); return; } \
DRF1=*(CpuRef *)(GlobPnt+ADV1); \
REFINDIRECTION(DRF1,TPTR,DSO1); \
(*ADP1)=(CpuAdr)reinterpret_cast<CpuAdr>(TPTR-BP); \
DMOD1=CpuDecMode::GlobInd; \
DebugMessage(DebugLevel::VrmRuntime,"Decode argument 1 as "+CpuDecModeName(DMOD1)); \
CONST_INSTEND_2_WW_VV; 

//Instruction macro DAGI2
#define INST_DAGI2 \
InstLabelDAGI2:; \
INSTDECODE_2_WW_VV; \
HPTR=(CpuWrd *)(CodePtr+IP+(*SHR1)); \
HICD=*(CpuIcd *)(CodePtr+IP+(*SHR1)+sizeof(CpuWrd)); \
(*HPTR)=(CpuWrd)&&RunProgInstEndRestore; \
ADP2=(CpuAdr *)(CodePtr+IP+(*SHR2)); \
ADV2=*ADP2; \
if(ADV2<0 || ADV2>=_Glob.Length()-1){ MAXADR=_Glob.Length()-1; System::Throw(SysExceptionCode::InvalidMemoryAddress,"global memory",HEXFORMAT(ADV2),HEXFORMAT(MAXADR)); return; } \
DRF2=*(CpuRef *)(GlobPnt+ADV2); \
REFINDIRECTION(DRF2,TPTR,DSO2); \
(*ADP2)=(CpuAdr)reinterpret_cast<CpuAdr>(TPTR-BP); \
DMOD2=CpuDecMode::GlobInd; \
DebugMessage(DebugLevel::VrmRuntime,"Decode argument 2 as "+CpuDecModeName(DMOD2)); \
CONST_INSTEND_2_WW_VV; 

//Instruction macro DAGI3
#define INST_DAGI3 \
InstLabelDAGI3:; \
INSTDECODE_2_WW_VV; \
HPTR=(CpuWrd *)(CodePtr+IP+(*SHR1)); \
HICD=*(CpuIcd *)(CodePtr+IP+(*SHR1)+sizeof(CpuWrd)); \
(*HPTR)=(CpuWrd)&&RunProgInstEndRestore; \
ADP3=(CpuAdr *)(CodePtr+IP+(*SHR2)); \
ADV3=*ADP3; \
if(ADV3<0 || ADV3>=_Glob.Length()-1){ MAXADR=_Glob.Length()-1; System::Throw(SysExceptionCode::InvalidMemoryAddress,"global memory",HEXFORMAT(ADV3),HEXFORMAT(MAXADR)); return; } \
DRF3=*(CpuRef *)(GlobPnt+ADV3); \
REFINDIRECTION(DRF3,TPTR,DSO3); \
(*ADP3)=(CpuAdr)reinterpret_cast<CpuAdr>(TPTR-BP); \
DMOD3=CpuDecMode::GlobInd; \
DebugMessage(DebugLevel::VrmRuntime,"Decode argument 3 as "+CpuDecModeName(DMOD3)); \
CONST_INSTEND_2_WW_VV; 

//Instruction macro DAGI4
#define INST_DAGI4 \
InstLabelDAGI4:; \
INSTDECODE_2_WW_VV; \
HPTR=(CpuWrd *)(CodePtr+IP+(*SHR1)); \
HICD=*(CpuIcd *)(CodePtr+IP+(*SHR1)+sizeof(CpuWrd)); \
(*HPTR)=(CpuWrd)&&RunProgInstEndRestore; \
ADP4=(CpuAdr *)(CodePtr+IP+(*SHR2)); \
ADV4=*ADP4; \
if(ADV4<0 || ADV4>=_Glob.Length()-1){ MAXADR=_Glob.Length()-1; System::Throw(SysExceptionCode::InvalidMemoryAddress,"global memory",HEXFORMAT(ADV4),HEXFORMAT(MAXADR)); return; } \
DRF4=*(CpuRef *)(GlobPnt+ADV4); \
REFINDIRECTION(DRF4,TPTR,DSO4); \
(*ADP4)=(CpuAdr)reinterpret_cast<CpuAdr>(TPTR-BP); \
DMOD4=CpuDecMode::GlobInd; \
DebugMessage(DebugLevel::VrmRuntime,"Decode argument 4 as "+CpuDecModeName(DMOD4)); \
CONST_INSTEND_2_WW_VV; 

//Instruction macro DALI1
#define INST_DALI1 \
InstLabelDALI1:; \
INSTDECODE_2_WW_VV; \
HPTR=(CpuWrd *)(CodePtr+IP+(*SHR1)); \
HICD=*(CpuIcd *)(CodePtr+IP+(*SHR1)+sizeof(CpuWrd)); \
(*HPTR)=(CpuWrd)&&RunProgInstEndRestore; \
ADP1=(CpuAdr *)(CodePtr+IP+(*SHR2)); \
ADV1=*ADP1; \
if(ADV1<0 || ADV1+BP>=_Stack.Length()-1){ MAXADR=_Stack.Length()-1; System::Throw(SysExceptionCode::InvalidMemoryAddress,"stack memory",HEXFORMAT(ADV1),HEXFORMAT(MAXADR)); return; } \
DRF1=*(CpuRef *)(StackPnt+BP+ADV1); \
REFINDIRECTION(DRF1,TPTR,DSO1); \
(*ADP1)=(CpuAdr)reinterpret_cast<CpuAdr>(TPTR-BP); \
DMOD1=CpuDecMode::LoclInd; \
DebugMessage(DebugLevel::VrmRuntime,"Decode argument 1 as "+CpuDecModeName(DMOD1)); \
CONST_INSTEND_2_WW_VV; 

//Instruction macro DALI2
#define INST_DALI2 \
InstLabelDALI2:; \
INSTDECODE_2_WW_VV; \
HPTR=(CpuWrd *)(CodePtr+IP+(*SHR1)); \
HICD=*(CpuIcd *)(CodePtr+IP+(*SHR1)+sizeof(CpuWrd)); \
(*HPTR)=(CpuWrd)&&RunProgInstEndRestore; \
ADP2=(CpuAdr *)(CodePtr+IP+(*SHR2)); \
ADV2=*ADP2; \
if(ADV2<0 || ADV2+BP>=_Stack.Length()-1){ MAXADR=_Stack.Length()-1; System::Throw(SysExceptionCode::InvalidMemoryAddress,"stack memory",HEXFORMAT(ADV2),HEXFORMAT(MAXADR)); return; } \
DRF2=*(CpuRef *)(StackPnt+BP+ADV2); \
REFINDIRECTION(DRF2,TPTR,DSO2); \
(*ADP2)=(CpuAdr)reinterpret_cast<CpuAdr>(TPTR-BP); \
DMOD2=CpuDecMode::LoclInd; \
DebugMessage(DebugLevel::VrmRuntime,"Decode argument 2 as "+CpuDecModeName(DMOD2)); \
CONST_INSTEND_2_WW_VV; 

//Instruction macro DALI3
#define INST_DALI3 \
InstLabelDALI3:; \
INSTDECODE_2_WW_VV; \
HPTR=(CpuWrd *)(CodePtr+IP+(*SHR1)); \
HICD=*(CpuIcd *)(CodePtr+IP+(*SHR1)+sizeof(CpuWrd)); \
(*HPTR)=(CpuWrd)&&RunProgInstEndRestore; \
ADP3=(CpuAdr *)(CodePtr+IP+(*SHR2)); \
ADV3=*ADP3; \
if(ADV3<0 || ADV3+BP>=_Stack.Length()-1){ MAXADR=_Stack.Length()-1; System::Throw(SysExceptionCode::InvalidMemoryAddress,"stack memory",HEXFORMAT(ADV3),HEXFORMAT(MAXADR)); return; } \
DRF3=*(CpuRef *)(StackPnt+BP+ADV3); \
REFINDIRECTION(DRF3,TPTR,DSO3); \
(*ADP3)=(CpuAdr)reinterpret_cast<CpuAdr>(TPTR-BP); \
DMOD3=CpuDecMode::LoclInd; \
DebugMessage(DebugLevel::VrmRuntime,"Decode argument 3 as "+CpuDecModeName(DMOD3)); \
CONST_INSTEND_2_WW_VV; 

//Instruction macro DALI4
#define INST_DALI4 \
InstLabelDALI4:; \
INSTDECODE_2_WW_VV; \
HPTR=(CpuWrd *)(CodePtr+IP+(*SHR1)); \
HICD=*(CpuIcd *)(CodePtr+IP+(*SHR1)+sizeof(CpuWrd)); \
(*HPTR)=(CpuWrd)&&RunProgInstEndRestore; \
ADP4=(CpuAdr *)(CodePtr+IP+(*SHR2)); \
ADV4=*ADP4; \
if(ADV4<0 || ADV4+BP>=_Stack.Length()-1){ MAXADR=_Stack.Length()-1; System::Throw(SysExceptionCode::InvalidMemoryAddress,"stack memory",HEXFORMAT(ADV4),HEXFORMAT(MAXADR)); return; } \
DRF4=*(CpuRef *)(StackPnt+BP+ADV4); \
REFINDIRECTION(DRF4,TPTR,DSO4); \
(*ADP4)=(CpuAdr)reinterpret_cast<CpuAdr>(TPTR-BP); \
DMOD4=CpuDecMode::LoclInd; \
DebugMessage(DebugLevel::VrmRuntime,"Decode argument 4 as "+CpuDecModeName(DMOD4)); \
CONST_INSTEND_2_WW_VV; 

//Instruction macro PUSHb
#define INST_PUSHb \
InstLabelPUSHb:; \
INSTDECODE_1_B_A; \
if(!_ParmSt.Append((const char *)BOL1,sizeof(CpuBol))){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(CpuBol))); EXCP_EXIT; } \
INSTEND_1_B_A;

//Intruction macro PUSHc
#define INST_PUSHc \
InstLabelPUSHc:; \
INSTDECODE_1_C_A; \
if(!_ParmSt.Append((const char *)CHR1,sizeof(CpuChr))){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(CpuChr))); EXCP_EXIT;} \
INSTEND_1_C_A;

//Intruction macro PUSHw
#define INST_PUSHw \
InstLabelPUSHw:; \
INSTDECODE_1_W_A; \
if(!_ParmSt.Append((const char *)SHR1,sizeof(CpuShr))){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(CpuShr))); EXCP_EXIT;} \
INSTEND_1_W_A;

//Intruction macro PUSHi
#define INST_PUSHi \
InstLabelPUSHi:; \
INSTDECODE_1_I_A; \
if(!_ParmSt.Append((const char *)INT1,sizeof(CpuInt))){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(CpuInt))); EXCP_EXIT;} \
INSTEND_1_I_A;

//Intruction macro PUSHl
#define INST_PUSHl \
InstLabelPUSHl:; \
INSTDECODE_1_L_A; \
if(!_ParmSt.Append((const char *)LON1,sizeof(CpuLon))){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(CpuLon))); EXCP_EXIT;} \
INSTEND_1_L_A;

//Intruction macro PUSHf
#define INST_PUSHf \
InstLabelPUSHf:; \
INSTDECODE_1_F_A; \
if(!_ParmSt.Append((const char *)FLO1,sizeof(CpuFlo))){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(CpuFlo))); EXCP_EXIT;} \
INSTEND_1_F_A;

//Intruction macro PUSHr
#define INST_PUSHr \
InstLabelPUSHr:; \
INSTDECODE_1_R_A; \
if(!_ParmSt.Append((const char *)REF1,sizeof(CpuRef))){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(CpuRef))); EXCP_EXIT;} \
INSTEND_1_R_A;

//Instruction macro REFPU
#define INST_REFPU \
InstLabelREFPU:;   \
  INSTDECODE_1_D_A;  \
  CpuRef Ref; \
  GET_ARG_AS_REFERENCE(1,Ref,AOFF_I) \
  if(!_ParmSt.Append((const char *)&Ref,sizeof(CpuRef))){  \
    System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(CpuRef))); \
    EXCP_EXIT;  \
  } \
  DebugMessage(DebugLevel::VrmRuntime,"Ref="+_ToStringCpuRef(Ref)); \
  INSTEND_1_D_A;

//Instruction macro LPUb
#define INST_LPUb \
InstLabelLPUb:; \
INSTDECODE_1_B_A; \
if(!_DlParm.Push((DlParmDef){})){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(DlParmDef))); EXCP_EXIT; } \
if(!_DlVPtr.Push((void *)BOL1)){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(void *))); EXCP_EXIT; } \
DebugMessage(DebugLevel::VrmRuntime,"DlVPtr["+ToString(_DlVPtr.Length()-1)+"] = "+HEXFORMAT(_DlVPtr[_DlVPtr.Length()-1])); \
INSTEND_1_B_A;

//Intruction macro LPUc
#define INST_LPUc \
InstLabelLPUc:; \
INSTDECODE_1_C_A; \
if(!_DlParm.Push((DlParmDef){})){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(DlParmDef))); EXCP_EXIT; } \
if(!_DlVPtr.Push((void *)CHR1)){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(void *))); EXCP_EXIT; } \
DebugMessage(DebugLevel::VrmRuntime,"DlVPtr["+ToString(_DlVPtr.Length()-1)+"] = "+HEXFORMAT(_DlVPtr[_DlVPtr.Length()-1])); \
INSTEND_1_C_A;

//Intruction macro LPUw
#define INST_LPUw \
InstLabelLPUw:; \
INSTDECODE_1_W_A; \
if(!_DlParm.Push((DlParmDef){})){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(DlParmDef))); EXCP_EXIT; } \
if(!_DlVPtr.Push((void *)SHR1)){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(void *))); EXCP_EXIT; } \
DebugMessage(DebugLevel::VrmRuntime,"DlVPtr["+ToString(_DlVPtr.Length()-1)+"] = "+HEXFORMAT(_DlVPtr[_DlVPtr.Length()-1])); \
INSTEND_1_W_A;

//Intruction macro LPUi
#define INST_LPUi \
InstLabelLPUi:; \
INSTDECODE_1_I_A; \
if(!_DlParm.Push((DlParmDef){})){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(DlParmDef))); EXCP_EXIT; } \
if(!_DlVPtr.Push((void *)INT1)){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(void *))); EXCP_EXIT; } \
DebugMessage(DebugLevel::VrmRuntime,"DlVPtr["+ToString(_DlVPtr.Length()-1)+"] = "+HEXFORMAT(_DlVPtr[_DlVPtr.Length()-1])); \
INSTEND_1_I_A;

//Intruction macro LPUl
#define INST_LPUl \
InstLabelLPUl:; \
INSTDECODE_1_L_A; \
if(!_DlParm.Push((DlParmDef){})){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(DlParmDef))); EXCP_EXIT; } \
if(!_DlVPtr.Push((void *)LON1)){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(void *))); EXCP_EXIT; } \
DebugMessage(DebugLevel::VrmRuntime,"DlVPtr["+ToString(_DlVPtr.Length()-1)+"] = "+HEXFORMAT(_DlVPtr[_DlVPtr.Length()-1])); \
INSTEND_1_L_A;

//Intruction macro LPUf
#define INST_LPUf \
InstLabelLPUf:; \
INSTDECODE_1_F_A; \
if(!_DlParm.Push((DlParmDef){})){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(DlParmDef))); EXCP_EXIT; } \
if(!_DlVPtr.Push((void *)FLO1)){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(void *))); EXCP_EXIT; } \
DebugMessage(DebugLevel::VrmRuntime,"DlVPtr["+ToString(_DlVPtr.Length()-1)+"] = "+HEXFORMAT(_DlVPtr[_DlVPtr.Length()-1])); \
INSTEND_1_F_A;

//Intruction macro LPUr
#define INST_LPUr \
InstLabelLPUr:; \
INSTDECODE_1_R_A; \
REFINDIRECTION(*REF1,VPTR,DSOZ); \
if(!_DlParm.Push((DlParmDef){})){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(DlParmDef))); EXCP_EXIT; } \
if(!_DlVPtr.Push(VPTR)){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(void *))); EXCP_EXIT; } \
DebugMessage(DebugLevel::VrmRuntime,"DlVPtr["+ToString(_DlVPtr.Length()-1)+"] = "+HEXFORMAT(_DlVPtr[_DlVPtr.Length()-1])); \
INSTEND_1_R_A;

//Intruction macro LPUSr
#define INST_LPUSr \
InstLabelLPUSr:; \
  \
  /*Decode instruction*/ \
  INSTDECODE_2_RB_AV; \
  \
  /*Get pointer to argument and do reference indirection to get block number*/ \
  REFINDIRECTION(*REF1,VPTR,DSOZ); \
  BLK=(CpuMbl *)VPTR; \
  \
  /*Check block number is valid*/ \
  if(!_Aux.IsValid(*BLK)){ System::Throw(SysExceptionCode::InvalidStringBlock,ToString(*BLK)); EXCP_EXIT; } \
  \
  /*Do block indirection*/\
  VPTR=_Aux.CharPtr(*BLK); \
  \
  /*Store pointer*/ \
  if(!_DlParm.Push((DlParmDef){.Update=(bool)(*BOL2?0:1),.IsString=1,.IsDynArray=0})){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(DlParmDef))); EXCP_EXIT; } \
  if(!_DlVPtr.Push((void*)nullptr)){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(void *))); EXCP_EXIT; } \
  \
  /*If string is constant store pointer to pointer*/ \
  if(*BOL2){ \
    _DlParm[_DlParm.Length()-1].CharPtr=(char *)VPTR; \
    _DlVPtr[_DlVPtr.Length()-1]=&_DlParm[_DlParm.Length()-1].CharPtr; \
  } \
  \
  /*If string is non constant store pointer to pointer and original block*/ \
  else{ \
    _DlParm[_DlParm.Length()-1].Blk=BLK; \
    _DlParm[_DlParm.Length()-1].CharPtr=(char *)VPTR; \
    _DlVPtr[_DlVPtr.Length()-1]=&_DlParm[_DlParm.Length()-1].CharPtr; \
  } \
  \
  /*Argument print*/ \
  DebugMessage(DebugLevel::VrmRuntime,"DlVPtr["+ToString(_DlVPtr.Length()-1)+"] = "+HEXFORMAT(_DlVPtr[_DlVPtr.Length()-1])); \
  \
  /*Instruction end*/ \
  INSTEND_2_RB_AV;

//Intruction macro LPADr
#define INST_LPADr \
InstLabelLPADr:; \
  \
  /*Decode instruction*/ \
  INSTDECODE_2_RB_AV; \
  \
  /*Get pointer to argument and do reference indirection to get block number*/ \
  REFINDIRECTION(*REF1,VPTR,DSOZ); \
  BLK=(CpuMbl *)VPTR; \
  \
  /*Check block number is valid*/ \
  if(!_Aux.IsValid(*BLK)){ System::Throw(SysExceptionCode::InvalidStringBlock,ToString(*BLK)); EXCP_EXIT; } \
  \
  /*Do block indirection*/\
  VPTR=_Aux.CharPtr(*BLK); \
  \
  /*Store pointer for constant arrays*/ \
  if(*BOL2){ \
    if(!_DlParm.Push((DlParmDef){.Update=0,.IsString=0,.IsDynArray=1})){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(DlParmDef))); EXCP_EXIT; } \
    _DlParm[_DlParm.Length()-1].CArray.ptr=VPTR; \
    _DlParm[_DlParm.Length()-1].CArray.len=(clong)_ArC.DynGetElements(*BLK); \
    if(!_DlVPtr.Push(&_DlParm[_DlParm.Length()-1].CArray)){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(void *))); EXCP_EXIT; } \
  } \
  \
  /*Store pointer for variable arrays*/ \
  else { \
    if(!_DlParm.Push((DlParmDef){.Update=1,.IsString=0,.IsDynArray=1})){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(DlParmDef))); EXCP_EXIT; } \
    _DlParm[_DlParm.Length()-1].ArrLen=(clong)_ArC.DynGetElements(*BLK); \
    _DlParm[_DlParm.Length()-1].DArray.ptr=VPTR; \
    _DlParm[_DlParm.Length()-1].DArray.len=&_DlParm[_DlParm.Length()-1].ArrLen; \
    if(!_DlVPtr.Push(&_DlParm[_DlParm.Length()-1].DArray)){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(void *))); EXCP_EXIT; } \
  } \
  \
  /*Argument print*/ \
  DebugMessage(DebugLevel::VrmRuntime,"DlVPtr["+ToString(_DlVPtr.Length()-1)+"] = "+HEXFORMAT(_DlVPtr[_DlVPtr.Length()-1])); \
  \
  /*Instruction end*/ \
  INSTEND_2_RB_AV;

//Intruction macro LPAFr
#define INST_LPAFr \
InstLabelLPAFr:; \
  \
  /*Decode instruction*/ \
  INSTDECODE_3_RBG_AVV; \
  \
  /*Get pointer to argument and do reference indirection to get block number*/ \
  REFINDIRECTION(*REF1,VPTR,DSOZ); \
  \
  /*Store pointer for constant arrays*/ \
  if(*BOL2){ \
    if(!_DlParm.Push((DlParmDef){})){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(DlParmDef))); EXCP_EXIT; } \
    _DlParm[_DlParm.Length()-1].CArray.ptr=VPTR; \
    _DlParm[_DlParm.Length()-1].CArray.len=(clong)_ArC.FixGetElements(*AGX3); \
    if(!_DlVPtr.Push(&_DlParm[_DlParm.Length()-1].CArray)){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(void *))); EXCP_EXIT; } \
  } \
  /*Store pointer for variable arrays*/ \
  else { \
    if(!_DlParm.Push((DlParmDef){})){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(DlParmDef))); EXCP_EXIT; } \
    _DlParm[_DlParm.Length()-1].FArray.ptr=VPTR; \
    _DlParm[_DlParm.Length()-1].FArray.len=(clong)_ArC.FixGetElements(*AGX3); \
    if(!_DlVPtr.Push(&_DlParm[_DlParm.Length()-1].FArray)){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(void *))); EXCP_EXIT; } \
  } \
  \
  /*Argument print*/ \
  DebugMessage(DebugLevel::VrmRuntime,"DlVPtr["+ToString(_DlVPtr.Length()-1)+"] = "+HEXFORMAT(_DlVPtr[_DlVPtr.Length()-1])); \
  \
  /*Instruction end*/ \
  INSTEND_3_RBG_AVV;

//Instruction macro LRPU
#define INST_LRPU \
InstLabelLRPU:;   \
  INSTDECODE_1_D_A;  \
  if(!_DlParm.Push((DlParmDef){})){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(DlParmDef))); EXCP_EXIT; } \
  _DlParm[_DlParm.Length()-1].VoidPtr=(void *)DAT1; \
  if(!_DlVPtr.Push((void *)&_DlParm[_DlParm.Length()-1].VoidPtr)){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(void *))); EXCP_EXIT; } \
  DebugMessage(DebugLevel::VrmRuntime,"DlVPtr["+ToString(_DlVPtr.Length()-1)+"] = "+HEXFORMAT(_DlVPtr[_DlVPtr.Length()-1])); \
  INSTEND_1_D_A;

//Instruction macro LRPUS
#define INST_LRPUS \
InstLabelLRPUS:;   \
  \
  /*Decode instruction*/ \
  INSTDECODE_2_DB_AV; \
  \
  /*Get block number*/ \
  BLK=(CpuMbl *)DAT1; \
  \
  /*Allocate string with 1 byte if it is not allocated*/ \
  if(!_Aux.IsValid(*BLK)){ \
    if(!_Aux.Alloc(_ScopeId,_ScopeNr,1,-1,BLK)){ \
      System::Throw(SysExceptionCode::StringAllocationError,ToString(1)); \
      EXCP_EXIT; \
    } \
  } \
  \
  /*Do block indirection*/\
  VPTR=_Aux.CharPtr(*BLK); \
  \
  /*Store pointer*/ \
  if(!_DlParm.Push((DlParmDef){.Update=(bool)(*BOL2?0:1),.IsString=1,.IsDynArray=0})){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(DlParmDef))); EXCP_EXIT; } \
  if(!_DlVPtr.Push((void *)nullptr)){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(void *))); EXCP_EXIT; } \
  \
  /*If string is constant store pointer to pointer*/ \
  if(*BOL2){ \
    _DlParm[_DlParm.Length()-1].CharPtr=(char *)VPTR; \
    _DlVPtr[_DlVPtr.Length()-1]=&_DlParm[_DlParm.Length()-1].CharPtr; \
  } \
  \
  /*If string is non constant store pointer to pointer to pointer and original block*/ \
  else{ \
    _DlParm[_DlParm.Length()-1].Blk=BLK; \
    _DlParm[_DlParm.Length()-1].CharPtr=(char *)VPTR; \
    _DlParm[_DlParm.Length()-1].CharPtr2=(char **)&_DlParm[_DlParm.Length()-1].CharPtr; \
    _DlVPtr[_DlVPtr.Length()-1]=&_DlParm[_DlParm.Length()-1].CharPtr2; \
  } \
  \
  /*Argument print*/ \
  DebugMessage(DebugLevel::VrmRuntime,"DlVPtr["+ToString(_DlVPtr.Length()-1)+"] = "+HEXFORMAT(_DlVPtr[_DlVPtr.Length()-1])); \
  \
  /*Instruction end*/ \
  INSTEND_2_DB_AV;

//Instruction macro LRPAD
#define INST_LRPAD \
InstLabelLRPAD:; \
  \
  /*Decode instruction*/ \
  INSTDECODE_2_DB_AV;  \
  \
  /*Get block number*/ \
  BLK=(CpuMbl *)DAT1; \
  \
  /*Check block number is valid*/ \
  if(!_Aux.IsValid(*BLK)){ System::Throw(SysExceptionCode::InvalidStringBlock,ToString(*BLK)); EXCP_EXIT; } \
  \
  /*Do block indirection*/\
  VPTR=_Aux.CharPtr(*BLK); \
  \
  /*Store pointer for constant arrays*/ \
  if(*BOL2){ \
    if(!_DlParm.Push((DlParmDef){.Update=0,.IsString=0,.IsDynArray=1})){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(DlParmDef))); EXCP_EXIT; } \
    _DlParm[_DlParm.Length()-1].CArray.ptr=VPTR; \
    _DlParm[_DlParm.Length()-1].CArray.len=(clong)_ArC.DynGetElements(*BLK); \
    if(!_DlVPtr.Push(&_DlParm[_DlParm.Length()-1].CArray)){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(void *))); EXCP_EXIT; } \
  } \
  \
  /*Store pointer for variable arrays*/ \
  else { \
    if(!_DlParm.Push((DlParmDef){.Update=1,.IsString=0,.IsDynArray=1})){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(DlParmDef))); EXCP_EXIT; } \
    _DlParm[_DlParm.Length()-1].ArrLen=(clong)_ArC.DynGetElements(*BLK); \
    _DlParm[_DlParm.Length()-1].DArray.ptr=VPTR; \
    _DlParm[_DlParm.Length()-1].DArray.len=&_DlParm[_DlParm.Length()-1].ArrLen; \
    if(!_DlVPtr.Push(&_DlParm[_DlParm.Length()-1].DArray)){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(void *))); EXCP_EXIT; } \
  } \
  \
  /*Argument print*/ \
  DebugMessage(DebugLevel::VrmRuntime,"DlVPtr["+ToString(_DlVPtr.Length()-1)+"] = "+HEXFORMAT(_DlVPtr[_DlVPtr.Length()-1])); \
  \
  /*Instruction end*/ \
  INSTEND_2_DB_AV;

//Instruction macro LRPAF
#define INST_LRPAF \
InstLabelLRPAF:;   \
  \
  /*Decode instruction*/ \
  INSTDECODE_3_DBG_AVV;  \
  \
  /*Store pointer for constant arrays*/ \
  if(*BOL2){ \
    if(!_DlParm.Push((DlParmDef){})){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(DlParmDef))); EXCP_EXIT; } \
    _DlParm[_DlParm.Length()-1].CArray.ptr=(void *)DAT1; \
    _DlParm[_DlParm.Length()-1].CArray.len=(clong)_ArC.FixGetElements(*AGX3); \
    if(!_DlVPtr.Push(&_DlParm[_DlParm.Length()-1].CArray)){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(void *))); EXCP_EXIT; } \
  } \
  /*Store pointer for variable arrays*/ \
  else { \
    if(!_DlParm.Push((DlParmDef){})){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(DlParmDef))); EXCP_EXIT; } \
    _DlParm[_DlParm.Length()-1].FArray.ptr=(void *)DAT1; \
    _DlParm[_DlParm.Length()-1].FArray.len=(clong)_ArC.FixGetElements(*AGX3); \
    if(!_DlVPtr.Push(&_DlParm[_DlParm.Length()-1].FArray)){ System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(void *))); EXCP_EXIT; } \
  } \
  \
  /*Argument print*/ \
  DebugMessage(DebugLevel::VrmRuntime,"DlVPtr["+ToString(_DlVPtr.Length()-1)+"] = "+HEXFORMAT(_DlVPtr[_DlVPtr.Length()-1])); \
  \
  /*Instruction end*/ \
  INSTEND_3_DBG_AVV;

//Instruction macro CALL
#define INST_CALL \
InstLabelCALL:;   \
  \
  /*Decode instruction*/ \
  INSTDECODE_1_A_V;  \
  \
  /*Save return address and base pointer*/ \
  if(!_CallSt.Push((CallStack){IP,(CpuAdr)(IP+AOFF_IA),BP,_Stack.Length(),_ScopeNr,_ArC.FixGetBP()})){ \
    System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(CallStack))); \
    EXCP_EXIT; \
  } \
  \
  /*Copy parameters into stack and calculate new base pointer*/ \
  if(_ParmSt.Length()!=0){ \
    if(!_Stack.Append((const char *)_ParmSt.Pnt(),_ParmSt.Length())){ \
      System::Throw(SysExceptionCode::StackOverflow,ToString(_ParmSt.Length())); \
      EXCP_EXIT; \
    } \
    if(StackPnt!=_Stack.Pnt()){ \
      if(!_DecodeLocalVariables(false,CodePtr,StackPnt,_Stack.Pnt())){ EXCP_EXIT; } \
      StackPnt=_Stack.Pnt(); \
    } \
  } \
  BP=_Stack.Length()-_ParmSt.Length(); \
  \
  /*Change istruction pointer, clear parameter stack*/ \
  IP=(*ADR1); \
  _ParmSt.Empty(); \
  \
  /*Change scope if not locked*/ \
  if(_ScopeUnlock){ \
    _ScopeId++; \
    _ScopeNr=(++CUMULSC); \
    if(_ScopeId==_GlobalScopeId){ \
      System::Throw(SysExceptionCode::SubroutineMaxNestingLevelReached,ToString(_GlobalScopeId)); \
      EXCP_EXIT; \
    } \
    _StC.SetScope(_ScopeId,_ScopeNr); \
    _ArC.DynSetScope(_ScopeId,_ScopeNr); \
  } \
  \
  /*Restore decoding*/ \
  RESTORE_HANDLER; \
  \
  /*Instruction end*/ \
  JMP_INSTEND_1_A_V;

//Instruction macro RET
#define INST_RET \
InstLabelRET:; \
   \
  /*Null decoder*/ \
  INSTDECODE_0;  \
  \
  /*Retrieve return address and base pointer*/ \
  if(!_CallSt.Pop(RETADR)){ \
    System::Throw(SysExceptionCode::CallStackUnderflow); \
    EXCP_EXIT; \
  } \
  \
  /*Free unused stack space*/ \
  if(!_Stack.Resize(RETADR.StackSize)){ \
    System::Throw(SysExceptionCode::StackUnderflow); \
    EXCP_EXIT; \
  } \
  if(StackPnt!=_Stack.Pnt()){ \
    if(!_DecodeLocalVariables(false,CodePtr,StackPnt,_Stack.Pnt())){ EXCP_EXIT; } \
    StackPnt=_Stack.Pnt(); \
  } \
  \
  /*Change pointers*/ \
  IP=RETADR.RetAddress; \
  BP=RETADR.BasePointer; \
  _ArC.FixSetBP(RETADR.AFBasePointer); \
  \
  /*Change scope if not locked*/ \
  if(_ScopeUnlock){ \
    _ScopeNr=RETADR.ScopeNr; \
    _ScopeId--; \
    _StC.SetScope(_ScopeId,_ScopeNr); \
    _ArC.DynSetScope(_ScopeId,_ScopeNr); \
  } \
  \
  /*Restore decoding*/ \
  RESTORE_HANDLER; \
  \
  /*Return*/ \
  JMP_INSTEND_0;

//Instruction macro CALLN
#define INST_CALLN \
InstLabelCALLN:;   \
  \
  /*Decode instruction*/ \
  INSTDECODE_1_A_V;  \
  \
  /*Save return address and base pointer*/ \
  if(!_CallSt.Push((CallStack){IP,(CpuAdr)(IP+AOFF_IA),BP,_Stack.Length(),_ScopeNr,_ArC.FixGetBP()})){ \
    System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(CallStack))); \
    EXCP_EXIT; \
  } \
  \
  /*Copy parameters into stack*/ \
  if(_ParmSt.Length()!=0){ \
    if(!_Stack.Append((const char *)_ParmSt.Pnt(),_ParmSt.Length())){ \
      System::Throw(SysExceptionCode::StackOverflow,ToString(_ParmSt.Length())); \
      EXCP_EXIT; \
    } \
    if(StackPnt!=_Stack.Pnt()){ \
      if(!_DecodeLocalVariables(false,CodePtr,StackPnt,_Stack.Pnt())){ EXCP_EXIT; } \
      StackPnt=_Stack.Pnt(); \
    } \
  } \
  \
  /*Change istruction pointer, clear parameter stack and increase scope counter*/ \
  IP=(*ADR1); \
  _ParmSt.Empty(); \
  \
  /*Change scope if not locked*/ \
  if(_ScopeUnlock){ \
    _ScopeId++; \
    _ScopeNr=(++CUMULSC); \
    if(_ScopeId==_GlobalScopeId){ \
      System::Throw(SysExceptionCode::SubroutineMaxNestingLevelReached,ToString(_GlobalScopeId)); \
      EXCP_EXIT; \
    } \
    _StC.SetScope(_ScopeId,_ScopeNr); \
    _ArC.DynSetScope(_ScopeId,_ScopeNr); \
  } \
  \
  /*Restore decoding*/ \
  RESTORE_HANDLER; \
  \
  /*Instruction end*/ \
  JMP_INSTEND_1_A_V;

//Instruction macro RETN
#define INST_RETN \
InstLabelRETN:;   \
   \
  /*Null decoder*/ \
  INSTDECODE_0;  \
  \
  /*Retrieve return address and base pointer*/ \
  if(!_CallSt.Pop(RETADR)){ \
    System::Throw(SysExceptionCode::CallStackUnderflow); \
    EXCP_EXIT; \
  } \
  \
  /*Change pointers and scope*/ \
  IP=RETADR.RetAddress; \
  \
  /*Change scope if not locked*/ \
  if(_ScopeUnlock){ \
    _ScopeNr=RETADR.ScopeNr; \
    _ScopeId--; \
    _StC.SetScope(_ScopeId,_ScopeNr); \
    _ArC.DynSetScope(_ScopeId,_ScopeNr); \
  } \
  \
  /*Restore decoding*/ \
  RESTORE_HANDLER; \
  \
  /*Return*/ \
  JMP_INSTEND_0;

//Instruction macro SULOK
#define INST_SULOK \
InstLabelSULOK:;   \
  INSTDECODE_0;  \
  _ScopeUnlock=true; \
  INSTEND_0;

//Instruction macro RPBEG
#define INST_RPBEG \
InstLabelRPBEG:;  \
  \
  /*Instruction decoder*/ \
  INSTDECODE_2_DD_AA; \
  \
  /*Set origin and destination for replication and clear replication rules*/ \
  _RpDestin=(char *)DAT1; \
  _RpSource=(char *)DAT2; \
  _RpRule.Empty(); \
  \
  /*Print output*/ \
  INSTEND_2_DD_AA;

//Instruction macro RPLOF
#define INST_RPLOF \
InstLabelRPLOF:;   \
  \
  /*Instruction decoder*/ \
  INSTDECODE_2_ZG_AV; \
  \
  /*Check inputs*/ \
  if(*WRD1<0){ \
    System::Throw(SysExceptionCode::ReplicationRuleNegative); \
    EXCP_EXIT; \
  } \
  \
  /*Store replication rule */ \
  if(!_RpRule.Push((ReplicRule){ReplicRuleMode::Fixed,*WRD1,*AGX2,0,0,0,0,0,nullptr,nullptr,false})){ \
    System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(ReplicRule))); \
    EXCP_EXIT; \
  } \
  \
  /*Instruction end*/ \
  INSTEND_2_ZG_AV;

//Instruction macro RPLOD
#define INST_RPLOD \
InstLabelRPLOD:;   \
  \
  /*Instruction decoder*/ \
  INSTDECODE_1_Z_A; \
  \
  /*Check inputs*/ \
  if(*WRD1<0){ \
    System::Throw(SysExceptionCode::ReplicationRuleNegative); \
    EXCP_EXIT; \
  } \
  \
  /*Store replication rule*/ \
  if(!_RpRule.Push((ReplicRule){ReplicRuleMode::Dynamic,*WRD1,0,0,0,0,0,0,nullptr,nullptr,false})){ \
    System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(ReplicRule))); \
    EXCP_EXIT; \
  } \
  \
  /*Instruction end*/ \
  INSTEND_1_Z_A;

//Instruction macro RPSTR
#define INST_RPSTR \
InstLabelRPSTR:;   \
  \
  /*Instruction decoder*/ \
  INSTDECODE_1_Z_V; \
   \
  /*Check inputs*/ \
  if(*WRD1<0){ \
    System::Throw(SysExceptionCode::ReplicationRuleNegative); \
    EXCP_EXIT; \
  } \
  \
  /*Launch inner block replication*/ \
  if(!_InnerBlockReplication(*WRD1,true,false)){ EXCP_EXIT; } \
  \
  /*Instruction end*/ \
  INSTEND_1_Z_V;

//Instruction macro RPARR
#define INST_RPARR \
InstLabelRPARR:;   \
  \
  /*Instruction decoder*/ \
  INSTDECODE_1_Z_V; \
  \
  /*Check inputs*/ \
  if(*WRD1<0){ \
    System::Throw(SysExceptionCode::ReplicationRuleNegative); \
    EXCP_EXIT; \
  } \
  \
  /*Launch inner block replication*/ \
  if(!_InnerBlockReplication(*WRD1,false,true)){ EXCP_EXIT; } \
  \
  /*Print output*/ \
  INSTEND_1_Z_V;

//Instruction macro RPEND
#define INST_RPEND \
InstLabelRPEND:;   \
  \
  /*Null decoder*/ \
  INSTDECODE_0;  \
  \
  /*Delete last replication rule*/ \
  ReplicRule Rule0; \
  if(!_RpRule.Pop(Rule0)){ \
    System::Throw(SysExceptionCode::ReplicationRuleInconsistent); \
    EXCP_EXIT; \
  } \
    \
  /*Return*/ \
  INSTEND_0;

//Instruction macro BIBEG
#define INST_BIBEG \
InstLabelBIBEG:;  \
  \
  /*Instruction decoder*/ \
  INSTDECODE_1_D_A; \
  \
  /*Set destination for initialization and clear replication rules*/ \
  _BiDestin=(char *)DAT1; \
  _BiRule.Empty(); \
  \
  /*Print output*/ \
  INSTEND_1_D_A;

//Instruction macro BILOF
#define INST_BILOF \
InstLabelBILOF:;   \
  \
  /*Instruction decoder*/ \
  INSTDECODE_2_ZG_AV; \
  \
  /*Check inputs*/ \
  if(*WRD1<0){ \
    System::Throw(SysExceptionCode::InitializationRuleNegative); \
    EXCP_EXIT; \
  } \
  \
  /*Store initialization rule */ \
  if(!_BiRule.Push((InitRule){*WRD1,*AGX2,0,0,0,0,nullptr,false})){ \
    System::Throw(SysExceptionCode::MemoryAllocationFailure,ToString((int)sizeof(InitRule))); \
    EXCP_EXIT; \
  } \
  \
  /*Instruction end*/ \
  INSTEND_2_ZG_AV;

//Instruction macro BISTR
#define INST_BISTR \
InstLabelBISTR:;   \
  \
  /*Instruction decoder*/ \
  INSTDECODE_1_Z_V; \
   \
  /*Check inputs*/ \
  if(*WRD1<0){ \
    System::Throw(SysExceptionCode::InitializationRuleNegative); \
    EXCP_EXIT; \
  } \
  \
  /*Launch inner block initialization*/ \
  if(!_InnerBlockInitialization(*WRD1,true,false,0,0)){ EXCP_EXIT; } \
  \
  /*Instruction end*/ \
  INSTEND_1_Z_V;

//Instruction macro BIARR
#define INST_BIARR \
InstLabelBIARR:;   \
  \
  /*Instruction decoder*/ \
  INSTDECODE_3_ZCZ_VVV; \
  \
  /*Check inputs*/ \
  if(*WRD1<0){ \
    System::Throw(SysExceptionCode::InitializationRuleNegative); \
    EXCP_EXIT; \
  } \
  \
  /*Launch inner block initialization*/ \
  if(!_InnerBlockInitialization(*WRD1,false,true,*CHR2,*WRD3)){ EXCP_EXIT; } \
  \
  /*Instruction end*/ \
  INSTEND_3_ZCZ_VVV;

//Instruction macro BIEND
#define INST_BIEND \
InstLabelBIEND:;   \
  \
  /*Null decoder*/ \
  INSTDECODE_0;  \
  \
  /*Delete last initialization rule*/ \
  InitRule Rule1; \
  if(!_BiRule.Pop(Rule1)){ \
    System::Throw(SysExceptionCode::InitializationRuleInconsistent); \
    EXCP_EXIT; \
  } \
    \
  /*Return*/ \
  INSTEND_0;

//Instruction macro SCALL
#define INST_SCALL \
InstLabelSCALL:;   \
  \
  /*Instruction decoder*/ \
  INSTDECODE_1_I_V; \
  \
  /*Init parameter stack pointer*/ \
  PST=0; \
  \
  /*Save system call (used for timming)*/ \
  SCNR=*INT1; \
  \
  /*Debug message*/ \
  DebugMessage(DebugLevel::VrmRuntime, "Entered system call "+ToString(SCNR)+" ("+SystemCallId((SystemCall)SCNR)+")"); \
  \
  /*Call system call handler*/ \
  goto *SysCallLabelPtr[SCNR]; \
  \
  /*System call will end here*/ \
  SystemCallEndLabel:; \
  \
  /*Reset parameter stack*/ \
  _ParmSt.Empty(); \
  \
  /*Instruction end*/ \
  INSTEND_1_I_V;

//Instruction macro LCALL
#define INST_LCALL \
InstLabelLCALL:;   \
  \
  /*Instruction decoder*/ \
  INSTDECODE_1_I_V; \
  \
  /*Init parameter stack pointer*/ \
  PST=0; \
  \
  /*Save call id (used for timming)*/ \
  LCNR=*INT1; \
  \
  /*Debug message*/ \
  DebugMessage(DebugLevel::VrmRuntime, "Entered dynamic library call "+ToString(LCNR)+" ("+_DlCallStr(LCNR)+")"); \
  \
  /*Enter dynamic library call handler*/ \
  _DlCallHandler(LCNR); \
  \
  /*Reset parameter stack*/ \
  _DlParm.Empty(); \
  _DlVPtr.Empty(); \
  \
  /*Instruction end*/ \
  INSTEND_1_I_V;

//systemcall<exit> void exit()
#define SYSCALL_PROGRAMEXIT \
SystemCallLabelProgramExit:; \
goto RunProgExit; \

//systemcall<panic> void panic(string message)
#define SYSCALL_PANIC \
SystemCallLabelPanic:; \
SCALLGETREFRINDIR(1,MBL,CpuMbl); \
if(!_Aux.IsValid(*MBL1)){ \
  System::Throw(SysExceptionCode::InvalidStringBlock,ToString(*MBL1)); \
  EXCP_EXIT; \
} \
System::Throw(SysExceptionCode::SystemPanic,_Aux.CharPtr(*MBL1)); \
EXCP_EXIT; \
goto SystemCallEndLabel; \

//systemcall<print> void print(string line)
#define SYSCALL_CONSOLEPRINT \
SystemCallLabelConsolePrint:; \
SCALLGETREFRINDIR(1,MBL,CpuMbl); \
if(!_Aux.IsValid(*MBL1)){ \
  System::Throw(SysExceptionCode::InvalidStringBlock,ToString(*MBL1)); \
  EXCP_EXIT; \
} \
_Stl->Console.Print(_Aux.CharPtr(*MBL1)); \
goto SystemCallEndLabel; \

//systemcall<println> void println(string line)
#define SYSCALL_CONSOLEPRINTLINE \
SystemCallLabelConsolePrintLine:; \
SCALLGETREFRINDIR(1,MBL,CpuMbl); \
if(!_Aux.IsValid(*MBL1)){ \
  System::Throw(SysExceptionCode::InvalidStringBlock,ToString(*MBL1)); \
  EXCP_EXIT; \
} \
_Stl->Console.PrintLine(_Aux.CharPtr(*MBL1)); \
goto SystemCallEndLabel; \

//systemcall<eprint> void eprint(string line)
#define SYSCALL_CONSOLEPRINTERROR \
SystemCallLabelConsolePrintError:; \
SCALLGETREFRINDIR(1,MBL,CpuMbl); \
if(!_Aux.IsValid(*MBL1)){ \
  System::Throw(SysExceptionCode::InvalidStringBlock,ToString(*MBL1));  \
  EXCP_EXIT; \
} \
_Stl->Console.PrintError(_Aux.CharPtr(*MBL1)); \
goto SystemCallEndLabel; \

//systemcall<println> void eprintln(string line)
#define SYSCALL_CONSOLEPRINTERRORLINE \
SystemCallLabelConsolePrintErrorLine:; \
SCALLGETREFRINDIR(1,MBL,CpuMbl); \
if(!_Aux.IsValid(*MBL1)){ \
  System::Throw(SysExceptionCode::InvalidStringBlock,ToString(*MBL1));  \
  EXCP_EXIT; \
} \
_Stl->Console.PrintErrorLine(_Aux.CharPtr(*MBL1)); \
goto SystemCallEndLabel; \

//systemcall<getfilename> string getfilename(string path)
#define SYSCALL_GETFILENAME \
SystemCallLabelGetFileName:; \
SCALLGETREFRINDIR(1,MBL,CpuMbl); \
SCALLGETREFRINDIR(2,MBL,CpuMbl); \
if(!_Aux.IsValid(*MBL2)){ \
  System::Throw(SysExceptionCode::InvalidStringBlock,ToString(*MBL2));  \
  EXCP_EXIT; \
} \
STR=_Stl->FileSystem.GetFileName(_Aux.CharPtr(*MBL2)); \
_StC.SCOPY(MBL1,STR.CharPnt(),STR.Length()); \
SCALLOUTPARAMETER(1,MBL,CpuMbl); \
goto SystemCallEndLabel; \

//systemcall<getfilenamenoext> string getfilenamenoext(string path)
#define SYSCALL_GETFILENAMENOEXT \
SystemCallLabelGetFileNameNoExt:; \
SCALLGETREFRINDIR(1,MBL,CpuMbl); \
SCALLGETREFRINDIR(2,MBL,CpuMbl); \
if(!_Aux.IsValid(*MBL2)){ \
  System::Throw(SysExceptionCode::InvalidStringBlock,ToString(*MBL2));  \
  EXCP_EXIT; \
} \
STR=_Stl->FileSystem.GetFileNameNoExt(_Aux.CharPtr(*MBL2)); \
_StC.SCOPY(MBL1,STR.CharPnt(),STR.Length()); \
SCALLOUTPARAMETER(1,MBL,CpuMbl); \
goto SystemCallEndLabel; \

//systemcall<getfileextension> string getfileextension(string path)
#define SYSCALL_GETFILEEXTENSION \
SystemCallLabelGetFileExtension:; \
SCALLGETREFRINDIR(1,MBL,CpuMbl); \
SCALLGETREFRINDIR(2,MBL,CpuMbl); \
if(!_Aux.IsValid(*MBL2)){ \
  System::Throw(SysExceptionCode::InvalidStringBlock,ToString(*MBL2));  \
  EXCP_EXIT; \
} \
STR=_Stl->FileSystem.GetFileExtension(_Aux.CharPtr(*MBL2)); \
_StC.SCOPY(MBL1,STR.CharPnt(),STR.Length()); \
SCALLOUTPARAMETER(1,MBL,CpuMbl); \
goto SystemCallEndLabel; \

//systemcall<getdirname> string getfoldername(string path)
#define SYSCALL_GETDIRNAME \
SystemCallLabelGetDirName:; \
SCALLGETREFRINDIR(1,MBL,CpuMbl); \
SCALLGETREFRINDIR(2,MBL,CpuMbl); \
if(!_Aux.IsValid(*MBL2)){ \
  System::Throw(SysExceptionCode::InvalidStringBlock,ToString(*MBL2));  \
  EXCP_EXIT; \
} \
STR=_Stl->FileSystem.GetDirName(_Aux.CharPtr(*MBL2)); \
_StC.SCOPY(MBL1,STR.CharPnt(),STR.Length()); \
SCALLOUTPARAMETER(1,MBL,CpuMbl); \
goto SystemCallEndLabel; \

//systemcall<fileexists> bool fileexists(string path)
#define SYSCALL_FILEEXISTS \
SystemCallLabelFileExists:; \
SCALLGETREFRINDIR(1,BOL,CpuBol); \
SCALLGETREFRINDIR(2,MBL,CpuMbl); \
if(!_Aux.IsValid(*MBL2)){ \
  System::Throw(SysExceptionCode::InvalidStringBlock,ToString(*MBL2));  \
  EXCP_EXIT; \
} \
*BOL1=_Stl->FileSystem.FileExists(String(_Aux.CharPtr(*MBL2))); \
SCALLOUTPARAMETER(1,BOL,CpuBol); \
goto SystemCallEndLabel; \

//systemcall<direxists> bool direxists(string path)
#define SYSCALL_DIREXISTS \
SystemCallLabelDirExists:; \
SCALLGETREFRINDIR(1,BOL,CpuBol); \
SCALLGETREFRINDIR(2,MBL,CpuMbl); \
if(!_Aux.IsValid(*MBL2)){ \
  System::Throw(SysExceptionCode::InvalidStringBlock,ToString(*MBL2));  \
  EXCP_EXIT; \
} \
*BOL1=_Stl->FileSystem.DirExists(String(_Aux.CharPtr(*MBL2))); \
SCALLOUTPARAMETER(1,BOL,CpuBol); \
goto SystemCallEndLabel; \

//systemcall<gethandler> bool newhnd(ref int hnd)
#define SYSCALL_GETHANDLER \
SystemCallLabelGetHandler:; \
int Handler; \
SCALLGETREFRINDIR(1,BOL,CpuBol); \
SCALLGETREFRINDIR(2,INT,CpuInt); \
*BOL1=_Stl->FileSystem.GetHandler(Handler); \
*INT2=Handler; \
SCALLOUTPARAMETER(1,BOL,CpuBol); \
goto SystemCallEndLabel; \

//systemcall<freehandler> void freehnd(int hnd)
#define SYSCALL_FREEHANDLER \
SystemCallLabelFreeHandler:; \
SCALLGETPARAMETER(1,INT,CpuInt); \
_Stl->FileSystem.FreeHandler((int)*INT1); \
goto SystemCallEndLabel; \

//syscall<getfilesize> bool getfilesize(int hnd,ref long size)
#define SYSCALL_GETFILESIZE \
SystemCallLabelGetFileSize:; \
SCALLGETREFRINDIR(1,BOL,CpuBol); \
SCALLGETPARAMETER(2,INT,CpuInt); \
SCALLGETREFRINDIR(3,LON,CpuLon); \
*BOL1=_Stl->FileSystem.GetFileSize((int)*INT2,*LON3); \
SCALLOUTPARAMETER(1,BOL,CpuBol); \
SCALLOUTPARAMETER(3,LON,CpuLon); \
goto SystemCallEndLabel; \

//systemcall<openforread> bool openread(int hnd,string path)
#define SYSCALL_OPENFORREAD \
SystemCallLabelOpenForRead:; \
SCALLGETREFRINDIR(1,BOL,CpuBol); \
SCALLGETPARAMETER(2,INT,CpuInt); \
SCALLGETREFRINDIR(3,MBL,CpuMbl); \
if(!_Aux.IsValid(*MBL3)){ \
  System::Throw(SysExceptionCode::InvalidStringBlock,ToString(*MBL3));  \
  EXCP_EXIT; \
} \
*BOL1=_Stl->FileSystem.OpenForRead((int)*INT2,String(_Aux.CharPtr(*MBL3))); \
SCALLOUTPARAMETER(1,BOL,CpuBol); \
goto SystemCallEndLabel; \

//systemcall<openforwrite> bool openwrite(int hnd,string path)
#define SYSCALL_OPENFORWRITE \
SystemCallLabelOpenForWrite:; \
SCALLGETREFRINDIR(1,BOL,CpuBol); \
SCALLGETPARAMETER(2,INT,CpuInt); \
SCALLGETREFRINDIR(3,MBL,CpuMbl); \
if(!_Aux.IsValid(*MBL3)){ \
  System::Throw(SysExceptionCode::InvalidStringBlock,ToString(*MBL3));  \
  EXCP_EXIT; \
} \
*BOL1=_Stl->FileSystem.OpenForWrite((int)*INT2,String(_Aux.CharPtr(*MBL3))); \
SCALLOUTPARAMETER(1,BOL,CpuBol); \
goto SystemCallEndLabel; \

//systemcall<openforappend> bool openappend(int hnd,string path)
#define SYSCALL_OPENFORAPPEND \
SystemCallLabelOpenForAppend:; \
SCALLGETREFRINDIR(1,BOL,CpuBol); \
SCALLGETPARAMETER(2,INT,CpuInt); \
SCALLGETREFRINDIR(3,MBL,CpuMbl); \
if(!_Aux.IsValid(*MBL3)){ \
  System::Throw(SysExceptionCode::InvalidStringBlock,ToString(*MBL3));  \
  EXCP_EXIT; \
} \
*BOL1=_Stl->FileSystem.OpenForAppend((int)*INT2,String(_Aux.CharPtr(*MBL3))); \
SCALLOUTPARAMETER(1,BOL,CpuBol); \
goto SystemCallEndLabel; \

//systemcall<read> bool read(int hnd,ref char[] buffer,long length)    
#define SYSCALL_READ \
SystemCallLabelRead:; \
SCALLGETREFRINDIR(1,BOL,CpuBol); \
SCALLGETPARAMETER(2,INT,CpuInt); \
SCALLGETREFRINDIR(3,MBL,CpuMbl); \
SCALLGETPARAMETER(4,LON,CpuLon); \
_ArC.RDCH(BOL1,*INT2,MBL3,*LON4); \
SCALLOUTPARAMETER(1,BOL,CpuBol); \
SCALLOUTPARAMETER(3,MBL,CpuMbl); \
goto SystemCallEndLabel; \

//systemcall<write> bool write(int hnd,char[] buffer,long length)
#define SYSCALL_WRITE \
SystemCallLabelWrite:; \
SCALLGETREFRINDIR(1,BOL,CpuBol); \
SCALLGETPARAMETER(2,INT,CpuInt); \
SCALLGETREFRINDIR(3,MBL,CpuMbl); \
SCALLGETPARAMETER(4,LON,CpuLon); \
_ArC.WRCH(BOL1,*INT2,*MBL3,*LON4); \
SCALLOUTPARAMETER(1,BOL,CpuBol); \
goto SystemCallEndLabel; \

//systemcall<readall> bool read(int hnd,ref char[] buffer)
#define SYSCALL_READALL \
SystemCallLabelReadAll:; \
SCALLGETREFRINDIR(1,BOL,CpuBol); \
SCALLGETPARAMETER(2,INT,CpuInt); \
SCALLGETREFRINDIR(3,MBL,CpuMbl); \
_ArC.RDALCH(BOL1,*INT2,MBL3); \
SCALLOUTPARAMETER(1,BOL,CpuBol); \
SCALLOUTPARAMETER(3,MBL,CpuMbl); \
goto SystemCallEndLabel; \

//systemcall<writeall> bool write(int hnd,char[] buffer)
#define SYSCALL_WRITEALL \
SystemCallLabelWriteAll:; \
SCALLGETREFRINDIR(1,BOL,CpuBol); \
SCALLGETPARAMETER(2,INT,CpuInt); \
SCALLGETREFRINDIR(3,MBL,CpuMbl); \
_ArC.WRALCH(BOL1,*INT2,*MBL3); \
SCALLOUTPARAMETER(1,BOL,CpuBol); \
goto SystemCallEndLabel; \

//systemcall<readstr> bool read(int hnd,ref string line)
#define SYSCALL_READSTR \
SystemCallLabelReadStr:; \
SCALLGETREFRINDIR(1,BOL,CpuBol); \
SCALLGETPARAMETER(2,INT,CpuInt); \
SCALLGETREFRINDIR(3,MBL,CpuMbl); \
if(!_Aux.IsValid(*MBL3)){ \
  System::Throw(SysExceptionCode::InvalidStringBlock,ToString(*MBL3));  \
  EXCP_EXIT; \
} \
_Stl->ClearError(); \
if(!_Stl->FileSystem.Read((int)*INT2,STR)){  \
  if(_Stl->Error()!=StlErrorCode::Ok){ *BOL1=false; EXCP_EXIT; } \
} \
if(!_StC.SCOPY(MBL3,STR.CharPnt(),STR.Length())){ *BOL1=false; EXCP_EXIT; } \
*BOL1=true; \
SCALLOUTPARAMETER(1,BOL,CpuBol); \
SCALLOUTPARAMETER(3,MBL,CpuMbl); \
goto SystemCallEndLabel; \

//systemcall<writestr> bool write(int hnd,string line)
#define SYSCALL_WRITESTR \
SystemCallLabelWriteStr:; \
SCALLGETREFRINDIR(1,BOL,CpuBol); \
SCALLGETPARAMETER(2,INT,CpuInt); \
SCALLGETREFRINDIR(3,MBL,CpuMbl); \
if(!_Aux.IsValid(*MBL3)){ \
  System::Throw(SysExceptionCode::InvalidStringBlock,ToString(*MBL3));  \
  EXCP_EXIT; \
} \
*BOL1=_Stl->FileSystem.Write((int)*INT2,String(_Aux.CharPtr(*MBL3))); \
SCALLOUTPARAMETER(1,BOL,CpuBol); \
goto SystemCallEndLabel; \

//systemcall<readstrall> bool read(int hnd,ref string[] line)
#define SYSCALL_READSTRALL \
SystemCallLabelReadStrAll:; \
SCALLGETREFRINDIR(1,BOL,CpuBol); \
SCALLGETPARAMETER(2,INT,CpuInt); \
SCALLGETREFRINDIR(3,MBL,CpuMbl); \
_ArC.RDALST(BOL1,*INT2,MBL3); \
SCALLOUTPARAMETER(1,BOL,CpuBol); \
SCALLOUTPARAMETER(3,MBL,CpuMbl); \
goto SystemCallEndLabel; \

//systemcall<writestrall> bool write(int hnd,string[] line)
#define SYSCALL_WRITESTRALL \
SystemCallLabelWriteStrAll:; \
SCALLGETREFRINDIR(1,BOL,CpuBol); \
SCALLGETPARAMETER(2,INT,CpuInt); \
SCALLGETREFRINDIR(3,MBL,CpuMbl); \
_ArC.WRALST(BOL1,*INT2,*MBL3); \
SCALLOUTPARAMETER(1,BOL,CpuBol); \
goto SystemCallEndLabel; \

//systemcall<closefile> void closefile(int hnd)
#define SYSCALL_CLOSEFILE \
SystemCallLabelCloseFile:; \
SCALLGETREFRINDIR(1,BOL,CpuBol); \
SCALLGETPARAMETER(2,INT,CpuInt); \
*BOL1=_Stl->FileSystem.CloseFile((int)*INT2); \
goto SystemCallEndLabel; \

//systemcall<hnd2file> string hnd2file(int hnd)
#define SYSCALL_HND2FILE \
SystemCallLabelHnd2File:; \
SCALLGETREFRINDIR(1,MBL,CpuMbl); \
SCALLGETPARAMETER(2,INT,CpuInt); \
STR=_Stl->FileSystem.Hnd2File((int)*INT2); \
_StC.SCOPY(MBL1,STR.CharPnt(),STR.Length()); \
SCALLOUTPARAMETER(1,MBL,CpuMbl); \
goto SystemCallEndLabel; \

//systemcall<file2hnd> int file2hnd(string filename)
#define SYSCALL_FILE2HND \
SystemCallLabelFile2Hnd:; \
SCALLGETREFRINDIR(1,INT,CpuInt); \
SCALLGETREFRINDIR(2,MBL,CpuMbl); \
if(!_Aux.IsValid(*MBL2)){ \
  System::Throw(SysExceptionCode::InvalidStringBlock,ToString(*MBL2));  \
  EXCP_EXIT; \
} \
*INT1=_Stl->FileSystem.File2Hnd(String(_Aux.CharPtr(*MBL2))); \
SCALLOUTPARAMETER(1,INT,CpuInt); \
goto SystemCallEndLabel; \

//systemcall<delay> void delay(int millisecs)
#define SYSCALL_DELAY \
SystemCallLabelDelay:; \
SCALLGETPARAMETER(1,INT,CpuInt); \
_Stl->Delay(*INT1); \
goto SystemCallEndLabel; \

//systemcall<execute1> bool execute(string execfile,string arg,ref string[] output,ref string[] stderr,bool redirect)
#define SYSCALL_EXECUTE1 \
SystemCallLabelExecute1:; \
SCALLGETREFRINDIR(1,BOL,CpuBol); \
SCALLGETREFRINDIR(2,MBL,CpuMbl); \
SCALLGETREFRINDIR(3,MBL,CpuMbl); \
SCALLGETREFRINDIR(4,MBL,CpuMbl); \
SCALLGETREFRINDIR(5,MBL,CpuMbl); \
SCALLGETPARAMETER(6,BOL,CpuBol); \
if(!_Aux.IsValid(*MBL2)){ \
  System::Throw(SysExceptionCode::InvalidStringBlock,ToString(*MBL2));  \
  EXCP_EXIT; \
} \
if(!_Aux.IsValid(*MBL3)){ \
  System::Throw(SysExceptionCode::InvalidStringBlock,ToString(*MBL3));  \
  EXCP_EXIT; \
} \
*BOL1=_ExecuteExternal(*MBL2,*MBL3,MBL4,MBL5,*BOL6,false); \
SCALLOUTPARAMETER(1,BOL,CpuBol); \
SCALLOUTPARAMETER(4,MBL,CpuMbl); \
SCALLOUTPARAMETER(5,MBL,CpuMbl); \
goto SystemCallEndLabel; \

//systemcall<execute2> bool execute(string execfile,string[] arg,ref string[] output,ref string[] stderr,bool redirect)
#define SYSCALL_EXECUTE2 \
SystemCallLabelExecute2:; \
SCALLGETREFRINDIR(1,BOL,CpuBol); \
SCALLGETREFRINDIR(2,MBL,CpuMbl); \
SCALLGETREFRINDIR(3,MBL,CpuMbl); \
SCALLGETREFRINDIR(4,MBL,CpuMbl); \
SCALLGETREFRINDIR(5,MBL,CpuMbl); \
SCALLGETPARAMETER(6,BOL,CpuBol); \
if(!_Aux.IsValid(*MBL2)){ \
  System::Throw(SysExceptionCode::InvalidStringBlock,ToString(*MBL2));  \
  EXCP_EXIT; \
} \
*BOL1=_ExecuteExternal(*MBL2,*MBL3,MBL4,MBL5,*BOL6,true); \
SCALLOUTPARAMETER(1,BOL,CpuBol); \
SCALLOUTPARAMETER(4,MBL,CpuMbl); \
SCALLOUTPARAMETER(5,MBL,CpuMbl); \
goto SystemCallEndLabel; \

//systemcall<error> syserrorcode error()
#define SYSCALL_ERROR \
SystemCallLabelError:; \
SCALLGETREFRINDIR(1,INT,CpuInt); \
*INT1=(CpuInt)_Stl->Error(); \
SCALLOUTPARAMETER(1,INT,CpuInt); \
goto SystemCallEndLabel; \

//systemcall<errortext> string errortext(syserrorcode code)
#define SYSCALL_ERRORTEXT \
SystemCallLabelErrorText:; \
SCALLGETREFRINDIR(1,MBL,CpuMbl); \
SCALLGETPARAMETER(2,INT,CpuInt); \
STR=_Stl->ErrorText((StlErrorCode)*INT2); \
_StC.SCOPY(MBL1,STR.CharPnt(),STR.Length()); \
SCALLOUTPARAMETER(1,MBL,CpuMbl); \
goto SystemCallEndLabel; \

//systemcall<lasterror> string lasterror()
#define SYSCALL_LASTERROR \
SystemCallLabelLastError:; \
SCALLGETREFRINDIR(1,MBL,CpuMbl); \
STR=_Stl->LastError(); \
_StC.SCOPY(MBL1,STR.CharPnt(),STR.Length()); \
SCALLOUTPARAMETER(1,MBL,CpuMbl); \
goto SystemCallEndLabel; \

//systemcall<getarg> string[] arg()
#define SYSCALL_GETARG \
SystemCallLabelGetArg:; \
SCALLGETREFRINDIR(1,MBL,CpuMbl); \
_ArC.GETARG(MBL1,_ArgNr,_Arg,_ArgStart); \
SCALLOUTPARAMETER(1,MBL,CpuMbl); \
goto SystemCallEndLabel; \

//systemcall<hostsystem> int hostsystem()
#define SYSCALL_HOSTSYSTEM \
SystemCallLabelHostSystem:; \
SCALLGETREFRINDIR(1,INT,CpuInt); \
*INT1=(CpuInt)GetHostSystem(); \
SCALLOUTPARAMETER(1,INT,CpuInt); \
goto SystemCallEndLabel; \

//systemcall<abschr> char abs(char a)
#define SYSCALL_ABSCHR \
SystemCallLabelAbsChr:; \
SCALLGETREFRINDIR(1,CHR,CpuChr); \
SCALLGETPARAMETER(2,CHR,CpuChr); \
*CHR1=std::abs(*CHR2); \
SCALLOUTPARAMETER(1,CHR,CpuChr); \
goto SystemCallEndLabel; \

//systemcall<absshr> short abs(short a)
#define SYSCALL_ABSSHR \
SystemCallLabelAbsShr:; \
SCALLGETREFRINDIR(1,SHR,CpuShr); \
SCALLGETPARAMETER(2,SHR,CpuShr); \
*SHR1=std::abs(*SHR2); \
SCALLOUTPARAMETER(1,SHR,CpuShr); \
goto SystemCallEndLabel; \

//systemcall<absint> int abs(int a)
#define SYSCALL_ABSINT \
SystemCallLabelAbsInt:; \
SCALLGETREFRINDIR(1,INT,CpuInt); \
SCALLGETPARAMETER(2,INT,CpuInt); \
*INT1=std::abs(*INT2); \
SCALLOUTPARAMETER(1,INT,CpuInt); \
goto SystemCallEndLabel; \

//systemcall<abslon> long abs(long a)
#define SYSCALL_ABSLON \
SystemCallLabelAbsLon:; \
SCALLGETREFRINDIR(1,LON,CpuLon); \
SCALLGETPARAMETER(2,LON,CpuLon); \
*LON1=std::abs(*LON2); \
SCALLOUTPARAMETER(1,LON,CpuLon); \
goto SystemCallEndLabel; \

//systemcall<absflo> float abs(float a)
#define SYSCALL_ABSFLO \
SystemCallLabelAbsFlo:; \
SCALLGETREFRINDIR(1,FLO,CpuFlo); \
SCALLGETPARAMETER(2,FLO,CpuFlo); \
*FLO1=std::abs(*FLO2); \
SCALLOUTPARAMETER(1,FLO,CpuFlo); \
goto SystemCallEndLabel; \

//systemcall<minchr> char min(char a,char b)
#define SYSCALL_MINCHR \
SystemCallLabelMinChr:; \
SCALLGETREFRINDIR(1,CHR,CpuChr); \
SCALLGETPARAMETER(2,CHR,CpuChr); \
SCALLGETPARAMETER(3,CHR,CpuChr); \
*CHR1=(*CHR2<*CHR3?*CHR2:*CHR3); \
SCALLOUTPARAMETER(1,CHR,CpuChr); \
goto SystemCallEndLabel; \

//systemcall<minshr> short min(short a,short b)
#define SYSCALL_MINSHR \
SystemCallLabelMinShr:; \
SCALLGETREFRINDIR(1,SHR,CpuShr); \
SCALLGETPARAMETER(2,SHR,CpuShr); \
SCALLGETPARAMETER(3,SHR,CpuShr); \
*SHR1=(*SHR2<*SHR3?*SHR2:*SHR3); \
SCALLOUTPARAMETER(1,SHR,CpuShr); \
goto SystemCallEndLabel; \

//systemcall<minint> int min(int a,int b)
#define SYSCALL_MININT \
SystemCallLabelMinInt:; \
SCALLGETREFRINDIR(1,INT,CpuInt); \
SCALLGETPARAMETER(2,INT,CpuInt); \
SCALLGETPARAMETER(3,INT,CpuInt); \
*INT1=(*INT2<*INT3?*INT2:*INT3); \
SCALLOUTPARAMETER(1,INT,CpuInt); \
goto SystemCallEndLabel; \

//systemcall<minlon> long min(long a,long b)
#define SYSCALL_MINLON \
SystemCallLabelMinLon:; \
SCALLGETREFRINDIR(1,LON,CpuLon); \
SCALLGETPARAMETER(2,LON,CpuLon); \
SCALLGETPARAMETER(3,LON,CpuLon); \
*LON1=(*LON2<*LON3?*LON2:*LON3); \
SCALLOUTPARAMETER(1,LON,CpuLon); \
goto SystemCallEndLabel; \

//systemcall<minflo> float min(float a,float b)
#define SYSCALL_MINFLO \
SystemCallLabelMinFlo:; \
SCALLGETREFRINDIR(1,FLO,CpuFlo); \
SCALLGETPARAMETER(2,FLO,CpuFlo); \
SCALLGETPARAMETER(3,FLO,CpuFlo); \
*FLO1=(*FLO2<*FLO3?*FLO2:*FLO3); \
SCALLOUTPARAMETER(1,FLO,CpuFlo); \
goto SystemCallEndLabel; \

//systemcall<maxchr> char max(char a,char b)
#define SYSCALL_MAXCHR \
SystemCallLabelMaxChr:; \
SCALLGETREFRINDIR(1,CHR,CpuChr); \
SCALLGETPARAMETER(2,CHR,CpuChr); \
SCALLGETPARAMETER(3,CHR,CpuChr); \
*CHR1=(*CHR2>*CHR3?*CHR2:*CHR3); \
SCALLOUTPARAMETER(1,CHR,CpuChr); \
goto SystemCallEndLabel; \

//systemcall<maxshr> short max(short a,short b)
#define SYSCALL_MAXSHR \
SystemCallLabelMaxShr:; \
SCALLGETREFRINDIR(1,SHR,CpuShr); \
SCALLGETPARAMETER(2,SHR,CpuShr); \
SCALLGETPARAMETER(3,SHR,CpuShr); \
*SHR1=(*SHR2>*SHR3?*SHR2:*SHR3); \
SCALLOUTPARAMETER(1,SHR,CpuShr); \
goto SystemCallEndLabel; \

//systemcall<maxint> int max(int a,int b)
#define SYSCALL_MAXINT \
SystemCallLabelMaxInt:; \
SCALLGETREFRINDIR(1,INT,CpuInt); \
SCALLGETPARAMETER(2,INT,CpuInt); \
SCALLGETPARAMETER(3,INT,CpuInt); \
*INT1=(*INT2>*INT3?*INT2:*INT3); \
SCALLOUTPARAMETER(1,INT,CpuInt); \
goto SystemCallEndLabel; \

//systemcall<maxlon> long max(long a,long b)
#define SYSCALL_MAXLON \
SystemCallLabelMaxLon:; \
SCALLGETREFRINDIR(1,LON,CpuLon); \
SCALLGETPARAMETER(2,LON,CpuLon); \
SCALLGETPARAMETER(3,LON,CpuLon); \
*LON1=(*LON2>*LON3?*LON2:*LON3); \
SCALLOUTPARAMETER(1,LON,CpuLon); \
goto SystemCallEndLabel; \

//systemcall<maxflo> float max(float a,float b)
#define SYSCALL_MAXFLO \
SystemCallLabelMaxFlo:; \
SCALLGETREFRINDIR(1,FLO,CpuFlo); \
SCALLGETPARAMETER(2,FLO,CpuFlo); \
SCALLGETPARAMETER(3,FLO,CpuFlo); \
*FLO1=(*FLO2>*FLO3?*FLO2:*FLO3); \
SCALLOUTPARAMETER(1,FLO,CpuFlo); \
goto SystemCallEndLabel; \

//systemcall<exp> float exp(float x)
#define SYSCALL_EXP \
SystemCallLabelExp:; \
SCALLGETREFRINDIR(1,FLO,CpuFlo); \
SCALLGETPARAMETER(2,FLO,CpuFlo); \
*FLO1=std::exp(*FLO2); \
SCALLOUTPARAMETER(1,FLO,CpuFlo); \
goto SystemCallEndLabel; \

//systemcall<ln> float ln(float x)
#define SYSCALL_LN \
SystemCallLabelLn:; \
SCALLGETREFRINDIR(1,FLO,CpuFlo); \
SCALLGETPARAMETER(2,FLO,CpuFlo); \
*FLO1=std::log(*FLO2); \
SCALLOUTPARAMETER(1,FLO,CpuFlo); \
goto SystemCallEndLabel; \

//systemcall<log> float log(float x)
#define SYSCALL_LOG \
SystemCallLabelLog:; \
SCALLGETREFRINDIR(1,FLO,CpuFlo); \
SCALLGETPARAMETER(2,FLO,CpuFlo); \
*FLO1=std::log10(*FLO2); \
SCALLOUTPARAMETER(1,FLO,CpuFlo); \
goto SystemCallEndLabel; \

//systemcall<logn> float logn(float base,float number)
#define SYSCALL_LOGN \
SystemCallLabelLogn:; \
SCALLGETREFRINDIR(1,FLO,CpuFlo); \
SCALLGETPARAMETER(2,FLO,CpuFlo); \
SCALLGETPARAMETER(3,FLO,CpuFlo); \
*FLO1=std::log(*FLO2)/std::log(*FLO3); \
SCALLOUTPARAMETER(1,FLO,CpuFlo); \
goto SystemCallEndLabel; \

//systemcall<pow> float pow(float base,float exponent)
#define SYSCALL_POW \
SystemCallLabelPow:; \
SCALLGETREFRINDIR(1,FLO,CpuFlo); \
SCALLGETPARAMETER(2,FLO,CpuFlo); \
SCALLGETPARAMETER(3,FLO,CpuFlo); \
*FLO1=std::pow(*FLO2,*FLO3); \
SCALLOUTPARAMETER(1,FLO,CpuFlo); \
goto SystemCallEndLabel; \

//systemcall<sqrt> float sqrt(float x)
#define SYSCALL_SQRT \
SystemCallLabelSqrt:; \
SCALLGETREFRINDIR(1,FLO,CpuFlo); \
SCALLGETPARAMETER(2,FLO,CpuFlo); \
*FLO1=std::sqrt(*FLO2); \
SCALLOUTPARAMETER(1,FLO,CpuFlo); \
goto SystemCallEndLabel; \

//systemcall<cbrt> float cbrt(float x)
#define SYSCALL_CBRT \
SystemCallLabelCbrt:; \
SCALLGETREFRINDIR(1,FLO,CpuFlo); \
SCALLGETPARAMETER(2,FLO,CpuFlo); \
*FLO1=std::cbrt(*FLO2); \
SCALLOUTPARAMETER(1,FLO,CpuFlo); \
goto SystemCallEndLabel; \

//systemcall<sin> float sin(float x)
#define SYSCALL_SIN \
SystemCallLabelSin:; \
SCALLGETREFRINDIR(1,FLO,CpuFlo); \
SCALLGETPARAMETER(2,FLO,CpuFlo); \
*FLO1=std::sin(*FLO2); \
SCALLOUTPARAMETER(1,FLO,CpuFlo); \
goto SystemCallEndLabel; \

//systemcall<cos> float cos(float x)
#define SYSCALL_COS \
SystemCallLabelCos:; \
SCALLGETREFRINDIR(1,FLO,CpuFlo); \
SCALLGETPARAMETER(2,FLO,CpuFlo); \
*FLO1=std::cos(*FLO2); \
SCALLOUTPARAMETER(1,FLO,CpuFlo); \
goto SystemCallEndLabel; \

//systemcall<tan> float tan(float x)
#define SYSCALL_TAN \
SystemCallLabelTan:; \
SCALLGETREFRINDIR(1,FLO,CpuFlo); \
SCALLGETPARAMETER(2,FLO,CpuFlo); \
*FLO1=std::tan(*FLO2); \
SCALLOUTPARAMETER(1,FLO,CpuFlo); \
goto SystemCallEndLabel; \

//systemcall<asin> float asin(float x)
#define SYSCALL_ASIN \
SystemCallLabelAsin:; \
SCALLGETREFRINDIR(1,FLO,CpuFlo); \
SCALLGETPARAMETER(2,FLO,CpuFlo); \
*FLO1=std::asin(*FLO2); \
SCALLOUTPARAMETER(1,FLO,CpuFlo); \
SCALLOUTPARAMETER(1,FLO,CpuFlo); \
goto SystemCallEndLabel; \

//systemcall<acos> float acos(float x)
#define SYSCALL_ACOS \
SystemCallLabelAcos:; \
SCALLGETREFRINDIR(1,FLO,CpuFlo); \
SCALLGETPARAMETER(2,FLO,CpuFlo); \
*FLO1=std::acos(*FLO2); \
SCALLOUTPARAMETER(1,FLO,CpuFlo); \
goto SystemCallEndLabel; \

//systemcall<atan> float atan(float x)
#define SYSCALL_ATAN \
SystemCallLabelAtan:; \
SCALLGETREFRINDIR(1,FLO,CpuFlo); \
SCALLGETPARAMETER(2,FLO,CpuFlo); \
*FLO1=std::atan(*FLO2); \
SCALLOUTPARAMETER(1,FLO,CpuFlo); \
goto SystemCallEndLabel; \

//systemcall<sinh> float sinh(float x)
#define SYSCALL_SINH \
SystemCallLabelSinh:; \
SCALLGETREFRINDIR(1,FLO,CpuFlo); \
SCALLGETPARAMETER(2,FLO,CpuFlo); \
*FLO1=std::sinh(*FLO2); \
SCALLOUTPARAMETER(1,FLO,CpuFlo); \
goto SystemCallEndLabel; \

//systemcall<cosh> float cosh(float x)
#define SYSCALL_COSH \
SystemCallLabelCosh:; \
SCALLGETREFRINDIR(1,FLO,CpuFlo); \
SCALLGETPARAMETER(2,FLO,CpuFlo); \
*FLO1=std::cosh(*FLO2); \
SCALLOUTPARAMETER(1,FLO,CpuFlo); \
goto SystemCallEndLabel; \

//systemcall<tanh> float tanh(float x)
#define SYSCALL_TANH \
SystemCallLabelTanh:; \
SCALLGETREFRINDIR(1,FLO,CpuFlo); \
SCALLGETPARAMETER(2,FLO,CpuFlo); \
*FLO1=std::tanh(*FLO2); \
SCALLOUTPARAMETER(1,FLO,CpuFlo); \
goto SystemCallEndLabel; \

//systemcall<asinh> float asinh(float x)
#define SYSCALL_ASINH \
SystemCallLabelAsinh:; \
SCALLGETREFRINDIR(1,FLO,CpuFlo); \
SCALLGETPARAMETER(2,FLO,CpuFlo); \
*FLO1=std::asinh(*FLO2); \
SCALLOUTPARAMETER(1,FLO,CpuFlo); \
goto SystemCallEndLabel; \

//systemcall<acosh> float acosh(float x)
#define SYSCALL_ACOSH \
SystemCallLabelAcosh:; \
SCALLGETREFRINDIR(1,FLO,CpuFlo); \
SCALLGETPARAMETER(2,FLO,CpuFlo); \
*FLO1=std::acosh(*FLO2); \
SCALLOUTPARAMETER(1,FLO,CpuFlo); \
goto SystemCallEndLabel; \

//systemcall<atanh> float atanh(float x)
#define SYSCALL_ATANH \
SystemCallLabelAtanh:; \
SCALLGETREFRINDIR(1,FLO,CpuFlo); \
SCALLGETPARAMETER(2,FLO,CpuFlo); \
*FLO1=std::atanh(*FLO2); \
SCALLOUTPARAMETER(1,FLO,CpuFlo); \
goto SystemCallEndLabel; \

//systemcall<ceil> long ceil(float x)
#define SYSCALL_CEIL \
SystemCallLabelCeil:; \
SCALLGETREFRINDIR(1,LON,CpuLon); \
SCALLGETPARAMETER(2,FLO,CpuFlo); \
*LON1=(CpuLon)std::ceil(*FLO2); \
SCALLOUTPARAMETER(1,LON,CpuLon); \
goto SystemCallEndLabel; \

//systemcall<floor> long floor(float x)
#define SYSCALL_FLOOR \
SystemCallLabelFloor:; \
SCALLGETREFRINDIR(1,LON,CpuLon); \
SCALLGETPARAMETER(2,FLO,CpuFlo); \
*LON1=(CpuLon)std::floor(*FLO2); \
SCALLOUTPARAMETER(1,LON,CpuLon); \
goto SystemCallEndLabel; \

//systemcall<round> long round(float number,int decimals)
#define SYSCALL_ROUND \
SystemCallLabelRound:; \
SCALLGETREFRINDIR(1,LON,CpuLon); \
SCALLGETPARAMETER(2,FLO,CpuFlo); \
SCALLGETPARAMETER(3,INT,CpuInt); \
*LON1=(CpuLon)_Stl->Math.Round(*FLO2,*INT3); \
SCALLOUTPARAMETER(1,LON,CpuLon); \
goto SystemCallEndLabel; \

//systemcall<seed> void seed(float number)
#define SYSCALL_SEED \
SystemCallLabelSeed:; \
SCALLGETPARAMETER(1,FLO,CpuFlo); \
_Stl->Math.Seed(*FLO1); \
goto SystemCallEndLabel; \

//systemcall<rand> float rand()
#define SYSCALL_RAND \
SystemCallLabelRand:; \
SCALLGETREFRINDIR(1,FLO,CpuFlo); \
*FLO1=_Stl->Math.Rand(); \
SCALLOUTPARAMETER(1,FLO,CpuFlo); \
goto SystemCallEndLabel; \

//systemcall<datevalid> bool datevalid(int year,int month,int day)
#define SYSCALL_DATEVALID \
SystemCallLabelDateValid:; \
SCALLGETREFRINDIR(1,BOL,CpuBol); \
SCALLGETPARAMETER(2,INT,CpuInt); \
SCALLGETPARAMETER(3,INT,CpuInt); \
SCALLGETPARAMETER(4,INT,CpuInt); \
*BOL1=_Stl->DateTime.DateValid(*INT2,*INT3,*INT4); \
SCALLOUTPARAMETER(1,BOL,CpuBol); \
goto SystemCallEndLabel; \

//systemcall<datevalue> int datevalue(int year,int month,int day)
#define SYSCALL_DATEVALUE \
SystemCallLabelDateValue:; \
SCALLGETREFRINDIR(1,INT,CpuInt); \
SCALLGETPARAMETER(2,INT,CpuInt); \
SCALLGETPARAMETER(3,INT,CpuInt); \
SCALLGETPARAMETER(4,INT,CpuInt); \
*INT1=_Stl->DateTime.DateValue(*INT2,*INT3,*INT4); \
SCALLOUTPARAMETER(1,INT,CpuInt); \
goto SystemCallEndLabel; \

//systemcall<begofmonth> int begofmonth(int date)
#define SYSCALL_BEGOFMONTH \
SystemCallLabelBegOfMonth:; \
SCALLGETREFRINDIR(1,INT,CpuInt); \
SCALLGETPARAMETER(2,INT,CpuInt); \
*INT1=_Stl->DateTime.BegOfMonth(*INT2); \
SCALLOUTPARAMETER(1,INT,CpuInt); \
goto SystemCallEndLabel; \

//systemcall<endofmonth> int endofmonth(int date)
#define SYSCALL_ENDOFMONTH \
SystemCallLabelEndOfMonth:; \
SCALLGETREFRINDIR(1,INT,CpuInt); \
SCALLGETPARAMETER(2,INT,CpuInt); \
*INT1=_Stl->DateTime.EndOfMonth(*INT2); \
SCALLOUTPARAMETER(1,INT,CpuInt); \
goto SystemCallEndLabel; \

//systemcall<datepart> int datepart(int date,datepart part)
#define SYSCALL_DATEPART \
SystemCallLabelDatePart:; \
SCALLGETREFRINDIR(1,INT,CpuInt); \
SCALLGETPARAMETER(2,INT,CpuInt); \
SCALLGETPARAMETER(3,INT,CpuInt); \
*INT1=_Stl->DateTime.DatePart(*INT2,(StlDatePart)*INT3); \
SCALLOUTPARAMETER(1,INT,CpuInt); \
goto SystemCallEndLabel; \

//systemcall<dateadd> int dateadd(int date,datepart part,int units)
#define SYSCALL_DATEADD \
SystemCallLabelDateAdd:; \
SCALLGETREFRINDIR(1,INT,CpuInt); \
SCALLGETPARAMETER(2,INT,CpuInt); \
SCALLGETPARAMETER(3,INT,CpuInt); \
SCALLGETPARAMETER(4,INT,CpuInt); \
*INT1=_Stl->DateTime.DateAdd(*INT2,(StlDatePart)*INT3,*INT4); \
SCALLOUTPARAMETER(1,INT,CpuInt); \
goto SystemCallEndLabel; \

//systemcall<timevalid> bool timevalid(int hour,int minute,int second,long nanosec)
#define SYSCALL_TIMEVALID \
SystemCallLabelTimeValid:; \
SCALLGETREFRINDIR(1,BOL,CpuBol); \
SCALLGETPARAMETER(2,INT,CpuInt); \
SCALLGETPARAMETER(3,INT,CpuInt); \
SCALLGETPARAMETER(4,INT,CpuInt); \
SCALLGETPARAMETER(5,LON,CpuLon); \
*BOL1=_Stl->DateTime.TimeValid(*INT2,*INT3,*INT4,*LON5); \
SCALLOUTPARAMETER(1,BOL,CpuBol); \
goto SystemCallEndLabel; \

//systemcall<timevalue> long timevalue(int hour,int minute,int second,long nanosec)
#define SYSCALL_TIMEVALUE \
SystemCallLabelTimeValue:; \
SCALLGETREFRINDIR(1,LON,CpuLon); \
SCALLGETPARAMETER(2,INT,CpuInt); \
SCALLGETPARAMETER(3,INT,CpuInt); \
SCALLGETPARAMETER(4,INT,CpuInt); \
SCALLGETPARAMETER(5,LON,CpuLon); \
*LON1=_Stl->DateTime.TimeValue(*INT2,*INT3,*INT4,*LON5); \
SCALLOUTPARAMETER(1,LON,CpuLon); \
goto SystemCallEndLabel; \

//systemcall<timepart> long timepart(long time,timepart part)
#define SYSCALL_TIMEPART \
SystemCallLabelTimePart:; \
SCALLGETREFRINDIR(1,LON,CpuLon); \
SCALLGETPARAMETER(2,LON,CpuLon); \
SCALLGETPARAMETER(3,INT,CpuInt); \
*LON1=_Stl->DateTime.TimePart(*LON2,(StlTimePart)*INT3); \
SCALLOUTPARAMETER(1,LON,CpuLon); \
goto SystemCallEndLabel; \

//systemcall<timeadd> long timeadd(long time,timepart part,long units,ref int dayrest)
#define SYSCALL_TIMEADD \
SystemCallLabelTimeAdd:; \
SCALLGETREFRINDIR(1,LON,CpuLon); \
SCALLGETPARAMETER(2,LON,CpuLon); \
SCALLGETPARAMETER(3,INT,CpuInt); \
SCALLGETPARAMETER(4,LON,CpuLon); \
SCALLGETREFRINDIR(5,INT,CpuInt); \
*LON1=_Stl->DateTime.TimeAdd(*LON2,(StlTimePart)*INT3,*LON4,INT5); \
SCALLOUTPARAMETER(1,LON,CpuLon); \
SCALLOUTPARAMETER(5,INT,CpuInt); \
goto SystemCallEndLabel; \

//systemcall<nsecadd> long nsecadd(long nsec,timepart part,long units,ref int dayrest)
#define SYSCALL_NANOSECADD \
SystemCallLabelNanoSecAdd:; \
SCALLGETREFRINDIR(1,LON,CpuLon); \
SCALLGETPARAMETER(2,LON,CpuLon); \
SCALLGETPARAMETER(3,INT,CpuInt); \
SCALLGETPARAMETER(4,LON,CpuLon); \
SCALLGETREFRINDIR(5,INT,CpuInt); \
*LON1=_Stl->DateTime.NanoSecAdd(*LON2,(StlTimePart)*INT3,*LON4,INT5); \
SCALLOUTPARAMETER(1,LON,CpuLon); \
SCALLOUTPARAMETER(5,INT,CpuInt); \
goto SystemCallEndLabel; \

//systemcall<getdate> int getdate(bool utc)
#define SYSCALL_GETDATE \
SystemCallLabelGetDate:; \
SCALLGETREFRINDIR(1,INT,CpuInt); \
SCALLGETPARAMETER(2,BOL,CpuBol); \
*INT1=_Stl->DateTime.GetDate(*BOL2); \
SCALLOUTPARAMETER(1,INT,CpuInt); \
goto SystemCallEndLabel; \

//systemcall<gettime> long gettime(bool utc)
#define SYSCALL_GETTIME \
SystemCallLabelGetTime:; \
SCALLGETREFRINDIR(1,LON,CpuLon); \
SCALLGETPARAMETER(2,BOL,CpuBol); \
*LON1=_Stl->DateTime.GetTime(*BOL2); \
SCALLOUTPARAMETER(1,LON,CpuLon); \
goto SystemCallEndLabel; \

//systemcall<datediff> int datediff(int date1,int date2)
#define SYSCALL_DATEDIFF \
SystemCallLabelDateDiff:; \
SCALLGETREFRINDIR(1,INT,CpuInt); \
SCALLGETPARAMETER(2,INT,CpuInt); \
SCALLGETPARAMETER(3,INT,CpuInt); \
*INT1=_Stl->DateTime.DateDiff(*INT2,*INT3); \
SCALLOUTPARAMETER(1,INT,CpuInt); \
goto SystemCallEndLabel; \

//systemcall<timediff> long timediff(long time1,long time2)
#define SYSCALL_TIMEDIFF \
SystemCallLabelTimeDiff:; \
SCALLGETREFRINDIR(1,LON,CpuLon); \
SCALLGETPARAMETER(2,LON,CpuLon); \
SCALLGETPARAMETER(3,LON,CpuLon); \
*LON1=_Stl->DateTime.TimeDiff(*LON2,*LON3); \
SCALLOUTPARAMETER(1,LON,CpuLon); \
goto SystemCallEndLabel; \

//Get library id
int Runtime::_GetLibraryId(char *DlName){
  int LibId=-1;
  for(int i=0;i<_DynLib.Length();i++){ 
    if(strcmp((char *)_DynLib[i].DlName,DlName)==0){
      LibId=i;
      break;
    }
  }
  if(LibId==-1){
    _DynLib.Push((DynLibDef){"",nullptr,nullptr,nullptr,nullptr});
    LibId=_DynLib.Length()-1;
    strncpy((char *)_DynLib[LibId].DlName,DlName,_MaxIdLen);
  }
  return LibId;
}

//Get current process
int Runtime::GetCurrentProcess(){
  return _ProcessId;
}

//Executable file read wrapper: Open executable file
bool Runtime::_OpenExecutable(const String& FileName,int& Hnd){
  if(_RomBuffer==nullptr){
    if(!_Stl->FileSystem.GetHandler(Hnd)){
      SysMessage(304).Print(FileName);
      return false;
    }
    if(!_Stl->FileSystem.OpenForRead(Hnd,FileName)){
      SysMessage(305).Print(FileName,_Stl->LastError());
      return false;
    }
  }
  else{
    _RomBuffReadNr=0;
  }
  return true;
}

//Executable file read wrapper: Close executable file
bool Runtime::_CloseExecutable(const String& FileName,int Hnd){
  if(_RomBuffer==nullptr){
    if(!_Stl->FileSystem.CloseFile(Hnd)){ 
      SysMessage(307).Print(FileName,_Stl->LastError());
      return false; 
    }
    if(!_Stl->FileSystem.FreeHandler(Hnd)){ 
      SysMessage(308).Print(FileName,_Stl->LastError());
      return false; 
    }
  }
  return true;
}

//Executable file read wrapper: Read executable file
bool Runtime::_ReadExecutable(const String& FileName,int Hnd,const char *Part,int Index,int SubIndex,Buffer& Buff,long Length){
  if(_RomBuffer==nullptr){
    if(!_Stl->FileSystem.Read(Hnd,Buff,Length)){ SysMessage(306).Print(FileName,Part,ToString(Index)+(SubIndex!=0?"."+ToString(SubIndex):""),_Stl->LastError()); return false; }
  }
  else{
    if(_RomBuffReadNr+Length>_RomBuffer->Length){
      SysMessage(356).Print(FileName,ToString(_RomBuffReadNr+Length),ToString(_RomBuffer->Length)); return false;
    }
    Buff.Copy(&_RomBuffer->Buff[_RomBuffReadNr],Length);
    _RomBuffReadNr+=Length;
  }
  return true;
}

//Executable file read wrapper: Read executable file
bool Runtime::_ReadExecutable(const String& FileName,int Hnd,const char *Part,int Index,int SubIndex,char *Pnt,long Length){
  if(_RomBuffer==nullptr){
    if(!_Stl->FileSystem.Read(Hnd,Pnt,Length)){ SysMessage(306).Print(FileName,Part,ToString(Index)+(SubIndex!=0?"."+ToString(SubIndex):""),_Stl->LastError()); return false; }
  }
  else{
    if(_RomBuffReadNr+Length>_RomBuffer->Length){
      SysMessage(356).Print(FileName,ToString(_RomBuffReadNr+Length),ToString(_RomBuffer->Length)); return false;
    }
    MemCpy(Pnt,&_RomBuffer->Buff[_RomBuffReadNr],Length);
    _RomBuffReadNr+=Length;
  }
  return true;
}

//Load program file (sets all program buffers)
bool Runtime::LoadProgram(const String& FileName,int ProcessId,int ArgNr,char *Arg[],int ArgStart){

  //Variables
  int i;
  int Hnd;
  Buffer Buff;
  CpuWrd Length;
  CpuWrd DimIndex;
  ArrayFixDef ArrFixDef;
  ArrayDynDef ArrDynDef;
  BlockDef Blk;
  DlCallDef DlCall;
  String Error;
  
  //Open executable file
  if(!_OpenExecutable(FileName,Hnd)){ return false; }

  //Read program header
  if(!_ReadExecutable(FileName,Hnd,"HEAD",0,0,Buff,sizeof(BinHdr))){ return false; }
  BinHdr=*reinterpret_cast<const BinaryHeader *>(Buff.BuffPnt());

  //Print program header
  DebugMessage(DebugLevel::VrmRuntime,"Loaded binary header");
  DebugMessage(DebugLevel::VrmRuntime,"FileMark     : "+String(Buffer(BinHdr.FileMark,sizeof(BinHdr.FileMark))));
  DebugMessage(DebugLevel::VrmRuntime,"BinFormat    : "+ToString(BinHdr.BinFormat));
  DebugMessage(DebugLevel::VrmRuntime,"Arquitecture : "+ToString(BinHdr.Arquitecture));
  DebugMessage(DebugLevel::VrmRuntime,"SysVersion   : "+String(BinHdr.SysVersion));
  DebugMessage(DebugLevel::VrmRuntime,"SysBuildDate : "+String(BinHdr.SysBuildDate));
  DebugMessage(DebugLevel::VrmRuntime,"SysBuildTime : "+String(BinHdr.SysBuildTime));
  DebugMessage(DebugLevel::VrmRuntime,"IsBinLibrary : "+ToString(BinHdr.IsBinLibrary));
  DebugMessage(DebugLevel::VrmRuntime,"DebugSymbols : "+ToString(BinHdr.DebugSymbols));
  DebugMessage(DebugLevel::VrmRuntime,"GlobBufferNr : "+ToString(BinHdr.GlobBufferNr));
  DebugMessage(DebugLevel::VrmRuntime,"BlockNr      : "+ToString(BinHdr.BlockNr));
  DebugMessage(DebugLevel::VrmRuntime,"ArrFixDefNr  : "+ToString(BinHdr.ArrFixDefNr));
  DebugMessage(DebugLevel::VrmRuntime,"ArrDynDefNr  : "+ToString(BinHdr.ArrDynDefNr));
  DebugMessage(DebugLevel::VrmRuntime,"CodeBufferNr : "+ToString(BinHdr.CodeBufferNr));
  DebugMessage(DebugLevel::VrmRuntime,"DlCallNr     : "+ToString(BinHdr.DlCallNr));
  DebugMessage(DebugLevel::VrmRuntime,"MemUnitSize  : "+ToString(BinHdr.MemUnitSize));
  DebugMessage(DebugLevel::VrmRuntime,"MemUnits     : "+ToString(BinHdr.MemUnits));
  DebugMessage(DebugLevel::VrmRuntime,"ChunkMemUnits: "+ToString(BinHdr.ChunkMemUnits));
  DebugMessage(DebugLevel::VrmRuntime,"BlockMax     : "+ToString(BinHdr.BlockMax));
  DebugMessage(DebugLevel::VrmRuntime,"LibMajorVers : "+ToString(BinHdr.LibMajorVers));
  DebugMessage(DebugLevel::VrmRuntime,"LibMinorVers : "+ToString(BinHdr.LibMinorVers));
  DebugMessage(DebugLevel::VrmRuntime,"LibRevisionNr: "+ToString(BinHdr.LibRevisionNr));
  DebugMessage(DebugLevel::VrmRuntime,"DependencyNr : "+ToString(BinHdr.DependencyNr));
  DebugMessage(DebugLevel::VrmRuntime,"UndefRefNr   : "+ToString(BinHdr.UndefRefNr));
  DebugMessage(DebugLevel::VrmRuntime,"RelocTableNr : "+ToString(BinHdr.RelocTableNr));
  DebugMessage(DebugLevel::VrmRuntime,"LnkSymDimNr  : "+ToString(BinHdr.LnkSymDimNr));
  DebugMessage(DebugLevel::VrmRuntime,"LnkSymTypNr  : "+ToString(BinHdr.LnkSymTypNr));
  DebugMessage(DebugLevel::VrmRuntime,"LnkSymVarNr  : "+ToString(BinHdr.LnkSymVarNr));
  DebugMessage(DebugLevel::VrmRuntime,"LnkSymFldNr  : "+ToString(BinHdr.LnkSymFldNr));
  DebugMessage(DebugLevel::VrmRuntime,"LnkSymFunNr  : "+ToString(BinHdr.LnkSymFunNr));
  DebugMessage(DebugLevel::VrmRuntime,"LnkSymParNr  : "+ToString(BinHdr.LnkSymParNr));
  DebugMessage(DebugLevel::VrmRuntime,"DbgSymModNr  : "+ToString(BinHdr.DbgSymModNr));
  DebugMessage(DebugLevel::VrmRuntime,"DbgSymTypNr  : "+ToString(BinHdr.DbgSymTypNr));
  DebugMessage(DebugLevel::VrmRuntime,"DbgSymVarNr  : "+ToString(BinHdr.DbgSymVarNr));
  DebugMessage(DebugLevel::VrmRuntime,"DbgSymFldNr  : "+ToString(BinHdr.DbgSymFldNr));
  DebugMessage(DebugLevel::VrmRuntime,"DbgSymFunNr  : "+ToString(BinHdr.DbgSymFunNr));
  DebugMessage(DebugLevel::VrmRuntime,"DbgSymParNr  : "+ToString(BinHdr.DbgSymParNr));
  DebugMessage(DebugLevel::VrmRuntime,"DbgSymLinNr  : "+ToString(BinHdr.DbgSymLinNr));
  DebugMessage(DebugLevel::VrmRuntime,"SuperInitAdr : "+HEXFORMAT(BinHdr.SuperInitAdr));

  //Check architecture matches
  if(BinHdr.Arquitecture!=GetArchitecture()){
    SysMessage(311).Print(ToString(BinHdr.Arquitecture),ToString(GetArchitecture()));
    return false;
  }

  //Set program name
  _Stl->FileSystem.GetFileNameNoExt(FileName).Copy(_ProgName,_MaxIdLen);

  //Copy command line arguments
  _ArgNr=ArgNr;
  _Arg=Arg;
  _ArgStart=ArgStart;

  //Init program buffers
  _Glob.Init(ProcessId,DEFAULT_CHUNKSIZE_GLOB,(char *)"_Glob");
  _Stack.Init(ProcessId,DEFAULT_CHUNKSIZE_STACK,(char *)"_Stack");
  _Code.Init(ProcessId,DEFAULT_CHUNKSIZE_CODE,(char *)"_Code");
  _CallSt.Init(ProcessId,DEFAULT_CHUNKSIZE_CALLST,(char *)"_CallSt");
  _ParmSt.Init(ProcessId,DEFAULT_CHUNKSIZE_PARMST,(char *)"_ParmSt");
  _DlParm.Init(ProcessId,DEFAULT_CHUNKSIZE_PARAM,(char *)"_DlParm");
  _DlVPtr.Init(ProcessId,DEFAULT_CHUNKSIZE_PARMPTR,(char *)"_DlVPtr");
  _RpRule.Init(ProcessId,DEFAULT_CHUNKSIZE_RPRULE,(char *)"_RpRule");
  _BiRule.Init(ProcessId,DEFAULT_CHUNKSIZE_RPRULE,(char *)"_BiRule");
  _DynLib.Init(ProcessId,DEFAULT_CHUNKSIZE_DYNLIB,(char *)"_DynLib");
  _DynFun.Init(ProcessId,DEFAULT_CHUNKSIZE_DYNFUN,(char *)"_DynFun");
  _DebugSym.Mod.Init(ProcessId,DEFAULT_CHUNKSIZE_DBGSYMMOD,(char *)"_DbgSymMod");
  _DebugSym.Typ.Init(ProcessId,DEFAULT_CHUNKSIZE_DBGSYMTYP,(char *)"_DbgSymTyp");
  _DebugSym.Var.Init(ProcessId,DEFAULT_CHUNKSIZE_DBGSYMVAR,(char *)"_DbgSymVar");
  _DebugSym.Fld.Init(ProcessId,DEFAULT_CHUNKSIZE_DBGSYMFLD,(char *)"_DbgSymFld");
  _DebugSym.Fun.Init(ProcessId,DEFAULT_CHUNKSIZE_DBGSYMFUN,(char *)"_DbgSymFun");
  _DebugSym.Par.Init(ProcessId,DEFAULT_CHUNKSIZE_DBGSYMPAR,(char *)"_DbgSymPar");
  _DebugSym.Lin.Init(ProcessId,DEFAULT_CHUNKSIZE_DBGSYMLIN,(char *)"_DbgSymLin");
  if(!_Aux.Init(ProcessId,BinHdr.BlockMax,BinHdr.MemUnits,BinHdr.ChunkMemUnits,BinHdr.MemUnitSize,Error)){
     SysMessage(310).Print(FileName,Error);
     return false;
  }
  _StC.Init(&_Aux);
  _ArC.FixInit(ProcessId,DEFAULT_CHUNKSIZE_ARRGEOM);
  _ArC.DynInit(ProcessId,DEFAULT_CHUNKSIZE_ARRMETA,&_Aux,&_StC);

  //Allocate memory on program buffers that are being loaded from file
  if(!_Glob.Resize(BinHdr.GlobBufferNr)){ SysMessage(309).Print("global",FileName); return false; }
  if(!_Code.Resize(BinHdr.CodeBufferNr)){ SysMessage(309).Print("code",FileName); return false; }

  //Read executable file
  if(!_ReadExecutable(FileName,Hnd,FILEMARKGLOB,-1,0,Buff,strlen(FILEMARKGLOB))){ return false; } 
  if(!_ReadExecutable(FileName,Hnd,FILEMARKGLOB,0,0,_Glob.Pnt(),BinHdr.GlobBufferNr)){ return false; } 
  DebugMessage(DebugLevel::VrmRuntime,"Loaded glob buffer with "+ToString(BinHdr.GlobBufferNr)+" bytes");
  if(!_ReadExecutable(FileName,Hnd,FILEMARKCODE,-1,0,Buff,strlen(FILEMARKCODE))){ return false; } 
  if(!_ReadExecutable(FileName,Hnd,FILEMARKCODE,0,0,_Code.Pnt(),BinHdr.CodeBufferNr)){ return false; } 
  DebugMessage(DebugLevel::VrmRuntime,"Loaded code buffer with "+ToString(BinHdr.CodeBufferNr)+" bytes");
  if(!_ReadExecutable(FileName,Hnd,FILEMARKFARR,-1,0,Buff,strlen(FILEMARKFARR))){ return false; } 
  for(i=0;i<BinHdr.ArrFixDefNr;i++){ 
    if(!_ReadExecutable(FileName,Hnd,FILEMARKFARR,i,0,Buff,sizeof(ArrayFixDef))){ return false; } 
    ArrFixDef=*reinterpret_cast<const ArrayFixDef *>(Buff.BuffPnt());
    if(!_ArC.FixStoreGeom(ArrFixDef.DimNr,ArrFixDef.CellSize,ArrFixDef.DimSize)){ SysMessage(309).Print("fixed array geometries",FileName); return false; }
    #ifdef __DEV__
    String ArrIndexes="";
    for(int j=0;j<ArrFixDef.DimNr;j++){ ArrIndexes+=((j==0?"":",")+ToString(ArrFixDef.DimSize.n[j])); }
    #endif
    DebugMessage(DebugLevel::VrmRuntime,"Loaded fix array metadata: index="+ToString(i)+" geomindex="+HEXFORMAT(ArrFixDef.GeomIndex)+" dimnr="+ToString(ArrFixDef.DimNr)+" cellsize="+ToString(ArrFixDef.CellSize)+" dimsize={"+ArrIndexes+"}");
  }
  if(!_ReadExecutable(FileName,Hnd,FILEMARKDARR,-1,0,Buff,strlen(FILEMARKDARR))){ return false; } 
  for(i=0;i<BinHdr.ArrDynDefNr;i++){ 
    if(!_ReadExecutable(FileName,Hnd,FILEMARKDARR,i,0,Buff,sizeof(ArrayDynDef))){ return false; } 
    ArrDynDef=*reinterpret_cast<const ArrayDynDef *>(Buff.BuffPnt());
    if(!_ArC.DynStoreMeta(0,0,ArrDynDef.DimNr,ArrDynDef.CellSize,ArrDynDef.DimSize)){ SysMessage(309).Print("dynamic array definitions",FileName); return false; }
    #ifdef __DEV__
    String ArrIndexes="";
    for(int j=0;j<ArrDynDef.DimNr;j++){ ArrIndexes+=((j==0?"":",")+ToString(ArrDynDef.DimSize.n[j])); }
    #endif
    DebugMessage(DebugLevel::VrmRuntime,"Loaded dyn array metadata: index="+ToString(i)+" dimnr="+ToString(ArrDynDef.DimNr)+" cellsize="+ToString(ArrDynDef.CellSize)+" dimsize={"+ArrIndexes+"}");
  }
  if(!_ReadExecutable(FileName,Hnd,FILEMARKBLCK,-1,0,Buff,strlen(FILEMARKBLCK))){ return false; } 
  for(i=0;i<BinHdr.BlockNr;i++){ 
    if(!_ReadExecutable(FileName,Hnd,FILEMARKBLCK,i,1,Buff,sizeof(DimIndex))){ return false; } 
    DimIndex=*reinterpret_cast<const CpuWrd *>(Buff.BuffPnt());
    if(!_ReadExecutable(FileName,Hnd,FILEMARKBLCK,i,2,Buff,sizeof(Length))){ return false; } 
    Length=*reinterpret_cast<const CpuWrd *>(Buff.BuffPnt());
    if(!_ReadExecutable(FileName,Hnd,FILEMARKBLCK,i,3,Buff,Length)){ return false; } 
    if(i>0){ //Avoid block 0, since it is a dummy block, inserted in block table so that block zero is avoided because it means no allocated block
      if(!_Aux.ForcedAlloc(0,0,Length,DimIndex,i)){ SysMessage(309).Print("block buffer",FileName); return false; }
      _Aux.Copy(i,Buff.BuffPnt(),Length);
      DebugMessage(DebugLevel::VrmRuntime,"Loaded memory block: index="+ToString(i)+" dimindex="+ToString(DimIndex)+" length="+ToString(Length)+" buffer=\""+String(Buff.BuffPnt(),Length)+"\"");
    }
  }
  if(!_ReadExecutable(FileName,Hnd,FILEMARKDLCA,-1,0,Buff,strlen(FILEMARKDLCA))){ return false; } 
  for(i=0;i<BinHdr.DlCallNr;i++){ 
    if(!_ReadExecutable(FileName,Hnd,FILEMARKDLCA,i,0,Buff,sizeof(DlCall))){ return false; } 
    DlCall=*reinterpret_cast<const DlCallDef *>(Buff.BuffPnt());
    if(!_DynFun.Reserve(1)){ SysMessage(309).Print("dynamic function table",FileName); return false; }
    strncpy((char *)_DynFun[i].DlFunction,(char *)DlCall.DlFunction,_MaxIdLen);
    _DynFun[i].LibIndex=_GetLibraryId((char *)DlCall.DlName);
    _DynFun[i].PhyFunId=-1;
  }
  if(BinHdr.DebugSymbols){
    if(!_ReadExecutable(FileName,Hnd,FILEMARKDMOD,-1,0,Buff,strlen(FILEMARKDMOD))){ return false; } 
    for(i=0;i<BinHdr.DbgSymModNr;i++){ 
      if(!_ReadExecutable(FileName,Hnd,FILEMARKDMOD,i,0,Buff,sizeof(DbgSymModule))){ return false; } 
      if(!_DebugSym.Mod.Reserve(1)){ SysMessage(309).Print("debug symbol module",FileName); return false; }
      MemCpy((char *)&_DebugSym.Mod[i],Buff.BuffPnt(),Buff.Length());
      DebugMessage(DebugLevel::VrmRuntime,"Loaded debug symbol MOD["+ToString(_DebugSym.Mod.Length()-1)+"]: "+System::GetDbgSymDebugStr(_DebugSym.Mod[i]));
    }
    if(!_ReadExecutable(FileName,Hnd,FILEMARKDTYP,-1,0,Buff,strlen(FILEMARKDTYP))){ return false; } 
    for(i=0;i<BinHdr.DbgSymTypNr;i++){ 
      if(!_ReadExecutable(FileName,Hnd,FILEMARKDTYP,i,0,Buff,sizeof(DbgSymType))){ return false; } 
      if(!_DebugSym.Typ.Reserve(1)){ SysMessage(309).Print("debug symbol type",FileName); return false; }
      MemCpy((char *)&_DebugSym.Typ[i],Buff.BuffPnt(),Buff.Length());
      DebugMessage(DebugLevel::VrmRuntime,"Loaded debug symbol TYP["+ToString(_DebugSym.Typ.Length()-1)+"]: "+System::GetDbgSymDebugStr(_DebugSym.Typ[i]));
    }
    if(!_ReadExecutable(FileName,Hnd,FILEMARKDVAR,-1,0,Buff,strlen(FILEMARKDVAR))){ return false; } 
    for(i=0;i<BinHdr.DbgSymVarNr;i++){ 
      if(!_ReadExecutable(FileName,Hnd,FILEMARKDVAR,i,0,Buff,sizeof(DbgSymVariable))){ return false; } 
      if(!_DebugSym.Var.Reserve(1)){ SysMessage(309).Print("debug symbol variable",FileName); return false; }
      MemCpy((char *)&_DebugSym.Var[i],Buff.BuffPnt(),Buff.Length());
      DebugMessage(DebugLevel::VrmRuntime,"Loaded debug symbol VAR["+ToString(_DebugSym.Var.Length()-1)+"]: "+System::GetDbgSymDebugStr(_DebugSym.Var[i]));
    }
    if(!_ReadExecutable(FileName,Hnd,FILEMARKDFLD,-1,0,Buff,strlen(FILEMARKDFLD))){ return false; } 
    for(i=0;i<BinHdr.DbgSymFldNr;i++){ 
      if(!_ReadExecutable(FileName,Hnd,FILEMARKDFLD,i,0,Buff,sizeof(DbgSymField))){ return false; } 
      if(!_DebugSym.Fld.Reserve(1)){ SysMessage(309).Print("debug symbol field",FileName); return false; }
      MemCpy((char *)&_DebugSym.Fld[i],Buff.BuffPnt(),Buff.Length());
      DebugMessage(DebugLevel::VrmRuntime,"Loaded debug symbol FLD["+ToString(_DebugSym.Fld.Length()-1)+"]: "+System::GetDbgSymDebugStr(_DebugSym.Fld[i]));
    }
    if(!_ReadExecutable(FileName,Hnd,FILEMARKDFUN,-1,0,Buff,strlen(FILEMARKDFUN))){ return false; } 
    for(i=0;i<BinHdr.DbgSymFunNr;i++){ 
      if(!_ReadExecutable(FileName,Hnd,FILEMARKDFUN,i,0,Buff,sizeof(DbgSymFunction))){ return false; } 
      if(!_DebugSym.Fun.Reserve(1)){ SysMessage(309).Print("debug symbol function",FileName); return false; }
      MemCpy((char *)&_DebugSym.Fun[i],Buff.BuffPnt(),Buff.Length());
      DebugMessage(DebugLevel::VrmRuntime,"Loaded debug symbol FUN["+ToString(_DebugSym.Fun.Length()-1)+"]: "+System::GetDbgSymDebugStr(_DebugSym.Fun[i]));
    }
    if(!_ReadExecutable(FileName,Hnd,FILEMARKDPAR,-1,0,Buff,strlen(FILEMARKDPAR))){ return false; } 
    for(i=0;i<BinHdr.DbgSymParNr;i++){ 
      if(!_ReadExecutable(FileName,Hnd,FILEMARKDPAR,i,0,Buff,sizeof(DbgSymParameter))){ return false; } 
      if(!_DebugSym.Par.Reserve(1)){ SysMessage(309).Print("debug symbol parameter",FileName); return false; }
      MemCpy((char *)&_DebugSym.Par[i],Buff.BuffPnt(),Buff.Length());
      DebugMessage(DebugLevel::VrmRuntime,"Loaded debug symbol PAR["+ToString(_DebugSym.Par.Length()-1)+"]: "+System::GetDbgSymDebugStr(_DebugSym.Par[i]));
    }
    if(!_ReadExecutable(FileName,Hnd,FILEMARKDLIN,-1,0,Buff,strlen(FILEMARKDLIN))){ return false; } 
    for(i=0;i<BinHdr.DbgSymLinNr;i++){ 
      if(!_ReadExecutable(FileName,Hnd,FILEMARKDLIN,i,0,Buff,sizeof(DbgSymLine))){ return false; } 
      if(!_DebugSym.Lin.Reserve(1)){ SysMessage(309).Print("debug symbol line",FileName); return false; }
      MemCpy((char *)&_DebugSym.Lin[i],Buff.BuffPnt(),Buff.Length());
      DebugMessage(DebugLevel::VrmRuntime,"Loaded debug symbol LIN["+ToString(_DebugSym.Lin.Length()-1)+"]: "+System::GetDbgSymDebugStr(_DebugSym.Lin[i]));
    }
  }

  //Close file
  if(!_CloseExecutable(FileName,Hnd)){
    return false; 
  }

  //Return code
  return true;

}

//Instruction timming macros
//Idea: Minimun measurable time is about 100ns on linux and 1mls on windows, difference is huge and on this interval either linux and windows go through several DS CPU cycles
//Time is measured on the instructions when a change in the clock is detected, when this happens all executed instructions in that interval get a fraction of that time

#define INST_TIMMING_DEF        int TimmingIndex=0; \
                                long CpuCycles=0; \
                                long AbsCpuCycles=0; \
                                double CycleDuration=0; \
                                double AvgTime; \
                                ClockPoint AbsClkStart; \
                                ClockPoint AbsClkEnd; \
                                ClockPoint ClockStart; \
                                ClockPoint ClockEnd;

#define INST_TIMMING_INIT       for(int i=0;i<_InstructionNr+_SystemCallNr+BinHdr.DlCallNr;i++){ _Timming.Add((InstTimmingTable){0,0,0,-1,-1,0}); } \
                                ClockStart=ClockGet(); \
                                while(ClockIntervalNSec(ClockGet(),ClockStart)==0); \
                                ClockStart=ClockGet(); \
                                AbsClkStart=ClockStart; \

#define INST_TIMMING_CHECK      CpuCycles++; \
                                AbsCpuCycles++; \
                                TimmingIndex=(ICODE==(CpuIcd)CpuInstCode::LCALL?_InstructionNr+_SystemCallNr+LCNR:(ICODE==(CpuIcd)CpuInstCode::SCALL?_InstructionNr+SCNR:ICODE)); \
                                _Timming[TimmingIndex].NotTimedCount++; \
                                ClockEnd=ClockGet(); \
                                if((CycleDuration=ClockIntervalNSec(ClockEnd,ClockStart))!=0){ \
                                  _Timming[TimmingIndex].CumulatedTime+=(CycleDuration/((double)CpuCycles)); \
                                  _Timming[TimmingIndex].ExecutionCount++; \
                                  _Timming[TimmingIndex].NotTimedCount--; \
                                  ClockStart=ClockEnd; \
                                  CpuCycles=0; \
                                } \

#define INST_TIMMING_FINISH     ClockEnd=ClockGet(); \
                                AbsClkEnd=ClockEnd; \
                                for(int i=0;i<_InstructionNr+_SystemCallNr+BinHdr.DlCallNr;i++){ \
                                  if(_Timming[i].NotTimedCount!=0){ \
                                    if(_Timming[i].ExecutionCount!=0){ AvgTime=_Timming[i].CumulatedTime/(double)_Timming[i].ExecutionCount; } \
                                    else{ AvgTime=ClockIntervalNSec(AbsClkEnd,AbsClkStart)/(double)AbsCpuCycles; } \
                                    _Timming[i].CumulatedTime+=AvgTime*(double)_Timming[i].NotTimedCount; \
                                    _Timming[i].ExecutionCount+=_Timming[i].NotTimedCount; \
                                    _Timming[i].Measured=(_Timming[i].ExecutionCount>_Timming[i].NotTimedCount?((double)_Timming[i].ExecutionCount-(double)_Timming[i].NotTimedCount)/(double)_Timming[i].ExecutionCount:0); \
                                  } \
                                  else if(_Timming[i].ExecutionCount!=0){ \
                                    _Timming[i].Measured=1; \
                                  } \
                                } \

//Debug messages on program loop
#define PROG_LOOP_MSG(mode) ("---"+(mode>1?" ["+ToString(_InstCount)+"]":String(""))+" IP="+HEXFORMAT(IP)+" BP="+HEXFORMAT(BP)+" ProcessId="+ToString(_ProcessId)+" ScopeId="+ToString(_ScopeId)+" ScopeNr="+ToString(_ScopeNr)+" "+(BinHdr.DebugSymbols?_AddressInfoLine(IP):"")+String(150,'-')).Left(150)

//Set current runtime instance
void SetCurrentRuntime(Runtime *Rt){
  _Rt=Rt;
}

//Get current process
int _GetCurrentProcess(){
  return _Rt->GetCurrentProcess();
}

//Run program
void Runtime::_RunProgram(int BenchMark,CpuAdr& LastIP){
  
  //Pointers
  char *CodePtr;  //Aux code pointer
  
  //Instruction tables
  INST_LABEL_TABLE;                         
  
  //System call label table
  SYSTEMCALL_LABEL_TABLE;

  //Control registers
  CpuAdr IP;               //Instruction pointer
  CpuAdr BP;               //Base pointer

  //Operation pointer registers
  void *REG1,*REG2,*REG3,*REG4,*REG5,*REG6;
  
  //Variables used only in DEV mode
  #ifdef __DEV__
  String Hex;                      //Disasembled line
  String Asm;                      //Disasembled line
  String Symbols;                  //Disassembling symbols
  String Arg[_MaxInstructionArgs]; //Assembler argument value (for log)
  #endif
  
  //Instruction timming variables
  INST_TIMMING_DEF;
  
  //Assign to stack a default size and do address decoding for first time
  if(!_Stack.Resize(DEFAULT_CHUNKSIZE_STACK)){ 
    System::Throw(SysExceptionCode::MemoryAllocationFailure);
    return; 
  }
  if(!_DecodeLocalVariables(true,_Code.Pnt(),_Stack.Pnt(),_Stack.Pnt())){ return; }
  _Stack.Empty();
  
  //If runtime debug level is enabled we need to force benchmark mode so execution loop goes through argument print
  #ifdef __DEV__
  if(DebugLevelEnabled(DebugLevel::VrmRuntime) && BenchMark<2){ BenchMark=2; }
  #endif

  //Set fake instruction table (for benchmark modes)
  void *InstEnd=nullptr;
  if(BenchMark>=2){
    if(BenchMark==2){ InstEnd=&&RunProgInstEnd2; }
    if(BenchMark==3){ InstEnd=&&RunProgInstEnd3; }
    for(int i=0;i<_InstructionNr;i++){ FakeInstAddress[i]=InstEnd; }
  }
  
  //Do decoding of instructions in code buffer
  _DecodeInstructionCodes(BenchMark,InstAddress,InstEnd,_Code.Pnt());
  
  //Init machine state
  IP=0;
  BP=0;
  PST=0;
  HPTR=nullptr;
  CUMULSC=1;
  CFIX=-1;
  CodePtr=_Code.Pnt();
  GlobPnt=_Glob.Pnt();
  StackPnt=_Stack.Pnt();
  DMOD1=CpuDecMode::LoclVar;
  DMOD2=CpuDecMode::LoclVar;
  DMOD3=CpuDecMode::LoclVar;
  DMOD4=CpuDecMode::LoclVar;
  InstAddressConstPtr=(BenchMark<2?&InstAddress[0]:&FakeInstAddress[0]);
  DebugMessage(DebugLevel::VrmRuntime,"Decoder mode for argument 1 as "+CpuDecModeName(DMOD1)); \
  DebugMessage(DebugLevel::VrmRuntime,"Decoder mode for argument 2 as "+CpuDecModeName(DMOD2)); \
  DebugMessage(DebugLevel::VrmRuntime,"Decoder mode for argument 3 as "+CpuDecModeName(DMOD3)); \
  DebugMessage(DebugLevel::VrmRuntime,"Decoder mode for argument 4 as "+CpuDecModeName(DMOD4)); \

  //Init instruction timmings (only for benchmark mode 3)
  if(BenchMark==3){ INST_TIMMING_INIT; }
  
  //Start execution (on fast exeucution enters disparcher loop otherwise enters instruction header)
  if(BenchMark<2){ REAL_INST_DISPATCH; }

  //Execution block
  try{

    //Debug instruction header
    RunProgInstrHeader3:;
    PREVIP=IP;
    PREVBP=BP;
    RunProgInstrHeader2:;
    #ifdef __DEV__
    if(DebugLevelEnabled(DebugLevel::VrmRuntime)){
      if(CFIX==-1 || (IP<_DebugSym.Fun[CFIX].BegAddress || IP>_DebugSym.Fun[CFIX].EndAddress)){
        for(int i=0;i<BinHdr.DbgSymFunNr;i++){
          if(IP>=_DebugSym.Fun[i].BegAddress && IP<=_DebugSym.Fun[i].EndAddress){ CFIX=i; break; }
        }
      }
      _DisassembleLine(CodePtr,StackPnt,IP,DMOD1,DMOD2,DMOD3,DMOD4,ADV1,ADV2,ADV3,ADV4,CFIX,true,Hex,Asm,Symbols);
      _GetArguments(IP,BP,GlobPnt,StackPnt,CodePtr,DMOD1,DMOD2,DMOD3,DMOD4,1,&Arg[0]);
      PREVIP=IP;
      PREVBP=BP;
    }
    DebugMessage(DebugLevel::VrmRuntime,"");
    DebugMessage(DebugLevel::VrmRuntime,PROG_LOOP_MSG(BenchMark));
    DebugMessage(DebugLevel::VrmRuntime,Asm+(Symbols.Length()!=0?" ;"+Symbols:""));
    #endif
    REAL_INST_DISPATCH;
    
    //Label to build expanded runtime
    //[$$EXPANSION_START]
    
    //Instruction switcher
    INST_SWITCHER;

    //System call switcher
    SYSTEMCALL_SWITCHER;

    //Label to build expanded runtime
    //[$$EXPANSION_END]

    //Instruction end to restore decoders
    RunProgInstEndRestore:;
    RESTORE_HANDLER;
    RESTORE_DECODER(1);
    RESTORE_DECODER(2);
    RESTORE_DECODER(3);
    RESTORE_DECODER(4);
    PROG_INST_DISPATCH;

    //Instruction end handlers (only for benchmark modes 2 & 3)
    RunProgInstEnd3:;
    _InstCount++;
    ICODE=*(CpuIcd *)(CodePtr+PREVIP+sizeof(CpuWrd));
    INST_TIMMING_CHECK;
    #ifdef __DEV__
    if(DebugLevelEnabled(DebugLevel::VrmRuntime)){
      _GetArguments(PREVIP,PREVBP,GlobPnt,StackPnt,CodePtr,(CpuDecMode)0,(CpuDecMode)0,(CpuDecMode)0,(CpuDecMode)0,2,&Arg[0]);
      if(Arg[0].Length()!=0){ DebugMessage(DebugLevel::VrmRuntime,Arg[0]); }
      if(Arg[1].Length()!=0){ DebugMessage(DebugLevel::VrmRuntime,Arg[1]); }
      if(Arg[2].Length()!=0){ DebugMessage(DebugLevel::VrmRuntime,Arg[2]); }
      if(Arg[3].Length()!=0){ DebugMessage(DebugLevel::VrmRuntime,Arg[3]); }
    }
    #endif
    goto RunProgInstrHeader3;
    RunProgInstEnd2:;
    _InstCount++;
    #ifdef __DEV__
    if(DebugLevelEnabled(DebugLevel::VrmRuntime)){
      _GetArguments(PREVIP,PREVBP,GlobPnt,StackPnt,CodePtr,(CpuDecMode)0,(CpuDecMode)0,(CpuDecMode)0,(CpuDecMode)0,2,&Arg[0]);
      if(Arg[0].Length()!=0){ DebugMessage(DebugLevel::VrmRuntime,Arg[0]); }
      if(Arg[1].Length()!=0){ DebugMessage(DebugLevel::VrmRuntime,Arg[1]); }
      if(Arg[2].Length()!=0){ DebugMessage(DebugLevel::VrmRuntime,Arg[2]); }
      if(Arg[3].Length()!=0){ DebugMessage(DebugLevel::VrmRuntime,Arg[3]); }
    }
    #endif
    goto RunProgInstrHeader2;

  }
  catch(BaseException& Ex){
    System::Throw(SysExceptionCode::RuntimeBaseException,Ex.Description());
  }

  //Exception handler exit
  RunProgInstrExceptionHandler:;
  LastIP=IP;

  //Program exit
  RunProgExit:;

  //Finish instruction timmings
  if(BenchMark==3){ INST_TIMMING_FINISH; }

}

//Execute program
bool Runtime::ExecProgram(int ProcessId,int BenchMark){

  //Variables
  CpuAdr LastIP;

  //Initialize machine internal registers
  _InstCount=0;
  _ProcessId=ProcessId;
  _ScopeId=1;
  _ScopeNr=1;
  _ScopeUnlock=false;
  _RpSource=nullptr;
  _RpDestin=nullptr;
  _StC.SetScope(_ScopeId,_ScopeNr);
  _ArC.DynSetScope(_ScopeId,_ScopeNr);
  _ArC.FixSetBP(BinHdr.ArrFixDefNr);

  //Timed execution
  //(Wait for clock sync in order to get less varying timmings when clock has low resolution (i.e.: windows))
  if(BenchMark!=0){
    _ProgStart=ClockGet();
    while(ClockIntervalNSec(ClockGet(),_ProgStart)==0);
    _ProgStart=ClockGet();
  }

  //Program execution
  _RunProgram(BenchMark,LastIP);

  //Print call stack
  if(System::ExceptionFlag()){
    System::ExceptionPrint();
    if(BinHdr.DebugSymbols){ _PrintCallStack(LastIP); }
  }

  //Calculate benchmark
  if(BenchMark!=0){
    _ProgEnd=ClockGet();
    _PrintBenchMark(BenchMark);
  }
  
  //Return code
  return (System::ExceptionFlag()?false:true);

}

//Reference indirection inside function (inner function)
void Runtime::_InnerRefIndirection(char *GlobPnt,char *StackPnt,CpuRef Ref,char **Ptr,CpuMbl &Scope){
  REFINDIRECTION(Ref,*Ptr,Scope);
}

//Reference indirection inside function
bool Runtime::_RefIndirection(char *GlobPnt,char *StackPnt,CpuRef Ref,char **Ptr,CpuMbl &Scope){
  _InnerRefIndirection(GlobPnt,StackPnt,Ref,Ptr,Scope);
  if(System::ExceptionFlag()){ return false; }
  return true;
}

//Decode instruction codes
bool Runtime::_DecodeInstructionCodes(int BenchMark,const void **InstAddress,const void *FakeInstHandler,char *CodePtr){

  //Variables
  bool Exit;
  CpuAdr IP;
  CpuInstCode InstCode;
  void **HandlerPtr;
  CpuIcd *InstCodePtr;

  //Debug message
  DebugMessage(DebugLevel::VrmRuntime,"Entered code buffer decoding (benchmark="+ToString(BenchMark)+" codeptr="+HEXFORMAT(CodePtr)+")");

  //Decoding loop
  IP=0;
  Exit=false;
  do{
    
    //Get instruction code
    InstCode=(CpuInstCode)(*(CpuIcd *)(CodePtr+IP));
    
    //Replace instruction code and insert instruction handler
    InstCodePtr=(CpuIcd *)(CodePtr+IP+sizeof(CpuWrd));
    HandlerPtr=(void **)(CodePtr+IP);
    (*InstCodePtr)=(CpuIcd)InstCode;
    (*HandlerPtr)=(BenchMark<=1?(void *)InstAddress[(int)InstCode]:(void *)FakeInstHandler);

    //Message
    DebugMessage(DebugLevel::VrmRuntime,HEXFORMAT(IP)+": "+_Inst[(int)InstCode].Mnemonic+" code="+HEXFORMAT(*InstCodePtr)+" handler="+HEXFORMAT(*HandlerPtr));

    //Increase IP
    IP+=_Inst[(int)InstCode].Length;

    //Exit loop
    if(IP>=_Code.Length()){ Exit=true; }

  }while(!Exit);

  //Message
  DebugMessage(DebugLevel::VrmRuntime,"Exited code buffer decoding");

  //Return success
  return true;

}

//Decode local variables
bool Runtime::_DecodeLocalVariables(bool FirstTime,char *CodePtr,const char *OldStackPtr,const char *NewStackPtr){

  //Variables
  int i;
  int ArgOffset;
  bool Exit;
  bool Restore1,Restore2,Restore3,Restore4;
  CpuAdr IP;
  CpuAdr ArgAdr;
  CpuAdr *ArgPtr;
  CpuInstCode InstCode;
  CpuDecMode ArgDecMode[_MaxInstructionArgs];

  //Debug message
  DebugMessage(DebugLevel::VrmRuntime,"Entered local variable address decoding (codeptr="+HEXFORMAT(CodePtr)+" oldstackptr="+HEXFORMAT(OldStackPtr)+" newstackptr="+HEXFORMAT(NewStackPtr)+")");

  //Init decoding loop
  IP=0;
  ArgDecMode[0]=CpuDecMode::LoclVar;
  ArgDecMode[1]=CpuDecMode::LoclVar;
  ArgDecMode[2]=CpuDecMode::LoclVar;
  ArgDecMode[3]=CpuDecMode::LoclVar;

  //Decoding loop
  Exit=false;
  Restore1=false;
  Restore2=false;
  Restore3=false;
  Restore4=false;
  do{
    
    //Get instruction code
    InstCode=(CpuInstCode)(*(CpuIcd *)(CodePtr+IP));
    if((int)InstCode<0 || (int)InstCode>=_InstructionNr){
      System::Throw(SysExceptionCode::InvalidInstructionCode,HEXFORMAT(InstCode));
      return false;
    }
    DebugMessage(DebugLevel::VrmRuntime,HEXFORMAT(IP)+": Decoded instruction "+_Inst[(int)InstCode].Mnemonic+" ("+HEXFORMAT(InstCode)+")");

    //Decode argument variable addresses
    for(i=0;i<_Inst[(int)InstCode].ArgNr;i++){
      ArgOffset=_Inst[(int)InstCode].Offset[i];
      if(_Inst[(int)InstCode].AdrMode[i]==CpuAdrMode::Address){
        if(ArgDecMode[i]==CpuDecMode::LoclVar){
          ArgPtr=(CpuAdr *)(CodePtr+IP+ArgOffset);
          if(FirstTime){
            if((*ArgPtr)<0){ System::Throw(SysExceptionCode::InvalidMemoryAddress,"stack memory",HEXFORMAT(*ArgPtr),"unknown"); return false; }
            (*ArgPtr)=(CpuAdr)reinterpret_cast<CpuAdr>(NewStackPtr+(*ArgPtr));
          }
          else{
            ArgAdr=reinterpret_cast<char *>(*ArgPtr)-OldStackPtr;
            if(ArgAdr<0){ System::Throw(SysExceptionCode::InvalidMemoryAddress,"stack memory",HEXFORMAT(ArgAdr),"unknown"); return false; }
            (*ArgPtr)=(CpuAdr)reinterpret_cast<CpuAdr>(NewStackPtr+ArgAdr);
          }
          DebugMessage(DebugLevel::VrmRuntime,"Encoded local variable on argument "+ToString(i+1));
        }
      }
    }

    //Set decoder modes
    switch(InstCode){
      case CpuInstCode::DAGV1: ArgDecMode[0]=CpuDecMode::GlobVar; Restore1=true; DebugMessage(DebugLevel::VrmRuntime,"Argument 1 set as GlobVar"); break;
      case CpuInstCode::DAGV2: ArgDecMode[1]=CpuDecMode::GlobVar; Restore2=true; DebugMessage(DebugLevel::VrmRuntime,"Argument 2 set as GlobVar"); break;
      case CpuInstCode::DAGV3: ArgDecMode[2]=CpuDecMode::GlobVar; Restore3=true; DebugMessage(DebugLevel::VrmRuntime,"Argument 3 set as GlobVar"); break;
      case CpuInstCode::DAGV4: ArgDecMode[3]=CpuDecMode::GlobVar; Restore4=true; DebugMessage(DebugLevel::VrmRuntime,"Argument 4 set as GlobVar"); break;
      case CpuInstCode::DAGI1: ArgDecMode[0]=CpuDecMode::GlobInd; Restore1=true; DebugMessage(DebugLevel::VrmRuntime,"Argument 1 set as GlobInd"); break;
      case CpuInstCode::DAGI2: ArgDecMode[1]=CpuDecMode::GlobInd; Restore2=true; DebugMessage(DebugLevel::VrmRuntime,"Argument 2 set as GlobInd"); break;
      case CpuInstCode::DAGI3: ArgDecMode[2]=CpuDecMode::GlobInd; Restore3=true; DebugMessage(DebugLevel::VrmRuntime,"Argument 3 set as GlobInd"); break;
      case CpuInstCode::DAGI4: ArgDecMode[3]=CpuDecMode::GlobInd; Restore4=true; DebugMessage(DebugLevel::VrmRuntime,"Argument 4 set as GlobInd"); break;
      case CpuInstCode::DALI1: ArgDecMode[0]=CpuDecMode::LoclInd; Restore1=true; DebugMessage(DebugLevel::VrmRuntime,"Argument 1 set as LoclInd"); break;
      case CpuInstCode::DALI2: ArgDecMode[1]=CpuDecMode::LoclInd; Restore2=true; DebugMessage(DebugLevel::VrmRuntime,"Argument 2 set as LoclInd"); break;
      case CpuInstCode::DALI3: ArgDecMode[2]=CpuDecMode::LoclInd; Restore3=true; DebugMessage(DebugLevel::VrmRuntime,"Argument 3 set as LoclInd"); break;
      case CpuInstCode::DALI4: ArgDecMode[3]=CpuDecMode::LoclInd; Restore4=true; DebugMessage(DebugLevel::VrmRuntime,"Argument 4 set as LoclInd"); break;
      default:
        if(Restore1){ ArgDecMode[0]=CpuDecMode::LoclVar; Restore1=false; DebugMessage(DebugLevel::VrmRuntime,"Argument 1 set as LoclVar"); }
        if(Restore2){ ArgDecMode[1]=CpuDecMode::LoclVar; Restore2=false; DebugMessage(DebugLevel::VrmRuntime,"Argument 2 set as LoclVar"); }
        if(Restore3){ ArgDecMode[2]=CpuDecMode::LoclVar; Restore3=false; DebugMessage(DebugLevel::VrmRuntime,"Argument 3 set as LoclVar"); }
        if(Restore4){ ArgDecMode[3]=CpuDecMode::LoclVar; Restore4=false; DebugMessage(DebugLevel::VrmRuntime,"Argument 4 set as LoclVar"); }
        break;
    }

    //Increase IP
    IP+=_Inst[(int)InstCode].Length;

    //Exit loop
    if(IP>=_Code.Length()){ Exit=true; }

  }while(!Exit);

  //Message
  DebugMessage(DebugLevel::VrmRuntime,"Exited local variable address decoding");

  //Return success
  return true;

}

//Get funtion debug name
String Runtime::_GetFunctionDebugName(int FunIndex){

  //Variables
  int i;
  String ArgList;
  String ReturnType;
  String TypeName;

  //Get function return type
  ReturnType="";
  if(FunIndex!=-1 && strcmp((const char *)&_DebugSym.Fun[FunIndex].Name[0],SYSTEM_NAMESPACE MAIN_FUNCTION)!=0){
    if(_DebugSym.Fun[FunIndex].IsVoid){
      ReturnType="void";
    }
    else{
      ReturnType=(_DebugSym.Fun[FunIndex].TypIndex>=0 && _DebugSym.Fun[FunIndex].TypIndex<=BinHdr.DbgSymTypNr-1 ? (char *)&_DebugSym.Typ[_DebugSym.Fun[FunIndex].TypIndex].Name[0] : "" );
    }
    if(ReturnType.Length()!=0){ ReturnType+=" "; }
  }

  //Get parameter list
  ArgList="";
  if(FunIndex!=-1){
    if(_DebugSym.Fun[FunIndex].ParmLow >=0 && _DebugSym.Fun[FunIndex].ParmLow <=BinHdr.DbgSymParNr-1
    && _DebugSym.Fun[FunIndex].ParmHigh>=0 && _DebugSym.Fun[FunIndex].ParmHigh<=BinHdr.DbgSymParNr-1){
      for(i=_DebugSym.Fun[FunIndex].ParmLow;i<=_DebugSym.Fun[FunIndex].ParmHigh;i++){
        if(strcmp((char *)&_DebugSym.Par[i].Name[0],SYSTEM_NAMESPACE FUNCTION_RESULT)!=0
        && strcmp((char *)&_DebugSym.Par[i].Name[0],SELF_REF_NAME)!=0){
          TypeName=(_DebugSym.Par[i].TypIndex>=0 && _DebugSym.Par[i].TypIndex<=BinHdr.DbgSymTypNr-1 ? (char *)&_DebugSym.Typ[_DebugSym.Par[i].TypIndex].Name[0] : "" );
          ArgList+=(ArgList.Length()!=0?",":"")+(TypeName.Length()!=0 ? TypeName+" "+String((char *)_DebugSym.Par[i].Name) : "{"+String((char *)_DebugSym.Par[i].Name)+"}" );
        }
      }
    }
  }

  //Return function name
  return ReturnType+String((char *)_DebugSym.Fun[FunIndex].Name)+"("+ArgList+")";
  
}

//Disassemble program
bool Runtime::Disassemble(){

  //Variables
  bool Exit;
  int i;
  int CurrLinIndex;
  int CurrFunIndex;
  CpuAdr IP;
  CpuAdr FuncAddress;
  CpuInstCode InstCode;
  CpuDecMode ArgDecMode[_MaxInstructionArgs];
  String Hex;
  String Asm;
  String Symbols;
  String SourceInfo;
  String FuncDebugName;
  Array<DisAsmLine> Lines;

  //Init disassembling
  IP=0;
  FuncAddress=0;
  ArgDecMode[0]=CpuDecMode::LoclVar;
  ArgDecMode[1]=CpuDecMode::LoclVar;
  ArgDecMode[2]=CpuDecMode::LoclVar;
  ArgDecMode[3]=CpuDecMode::LoclVar;
  CurrLinIndex=-1;
  CurrFunIndex=-1;
  FuncDebugName="";

  //Disassemble loop
  Exit=false;
  do{
    
    //Get instruction code
    InstCode=(CpuInstCode)(*(CpuIcd *)(&_Code[IP]));

    //Identify current source line
    SourceInfo="";
    if(CurrLinIndex==-1 || (IP<_DebugSym.Lin[CurrLinIndex].BegAddress || IP>_DebugSym.Lin[CurrLinIndex].EndAddress)){
      for(i=0;i<BinHdr.DbgSymLinNr;i++){
        if(IP>=_DebugSym.Lin[i].BegAddress && IP<=_DebugSym.Lin[i].EndAddress){ 
          CurrLinIndex=i; 
          SourceInfo=String((char *)_DebugSym.Mod[_DebugSym.Lin[i].ModIndex].Name)+String(SOURCE_EXT)+String(":")+ToString(_DebugSym.Lin[i].LineNr);
          break; 
        }
      }
    }

    //Identify current function 
    if(CurrFunIndex==-1 || (IP<_DebugSym.Fun[CurrFunIndex].BegAddress || IP>_DebugSym.Fun[CurrFunIndex].EndAddress)){
      for(i=0;i<BinHdr.DbgSymFunNr;i++){
        if(IP>=_DebugSym.Fun[i].BegAddress && IP<=_DebugSym.Fun[i].EndAddress){ 
          _DumpDisassembledLines(FuncAddress,FuncDebugName,Lines);
          Lines.Reset();
          FuncAddress=_DebugSym.Fun[i].BegAddress;
          FuncDebugName=_GetFunctionDebugName(i);
          CurrFunIndex=i; 
          break; 
        }
      }
    }

    //Disassemble line at IP
    if(!_DisassembleLine(_Code.Pnt(),nullptr,IP,ArgDecMode[0],ArgDecMode[1],ArgDecMode[2],ArgDecMode[3],0,0,0,0,CurrFunIndex,false,Hex,Asm,Symbols)){ return false; }
    Lines.Add((DisAsmLine){IP,Hex,Asm,Symbols,SourceInfo});    
  
    //Set decoder modes
    switch(InstCode){
      case CpuInstCode::DAGV1: ArgDecMode[0]=CpuDecMode::GlobVar; break;
      case CpuInstCode::DAGV2: ArgDecMode[1]=CpuDecMode::GlobVar; break;
      case CpuInstCode::DAGV3: ArgDecMode[2]=CpuDecMode::GlobVar; break;
      case CpuInstCode::DAGV4: ArgDecMode[3]=CpuDecMode::GlobVar; break;
      case CpuInstCode::DAGI1: ArgDecMode[0]=CpuDecMode::GlobInd; break;
      case CpuInstCode::DAGI2: ArgDecMode[1]=CpuDecMode::GlobInd; break;
      case CpuInstCode::DAGI3: ArgDecMode[2]=CpuDecMode::GlobInd; break;
      case CpuInstCode::DAGI4: ArgDecMode[3]=CpuDecMode::GlobInd; break;
      case CpuInstCode::DALI1: ArgDecMode[0]=CpuDecMode::LoclInd; break;
      case CpuInstCode::DALI2: ArgDecMode[1]=CpuDecMode::LoclInd; break;
      case CpuInstCode::DALI3: ArgDecMode[2]=CpuDecMode::LoclInd; break;
      case CpuInstCode::DALI4: ArgDecMode[3]=CpuDecMode::LoclInd; break;
      default:
        ArgDecMode[0]=CpuDecMode::LoclVar;
        ArgDecMode[1]=CpuDecMode::LoclVar;
        ArgDecMode[2]=CpuDecMode::LoclVar;
        ArgDecMode[3]=CpuDecMode::LoclVar;
        break;
    }

    //Increase IP
    IP+=_Inst[(int)InstCode].Length;

    //Exit loop
    if(IP>=_Code.Length()){ Exit=true; }

  }while(!Exit);

  //Dump last function
  if(Lines.Length()!=0){
    _DumpDisassembledLines(FuncAddress,FuncDebugName,Lines);
  }
  
  //Return success
  return true;

}

//Dump disassembled lines
bool Runtime::_DumpDisassembledLines(CpuAdr FuncAddress,const String& FuncDebugName,const Array<DisAsmLine>& Lines){

  //Symbols
  struct SymbolRef{
    String Name;
    String Address;
    const String& SortKey() const { return Address; }
  };

  //Variables
  int i,j;
  int Pos;
  int LabelIndex;
  int MaxSource;
  int MaxLabel;
  int MaxAsm;
  String Source;
  String Label;
  String Name;
  String Address;
  Array<String> SymbolList;
  SortedArray<SymbolRef,const String&> Symbols;

  //Get all jump labels and replace in assembler
  if(BinHdr.DebugSymbols!=0){
    for(i=0;i<Lines.Length();i++){
      Pos=Lines[i].Asm.Search("{");
      if(Pos!=-1){
        Address=Lines[i].Asm.CutLeft(Pos+1).GetUntil("}");
        if((LabelIndex=Symbols.Search("J"+Address))==-1){
          Label="{Label"+ToString(Symbols.Length())+"}";
          Symbols.Add((SymbolRef){Label,"J"+Address});
        }
        LabelIndex=Symbols.Search("J"+Address);
        Lines[i].Asm=Lines[i].Asm.Replace("{"+Address+"}",Symbols[LabelIndex].Name);
      }
    }
  }

  //Get all symbols
  for(i=0;i<Lines.Length();i++){
    if(Lines[i].Symbols.Length()!=0){
      SymbolList=Lines[i].Symbols.Split(",");
      for(j=0;j<SymbolList.Length();j++){
        Pos=SymbolList[j].Search("=");
        Name=SymbolList[j].Mid(0,Pos);
        Address=SymbolList[j].Mid(Pos+1,SymbolList[j].Length()-Pos-1);
        if(Symbols.Search(Address)==-1){
          Symbols.Add((SymbolRef){Name,Address});
        }
      }
    }
  }

  //Calculate maximun sizes
  MaxSource=0;
  MaxLabel=0;
  MaxAsm=0;
  for(i=0;i<Lines.Length();i++){
    Address=HEXFORMAT(Lines[i].IP);
    LabelIndex=Symbols.Search("J"+Address);
    if(LabelIndex!=-1){ Label=Symbols[LabelIndex].Name; } else{ Label=""; }
    if(Label.Length()+1>MaxLabel){ MaxLabel=Label.Length()+1; }
    if(Lines[i].SourceInfo.Length()+2>MaxSource){ MaxSource=Lines[i].SourceInfo.Length()+2; }
    if(Lines[i].Asm.Length()>MaxAsm){ MaxAsm=Lines[i].Asm.Length(); }
  }
  MaxSource=(MaxSource<23?23:MaxSource);
  MaxLabel=(MaxLabel<10?10:MaxLabel);
  MaxAsm=(MaxAsm<63?63:MaxAsm);

  //Dump function name
  if(BinHdr.DebugSymbols!=0){
    if(FuncAddress==0){
      std::cout << "; ----- START CODE -----" << std::endl;
    }
    else{
      std::cout << std::endl;
      std::cout << "; ----- FUNCTION " << (FuncDebugName.Length()!=0?FuncDebugName:"("+HEXFORMAT(FuncAddress)+")") << " -----" << std::endl;
    }
  }

  //Dump symbol list
  if(Symbols.Length()!=0){ std::cout << std::endl; }
  for(i=0;i<Symbols.Length();i++){
    std::cout << Symbols[i].Address.CutLeft(1) << " " << Symbols[i].Address.Left(1) << "SYMBOL " << Symbols[i].Name << std::endl;
  }

  //Dump assembler
  if(BinHdr.DebugSymbols!=0){ std::cout << std::endl; }
  for(i=0;i<Lines.Length();i++){
    Source=(Lines[i].SourceInfo.Length()!=0?"["+Lines[i].SourceInfo+"]":"");
    Address=HEXFORMAT(Lines[i].IP);
    LabelIndex=Symbols.Search("J"+Address);
    if(LabelIndex!=-1){ Label=Symbols[LabelIndex].Name+":"; } else{ Label=""; }
    std::cout << Source.LJust(MaxSource) << " " << Label.LJust(MaxLabel) << " " << Lines[i].Asm.LJust(MaxAsm) << " ;" << Lines[i].Hex << std::endl;
  }

  //Return success
  return true;

}

//Disassemble line
bool Runtime::_DisassembleLine(char *CodePtr,char *StackPtr,CpuAdr IP,CpuDecMode DecMode1,CpuDecMode DecMode2,CpuDecMode DecMode3,CpuDecMode DecMode4,
                               CpuAdr Adv1,CpuAdr Adv2,CpuAdr Adv3,CpuAdr Adv4,int CurrFunIndex,bool RuntimeMode,String& Hex,String& Asm,String& Symbols){

  //Variables
  int i,j;
  int ArgOffset;
  int FunIndex;
  int VarIndex;
  CpuAdr ValueAdr;
  CpuAgx ValueAgx;
  CpuShr Code;
  CpuInstCode InstCode;
  CpuDataType ArgType;
  CpuAdrMode ArgAdrMode;
  CpuDecMode ArgDecMode[_MaxInstructionArgs];
  CpuAdr ArgAdv[_MaxInstructionArgs];
  String Address;
  String Symbol;

  //Init output
  Hex="";
  Asm="";
  Symbols="";

  //Put decoder modes into array
  ArgDecMode[0]=DecMode1;
  ArgDecMode[1]=DecMode2;
  ArgDecMode[2]=DecMode3;
  ArgDecMode[3]=DecMode4;
  ArgAdv[0]=Adv1;
  ArgAdv[1]=Adv2;
  ArgAdv[2]=Adv3;
  ArgAdv[3]=Adv4;

  //Get instruction code
  if(RuntimeMode){
    InstCode=(CpuInstCode)(*(CpuIcd *)(CodePtr+IP+sizeof(CpuWrd)));
  }
  else{
    InstCode=(CpuInstCode)(*(CpuIcd *)(CodePtr+IP));
  }
  if((int)InstCode<0 || (int)InstCode>_InstructionNr-1){
    if(!RuntimeMode){ Code=(int)InstCode; SysMessage(576).Print(HEXFORMAT(IP),ToString((int)InstCode),HEXFORMAT(Code)); return false; }
  }

  //Output code address
  Hex+="{"+HEXFORMAT(IP)+"} ";

  //Output instruction code
  Asm+=_Inst[(int)InstCode].Mnemonic.LJust(6);
  Hex+=String(Buffer((char *)&InstCode,sizeof(CpuIcd)).ToHex().BuffPnt())+" ";
  Hex+=String(Buffer(sizeof(CpuWrd),0).ToHex().BuffPnt())+" ";

  //Output arguments
  for(i=0;i<_Inst[(int)InstCode].ArgNr;i++){
    
    //Get argument type, addressing mode and offset
    ArgType=_Inst[(int)InstCode].Type[i];
    ArgOffset=_Inst[(int)InstCode].Offset[i];
    ArgAdrMode=_Inst[(int)InstCode].AdrMode[i];

    //Switch on addressing mode of argument
    switch(ArgAdrMode){
      
      //Litteral values
      case CpuAdrMode::LitValue:
        if(ArgType==(CpuDataType)-1){
          if(GetArchitecture()==32){
            Asm+="("+CpuDataTypeCharId(CpuDataType::Integer)+")"+ToString(*(CpuInt *)(CodePtr+IP+ArgOffset));
            Hex+=String(Buffer((char *)(CodePtr+IP+ArgOffset),sizeof(CpuInt)).ToHex().BuffPnt());
          }
          else{
            Asm+="("+CpuDataTypeCharId(CpuDataType::Long)+")"+ToString(*(CpuLon *)(CodePtr+IP+ArgOffset));
            Hex+=String(Buffer((char *)(CodePtr+IP+ArgOffset),sizeof(CpuLon)).ToHex().BuffPnt());
          }
        }
        else{
          switch(ArgType){      
            case CpuDataType::Boolean:
              Asm+="("+CpuDataTypeCharId(ArgType)+")"+Buffer(*(char *)(CodePtr+IP+ArgOffset)).ToHex().BuffPnt(); 
              Hex+=String(Buffer((char *)(CodePtr+IP+ArgOffset),sizeof(CpuBol)).ToHex().BuffPnt());
              break;
            case CpuDataType::Char:
              Asm+="("+CpuDataTypeCharId(ArgType)+")"+Buffer(*(char *)(CodePtr+IP+ArgOffset)).ToHex().BuffPnt(); 
              Hex+=String(Buffer((char *)(CodePtr+IP+ArgOffset),sizeof(CpuChr)).ToHex().BuffPnt());
              break;
            case CpuDataType::Short:
              Asm+="("+CpuDataTypeCharId(ArgType)+")"+ToString(*(CpuShr *)(CodePtr+IP+ArgOffset));
              Hex+=String(Buffer((char *)(CodePtr+IP+ArgOffset),sizeof(CpuShr)).ToHex().BuffPnt());
              break;
            case CpuDataType::Integer:
              Asm+="("+CpuDataTypeCharId(ArgType)+")"+ToString(*(CpuInt *)(CodePtr+IP+ArgOffset));
              Hex+=String(Buffer((char *)(CodePtr+IP+ArgOffset),sizeof(CpuInt)).ToHex().BuffPnt());
              break;
            case CpuDataType::Long:
              Asm+="("+CpuDataTypeCharId(ArgType)+")"+ToString(*(CpuLon *)(CodePtr+IP+ArgOffset));
              Hex+=String(Buffer((char *)(CodePtr+IP+ArgOffset),sizeof(CpuLon)).ToHex().BuffPnt());
              break;
            case CpuDataType::Float:
              Asm+="("+CpuDataTypeCharId(ArgType)+")"+ToString(*(CpuFlo *)(CodePtr+IP+ArgOffset));
              Hex+=String(Buffer((char *)(CodePtr+IP+ArgOffset),sizeof(CpuFlo)).ToHex().BuffPnt());
              break;
            case CpuDataType::ArrGeom:
              ValueAgx=*(CpuAgx *)(CodePtr+IP+ArgOffset);
              if(ValueAgx&ARRGEOMMASK80){
                ValueAgx=ValueAgx&ARRGEOMMASK7F;
                Asm+="AGX(["+NZHEXFORMAT(ValueAgx)+"])";
              }
              else{
                Asm+="AGX(<"+NZHEXFORMAT(ValueAgx)+">)";
              }
              Hex+=String(Buffer((char *)(CodePtr+IP+ArgOffset),sizeof(CpuAgx)).ToHex().BuffPnt());
              break;
            case CpuDataType::FunAddr:
              ValueAdr=*(CpuAdr *)(CodePtr+IP+ArgOffset);
              FunIndex=-1;
              for(j=0;j<BinHdr.DbgSymFunNr;j++){
                if(_DebugSym.Fun[j].BegAddress==ValueAdr){ FunIndex=j; break; }
              }
              if(FunIndex!=-1){
                Symbol="("+String((char *)_DebugSym.Fun[FunIndex].Name)+")";
                Address=(RuntimeMode?NZHEXFORMAT(ValueAdr):"F"+HEXFORMAT(ValueAdr));
                Symbols+=(Symbols.Length()!=0?String(","):String(""))+Symbol+"="+Address;
                Asm+=Symbol;
              }
              else{
                Asm+="("+NZHEXFORMAT(ValueAdr)+")";
              }
              Hex+=String(Buffer((char *)(CodePtr+IP+ArgOffset),sizeof(CpuAdr)).ToHex().BuffPnt());
              break;
            case CpuDataType::JumpAddr:
              ValueAdr=IP+(*(CpuAdr *)(CodePtr+IP+ArgOffset));
              Asm+="{"+(BinHdr.DebugSymbols!=0 && RuntimeMode==false?HEXFORMAT(ValueAdr):NZHEXFORMAT(ValueAdr))+"}";
              Hex+=String(Buffer((char *)(CodePtr+IP+ArgOffset),sizeof(CpuAdr)).ToHex().BuffPnt());
              break;
            case CpuDataType::VarAddr:
              ValueAdr=*(CpuAdr *)(CodePtr+IP+ArgOffset);
              switch(ArgDecMode[i]){
                case CpuDecMode::LoclVar: Asm+="&<"+NZHEXFORMAT(ValueAdr)+">"; break;
                case CpuDecMode::GlobVar: Asm+="&["+NZHEXFORMAT(ValueAdr)+"]"; break;
                case CpuDecMode::LoclInd: if(!RuntimeMode){ SysMessage(568).Print(ToString(i+1),_Inst[(int)InstCode].Mnemonic); return false; } break;
                case CpuDecMode::GlobInd: if(!RuntimeMode){ SysMessage(569).Print(ToString(i+1),_Inst[(int)InstCode].Mnemonic); return false; } break;
              }
              Hex+=String(Buffer((char *)(CodePtr+IP+ArgOffset),sizeof(CpuAdr)).ToHex().BuffPnt());
              break;
            case CpuDataType::StrBlk:
            case CpuDataType::ArrBlk: 
            case CpuDataType::Undefined: 
              if(!RuntimeMode){ SysMessage(570).Print(CpuDataTypeName(ArgType),ToString(i+1),_Inst[(int)InstCode].Mnemonic); return false; }
              break;
          }
        }
        break;
      
      //Variable addresses
      case CpuAdrMode::Address:
        if(RuntimeMode==true){
          if(ArgDecMode[i]==CpuDecMode::LoclVar){
            ValueAdr=reinterpret_cast<CpuAdr>(reinterpret_cast<char *>(*(CpuAdr *)(CodePtr+IP+ArgOffset))-StackPtr);
          }
          else{
            ValueAdr=ArgAdv[i];
          }
        }
        else{
          ValueAdr=*(CpuAdr *)(CodePtr+IP+ArgOffset);
        }
        VarIndex=-1;
        if(ArgDecMode[i]==CpuDecMode::LoclVar || ArgDecMode[i]==CpuDecMode::LoclInd){
          if(CurrFunIndex!=-1){
            for(j=0;j<BinHdr.DbgSymVarNr;j++){
              if(_DebugSym.Var[j].Glob==false && _DebugSym.Var[j].FunIndex==CurrFunIndex && _DebugSym.Var[j].Address==ValueAdr){ VarIndex=j; break; }
            }
          }
        }
        else{
          for(j=0;j<BinHdr.DbgSymVarNr;j++){
            if(_DebugSym.Var[j].Glob==true && _DebugSym.Var[j].Address==ValueAdr){ VarIndex=j; break; }
          }
        }
        if(VarIndex!=-1){
          switch(ArgDecMode[i]){
            case CpuDecMode::LoclVar: Symbol="<" +String((char *)_DebugSym.Var[VarIndex].Name)+">"; 
              Address=(RuntimeMode?NZHEXFORMAT(ValueAdr):"L"+HEXFORMAT(ValueAdr));
              Symbols+=(Symbols.Length()!=0?String(","):String(""))+Symbol+"="+Address;
              Asm+=Symbol;
              break;
            case CpuDecMode::GlobVar: Symbol="[" +String((char *)_DebugSym.Var[VarIndex].Name)+"]"; 
              Address=(RuntimeMode?NZHEXFORMAT(ValueAdr):"G"+HEXFORMAT(ValueAdr));
              Symbols+=(Symbols.Length()!=0?String(","):String(""))+Symbol+"="+Address;
              Asm+=Symbol;
              break;
            case CpuDecMode::LoclInd: Symbol="<"+String((char *)_DebugSym.Var[VarIndex].Name)+">"; 
              Address=(RuntimeMode?NZHEXFORMAT(ValueAdr):"L"+HEXFORMAT(ValueAdr));
              Symbols+=(Symbols.Length()!=0?String(","):String(""))+Symbol+"="+Address;
              Asm+="*"+Symbol;
              break;
            case CpuDecMode::GlobInd: Symbol="["+String((char *)_DebugSym.Var[VarIndex].Name)+"]"; 
              Address=(RuntimeMode?NZHEXFORMAT(ValueAdr):"G"+HEXFORMAT(ValueAdr));
              Symbols+=(Symbols.Length()!=0?String(","):String(""))+Symbol+"="+Address;
              Asm+="*"+Symbol;
              break;
          }
        }
        else{
          switch(ArgDecMode[i]){
            case CpuDecMode::LoclVar: Asm+="<" +NZHEXFORMAT(ValueAdr)+">"; break;
            case CpuDecMode::GlobVar: Asm+="[" +NZHEXFORMAT(ValueAdr)+"]"; break;
            case CpuDecMode::LoclInd: Asm+="*<"+NZHEXFORMAT(ValueAdr)+">"; break;
            case CpuDecMode::GlobInd: Asm+="*["+NZHEXFORMAT(ValueAdr)+"]"; break;
          }
        }
        Hex+=String(Buffer((char *)&ValueAdr,sizeof(CpuAdr)).ToHex().BuffPnt());
        break;

      //Indirections (invalid here, only through decoder programming)
      case CpuAdrMode::Indirection:
        if(!RuntimeMode){ SysMessage(571).Print(ToString(i+1),_Inst[(int)InstCode].Mnemonic); return false; }
        break;

    }

    //Separate arguments by comma
    if(_Inst[(int)InstCode].ArgNr>1 && i<_Inst[(int)InstCode].ArgNr-1){
      Asm+=",";
      Hex+=" ";
    }

  }

  //Return success
  return true;

}

//Get arguments
bool Runtime::_GetArguments(CpuAdr IP,CpuAdr BP,char *GlobPnt,char *StackPnt,char *CodePtr,CpuDecMode DecMode1,CpuDecMode DecMode2,CpuDecMode DecMode3,CpuDecMode DecMode4,int Stage,String *Arg){

  //Variables
  int i;
  int ArgOffset;
  CpuInstCode InstCode;
  CpuDataType ArgType;
  CpuAdrMode ArgAdrMode;
  CpuRef Ref;
  CpuMbl Scope;
  char *ArgPtr;
  String DecMode;
  static CpuDecMode ArgDecMode[_MaxInstructionArgs];

  //Argument attributes into arrays
  //(We get them in stage 1, as in stage 2 they might be modified)
  if(Stage==1){
    ArgDecMode[0]=DecMode1; ArgDecMode[1]=DecMode2; ArgDecMode[2]=DecMode3; ArgDecMode[3]=DecMode4;  
  }

  //Initialize arguments
  if(Stage==1){ Arg[0]=""; Arg[1]=""; Arg[2]=""; Arg[3]=""; }

  //Get instruction code
  InstCode=(CpuInstCode)(*(CpuIcd *)(CodePtr+IP+sizeof(CpuWrd)));

  //Output arguments
  for(i=0;i<_Inst[(int)InstCode].ArgNr;i++){
    
    //Get argument type, addressing mode and offset
    ArgType=_Inst[(int)InstCode].Type[i];
    ArgOffset=_Inst[(int)InstCode].Offset[i];
    ArgAdrMode=_Inst[(int)InstCode].AdrMode[i];

    //Get argument decoder mode
    switch(ArgDecMode[i]){
      case CpuDecMode::LoclVar: DecMode="(lv)"; break;
      case CpuDecMode::GlobVar: DecMode="(gv)"; break;
      case CpuDecMode::LoclInd: DecMode="(li)"; break;
      case CpuDecMode::GlobInd: DecMode="(gi)"; break;
    }
    
    //Switch on addressing mode of argument
    switch(ArgAdrMode){
      
      //Litteral values
      case CpuAdrMode::LitValue:
        switch(ArgType){      
          case CpuDataType::Boolean:
            Arg[i]+=(Stage==1?"BOL"+ToString(i+1)+DecMode+"=":" -> ")+_ToStringCpuBol(*(CpuBol *)(CodePtr+IP+ArgOffset)); 
            break;
          case CpuDataType::Char:
            Arg[i]+=(Stage==1?"CHR"+ToString(i+1)+DecMode+"=":" -> ")+_ToStringCpuChr(*(CpuChr *)(CodePtr+IP+ArgOffset)); 
            break;
          case CpuDataType::Short:
            Arg[i]+=(Stage==1?"SHR"+ToString(i+1)+DecMode+"=":" -> ")+_ToStringCpuShr(*(CpuShr *)(CodePtr+IP+ArgOffset)); 
            break;
          case CpuDataType::Integer:
            Arg[i]+=(Stage==1?"INT"+ToString(i+1)+DecMode+"=":" -> ")+_ToStringCpuInt(*(CpuInt *)(CodePtr+IP+ArgOffset)); 
            break;
          case CpuDataType::Long:
            Arg[i]+=(Stage==1?"LON"+ToString(i+1)+DecMode+"=":" -> ")+_ToStringCpuLon(*(CpuLon *)(CodePtr+IP+ArgOffset)); 
            break;
          case CpuDataType::Float:
            Arg[i]+=(Stage==1?"FLO"+ToString(i+1)+DecMode+"=":" -> ")+_ToStringCpuFlo(*(CpuFlo *)(CodePtr+IP+ArgOffset)); 
            break;
          case CpuDataType::ArrGeom:
            Arg[i]+=(Stage==1?"AGX"+ToString(i+1)+DecMode+"=":" -> ")+_ToStringCpuAgx(*(CpuAgx *)(CodePtr+IP+ArgOffset)); 
            break;
          case CpuDataType::FunAddr:
            Arg[i]+=(Stage==1?"ADR"+ToString(i+1)+DecMode+"=":" -> ")+_ToStringCpuAdr(*(CpuAdr *)(CodePtr+IP+ArgOffset)); 
            break;
          case CpuDataType::JumpAddr:
            Arg[i]+=(Stage==1?"ADR"+ToString(i+1)+DecMode+"=":" -> ")+_ToStringCpuAdr(*(CpuAdr *)(CodePtr+IP+ArgOffset)); 
            break;
          case CpuDataType::VarAddr:
            Arg[i]+=(Stage==1?"ADR"+ToString(i+1)+DecMode+"=":" -> ")+_ToStringCpuAdr(*(CpuAdr *)(CodePtr+IP+ArgOffset)); 
            break;
          case CpuDataType::StrBlk:
          case CpuDataType::ArrBlk: 
          case CpuDataType::Undefined: 
            SysMessage(572).Print(CpuDataTypeName(ArgType),ToString(i+1),_Inst[(int)InstCode].Mnemonic);
            return false;
            break;
        }
        break;
      
      //Variable addresses
      case CpuAdrMode::Address:
    
        //Decode argument
        if(Stage==1){
          ArgPtr=(reinterpret_cast<char*>(*(CpuAdr *)(CodePtr+IP+ArgOffset))+BP);
        }
        else{
          switch(ArgDecMode[i]){
            case CpuDecMode::LoclVar: 
              ArgPtr=(reinterpret_cast<char*>(*(CpuAdr *)(CodePtr+IP+ArgOffset))+BP);
              break;
            case CpuDecMode::GlobVar: 
              ArgPtr=GlobPnt+(*(CpuAdr *)(CodePtr+IP+ArgOffset));
              break;
            case CpuDecMode::LoclInd: 
              Ref=*(CpuRef *)(StackPnt+(*(CpuAdr *)(CodePtr+IP+ArgOffset))+BP);
              if(!_RefIndirection(GlobPnt,StackPnt,Ref,&ArgPtr,Scope)){
                SysMessage(578).Print(CpuDataTypeName(ArgType),ToString(i+1),_Inst[(int)InstCode].Mnemonic);
                return false;
              }
              break;
            case CpuDecMode::GlobInd: 
              Ref=*(CpuRef *)(GlobPnt+(*(CpuAdr *)(CodePtr+IP+ArgOffset)));
              if(!_RefIndirection(GlobPnt,StackPnt,Ref,&ArgPtr,Scope)){
                SysMessage(564).Print(CpuDataTypeName(ArgType),ToString(i+1),_Inst[(int)InstCode].Mnemonic);
                return false;
              }
              break;
          }
        }
    
        //Get argument value
        switch(ArgType){      
          case CpuDataType::Boolean:
            Arg[i]+=(Stage==1?"BOL"+ToString(i+1)+DecMode+"=":" -> ")+_ToStringCpuBol(*reinterpret_cast<CpuBol*>(ArgPtr)); 
            break;
          case CpuDataType::Char:
            Arg[i]+=(Stage==1?"CHR"+ToString(i+1)+DecMode+"=":" -> ")+_ToStringCpuChr(*reinterpret_cast<CpuChr*>(ArgPtr));
            break;
          case CpuDataType::Short:
            Arg[i]+=(Stage==1?"SHR"+ToString(i+1)+DecMode+"=":" -> ")+_ToStringCpuShr(*reinterpret_cast<CpuShr*>(ArgPtr));
            break;
          case CpuDataType::Integer:
            Arg[i]+=(Stage==1?"INT"+ToString(i+1)+DecMode+"=":" -> ")+_ToStringCpuInt(*reinterpret_cast<CpuInt*>(ArgPtr));
            break;
          case CpuDataType::Long:
            Arg[i]+=(Stage==1?"LON"+ToString(i+1)+DecMode+"=":" -> ")+_ToStringCpuLon(*reinterpret_cast<CpuLon*>(ArgPtr));
            break;
          case CpuDataType::Float:
            Arg[i]+=(Stage==1?"FLO"+ToString(i+1)+DecMode+"=":" -> ")+_ToStringCpuFlo(*reinterpret_cast<CpuFlo*>(ArgPtr));
            break;
          case CpuDataType::ArrGeom:
            Arg[i]+=(Stage==1?"AGX"+ToString(i+1)+DecMode+"=":" -> ")+_ToStringCpuAgx(*reinterpret_cast<CpuAgx*>(ArgPtr));
            break;
          case CpuDataType::StrBlk:
            Arg[i]+=(Stage==1?"MBL"+ToString(i+1)+DecMode+"=":" -> ")+_ToStringCpuMbl(*reinterpret_cast<CpuMbl*>(ArgPtr));
            break;
          case CpuDataType::ArrBlk: 
            Arg[i]+=(Stage==1?"MBL"+ToString(i+1)+DecMode+"=":" -> ")+_ToStringCpuMbl(*reinterpret_cast<CpuMbl*>(ArgPtr));
            break;
          case CpuDataType::Undefined: 
            Arg[i]+=(Stage==1?"DAT"+ToString(i+1)+DecMode+"=":" -> ")+_ToStringCpuDat(*reinterpret_cast<CpuDat*>(ArgPtr));
            break;
          case CpuDataType::FunAddr:
          case CpuDataType::JumpAddr:
          case CpuDataType::VarAddr:
            SysMessage(573).Print(CpuDataTypeName(ArgType),ToString(i+1),_Inst[(int)InstCode].Mnemonic);
            return false;
            break;
        }
        break;

      //Indirections (invalid here, only through argument decoding)
      case CpuAdrMode::Indirection:
        SysMessage(574).Print(ToString(i+1),_Inst[(int)InstCode].Mnemonic);
        return false;
        break;

    }

  }

  //Return success
  return true;

}

//Get code address information
String Runtime::_AddressInfoLine(CpuAdr CodeAddress){

  //Variables
  int i;
  int LinIndex;

  //Find source line index
  LinIndex=-1;
  for(i=0;i<BinHdr.DbgSymLinNr;i++){
    if(CodeAddress>=_DebugSym.Lin[i].BegAddress && CodeAddress<=_DebugSym.Lin[i].EndAddress){ LinIndex=i; break; }
  }

  //Return information
  if(LinIndex!=-1){
    return "["+String((char *)_DebugSym.Mod[_DebugSym.Lin[LinIndex].ModIndex].Path)+":"+ToString(_DebugSym.Lin[LinIndex].LineNr)+"] ";
  }
  else{
    return "";
  }

}

//Get code address information
String Runtime::_AddressInfo(CpuAdr CodeAddress,bool& MainFrame){

  //Variables
  int i;
  int LinIndex;
  int FunIndex;
  String Info;
  String FuncDebugName;

  //Entry point address
  if(CodeAddress==0){
    MainFrame=false;
    return HEXFORMAT(CodeAddress)+" (entry address)";
  }

  //Find source line index
  LinIndex=-1;
  for(i=0;i<BinHdr.DbgSymLinNr;i++){
    if(CodeAddress>=_DebugSym.Lin[i].BegAddress && CodeAddress<=_DebugSym.Lin[i].EndAddress){ LinIndex=i; break; }
  }

  //Find function index
  FunIndex=-1;
  for(i=0;i<BinHdr.DbgSymFunNr;i++){
    if(CodeAddress>=_DebugSym.Fun[i].BegAddress && CodeAddress<=_DebugSym.Fun[i].EndAddress){ FunIndex=i; break; }
  }

  //Get function debug name
  FuncDebugName=_GetFunctionDebugName(FunIndex);

  //Compose line
  Info="";
  Info+=(LinIndex!=-1 ? String((char *)_DebugSym.Mod[_DebugSym.Lin[LinIndex].ModIndex].Path)+":"+ToString(_DebugSym.Lin[LinIndex].LineNr) : HEXFORMAT(CodeAddress));
  Info+=(FunIndex!=-1 ? " from "+FuncDebugName : "");

  //Main frame
  MainFrame=(FunIndex!=-1 && strcmp((const char *)&_DebugSym.Fun[FunIndex].Name[0],SYSTEM_NAMESPACE MAIN_FUNCTION)==0)?true:false;

  //Return address information
  return Info;
  
}

//Print call stack
void Runtime::_PrintCallStack(CpuAdr IP){
  
  //Variables
  bool MainFrame;
  int Frame;
  CpuAdr Adr;
  String Info;

  //Print call stack
  Frame=-1;
  do{

    //Begin with current instruction pointer
    if(Frame==-1){
      Adr=IP;
    }
    else{
      Adr=_CallSt[Frame].OrgAddress;
    }

    //Get address information
    Info=_AddressInfo(Adr,MainFrame);
    
    //Print origin of error
    _Stl->Console.PrintErrorLine("["+ToString((Frame==-1?_CallSt.Length():Frame),"%02i")+"] -> At "+Info);

    //Next frame
    if(Frame==-1){
      Frame=_CallSt.Length()-1;
    }
    else{
      Frame--;
    }

    //Exit at last frame or when entry point is found
    if(Frame==-1 || MainFrame){ break; }

  }while(true);

}

//Set dynamic library path
void Runtime::SetLibPaths(const String& DynLibPath,const String& TmpLibPath){
  DynLibPath.Copy(_DynLibPath,FILEPATHLEN);
  TmpLibPath.Copy(_TmpLibPath,FILEPATHLEN);
}

//Closed all open files
void Runtime::CloseAllFiles(){
  _Stl->FileSystem.CloseAll();
}

//Unload dynamic libraries
void Runtime::UnloadLibraries(){
  for(int i=0;i<_DynLib.Length();i++){
    if(_DynLib[i].Handler!=nullptr){
      if(_DynLib[i].CloseDispatcher!=nullptr){ _DynLib[i].CloseDispatcher(); }
      _CloseDynLibrary(_DynLib[i].Handler);
    }
  }
}

//Set rom buffer pointer
void Runtime::SetRomBuffer(RomFileBuffer *Ptr){
  _RomBuffer=Ptr;
}

//Measure minimun clock tick
double Runtime::_MinimunClockTick(){
  int Measures=0;
  double Duration=0;
  ClockPoint ClockStart;
  ClockPoint ClockEnd;
  while(true){
    while(ClockIntervalNSec(ClockGet(),ClockStart)==0);
    ClockStart=ClockGet();
    while(ClockIntervalNSec((ClockEnd=ClockGet()),ClockStart)==0);
    Duration+=ClockIntervalNSec(ClockEnd,ClockStart);
    Measures++;
    if(Duration>500000000 || Measures>1000){ break; }
  }
  return Duration/((double)Measures);
}

//Get time (nanoseconds) in appropriate scale
String Runtime::_GetScaledTime(double NanoSecs){
  String Text;
  if(NanoSecs<1000){
    Text=ToString(NanoSecs,"%0.2fns");
  }
  else if(NanoSecs>=1000 && NanoSecs<1000000){
    Text=ToString(NanoSecs/((double)1000),"%0.2fus");
  }
  else if(NanoSecs>=1000000 && NanoSecs<1000000000){
    Text=ToString(NanoSecs/((double)1000000),"%0.2fms");
  }
  else if(NanoSecs>=1000000000){
    Text=ToString(NanoSecs/((double)1000000000),"%0.2fs ");
  }
  return Text;
}

//Output benchmark
void Runtime::_PrintBenchMark(int BenchMark){

  //Variables
  int i;
  String Measure;
  int OrderNumber;
  int RankNumber;
  int Index;
  double MinTimeMeasure;
  double TotalTime;
  double MaxTime;
  double Seconds;
  double Speed;
  double AvgInstTime;
  Array<String> Mnemonic;
  Array<String> ExecCount;
  Array<String> CumulTime;
  Array<String> AvgTime;
  Array<String> Measured;
  String Headings;
  Array<String> Rows;

  //Output instruction timmings
  if(BenchMark==3){
    
    //Calculate minimun measurable time
    MinTimeMeasure=_MinimunClockTick();

    //Calculate total time
    TotalTime=0;
    for(i=0;i<_InstructionNr+_SystemCallNr+BinHdr.DlCallNr;i++){
      if(_Timming[i].ExecutionCount!=0){ 
        TotalTime+=_Timming[i].CumulatedTime;
      }
    }

    //Calculate average time ranking
    RankNumber=1;
    do{
      Index=-1;
      MaxTime=-1;
      for(i=0;i<_InstructionNr+_SystemCallNr+BinHdr.DlCallNr;i++){
        if(_Timming[i].CumulatedTime/(double)_Timming[i].ExecutionCount>MaxTime && _Timming[i].Rank==-1 && _Timming[i].ExecutionCount!=0){ 
          MaxTime=_Timming[i].CumulatedTime/(double)_Timming[i].ExecutionCount; 
          Index=i; 
        }
      }
      if(Index!=-1){
        _Timming[Index].Rank=RankNumber;
        RankNumber++;
      }
      else{
        break;
      }
    }while(true);

    //Calculate numbers in string format ordered by cumulated time
    OrderNumber=0;
    do{
      Index=-1;
      MaxTime=-1;
      for(i=0;i<_InstructionNr+_SystemCallNr+BinHdr.DlCallNr;i++){
        if(_Timming[i].CumulatedTime>MaxTime && _Timming[i].Order==-1 && _Timming[i].ExecutionCount!=0){ 
          MaxTime=_Timming[i].CumulatedTime; 
          Index=i; 
        }
      }
      if(Index!=-1){
        if(Index>=_InstructionNr+_SystemCallNr){
          Mnemonic.Add("LCALL "+ToString(Index-_InstructionNr-_SystemCallNr)+":"+_DlCallStr(Index-_InstructionNr-_SystemCallNr));
        }
        else if(Index>=_InstructionNr){
          Mnemonic.Add("SCALL "+ToString(Index-_InstructionNr)+":"+SystemCallId((SystemCall)(Index-_InstructionNr)));
        }
        else{
          Mnemonic.Add(_Inst[Index].Mnemonic+((CpuInstCode)Index==CpuInstCode::SCALL || (CpuInstCode)Index==CpuInstCode::LCALL?" *":""));
        }
        _Timming[Index].Order=OrderNumber;
        ExecCount.Add(ToString(_Timming[Index].ExecutionCount));
        CumulTime.Add(_GetScaledTime(_Timming[Index].CumulatedTime)+"  "+("["+ToString(((double)100)*_Timming[Index].CumulatedTime/TotalTime,"%0.2f%%")+"]").RJust(8));
        AvgTime.Add(_GetScaledTime(_Timming[Index].CumulatedTime/(double)_Timming[Index].ExecutionCount)+"  "+("["+ToString(RankNumber-_Timming[Index].Rank)+"]").RJust(5));
        Measured.Add(ToString(100*_Timming[Index].Measured,"%0.2f")+"%");
        OrderNumber++;
      }
      else{
        break;
      }
    }while(true);

    //Print instruction timmings
    Headings="Nr.~Instr.~Exec.Count~Cumulated time~Average time~True measures";
    for(i=0;i<Mnemonic.Length();i++){
      Rows.Add(ToString(i+1)+"~"+Mnemonic[i]+"~"+ExecCount[i]+"~"+CumulTime[i]+"~"+AvgTime[i]+"~"+Measured[i]);
    }
    _Stl->Console.PrintTable(Headings,Rows,"~","LRLLLL");
    _Stl->Console.PrintLine("Time resolution: "+_GetScaledTime(MinTimeMeasure));
    _Stl->Console.PrintLine("Measured time: "+_GetScaledTime(TotalTime));
  
  }

  //Calculate total on benchmark mode 1
  if(BenchMark==1){
    Seconds=ClockIntervalSec(_ProgEnd,_ProgStart);
    Measure=ToString(Seconds,"%0.5f")+"s";
  }

  //Calculate total in benchmark modes 2 & 3
  else{
    Seconds=ClockIntervalSec(_ProgEnd,_ProgStart);
    Speed=((double)_InstCount/Seconds)/1000000;
    AvgInstTime=(Seconds/_InstCount)*(double)1000000000;
    Measure=ToString(Seconds,"%0.5f")+"s, "+ToString(_InstCount)+" instructions, "+ToString(Speed,"%0.5f")+" MIPS ("+_GetScaledTime(AvgInstTime)+"/instr.)";
  }

  //Output total benchmark
  _Stl->Console.PrintLine("DS Benchmark: "+Measure);
  
}

//Print replication rule
String Runtime::_RpRulePrint(int Rule){
  String RuleDesc;
  if(_RpRule[Rule].Mode==ReplicRuleMode::Fixed){
    RuleDesc="mode=Fix baseoffset="+ToString(_RpRule[Rule].BaseOffset)+" arrgeom="+HEXFORMAT(_RpRule[Rule].ArrGeom)+" cellsize="+ToString(_RpRule[Rule].CellSize)+
    " elements="+ToString(_RpRule[Rule].Elements)+" sourcepnt="+PTRFORMAT(_RpRule[Rule].SourcePnt)+" destinpnt="+PTRFORMAT(_RpRule[Rule].DestinPnt);
  }
  else{
    RuleDesc="mode=dyn baseoffset="+ToString(_RpRule[Rule].BaseOffset)+" cellsize="+ToString(_RpRule[Rule].CellSize)+" elements="+ToString(_RpRule[Rule].Elements)+
    " sourceblock="+HEXFORMAT(_RpRule[Rule].SourceBlock)+" destinblock="+HEXFORMAT(_RpRule[Rule].DestinBlock)+
    " sourcepnt="+PTRFORMAT(_RpRule[Rule].SourcePnt)+" destinpnt="+PTRFORMAT(_RpRule[Rule].DestinPnt);
  }
  return RuleDesc;
}

//Inner block replication (go to first element)
void Runtime::_RpGotoFirstElement(int Rule){
  _RpRule[Rule].Index=0;
  _RpRule[Rule].End=false;
  _RpRule[Rule].SourcePnt=nullptr;
  _RpRule[Rule].DestinPnt=nullptr;
  _RpRule[Rule].SourceBlock=0;
  _RpRule[Rule].DestinBlock=0;
  if(_RpRule[Rule].Mode==ReplicRuleMode::Fixed){
    if(Rule==0){
      _RpRule[Rule].SourcePnt=_RpSource+_RpRule[Rule].BaseOffset;
      _RpRule[Rule].DestinPnt=_RpDestin+_RpRule[Rule].BaseOffset;  
    }
    else{
      if(_RpRule[Rule-1].SourcePnt!=nullptr){
        _RpRule[Rule].SourcePnt=_RpRule[Rule-1].SourcePnt+_RpRule[Rule].BaseOffset;
        _RpRule[Rule].DestinPnt=_RpRule[Rule-1].DestinPnt+_RpRule[Rule].BaseOffset;
      }
    }
    _RpRule[Rule].CellSize=_ArC.FixGetCellSize(_RpRule[Rule].ArrGeom);
    _RpRule[Rule].Elements=_ArC.FixGetElements(_RpRule[Rule].ArrGeom);
  }
  else if(_RpRule[Rule].Mode==ReplicRuleMode::Dynamic){
    if(Rule==0){
      _RpRule[Rule].SourceBlock=*(CpuMbl *)(_RpSource+_RpRule[Rule].BaseOffset);
      _RpRule[Rule].DestinBlock=*(CpuMbl *)(_RpDestin+_RpRule[Rule].BaseOffset);  
    }
    else{
      if(_RpRule[Rule-1].SourcePnt!=nullptr){
        _RpRule[Rule].SourceBlock=*(CpuMbl *)(_RpRule[Rule-1].SourcePnt+_RpRule[Rule].BaseOffset);
        _RpRule[Rule].DestinBlock=*(CpuMbl *)(_RpRule[Rule-1].DestinPnt+_RpRule[Rule].BaseOffset);
      }
    }
    _RpRule[Rule].SourcePnt=(_RpRule[Rule].SourceBlock!=0?_Aux.CharPtr(_RpRule[Rule].SourceBlock):nullptr);
    _RpRule[Rule].DestinPnt=(_RpRule[Rule].DestinBlock!=0?_Aux.CharPtr(_RpRule[Rule].DestinBlock):nullptr);
    _RpRule[Rule].CellSize=_ArC.DynGetCellSize(_RpRule[Rule].SourceBlock);
    _RpRule[Rule].Elements=_ArC.DynGetElements(_RpRule[Rule].SourceBlock);
  }
  DebugMessage(DebugLevel::VrmInnerBlockRpl,"Goto first element on rule: "+_RpRulePrint(Rule));
}

//Inner block replication (go to next element)
void Runtime::_RpGotoNextElement(int Rule){
  DebugMessage(DebugLevel::VrmInnerBlockRpl,"Entered goto next element on rule "+ToString(Rule));
  if(_RpRule[Rule].Index==_RpRule[Rule].Elements-1){
    _RpRule[Rule].End=true;
    if(Rule==0){
      DebugMessage(DebugLevel::VrmInnerBlockRpl,"Exit goto next element on rule "+ToString(Rule)+": Last element reached");
      return;
    }
    else{
      _RpGotoNextElement(Rule-1);
      _RpGotoFirstElement(Rule);
    }
  }
  else{
    _RpRule[Rule].Index++;
    _RpRule[Rule].SourcePnt+=_RpRule[Rule].CellSize;
    _RpRule[Rule].DestinPnt+=_RpRule[Rule].CellSize;
  }
  DebugMessage(DebugLevel::VrmInnerBlockRpl,"Exit goto next element on rule: "+_RpRulePrint(Rule));
  return;
}

//Inner block replication
bool Runtime::_InnerBlockReplication(CpuWrd Offset,bool ForString,bool ForArray){

  //Variables
  int i;

  //Debug message
  DebugMessage(DebugLevel::VrmInnerBlockRpl,"Entered inner block replication for "+String(ForString?"string":"")+String(ForArray?"array":"")+" at offset "+ToString(Offset));
  
  //Replication without rules
  if(_RpRule.Length()==0){
    DebugMessage(DebugLevel::VrmInnerBlockRpl,"Replication for element at offset "+ToString(Offset)+" (srcblock="+HEXFORMAT(*(CpuMbl *)(_RpSource+Offset))+") without rules");
    *(CpuMbl *)(_RpDestin+Offset)=0;
    if(ForString){     if(!_StC.SCOPY((CpuMbl *)(_RpDestin+Offset),*(CpuMbl *)(_RpSource+Offset))){ return false; } }
    else if(ForArray){ if(!_ArC.ACOPY((CpuMbl *)(_RpDestin+Offset),*(CpuMbl *)(_RpSource+Offset))){ return false; } }
    DebugMessage(DebugLevel::VrmInnerBlockRpl,"Inner block replication completed (dstblock="+HEXFORMAT(*(CpuMbl *)(_RpDestin+Offset))+")");
    return true;
  }

  //Prepare calculations (rewind all rules)
  DebugMessage(DebugLevel::VrmInnerBlockRpl,"Rewind all rules");
  for(i=0;i<_RpRule.Length();i++){
    _RpGotoFirstElement(i);
    if(_RpRule[i].Elements==0){
      DebugMessage(DebugLevel::VrmInnerBlockRpl,"Rule "+ToString(i)+" has zero elements, replication finished");
      return true;
    }
  }

  //Source and destination blocks (for messages)
  #ifdef __DEV__
  String SrcBlocks;
  String DstBlocks;
  #endif

  //Replication loop
  do{

    //Calculate next element to replicate
    #ifdef __DEV__
    String RuleIndexes="[";
    for(i=0;i<_RpRule.Length();i++){
      RuleIndexes+=(i!=0?",":"")+ToString(_RpRule[i].Index)+(_RpRule[i].End?"End":"");
    }
    RuleIndexes+="]";
    #endif

    //Replicate most inner block at current position
    //(replication does only happen at the most inner array, in the last replication rule)
    if(_RpRule[_RpRule.Length()-1].SourcePnt!=nullptr){
      *(CpuMbl *)(_RpRule[_RpRule.Length()-1].DestinPnt+Offset)=0;
      DebugMessage(DebugLevel::VrmInnerBlockRpl,"Replication for element at offset "+ToString(Offset)+" (srcblock="+HEXFORMAT(*(CpuMbl *)(_RpRule[_RpRule.Length()-1].SourcePnt+Offset))+" indexes="+RuleIndexes+")");
      if(ForString){     if(!_StC.SCOPY((CpuMbl *)(_RpRule[_RpRule.Length()-1].DestinPnt+Offset),*(CpuMbl *)(_RpRule[_RpRule.Length()-1].SourcePnt+Offset))){ return false; } }
      else if(ForArray){ if(!_ArC.ACOPY((CpuMbl *)(_RpRule[_RpRule.Length()-1].DestinPnt+Offset),*(CpuMbl *)(_RpRule[_RpRule.Length()-1].SourcePnt+Offset))){ return false; } }
      DebugMessage(DebugLevel::VrmInnerBlockRpl,"Block replicated (dstblock="+HEXFORMAT(*(CpuMbl *)(_RpRule[_RpRule.Length()-1].DestinPnt+Offset))+")");
      #ifdef __DEV__
      SrcBlocks+=HEXFORMAT(*(CpuMbl *)(_RpRule[_RpRule.Length()-1].SourcePnt+Offset))+" ";
      DstBlocks+=HEXFORMAT(*(CpuMbl *)(_RpRule[_RpRule.Length()-1].DestinPnt+Offset))+" ";
      #endif
    }

    //Calculate next element to replicate
    _RpGotoNextElement(_RpRule.Length()-1);

  }while(!_RpRule[0].End);

  //return code
  DebugMessage(DebugLevel::VrmInnerBlockRpl,"Inner block replication completed");
  DebugMessage(DebugLevel::VrmInnerBlockRpl,String(ForString?"String":"")+String(ForArray?"Array":"")+" source blocks level "+ToString(_RpRule.Length())+": "+SrcBlocks);
  DebugMessage(DebugLevel::VrmInnerBlockRpl,String(ForString?"String":"")+String(ForArray?"Array":"")+" destin blocks level "+ToString(_RpRule.Length())+": "+DstBlocks);
  return true;

}

//Initialization rule print
String Runtime::_BiRulePrint(int Rule){
  String RuleDesc;
  RuleDesc="mode=Fix baseoffset="+ToString(_BiRule[Rule].BaseOffset)+" arrgeom="+HEXFORMAT(_BiRule[Rule].ArrGeom)+" cellsize="+ToString(_BiRule[Rule].CellSize)+
  " elements="+ToString(_BiRule[Rule].Elements)+" destinpnt="+PTRFORMAT(_BiRule[Rule].DestinPnt);
  return RuleDesc;
}

//Initialization rule goto first element
void Runtime::_BiGotoFirstElement(int Rule){
  _BiRule[Rule].Index=0;
  _BiRule[Rule].End=false;
  _BiRule[Rule].DestinPnt=nullptr;
  _BiRule[Rule].DestinBlock=0;
  if(Rule==0){
    _BiRule[Rule].DestinPnt=_BiDestin+_BiRule[Rule].BaseOffset;  
  }
  else{
    _BiRule[Rule].DestinPnt=_BiRule[Rule-1].DestinPnt+_BiRule[Rule].BaseOffset;
  }
  _BiRule[Rule].CellSize=_ArC.FixGetCellSize(_BiRule[Rule].ArrGeom);
  _BiRule[Rule].Elements=_ArC.FixGetElements(_BiRule[Rule].ArrGeom);
  DebugMessage(DebugLevel::VrmInnerBlockInit,"Goto first element on rule: "+_BiRulePrint(Rule));
}

//Initialization rule goto next element
void Runtime::_BiGotoNextElement(int Rule){
  DebugMessage(DebugLevel::VrmInnerBlockInit,"Entered goto next element on rule "+ToString(Rule));
  if(_BiRule[Rule].Index==_BiRule[Rule].Elements-1){
    _BiRule[Rule].End=true;
    if(Rule==0){
      DebugMessage(DebugLevel::VrmInnerBlockInit,"Exit goto next element on rule "+ToString(Rule)+": Last element reached");
      return;
    }
    else{
      _BiGotoNextElement(Rule-1);
      _BiGotoFirstElement(Rule);
    }
  }
  else{
    _BiRule[Rule].Index++;
    _BiRule[Rule].DestinPnt+=_BiRule[Rule].CellSize;
  }
  DebugMessage(DebugLevel::VrmInnerBlockInit,"Exit goto next element on rule: "+_BiRulePrint(Rule));
  return;
}

//Inner block initialization
bool Runtime::_InnerBlockInitialization(CpuWrd Offset,bool ForString,bool ForArray,int DimNr,CpuWrd CellSize){

  //Variables
  int i;

  //Debug message
  DebugMessage(DebugLevel::VrmInnerBlockInit,"Entered inner block initialization for "+String(ForString?"string":"")+String(ForArray?"array":"")+" at offset "+ToString(Offset));
  
  //Initialization without rules
  if(_BiRule.Length()==0){
    DebugMessage(DebugLevel::VrmInnerBlockInit,"Initialization for element at offset "+ToString(Offset)+" without rules");
    *(CpuMbl *)(_BiDestin+Offset)=0;
    if(ForString){     if(!_StC.SEMP ((CpuMbl *)(_BiDestin+Offset))){ return false; } }
    else if(ForArray){ if(!_ArC.ADEMP((CpuMbl *)(_BiDestin+Offset),DimNr,CellSize)){ return false; } }
    DebugMessage(DebugLevel::VrmInnerBlockInit,"Inner block initialization completed (dstblock="+HEXFORMAT(*(CpuMbl *)(_BiDestin+Offset))+")");
    return true;
  }

  //Prepare calculations (rewind all rules)
  DebugMessage(DebugLevel::VrmInnerBlockInit,"Rewind all rules");
  for(i=0;i<_BiRule.Length();i++){
    _BiGotoFirstElement(i);
    if(_BiRule[i].Elements==0){
      DebugMessage(DebugLevel::VrmInnerBlockRpl,"Rule "+ToString(i)+" has zero elements, replication finished");
      return true;
    }
  }

  //Destination blocks (for messages)
  #ifdef __DEV__
  String DstBlocks;
  #endif

  //Initialization loop
  do{

    //Calculate next element to replicate
    #ifdef __DEV__
    String RuleIndexes="[";
    for(i=0;i<_BiRule.Length();i++){
      RuleIndexes+=(i!=0?",":"")+ToString(_BiRule[i].Index)+(_BiRule[i].End?"End":"");
    }
    RuleIndexes+="]";
    #endif

    //Initialize most inner block at current position
    //(initialization does only happen at the most inner array, in the last initialization rule)
    *(CpuMbl *)(_BiRule[_BiRule.Length()-1].DestinPnt+Offset)=0;
    DebugMessage(DebugLevel::VrmInnerBlockInit,"Initialization for element at offset "+ToString(Offset)+" (indexes="+RuleIndexes+")");
    if(ForString){     if(!_StC.SEMP ((CpuMbl *)(_BiRule[_BiRule.Length()-1].DestinPnt+Offset))){ return false; } }
    else if(ForArray){ if(!_ArC.ADEMP((CpuMbl *)(_BiRule[_BiRule.Length()-1].DestinPnt+Offset),DimNr,CellSize)){ return false; } }
    DebugMessage(DebugLevel::VrmInnerBlockInit,"Block initialized (dstblock="+HEXFORMAT(*(CpuMbl *)(_BiRule[_BiRule.Length()-1].DestinPnt+Offset))+")");
    #ifdef __DEV__
    DstBlocks+=HEXFORMAT(*(CpuMbl *)(_BiRule[_BiRule.Length()-1].DestinPnt+Offset))+" ";
    #endif

    //Calculate next element to replicate
    _BiGotoNextElement(_BiRule.Length()-1);

  }while(!_BiRule[0].End);

  //return code
  DebugMessage(DebugLevel::VrmInnerBlockInit,"Inner block initialization completed");
  DebugMessage(DebugLevel::VrmInnerBlockInit,String(ForString?"String":"")+String(ForArray?"Array":"")+" destin blocks level "+ToString(_BiRule.Length())+": "+DstBlocks);
  return true;

}


//Windows get last error function
#ifdef __WIN__
String _GetLastError(){
  String Result;
  CpuInt IntError;
  HRESULT WinError;
  LPTSTR ErrorText = NULL;
  WinError=GetLastError();
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_IGNORE_INSERTS,  
  NULL,WinError,MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),(LPTSTR)&ErrorText,0,NULL);
  if(ErrorText!=NULL){
    IntError=WinError;
    Result=HEXVALUE(IntError)+": "+String(ErrorText);
    LocalFree(ErrorText);
    ErrorText=NULL;
  }
  else{
    IntError=WinError;
    Result=HEXVALUE(IntError);
  }  
  return Result.Replace(LINE_END," ");
}

//Linux check file descriptor readable status
#else
bool _GetFdStatus(int Fd,int MaxTimeInSeconds,FdStatus &Status){

  //Variables
  struct pollfd Fds[2];
  int RetCode;

  //Watch fd for read
  Fds[0].fd=Fd;
  Fds[0].events=POLLIN;
  RetCode=poll(Fds,1,MaxTimeInSeconds*1000);

  //Poll error
  if(RetCode<0){
    return false;
  }

  //Timeout
  else if(RetCode==0){
    Status=FdStatus::Timeout;
  }

  //Success
  else{
    if(Fds[0].revents&POLLIN){
      Status=FdStatus::Readable;
    }
    else if(Fds[0].revents&POLLHUP){
      Status=FdStatus::Closed;
    }
    else{
      Status=FdStatus::Error;
    }
  }

  //Return code
  return true;

}
#endif

//Execute external program
bool Runtime::_ExecuteExternal(CpuMbl ExecutableFile,CpuMbl Arguments,CpuMbl *StdOut,CpuMbl *StdErr,bool Redirect,bool ArgIsArray){

  //Variables
  CpuWrd Lines;
  String Command;
  CpuMbl Arg=0;
  CpuMbl Delimiter=0;
  CpuMbl OutResult=0;
  CpuMbl ErrResult=0;
  CpuWrd OutLength=0;
  CpuWrd ErrLength=0;
  bool MemoryError=false;
  char OutBuff[1024];
  char ErrBuff[1024];

  //Debug message
  DebugMessage(DebugLevel::VrmRuntime, "Entered execute external program interface. exec="+String(_StC.CharPtr(ExecutableFile))
  +" arg="+String(_StC.CharPtr(Arguments))+" redirect="+(Redirect?String("yes"):String("no"))+" argisarray="+(ArgIsArray?String("yes"):String("no")));

  //Check executable file exists
  if(!_Stl->FileSystem.FileExists(String(_StC.CharPtr(ExecutableFile)))){
    _Stl->SetError(StlErrorCode::ExecFileNotExists,_StC.CharPtr(ExecutableFile));
    return false; 
  }

  //Get command string (from string[] array)
  Command=String(_StC.CharPtr(ExecutableFile));
  if(ArgIsArray){
    if(!_ArC.STAOPR(Arguments,&Lines)){ return false; }
    for(int i=0;i<Lines;i++){
      if(!_ArC.STARDL(i,&Arg)){ return false; }
      Command+=" \""+String(_StC.CharPtr(Arg))+"\"";
    }
    if(!_ArC.STACLO()){ return false; }
  }
  else{
    Command+=" "+String(_StC.CharPtr(Arguments));
  }
  if(Redirect){ Command+=" " REDIRCOMMAND; }
  DebugMessage(DebugLevel::VrmRuntime, "Command: "+Command);

  //Windows version ----------------------------------------------------------------
  #ifdef __WIN__

  //Declare pipe handlers
  HANDLE PipeStdOutRead=0, PipeStdOutWrite=0;
  HANDLE PipeStdErrRead=0, PipeStdErrWrite=0;

  //Set pipe security (Pipe handles are inherited by child process)
  SECURITY_ATTRIBUTES saAttr={sizeof(SECURITY_ATTRIBUTES)};
  saAttr.bInheritHandle=TRUE; 
  saAttr.lpSecurityDescriptor=NULL;

  //Create a pipe to get results from child's stdout
  DebugMessage(DebugLevel::VrmRuntime, "Windows: Create pipe for stdout");
  if(!CreatePipe(&PipeStdOutRead,&PipeStdOutWrite,&saAttr,0)){
    System::Throw(SysExceptionCode::CannotCreatePipe,_GetLastError());
    return false; 
  }

  //Create a pipe to get results from child's stderr
  if(!Redirect){
    DebugMessage(DebugLevel::VrmRuntime, "Windows: Create pipe for stderr");
    if(!CreatePipe(&PipeStdErrRead,&PipeStdErrWrite,&saAttr,0)){
      System::Throw(SysExceptionCode::CannotCreatePipe,_GetLastError());
      return false; 
    }
  }

  //Process info structures
  STARTUPINFO si={sizeof(STARTUPINFO)};
  si.dwFlags    =STARTF_USESHOWWINDOW|STARTF_USESTDHANDLES;
  si.hStdOutput =PipeStdOutWrite;
  si.hStdError  =(Redirect?PipeStdOutWrite:PipeStdErrWrite);
  si.wShowWindow=SW_HIDE; 
  PROCESS_INFORMATION pi={0};

  //Create process
  DebugMessage(DebugLevel::VrmRuntime, "Windows: Create process");
  BOOL fSuccess=CreateProcess(NULL,Command.CharPnt(),NULL,NULL,TRUE,CREATE_NEW_CONSOLE,NULL,NULL,&si,&pi);
  if(!fSuccess){
    String LastError=_GetLastError();
    CloseHandle(PipeStdOutWrite);
    CloseHandle(PipeStdOutRead);
    if(!Redirect){
      CloseHandle(PipeStdErrWrite);
      CloseHandle(PipeStdErrRead);
    }
    System::Throw(SysExceptionCode::UnableToCreateProcess,LastError);
    return false; 
  }

  //Get process output
  bool ProcessEnded=false;
  memset((void *)OutBuff,0,sizeof(OutBuff));
  memset((void *)ErrBuff,0,sizeof(ErrBuff));
  DebugMessage(DebugLevel::VrmRuntime, "Windows: Read process output");
  while(!ProcessEnded){

    //Give some timeslice (50 ms), so we won't waste 100% CPU.
    ProcessEnded=(WaitForSingleObject(pi.hProcess,50)==WAIT_OBJECT_0);

    //Read loop variables
    DWORD ReadLength;
    DWORD Available;
    DWORD MaxLength;
    
    //Get output from stdout
    do{
      ReadLength = 0;
      Available = 0;
      MaxLength = 0;
      DebugMessage(DebugLevel::VrmRuntime, "Windows: Peek stdout pipe");
      if(!PeekNamedPipe(PipeStdOutRead, NULL, 0, NULL, &Available, NULL)){ break; }
      if(!Available){ break; }
      MaxLength=(sizeof(OutBuff)-1<Available?sizeof(OutBuff)-1:Available);
      DebugMessage(DebugLevel::VrmRuntime, "Windows: Read stdout pipe");
      if(!ReadFile(PipeStdOutRead,OutBuff,MaxLength,&ReadLength,NULL) || !ReadLength){ break; }
      DebugMessage(DebugLevel::VrmRuntime, "Windows: Read "+ToString((CpuInt)ReadLength)+" bytes from stdout");
      OutBuff[ReadLength] = 0;
      if(OutResult==0){
        if(!_Aux.Alloc(_ScopeId,_ScopeNr,ReadLength+1,-1,&OutResult)){ MemoryError=true; break; }
        _Aux.SetLen(OutResult,ReadLength); //+1)
      }
      else{
        if(!_Aux.Realloc(_ScopeId,_ScopeNr,OutResult,_Aux.GetLen(OutResult)+ReadLength)){ MemoryError=true; break; }
        _Aux.SetLen(OutResult,_Aux.GetLen(OutResult)+ReadLength);
      }
      MemCpy((void *)&_Aux.CharPtr(OutResult)[OutLength],OutBuff,ReadLength);
      OutLength+=ReadLength;
    }while(true);

    //Get output from stderr
    if(!Redirect){
      do{
        ReadLength = 0;
        Available = 0;
        MaxLength = 0;
        DebugMessage(DebugLevel::VrmRuntime, "Windows: Peek stderr pipe");
        if(!PeekNamedPipe(PipeStdErrRead, NULL, 0, NULL, &Available, NULL)){ break; }
        if(!Available){ break; }
        MaxLength=(sizeof(ErrBuff)-1<Available?sizeof(ErrBuff)-1:Available);
        DebugMessage(DebugLevel::VrmRuntime, "Windows: Read stderr pipe");
        if(!ReadFile(PipeStdErrRead,ErrBuff,MaxLength,&ReadLength,NULL) || !ReadLength){ break; }
        DebugMessage(DebugLevel::VrmRuntime, "Windows: Read "+ToString((CpuInt)ReadLength)+" bytes from stderr");
        ErrBuff[ReadLength] = 0;
        if(ErrResult==0){
          if(!_Aux.Alloc(_ScopeId,_ScopeNr,ReadLength+1,-1,&ErrResult)){ MemoryError=true; break; }
          _Aux.SetLen(ErrResult,ReadLength); //+1)
        }
        else{
          if(!_Aux.Realloc(_ScopeId,_ScopeNr,ErrResult,_Aux.GetLen(ErrResult)+ReadLength)){ MemoryError=true; break; }
          _Aux.SetLen(ErrResult,_Aux.GetLen(ErrResult)+ReadLength);
        }
        MemCpy((void *)&_Aux.CharPtr(ErrResult)[ErrLength],ErrBuff,ReadLength);
        ErrLength+=ReadLength;
      }while(true);
    }

  }
  
  //Close output buffers
  if(OutResult!=0){ _Aux.CharPtr(OutResult)[OutLength]=0; }
  else{ if(!_Aux.EmptyAlloc(_ScopeId,_ScopeNr,&OutResult)){ MemoryError=true; } }
  if(!Redirect){
    if(ErrResult!=0){ _Aux.CharPtr(ErrResult)[ErrLength]=0; }
    else{ if(!_Aux.EmptyAlloc(_ScopeId,_ScopeNr,&ErrResult)){ MemoryError=true; } }
  }

  //Close everything
  DebugMessage(DebugLevel::VrmRuntime, "Windows: Close all handlers");
  CloseHandle(PipeStdOutWrite);
  CloseHandle(PipeStdOutRead);
  if(!Redirect){
    CloseHandle(PipeStdErrWrite);
    CloseHandle(PipeStdErrRead);
  }
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  //Linux version ------------------------------------------------------------------
  #else
    
    //Constants
    #define PIPEOUT 0
    #define PIPEIN  1
    #define FDPOLLTIMEOUT 10
    
    //Variables
    pid_t Pid;
    int PidStatus;
    int FdPipeOut[2];
    int FdPipeErr[2];
    ssize_t ReadLength;
    FdStatus Status;

    //Create pipes for stderr and stdout
    DebugMessage(DebugLevel::VrmRuntime, "Linux: Crete pipe for stdout");
    if(pipe(FdPipeOut)==-1){
      System::Throw(SysExceptionCode::CannotCreatePipe);
      return false; 
    }
    if(!Redirect){
      DebugMessage(DebugLevel::VrmRuntime, "Linux: Crete pipe for stderr");
      if(pipe(FdPipeErr)==-1){
        System::Throw(SysExceptionCode::CannotCreatePipe);
        return false; 
      }
    }
    
    //Fork current process
    DebugMessage(DebugLevel::VrmRuntime, "Linux: Fork process");
    Pid=fork();
    if(Pid==-1) {
      System::Throw(SysExceptionCode::UnableToCreateProcess);
      return false; 
    } 

    //Child process
    else if(Pid==0){
      
      //Message
      DebugMessage(DebugLevel::VrmRuntime, "Linux child process: Execute command -> "+Command);
      
      //Redirect stdout / stderr to pipes
      while((dup2(FdPipeOut[PIPEIN],STDOUT_FILENO)==-1) && (errno==EINTR)){}
      if(!Redirect){ while((dup2(FdPipeErr[PIPEIN],STDERR_FILENO)==-1) && (errno==EINTR)){} }

      //Close original pipe in ends
      close(FdPipeOut[PIPEIN]); 
      if(!Redirect){ close(FdPipeErr[PIPEIN]); }

      //Close pipe outs as they are not used by process
      close(FdPipeOut[PIPEOUT]); 
      if(!Redirect){ close(FdPipeErr[PIPEOUT]); }
      
      //Call external program
      execl("/bin/sh","sh","-c",Command.CharPnt(),(char*)0);

      //Terminate child
      _exit(1);
    
    }

    //Close pipe inputs as they will not be used by parent
    DebugMessage(DebugLevel::VrmRuntime, "Linux: Close stdout pipe input");
    close(FdPipeOut[PIPEIN]); 
    if(!Redirect){ 
      DebugMessage(DebugLevel::VrmRuntime, "Linux: Close stderr pipe input");
      close(FdPipeErr[PIPEIN]); 
    }
   
    //Wait for child process to terminate
    DebugMessage(DebugLevel::VrmRuntime, "Linux: Wait for child process "+ToString(Pid));
    if(waitpid(Pid,&PidStatus,0)==-1){
      System::Throw(SysExceptionCode::ErrorWaitingChild);
      return false; 
    }

    //Read stdout from child process
    do{
      DebugMessage(DebugLevel::VrmRuntime, "Linux: Get stdout pipe status");
      if(!_GetFdStatus(FdPipeOut[PIPEOUT],FDPOLLTIMEOUT,Status)){
        System::Throw(SysExceptionCode::UnableToCheckFdStatus,"stdout");
        return false; 
      }
      else if(Status==FdStatus::Timeout){
        _Stl->SetError(StlErrorCode::ReadTimeout,"stdout");
        return false; 
      }
      else if(Status==FdStatus::Error){
        System::Throw(SysExceptionCode::FdStatusError,"stdout");
        return false; 
      }
      else if(Status==FdStatus::Closed){
        break; 
      }
      else if(Status==FdStatus::Readable){
        DebugMessage(DebugLevel::VrmRuntime, "Linux: Read stdout pipe");
        ReadLength=read(FdPipeOut[PIPEOUT],OutBuff,sizeof(OutBuff));
        if(ReadLength==-1){
          if(errno==EINTR){ continue; } 
          else{
            System::Throw(SysExceptionCode::ReadError,"stdout");
            return false; 
          }
        } 
        else if(ReadLength==0){
          break;
        } 
        else{
          DebugMessage(DebugLevel::VrmRuntime, "Linux: Read "+ToString((CpuInt)ReadLength)+" bytes from stdout");
          if(OutResult==0){
            if(!_Aux.Alloc(_ScopeId,_ScopeNr,ReadLength+1,-1,&OutResult)){ MemoryError=true; break; }
            _Aux.SetLen(OutResult,ReadLength); //+1)
          }
          else{
            if(!_Aux.Realloc(_ScopeId,_ScopeNr,OutResult,_Aux.GetLen(OutResult)+ReadLength)){ MemoryError=true; break; }
            _Aux.SetLen(OutResult,_Aux.GetLen(OutResult)+ReadLength);
          }
          MemCpy((void *)&_Aux.CharPtr(OutResult)[OutLength],OutBuff,ReadLength);
          OutLength+=ReadLength;
        }
      }
    }while(true);
    if(OutResult!=0){ _Aux.CharPtr(OutResult)[OutLength]=0; }
    else{ if(!_Aux.EmptyAlloc(_ScopeId,_ScopeNr,&OutResult)){ MemoryError=true; } }

    //Read stderr from child process
    if(!Redirect){
      do{
        DebugMessage(DebugLevel::VrmRuntime, "Linux: Get stderr pipe status");
        if(!_GetFdStatus(FdPipeErr[PIPEOUT],FDPOLLTIMEOUT,Status)){
          System::Throw(SysExceptionCode::UnableToCheckFdStatus,"stderr");
          return false; 
        }
        else if(Status==FdStatus::Timeout){
          _Stl->SetError(StlErrorCode::ReadTimeout,"stderr");
          return false; 
        }
        else if(Status==FdStatus::Error){
          System::Throw(SysExceptionCode::FdStatusError,"stdout");
          return false; 
        }
        else if(Status==FdStatus::Closed){
          break; 
        }
        else if(Status==FdStatus::Readable){
          DebugMessage(DebugLevel::VrmRuntime, "Linux: Read stderr pipe");
          ReadLength=read(FdPipeErr[PIPEOUT],ErrBuff,sizeof(ErrBuff));
          if(ReadLength==-1){
            if(errno==EINTR){ continue; } 
            else{
              System::Throw(SysExceptionCode::ReadError,"stderr");
              return false; 
            }
          } 
          else if(ReadLength==0){
            break;
          } 
          else{
            DebugMessage(DebugLevel::VrmRuntime, "Linux: Read "+ToString((CpuInt)ReadLength)+" bytes from stderr");
            if(ErrResult==0){
              if(!_Aux.Alloc(_ScopeId,_ScopeNr,ReadLength+1,-1,&ErrResult)){ MemoryError=true; break; }
              _Aux.SetLen(ErrResult,ReadLength); //+1)
            }
            else{
              if(!_Aux.Realloc(_ScopeId,_ScopeNr,ErrResult,_Aux.GetLen(ErrResult)+ReadLength)){ MemoryError=true; break; }
              _Aux.SetLen(ErrResult,_Aux.GetLen(ErrResult)+ReadLength);
            }
            MemCpy((void *)&_Aux.CharPtr(ErrResult)[ErrLength],ErrBuff,ReadLength);
            ErrLength+=ReadLength;
          }
        }
      }while(true);
      if(ErrResult!=0){ _Aux.CharPtr(ErrResult)[ErrLength]=0; }
      else{ if(!_Aux.EmptyAlloc(_ScopeId,_ScopeNr,&ErrResult)){ MemoryError=true; } }
    }
    
    //Close pipes
    DebugMessage(DebugLevel::VrmRuntime, "Linux: Close stdout pipe");
    close(FdPipeOut[PIPEOUT]);
    if(!Redirect){
      DebugMessage(DebugLevel::VrmRuntime, "Linux: Close stderr pipe");
      close(FdPipeErr[PIPEOUT]);
    }

  #endif

  //--------------------------------------------------------------------------------
  
  //Memory error
  if(MemoryError){
    System::Throw(SysExceptionCode::MemoryAllocationFailure);
    return false;
  }

  //Output results to log
  DebugMessage(DebugLevel::VrmRuntime, "StdOut: \""+NonAsciiEscape(String(_Aux.CharPtr(OutResult)))+"\"");
  if(!Redirect){
    DebugMessage(DebugLevel::VrmRuntime, "StdErr: \""+NonAsciiEscape(String(_Aux.CharPtr(ErrResult)))+"\"");
  }

  //Return results
  #ifdef __WIN__
  if(!_StC.SCOPY(&Delimiter,(char *)LINE_END)){ return false; }
  #else
  if(!_StC.SCOPY(&Delimiter,(char *)LINE_END)){ return false; }
  #endif
  DebugMessage(DebugLevel::VrmRuntime, "Create stdout string[] array");
  if(!_ArC.SSPL(StdOut,OutResult,Delimiter)){ return false; }
  DebugMessage(DebugLevel::VrmRuntime, "Created stdout array: "+_ToStringCpuMbl(*StdOut));
  if(!Redirect){
    DebugMessage(DebugLevel::VrmRuntime, "Create stderr string[] array");
    if(!_ArC.SSPL(StdErr,ErrResult,Delimiter)){ return false; }
    DebugMessage(DebugLevel::VrmRuntime, "Created sterr array: "+_ToStringCpuMbl(*StdErr));
  }
  else{
    if(!_ArC.AD1EM(StdErr,sizeof(CpuMbl))){ return false; }
  }

  //Return success
  DebugMessage(DebugLevel::VrmRuntime, "External execution successful");
  return true;

}

//Get dynamic library call description string
String Runtime::_DlCallStr(CpuInt DlCallId){
  return String((char *)_DynFun[DlCallId].DlFunction)+"("+String((char *)_DynLib[_DynFun[DlCallId].LibIndex].DlName)+")";
}

//Open dynamic library
bool Runtime::_OpenDynLibrary(const String& LibFile,void **Handler,DlFuncPtr *FuncPtr){
  
  //Variables
  String LibName;
  String TmpLibFile;
  char BuildNr[BUILDNRLEN+1];

  //Get library name
  LibName=_Stl->FileSystem.GetFileNameNoExt(LibFile);

  //Open library
  #ifdef __WIN__
    *Handler=LoadLibrary(LibFile.CharPnt());
  #else
    *Handler=dlopen(LibFile.CharPnt(),RTLD_LAZY);
  #endif
  if(*Handler==nullptr){
    System::Throw(SysExceptionCode::MissingDynamicLibrary,LibFile);
    return false;
  }

  //Get function pointers
  if(!_ScanDynLibrary(*Handler,LibFile,FuncPtr)){ return false; }

  //If we have a system library it is opened in shared space (nothing else to do)
  if(FuncPtr->IsSystemLibrary()){
    return true;
  }

  //If we have a user library it is opened in non shared space so we must make a temporary copy of the library for use of current program
  FuncPtr->GetBuildNumber(BuildNr);
  TmpLibFile=String(_TmpLibPath)+LibName+"-"+String(BuildNr)+"-"+String(_ProgName)+DYNLIB_EXT;
  if(!_Stl->FileSystem.FileExists(TmpLibFile)){
    std::ifstream Sour(LibFile.CharPnt(),std::ios::binary);
    std::ofstream Dest(TmpLibFile.CharPnt(),std::ios::binary);
    Dest << Sour.rdbuf();
    if(_Stl->FileSystem.FileExists(TmpLibFile)){
      System::Throw(SysExceptionCode::UserLibraryCopyFailed,LibFile,TmpLibFile);
      return false;
    }
  }

  //Reopen library from temporary file
  _CloseDynLibrary(*Handler);
  #ifdef __WIN__
    *Handler=LoadLibrary(TmpLibFile.CharPnt());
  #else
    *Handler=dlopen(TmpLibFile.CharPnt(),RTLD_LAZY);
  #endif
  if(*Handler==nullptr){
    System::Throw(SysExceptionCode::MissingDynamicLibrary,TmpLibFile);
    return false;
  }

  //Get function pointers
  if(!_ScanDynLibrary(*Handler,LibFile,FuncPtr)){ return false; }

  //Return code
  return true;

}

//Get function pointers in dynamiclibrary
bool Runtime::_ScanDynLibrary(void *Handler,const String& LibFile,DlFuncPtr *FuncPtr){

  //Variables
  String LibName;

  //Get library name
  LibName=_Stl->FileSystem.GetFileNameNoExt(LibFile);

  //Get library procedure addresses (windows)
  #ifdef __WIN__
    FuncPtr->IsSystemLibrary=(FunPtrIsSystemLibrary)GetProcAddress((HMODULE)Handler,"_IsSystemLibrary");
    FuncPtr->LibArchitecture=(FunPtrLibArchitecture)GetProcAddress((HMODULE)Handler,"_LibArchitecture");
    FuncPtr->GetBuildNumber=(FunPtrGetBuildNumber)GetProcAddress((HMODULE)Handler,"_GetBuildNumber");
    FuncPtr->InitDispatcher=(FunPtrInitDispatcher)GetProcAddress((HMODULE)Handler,"_InitDispatcher");
    FuncPtr->SearchFunction=(FunPtrSearchFunction)GetProcAddress((HMODULE)Handler,"_SearchFunction");
    FuncPtr->CallFunction=(FunPtrCallFunction)GetProcAddress((HMODULE)Handler,"_CallFunction");
    FuncPtr->SetDbgMsgInterf=(FunPtrSetDbgMsgInterf)GetProcAddress((HMODULE)Handler,"_SetDbgMsgInterf");
    FuncPtr->CloseDispatcher=(FunPtrCloseDispatcher)GetProcAddress((HMODULE)Handler,"_CloseDispatcher");

  //Get library procedure addresses (linux)
  #else
    FuncPtr->IsSystemLibrary=(FunPtrIsSystemLibrary)dlsym(Handler,"_IsSystemLibrary");
    FuncPtr->LibArchitecture=(FunPtrLibArchitecture)dlsym(Handler,"_LibArchitecture");
    FuncPtr->GetBuildNumber=(FunPtrGetBuildNumber)dlsym(Handler,"_GetBuildNumber");
    FuncPtr->InitDispatcher=(FunPtrInitDispatcher)dlsym(Handler,"_InitDispatcher");
    FuncPtr->SearchFunction=(FunPtrSearchFunction)dlsym(Handler,"_SearchFunction");
    FuncPtr->CallFunction=(FunPtrCallFunction)dlsym(Handler,"_CallFunction");
    FuncPtr->SetDbgMsgInterf=(FunPtrSetDbgMsgInterf)dlsym(Handler,"_SetDbgMsgInterf");
    FuncPtr->CloseDispatcher=(FunPtrCloseDispatcher)dlsym(Handler,"_CloseDispatcher");
  #endif

  //Check function pointers
  if(FuncPtr->IsSystemLibrary==nullptr || FuncPtr->LibArchitecture==nullptr 
  || FuncPtr->GetBuildNumber==nullptr  || FuncPtr->InitDispatcher==nullptr 
  || FuncPtr->SearchFunction==nullptr  || FuncPtr->CloseDispatcher==nullptr
  || FuncPtr->SetDbgMsgInterf==nullptr){
    if(FuncPtr->IsSystemLibrary==nullptr){ System::Throw(SysExceptionCode::MissingLibraryFunc,"_IsSystemLibrary",LibName); }
    if(FuncPtr->LibArchitecture==nullptr){ System::Throw(SysExceptionCode::MissingLibraryFunc,"_LibArchitecture",LibName); }
    if(FuncPtr->GetBuildNumber==nullptr){ System::Throw(SysExceptionCode::MissingLibraryFunc,"_GetBuildNumber",LibName); }
    if(FuncPtr->InitDispatcher==nullptr){ System::Throw(SysExceptionCode::MissingLibraryFunc,"_InitDispatcher",LibName); }
    if(FuncPtr->SearchFunction==nullptr){ System::Throw(SysExceptionCode::MissingLibraryFunc,"_SearchFunction",LibName); }
    if(FuncPtr->CallFunction==nullptr){ System::Throw(SysExceptionCode::MissingLibraryFunc,"_CallFunction",LibName); }
    if(FuncPtr->SetDbgMsgInterf==nullptr){ System::Throw(SysExceptionCode::MissingLibraryFunc,"_SetDbgMsgInterf",LibName); }
    if(FuncPtr->CloseDispatcher==nullptr){ System::Throw(SysExceptionCode::MissingLibraryFunc,"_CloseDispatcher",LibName); }
    return false;
  }

  //Return code
  return true;

}

//Close dynamic library
void Runtime::_CloseDynLibrary(void *Handler){
  #ifdef __WIN__
    FreeLibrary((HMODULE)Handler);
  #else
    dlclose(Handler);
  #endif
}

//Dynamic library call handler
void Runtime::_DlCallHandler(CpuInt DlCallId){

  //Variables
  int i;
  int PhyFunId;
  bool Result;
  void *VoidPtr;
  char **Error;

  //Find physical function id if it is not located already
  if(_DynFun[DlCallId].PhyFunId==-1){
    
    //Load library if not loaded already
    if(_DynLib[_DynFun[DlCallId].LibIndex].Handler==nullptr){
    
      //Function pointers to library manager functions
      void *Handler;
      DlFuncPtr FuncPtr;
      String LibFile=String(_DynLibPath)+String((char *)_DynLib[_DynFun[DlCallId].LibIndex].DlName)+DYNLIB_EXT;

      //Open library
      if(!_OpenDynLibrary(LibFile,&Handler,&FuncPtr)){ return; }
      DebugMessage(DebugLevel::SysDynLib,"Opened dynamic library "+LibFile);

      //Check library architecture matches
      if(GetArchitecture()!=FuncPtr.LibArchitecture()){
        System::Throw(SysExceptionCode::DynLibArchMissmatch,LibFile,ToString(FuncPtr.LibArchitecture()),ToString(GetArchitecture()));
        _CloseDynLibrary(Handler);
        DebugMessage(DebugLevel::SysDynLib,"Closed dynamic library "+LibFile);
        return;
      }

      //Call init dispatcher function
      DebugMessage(DebugLevel::SysDynLib,"Initializing call dispatcher initialized on dynamic library "+LibFile);
      if(!FuncPtr.InitDispatcher()){
        System::Throw(SysExceptionCode::DynLibInit1Failed,LibFile);
        _CloseDynLibrary(Handler);
        DebugMessage(DebugLevel::SysDynLib,"Closed dynamic library "+LibFile);
        return;
      }
      DebugMessage(DebugLevel::SysDynLib,"Call dispatcher initialized on dynamic library "+LibFile);

      //Set debug message interface
      DebugMessage(DebugLevel::SysDynLib,"Setting debug message interface on dynamic library "+LibFile);
      FuncPtr.SetDbgMsgInterf(&LibDebugMessage);

      //Call library initialization function (if it exists)
      if((PhyFunId=FuncPtr.SearchFunction((char *)"Init"))!=-1){
        DebugMessage(DebugLevel::SysDynLib,"Calling Init() procedure on dynamic library "+LibFile);
        VoidPtr=(void *)&Result;
        void *Params[3]={(void *)&VoidPtr,(void *)&_ProcessId,(void *)&Error};
        FuncPtr.CallFunction(PhyFunId,Params);
        if(!Result){
          System::Throw(SysExceptionCode::DynLibInit2Failed,LibFile,String(*Error));
         _CloseDynLibrary(Handler);
          DebugMessage(DebugLevel::SysDynLib,"Closed dynamic library "+LibFile);
          return;
        }
      }

      //Save library data into table
      _DynLib[_DynFun[DlCallId].LibIndex].Handler=Handler;
      _DynLib[_DynFun[DlCallId].LibIndex].SearchFunction=FuncPtr.SearchFunction;
      _DynLib[_DynFun[DlCallId].LibIndex].CallFunction=FuncPtr.CallFunction;
      _DynLib[_DynFun[DlCallId].LibIndex].CloseDispatcher=FuncPtr.CloseDispatcher;
  
    }
  
    //Search dl function name
    if((PhyFunId=_DynLib[_DynFun[DlCallId].LibIndex].SearchFunction((char *)_DynFun[DlCallId].DlFunction))==-1){
      System::Throw(SysExceptionCode::MissingLibraryFunc,String((char *)_DynFun[DlCallId].DlFunction),String((char *)_DynLib[_DynFun[DlCallId].LibIndex].DlName));
      return;
    }
    _DynFun[DlCallId].PhyFunId=PhyFunId;
    DebugMessage(DebugLevel::SysDynLib,"Function "+String((char *)_DynFun[DlCallId].DlFunction)+"() found with id "+ToString(PhyFunId)+" on dynamic library "+String((char *)_DynLib[_DynFun[DlCallId].LibIndex].DlName));

  }

  //Call function
  DebugMessage(DebugLevel::SysDynLib,"Calling "+String((char *)_DynFun[DlCallId].DlFunction)+"() on dynamic library "+String((char *)_DynLib[_DynFun[DlCallId].LibIndex].DlName));
  _DynLib[_DynFun[DlCallId].LibIndex].CallFunction(_DynFun[DlCallId].PhyFunId,(void **)_DlVPtr.Pnt());
  DebugMessage(DebugLevel::SysDynLib,"Called "+String((char *)_DynFun[DlCallId].DlFunction)+"() on dynamic library "+String((char *)_DynLib[_DynFun[DlCallId].LibIndex].DlName));

  //Update of strings and dyn arrays
  for(i=0;i<_DlParm.Length();i++){
    if(_DlParm[i].Update){
      if(_DlParm[i].IsString){
        DebugMessage(DebugLevel::SysDynLib,"Parameter "+ToString(i)+": Updating string block "+ToString(*_DlParm[i].Blk)+" from string \""+_DlParm[i].CharPtr+"\"");
        _StC.SCOPY(_DlParm[i].Blk,_DlParm[i].CharPtr);
      }
      else if(_DlParm[i].IsDynArray){
        DebugMessage(DebugLevel::SysDynLib,"Parameter "+ToString(i)+": Updating array block "+ToString(*_DlParm[i].Blk)+" for "+ToString(*_DlParm[i].DArray.len)+" elements");
        _ArC.ADVCP(_DlParm[i].Blk,(void *)_DlParm[i].DArray.ptr,*_DlParm[i].DArray.len);
      }
    }
  }

}

//Main
bool CallRuntime(const String& BinaryFile,CpuWrd MemoryUnitKB,CpuWrd StartUnits,CpuWrd ChunkUnits,bool LockMemory,int BenchMark,
                 const String& DynLibPath,const String& TmpLibPath,int ArgNr,char *Arg[],int ArgStart,RomFileBuffer *RomBuffer){

  //On windows redirection command is seen as additional argument 
  //We remove it to align with linux and only if appears in last place, because we assume it was added by _ExecuteExternal()
  #ifdef __WIN__
  if(strcmp(Arg[ArgNr-1],REDIRCOMMAND)==0){ ArgNr--; }
  #endif

  //Catch exceptions
  try{

    //Open debug log
    DebugOpen(RomBuffer!=nullptr?String(RomBuffer->FileName):BinaryFile,RUN_LOG_EXT);
  
    //Init memory manager
    if(!MemoryManager::Init(StartUnits,ChunkUnits,MemoryUnitKB*1024,LockMemory)){
      SysMessage(312).Print();
      DebugClose();
      return false;
    }

    //Open scope here to force program destructor being called at scope end
    {

      //Variables
      Runtime Prog;

      //Save pointer to rom buffer
      Prog.SetRomBuffer(RomBuffer);

      //Load executable file into execution environment
      if(!Prog.LoadProgram(BinaryFile,System::CurrentProcessId(),ArgNr,Arg,ArgStart)){ DebugClose(); return false; }
    
      //Set dynamic library path
      Prog.SetLibPaths(DynLibPath,TmpLibPath);

      //Set pointer to current runtime instance
      SetCurrentRuntime(&Prog);

      //Execute program
      if(!Prog.ExecProgram(System::CurrentProcessId(),BenchMark)){ DebugClose(); return false; }

      //Close all files and unload dynamic libraries
      Prog.CloseAllFiles();
      Prog.UnloadLibraries();

    }

    //Terminate memory manager
    MemoryManager::Terminate();

  }

  //Exception handler
  catch(BaseException& Ex){
    _Stl->Console.PrintLine(Ex.Description());
    DebugClose();
    return false;
  }

  //Close debug log
  DebugClose();

  //Return code
  return true;

}

//Disassemble file entry point
bool CallDisassembleFile(const String& BinaryFile,CpuWrd MemoryUnitKB,CpuWrd StartUnits,CpuWrd ChunkUnits,int ArgNr,char *Arg[],int ArgStart){

  //Catch exceptions
  try{

    //Init memory manager
    if(!MemoryManager::Init(StartUnits,ChunkUnits,MemoryUnitKB*1024,false)){
      return false;
    }

    //Load program and call disassembler
    //(Open scope here to force program destructor being called at scope end)
    {
      Runtime Prog;
      Prog.SetRomBuffer(nullptr);
      if(!Prog.LoadProgram(BinaryFile,System::CurrentProcessId(),ArgNr,Arg,ArgStart)){ return false; }
      if(!Prog.Disassemble()){ return false; }
    }

    //Terminate memory manager
    MemoryManager::Terminate();

  }

  //Exception handler
  catch(BaseException& Ex){
    _Stl->Console.PrintLine(Ex.Description());
    DebugClose();
    return false;
  }

  //Return code
  return true;

}

//Debug message for libraries
void LibDebugMessage(char *Msg){
  DebugMessage(DebugLevel::VrmRuntime,String(Msg));
}
