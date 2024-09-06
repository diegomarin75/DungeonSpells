import datetime
from dateutil.relativedelta import relativedelta

print("125 ------------------")
result=""
bs=datetime.datetime(2021,10,15);  result+="bs="  +str(bs   )+"|"
td=relativedelta(years=1);    result+="y+1=" +str(bs+td)+"|"
td=relativedelta(years=-1);   result+="y-1=" +str(bs+td)+"|"
td=relativedelta(months=25);  result+="m+25="+str(bs+td)+"|"
td=relativedelta(months=-26); result+="m-26="+str(bs+td)+"|"
td=relativedelta(days=25);    result+="d+25="+str(bs+td)+"|"
td=relativedelta(days=-26);   result+="d-26="+str(bs+td)+"|"
print(result)

print("128 ------------------")
dt=datetime.datetime(2021,10,15,23,10,15,123456)
nsecsinday=24*3600*1E9
nsecsinhour=3600*1E9
nsecsinminute=60*1E9
nsecsinsecond=1E9
nsecs=(15*nsecsinday+23*nsecsinhour+10*nsecsinminute+15*nsecsinsecond+123456789)
print("days="+str(nsecs/nsecsinday))
print("hours="+str(nsecs/nsecsinhour))
print("minutes="+str(nsecs/nsecsinminute))
print("seconds="+str(nsecs/nsecsinsecond))
print("millisecs="+str(nsecs/1E6))
print("microsecs="+str(nsecs/1E3))
print("nanosecs="+str(nsecs))
print("base="+str(dt))
delta=datetime.timedelta(days=25);  print("d+25="+str(dt+delta))
delta=datetime.timedelta(days=26);  print("d-26="+str(dt-delta))
delta=datetime.timedelta(hours=50); print("hr+50="+str(dt+delta))
delta=datetime.timedelta(hours=50); print("hr-50="+str(dt-delta))
delta=datetime.timedelta(minutes=500); print("mi+500="+str(dt+delta))
delta=datetime.timedelta(minutes=500); print("mi-500="+str(dt-delta))
delta=datetime.timedelta(seconds=5000); print("se+5009="+str(dt+delta))
delta=datetime.timedelta(seconds=5000); print("se-5009="+str(dt-delta))
delta=datetime.timedelta(milliseconds=5001); print("ms+5001="+str(dt+delta))
delta=datetime.timedelta(milliseconds=5001); print("ms-5001="+str(dt-delta))
delta=datetime.timedelta(microseconds=5001); print("us+5001="+str(dt+delta))
delta=datetime.timedelta(microseconds=5001); print("us-5001="+str(dt-delta))

print("129 ------------------")
result=""
d1=datetime.datetime(2021,10,15); d2=datetime.datetime(2025,10,15); result+="d2-d1 (yr)="+str(d2-d1)+"|"                                                                     
d1=datetime.datetime(2021,10,15); d2=datetime.datetime(2025,10,15); result+="d1-d2 (yr)="+str(d1-d2)+"|"                                                                     
d1=datetime.datetime(2021,10,15); d2=datetime.datetime(2022,12,15); result+="d2-d1 (mo)="+str(d2-d1)+"|"                                                                     
d1=datetime.datetime(2021,10,15); d2=datetime.datetime(2022,12,15); result+="d1-d2 (mo)="+str(d1-d2)+"|"                                                                     
d1=datetime.datetime(2021,10,15); d2=datetime.datetime(2021,11,20); result+="d2-d1 (da)="+str(d2-d1)+"|"                                                                     
d1=datetime.datetime(2021,10,15); d2=datetime.datetime(2021,11,20); result+="d1-d2 (da)="+str(d1-d2)+"|"                                                                     
print(result)


