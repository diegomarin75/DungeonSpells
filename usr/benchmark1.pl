use Time::HiRes qw(time);
$time_start = time();

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

$time = time() - $time_start;
printf "PL Benchmark: %.5fs\n",$time;

