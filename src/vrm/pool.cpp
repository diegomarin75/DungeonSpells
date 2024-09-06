//Memman.cpp: Memory manager class
#include "bas/basedefs.hpp"
#include "bas/allocator.hpp"
#include "bas/array.hpp"
#include "bas/sortedarray.hpp"
#include "bas/allocator.hpp"
#include "bas/array.hpp"
#include "bas/stack.hpp"
#include "bas/buffer.hpp"
#include "bas/string.hpp"
#include "sys/sysdefs.hpp"
#include "sys/system.hpp"
#include "sys/stl.hpp"
#include "sys/msgout.hpp"
#include "vrm/pool.hpp"

//Memory check macro
#ifdef __DEV__
  #define MemoryCheckReturn(op) if(DebugLevelEnabled(DebugLevel::VrmMemPoolCheck)){ if(!MemoryCheck(op)){ return false; } }
  #define MemoryCheckVoid(op)   if(DebugLevelEnabled(DebugLevel::VrmMemPoolCheck)){ MemoryCheck(op); }
#else
  #define MemoryCheckReturn(op) 
  #define MemoryCheckVoid(op) 
#endif

//Block pointer (used only on memory check)
struct BlockPointer{
  BlockHeader *Ptr; 
  BlockHeader *SortKey() const { return Ptr; }
};

//Calculate minimun memory unit size
CpuWrd MemoryPool::MinUnitSize() const {
  CpuWrd MinSize=1;
  while(MinSize<(CpuWrd)(8*sizeof(BlockHeader))){ MinSize*=2; }
  return MinSize;
}

//Memory pool creation
bool MemoryPool::Create(CpuWrd Units,CpuWrd ChunkUnits,CpuWrd UnitSize,long FreeListNr,int FreeBits,int PoolOwner,bool Lock,FunPtrAlloc InnerAlloc,FunPtrFree InnerFree){

  //Variables
  int i;
  char *Memory;
  CpuWrd TotalMemory;
  CpuWrd FreeListMemory;
  BlockHeader Header;
  char *FreeList;  
  PageHeader PageHdr;

  //Save pool owner plus host allocator / releaser
  _PoolOwner=PoolOwner;
  _InnerAlloc=InnerAlloc;
  _InnerFree=InnerFree;

  //Debug message
  DebugMessage(DebugLevel::VrmMemPool,MEMORY_POOL_NAME(_PoolOwner)+
  "Init (memory_unit="+ToString(UnitSize)+
  " chunk_units="+ToString(ChunkUnits)+
  " memory_units="+ToString(Units)+
  " lock="+(Lock?"true":"false")+
  " blockheader="+ToString((int)sizeof(BlockHeader)));

  //Calculate minimun memory unit and check
  if(UnitSize<MinUnitSize()){
    _Error=MemPoolError::SmallUnitSize;
    return false; 
  }

  //Calculate total memory and reserve
  TotalMemory=Units*UnitSize+sizeof(PageHeader);
  try{ Memory=_InnerAlloc(TotalMemory,PoolOwner); } 
  catch(std::bad_alloc& Ex){ 
    _Error=MemPoolError::AllocationError;
    return false; 
  }

  //Reserve memory for free block list
  FreeListMemory=FreeListNr*sizeof(FreeBlock);
  try{ FreeList=_InnerAlloc(FreeListMemory,PoolOwner); } 
  catch(std::bad_alloc& Ex){ 
    _Error=MemPoolError::AllocationError;
    _InnerFree(Memory);
    return false; 
  }

  //Lock memory pages
  if(Lock){
    if(!_LockMemory(Memory,TotalMemory)){
      _Error=MemPoolError::PageLockFailure;
      _InnerFree(Memory);
      _InnerFree(FreeList);
      return false; 
    }
    if(!_LockMemory(FreeList,FreeListMemory)){
      _Error=MemPoolError::PageLockFailure;
      _InnerFree(Memory);
      _InnerFree(FreeList);
      return false; 
    }
  }

  //Set page header
  PageHdr.BlockNr=1;
  PageHdr.UsedNr=0;
  PageHdr.Units=Units;
  PageHdr.TotalMemory=TotalMemory;
  SetPageMarks(&PageHdr);
  MemCpy(Memory,(char *)&PageHdr,sizeof(PageHeader));
  
  //Set one big free block with size of entire memory pool
  Header.Used=false;
  Header.BlockOwner=PoolOwner;
  Header.PagePtr=reinterpret_cast<PageHeader *>(Memory);
  Header.Units=Units;
  Header.FreeIndex=-1;
  Header.Next=nullptr;
  Header.Prev=nullptr;
  SetBlockMarks(&Header);
  MemCpy(Memory+sizeof(PageHeader),(char *)&Header,sizeof(Header));

  //Init free list
  _FreeList=reinterpret_cast<FreeBlock *>(FreeList);
  _FreeListNr=FreeListNr;
  _FreeIndex=-1;
  _FreeBlocks=0;
  _FreeBits=FreeBits;
  for(i=0;i<_FreeListNr;i++){ _FreeList[i].Block=nullptr; }

  //Init rest of internal fields
  _List=reinterpret_cast<BlockHeader *>(Memory+sizeof(PageHeader));
  _Units=Units;
  _ChunkUnits=ChunkUnits;
  _UnitSize=UnitSize;
  _Lock=Lock;
  _Error=(MemPoolError)0;
  _BlockCount=1;
  
  //Add block into free list
  _FreeListAdd(_List);

  //Memory check failure flag
  #ifdef __DEV__
  _CheckFailure=false;
  #endif

  //Memory check
  MemoryCheckReturn(MemOperation::Init);

  //Debug message
  DebugMessage(DebugLevel::VrmMemPool,MEMORY_POOL_NAME(_PoolOwner)+"Initiated successfully");
  
  //Return code
  return(true);

}

