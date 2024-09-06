#!/usr/bin/php
<?php

#Calculate point
function calculatepoint($iter,$maxval,$cx,$cy){
  
  #Init loop
  $x0=$cx;
  $y0=$cy;
  $overflow=0;
  
  #Calculation loop
  $i=0;   
  while($i<$iter){
    $x1=($x0*$x0)-($y0*$y0)+$cx;
    $y1=(2*$x0*$y0)+$cy;
    if(($x1*$x1)+($y1*$y1)>$maxval*$maxval){
      $overflow=1;
      break;
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
function mandelbrotset($iter,$maxval,$x1,$x2,$stepsx,$y1,$y2,$stepsy){

  #Print loop
  $j=0;
  while($j<$stepsy){
    $line="";
    $i=0;
    while($i<$stepsx){
      $cx=$x1+floatval($i)*($x2-$x1)/floatval($stepsx);
      $cy=$y1+floatval($j)*($y2-$y1)/floatval($stepsy);
      $k=calculatepoint($iter,$maxval,$cx,$cy);
      $line.=chr($k>0?32+($k%94):32);
      $i+=1;
    }
    $j+=1;
  }

}

#Start time
$time_start = microtime(true);

#Variables
$iter=1000;
$maxval=1000;
$x1=-2.1;
$x2=1.0;
$stepsx=200;
$y1=-1.2;
$y2=1.2;
$stepsy=100;

#Call mandelbrot set
mandelbrotset($iter,$maxval,$x1,$x2,$stepsx,$y1,$y2,$stepsy);

$time = microtime(true) - $time_start;
printf("PH Benchmark: %.5fs\n",$time);
?>