print("130 ------------------")
result=""
t1=datetime.datetime(2000,1,1,14,30,15);            t2=datetime.datetime(2000,1,1,20,30,15);            result+="t2-t1 (hr)="+str(t2-t1)+"|"
t1=datetime.datetime(2000,1,1,14,30,15);            t2=datetime.datetime(2000,1,1,20,30,15);            result+="t1-t2 (hr)="+str(t1-t2)+"|"
t1=datetime.datetime(2000,1,1,14,30,15);            t2=datetime.datetime(2000,1,1,15,35,15);            result+="t2-t1 (mi)="+str(t2-t1)+"|"
t1=datetime.datetime(2000,1,1,14,30,15);            t2=datetime.datetime(2000,1,1,15,35,15);            result+="t1-t2 (mi)="+str(t1-t2)+"|"
t1=datetime.datetime(2000,1,1,14,30,15);            t2=datetime.datetime(2000,1,1,14,35,25);            result+="t2-t1 (se)="+str(t2-t1)+"|"
t1=datetime.datetime(2000,1,1,14,30,15);            t2=datetime.datetime(2000,1,1,14,35,25);            result+="t1-t2 (se)="+str(t1-t2)+"|"
t1=datetime.datetime(2000,1,1,14,30,15,10000);      t2=datetime.datetime(2000,1,1,14,30,25,20000);      result+="t2-t1 (ms)="+str(t2-t1)+"|"
t1=datetime.datetime(2000,1,1,14,30,15,10000);      t2=datetime.datetime(2000,1,1,14,30,25,20000);      result+="t1-t2 (ms)="+str(t1-t2)+"|"
t1=datetime.datetime(2000,1,1,14,30,15,10*1000+20); t2=datetime.datetime(2000,1,1,14,30,25,20*1000+30); result+="t2-t1 (us)="+str(t2-t1)+"|"
t1=datetime.datetime(2000,1,1,14,30,15,10*1000+20); t2=datetime.datetime(2000,1,1,14,30,25,20*1000+30); result+="t1-t2 (us)="+str(t1-t2)+"|"
print(result)

print("131 ------------------")
result=""
dt1=datetime.datetime(2021,10,15);                     dt2=datetime.datetime(2025,10,15);                     result+="dt2-dt1 (yr)="+str(dt2-dt1)+"|"                   
dt1=datetime.datetime(2021,10,15);                     dt2=datetime.datetime(2025,10,15);                     result+="dt1-dt2 (yr)="+str(dt1-dt2)+"|"                   
dt1=datetime.datetime(2021,10,15);                     dt2=datetime.datetime(2022,12,15);                     result+="dt2-dt1 (mo)="+str(dt2-dt1)+"|"                   
dt1=datetime.datetime(2021,10,15);                     dt2=datetime.datetime(2022,12,15);                     result+="dt1-dt2 (mo)="+str(dt1-dt2)+"|"                   
dt1=datetime.datetime(2021,10,15);                     dt2=datetime.datetime(2021,11,20);                     result+="dt2-dt1 (da)="+str(dt2-dt1)+"|"                   
dt1=datetime.datetime(2021,10,15);                     dt2=datetime.datetime(2021,11,20);                     result+="dt1-dt2 (da)="+str(dt1-dt2)+"|"                   
dt1=datetime.datetime(2021,10,15,14,30,15);            dt2=datetime.datetime(2021,11,20,20,30,15);            result+="dt2-dt1 (hr)="+str(dt2-dt1)+"|"                   
dt1=datetime.datetime(2021,10,15,14,30,15);            dt2=datetime.datetime(2021,11,20,20,30,15);            result+="dt1-dt2 (hr)="+str(dt1-dt2)+"|"                   
dt1=datetime.datetime(2021,10,15,14,30,15);            dt2=datetime.datetime(2021,11,20,15,35,15);            result+="dt2-dt1 (mi)="+str(dt2-dt1)+"|"                   
dt1=datetime.datetime(2021,10,15,14,30,15);            dt2=datetime.datetime(2021,11,20,15,35,15);            result+="dt1-dt2 (mi)="+str(dt1-dt2)+"|"                   
dt1=datetime.datetime(2021,10,15,14,30,15);            dt2=datetime.datetime(2021,11,20,14,35,25);            result+="dt2-dt1 (se)="+str(dt2-dt1)+"|"                   
dt1=datetime.datetime(2021,10,15,14,30,15);            dt2=datetime.datetime(2021,11,20,14,35,25);            result+="dt1-dt2 (se)="+str(dt1-dt2)+"|"                   
dt1=datetime.datetime(2021,10,15,14,30,15,10*1000);    dt2=datetime.datetime(2021,11,20,14,30,25,20*1000);    result+="dt2-dt1 (ms)="+str(dt2-dt1)+"|"                   
dt1=datetime.datetime(2021,10,15,14,30,15,10*1000);    dt2=datetime.datetime(2021,11,20,14,30,25,20*1000);    result+="dt1-dt2 (ms)="+str(dt1-dt2)+"|"                   
dt1=datetime.datetime(2021,10,15,14,30,15,10*1000+20); dt2=datetime.datetime(2021,11,20,14,30,25,20*1000+30); result+="dt2-dt1 (us)="+str(dt2-dt1)+"|"                   
dt1=datetime.datetime(2021,10,15,14,30,15,10*1000+20); dt2=datetime.datetime(2021,11,20,14,30,25,20*1000+30); result+="dt1-dt2 (us)="+str(dt1-dt2)+"|"                   
print(result)

