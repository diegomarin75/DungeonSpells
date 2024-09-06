//Time start
var start = process.hrtime();

//Variables
var i;
var j;
var z;
var max1=10000;
var max2=1000;
var cons=2;

//Loop
z=0;
i=0;
while(i<max1){
  j=0;
  while(j<max2){
    z=cons*z+j;
    j++;
  }
  i++;
}

//Elapsed time
var elapsed = process.hrtime(start)
var secs = elapsed[0]+(elapsed[1]/1000000000)
console.log('JS Benchmark: '+secs.toFixed(5)+'s')

