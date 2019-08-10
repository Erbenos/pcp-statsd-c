require 'socket'
require 'open3'
require 'inifile'
require 'fileutils'
require 'colorize'

print "⧖ Checking port option..."

ds = UDPSocket.new
ds.connect('localhost', 8123)
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
config["global"]["port"] = 8123
config.write

restart_agent

# try sending datagram
ds.send("hello_test:1|c", 0)
sleep(0.1)
stdout, stderr, status = Open3.capture3("pminfo statsd.pmda.received -f")
if !stderr.empty? || !stdout.include?("value 1")
    err_count = err_count + 1
end

# try sending on incorrect port
ds.connect('localhost', 8124)
ds.send("hello_test:1|c", 0)

sleep(0.1)
stdout, stderr, status = Open3.capture3("pminfo statsd.pmda.received -f")
if !stderr.empty? || !stdout.include?("value 1")
    puts stdout
    err_count = err_count + 1
end

# check if port is available metric
stdout, stderr, status = Open3.capture3("pminfo statsd.pmda.settings.port -f");
if !stderr.empty? || !stdout.include?("value 8123")
    err_count = err_count + 1
end

config = IniFile.load(config_file)
config.delete_section "global"
config["global"]["port"] = -10
config.write

restart_agent

# check that fallback is used when incorrect value is supplied
stdout, stderr, status = Open3.capture3("pminfo statsd.pmda.settings.port -f");
if !stderr.empty? || !stdout.include?("value 8125")
    err_count = err_count + 1
end

print "\r"

$stdout.flush
if err_count == 0
  puts "✔".green + " Option port is ok                  "
else 
  puts "✖".red + " Option port fails: " + err_count.to_s + "   "
end

