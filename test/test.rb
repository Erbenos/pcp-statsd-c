#!/usr/bin/env ruby
require 'socket'
require 'open3'
require 'inifile'
require 'fileutils'

stdout, stderr, status = Open3.capture3(". /etc/pcp.conf && cd $PCP_PMDAS_DIR/statsd && echo $PWD")

statsd_pmda_dir = stdout.chomp
statsd_pmda_config = "pmdastatsd.ini"

original_config = File.join(statsd_pmda_dir, statsd_pmda_config)
backup = FileUtils.cp(original_config, "." + statsd_pmda_config)

config = IniFile.load(original_config)
config['global']['max_udp_packet_size'] = 1472
config['global']['port'] = 8125
config['global']['max_unprocessed_packets'] = 1024
config['global']['tcp_address'] = '0.0.0.0'
config['global']['verbose'] = 1
config['global']['debug'] = 0
config['global']['debug_output_filename'] = 'debug'
config['global']['duration_aggregation_type'] = 0
config['global']['parser_type'] = 0
config.write

restart_files = Dir.glob(["restart/**/*.rb"], 0)
restart = restart_files.sort

base_files = Dir.glob(["base/**/*.rb"], 0)
base = base_files.sort

metric_files = Dir.glob(["metrics/**/*.rb"], 0);
metrics = metric_files.sort

# Config files provide restart functionality by themselves
config_files = Dir.glob(["config/**/*.rb"], 0)
config = config_files.sort

barebones = restart | base
metric_tests = restart | metrics

puts "▸ Testing agent barebones"

barebones.each{|s| 
  load s
}

puts "▸ Testing settings:"

config_files.each{|s|
  load s
}

config = IniFile.load(original_config)
config['global']['duration_aggregation_type'] = 0
config['global']['parser_type'] = 0
config.write
puts "▸ Testing parser_type: BASIC, duration_aggregation: BASIC"

metric_tests.each{|s|
  load s
}

config = IniFile.load(original_config)
config['global']['duration_aggregation_type'] = 0
config['global']['parser_type'] = 1
config.write
puts "▸ Testing parser_type: RAGEL, duration_aggregation: BASIC"

metric_tests.each{|s|
  load s
}

config = IniFile.load(original_config)
config['global']['duration_aggregation_type'] = 1
config['global']['parser_type'] = 0
config.write
puts "▸ Testing parser_type: BASIC, duration_aggregation: HDR_HISTOGRAM"

metric_tests.each{|s|
  load s
}

config = IniFile.load(original_config)
config['global']['duration_aggregation_type'] = 1
config['global']['parser_type'] = 1
config.write
puts "▸ Testing parser_type: RAGEL, duration_aggregation: HDR_HISTOGRAM"

metric_tests.each{|s|
  load s
}

# replace config with backup
backup = FileUtils.cp("." + statsd_pmda_config, original_config)
File.delete("." + statsd_pmda_config) if File.exist?("." + statsd_pmda_config)
