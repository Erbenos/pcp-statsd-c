#!/usr/bin/env ruby
require 'open3'
require 'colorize'
require 'socket'

# Test if agent retains all metrics

print "⧖ Stress testing..."
err_count = 0

ds = UDPSocket.new
ds.connect('localhost', 8125)

def restart_agent
  preamble = ". /etc/pcp.conf && cd $PCP_PMDAS_DIR/statsd"
  stdout, stderr, status = Open3.capture3(preamble + " && sudo make deactivate")
  stdout, stderr, status = Open3.capture3(preamble + " && sudo make activate")
end

restart_agent

N = 1000000
ts = []

ts << Thread.new do
  N.times do
    ds.send("thread_1.counter:1|c", 0)
  end
end

ts << Thread.new do
  N.times do
    ds.send("thread_2.counter:2|c", 0)
  end
end

ts << Thread.new do
  N.times do
    ds.send("thread_3.gauge:#{Random.rand(500)}|g", 0)
  end
end

ts << Thread.new do
  time_spent = 0.001
  N.times do
    start = Process.clock_gettime(Process::CLOCK_MONOTONIC)
    ds.send("thread_4.ms:#{sprintf('%8.8f', time_spent * 1000)}|ms", 0)
    finish = Process.clock_gettime(Process::CLOCK_MONOTONIC)
    time_spent = finish - start
  end
end

ts.each(&:join)

# Check agent didnt crash and has all metrics tracked
stdout, stderr, status = Open3.capture3("pminfo statsd")
if stderr.empty? 
  lines = stdout.chomp.split("\n")
  unless lines.length == 19 # 15 hardcoded + 4 new
    err_count = err_count + 1
  end
else  
  err_count = err_count + 1
end

print "\r"
$stdout.flush
if err_count == 0
  puts "✔".green + " Stress test OK                      "
else 
  puts "✖".red + " Stress test failed: " + err_count.to_s + "       "
end
