from timeit import default_timer as timer

#Calculate point
def calculatepoint(iter,maxval,cx,cy):
  
  #Init loop
  x0=cx
  y0=cy
  overflow=False
  
  #Calculation loop
  for i in range(iter):
    x1=(x0*x0)-(y0*y0)+cx
    y1=(2*x0*y0)+cy
    if((x1*x1)+(y1*y1)>maxval*maxval): 
      overflow=True
      break 
    else: 
      x0=x1 
      y0=y1 

  #Return value
  return (i if overflow else -i)

#Print mandelbrot set to console
def mandelbrotset(iter,maxval,x1,x2,stepsx,y1,y2,stepsy):

  #Print loop
  for j in range(stepsy):
    line=""
    for i in range(stepsx):
      cx=x1+float(i)*(x2-x1)/float(stepsx)
      cy=y1+float(j)*(y2-y1)/float(stepsy)
      k=calculatepoint(iter,maxval,cx,cy)
      line+=chr(32+(k%94) if k>0 else 32)
    print(line[0:stepsx])

# Start time
start = timer()

#Version
version="Mandelbrot set calculation"

#Variables
iter=1000
maxval=1000
x1=-2.1
x2=1.0
stepsx=180
y1=-1.2
y2=1.2
stepsy=90

#Message
print(version)
print("--- Parameters ---")
print("iter  : "+str(iter))
print("maxval: "+str(maxval))
print("stepsx: "+str(stepsx))
print("stepsy: "+str(stepsy))
print("x-axis: "+str(x1)+" - "+str(x2))
print("y-axis: "+str(y1)+" - "+str(y2))

#Call mandelbrot set
mandelbrotset(iter,maxval,x1,x2,stepsx,y1,y2,stepsy)

#Elapsed time
end = timer()
print("PY Benchmark: {0:.2f}s".format(end-start))