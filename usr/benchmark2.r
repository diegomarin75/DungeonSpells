#Calculate point
calculatepoint <- function(iter,maxval,cx,cy){
  
  #Init loop
  x0 <- cx
  y0 <- cy
  overflow <- FALSE
  
  #Calculation loop
  k <- 0
  while(k<iter){
    x1 <- (x0*x0)-(y0*y0)+cx
    y1 <- (2*x0*y0)+cy
    if((x1*x1)+(y1*y1)>maxval*maxval){
      overflow <- TRUE
      break
    }
    else{
      x0 <- x1 
      y0 <- y1 
    }
    k <- k + 1
  }

  #Return value
  return(if(overflow==TRUE) k else -k)
}

#Print mandelbrot set to console
mandelbrotset <- function(iter,maxval,x1,x2,stepsx,y1,y2,stepsy){

  #Print loop
  b <- 0
  while(b<stepsy){
    line <- ""
    a <- 0
    while(a<stepsx){
      cx <- x1+a*(x2-x1)/stepsx
      cy <- y1+b*(y2-y1)/stepsy
      k <- calculatepoint(iter,maxval,cx,cy)
      line <- paste0(line,intToUtf8(if(k>0) 32+(k%%94) else 32))
      a <- a + 1
    }
    b <- b + 1
  }

}

#Start time
start_time <- Sys.time()

#Variables
iter <- 1000
maxval <- 1000
x1 <- -2.1
x2 <- 1.0
stepsx <- 200
y1 <- -1.2
y2 <- 1.2
stepsy <- 100

#Call mandelbrot set
mandelbrotset(iter,maxval,x1,x2,stepsx,y1,y2,stepsy)

#Elapsed time
time <- Sys.time() - start_time
cat(gsub("_"," ",gsub(" ","",paste("RL_Benchmark:_",format(as.numeric(time),digits <- 5),"s"))))