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

//Global variable
int gresult=0;                        

//Main function
int main(void){

  //Start timming
  TIMMING_START;

  //Calculation parameters
  short iter=(short)1000;
  float maxval=1000;
  float xmin=-2.1;
  float xmax=1.0;
  int stepsx=200;
  float ymin=-1.2;
  float ymax=1.2;
  int stepsy=100;

  //Variables
  float cx;
  float cy;
  int i;
  int j;
  int k;
  float x0;
  float y0;
  float x1;
  float y1;
  bool overflow;
  long result;
  float lit2=2;
  int lit32=32;
  int lit94=94;

  //Print loop
  result=0;
  for(j=0;j<stepsy;j++){
    for(i=0;i<stepsx;i++){
      cx=xmin+i*(xmax-xmin)/stepsx;
      cy=ymin+j*(ymax-ymin)/stepsy;
      x0=cx;
      y0=cy;
      overflow=false;
      for(k=0;k<iter;k++){
        x1=(x0*x0)-(y0*y0)+cx;
        y1=(lit2*x0*y0)+cy;
        if((x1*x1)+(y1*y1)>maxval*maxval){
          overflow=true;
          break;
        }
        else{
          x0=x1;
          y0=y1;
        }
      }
      result+=(char)(overflow?lit32+k%lit94:lit32);
    }
  }

  //End timming
  TIMMING_END;

  //Ensure loop is not skipped
  gresult=result;

}