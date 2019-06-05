#!/bin/bash

call_endpoint() {
    nc -w 1 -u 0.0.0.0 8125
}

i=0

echo "example:$i|g" | call_endpoint
echo "example:-$i|g" | call_endpoint
echo "example.counter:1|c" | call_endpoint
echo "example.counter:$i|c" | call_endpoint
echo "example.counter_tens:10|c" | call_endpoint
echo "example.counter_random:$RANDOM|c" | call_endpoint
echo "example.timer:$RANDOM|ms" | call_endpoint
echo "example.gauge:-$i|g" | call_endpoint
echo "example.counter,instance=1:1|c" | call_endpoint
echo "example.gauge,instance=0:+$i|g" | call_endpoint
echo "example.gauge,instance=bazinga:-$i|g" | call_endpoint
echo "example.gauge,x=10:-$i|g" | call_endpoint
echo "example.counter,tagY=Y,tagX=X:1|c" | call_endpoint
echo "example.gauge,x=10,y=20:-$i|g" | call_endpoint
echo "example.counter,tagY=Y,tagX=X,instance=1:1|c" | call_endpoint
echo "example.gauge,i=30,b=10,instance=1:$RANDOM|g" | call_endpoint
echo "example.gauge,x=10,y=20,instance=bazinga:-$i|g" | call_endpoint
echo "example.g a-u/g e,instance=bazinga:-$i|g" | call_endpoint
