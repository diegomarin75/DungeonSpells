from timeit import default_timer as timer
start = timer()
z=0
i=0
while i<10000:
  z=0
  j=0
  while j<1000:
    z=2*z+j
    j+=1
  i+=1
end = timer()
print("PY Benchmark: {0:.5f}s".format(end-start))
