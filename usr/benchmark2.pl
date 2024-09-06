use Time::HiRes qw(time);

#Calculate point
sub calculatepoint {
  my($iter,$maxval,$cx,$cy)=@_;
  
  #Init loop
  my $x0=$cx;
  my $y0=$cy;
  my $overflow=0;
  
  #Calculation loop
  my $i=0;   
  while($i<$iter){
    my $x1=($x0*$x0)-($y0*$y0)+$cx;
    my $y1=(2*$x0*$y0)+$cy;
    if(($x1*$x1)+($y1*$y1)>$maxval*$maxval){
      $overflow=1;
      last;
    }
    else{
      $x0=$x1;
      $y0=$y1;
    }
    $i+=1;
  }

  #Return value
  return ($overflow==1?$i:-$i);

}

#Print mandelbrot set to console
sub mandelbrotset{
  my($iter,$maxval,$x1,$x2,$stepsx,$y1,$y2,$stepsy)=@_;

  #Print loop
  my $j=0;
  while($j<$stepsy){
    my $line="";
    my $i=0;
    while($i<$stepsx){
      my $cx=$x1+$i*($x2-$x1)/$stepsx;
      my $cy=$y1+$j*($y2-$y1)/$stepsy;
      my $k=calculatepoint($iter,$maxval,$cx,$cy);
      $line=$line . chr($k>0?32+($k%94):32);
      $i+=1;
    }
    $j+=1;
  }

}

#Start time
my $time_start = time();

#Variables
my $iter=1000;
my $maxval=1000;
my $x1=-2.1;
my $x2=1.0;
my $stepsx=200;
my $y1=-1.2;
my $y2=1.2;
my $stepsy=100;

#Call mandelbrot set
mandelbrotset($iter,$maxval,$x1,$x2,$stepsx,$y1,$y2,$stepsy);

my $time = time() - $time_start;
printf "PL Benchmark: %.5fs\n",$time;
