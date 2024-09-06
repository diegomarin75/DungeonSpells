//auxmem.hpp: Auxiliar CPU memory manager
//This memory controller uses a chained list instead of a memory map, however there is still a table for the memory handlers
//The reason to have a table for the memory handers is that compiler assigns block numbers sequentially when allocating constant strings and arrays

//Wrap include
#ifndef _AU2MEM_HPP
#define _AU2MEM_HPP

//Memory definitions
#define AUXMAN_FREEBITS 64
#define AUXMAN_FREELIST 256

//Aux memory manager exception numbers
enum class AuxMemoryException{
  FreeInvalidBlock=1,
  FreeAlreadyReleased=2
};

//Process memory block handler definition
struct AuxBlock{
  int ScopeId;    //ScopeId of allocated block
  CpuLon ScopeNr; //ScopeNr of allocated block
  bool Used;      //Used flag
  CpuWrd Size;    //Memory request size
  CpuWrd Length;  //String length (used when block allocates a string)
  int ArrIndex;   //Array definition index (used when block allocates an array)
  char *Ptr;      //Pointer to data
};

//Auxiliar memory manager class
class AuxMemoryManager{
  
  //Private menbers
  private:

    //Internal data
    bool _Init;             //Memory manager initialized
    AuxBlock *_Block;       //Memory handler table
    CpuMbl _BlockMax;       //Memory handler table size
    int _ProcessId;         //Process Id owner
    CpuMbl _LastBlockAsg;   //Last assigned handler (used to optimize free handler search)
    MemoryPool _MemoryPool; //Internal memory pool     

    //Handler methods
    bool _ExtendHandlers();
    bool _GetHandler(int ScopeId,CpuLon ScopeNr,CpuMbl *Block);
    void _Free(CpuMbl Block);

  //Public members
  public:
    
    //Handler methods
    bool Init(int ProcessId,CpuMbl BlockMax,CpuWrd Units,CpuWrd ChunkUnits,CpuWrd UnitSize,String& Error);
    void Terminate();
    bool EmptyAlloc(int ScopeId,CpuLon ScopeNr,CpuMbl *Block);
    bool Alloc(int ScopeId,CpuLon ScopeNr,CpuWrd Size,int ArrIndex,CpuMbl *Block);
    bool ForcedAlloc(int ScopeId,CpuLon ScopeNr,CpuWrd Size,int ArrIndex,CpuMbl Block);
    bool Realloc(int ScopeId,CpuLon ScopeNr,CpuMbl Block,CpuWrd Size);
    void Free(CpuMbl Block);
    void Clear(CpuMbl Block);
    void Copy(CpuMbl Block, char *Src,CpuWrd Length);
    inline char *CharPtr(CpuMbl Block){ return _Block[Block].Ptr; }
    inline void SetPtr(CpuMbl Block,char *Pnt){ _Block[Block].Ptr=Pnt; }
    inline int ScopeId(CpuMbl Block){ return _Block[Block].ScopeId; }        
    inline CpuLon ScopeNr(CpuMbl Block){ return _Block[Block].ScopeNr; }        
    inline bool IsValid(CpuMbl Block){ return ((Block)>=0&&(Block)<=_BlockMax-1?_Block[Block].Used:false); }
    inline String IsValidText(CpuMbl Block){ return (Block<0?"invalid (block negative)":(Block>_BlockMax-1?"invalid (block outside table, blocks="+ToString(_BlockMax)+")":(!_Block[Block].Used?"invalid (block not used)":"valid"))); }
    inline int GetArrIndex(CpuMbl Block){ return _Block[Block].ArrIndex; }         
    inline CpuWrd GetLen(CpuMbl Block){ return _Block[Block].Length; }         
    inline CpuWrd GetSize(CpuMbl Block){ return _Block[Block].Size; }         
    inline void SetLen(CpuMbl Block,CpuWrd Length){ _Block[Block].Length=Length; }         
    inline void SetSize(CpuMbl Block,CpuWrd Size){ _Block[Block].Size=Size; }         
    inline bool IsZombie(CpuMbl Block,int ScopeId,CpuLon ScopeNr){ return (_Block[Block].ScopeId>ScopeId || (_Block[Block].ScopeId==ScopeId && _Block[Block].ScopeNr!=ScopeNr))?true:false; }
    String GetStatus(int ScopeId,CpuLon ScopeNr);    

    //Constructors/Destructors
    AuxMemoryManager();
    ~AuxMemoryManager();

};

#endif