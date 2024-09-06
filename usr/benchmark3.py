from timeit import default_timer as timer

# Start time
start = timer()

#Variables
iter=1000
maxval=1000
xmin=-2.1
xmax=1.0
stepsx=200
ymin=-1.2
ymax=1.2
stepsy=100

#Print loop
lit2=2.0
lit32=32
lit94=94
result=0
for j in range(stepsy):
  for i in range(stepsx):
    cx=xmin+i*(xmax-xmin)/stepsx
    cy=ymin+j*(ymax-ymin)/stepsy
    x0=cx
    y0=cy
    overflow=False
    for k in range(iter):
      x1=(x0*x0)-(y0*y0)+cx
      y1=(lit2*x0*y0)+cy
      if((x1*x1)+(y1*y1)>maxval*maxval): 
        overflow=True
        break 
      else: 
        x0=x1 
        y0=y1 
    result+=(lit32+(k%lit94) if overflow>0 else lit32)

#Elapsed time
end = timer()
print("PY Benchmark: {0:.5f}s".format(end-start))