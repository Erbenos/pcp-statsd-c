#!/bin/bash

call_endpoint() {
    nc -w 1 -u 0.0.0.0 8125
}

############################
# Simple correct case

echo "cpu_wait:200|ms"  | call_endpoint
echo "cpu_wait:100|ms"  | call_endpoint
echo "cpu_wait:200|ms"  | call_endpoint
echo "cpu_busy:100|ms"  | call_endpoint
echo "cpu_busy:10|ms"   | call_endpoint
echo "cpu_busy:20|ms"   | call_endpoint

## Results:
## cpu_wait
###[Mean    =      166.667, StdDeviation   =       47.140]
###[Max     =      200.000, Total count    =            6]
###[Buckets =           22, SubBuckets     =         2048]
## 
## cpu_busy
###[Mean    =       43.333, StdDeviation   =       40.277]
###[Max     =      100.000, Total count    =            3]
###[Buckets =           22, SubBuckets     =         2048]
############################