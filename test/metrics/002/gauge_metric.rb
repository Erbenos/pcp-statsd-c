#!/usr/bin/env ruby
require 'open3'
require 'colorize'
require 'socket'

# Test if gauge metric datagrams are processed properly

print "⧖ Checking gauge metric functionality..."
err_count = 0

ds = UDPSocket.new
ds.connect('localhost', 8125)

payloads = [  
  # ok
  0,
  1,
  10, 
  100,
  1000,
  10000,
  "+1",
  "+10",
  "+100",
  "+1000",
  "+10000",
  "-0.1",
  "-0.01",
  "-0.001",
  "-0.0001",
  "-0.00001",
  # thrown away 
  "-1wqeqe",
  "-20weqe0",
  "-wqewqe20"
]

expected_dropped_count = 5
stdout, stderr, status = Open3.capture3("pminfo statsd.pmda.dropped -f")
lines = stdout.chomp.split "\n"
current_dropped_count = lines[2].split(" ").last.to_i

gauge_expected_result = 10000 + 11111 - 0.99999

payloads.each { |payload|
  ds.send("test_gauge:" + payload.to_s + "|g", 0)
  sleep(0.1)
} 

# Check received stat
stdout, stderr, status = Open3.capture3("pminfo statsd.test_gauge -f")
if !stderr.empty? && stdout.include?("value " + gauge_expected_result.to_s)
  err_count = err_count + 1
end


# Check double bounds
test_payload = (Float::MAX * 0.6).round
# Check for overflow
ds.send("test_gauge_overflow:+" + test_payload.to_s + "|g", 0);
ds.send("test_gauge_overflow:+" + test_payload.to_s + "|g", 0);

stdout, stderr, status = Open3.capture3("pminfo statsd.test_gauge_overflow -f")
# this should match, because we recorded value only once, second datagram resulted in overflow and was ignored
unless stderr.empty? && stdout.include?("value 1.078615880917389e+308") # this is equal to Float::MAX * 0.6
  err_count = err_count + 1
end

# Check for underflow
ds.send("test_gauge_underflow:-" + test_payload.to_s + "|g", 0);
ds.send("test_gauge_underflow:-" + test_payload.to_s + "|g", 0);

stdout, stderr, status = Open3.capture3("pminfo statsd.test_gauge_underflow -f")
# this should match, because we recorded value only once, second datagram resulted in underflow and was ignored
unless stderr.empty? && stdout.include?("value -1.078615880917389e+308") # this is equal to Float::MAX * 0.6
  err_count = err_count + 1
end

expected_val = (current_dropped_count + expected_dropped_count).to_s
stdout, stderr, status = Open3.capture3("pminfo statsd.pmda.dropped -f")
unless stderr.empty? && stdout.include?("value " + expected_val)
  err_count = err_count + 1
end

print "\r"

$stdout.flush
if err_count == 0
  puts "✔".green + " Gauge stat updating accordingly                 "
else 
  puts "✖".red + " Gauge stat wasn't updated as expected: " + err_count.to_s
end
