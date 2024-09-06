class Simple{  
  
  //Calculate point
  static int calculatepoint(short iter,float maxval,float cx,float cy){
    
    //Variables
    int i;
    float x0;
    float y0;
    float x1;
    float y1;
    boolean overflow;

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
    return (overflow?i:-i);

  }

  //Print mandelbrot set to console
  static void mandelbrotset(short iter,float maxval,float x1,float x2,int stepsx,float y1,float y2,int stepsy){

    //Variables
    float cx;
    float cy;
    int i;
    int j;
    int k;
    String line;

    //Print loop
    j=0;
    while(j<stepsy){
      line="";
      i=0;
      while(i<stepsx){
        cx=x1+i*(x2-x1)/stepsx;
        cy=y1+j*(y2-y1)/stepsy;
        k=calculatepoint(iter,maxval,cx,cy);
        line+=(char)(k>0?32+k%94:32);
        i++;
      }
      j++;
    }

  }

  //Main
  public static void main(String args[]){  
    
    //Time start
    long Start = System.nanoTime();
  
    //Variables
    short iter=(short)1000;
    float maxval=1000;
    float x1=(float)-2.1;
    float x2=(float)1.0;
    int stepsx=200;
    float y1=(float)-1.2;
    float y2=(float)1.2;
    int stepsy=100;

    //Call mandelbrot set
    mandelbrotset(iter,maxval,x1,x2,stepsx,y1,y2,stepsy);

    //Time end
    float Time = ((float)(System.nanoTime() - Start))/1000000000;
    System.out.format("JV Benchmark: %.5fs\n",Time);  
  
  }  

}