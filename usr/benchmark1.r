start_time <- Sys.time()
z <- 0
i <- 0
while(i<10000){
  z <- 0
  j <- 0
  while(j<1000){
    z <- 2*z+j
    j <- j+1
  }
  i <- i+1
}
time <- Sys.time() - start_time
cat(gsub("_"," ",gsub(" ","",paste("RL_Benchmark:_",format(as.numeric(time),digits=5),"s"))))