print("132 ------------------")
result=""
d1=datetime.datetime(2021,10,15); ts=datetime.timedelta(days=+500); result+="d1+(+500d)="+str(d1+ts)+"|"
d1=datetime.datetime(2021,10,15); ts=datetime.timedelta(days=-500); result+="d1+(-500d)="+str(d1+ts)+"|"
d1=datetime.datetime(2021,10,15); ts=datetime.timedelta(days=+80);  result+="d1+(+80d)="+str(d1+ts)+"|"
d1=datetime.datetime(2021,10,15); ts=datetime.timedelta(days=-80);  result+="d1+(-80d)="+str(d1+ts)+"|"
d1=datetime.datetime(2021,10,15); ts=datetime.timedelta(days=+5);   result+="d1+(+5d)="+str(d1+ts)+"|"
d1=datetime.datetime(2021,10,15); ts=datetime.timedelta(days=-5);   result+="d1+(-5d)="+str(d1+ts)+"|"
print(result)

print("133 ------------------")
result=""
t1=datetime.datetime(2000,1,1,14,10,15,100*1000+100); ts=datetime.timedelta(days=0,hours=+26,minutes=0,seconds=0);                                    result+="t1+(+26h)="+str(t1+ts)+"|"
t1=datetime.datetime(2000,1,1,14,10,15,100*1000+100); ts=datetime.timedelta(days=0,hours=-26,minutes=0,seconds=0);                                    result+="t1+(-26h)="+str(t1+ts)+"|"
t1=datetime.datetime(2000,1,1,14,10,15,100*1000+100); ts=datetime.timedelta(days=0,hours=+1,minutes=+45,seconds=0);                                   result+="t1+(+1h+45m)="+str(t1+ts)+"|"
t1=datetime.datetime(2000,1,1,14,10,15,100*1000+100); ts=datetime.timedelta(days=0,hours=-1,minutes=-45,seconds=0);                                   result+="t1+(-1h-45m)="+str(t1+ts)+"|"
t1=datetime.datetime(2000,1,1,14,10,15,100*1000+100); ts=datetime.timedelta(days=0,hours=+1,minutes=+1,seconds=+45);                                  result+="t1+(+1h+1m+45s)="+str(t1+ts)+"|"
t1=datetime.datetime(2000,1,1,14,10,15,100*1000+100); ts=datetime.timedelta(days=0,hours=-1,minutes=-1,seconds=-45);                                  result+="t1+(-1h-1m-45s)="+str(t1+ts)+"|"
t1=datetime.datetime(2000,1,1,14,10,15,100*1000+100); ts=datetime.timedelta(days=0,hours=+1,minutes=+1,seconds=+1,milliseconds=+500);                 result+="t1+(+1h+1m+1s+500ms)="+str(t1+ts)+"|"
t1=datetime.datetime(2000,1,1,14,10,15,100*1000+100); ts=datetime.timedelta(days=0,hours=-1,minutes=-1,seconds=-1,milliseconds=-500);                 result+="t1+(-1h-1m-1s-500ms)="+str(t1+ts)+"|"
t1=datetime.datetime(2000,1,1,14,10,15,100*1000+100); ts=datetime.timedelta(days=0,hours=+1,minutes=+1,seconds=+1,milliseconds=+1,microseconds=+500); result+="t1+(+1h+1m+1s+ms+500us)="+str(t1+ts)+"|"
t1=datetime.datetime(2000,1,1,14,10,15,100*1000+100); ts=datetime.timedelta(days=0,hours=-1,minutes=-1,seconds=-1,milliseconds=-1,microseconds=-500); result+="t1+(-1h-1m-1s-ms-500us)="+str(t1+ts)+"|"
print(result)