//Memory pool destruction
void MemoryPool::Destroy(){

  //Variables
  PageHeader *LastPtr;
  BlockHeader *Header;
  Array<PageHeader *> Pages;
  CpuWrd BlockNr;
  CpuWrd UsedNr;
  CpuWrd Units;
  CpuWrd TotalMemory;
  
  //Debug message
  DebugMessage(DebugLevel::VrmMemPool,MEMORY_POOL_NAME(_PoolOwner)+"Destroy started");

  //Avoid destroy if memory check failure is detected
  #ifdef __DEV__
  if(_CheckFailure){
    DebugMessage(DebugLevel::VrmMemPool,MEMORY_POOL_NAME(_PoolOwner)+"Destruction request avoided since memory pool is in invalid state");
    return;
  }
  #endif
  
  //Memory check
  MemoryCheckVoid(MemOperation::Destroy);

  //Nothing to destroy if list pointer is null already
  if(_List==nullptr){
    DebugMessage(DebugLevel::VrmMemPool,MEMORY_POOL_NAME(_PoolOwner)+"Nothing to do, memory pool is already destroyed");
    return;
  }

  //Traverse block list to get all inner pointers to free
  BlockNr=0;
  UsedNr=0;
  Units=0;
  TotalMemory=0;
  LastPtr=nullptr;
  Header=_List;
  do{
    if(Header->PagePtr!=LastPtr){
      if(LastPtr!=nullptr){ DebugMessage(DebugLevel::VrmMemPool,MEMORY_POOL_NAME(_PoolOwner)+"Page "+PAGE_PTR_STRING(LastPtr)+" selected for release "
      " blocknr="+ToString(LastPtr->BlockNr)+" usednr="+ToString(LastPtr->UsedNr)+" units="+ToString(LastPtr->Units)+" totalmemory="+ToString(LastPtr->TotalMemory)); }
      Pages.Add(Header->PagePtr);
      LastPtr=Header->PagePtr;
      BlockNr+=Header->PagePtr->BlockNr;
      UsedNr+=Header->PagePtr->UsedNr;
      Units+=Header->PagePtr->Units;
      TotalMemory+=Header->PagePtr->TotalMemory;
    }
    if(Header->Next!=nullptr){
      Header=Header->Next;
    }
    else{
      if(LastPtr!=nullptr){ DebugMessage(DebugLevel::VrmMemPool,MEMORY_POOL_NAME(_PoolOwner)+"Page "+PAGE_PTR_STRING(LastPtr)+" selected for release"
      " blocknr="+ToString(LastPtr->BlockNr)+" usednr="+ToString(LastPtr->UsedNr)+" units="+ToString(LastPtr->Units)+" totalmemory="+ToString(LastPtr->TotalMemory)); }
      break;
    }
  }while(true);

  //Inform about allocated total
  DebugMessage(DebugLevel::VrmMemPool,MEMORY_POOL_NAME(_PoolOwner)+"Total allocated,"
  " blocknr="+ToString(BlockNr)+" usednr="+ToString(UsedNr)+" units="+ToString(Units)+" totalmemory="+ToString(TotalMemory)+" pages="+ToString(Pages.Length()));

  //Free pages
  for(int i=0;i<Pages.Length();i++){
    DebugMessage(DebugLevel::VrmMemPool,MEMORY_POOL_NAME(_PoolOwner)+"Free page "+PAGE_PTR_STRING(Pages[i]));
    _InnerFree(reinterpret_cast<char *>(Pages[i]));
  }

  //Release free block list
  DebugMessage(DebugLevel::VrmMemPool,MEMORY_POOL_NAME(_PoolOwner)+"Free page for free block list "+PTRFORMAT(_FreeList));
  _InnerFree(reinterpret_cast<char *>(_FreeList));

  //Last messafe
  DebugMessage(DebugLevel::VrmMemPool,MEMORY_POOL_NAME(_PoolOwner)+"Destroy completed");

}

