#Start time
start_time <- Sys.time()

#Variables
iter <- 1000
maxval <- 1000
xmin <- -2.1
xmax <- 1.0
stepsx <- 200
ymin <- -1.2
ymax <- 1.2
stepsy <- 100

#Print loop
lit2 <- 2.0
lit32 <- 32
lit94 <- 94
result <- 0
for(b in 0:stepsy-1){
  for(a in 0:stepsx-1){
    cx <- xmin+a*(xmax-xmin)/stepsx
    cy <- ymin+b*(ymax-ymin)/stepsy
    x0 <- cx
    y0 <- cy
    overflow <- FALSE
    for(k in 0:iter-1){
      x1 <- (x0*x0)-(y0*y0)+cx
      y1 <- (lit2*x0*y0)+cy
      if((x1*x1)+(y1*y1)>maxval*maxval){
        overflow <- TRUE
        break 
      }
      else{
        x0 <- x1 
        y0 <- y1 
      }
    }
    result <- result + ( if(overflow>0) lit32+(k%%lit94) else lit32 ) 
  }
}

#Elapsed time
time <- Sys.time() - start_time
cat(gsub("_"," ",gsub(" ","",paste("RL_Benchmark:_",format(as.numeric(time),digits <- 5),"s"))))
