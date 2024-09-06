#include <string>
#include <iostream>
#include <chrono>

#define TIMMING_START   double Seconds; \
                        char TimeStr[20+1]; \
                        std::chrono::time_point<std::chrono::steady_clock> ClockStart; \
                        std::chrono::time_point<std::chrono::steady_clock> ClockEnd; \
                        ClockStart=std::chrono::steady_clock::now(); \

#define TIMMING_END     ClockEnd=std::chrono::steady_clock::now(); \
                        Seconds=(((std::chrono::duration<double,std::micro>)(ClockEnd-ClockStart)).count())/1000000.0; \
                        snprintf(TimeStr,20,"%0.5f",Seconds); \
                        std::cout << "CP Benchmark: " + std::string(TimeStr) + "s" << std::endl; \

//Function declarations
void mandelbrotset(short iter, float maxval,float x1,float x2,int stepsx,float y1,float y2,int stepsy);
int calculatepoint(short iter,float maxval,float cx,float cy,bool& overflow);

//Main function
int main(void){

  //Start timming
  TIMMING_START;

  //Variables
  short iter=(short)1000;
  float maxval=1000;
  float x1=-2.1;
  float x2=1.0;
  int stepsx=180;
  float y1=-1.2;
  float y2=1.2;
  int stepsy=90;

  //Call mandelbrot set
  mandelbrotset(iter,maxval,x1,x2,stepsx,y1,y2,stepsy);

  //End timming
  TIMMING_END;

}

//Calculate point
int calculatepoint(short iter,float maxval,float cx,float cy,bool& overflow){
  
  //Variables
  int i;
  float x0;
  float y0;
  float x1;
  float y1;

  //Init loop
  x0=cx;
  y0=cy;
  overflow=false;
  
  //Calculation loop
  for(i=0;i<iter;i++){
    x1=(x0*x0)-(y0*y0)+cx;
    y1=(2*x0*y0)+cy;
    if((x1*x1)+(y1*y1)>maxval*maxval){
      overflow=true;
      break;
    }
    else{
      x0=x1;
      y0=y1;
    }
  }

  //Return value
  return i;

}

//Print mandelbrot set to console
void mandelbrotset(short iter,float maxval,float x1,float x2,int stepsx,float y1,float y2,int stepsy){

  //Variables
  float cx;
  float cy;
  int i;
  int j;
  int k;
  bool overflow;
  std::string line;

  //Print loop
  for(j=0;j<stepsy;j++){
    line="";
    for(i=0;i<stepsx;i++){
      cx=x1+i*(x2-x1)/stepsx;
      cy=y1+j*(y2-y1)/stepsy;
      overflow=false;
      k=calculatepoint(iter,maxval,cx,cy,overflow);
      line+=(char)(overflow?32+k%94:32);
    }
    std::cout << line << std::endl;
  }

}