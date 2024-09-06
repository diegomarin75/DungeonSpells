//String.hpp: Header file for string class
#ifndef _STRING_HPP
#define _STRING_HPP
 
//Exception numbers
enum class StringException{
  InvalidIndex=1,
  InvalidMidCall=2,
  InvalidRightCall=3,
  InvalidLeftCall=4,
  InvalidCutRightCall=5,
  InvalidCutLeftCall=6,
  StringOverflow=7
};

//String class
class String {
  
  //Private members
  private:

    //Constants
    const int DEFAULT_STREAM_INPUT = 256;
    const int DATA_CHUNK = 128;

    //Private members
    char *_Chr;
    long _Size;
    long _Length;

    //Private methods
    void _Allocate(long Size);
    void _Free();

  //Public members
  public:

    //Constructors
    String();
    String(const char *Pnt);
    String(const char *Pnt,long Length);
    String(const String& Str);
    String(char Chr);
    String(long Times,char Chr);
    String(const Buffer& Buff);
    
    //Destructor
    inline ~String(){ _Free(); }
    
    //Inline functions
    inline long Length() const { return _Length; }
    inline char *CharPnt() const { return (char *)&_Chr[0]; }
 
    //String manipulations
    Buffer ToBuffer() const;
    void Copy(char *Dest,long MaxLen) const;
    String Mid(long Position,long Length) const;
    String Right(long Length) const;
    String Left(long Length) const;
    String CutRight(long Length) const;
    String CutLeft(long Length) const;
    int SearchCount(const String& Str, long StartPos=0) const;
    bool SearchAt(char c, long StartPos=0) const;
    long Search(char c, long StartPos=0, int Occurence=1) const;
    long Search(char c, const String& TextQualifier, const String& TextEscaper, const String& RawStrBeg, const String& RawStrEnd, const String& LevelUp, const String& LevelDown, long StartPos=0, int Occurence=1) const;
    bool SearchAt(const String& Str, long StartPos=0) const;
    bool SearchAnyAt(const String& Str, long StartPos=0) const;
    long Search(const String& Str, long StartPos=0, int Occurence=1) const;
    long Search(const String& FindStr, const String& TextQualifier, const String& TextEscaper, const String& RawStrBeg, const String& RawStrEnd, const String& LevelUp, const String& LevelDown, long StartPos=0, int Occurence=1) const;
    bool Match(const String& Regex) const;
    bool Like(const String& Pattern) const;
    void ReplaceWithin(const String& Old, const String& New, int Occurence=0);
    void ReplaceWithin(const String& Old, const String& New, const String& TextQualifier, const String& TextEscaper, const String& RawStrBeg, const String& RawStrEnd, const String& LevelUp, const String& LevelDown, int Occurence=0);
    String Replace(const String& Old, const String& New, int Occurence=0) const;
    String Replace(const String& Old, const String& New, const String& TextQualifier, const String& TextEscaper, const String& RawStrBeg, const String& RawStrEnd, const String& LevelUp, const String& LevelDown, int Occurence=0) const;
    String TrimRight(char TrimChar=32) const;
    String TrimLeft(char TrimChar=32) const;
    String Trim(char TrimChar=32) const;
    String Upper() const;
    String Lower() const;
    String Purge(const String& TextQualif, const String& TextEscape) const;
    String LJust(int Length, char Chr=32) const;
    String RJust(int Length, char Chr=32) const;
    String Replicate(const String& Str, int Times) const;
    String ConcatWithSepar(const String& Str, const String& Separ) const;
    bool Contains(char c) const;
    bool Contains(const Array<char>& c) const;
    bool Contains(const String& Substring) const;
    bool ContainsOnly(const String& Pattern) const;
    long FirstDifferent(const String& CharList) const;
    Array<String> Split(const String& Separator, int MaxParts=0) const;
    Array<String> Split(const String& Separator, const String& TextQualifier, const String& TextEscaper, const String& RawStrBeg, const String& RawStrEnd, const String& LevelUp, const String& LevelDown, int MaxParts=0) const;
    bool IsAny(const String& CharList) const;
    bool StartsWith(const String& Str) const;
    bool StartsWithAny(const String& CharList) const;
    bool StartsWithAny(const Array<String>& List) const;
    bool EndsWith(const String& Str) const;
    bool EndsWithAny(const String& CharList) const;
    bool EndsWithAny(const Array<String>& List) const;
    String GetWhile(const String& CharList) const;
    String GetWhile(const Array<String>& List) const;
    String GetUntil(const String& CharList) const;
    String GetUntil(const Array<String>& List) const;
    int ToInt(int Base=10) const;
    int ToInt(bool& Error,int Base=10) const;
    long ToLong(int Base=10) const;
    long ToLong(bool& Error,int Base=10) const;
    long long ToLongLong(int Base=10) const;
    long long ToLongLong(bool& Error,int Base=10) const;
    float ToFloat() const;
    float ToFloat(bool& Error) const;
    double ToDouble() const;
    double ToDouble(bool& Error) const;
    friend String ToString(int Number,const String& FormatStr);
    friend String ToString(long Number,const String& FormatStr);
    friend String ToString(long long Number,const String& FormatStr);
    friend String ToString(float Number,const String& FormatStr);
    friend String ToString(double Number,const String& FormatStr);
    template <typename ... Args> friend String Format(const String& FormatStr, Args ... args);