//Internal memory allocator
bool MemoryPool::_Allocate(BlockHeader **Block,CpuWrd Size,int BlockOwner){

  //Variables
  bool Found;
  CpuWrd Count;
  CpuWrd MemUnits;
  BlockHeader *Header;
  BlockHeader *AuxHeader;

  //Calculate required memory units
  MemUnits=((Size+sizeof(BlockHeader))/_UnitSize)+1;

  //Debug message header
  #ifdef __DEV__
  String DebugMsgHeader;
  if(DebugLevelEnabled(DebugLevel::VrmMemPool)){
    DebugMsgHeader=MEMORY_POOL_NAME(_PoolOwner)+"Internal allocator, "+
    (*Block==nullptr?String("new allocation"):"allocation on existing block "+BLOCK_PTR_STRING(*Block))+" for size "+ToString(Size)+" (units="+ToString(MemUnits)+"): ";
  }
  #endif

  //If we have a reallocation check if block needs to be shortened, extended or it fits
  if(*Block!=nullptr){
    
    //Check if current block fits
    if((*Block)->Units==MemUnits){
      DebugMessage(DebugLevel::VrmMemPool,DebugMsgHeader+"Current block fits");
      MemoryCheckReturn(MemOperation::ReallocFits);
      return true;
    }

    //Check if current block has to be shortened
    else if((*Block)->Units>MemUnits){
    
      //Debug message
      DebugMessage(DebugLevel::VrmMemPool,DebugMsgHeader+"Current block is shortened");
      
      //Create new free block at end of current block
      Header=reinterpret_cast<BlockHeader *>(reinterpret_cast<char *>(*Block)+MemUnits*_UnitSize);
      Header->Used=false;
      Header->BlockOwner=_PoolOwner;
      Header->PagePtr=(*Block)->PagePtr;
      Header->Units=(*Block)->Units-MemUnits;
      Header->FreeIndex=-1;
      Header->Next=(*Block)->Next;
      Header->Prev=(*Block);
      if(Header->Next!=nullptr){ Header->Next->Prev=Header; }
      SetBlockMarks(Header);
      _FreeListAdd(Header);

      //Update block
      (*Block)->Units=MemUnits;
      (*Block)->Next=Header;
      _BlockCount++;
      (*Block)->PagePtr->BlockNr++;
      DebugMessage(DebugLevel::VrmMemPool,DebugMsgHeader+"block split "+_BlockSplitPrint(*Block)+" (blockcount="+ToString(_BlockCount)+")");
      MemoryCheckReturn(MemOperation::ReallocShortenFreeSplit);

      //If new free block has another free block as next one they must be joined
      if(Header->Next!=nullptr && Header->Next->Used==false && Header->Next->PagePtr==Header->PagePtr){
        DebugMessage(DebugLevel::VrmMemPool,DebugMsgHeader+"block join "+_BlockJoinPrint(Header)+" (blockcount="+ToString(_BlockCount-1)+")");
        _FreeListRemove(Header->Next);
        Header->Units+=Header->Next->Units;
        Header->Next=Header->Next->Next;
        if(Header->Next!=nullptr){ Header->Next->Prev=Header; }
        _BlockCount--;
        Header->PagePtr->BlockNr--;
        MemoryCheckReturn(MemOperation::ReallocShortenFreeJoin);
      }

      //Return
      return true;

    }

    //Check if current block can be extended
    else if((*Block)->Units<MemUnits){
      
      //Check if next block is a free block that can be used to satisfy memory request
      if((*Block)->Next!=nullptr 
      && (*Block)->Next->Used==false 
      && (*Block)->Units+(*Block)->Next->Units>=MemUnits 
      && (*Block)->Next->PagePtr==(*Block)->PagePtr){

        //Debug message
        DebugMessage(DebugLevel::VrmMemPool,DebugMsgHeader+"Current block is extended");
      
        //Join next block
        DebugMessage(DebugLevel::VrmMemPool,DebugMsgHeader+"block join "+_BlockJoinPrint(*Block)+" (blockcount="+ToString(_BlockCount-1)+")");
        _FreeListRemove((*Block)->Next);
        (*Block)->Units+=(*Block)->Next->Units;
        (*Block)->Next=(*Block)->Next->Next;
        if((*Block)->Next!=nullptr){ (*Block)->Next->Prev=(*Block); }
        _BlockCount--;
        (*Block)->PagePtr->BlockNr--;
        MemoryCheckReturn(MemOperation::ReallocWidenJoin);

        //If resulting block is bigger create a free block with rest of memory
        if((*Block)->Units>MemUnits){

          //Create new free block at end of current block
          Header=reinterpret_cast<BlockHeader *>(reinterpret_cast<char *>(*Block)+MemUnits*_UnitSize);
          Header->Used=false;
          Header->BlockOwner=_PoolOwner;
          Header->PagePtr=(*Block)->PagePtr;
          Header->Units=(*Block)->Units-MemUnits;
          Header->FreeIndex=-1;
          Header->Next=(*Block)->Next;
          Header->Prev=(*Block);
          if(Header->Next!=nullptr){ Header->Next->Prev=Header; }
          SetBlockMarks(Header);
          _FreeListAdd(Header);

          //Update block
          (*Block)->Units=MemUnits;
          (*Block)->Next=Header;
          _BlockCount++;
          (*Block)->PagePtr->BlockNr++;
          DebugMessage(DebugLevel::VrmMemPool,DebugMsgHeader+"block split "+_BlockSplitPrint(*Block)+" (blockcount="+ToString(_BlockCount)+")");
          MemoryCheckReturn(MemOperation::ReallocWidenFreeSplit);

          //If new free block has another free block as next one they must be joined
          if(Header->Next!=nullptr && Header->Next->Used==false && Header->Next->PagePtr==Header->PagePtr){
            DebugMessage(DebugLevel::VrmMemPool,DebugMsgHeader+"block join "+_BlockJoinPrint(Header)+" (blockcount="+ToString(_BlockCount-1)+")");
            _FreeListRemove(Header->Next);
            Header->Units+=Header->Next->Units;
            Header->Next=Header->Next->Next;
            if(Header->Next!=nullptr){ Header->Next->Prev=Header; }
            _BlockCount--;
            Header->PagePtr->BlockNr--;
            MemoryCheckReturn(MemOperation::ReallocWidenFreeJoin);
          }

        }

        //Return
        return true;
 
      }

    }

  }

  //Try to find a free block on free list
  Found=false;
  if(_FreeBlocks!=0){
    DebugMessage(DebugLevel::VrmMemPool,DebugMsgHeader+"Free block list search");
    CpuWrd i;
    int Distance=0;
    for(i=_FreeIndex;i>=0 && Distance<_FreeBits;i--,Distance++){
      //std::cout << "Search 1: index="+ToString(i)+" block="+BLOCK_PTR_STRING(_FreeList[i].Block) << std::endl;
      if(_FreeList[i].Block!=nullptr && _FreeList[i].Block->Used==false && _FreeList[i].Block->Units>=MemUnits){ Header=_FreeList[i].Block; Found=true; break; }
    }
    if(Found==false){
      for(i=_FreeListNr-1;i>_FreeIndex && Distance<_FreeBits;i--,Distance++){
        //std::cout << "Search 2: index="+ToString(i)+" block="+BLOCK_PTR_STRING(_FreeList[i].Block) << std::endl;
        if(_FreeList[i].Block!=nullptr && _FreeList[i].Block->Used==false && _FreeList[i].Block->Units>=MemUnits){ Header=_FreeList[i].Block; Found=true; break; }
      }
    }
  }

  //Traverse block list to find a suitable free block
  if(Found==false){
    DebugMessage(DebugLevel::VrmMemPool,DebugMsgHeader+"Full block list search");
    Count=0;
    Header=_List;
    do{
      //std::cout << "Search 3: block="+BLOCK_PTR_STRING(Header) << std::endl;
      if(Header->Units>=MemUnits && Header->Used==false){
        Found=true;
        break;
      }
      if(Header->Next!=nullptr){
        Header=Header->Next;
      }
      else{
        break;
      }
      Count++;
      if(Count>_BlockCount){
        DebugMessage(DebugLevel::VrmMemPool,DebugMsgHeader+"Block list traverse exceeds block count ("+ToString(_BlockCount)+")");
        break;
      }
    }while(true);
    if(!Found){ _Error=MemPoolError::RequestError; return false; }
  }
  
  //Found block satisfies memory request
  if(Header->Units==MemUnits){
    DebugMessage(DebugLevel::VrmMemPool,DebugMsgHeader+"Found block fits request");
    Header->Used=true;
    Header->BlockOwner=BlockOwner;
    _FreeListRemove(Header);
    if(*Block!=nullptr){ 
      MemCpy(reinterpret_cast<char*>(Header)+sizeof(BlockHeader),reinterpret_cast<char*>(*Block)+sizeof(BlockHeader),(*Block)->Units*_UnitSize-sizeof(BlockHeader)); 
      _Free(*Block);
    }
    Header->PagePtr->UsedNr++;
    *Block=Header;
    MemoryCheckReturn(MemOperation::AllocFits);
  }

  //Found block is bigger than memory request
  else if(Header->Units>MemUnits){

    //Debug message
    DebugMessage(DebugLevel::VrmMemPool,DebugMsgHeader+"Found block bigger than request");

    //Create new free block after found block
    AuxHeader=reinterpret_cast<BlockHeader *>(reinterpret_cast<char *>(Header)+MemUnits*_UnitSize);
    AuxHeader->Used=false;
    AuxHeader->BlockOwner=_PoolOwner;
    AuxHeader->PagePtr=Header->PagePtr;
    AuxHeader->Units=Header->Units-MemUnits;
    AuxHeader->FreeIndex=-1;
    AuxHeader->Next=Header->Next;
    AuxHeader->Prev=Header;
    if(AuxHeader->Next!=nullptr){ AuxHeader->Next->Prev=AuxHeader; }
    SetBlockMarks(AuxHeader);
    _FreeListAdd(AuxHeader);

    //Update found block
    Header->Used=true;
    Header->Units=MemUnits;
    Header->BlockOwner=BlockOwner;
    Header->Next=AuxHeader;
    _BlockCount++;
    Header->PagePtr->BlockNr++;
    Header->PagePtr->UsedNr++;
    _FreeListRemove(Header);
    DebugMessage(DebugLevel::VrmMemPool,DebugMsgHeader+"block split "+_BlockSplitPrint(Header)+" (blockcount="+ToString(_BlockCount)+")");
    MemoryCheckReturn(MemOperation::AllocFreeSplit);

    //If new free block has another free block as next one they must be joined
    if(AuxHeader->Next!=nullptr && AuxHeader->Next->Used==false && AuxHeader->Next->PagePtr==AuxHeader->PagePtr){
      DebugMessage(DebugLevel::VrmMemPool,DebugMsgHeader+"block join "+_BlockJoinPrint(AuxHeader)+" (blockcount="+ToString(_BlockCount-1)+")");
      _FreeListRemove(AuxHeader->Next);
      AuxHeader->Units+=AuxHeader->Next->Units;
      AuxHeader->Next=AuxHeader->Next->Next;
      if(AuxHeader->Next!=nullptr){ AuxHeader->Next->Prev=AuxHeader; }
      _BlockCount--;
      AuxHeader->PagePtr->BlockNr--;
      MemoryCheckReturn(MemOperation::AllocFreeJoin);
    }

    //Copy memory if we have a reallocation
    if(*Block!=nullptr){ 
      MemCpy(reinterpret_cast<char*>(Header)+sizeof(BlockHeader),reinterpret_cast<char*>(*Block)+sizeof(BlockHeader),(*Block)->Units*_UnitSize-sizeof(BlockHeader)); 
      _Free(*Block);
      MemoryCheckReturn(MemOperation::AllocFree);
    }

    //Return block
    *Block=Header;

  }

  //Return success
  return true;

}

