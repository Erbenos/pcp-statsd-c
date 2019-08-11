require 'socket'
require 'open3'
require 'inifile'
require 'fileutils'
require 'colorize'

print "⧖ Checking parser_type option..."

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

# set "Basic" duration aggregation type
config_file = File.join(statsd_pmda_dir, statsd_pmda_config)
config = IniFile.load(config_file)
config.delete_section "global"
config["global"]["parser_type"] = 0
config.write

restart_agent

# check if "Basic" duration aggregation type is set
stdout, stderr, status = Open3.capture3("pminfo statsd.pmda.settings.parser_type -f")
if !stderr.empty? || !stdout.include?("value \"Basic\"")
  err_count = err_count + 1
end

# set "Ragel" duration aggregation type
config_file = File.join(statsd_pmda_dir, statsd_pmda_config)
config = IniFile.load(config_file)
config.delete_section "global"
config["global"]["parser_type"] = 1
config.write

restart_agent

# check if "HDR histogram" duration aggregation type is set
stdout, stderr, status = Open3.capture3("pminfo statsd.pmda.settings.parser_type -f")
if !stderr.empty? || !stdout.include?("value \"Ragel\"")
  puts stdout
  err_count = err_count + 1
end

print "\r"

$stdout.flush
if err_count == 0
  puts "✔".green + " Option parser_type is ok                  "
else 
  puts "✖".red + " Option parser_type fails: " + err_count.to_s + "   "
end

config_file = File.join(statsd_pmda_dir, statsd_pmda_config)
config = IniFile.load(config_file)
config.delete_section "global"
config.write
