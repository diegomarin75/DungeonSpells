from timeit import default_timer as timer

#Calculate point
def calculatepoint(iter,maxval,cx,cy):
  
  #Init loop
  x0=cx
  y0=cy
  overflow=False
  
  #Calculation loop
  i=0
  while i<iter:
    x1=(x0*x0)-(y0*y0)+cx
    y1=(2*x0*y0)+cy
    if((x1*x1)+(y1*y1)>maxval*maxval): 
      overflow=True
      break 
    else: 
      x0=x1 
      y0=y1 
    i+=1

  #Return value
  return (i if overflow else -i)

#Print mandelbrot set to console
def mandelbrotset(iter,maxval,x1,x2,stepsx,y1,y2,stepsy):

  #Print loop
  j=0
  while j<stepsy:
    line=""
    i=0
    while i<stepsx:
      cx=x1+float(i)*(x2-x1)/float(stepsx)
      cy=y1+float(j)*(y2-y1)/float(stepsy)
      k=calculatepoint(iter,maxval,cx,cy)
      line+=chr(32+(k%94) if k>0 else 32)
      i+=1
    j+=1

# Start time
start = timer()

#Variables
iter=1000
maxval=1000
x1=-2.1
x2=1.0
stepsx=200
y1=-1.2
y2=1.2
stepsy=100

#Call mandelbrot set
mandelbrotset(iter,maxval,x1,x2,stepsx,y1,y2,stepsy)

#Elapsed time
end = timer()
print("PY Benchmark: {0:.5f}s".format(end-start))