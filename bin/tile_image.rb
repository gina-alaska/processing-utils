#!/usr/bin/env ruby
require "trollop"
require "yaml"
require "pp"
#############
# Simple command to run several things at once
# ./do_several_at_once.rb -h is your friend/fiend


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

  opt :output_dir, "The output dir.", :type => String
  opt :tile_size, "X and Y size to chop to (default 10000).", :type => Integer, :default => 10000
  opt :verbrose, "Maxium Verbrosity.", :short => "V"
  opt :dry_run, "Don't actually run the command(s)"
  opt :fiddle, "How much overlap the tiles should have.", :type=>Integer, :default => 24
  opt :big_tiff, "Make bigtiffs"
end

opts = Trollop::with_standard_exception_handling(parser) do
  o = parser.parse ARGV
  raise Trollop::HelpNeeded if ARGV.length == 0 # show help screen
  
  if(o[:tile_size] && o[:tile_size]<=0 )
    puts("Error: size should be greater than 0\n\n\n\n\n\n")
   raise Trollop::HelpNeeded
  end
  o
end

source_file =ARGV.first

fiddle = opts[:fiddle].to_i

#drop extention of output file..
out_file_s = source_file.split(".")
out_file_s.delete_at(-1)
out_file = out_file_s.join(".")

if ( opts[:output_dir])
    out_file = opts[:output_dir] + "/" + File.basename(out_file)
end

size = 10000
fiddle = 24
size = opts[:tile_size].to_i if (opts[:tile_size])

conf = YAML.load(`gdal_list_corners #{source_file}`)

xi = 0
x = 0
while (x < conf["width"])
  y = 0
  yi = 0
  while (y < conf["height"])
    x_end = x + size
    x_end = conf["width"] if (conf["width"] < x_end)
    
    y_end = y + size
    y_end = conf["height"] if (conf["height"] < y_end)
    
    command = ["gdal_translate", "-co", "TILED=YES", "-co", "COMPRESS=LZW"]
    command += ["-co", "BIGTIFF=YES"] if ( opts[:big_tiff])
    command +=[ "-srcwin", x.to_s, y.to_s, (x_end -x).to_s,( y_end-y).to_s, source_file, out_file + ".tile." + xi.to_s+"."+yi.to_s+".tif"]
    runner(command, opts)
    runner([File.dirname(__FILE__) +"/add_overviews.rb", out_file + ".tile." + xi.to_s+"."+yi.to_s+".tif"], opts)
    y += size - fiddle
    yi += 1
  end
  x += size - fiddle
  xi += 1
end
