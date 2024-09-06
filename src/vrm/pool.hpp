//pool.hpp: Memory pool

//Wrap include
#ifndef _POOL_HPP
#define _POOL_HPP

//Memory marks
#ifdef __DEV__
  #define PAGE_MARK1       0x6E7D8C9B
  #define PAGE_MARK2       0x33BCE8F9
  #define BLOCK_MARK1      0x7F8E9DAC
  #define BLOCK_MARK2      0x44CAD9E8
  #define SetPageMarks(p)  (p)->Mark1=PAGE_MARK1;  (p)->Mark2=PAGE_MARK2;
  #define SetBlockMarks(p) (p)->Mark1=BLOCK_MARK1; (p)->Mark2=BLOCK_MARK2;
#else
  #define SetPageMarks(p)
  #define SetBlockMarks(p)
#endif

//Memory pool name in dbug messages
#define MEMORY_POOL_NAME(Owner) ((Owner)==-1?String("Main memory pool: "):"Aux memory pool(processid="+ToString(Owner)+"): ")

//Page/Block pointer formatting
#define PAGE_PTR_STRING(p) (PTRFORMAT(p)+"{blocks="+ToString((p)->BlockNr)+", used="+ToString((p)->UsedNr)+", units="+ToString((p)->Units)+"}")
#define BLOCK_PTR_STRING(p) ((p)==nullptr?"nullptr":PTRFORMAT(p)+"("+ToString((p)->Units)+")")
#define BLOCK_INFO(p) PTRFORMAT(p)+"(used="+((p)->Used==true?"1":"0")+", units="+ToString((p)->Units)+", prev="+PTRFORMAT((p)->Prev)+", next="+PTRFORMAT((p)->Next)+", pageptr="+PTRFORMAT((p)->PagePtr)+")"

//Memory pool exception numbers
enum class MemoryPoolException{
  FreeAlreadyReleased=1,
  MemoryCheckError=2
};

//Memory pool error codes
enum class MemPoolError:int{
  SmallUnitSize=1,   //Too small memory unit size
  AllocationError=2, //Memory allocation error
  PageLockFailure=3, //Page lock failure
  RequestError=4     //Memory request error
};

//Memory operations for memory check
enum class MemOperation:int{
  Init=1,
  Destroy=2,
  ReallocFits=3,
  ReallocShortenFreeSplit=4,
  ReallocShortenFreeJoin=5,
  ReallocWidenJoin=6,
  ReallocWidenFreeJoin=7,
  ReallocWidenFreeSplit=8,
  AllocFits=9,
  AllocFreeJoin=10,
  AllocFreeSplit=11,
  AllocFree=12,
  FreeRelease=13,
  FreeReleaseJoinNext=14,
  FreeReleaseJoinPrev=15,
  Extend=16,
  OuterCheck=17,
  PageRelease=18
};

//Memory page header
struct PageHeader{
  #ifdef __DEV__
  CpuInt Mark1;         //Page header mark 1
  #endif
  CpuWrd BlockNr;       //Total allocated blocks (used and free)
  CpuWrd UsedNr;        //Total allocated used blocks
  CpuWrd Units;         //Size in assignment units (page header not counted)
  CpuWrd TotalMemory;   //Allocated memory (page header not counted)
  #ifdef __DEV__
  CpuInt Mark2;         //Page header mark 2
  #endif
};

//Memory block header (inside chained lists)
struct BlockHeader{
  #ifdef __DEV__
  CpuInt Mark1;         //Block mark 1
  #endif
  bool Used;            //Used flag
  int BlockOwner;       //Tag to identify owner of memory block
  PageHeader *PagePtr;  //Memory id from inner allocator (contains PageHeader at beginnning)
  CpuWrd Units;         //Size in assignment units (including header)
  CpuWrd FreeIndex;     //Free block index
  BlockHeader *Next;    //Physical pointer to next block
  BlockHeader *Prev;    //Physical pointer to previous block
  #ifdef __DEV__
  CpuInt Mark2;         //Block mark 2
  #endif
};

//Free memory block table
struct FreeBlock{ 
  BlockHeader *Block; //Block header pointer
};

//Function pointers for inner memory allocator
typedef char *(*FunPtrAlloc)(CpuWrd Size,int Owner);
typedef void (*FunPtrFree)(char *Ptr);

//Memory pool class
class MemoryPool{
  
  //Private menbers
  private:

    //Internal data
    BlockHeader *_List;         //Pointer to block list
    CpuWrd _Units;              //Number of memory units in pool
    CpuWrd _ChunkUnits;         //Number of units to take when memory pool has to be increased / decreased
    CpuWrd _UnitSize;           //Memory assignment unit
    CpuWrd _BlockCount;         //Counter of memory blocks in pool