    //Operators
    String& operator=(const char *Pnt);
    String& operator=(const String& Str);
    char& operator[](int Index) const;
    friend String operator+(const String& Str1,const String& Str2);
    friend String operator+(const String& Str,const char *Pnt);
    friend String operator+(const char *Pnt,const String& Str);
    friend String operator+(const String& Str,char c);
    friend String operator+(char c,const String& Str);
    String& operator+=(const String& Str);
    String& operator+=(const char *Pnt);
    String& operator+=(const char c);
    friend bool operator==(const String& Str1,const String& Str2);
    friend bool operator==(const String& Str ,const char *Pnt);
    friend bool operator==(const char *Pnt,const String& Str);
    friend bool operator!=(const String& Str1,const String& Str2);
    friend bool operator!=(const String& Str ,const char *Pnt);
    friend bool operator!=(const char *Pnt,const String& Str);
    friend bool operator< (const String& Str1,const String& Str2);
    friend bool operator< (const String& Str ,const char *Pnt);
    friend bool operator< (const char *Pnt,const String& Str);
    friend bool operator> (const String& Str1,const String& Str2);
    friend bool operator> (const String& Str ,const char *Pnt);
    friend bool operator> (const char *Pnt,const String& Str);
    friend bool operator<=(const String& Str1,const String& Str2);
    friend bool operator<=(const String& Str ,const char *Pnt);
    friend bool operator<=(const char *Pnt,const String& Str);
    friend bool operator>=(const String& Str1,const String& Str2);
    friend bool operator>=(const String& Str ,const char *Pnt);
    friend bool operator>=(const char *Pnt,const String& Str);
    friend std::ostream& operator<<(std::ostream& stream, const String& Str);
    friend std::istream& operator>>(std::istream& stream, String& Str);
};

//These functions need to be declared here as well
String ToString(int Number,const String& FormatStr="");
String ToString(long Number,const String& FormatStr="");
String ToString(long long Number,const String& FormatStr="");
String ToString(float Number,const String& FormatStr="");
String ToString(double Number,const String& FormatStr="");

//String format (defined here because uses templates)
template <typename ... Args> 
String Format(const String& FormatStr, Args ... args) {
  long Size=snprintf(nullptr,0,FormatStr._Chr, args ... );
  if(Size<0) return "";
  String Str=String(Size,' ');
  snprintf(Str._Chr,Size+1,FormatStr._Chr,args ...);
  return Str;
}

#endif