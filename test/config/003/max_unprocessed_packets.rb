require 'socket'
require 'open3'
require 'inifile'
require 'fileutils'
require 'colorize'

print "⧖ Checking max_unprocessed_packets option..."

ds = UDPSocket.new
ds.connect('localhost', 8125)
preamble = ". /etc/pcp.conf && cd $PCP_PMDAS_DIR/statsd"
err_count = 0

stdout, stderr, status = Open3.capture3(preamble + " && echo $PWD")
statsd_pmda_dir = stdout.chomp
statsd_pmda_config = "pmdastatsd.ini"

def restart_agent
  preamble = ". /etc/pcp.conf && cd $PCP_PMDAS_DIR/statsd"
  stdout, stderr, status = Open3.capture3(preamble + " && sudo make deactivate")
  stdout, stderr, status = Open3.capture3(preamble + " && sudo make activate")
end

# try sending datagram
config_file = File.join(statsd_pmda_dir, statsd_pmda_config)
config = IniFile.load(config_file)
config.delete_section "global"
config["global"]["max_unprocessed_packets"] = 1
config.write

restart_agent

ds.send("hello_test:1|c", 0)
sleep(0.1)
stdout, stderr, status = Open3.capture3("pminfo statsd.pmda.aggregated -f")
if !stderr.empty? || !stdout.include?("value 1")
  err_count = err_count + 1
end

# check that options was really updated
sleep(0.1)
stdout, stderr, status = Open3.capture3("pminfo statsd.pmda.settings.max_unprocessed_packets -f")
if !stderr.empty? || !stdout.include?("value 1")
  err_count = err_count + 1
end

# check fallback when invalid value is supplied
config_file = File.join(statsd_pmda_dir, statsd_pmda_config)
config = IniFile.load(config_file)
config.delete_section "global"
config["global"]["max_unprocessed_packets"] = -10
config.write

restart_agent

sleep(0.1)
stdout, stderr, status = Open3.capture3("pminfo statsd.pmda.settings.max_unprocessed_packets -f")
if !stderr.empty? || !stdout.include?("value 2048")
  err_count = err_count + 1
end

print "\r"

$stdout.flush
if err_count == 0
  puts "✔".green + " Option max_unprocessed_packets is ok                  "
else 
  puts "✖".red + " Option max_unprocessed_packets fails: " + err_count.to_s + "   "
end

config_file = File.join(statsd_pmda_dir, statsd_pmda_config)
config = IniFile.load(config_file)
config.delete_section "global"
config.write
