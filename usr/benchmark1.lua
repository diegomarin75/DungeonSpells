local time_start = os.clock()

local i=0
local j=0
local z=0
while(i<10000)
do
  j=0
  while(j<1000)
  do
    z=2*z+j
    j=j+1
  end
  i=i+1
end

local time_end = os.clock()
print(string.format("LU Benchmark: %.5fs", time_end-time_start))