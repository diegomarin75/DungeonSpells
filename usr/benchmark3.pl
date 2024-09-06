use Time::HiRes qw(time);
$time_start = time();

#Variables
my $iter=1000;
my $maxval=1000;
my $xmin=-2.1;
my $xmax=1.0;
my $stepsx=200;
my $ymin=-1.2;
my $ymax=1.2;
my $stepsy=100;

#Print loop
my $lit2=2.0;
my $lit32=32;
my $lit94=94;
my $result=0;
for($j=0;$j<$stepsy;$j++){
  for($i=0;$i<$stepsx;$i++){
    my $cx=$xmin+$i*($xmax-$xmin)/$stepsx;
    my $cy=$ymin+$j*($ymax-$ymin)/$stepsy;
    my $x0=$cx;
    my $y0=$cy;
    my $overflow=0;
    my $k=0;
    for(;$k<$iter;$k++){
      my $x1=($x0*$x0)-($y0*$y0)+$cx;
      my $y1=($lit2*$x0*$y0)+$cy;
      if(($x1*$x1)+($y1*$y1)>$maxval*$maxval){
        $overflow=1;
        last;
      }
      else{
        $x0=$x1;
        $y0=$y1; 
      }
    }
    $result=$result+($overflow>0?$lit32+($k % $lit94):$lit32);
  }
}

$time = time() - $time_start;
printf "PL Benchmark: %.5fs\n",$time;
