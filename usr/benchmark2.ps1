#Calculate point
function calculatepoint{
  param([int]$iter,[float]$maxval,[float]$cx,[float]$cy)
  
  #Init loop
  $x0=$cx
  $y0=$cy
  $overflow=[int]0
  
  #Calculation loop
  $i=[int]0;   
  while($i -lt $iter){
    $x1=($x0*$x0)-($y0*$y0)+$cx
    $y1=(2*$x0*$y0)+$cy
    if(($x1*$x1)+($y1*$y1) -gt $maxval*$maxval){
      $overflow=1
      break
    }
    else{
      $x0=$x1
      $y0=$y1
    }
    $i=$i+1
  }

  #Return value
  if($overflow -eq 1){
    return $i
  }
  else{
    return -$i
  }

}

function mandelbrotset{
  param([int]$iter, [float]$maxval,[float]$x1,[float]$x2,[int]$stepsx,[float]$y1,[float]$y2,[int]$stepsy)

  #Print loop
  $j=[int]0
  while($j -lt $stepsy){
    $line=""
    $i=[int]0
    while($i -lt $stepsx){
      $cx=$x1+$i*($x2-$x1)/$stepsx
      $cy=$y1+$j*($y2-$y1)/$stepsy
      $k=(calculatepoint -iter $iter -maxval $maxval -cx $cx -cy $cy)
      if($k -gt 0){
        $line=$line+[char](32+($k%94))
      }
      else{
        $line=$line+[char](32)
      }
      $i=$i+1
    }
    $j=$j+1
  }

}

#Start time
$StartTime = $(get-date)

#Variables
$iter=[int]1000
$maxval=[int]1000
$x1=[float]-2.1
$x2=[float]1.0
$stepsx=[int]200
$y1=[float]-1.2
$y2=[float]1.2
$stepsy=[int]100

#Call mandelbrot set
mandelbrotset -iter $iter -maxval $maxval -x1 $x1 -x2 $x2 -stepsx $stepsx -y1 $y1 -y2 $y2 -stepsy $stepsy
$ElapsedTime = $(get-date) - $StartTime
Write-Host ("PS Benchmark: "+("{0:f5}" -f $ElapsedTime.TotalSeconds)+"s")
