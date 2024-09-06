using System;

public class Benchmark{
  
  public static void Main(string[] args){
 
    //Start timming
    var Timer=System.Diagnostics.Stopwatch.StartNew();

    //Variables
    short iter=(short)1000;
    double maxval=1000;
    double x1=-2.1;
    double x2=1.0;
    int stepsx=200;
    double y1=-1.2;
    double y2=1.2;
    int stepsy=100;

    //Call mandelbrot set
    mandelbrotset(iter,maxval,x1,x2,stepsx,y1,y2,stepsy);

    //End timming
    Timer.Stop();
    Console.WriteLine("CS Benchmark: "+(((double)Timer.ElapsedTicks/(double)TimeSpan.TicksPerSecond).ToString("0.00000")));

  }

  //Calculate point
  public static int calculatepoint(short iter,double maxval,double cx,double cy,ref bool overflow){
    
    //Variables
    int i;
    double x0;
    double y0;
    double x1;
    double y1;

    //Init loop
    x0=cx;
    y0=cy;
    overflow=false;
    
    //Calculation loop
    i=0;
    while(i<iter){
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
      i++;
    }

    //Return value
    return i;

  }

  //Print mandelbrot set to console
  public static void mandelbrotset(short iter,double maxval,double x1,double x2,int stepsx,double y1,double y2,int stepsy){

    //Variables
    double cx;
    double cy;
    int i;
    int j;
    int k;
    bool overflow;
    string line;

    //Print loop
    j=0;
    while(j<stepsy){
      line="";
      i=0;
      while(i<stepsx){
        cx=x1+i*(x2-x1)/stepsx;
        cy=y1+j*(y2-y1)/stepsy;
        overflow=false;
        k=calculatepoint(iter,maxval,cx,cy,ref overflow);
        line+=(char)(overflow?32+k%94:32);
        i++;
      }
      j++;
    }

  }

}