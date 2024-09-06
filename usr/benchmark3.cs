using System;

public class Benchmark{
  
  public static long gresult=0;

  public static void Main(string[] args){
 
    //Start timming
    var Timer=System.Diagnostics.Stopwatch.StartNew();

    //Calculation parameters
    short iter=(short)1000;
    double maxval=1000;
    double xmin=-2.1;
    double xmax=1.0;
    int stepsx=200;
    double ymin=-1.2;
    double ymax=1.2;
    int stepsy=100;

    //Variables
    double cx;
    double cy;
    int i;
    int j;
    int k;
    double x0;
    double y0;
    double x1;
    double y1;
    bool overflow;
    long result;
    double lit2=2;
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
    Timer.Stop();
    Console.WriteLine("CS Benchmark: "+(((double)Timer.ElapsedTicks/(double)TimeSpan.TicksPerSecond).ToString("0.00000")));
    gresult=result;

  }

}