print("134 ------------------")
result=""
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=+500);                                                                result+="dt1+(+500d)="+str(dt1+ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=-500);                                                                result+="dt1+(-500d)="+str(dt1+ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=+80);                                                                 result+="dt1+(+80d)="+str(dt1+ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=-80);                                                                 result+="dt1+(-80d)="+str(dt1+ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=+5);                                                                  result+="dt1+(+5d)="+str(dt1+ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=-5);                                                                  result+="dt1+(-5d)="+str(dt1+ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=+1,hours=+26,minutes=0,seconds=0);                                    result+="dt1+(+5d+26h)="+str(dt1+ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=-1,hours=-26,minutes=0,seconds=0);                                    result+="dt1+(-5d-26h)="+str(dt1+ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=+1,hours=+1,minutes=+45,seconds=0);                                   result+="dt1+(+1d+1h+45m)="+str(dt1+ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=-1,hours=-1,minutes=-45,seconds=0);                                   result+="dt1+(-1d-1h-45m)="+str(dt1+ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=+1,hours=+1,minutes=+1,seconds=+45);                                  result+="dt1+(+1d+1h+1m+45s)="+str(dt1+ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=-1,hours=-1,minutes=-1,seconds=-45);                                  result+="dt1+(-1d-1h-1m-45s)="+str(dt1+ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=+1,hours=+1,minutes=+1,seconds=+1,milliseconds=+500);                 result+="dt1+(+1d+1h+1m+1s+500ms)="+str(dt1+ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=-1,hours=-1,minutes=-1,seconds=-1,milliseconds=-500);                 result+="dt1+(-1d-1h-1m-1s-500ms)="+str(dt1+ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=+1,hours=+1,minutes=+1,seconds=+1,milliseconds=+1,microseconds=+500); result+="dt1+(+1d+1h+1m+1s+ms+500us)="+str(dt1+ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=-1,hours=-1,minutes=-1,seconds=-1,milliseconds=-1,microseconds=-500); result+="dt1+(-1d-1h-1m-1s-ms-500us)="+str(dt1+ts)+"|"
print(result)

print("135 ------------------")
result=""
d1=datetime.datetime(2021,10,15); ts=datetime.timedelta(days=+500); result+="d1-(+500d)="+str(d1-ts)+"|"
d1=datetime.datetime(2021,10,15); ts=datetime.timedelta(days=-500); result+="d1-(-500d)="+str(d1-ts)+"|"
d1=datetime.datetime(2021,10,15); ts=datetime.timedelta(days=+80);  result+="d1-(+80d)="+str(d1-ts)+"|"
d1=datetime.datetime(2021,10,15); ts=datetime.timedelta(days=-80);  result+="d1-(-80d)="+str(d1-ts)+"|"
d1=datetime.datetime(2021,10,15); ts=datetime.timedelta(days=+5);   result+="d1-(+5d)="+str(d1-ts)+"|"
d1=datetime.datetime(2021,10,15); ts=datetime.timedelta(days=-5);   result+="d1-(-5d)="+str(d1-ts)+"|"
print(result)

