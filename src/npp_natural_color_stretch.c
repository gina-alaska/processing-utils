#include "gdal.h"
#include "cpl_string.h"
#include "cpl_conv.h"

/************************************************************/
/* A simple util for making natural color npp images*/
/* Use like:            */
/* ./npp_natural_color_stretch <infile>  <outfile> */
/* <infile> should be a three banded tiff with atm correction applied ..*/
/* questions or comments? jc@alaska.edu     */
/* compile like: gcc $(gdal-config --cflags) -o modis_natural_color_stretch modis_natural_color_stretch.c $(gdal-config --libs) */
/************************************************************/



#define MAX_MODIS (0.99995)
#define MIN_VALUE (0.00001)
//(65500)


/* Makes a copy of a dataset, and opens it for writing.. */
GDALDatasetH
make_me_a_sandwitch (GDALDatasetH * in_dataset, const char *copy_file_name)
{
  char **papszOptions = NULL;
  const char *pszFormat = "GTiff";
  double adfGeoTransform[6];
  GDALDriverH hDriver;
  GDALDatasetH out_gdalfile;
  hDriver = GDALGetDriverByName (pszFormat);
  papszOptions = CSLSetNameValue (papszOptions, "TILED", "YES");
  papszOptions = CSLSetNameValue (papszOptions, "COMPRESS", "DEFLATE");

  /*Perhaps controversal - default to bigtiff... */
  /*papszOptions = CSLSetNameValue( papszOptions, "BIGTIFF", "YES" ); */

  /*return GDALCreateCopy( hDriver, copy_file_name, *in_dataset, FALSE, papszOptions, NULL, NULL ); */
  out_gdalfile = GDALCreate (hDriver, copy_file_name,
			     GDALGetRasterXSize (*in_dataset),
			     GDALGetRasterYSize (*in_dataset),
			     GDALGetRasterCount (*in_dataset),
			     GDT_Byte, papszOptions);

  /* Set geotransform */
  GDALGetGeoTransform (*in_dataset, adfGeoTransform);
  GDALSetGeoTransform (out_gdalfile, adfGeoTransform);

  /* Set projection */
  GDALSetProjection (out_gdalfile, GDALGetProjectionRef (*in_dataset));
  return out_gdalfile;
}


/* Opens a file */
GDALDatasetH
GDAL_open_read (const char *file_name)
{
  GDALDatasetH gdalDataset;
  gdalDataset = GDALOpen (file_name, GA_ReadOnly);
  if (gdalDataset == NULL)
    {
      printf
	("Hmm, could not open '%s' for reading.. this be an error, exiting..\n",
	 file_name);
      exit (-1);
    }
  return gdalDataset;
}



/* performs the stretch - first scales to 1.0 to 255, then does a piecewise color enhancement.*/
char
scale (double *x, double *y, double value, double min, double max)
{
  int i = 0;

  double b, m;

  if (value > MAX_MODIS)
    return 0;			/*nodata value */
  if (value < MIN_VALUE)
    return 0;			/*nodata value */

  value = 1.0 + (254.0 + 0.9999) * (value - min) / (max - min);

  /*Find correct "bin" */
  while (i < 254 && !(value >= x[i] && value <= x[i + 1]))
    {
      i++;
    };

  /* perform the mapping */
  m = (y[i + 1] - y[i]) / (x[i + 1] - x[i]);
  b = y[i + 1] - m * x[i + 1];
  value = (m * value + b);

  /* double check values make sence. */
  if (value > 255)
    return 255;
  return (char) value;


/* Original IDL code this is modeled after : 
  x1 = x[index]
  x2 = x[index + 1]
  y1 = y[index]
  y2 = y[index + 1]
  m = (y2 - y1) / float((x2 - x1))
  b = y2 - (m * x2)
  mask = (image ge x1) and (image lt x2)
  scaled = scaled + mask * byte(m * image + b)
*/


}



/* print some usage information and quit..*/
void
ussage ()
{
  printf
    ("This tool is a simple util for making natural color modis images from atm corrected bands of modis.\n");
  printf ("Use it like:\n");
  printf ("\tnpp_natural_color_stretch <infile> <outfile>\n");
  printf ("\t\twhere:\n");
  printf ("\t\t\t<infile> is a three banded tiff with bands 1, 4, 3 \n");
  printf
    ("\t\t\t<outfile> is the output file.  It will be deflate compressed, and tiled, with 0 as nodata.\n");
  exit (-1);
}