    //Free block list
    FreeBlock *_FreeList;       //Free block list array
    CpuWrd _FreeListNr;         //Free block list size
    CpuWrd _FreeIndex;          //Free block list insertion position
    CpuLon _FreeBlocks;         //Bitmap indicating extistence of last N free blocks up to distance N from insertion pointer
    int _FreeBits;              //Value of N
    int _PoolOwner;             //Memory pool owner
    bool _Lock;                 //Lock memory pages
    MemPoolError _Error;        //Last error code

    //Internal allocator
    FunPtrAlloc _InnerAlloc;    //Inner allocator
    FunPtrFree _InnerFree;      //Inner releaser

    //Memory check failure flag
    bool _CheckFailure=false;

    //Lock memory pages to prevent page faults (and increase performance)
    inline bool _LockMemory(char *Ram,CpuWrd Size){
      DebugMessage(DebugLevel::VrmMemPool,MEMORY_POOL_NAME(_PoolOwner)+"Page lock "+PTRFORMAT(Ram)+" (size="+ToString(Size)+")");
      #ifdef __WIN__
      if(!VirtualLock(Ram,Size)){ return false; }
      #else
      if(mlockall(MCL_CURRENT | MCL_FUTURE)==-1){ return false; }
      #endif
      return true;
    }

    //Set free bits
    inline void _FreeBitSet(CpuWrd Index){
      int Distance;
      if(Index<=_FreeIndex){ Distance=_FreeIndex-Index; } else{ Distance=_FreeListNr-Index+_FreeIndex+1; }
      if(Distance<_FreeBits){ _FreeBlocks|=(0x01<<(Distance-1)); }
      DebugMessage(DebugLevel::VrmMemPool,MEMORY_POOL_NAME(_PoolOwner)+"Free bit set on index "+
      ToString(Index)+" "+String((Distance<_FreeBits?"[Success]":"[OutOfRange]"))+" (distance="+
      ToString(Distance)+" freelistzise="+ToString(_FreeListNr)+" freebits="+ToString(_FreeBits)+")");
    }

    //Clear free bits
    inline void _FreeBitClr(CpuWrd Index){
      int Distance;
      if(Index<=_FreeIndex){ Distance=_FreeIndex-Index; } else{ Distance=_FreeListNr-Index+_FreeIndex+1; }
      if(Distance<_FreeBits){ _FreeBlocks&=(~(0x01<<(Distance-1))); }
      DebugMessage(DebugLevel::VrmMemPool,MEMORY_POOL_NAME(_PoolOwner)+"Free bit clear on index "+
      ToString(Index)+" "+String((Distance<_FreeBits?"[Success]":"[OutOfRange]"))+" (distance="+
      ToString(Distance)+" freelistzise="+ToString(_FreeListNr)+" freebits="+ToString(_FreeBits)+")");
    }

    //Add block into free list
    inline void _FreeListAdd(BlockHeader *Header){
      DebugMessage(DebugLevel::VrmMemPool,MEMORY_POOL_NAME(_PoolOwner)+"Add block to free list "+BLOCK_PTR_STRING(Header));
      if(Header->FreeIndex==-1 || (Header->FreeIndex!=-1 && _FreeList[Header->FreeIndex].Block!=Header)){
        _FreeIndex++;
        if(_FreeIndex>_FreeListNr-1){ _FreeIndex=0; }
        _FreeList[_FreeIndex].Block=Header;
        Header->FreeIndex=_FreeIndex;
      }
      _FreeBitSet(Header->FreeIndex);
    }

    //Remove block from free list
    inline void _FreeListRemove(BlockHeader *Header){
      DebugMessage(DebugLevel::VrmMemPool,MEMORY_POOL_NAME(_PoolOwner)+"Remove block from free list "+BLOCK_PTR_STRING(Header));
      if(Header->FreeIndex!=-1 && _FreeList[Header->FreeIndex].Block==Header){
        _FreeList[Header->FreeIndex].Block=nullptr;
        _FreeBitClr(Header->FreeIndex);
        Header->FreeIndex=-1;
        if(_FreeIndex<0){ _FreeIndex=_FreeListNr-1; }
      }
    }

    //Block split print
    inline String _BlockSplitPrint(BlockHeader *Header){
      String Line;
      Line=BLOCK_PTR_STRING(Header);
      Line+=(Header!=nullptr?" -> ["+BLOCK_PTR_STRING(Header->Next)+"]":"");
      Line+=(Header!=nullptr && Header->Next!=nullptr?" -> "+BLOCK_PTR_STRING(Header->Next->Next):"");
      return Line;
    }

