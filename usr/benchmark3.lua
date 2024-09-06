function donothing(result)
  result=0
end

-- Time start
local time_start = os.clock()

-- Calculation parameters
local iter=1000
local maxval=1000
local xmin=-2.1
local xmax=1.0
local stepsx=200
local ymin=-1.2
local ymax=1.2
local stepsy=100

  -- Variables
  local cx=0.0
  local cy=0.0
  local i=0
  local j=0
  local k=0
  local overflow=false
  local line=""
  local x0=0.0
  local y0=0.0
  local x1=0.0
  local y1=0.0
  local result=0
  local lit2=2.0
  local lit32=32
  local lit94=94

  -- Print loop
  j=0
  while j<stepsy do
    i=0
    while i<stepsx do
      cx=xmin+i*(xmax-xmin)/stepsx
      cy=ymin+j*(ymax-ymin)/stepsy
      x0=cx
      y0=cy
      overflow=false
      k=0
      while k<iter do
        x1=(x0*x0)-(y0*y0)+cx
        y1=(lit2*x0*y0)+cy
        if (x1*x1)+(y1*y1)>maxval*maxval then
          overflow=true
          break 
        else
          x0=x1 
          y0=y1 
        end
        k=k+1
      end
      if overflow then
        result=result+(lit32+k%lit94)
      else
        result=result+lit32
      end
      i=i+1
    end
    j=j+1
  end
  donothing(result)

-- Time end
local time_end = os.clock()
print(string.format("LU Benchmark: %.5fs", time_end-time_start))