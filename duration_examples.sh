#!/bin/bash
make clean && make

make run &

call_endpoint() {
    nc -w 1 -u 0.0.0.0 8125
}

# wait until program is ready to listen
# not sure how else to it now
sleep 3;

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


############################
# Incorrect case - missing count

echo "session_duration:|ms" | call_endpoint
echo "cache_loopup:|ms"     | call_endpoint

## Results:
## Should be thrown away
############################


############################
# Incorrect case - value includes incorrect character

echo "session_duration:1wq|ms"  | call_endpoint
echo "cache_cleared:4ěš|ms"     | call_endpoint
echo "session_started:1_4w|ms"  | call_endpoint

## Results:
## Should be thrown away
############################


############################
# Incorrect case - value is negative

echo "session_started:-1|ms" | call_endpoint
echo "cache_cleared:-4|ms"   | call_endpoint
echo "cache_cleared:-1|ms"   | call_endpoint

## Results:
## This will successfuly get parsed but will be thrown away at later time when trying to update / create metric value in consumer
############################


############################
# Incorrect case - incorrect type specifier

echo "session_started:1|mss"     | call_endpoint
echo "cache_cleared:4|msd"       | call_endpoint
echo "cache_cleared:1|msa"       | call_endpoint

## Results:
## Should be thrown away
############################


############################
# Incorrect case - value is missing

echo "session_started:|ms"      | call_endpoint

## Results:
## Should be thrown away
############################


############################
# Incorrect case - metric is missing

echo ":20|ms"     | call_endpoint

## Results:
## Should be thrown away
############################

pid=$(pgrep pcp-statsd.out)
kill -USR1 $pid
# I have no idea how to watch output of parallel task and block until given text is found
sleep 5
kill -INT $pid