    //Block join print
    inline String _BlockJoinPrint(BlockHeader *Header){
      String Line;
      Line=BLOCK_PTR_STRING(Header);
      Line+=(Header!=nullptr?" -> ("+BLOCK_PTR_STRING(Header->Next)+")":"");
      Line+=(Header!=nullptr && Header->Next!=nullptr?" -> "+BLOCK_PTR_STRING(Header->Next->Next):"");
      return Line;
    }

    //Internal methods
    bool _Allocate(BlockHeader **Block,CpuWrd Size,int BlockOwner);
    void _Free(BlockHeader *Block);
    bool _Extend(CpuWrd Chunks);
    
  //Public members
  public:
    
    //Memory allocation
    inline char *Allocate(CpuWrd Size,int Owner,bool AutoExtend=true){
      CpuWrd Chunks;
      BlockHeader *Header=nullptr;
      DebugMessage(DebugLevel::VrmMemPool,MEMORY_POOL_NAME(_PoolOwner)+"Allocation request (size="+ToString(Size)+
      " blockowner="+ToString(Owner)+" autoextend="+ToString(AutoExtend)+")");
      #ifdef __DEV__
      if(_CheckFailure){
        DebugMessage(DebugLevel::VrmMemPool,MEMORY_POOL_NAME(_PoolOwner)+"Allocation request skipped since memory pool is in invalid state");
        return nullptr;
      }
      #endif
      if(!_Allocate(&Header,Size,Owner)){ 
        if(!AutoExtend){ return nullptr; }
        Chunks=(Size+sizeof(BlockHeader)+_ChunkUnits*_UnitSize)/(_ChunkUnits*_UnitSize);
        if(!_Extend(Chunks)){ return nullptr; }
        if(!_Allocate(&Header,Size,Owner)){ return nullptr; }
      }
      return reinterpret_cast<char *>(Header)+sizeof(BlockHeader);
    }

    //Memory re-allocation
    inline bool ReAllocate(char **Ptr,CpuWrd Size,bool AutoExtend=true){
      CpuWrd Chunks;
      BlockHeader *Header=reinterpret_cast<BlockHeader *>((*Ptr)-sizeof(BlockHeader));
      DebugMessage(DebugLevel::VrmMemPool,MEMORY_POOL_NAME(_PoolOwner)+"Re-allocation request (ptr="+PTRFORMAT(*Ptr)+
      " header="+BLOCK_PTR_STRING(Header)+" size="+ToString(Size)+" autoextend="+ToString(AutoExtend)+")");
      #ifdef __DEV__
      if(_CheckFailure){
        DebugMessage(DebugLevel::VrmMemPool,MEMORY_POOL_NAME(_PoolOwner)+"Re-allocation request skipped since memory pool is in invalid state");
        return false;
      }
      #endif
      if(!_Allocate(&Header,Size,Header->BlockOwner)){ 
        if(!AutoExtend){ return false; }
        Chunks=(Size+sizeof(BlockHeader)+_ChunkUnits*_UnitSize)/(_ChunkUnits*_UnitSize);
        if(!_Extend(Chunks)){ return false; }
        if(!_Allocate(&Header,Size,Header->BlockOwner)){ return false; }
      }
      *Ptr=reinterpret_cast<char *>(Header)+sizeof(BlockHeader);
      return true;
    }

    //Memory release
    inline void Free(char *Ptr){
      if(Ptr==nullptr){ return; }
      BlockHeader *Header=reinterpret_cast<BlockHeader *>((Ptr)-sizeof(BlockHeader));
      DebugMessage(DebugLevel::VrmMemPool,MEMORY_POOL_NAME(_PoolOwner)+"Release (ptr="+PTRFORMAT(Ptr)+" header="+BLOCK_PTR_STRING(Header)+")");
      #ifdef __DEV__
      if(_CheckFailure){
        DebugMessage(DebugLevel::VrmMemPool,MEMORY_POOL_NAME(_PoolOwner)+"Release request skipped since memory pool is in invalid state");
        return;
      }
      #endif
      _Free(Header);
    }

    //Public methods
    CpuWrd MinUnitSize() const;
    bool Create(CpuWrd Units,CpuWrd ChunkUnits,CpuWrd UnitSize,long FreeListNr,int FreeBits,int PoolOwner,bool Lock,FunPtrAlloc InnerAlloc,FunPtrFree InnerFree);
    void Destroy();
    MemPoolError LastError() const;
    String ErrorText(MemPoolError Error) const;
    String MemOperationText(MemOperation MemOper) const;
    bool MemoryCheck(MemOperation Oper);

    //Constructors/Destructors
    MemoryPool(){}
    ~MemoryPool(){}

};

#endif