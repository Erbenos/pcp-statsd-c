require 'socket'
require 'open3'
require 'inifile'
require 'fileutils'
require 'colorize'

print "⧖ Checking debug option..."

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

# check 1
config_file = File.join(statsd_pmda_dir, statsd_pmda_config)
config = IniFile.load(config_file)
config.delete_section "global"
config["global"]["debug"] = 1
config.write

restart_agent

sleep(0.1)
stdout, stderr, status = Open3.capture3("pminfo statsd.pmda.settings.debug -f")
if !stderr.empty? || !stdout.include?("value 1")
  err_count = err_count + 1
end

# check 0
config_file = File.join(statsd_pmda_dir, statsd_pmda_config)
config = IniFile.load(config_file)
config.delete_section "global"
config["global"]["debug"] = 0
config.write

restart_agent

sleep(0.1)
stdout, stderr, status = Open3.capture3("pminfo statsd.pmda.settings.debug -f")
if !stderr.empty? || !stdout.include?("value 0")
  err_count = err_count + 1
end

# check fallback when incorrect value is supplied
config_file = File.join(statsd_pmda_dir, statsd_pmda_config)
config = IniFile.load(config_file)
config.delete_section "global"
config["global"]["debug"] = -1
config.write

restart_agent

sleep(0.1)
stdout, stderr, status = Open3.capture3("pminfo statsd.pmda.settings.debug -f")
if !stderr.empty? || !stdout.include?("value 0")
  err_count = err_count + 1
end

print "\r"

$stdout.flush
if err_count == 0
  puts "✔".green + " Option debug is ok                  "
else 
  puts "✖".red + " Option debug fails: " + err_count.to_s + "   "
end

config_file = File.join(statsd_pmda_dir, statsd_pmda_config)
config = IniFile.load(config_file)
config.delete_section "global"
config.write
