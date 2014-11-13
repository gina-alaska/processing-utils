processing-utils
================

a collection of processing tools, mainly for processing raster data

basic structure (ruby gem style):
* processing-utils
  * bin
    * add_overviews.rb 
      * adds overviews
  * lib
      * shared stuff..
  * config  _Each tool in bin has a config file, optionally_
    * none at the moment

###Notes
===========

__add_mask__

__add_overviews.rb__

Adds the "proper" number of overviews to a gdal file, in power of 2 divions, so a 120 by 120 image would get overviews at 2,4,8,16, and 32.  It should work on anything that gdal can add overviews to. 
Example:
```
$ add_overviews.rb test.tif
Runner running "gdaladdo -r average --config GDAL_CACHEMAX 500 test.tif  2  4  8  16  32 "
0...10...20...30...40...50...60...70...80...90...100 - done.
This run took 0 s
$ gdalinfo test.tif | grep Overview
  Overviews: 60x60, 30x30, 15x15, 8x8, 4x4

```

__get_gcp__

Dumps out, in YAML, any GCPs a file might have. Useful for clipping rasters when the bounds of the raster are stored as GCP.

__image_info__

	Dumps out, in YAML, details about an image, including the amount of nodata, what the nodata value is, number of valid pixesl, projection, etc.
Example:
```
$ image_info test/npp.14281.0101_DNB.tif
infile: test/npp.14281.0101_DNB.tif
width: 6000
height: 6000
projection: 'PROJCS["alaska_albers_small",GEOGCS["NAD83",DATUM["North_American_Datum_1983",SPHEROID["GRS 1980",6378137,298.2572221010002,AUTHORITY["EPSG","7019"]],AUTHORITY["EPSG","6269"]],PRIMEM["Greenwich",0],UNIT["degree",0.0174532925199433],AUTHORITY[0],UNIT["metre",1,AUTHORITY["EPSGs_Conic_Equal_Area"],PARAMETER["standard_parallel_1",55],PARAMETER["standard_parallel_2",65],PARAMETER["latitude_of_center",50],PARAMETER["longitude_of_center",-154],PARAMETER["false_easting",0],PARAMETER["false_northing",0],UNIT["metre",1,AUTHORITY["EPSG","9001"]]]' 
geo_transform: 
- -1.50268e+06
- 600
- 0
- 3.46768e+06
- 0
- -600
bands:
- band_no: 1
  overviews: 11
  valid_pixels: 5649445
  nodata_pixels: 30350555
  nodata_value: 0
  min: 1
  max: 255

```
__masker__

Masked off a file with a mask. 
Use it like: masker <infile> <maskfile> <outfile> 
Needis command line parsing that doesn't suck. 

__modis_natural_color_stretch__

Provides a useful modis natural color stretch. 

__no_data_check__

Generates a black and white image with nodata values highlighted - used to visually check that you don't have nodata values where you didn't expect them, and vice-versa. 

__npp_natural_color_stretch__

modis_natural_color_stretch for pytrol processed npp data.

__tile_image.rb__

Chops a large image up into tiles.
```
tile_image.rb 

  This util chops an image into a number of tiles.

Usage:
      tile_image.rb [options] --command_to_run <command> <file1>  ....
where [options] is:
  --output-dir, -o <s>:   The output dir.
   --tile-size, -t <i>:   X and Y size to chop to (default 10000). (Default:
                          10000)
        --verbrose, -V:   Maxium Verbrosity.
         --dry-run, -d:   Don't actually run the command(s)
      --fiddle, -f <i>:   How much overlap the tiles should have. (Default: 24)
        --big-tiff, -b:   Make bigtiffs
         --version, -v:   Print version and exit
            --help, -h:   Show this message
```


