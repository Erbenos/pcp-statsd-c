require 'socket'
require 'open3'
require 'inifile'
require 'fileutils'
require 'colorize'

print "⧖ Checking max_udp_packet_size option..."

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

config_file = File.join(statsd_pmda_dir, statsd_pmda_config)
config = IniFile.load(config_file)
config.delete_section "global"
config["global"]["max_udp_packet_size"] = 1472
config.write

restart_agent

# try sending datagram
ds.send("hello_test:1|c", 0)
sleep(0.1)
stdout, stderr, status = Open3.capture3("pminfo statsd.pmda.received -f")
if !stderr.empty? || !stdout.include?("value 1")
    err_count = err_count + 1
end

# try setting too small value
config = IniFile.load(config_file)
config["global"]["max_udp_packet_size"] = 1
config.write

restart_agent

# try sending datagram (should be not be received as it overflows buffer)
ds.send("hello_test:1|c", 0)
sleep(0.1)
stdout, stderr, status = Open3.capture3("pminfo statsd.pmda.received -f")
if !stderr.empty? || !stdout.include?("value 0")
    err_count = err_count + 1
end

# check if small value was used
stdout, stderr, status = Open3.capture3("pminfo statsd.pmda.settings.max_udp_packet_size -f")
if !stderr.empty? || !stdout.include?("value 1")
    err_count = err_count + 1
end

# try setting invalid value
config = IniFile.load(config_file)
config["global"]["max_udp_packet_size"] = -1
config.write

restart_agent

# check if default fallback was used
stdout, stderr, status = Open3.capture3("pminfo statsd.pmda.settings.max_udp_packet_size -f")
if !stderr.empty? || !stdout.include?("value 1472")
    err_count = err_count + 1
end

print "\r"

$stdout.flush
if err_count == 0
  puts "✔".green + " Option max_udp_packet_size is ok                  "
else 
  puts "✖".red + " Option max_udp_packet_size fails: " + err_count.to_s + "   "
end

