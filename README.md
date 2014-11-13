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

* add_mask

** Add_overviews.rb 

*** Adds the "proper" number of overviews to a gdal file, in power of 2 divions, so a 120 by 120 image would get overviews at 2,4,8,16, and 32.  It should work on anything that gdal can add overviews to. 
*** Example:
*** t```
$ add_overviews.rb test.tif
Runner running "gdaladdo -r average --config GDAL_CACHEMAX 500 test.tif  2  4  8  16  32 "
0...10...20...30...40...50...60...70...80...90...100 - done.
This run took 0 s
$ gdalinfo test.tif | grep Overview
  Overviews: 60x60, 30x30, 15x15, 8x8, 4x4

```


dd_mask
add_overviews.rb
get_gcp
image_info
masker
modis_natural_color_stretch
no_data_check
npp_natural_color_stretch
tile_image.rb


