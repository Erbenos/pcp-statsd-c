require 'socket'
require 'open3'
require 'inifile'
require 'fileutils'
require 'colorize'

print "⧖ Checking debug_output_filename option..."

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

# set custom debug output filename
config_file = File.join(statsd_pmda_dir, statsd_pmda_config)
config = IniFile.load(config_file)
config.delete_section "global"
config["global"]["debug_output_filename"] = "test_output"
config.write

restart_agent

ds.send("test:1|c", 0)
sleep(0.1)

# send USR1 signal
stdout, stderr, status = Open3.capture3("pgrep pmdastatsd")
pid = stdout.chomp
stdout, stderr, status = Open3.capture3("sudo kill -USR1 " + pid)
sleep(1)

# check if file exists
unless File.file?(File.join(statsd_pmda_dir, "test_output"))
  err_count = err_count + 1
else
  File.delete(File.join(statsd_pmda_dir, "test_output"))
end

print "\r"

$stdout.flush
if err_count == 0
  puts "✔".green + " Option debug_output_filename is ok                  "
else 
  puts "✖".red + " Option debug_output_filename fails: " + err_count.to_s + "   "
end

config_file = File.join(statsd_pmda_dir, statsd_pmda_config)
config = IniFile.load(config_file)
config.delete_section "global"
config.write