print("136 ------------------")
result=""
t1=datetime.datetime(2000,1,1,14,10,15,100*1000+100); ts=datetime.timedelta(days=0,hours=+26,minutes=0,seconds=0);                                    result+="t1-(+26h)="+str(t1-ts)+"|"
t1=datetime.datetime(2000,1,1,14,10,15,100*1000+100); ts=datetime.timedelta(days=0,hours=-26,minutes=0,seconds=0);                                    result+="t1-(-26h)="+str(t1-ts)+"|"
t1=datetime.datetime(2000,1,1,14,10,15,100*1000+100); ts=datetime.timedelta(days=0,hours=+1,minutes=+45,seconds=0);                                   result+="t1-(+1h+45m)="+str(t1-ts)+"|"
t1=datetime.datetime(2000,1,1,14,10,15,100*1000+100); ts=datetime.timedelta(days=0,hours=-1,minutes=-45,seconds=0);                                   result+="t1-(-1h-45m)="+str(t1-ts)+"|"
t1=datetime.datetime(2000,1,1,14,10,15,100*1000+100); ts=datetime.timedelta(days=0,hours=+1,minutes=+1,seconds=+45);                                  result+="t1-(+1h+1m+45s)="+str(t1-ts)+"|"
t1=datetime.datetime(2000,1,1,14,10,15,100*1000+100); ts=datetime.timedelta(days=0,hours=-1,minutes=-1,seconds=-45);                                  result+="t1-(-1h-1m-45s)="+str(t1-ts)+"|"
t1=datetime.datetime(2000,1,1,14,10,15,100*1000+100); ts=datetime.timedelta(days=0,hours=+1,minutes=+1,seconds=+1,milliseconds=+500);                 result+="t1-(+1h+1m+1s+500ms)="+str(t1-ts)+"|"
t1=datetime.datetime(2000,1,1,14,10,15,100*1000+100); ts=datetime.timedelta(days=0,hours=-1,minutes=-1,seconds=-1,milliseconds=-500);                 result+="t1-(-1h-1m-1s-500ms)="+str(t1-ts)+"|"
t1=datetime.datetime(2000,1,1,14,10,15,100*1000+100); ts=datetime.timedelta(days=0,hours=+1,minutes=+1,seconds=+1,milliseconds=+1,microseconds=+500); result+="t1-(+1h+1m+1s+ms+500us)="+str(t1-ts)+"|"
t1=datetime.datetime(2000,1,1,14,10,15,100*1000+100); ts=datetime.timedelta(days=0,hours=-1,minutes=-1,seconds=-1,milliseconds=-1,microseconds=-500); result+="t1-(-1h-1m-1s-ms-500us)="+str(t1-ts)+"|"
print(result)

print("137 ------------------")
result=""
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=+500);                                                                result+="dt1+(+500d)="+str(dt1-ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=-500);                                                                result+="dt1+(-500d)="+str(dt1-ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=+80);                                                                 result+="dt1+(+80d)="+str(dt1-ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=-80);                                                                 result+="dt1+(-80d)="+str(dt1-ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=+5);                                                                  result+="dt1+(+5d)="+str(dt1-ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=-5);                                                                  result+="dt1+(-5d)="+str(dt1-ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=+1,hours=+26,minutes=0,seconds=0);                                    result+="dt1+(+5d+26h)="+str(dt1-ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=-1,hours=-26,minutes=0,seconds=0);                                    result+="dt1+(-5d-26h)="+str(dt1-ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=+1,hours=+1,minutes=+45,seconds=0);                                   result+="dt1+(+1d+1h+45m)="+str(dt1-ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=-1,hours=-1,minutes=-45,seconds=0);                                   result+="dt1+(-1d-1h-45m)="+str(dt1-ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=+1,hours=+1,minutes=+1,seconds=+45);                                  result+="dt1+(+1d+1h+1m+45s)="+str(dt1-ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=-1,hours=-1,minutes=-1,seconds=-45);                                  result+="dt1+(-1d-1h-1m-45s)="+str(dt1-ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=+1,hours=+1,minutes=+1,seconds=+1,milliseconds=+500);                 result+="dt1+(+1d+1h+1m+1s+500ms)="+str(dt1-ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=-1,hours=-1,minutes=-1,seconds=-1,milliseconds=-500);                 result+="dt1+(-1d-1h-1m-1s-500ms)="+str(dt1-ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=+1,hours=+1,minutes=+1,seconds=+1,milliseconds=+1,microseconds=+500); result+="dt1+(+1d+1h+1m+1s+ms+500us)="+str(dt1-ts)+"|"
dt1=datetime.datetime(2021,10,15,14,10,15,100*1000+100); ts=datetime.timedelta(days=-1,hours=-1,minutes=-1,seconds=-1,milliseconds=-1,microseconds=-500); result+="dt1+(-1d-1h-1m-1s-ms-500us)="+str(dt1-ts)+"|"
print(result)

