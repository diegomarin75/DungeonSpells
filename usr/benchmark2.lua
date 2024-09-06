function donothing(line)
  line=""
end

function calculatepoint(iter,maxval,cx,cy)
  
  -- Variables
  local i=0
  local x0=0.0
  local y0=0.0
  local x1=0.0
  local y1=0.0

  -- Init loop
  x0=cx
  y0=cy
  overflow=false
  
  -- Calculation loop
  i=0
  while i<iter do
    x1=(x0*x0)-(y0*y0)+cx
    y1=(2*x0*y0)+cy
    if (x1*x1)+(y1*y1)>maxval*maxval then
      overflow=true
      break 
    else
      x0=x1 
      y0=y1 
    end
    i=i+1
  end

  -- Return value 
  return i,overflow

end

-- Print mandelbrot set to console
function mandelbrotset(iter,maxval,x1,x2,stepsx,y1,y2,stepsy)

  -- Variables
  local cx=0.0
  local cy=0.0
  local i=0
  local j=0
  local k=0
  local overflow=false
  local line=""

  -- Print loop
  j=0
  while j<stepsy do
    line=""
    i=0
    while i<stepsx do
      cx=x1+i*(x2-x1)/stepsx
      cy=y1+j*(y2-y1)/stepsy
      overflow=false
      k,overflow=calculatepoint(iter,maxval,cx,cy)
      if overflow then
        line=line..string.char(32+k%94)
      else
        line=line.." "
      end
      i=i+1
    end
    j=j+1
  end
  donothing(line)

end

-- Time start
local time_start = os.clock()

-- Variables
local iter=1000
local maxval=1000
local x1=-2.1
local x2=1.0
local stepsx=200
local y1=-1.2
local y2=1.2
local stepsy=100

-- Call mandelbrot set
mandelbrotset(iter,maxval,x1,x2,stepsx,y1,y2,stepsy)

-- Time end
local time_end = os.clock()
print(string.format("LU Benchmark: %.5fs", time_end-time_start))