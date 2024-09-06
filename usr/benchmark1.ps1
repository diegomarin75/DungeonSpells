$StartTime = $(get-date)

$z=0
$i=0
while($i -lt 10000){
  $j=0
  while($j -lt 1000){
    $z=2*$z+$j
    $j+=1
  }
  $i+=1
}

$ElapsedTime = $(get-date) - $StartTime
Write-Host ("PS Benchmark: "+("{0:f5}" -f $ElapsedTime.TotalSeconds)+"s")