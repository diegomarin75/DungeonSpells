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

//Main function
int main(void){

  //Start timming
  TIMMING_START;

  //Variables
  int i;
  int j;
  volatile int z;

  //Calculation
  z=0;
  i=0;
  while(i<10000){
    j=0;
    while(j<1000){
      z=2*z+j;
      j++;
    }
    i++;
  }

  //End timming
  TIMMING_END;

}