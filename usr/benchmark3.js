//Time start
var start = process.hrtime();


  //Calculation parameters
  var iter=1000;
  var maxval=1000;
  var xmin=-2.1;
  var xmax=1.0;
  var stepsx=200;
  var ymin=-1.2;
  var ymax=1.2;
  var stepsy=100;

  //Variables
  var cx;
  var cy;
  var i;
  var j;
  var k;
  var x0;
  var y0;
  var x1;
  var y1;
  var overflow;
  var result;
  var lit2=2;
  var lit32=32;
  var lit94=94;

  //Print loop
  result=0;
  for(j=0;j<stepsy;j++){
    for(i=0;i<stepsx;i++){
      cx=xmin+i*(xmax-xmin)/stepsx;
      cy=ymin+j*(ymax-ymin)/stepsy;
      x0=cx;
      y0=cy;
      overflow=0;
      for(k=0;k<iter;k++){
        x1=(x0*x0)-(y0*y0)+cx;
        y1=(lit2*x0*y0)+cy;
        if((x1*x1)+(y1*y1)>maxval*maxval){
          overflow=1;
          break;
        }
        else{
          x0=x1;
          y0=y1;
        }
      }
      result+=(overflow?lit32+k%lit94:lit32);
    }
  }

//Elapsed time
var elapsed = process.hrtime(start)
var secs = elapsed[0]+(elapsed[1]/1000000000)
console.log('JS Benchmark: '+secs.toFixed(5)+'s')
