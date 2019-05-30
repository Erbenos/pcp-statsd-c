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

echo "login:1|c"    | call_endpoint
echo "login:3|c"    | call_endpoint
echo "login:5|c"    | call_endpoint
echo "logout:4|c"   | call_endpoint
echo "logout:2|c"   | call_endpoint
echo "logout:2|c"   | call_endpoint

## Results:
## login = 8
## logout = 8
############################

############################
# Incorrect case - value includes incorrect character

echo "session_started:1wq|c"    | call_endpoint
echo "cache_cleared:4ěš|c"      | call_endpoint
echo "session_started:1_4w|c"   | call_endpoint

## Results:
## Should be thrown away
############################


############################
# Incorrect case - value is negative

echo "session_started:-1|c" | call_endpoint
echo "cache_cleared:-4|c"   | call_endpoint
echo "cache_cleared:-1|c"   | call_endpoint

## Results:
## This will successfuly get parsed but will be thrown away at later time when trying to update / create metric value in consumer
############################


############################
# Incorrect case - incorrect type specifier

echo "session_started:1|cx"     | call_endpoint
echo "cache_cleared:4|cw"       | call_endpoint
echo "cache_cleared:1|rc"       | call_endpoint

## Results:
## Should be thrown away
############################

############################
# Incorrect case - value is missing

echo "session_started:|c"     | call_endpoint

## Results:
## Should be thrown away
############################

############################
# Incorrect case - metric is missing

echo ":20|c"     | call_endpoint

## Results:
## Should be thrown away
############################

pid=$(pgrep pcp-statsd.out)
kill -USR1 $pid
# I have no idea how to watch output of parallel task and block until given text is found
sleep 5
kill -INT $pid
