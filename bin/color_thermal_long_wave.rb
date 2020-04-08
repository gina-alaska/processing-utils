#!/usr/bin/env ruby
require "trollop"
require "yaml"
require "pp"
#############
# Simple  tool to colorize long wave thermal floating point data like we do for awips. 
# ./color_thermal_long_wave.rb -h is your friend/fiend


#wrapper for system - runs command on task
def runner ( command, opts)
  puts("Info: Running: #{command.join(" ")}") if (opts[:verbrose])
  start_time = Time.now
  system(*command) if (!opts[:dry_run])
  puts("Info: Done in #{(Time.now - start_time)/60.0}m.") if (opts[:verbrose])
end


## Command line parsing action..
parser = Trollop::Parser.new do
  version "0.0.1 jay@alaska.edu"
  banner <<-EOS
  This util chops an image into a number of tiles.

Usage:
      tile_image.rb [options] --command_to_run <command> <file1>  ....
where [options] is:
EOS

  opt :red, "The I05 band", :type => String
  opt :blue, "Unused, just to fit withing the geotif handling in GINA's NRT stack", :type => String
  opt :green, "Unused, just to fit withing the geotif handling in GINA's NRT stack", :type => String
  opt :colormap, "Colorman to use", :type => String, :default => "#{File.dirname(__FILE__)}/../config/short_wave.k.cmap"
  opt :verbrose, "Maxium Verbrosity.", :short => "V"
end

opts = Trollop::with_standard_exception_handling(parser) do
  o = parser.parse ARGV
  raise Trollop::HelpNeeded if ARGV.length == 0 # show help screen
  o
end

output_file =ARGV.first
gdal_options = ["-co", "COMPRESS=LZW","-co",  "TILED=YES"]
runner(["gdaldem","color-relief",*gdal_options, opts[:red], opts[:colormap], output_file], opts)
