#!/usr/bin/env ruby
require 'open3'
require 'colorize'
require 'socket'

# Test if labels are processed properly

print "⧖ Checking label functionality..."
err_count = 0

def restart_agent
  preamble = ". /etc/pcp.conf && cd $PCP_PMDAS_DIR/statsd"
  stdout, stderr, status = Open3.capture3(preamble + " && sudo make deactivate")
  stdout, stderr, status = Open3.capture3(preamble + " && sudo make activate")
end

ds = UDPSocket.new
ds.connect('localhost', 8125)

payloads = [
  "label_test:0|c", # no label
  "label_test,tagX=X:1|c", # single label
  "label_test,tagX=X,tagY=Y:2|c", # two labels
  "label_test,tagC=C,tagB=B,tagA=A:3|c", # labels that will be ordered
  "label_test:4|c|#A:A", # labels in dogstatsd-ruby format
  "label_test,A=A:5|c|#B:B,C:C", # labels in dogstatsd-ruby format combined with standard format
  "label_test,A=A,A=10:6|c", # labels with non-unique keys, right-most takes precedence
]

payloads.each { |payload|
  ds.send(payload, 0)
  sleep(0.001)
}

# Standard duration
(1..100).each { |payload|
  ds.send("label_test2:" + payload.to_s + "|ms", 0)
  sleep(0.001)
}

# Labeled duration
(1..100).each { |payload|
  ds.send("label_test2:" + (payload * 2).to_s + "|ms|#label:X", 0)
  sleep(0.001)
}

label_test_status = ""
stdout, stderr, status = Open3.capture3("pminfo statsd.label_test -f")
if stderr.empty?
  lines = stdout.split "\n"
  # instance order is arbitrary but deterministic
  unless lines[2].include?('inst [0 or "/"] value 0')
    err_count += 1
    label_test_status << "- " + "✖".red + ' statsd.label_test inst [0 or "/"] is not correct' + "\n"
  end
  unless lines[3].include?('inst [1 or "/tagX=X::tagY=Y"] value 2')
    err_count += 1
    label_test_status << "- " + "✖".red + ' statsd.label_test inst [1 or "/tagX=X::tagY=Y"] is not correct' + "\n"
  end
  unless lines[4].include?('inst [2 or "/tagA=A::tagB=B::tagC=C"]')
    err_count += 1
    label_test_status << "- " + "✖".red + ' statsd.label_test inst [2 or "/tagA=A::tagB=B::tagC=C"] is not correct' + "\n"
  end
  unless lines[5].include?('inst [3 or "/A=A"] value 4')
    err_count += 1
    label_test_status << "- " + "✖".red + ' statsd.label_test inst [3 or "/A=A"] is not correct' + "\n"
  end
  unless lines[6].include?('inst [4 or "/A=A::B=B::C=C"] value 5')
    err_count += 1
    label_test_status << "- " + "✖".red + ' statsd.label_test inst [4 or "/A=A::B=B::C=C"] is not correct' + "\n"
  end
  unless lines[7].include?('inst [5 or "/tagX=X"] value 1')
    err_count += 1
    label_test_status << "- " + "✖".red + ' statsd.label_test inst [5 or "/tagX=X"] is not correct' + "\n"
  end
  unless lines[8].include?('inst [6 or "/A=10"] value 6')
    err_count += 1
    label_test_status << "- " + "✖".red + ' statsd.label_test inst [6 or "/A=10"] is not correct' + "\n"
  end
else
  err_count += 1
end

duration_status = ""
stdout, stderr, status = Open3.capture3("pminfo statsd.label_test2 -f")
if stderr.empty?
  lines = stdout.split "\n"
  # instance order is arbitrary but deterministic
  unless lines[11].include?('inst [9 or "/min::label=X"] value 2')
    err_count += 1
    duration_status << "- " + "✖".red + ' statsd.label_test2 inst [9 or "/min::label=X"] is not correct' + "\n"
  end
  unless lines[12].include?('inst [10 or "/max::label=X"] value 200')
    err_count += 1
    duration_status << "- " + "✖".red + ' statsd.label_test2 inst [10 or "/max::label=X"] is not correct' + "\n"
  end
  unless lines[13].include?('inst [11 or "/median::label=X"] value 100')
    err_count += 1
    duration_status << "- " + "✖".red + ' statsd.label_test2 inst [11 or "/median::label=X"] is not correct' + "\n"
  end
  unless lines[14].include?('inst [12 or "/average::label=X"] value 101')
    err_count += 1
    duration_status << "- " + "✖".red + ' statsd.label_test2 inst [12 or "/average::label=X"] is not correct' + "\n"
  end
  unless lines[15].include?('inst [13 or "/percentile90::label=X"] value 180')
    err_count += 1
    duration_status << "- " + "✖".red + ' statsd.label_test2 inst [13 or "/percentile90::label=X"] is not correct' + "\n"
  end
  unless lines[16].include?('inst [14 or "/percentile95::label=X"] value 190')
    err_count += 1
    duration_status << "- " + "✖".red + ' statsd.label_test2 inst [14 or "/percentile95::label=X"] is not correct' + "\n"
  end
  unless lines[17].include?('inst [15 or "/percentile99::label=X"] value 198')
    err_count += 1
    duration_status << "- " + "✖".red + ' statsd.label_test2 inst [15 or "/percentile99::label=X"] is not correct' + "\n"
  end
  unless lines[18].include?('inst [16 or "/count::label=X"] value 100')
    err_count += 1
    duration_status << "- " + "✖".red + ' statsd.label_test2 inst [16 or "/count::label=X"] is not correct' + "\n"
  end
  unless lines[19].include?('inst [17 or "/std_deviation::label=X"] value 57.73214009544424')
    err_count += 1
    duration_status << "- " + "✖".red + ' statsd.label_test2 inst [17 or "/std_deviation::label=X"] is not correct' + "\n"
  end
else
  err_count += 1
end

print "\r"

$stdout.flush
if err_count == 0
  puts "✔".green + " Labels working as intended                "
else 
  puts "✖".red + " Labels not working as intended: " + err_count.to_s + "    "
  if label_test_status != ""
    puts label_test_status
  end
  if duration_status != ""
    puts duration_status
  end
end
