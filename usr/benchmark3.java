class Simple{  
  
  public static void main(String args[]){  
    
    long Start = System.nanoTime();
    
    //Calculation parameters
    short iter=(short)1000;
    float maxval=1000;
    float xmin=(float)-2.1;
    float xmax=(float)1.0;
    int stepsx=200;
    float ymin=(float)-1.2;
    float ymax=(float)1.2;
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
    boolean overflow;
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

    float Time = ((float)(System.nanoTime() - Start))/1000000000;
    System.out.format("JV Benchmark: %.5fs\n",Time);  
  
  }  

}