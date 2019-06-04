#!/bin/bash

call_endpoint() {
    nc -w 1 -u 0.0.0.0 8125
}

############################
# Simple correct case

echo "login:1|g"        | call_endpoint
echo "login:3|g"        | call_endpoint
echo "login:5|g"        | call_endpoint
echo "logout:4|g"       | call_endpoint
echo "logout:2|g"       | call_endpoint
echo "logout:2|g"       | call_endpoint
echo "login:+0.5|g"     | call_endpoint
echo "login:-0.12|g"    | call_endpoint
echo "logout:0.128|g"   | call_endpoint

## Results:
## login = 5
## logout = 2
############################


############################
# Add and decrement cases

echo "success:0|g"      | call_endpoint
echo "success:+5|g"     | call_endpoint
echo "success:-12|g"    | call_endpoint
echo "error:0|g"        | call_endpoint
echo "error:+9|g"       | call_endpoint
echo "error:-0|g"       | call_endpoint

## Results:
## success = -7 
## error = 9
############################


############################
# Incorrect case - value includes incorrect character

echo "session_started:1wq|g"    | call_endpoint
echo "cache_cleared:4ěš|g"      | call_endpoint
echo "session_started:1_4w|g"   | call_endpoint

## Results:
## Should be thrown away
############################


############################
# Incorrect case - value is negative NaN

echo "session_started:-we|g"    | call_endpoint
echo "cache_cleared:-0ě2|g"     | call_endpoint
echo "cache_cleared:-02x|g"     | call_endpoint

## Results:
## Should be thrown away
############################


############################
# Incorrect case - incorrect type specifier

echo "session_started:1|gx"     | call_endpoint
echo "cache_cleared:4|gw"       | call_endpoint
echo "cache_cleared:1|rg"       | call_endpoint

## Results:
## Should be thrown away
############################


############################
# Incorrect case - value is missing

echo "session_started:|g"   | call_endpoint
echo "cache_cleared:|g"     | call_endpoint

## Results:
## Should be thrown away
############################
