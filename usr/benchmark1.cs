using System;

public class Benchmark{
  
  public static void Main(string[] args){
 
    //Start timming
    var Timer=System.Diagnostics.Stopwatch.StartNew();

    //Variables
    int i;
    int j;
    int z;

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
    Timer.Stop();
    Console.WriteLine("CS Benchmark: "+(((float)Timer.ElapsedTicks/(float)TimeSpan.TicksPerSecond).ToString("0.00000")));
    Console.WriteLine(z.ToString());

  }

}