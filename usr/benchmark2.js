//Time start
var start = process.hrtime();

//Calculate point
function calculatepoint(iter,maxval,cx,cy){
  
  //Variables
  var i;
  var x0;
  var y0;
  var x1;
  var y1;
  var overflow;

  //Init loop
  x0=cx;
  y0=cy;
  overflow=0;
  
  //Calculation loop
  i=0;
  while(i<iter){
    x1=(x0*x0)-(y0*y0)+cx;
    y1=(2*x0*y0)+cy;
    if((x1*x1)+(y1*y1)>maxval*maxval){
      overflow=1;
      break;
    }
    else{
      x0=x1;
      y0=y1;
    }
    i++;
  }

  //Return value
  if(overflow==1){
    return i;
  }
  else{
    return -i;
  }

}

//Print mandelbrot set to console
function mandelbrotset(iter,maxval,x1,x2,stepsx,y1,y2,stepsy){

  //Variables
  var cx;
  var cy;
  var i;
  var j;
  var k;
  var line="";

  //Print loop
  j=0;
  while(j<stepsy){
    line="";
    i=0;
    while(i<stepsx){
      cx=x1+i*(x2-x1)/stepsx;
      cy=y1+j*(y2-y1)/stepsy;
      k=calculatepoint(iter,maxval,cx,cy);
      line+=String.fromCharCode(k>0?32+k%94:32);
      i++;
    }
    j++;
  }

}

//Variables
var iter=1000;
var maxval=1000;
var x1=-2.1;
var x2=1.0;
var stepsx=200;
var y1=-1.2;
var y2=1.2;
var stepsy=100;

//Call mandelbrot set
mandelbrotset(iter,maxval,x1,x2,stepsx,y1,y2,stepsy);

//Elapsed time
var elapsed = process.hrtime(start)
var secs = elapsed[0]+(elapsed[1]/1000000000)
console.log('JS Benchmark: '+secs.toFixed(5)+'s')