print("138 ------------------")
result=""
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=+500);                                                                result+="bs+(+500d)="+str(bs+ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=-500);                                                                result+="bs+(-500d)="+str(bs+ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=+80);                                                                 result+="bs+(+80d)="+str(bs+ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=-80);                                                                 result+="bs+(-80d)="+str(bs+ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=+5);                                                                  result+="bs+(+5d)="+str(bs+ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=-5);                                                                  result+="bs+(-5d)="+str(bs+ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=+1,hours=+26,minutes=0,seconds=0);                                    result+="bs+(+1d+26h)="+str(bs+ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=-1,hours=-26,minutes=0,seconds=0);                                    result+="bs+(-1d-26h)="+str(bs+ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=+1,hours=+1,minutes=+45,seconds=0);                                   result+="bs+(+1d+1h+45m)="+str(bs+ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=-1,hours=-1,minutes=-45,seconds=0);                                   result+="bs+(-1d-1h-45m)="+str(bs+ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=+1,hours=+1,minutes=+1,seconds=+45);                                  result+="bs+(+1d+1h+1m+45s)="+str(bs+ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=-1,hours=-1,minutes=-1,seconds=-45);                                  result+="bs+(-1d-1h-1m-45s)="+str(bs+ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=+1,hours=+1,minutes=+1,seconds=+1,milliseconds=+500);                 result+="bs+(+1d+1h+1m+1s+500ms)="+str(bs+ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=-1,hours=-1,minutes=-1,seconds=-1,milliseconds=-500);                 result+="bs+(-1d-1h-1m-1s-500ms)="+str(bs+ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=+1,hours=+1,minutes=+1,seconds=+1,milliseconds=+1,microseconds=+500); result+="bs+(+1d+1h+1m+1s+ms+500us)="+str(bs+ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=-1,hours=-1,minutes=-1,seconds=-1,milliseconds=-1,microseconds=-500); result+="bs+(-1d-1h-1m-1s-ms-500us)="+str(bs+ts)+"|"
print(result)

print("139 ------------------")
result=""
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=+500);                                                                result+="bs-(+500d)="+str(bs-ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=-500);                                                                result+="bs-(-500d)="+str(bs-ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=+80);                                                                 result+="bs-(+80d)="+str(bs-ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=-80);                                                                 result+="bs-(-80d)="+str(bs-ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=+5);                                                                  result+="bs-(+5d)="+str(bs-ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=-5);                                                                  result+="bs-(-5d)="+str(bs-ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=+1,hours=+26,minutes=0,seconds=0);                                    result+="bs-(+1d+26h)="+str(bs-ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=-1,hours=-26,minutes=0,seconds=0);                                    result+="bs-(-1d-26h)="+str(bs-ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=+1,hours=+1,minutes=+45,seconds=0);                                   result+="bs-(+1d+1h+45m)="+str(bs-ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=-1,hours=-1,minutes=-45,seconds=0);                                   result+="bs-(-1d-1h-45m)="+str(bs-ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=+1,hours=+1,minutes=+1,seconds=+45);                                  result+="bs-(+1d+1h+1m+45s)="+str(bs-ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=-1,hours=-1,minutes=-1,seconds=-45);                                  result+="bs-(-1d-1h-1m-45s)="+str(bs-ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=+1,hours=+1,minutes=+1,seconds=+1,milliseconds=+500);                 result+="bs-(+1d+1h+1m+1s+500ms)="+str(bs-ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=-1,hours=-1,minutes=-1,seconds=-1,milliseconds=-500);                 result+="bs-(-1d-1h-1m-1s-500ms)="+str(bs-ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=+1,hours=+1,minutes=+1,seconds=+1,milliseconds=+1,microseconds=+500); result+="bs-(+1d+1h+1m+1s+ms+500us)="+str(bs-ts)+"|"
bs=datetime.timedelta(days=1,hours=1,minutes=1,seconds=1,milliseconds=1,microseconds=1); ts=datetime.timedelta(days=-1,hours=-1,minutes=-1,seconds=-1,milliseconds=-1,microseconds=-500); result+="bs-(-1d-1h-1m-1s-ms-500us)="+str(bs-ts)+"|"
print(result)

