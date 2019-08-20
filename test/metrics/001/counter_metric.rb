#!/usr/bin/env ruby
require 'open3'
require 'colorize'
require 'socket'

# Test if counter metric datagrams are processed properly

print "⧖ Checking counter metric functionality..."
err_count = 0

ds = UDPSocket.new
ds.connect('localhost', 8125)

payloads = [
  # thrown away
  "-1",
  "-1wqeqe",
  "-20weqe0",
  "-wqewqe20",
  # ok
  "0",
  "1",
  "10", 
  "100",
  "1000",
  "10000",
  "+1",
  "+10",
  "+100",
  "+1000",
  "+10000",
  "0.1",
  "0.01",
  "0.001",
  "0.0001",
  "0.00001"
]

expected_dropped_count = 5
stdout, stderr, status = Open3.capture3("pminfo statsd.pmda.dropped -f")
lines = stdout.chomp.split "\n"
current_dropped_count = lines[2].split(" ").last.to_i

counter_expected_result = 22222.1111

payloads.each { |payload|
  ds.send("test_counter:" + payload + "|c", 0)
  sleep(0.1)
} 

# Check received stat
stdout, stderr, status = Open3.capture3("pminfo statsd.test_counter -f")
unless stderr.empty? && stdout.include?("value " + counter_expected_result.to_s)
  err_count = err_count + 1
end

# Check for overflow
test_payload = (Float::MAX * 0.6).round
ds.send("test_counter_overflow:" + test_payload.to_s + "|c", 0);
ds.send("test_counter_overflow:" + test_payload.to_s + "|c", 0);

stdout, stderr, status = Open3.capture3("pminfo statsd.test_counter_overflow -f")
# this should match, because we recorded value only once, second datagram resulted in overflow and was ignored
unless stderr.empty? && stdout.include?("value 1.078615880917389e+308") # this is equal to Float::MAX * 0.6
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
  puts "✔".green + " Counter stat updating accordingly                 "
else 
  puts "✖".red + " Counter stat wasn't updated as expected: " + err_count.to_s
end