//Internal memory releaser (block pointer must not be nullptr)
void MemoryPool::_Free(BlockHeader *Block){
  
  //Variables
  PageHeader *PagePtr;
  BlockHeader *KeptBlock;
  BlockHeader *Header;
  BlockHeader *First;
  BlockHeader *Last;

  //Debug message header
  #ifdef __DEV__
  String DebugMsgHeader;
  if(DebugLevelEnabled(DebugLevel::VrmMemPool)){
    DebugMsgHeader=MEMORY_POOL_NAME(_PoolOwner)+"Internal releaser, ";
  }
  #endif

  //Debug message
  DebugMessage(DebugLevel::VrmMemPool,DebugMsgHeader+"release on block "+BLOCK_PTR_STRING(Block));

  //Trow exception if block is already released
  if(Block->Used==false){
    String Message="Tried to free block "+BLOCK_PTR_STRING(Block)+" which is already released";
    ThrowBaseException((int)ExceptionSource::MemoryPool,(int)MemoryPoolException::FreeAlreadyReleased,Message.CharPnt());
  }    

  //Set block as free
  Block->Used=false;
  Block->FreeIndex=-1;
  Block->BlockOwner=_PoolOwner;
  Block->PagePtr->UsedNr--;
  _FreeListAdd(Block);
  MemoryCheckVoid(MemOperation::FreeRelease);
  KeptBlock=Block;

  //If next block is also a free block they must be joined
  if(Block->Next!=nullptr && Block->Next->Used==false && Block->Next->PagePtr==Block->PagePtr){
    DebugMessage(DebugLevel::VrmMemPool,DebugMsgHeader+"block join "+_BlockJoinPrint(Block)+" (blockcount="+ToString(_BlockCount-1)+")");
    _FreeListRemove(Block->Next);
    Block->Units+=Block->Next->Units;
    Block->Next=Block->Next->Next;
    if(Block->Next!=nullptr){ Block->Next->Prev=Block; }
    _BlockCount--;
    Block->PagePtr->BlockNr--;
    MemoryCheckVoid(MemOperation::FreeReleaseJoinNext);
    KeptBlock=Block;
  }

  //If previous block is also a free block they must be joined
  if(Block->Prev!=nullptr && Block->Prev->Used==false && Block->Prev->PagePtr==Block->PagePtr){
    DebugMessage(DebugLevel::VrmMemPool,DebugMsgHeader+"block join "+_BlockJoinPrint(Block->Prev)+" (blockcount="+ToString(_BlockCount-1)+")");
    _FreeListRemove(Block);
    Block->Prev->Units+=Block->Units;
    Block->Prev->Next=Block->Next;
    if(Block->Next!=nullptr){ Block->Next->Prev=Block->Prev; }
    _BlockCount--;
    Block->PagePtr->BlockNr--;
    MemoryCheckVoid(MemOperation::FreeReleaseJoinPrev);
    KeptBlock=Block->Prev;
  }

  //Release page if it does not countain any used blocks
  if(KeptBlock->PagePtr->UsedNr==0){

    //Debug message
    DebugMessage(DebugLevel::VrmMemPool,DebugMsgHeader+"page "+PAGE_PTR_STRING(KeptBlock->PagePtr)+" does not contain used blocks, selected for release");

    //Save page pointer
    PagePtr=KeptBlock->PagePtr;
    
    //Update free list
    _FreeListRemove(KeptBlock);
    _BlockCount--;

    //Get last block on previous page
    Header=KeptBlock;
    do{
      if(Header->PagePtr!=KeptBlock->PagePtr){
        Last=Header;
        break;
      }
      if(Header->Prev!=nullptr){
        if(Header!=KeptBlock){ _FreeListRemove(Header); _BlockCount--; }
        Header=Header->Prev;
      }
      else{
        Last=nullptr;
        break;
      }
    }while(true);
    DebugMessage(DebugLevel::VrmMemPool,DebugMsgHeader+"previous page last block is "+BLOCK_PTR_STRING(Last));

    //Get first block on next page
    Header=KeptBlock;
    do{
      if(Header->PagePtr!=KeptBlock->PagePtr){
        First=Header;
        break;
      }
      if(Header->Next!=nullptr){
        if(Header!=KeptBlock){ _FreeListRemove(Header); _BlockCount--; }
        Header=Header->Next;
      }
      else{
        First=nullptr;
        break;
      }
    }while(true);
    DebugMessage(DebugLevel::VrmMemPool,DebugMsgHeader+"next page first block is "+BLOCK_PTR_STRING(First));


    //Connect blocks
    DebugMessage(DebugLevel::VrmMemPool,DebugMsgHeader+"connect "+BLOCK_PTR_STRING(Last)+" to "+BLOCK_PTR_STRING(First));
    if(Last!=nullptr){ Last->Next=First; }
    if(First!=nullptr){ First->Prev=Last; }

    //Update list pointer
    if(_List->PagePtr==PagePtr){
      _List=First;
    }

    //Update total memory units
    _Units-=PagePtr->Units;

    //Memory check
    MemoryCheckVoid(MemOperation::PageRelease);
    #ifdef __DEV__
    if(_CheckFailure){ return; }
    #endif

    //Release page
    DebugMessage(DebugLevel::VrmMemPool,DebugMsgHeader+"release page "+PAGE_PTR_STRING(PagePtr));
    _InnerFree(reinterpret_cast<char *>(PagePtr));
    
  }

}