print("140 ------------------")
result=""
d1 =datetime.datetime(1000,1,1,0,0,0,0); ts=datetime.timedelta(days=1);         result+="dz+(1d)="+str(d1+ts)+"|\n"
d1 =datetime.datetime(1000,1,1,0,0,0,0); ts=datetime.timedelta(days=1);         result+="dz-(1d)="+str(d1-ts)+"|\n"
t1 =datetime.datetime(1000,1,1,0,0,0,0); ts=datetime.timedelta(hours=1);        result+="tz+(1hr)="+str(t1+ts)+"|\n"
t1 =datetime.datetime(1000,1,1,0,0,0,0); ts=datetime.timedelta(hours=1);        result+="tz-(1hr)="+str(t1-ts)+"|\n"
t1 =datetime.datetime(1000,1,1,0,0,0,0); ts=datetime.timedelta(minutes=1);      result+="tz+(1mi)="+str(t1+ts)+"|\n"
t1 =datetime.datetime(1000,1,1,0,0,0,0); ts=datetime.timedelta(minutes=1);      result+="tz-(1mi)="+str(t1-ts)+"|\n"
t1 =datetime.datetime(1000,1,1,0,0,0,0); ts=datetime.timedelta(seconds=1);      result+="tz+(1se)="+str(t1+ts)+"|\n"
t1 =datetime.datetime(1000,1,1,0,0,0,0); ts=datetime.timedelta(seconds=1);      result+="tz-(1se)="+str(t1-ts)+"|\n"
t1 =datetime.datetime(1000,1,1,0,0,0,0); ts=datetime.timedelta(milliseconds=1); result+="tz+(1ms)="+str(t1+ts)+"|\n"
t1 =datetime.datetime(1000,1,1,0,0,0,0); ts=datetime.timedelta(milliseconds=1); result+="tz-(1ms)="+str(t1-ts)+"|\n"
t1 =datetime.datetime(1000,1,1,0,0,0,0); ts=datetime.timedelta(microseconds=1); result+="tz+(1us)="+str(t1+ts)+"|\n"
t1 =datetime.datetime(1000,1,1,0,0,0,0); ts=datetime.timedelta(microseconds=1); result+="tz-(1us)="+str(t1-ts)+"|\n"
dt1=datetime.datetime(1000,1,1,0,0,0,0); ts=datetime.timedelta(days=1);         result+="dtz+(1da)="+str(dt1+ts)+"|\n"
dt1=datetime.datetime(1000,1,1,0,0,0,0); ts=datetime.timedelta(days=1);         result+="dtz-(1da)="+str(dt1-ts)+"|\n"
dt1=datetime.datetime(1000,1,1,0,0,0,0); ts=datetime.timedelta(hours=1);        result+="dtz+(1hr)="+str(dt1+ts)+"|\n"
dt1=datetime.datetime(1000,1,1,0,0,0,0); ts=datetime.timedelta(hours=1);        result+="dtz-(1hr)="+str(dt1-ts)+"|\n"
dt1=datetime.datetime(1000,1,1,0,0,0,0); ts=datetime.timedelta(minutes=1);      result+="dtz+(1mi)="+str(dt1+ts)+"|\n"
dt1=datetime.datetime(1000,1,1,0,0,0,0); ts=datetime.timedelta(minutes=1);      result+="dtz-(1mi)="+str(dt1-ts)+"|\n"
dt1=datetime.datetime(1000,1,1,0,0,0,0); ts=datetime.timedelta(seconds=1);      result+="dtz+(1se)="+str(dt1+ts)+"|\n"
dt1=datetime.datetime(1000,1,1,0,0,0,0); ts=datetime.timedelta(seconds=1);      result+="dtz-(1se)="+str(dt1-ts)+"|\n"
dt1=datetime.datetime(1000,1,1,0,0,0,0); ts=datetime.timedelta(milliseconds=1); result+="dtz+(1ms)="+str(dt1+ts)+"|\n"
dt1=datetime.datetime(1000,1,1,0,0,0,0); ts=datetime.timedelta(milliseconds=1); result+="dtz-(1ms)="+str(dt1-ts)+"|\n"
dt1=datetime.datetime(1000,1,1,0,0,0,0); ts=datetime.timedelta(microseconds=1); result+="dtz+(1us)="+str(dt1+ts)+"|\n"
dt1=datetime.datetime(1000,1,1,0,0,0,0); ts=datetime.timedelta(microseconds=1); result+="dtz-(1us)="+str(dt1-ts)+"|\n"
print(result)
