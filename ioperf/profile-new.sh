#!/bin/bash

./a.out &

nsamples=25
sleeptime=0
pid=$(pidof a.out)

for x in $(seq 1 $nsamples)
  do
    gdb -ex "set pagination 0" -ex "thread apply all bt" -batch -p $pid
    sleep $sleeptime
  done | \
awk '
  BEGIN { s = ""; } 
  /^Thread/ { print s; s = ""; } 
  /^\#/ { if (s != "" ) { s = s "," }; if ($3 == "in") { s = s $4 } else if ($4 == "at") { s = s $2 } }
  END { print s }' | \
sort | uniq -c | sort -r -n -k 1,1
