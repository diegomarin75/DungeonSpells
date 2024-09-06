#!/usr/bin/php
<?php
$time_start = microtime(true);

#Variables
$iter=1000;
$maxval=1000;
$xmin=-2.1;
$xmax=1.0;
$stepsx=200;
$ymin=-1.2;
$ymax=1.2;
$stepsy=100;

#Print loop
$lit2=2.0;
$lit32=32;
$lit94=94;
$result=0;
for($j=0;$j<$stepsy;$j++){
  for($i=0;$i<$stepsx;$i++){
    $cx=$xmin+$i*($xmax-$xmin)/$stepsx;
    $cy=$ymin+$j*($ymax-$ymin)/$stepsy;
    $x0=$cx;
    $y0=$cy;
    $overflow=0;
    for($k=0;$k<$iter;$k++){
      $x1=($x0*$x0)-($y0*$y0)+$cx;
      $y1=($lit2*$x0*$y0)+$cy;
      if(($x1*$x1)+($y1*$y1)>$maxval*$maxval){
        $overflow=1;
        break;
      }
      else{
        $x0=$x1;
        $y0=$y1; 
      }
    }
    $result=$result+($overflow>0?$lit32+($k%$lit94):$lit32);
  }
}

$time = microtime(true) - $time_start;
printf("PH Benchmark: %.5fs\n",$time);
?>