int
main (int argc, const char *argv[])
{
  GDALDriverH hDriver;
  double adfGeoTransform[6];
  GDALDatasetH in_Dataset;
  GDALDatasetH out_Dataset;
  double *data_scan_line;
  char *out_scan_line;
  int nBlockXSize, nBlockYSize;
  int bGotMin, bGotMax;
  int bands;
  int xsize;
  double lower_fiddle, upper_fiddle;
  double adfMinMax[2];
  /* These are hard coded for now - need to figure out a better way to handle this.. */
  double palette_in[256] =
    { 0.0, 1.00011, 1.9999649999999998, 3.000075, 3.9999299999999995, 5.00004,
5.999895, 7.000005, 8.000115, 8.99997, 10.00008, 10.999935, 12.000044999999998, 12.9999,
14.00001, 15.00012, 15.999975, 17.000085000000002, 17.99994, 19.00005, 19.999905000000002,
21.000014999999998, 22.000125, 22.99998, 24.000089999999997, 24.999945, 26.000055, 26.99991,
28.00002, 28.999875000000003, 29.999985, 31.000094999999998, 31.99995, 33.00006, 33.999915,
35.000024999999994, 35.99988, 36.999990000000004, 38.0001, 38.999955, 40.000065, 40.99992,
42.000029999999995, 42.999885, 43.999995000000006, 45.000105, 45.99996, 47.00007,
47.999925000000005, 49.000035, 49.99989, 51.0, 52.00011, 52.999965, 54.000075, 54.99993,
56.00004, 56.999895, 58.000004999999994, 59.000115, 59.99997, 61.000080000000004, 61.999935,
63.000045, 63.9999, 65.00001, 66.00012, 66.999975, 68.000085, 68.99994, 70.00004999999999,
70.999905, 72.000015, 73.000125, 73.99998000000001, 75.00009, 75.999945, 77.00005499999999,
77.99991, 79.00002, 79.99987499999999, 80.99998500000001, 82.000095, 82.99995,
84.00005999999999, 84.999915, 86.00002500000001, 86.99987999999999, 87.99999000000001, 89.0001,
89.999955, 91.00006499999999, 91.99992, 93.00003, 93.99988499999999, 94.999995, 96.000105,
96.99996, 98.00007, 98.999925, 100.000035, 100.99989, 102.0, 103.00011, 103.999965, 105.000075,
105.99993, 107.00004, 107.999895, 109.000005, 110.00011500000001, 110.99997, 112.00008,
112.99993500000001, 114.000045, 114.9999, 116.00000999999999, 117.00012, 117.999975,
119.000085, 119.99994, 121.00005, 121.999905, 123.00001499999999, 124.000125,
124.99998000000001, 126.00009, 126.999945, 128.000055, 128.99991, 130.00002, 130.999875,
131.99998499999998, 133.000095, 133.99995, 135.00006, 135.999915, 137.00002500000002,
137.99988, 138.99999, 140.00009999999997, 140.999955, 142.000065, 142.99991999999997,
144.00003, 144.999885, 145.99999499999998, 147.000105, 147.99996000000002, 149.00007,
149.999925, 151.00003500000003, 151.99989, 153.0, 154.00010999999998, 154.999965, 156.000075,
156.99992999999998, 158.00004, 158.999895, 160.000005, 161.000115, 161.99997000000002,
163.00008, 163.999935, 165.000045, 165.9999, 167.00001, 168.00011999999998, 168.999975,
170.000085, 170.99993999999998, 172.00005000000002, 172.999905, 174.000015, 175.000125,
175.99998000000002, 177.00009, 177.999945, 179.00005499999997, 179.99991, 181.00002,
181.999875, 182.999985, 184.00009500000002, 184.99994999999998, 186.00006, 186.99991500000002,
188.000025, 188.99988, 189.99999, 191.0001, 191.999955, 193.00006499999998, 193.99992,
195.00003, 195.99988499999998, 196.999995, 198.00010500000002, 198.99996, 200.00007,
200.99992500000002, 202.000035, 202.99989, 204.0, 205.00011, 205.999965, 207.00007499999998,
207.99993, 209.00004, 209.99989499999998, 211.00000500000002, 212.000115, 212.99997, 214.00008,
214.999935, 216.000045, 216.9999, 218.00001, 219.00012, 219.999975, 221.00008499999998,
221.99994, 223.00005000000002, 223.99990499999998, 225.00001500000002, 226.000125, 226.99998,
228.00009, 228.999945, 230.000055, 230.99991, 232.00001999999998, 232.999875, 233.999985,
235.000095, 235.99995, 237.00006, 237.999915, 239.000025, 239.99988, 240.99999, 242.0001,
242.999955, 244.000065, 244.99992, 246.00002999999998, 246.999885, 247.999995, 249.000105,
249.99996000000002, 251.00007, 251.999925, 253.000035, 253.99989, 255.0 };
  double palette_out[256] =
    { 0.0, 0.042075, 0.08415, 0.126225, 0.169065, 0.21216, 0.255765, 0.29988,
0.345015, 0.390915, 0.437835, 0.48602999999999996, 0.5352450000000001, 0.58599, 0.63801,
0.69156, 0.7468950000000001, 0.804015, 0.8629199999999999, 0.9238649999999999, 0.98685, 1.05213,
1.119705, 1.18983, 1.26225, 1.337475, 1.415505, 1.49634, 1.580235, 1.6674449999999998, 1.75746,
1.851045, 1.9482, 2.04867, 2.152965, 2.2608300000000003, 2.3727750000000003, 2.4885450000000002,
2.6083950000000002, 2.73258, 2.8611000000000004, 2.993955, 3.1313999999999997, 3.27369, 3.42057,
3.572295, 3.72912, 3.891045, 4.05807, 4.23045, 4.408440000000001, 4.591785, 4.780995, 4.975815,
5.1765, 5.38305, 5.595975, 5.8150200000000005, 6.040185, 6.27198, 6.510405, 6.755205, 7.007145,
7.265715, 7.53117, 7.8040199999999995, 8.084010000000001, 8.37114, 8.66592, 8.968095, 9.27792,
9.59565, 9.92103, 10.25457, 10.596015, 10.945875, 11.30415, 11.670585, 12.04569, 12.429465,
12.82191, 13.223279999999999, 13.63383, 14.053305, 14.48196, 14.919794999999999, 15.36732,
15.82428, 16.29093, 16.767270000000003, 17.253555, 17.749785, 18.256215, 18.925845,
19.456245000000003, 19.99863, 20.552745, 21.11859, 21.696165, 22.285725, 22.887014999999998,
23.500035, 24.125295, 24.762285, 25.411260000000002, 26.071965, 26.74491, 27.42984, 28.12701,
28.835910000000002, 29.55705, 30.29043, 31.035795, 31.7934, 32.56299, 33.345075, 34.139145,
34.94571, 35.764514999999996, 36.595305, 37.438845, 38.294625, 39.162645, 40.04316, 40.935915,
41.84142, 42.759165, 43.68966, 44.632394999999995, 45.58788, 46.55586, 47.536335,
48.529560000000004, 49.535535, 50.554005000000004, 51.58497, 52.62894, 53.68566, 54.75513,
55.837095, 56.932064999999994, 58.040040000000005, 59.160765, 60.294239999999995, 61.44072,
62.59995000000001, 63.772439999999996, 64.95768000000001, 66.15592500000001, 67.36743,
68.591685, 69.82919999999999, 71.07972, 72.343245, 73.620285, 74.910075, 76.21337999999999,
77.52968999999999, 78.85926, 80.20209, 81.55843499999999, 82.927785, 84.31065, 85.66419,
87.10264500000001, 88.611225, 90.18712500000001, 91.82779500000001, 93.52992, 95.291205,
97.108845, 98.979525, 100.90095, 102.87006, 104.884305, 106.94037, 109.03596, 111.16827,
113.33424000000001, 115.531065, 117.756195, 120.00657, 122.27964, 124.572345,
126.88213499999999, 129.20595, 131.54124, 133.8852, 136.23477, 138.58714500000002, 140.939775,
143.289855, 145.63458, 147.97089, 150.296235, 152.60781, 154.902555, 157.17817499999998,
159.431355, 161.65929, 163.859685, 166.02948, 168.165615, 170.26554, 172.32645, 174.34554,
176.320005, 178.24704, 180.292905, 182.130945, 183.953685, 185.76138, 187.554285, 189.33291,
191.09751, 192.84834, 194.58591, 196.310475, 198.02229, 199.72161, 201.4092, 203.084805,
204.74944499999998, 206.402865, 208.04557499999999, 209.67808499999998, 211.30065, 212.913525,
214.516965, 216.11148, 217.69758000000002, 219.27501, 220.84479, 222.406665, 223.9614,
225.50924999999998, 227.05047000000002, 228.585315, 230.114295, 231.63766500000003,
233.15568000000002, 234.66885000000002, 236.17743, 237.681675, 239.18184, 240.67869, 242.172225,
243.66295499999998, 245.15088, 246.636765, 248.12061000000003, 249.602925, 251.083965,
252.56424, 254.04375, 255.0 };


  GDALAllRegister ();


  /* ussage.. */
  if (argc != 3)
    ussage ();

  /* Set cache to something reasonable.. - 1/2 gig */
  CPLSetConfigOption ("GDAL_CACHEMAX", "512");

  /* open datasets.. */
  in_Dataset = GDAL_open_read (argv[1]);
  out_Dataset = make_me_a_sandwitch (&in_Dataset, argv[2]);

  /* Basic info on source dataset.. */
  GDALGetBlockSize (GDALGetRasterBand (in_Dataset, 1), &nBlockXSize,
		    &nBlockYSize);
  printf ("Block=%dx%d Type=%s, ColorInterp=%s\n", nBlockXSize, nBlockYSize,
	  GDALGetDataTypeName (GDALGetRasterDataType
			       (GDALGetRasterBand (in_Dataset, 1))),
	  GDALGetColorInterpretationName (GDALGetRasterColorInterpretation
					  (GDALGetRasterBand
					   (in_Dataset, 1))));

  /* Loop though bands, scaling the data.. */
  xsize = GDALGetRasterXSize (in_Dataset);
  data_scan_line = (double *) CPLMalloc (sizeof (double) * xsize);
  out_scan_line = (char *) CPLMalloc (sizeof (char) * xsize);

  for (bands = 1; bands <= GDALGetRasterCount (in_Dataset); bands++)
    {
      int x;
      double min = 9999999.0, max = 0.0;	/* probibly a better way to set these.. */
      double dmin, dmax, middle;
      GDALRasterBandH data_band, out_band;
      int y_index = 0;
      data_band = GDALGetRasterBand (in_Dataset, bands);
      out_band = GDALGetRasterBand (out_Dataset, bands);

      /* Set nodata for that band */
      GDALSetRasterNoDataValue (out_band, 0.0);

      /*Find Min,Max, required for scaling */
      for (y_index = 0; y_index < GDALGetRasterYSize (in_Dataset); y_index++)
	{
	  /* Read data.. */
	  GDALRasterIO (data_band, GF_Read, 0, y_index, xsize, 1,
			data_scan_line, xsize, 1, GDT_Float64, 0, 0);
	  for (x = 0; x < xsize; x++)
	    {
	      if (data_scan_line[x] < MAX_MODIS
		  && data_scan_line[x] > MIN_VALUE)
		{
		  if (data_scan_line[x] > max)
		    max = data_scan_line[x];
		  else if (data_scan_line[x] < min)
		    min = data_scan_line[x];
		}
	    }
	}

      dmax = (double) max;
      dmin = (double) min;

      printf ("Info: For Band %d -> Min=%g,Max=%g\n", bands, dmin, dmax);

      for (y_index = 0; y_index < GDALGetRasterYSize (in_Dataset); y_index++)
	{
	  double scaled;

	  /* Read data.. */
	  GDALRasterIO (data_band, GF_Read, 0, y_index, xsize, 1,
			data_scan_line, xsize, 1, GDT_Float64, 0, 0);

	  /* scale each .. */
	  for (x = 0; x < xsize; x++)
	    {
	      out_scan_line[x] =
		scale (palette_in, palette_out, data_scan_line[x], dmin,
		       dmax);
	    }

	  /* now write out band.. */
	  GDALRasterIO (out_band, GF_Write, 0, y_index, xsize, 1,
			out_scan_line, xsize, 1, GDT_Byte, 0, 0);
	}

    }


  /* close file, and we are done. */
  GDALClose (out_Dataset);

}