//Extend memory pool
bool MemoryPool::_Extend(CpuWrd Chunks){
  
  //Variables
  char *Memory;
  CpuWrd TotalMemory;
  BlockHeader *Header;
  PageHeader PageHdr;

  //Debug message header
  #ifdef __DEV__
  String DebugMsgHeader;
  if(DebugLevelEnabled(DebugLevel::VrmMemPool)){
    DebugMsgHeader=MEMORY_POOL_NAME(_PoolOwner)+"Memory pool extender, ";
  }
  #endif

  //Debug message
  DebugMessage(DebugLevel::VrmMemPool,DebugMsgHeader+"extension by "+ToString(Chunks*_ChunkUnits)+" memory units) ");

  //Get memory chunks
  TotalMemory=Chunks*_ChunkUnits*_UnitSize+sizeof(PageHeader);
  try{ Memory=_InnerAlloc(TotalMemory,_PoolOwner); } 
  catch(std::bad_alloc& Ex){ 
    _Error=MemPoolError::AllocationError;
    return false; 
  }

  //Lock memory page
  if(_Lock){
    if(!_LockMemory(Memory,TotalMemory)){
      _Error=MemPoolError::PageLockFailure;
      _InnerFree(Memory);
      return false; 
    }
  }

  //Set page header
  PageHdr.BlockNr=1;
  PageHdr.UsedNr=0;
  PageHdr.Units=Chunks*_ChunkUnits;
  PageHdr.TotalMemory=TotalMemory;
  SetPageMarks(&PageHdr);
  MemCpy(Memory,(char *)&PageHdr,sizeof(PageHeader));
  
  //Create free block at begining of list
  Header=reinterpret_cast<BlockHeader *>(Memory+sizeof(PageHeader));
  Header->Used=false;
  Header->BlockOwner=_PoolOwner;
  Header->PagePtr=reinterpret_cast<PageHeader *>(Memory);
  Header->Units=Chunks*_ChunkUnits;
  Header->FreeIndex=-1;
  Header->Next=_List;
  Header->Prev=nullptr;
  Header->Next->Prev=Header;
  SetBlockMarks(Header);
  _List=Header;
  _Units+=(Chunks*_ChunkUnits);
  _BlockCount++;
  DebugMessage(DebugLevel::VrmMemPool,DebugMsgHeader+"block extension ["+BLOCK_PTR_STRING(_List)+"] -> "+BLOCK_PTR_STRING(_List->Next)+" (blockcount="+ToString(_BlockCount)+")");

  //Update free list
  _FreeListAdd(Header);

  //Memory check
  MemoryCheckReturn(MemOperation::Extend);

  //Return success
  return true;

}

