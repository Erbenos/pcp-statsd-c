#!/bin/bash
make clean && make

make run &

# wait until program is ready to listen
# not sure how else to it now
sleep 3;

echo "<TEST DATA SEND START>"
# valid
echo "VALID CASES:"
./test/data/counter/valid.sh
# invalid
echo "INVALID CASES:"
./test/data/counter/invalid.sh
echo "<TEST DATA SEND END>"

pid=$(pgrep pcp-statsd)
kill -USR1 $pid
# I have no idea how to watch output of parallel task and block until given text is found
sleep 5
kill -TERM $pid
