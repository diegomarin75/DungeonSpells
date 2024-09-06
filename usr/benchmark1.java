class Simple{  
  
  public static void main(String args[]){  
    
    long Start = System.nanoTime();
    
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

    float Time = ((float)(System.nanoTime() - Start))/1000000000;
    System.out.format("JV Benchmark: %.5fs\n",Time);  
  
  }  

}