//Return last error
MemPoolError MemoryPool::LastError() const {
  return _Error;
}

//Return last error text
String MemoryPool::ErrorText(MemPoolError Error) const {
  String Text; 
  switch(Error){
    case MemPoolError::SmallUnitSize  : Text="Memory unit is too small (minimun unit size is "+ToString(MinUnitSize())+" bytes)"; break;
    case MemPoolError::AllocationError: Text="Memory alocation failure"; break;
    case MemPoolError::PageLockFailure: Text="Page lock failure"; break;
    case MemPoolError::RequestError   : Text="Unable to find a free block"; break;
  }
  return Text;
}

//Memory operation string
String MemoryPool::MemOperationText(MemOperation MemOper) const {
  String Text;
  switch(MemOper){
    case MemOperation::Init                   : Text="Init";                    break;
    case MemOperation::Destroy                : Text="Destroy";                 break;
    case MemOperation::ReallocFits            : Text="ReallocFits";             break;
    case MemOperation::ReallocShortenFreeSplit: Text="ReallocShortenFreeSplit"; break;
    case MemOperation::ReallocShortenFreeJoin : Text="ReallocShortenFreeJoin";  break;
    case MemOperation::ReallocWidenJoin       : Text="ReallocWidenJoin";        break;
    case MemOperation::ReallocWidenFreeJoin   : Text="ReallocWidenFreeJoin";    break;
    case MemOperation::ReallocWidenFreeSplit  : Text="ReallocWidenFreeSplit";   break;
    case MemOperation::AllocFits              : Text="AllocFits";               break;
    case MemOperation::AllocFreeJoin          : Text="AllocFreeJoin";           break;
    case MemOperation::AllocFreeSplit         : Text="AllocFreeSplit";          break;
    case MemOperation::AllocFree              : Text="AllocFree";               break;
    case MemOperation::FreeRelease            : Text="FreeRelease";             break;
    case MemOperation::FreeReleaseJoinNext    : Text="FreeReleaseJoinNext";     break;
    case MemOperation::FreeReleaseJoinPrev    : Text="FreeReleaseJoinPrev";     break;
    case MemOperation::Extend                 : Text="Extend";                  break;
    case MemOperation::OuterCheck             : Text="OuterCheck";              break;
    case MemOperation::PageRelease            : Text="PageRelease";             break;
  }
  return Text;
}

