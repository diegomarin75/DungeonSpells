#!/usr/bin/php
<?php
$time_start = microtime(true);

$z=0;
$i=0;
while($i<10000){
  $j=0;
  while($j<1000){
    $z=2*$z+$j;
    $j+=1;
  }
  $i+=1;
}

$time = microtime(true) - $time_start;
printf("PH Benchmark: %.5fs\n",$time);
?>