//Memory pool internal check
bool MemoryPool::MemoryCheck(MemOperation Oper){
  
  //Variables
  bool First;
  bool Last;
  bool Error;
  CpuWrd Units;
  CpuWrd Count;
  BlockHeader *Header;
  SortedArray<BlockPointer,BlockHeader *> SBlock;
  Array<BlockHeader *> TBlock;
  String Message;
 
  //Checks when list pointer is null
  if(_List==nullptr){ 
  
    //Init error flag
    Error=false;

    //Check memory unit count is zero
    if(!Error && _Units!=0){
      Message="Memory pool list pointer is null but unit count is not zero ("+ToString(_Units)+")";
      Error=true;
    }

    //Check block count is zero
    if(!Error && _BlockCount!=0){
      Message="Memory pool list pointer is null but block count is not zero ("+ToString(_BlockCount)+")";
      Error=true;
    }
    
    //Check free block bitmap is zero
    if(!Error && _FreeBlocks!=0){
      Message="Memory pool list pointer is null but free block bitmap is not zero ("+ToString(_FreeBlocks)+")";
      Error=true;
    }
    
    //Check all entries in free block list are null pointers
    if(!Error){
      for(int i=0;i<_FreeListNr;i++){ 
        if(_FreeList[i].Block!=nullptr){ 
          Message="Memory pool list pointer is null but free block list contains a non null pointer (freeindex="+ToString(i)+", block="+PTRFORMAT(_FreeList[i].Block)+")";
          Error=true;
          break;
        }
      }
    }

    //Send error
    if(Error){
      _CheckFailure=true;
      DebugMessage(DebugLevel::VrmMemPoolCheck,"Memory check failure ["+MEMORY_POOL_NAME(_PoolOwner).Replace(": ","")+"]: "+Message+" (memory operation: "+MemOperationText(Oper)+")");
      ThrowBaseException((int)ExceptionSource::MemoryPool,(int)MemoryPoolException::MemoryCheckError,Message.CharPnt());
    }

    //Return success
    return true; 
  
  }

  //Traverse the list and test all pointers 
  Count=0;
  Units=0;
  Error=false;
  Header=_List;
  do{

    //Check pointer loop
    if(SBlock.Search(Header)!=-1){
      Message="Pointer loop detected, block [#"+ToString(Count)+"] "+BLOCK_INFO(Header)+" was traversed already";
      Error=true;
      break;
    }

    //Save traversed blocks
    SBlock.Add((BlockPointer){Header});
    TBlock.Add(Header);

    //Detect block position
    if(Header==_List){ First=true; } else{ First=false; }
    if(Header->Next==nullptr){ Last=true; } else{ Last=false; }

    //Check page header marks
    #ifdef __DEV__
    if(Header->PagePtr->Mark1!=PAGE_MARK1){
      Message="On block [#"+ToString(Count)+"] "+BLOCK_INFO(Header)+" page header begining mark test fails (mark="+HEXFORMAT(Header->PagePtr->Mark1)+")"; 
      Error=true;
      break;
    }
    if(Header->PagePtr->Mark2!=PAGE_MARK2){
      Message="On block [#"+ToString(Count)+"] "+BLOCK_INFO(Header)+" page header ending mark test fails (mark="+HEXFORMAT(Header->PagePtr->Mark2)+")"; 
      Error=true;
      break;
    }
    #endif

    //Check block counters on page header
    if(Header->PagePtr->BlockNr<=0){
      Message="On block [#"+ToString(Count)+"] "+BLOCK_INFO(Header)+" page header block count is less or equal to zero (blocknr="+ToString(Header->PagePtr->BlockNr)+")"; 
      Error=true;
      break;
    }
    if(Header->PagePtr->UsedNr<0){
      Message="On block [#"+ToString(Count)+"] "+BLOCK_INFO(Header)+" page header used count is less than zero (usednr="+ToString(Header->PagePtr->UsedNr)+")"; 
      Error=true;
      break;
    }

    //Check block marks
    #ifdef __DEV__
    if(Header->Mark1!=BLOCK_MARK1){
      Message="On block [#"+ToString(Count)+"] "+BLOCK_INFO(Header)+" block begining mark test fails (mark="+HEXFORMAT(Header->Mark1)+")"; 
      Error=true;
      break;
    }
    if(Header->Mark2!=BLOCK_MARK2){
      Message="On block [#"+ToString(Count)+"] "+BLOCK_INFO(Header)+" block ending mark test fails (mark="+HEXFORMAT(Header->Mark2)+")"; 
      Error=true;
      break;
    }
    #endif

    //Check next pointer
    if(!Last && Header->Next->Prev!=Header){ 
      Message="On block [#"+ToString(Count)+"] "+BLOCK_INFO(Header)+" next block does not point back"; 
      Error=true;
      break;
    }

    //Check prev pointer
    if(!First){
      if(Header->Prev==nullptr){ 
        Message="On block [#"+ToString(Count)+"] "+BLOCK_INFO(Header)+" prev block pointer is null"; 
        Error=true;
        break;
      }
      if(Header->Prev->Next!=Header){ 
        Message="On block [#"+ToString(Count)+"] "+BLOCK_INFO(Header)+" prev block does not point forward"; 
        Error=true;
        break;
      }
    }

    //Check unit count
    Units+=Header->Units;
    if(Units>_Units){
      Message="On block [#"+ToString(Count)+"] "+BLOCK_INFO(Header)+" list traverse exceeds memory unit count ("+ToString(_Units)+")";
      Error=true;
      break;
    }

    //Check block count
    Count++;
    if(Count>_BlockCount){
      Message="On block [#"+ToString(Count)+"] "+BLOCK_INFO(Header)+" list traverse exceeds block count ("+ToString(_BlockCount)+")";
      Error=true;
      break;
    }

    //Check memory block bounds
    if(reinterpret_cast<char *>(Header) < reinterpret_cast<char *>(Header->PagePtr)+sizeof(PageHeader)){
      Message="Block [#"+ToString(Count)+"] "+BLOCK_INFO(Header)+" points below inner block";
      Error=true;
      break;
    }
    if(reinterpret_cast<char *>(Header->PagePtr)+sizeof(PageHeader)-reinterpret_cast<char *>(Header)+_UnitSize*Header->Units > Header->PagePtr->TotalMemory){
      Message="Block [#"+ToString(Count)+"] "+BLOCK_INFO(Header)+" points over inner block";
      Error=true;
      break;
    }

    //Point to next block / exit
    if(Header->Next==nullptr){ break; } else{ Header=Header->Next; }

  }while(true);

  //Check unit count
  if(!Error && Units!=_Units){
    Message="At list traverse end memory unit count does not match (units="+ToString(_Units)+", traversed="+ToString(Units)+")";
    Error=true;
  }

  //Check block count
  if(!Error && Count!=_BlockCount){
    Message="At list traverse end block count dos not match (blockcount="+ToString(_BlockCount)+", traversed="+ToString(Count)+")";
    Error=true;
  }

  //Check all pointers in free list are valid block pointers
  if(!Error){
    for(int i=0;i<_FreeListNr;i++){ 
      if(_FreeList[i].Block!=nullptr){ 
        if(SBlock.Search(_FreeList[i].Block)==-1){
          Message="Detected invalid block pointer in free list (freeindex="+ToString(i)+", block="+PTRFORMAT(_FreeList[i].Block)+")";
          Error=true;
          break;
        }
        if(_FreeList[i].Block->Used==true){
          Message="Detected used block on free list (freeindex="+ToString(i)+", block="+BLOCK_PTR_STRING(_FreeList[i].Block)+")";
          Error=true;
          break;
        }
      }
    }
  }

  //Return error
  if(Error){
    DebugMessage(DebugLevel::VrmMemPoolCheck,"Memory check failure ["+MEMORY_POOL_NAME(_PoolOwner).Replace(": ","")+"]: "+Message+" (memory operation: "+MemOperationText(Oper)+")");
    DebugMessage(DebugLevel::VrmMemPoolCheck,"Traversed blocks:");
    for(int i=0;i<TBlock.Length();i++){
      DebugMessage(DebugLevel::VrmMemPoolCheck,"#"+ToString(i).RJust(4)+": "+BLOCK_INFO(TBlock[i]));
    }
    DebugMessage(DebugLevel::VrmMemPoolCheck,"Free list: index="+ToString(_FreeIndex)+", blocks="+HEXFORMAT(_FreeBlocks)+", bits="+ToString(_FreeBits));
    DebugMessage(DebugLevel::VrmMemPoolCheck,"Free blocks:");
    for(int i=0;i<_FreeListNr;i++){ 
      DebugMessage(DebugLevel::VrmMemPoolCheck,"#"+ToString(i).RJust(4)+": "+(_FreeList[i].Block==nullptr?"nullptr":PTRFORMAT(_FreeList[i].Block)));
    }
    _CheckFailure=true;
    ThrowBaseException((int)ExceptionSource::MemoryPool,(int)MemoryPoolException::MemoryCheckError,Message.CharPnt());
  }

  //Return success
  return true;

}