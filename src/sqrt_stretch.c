#include "gdal.h"
#include "cpl_string.h"
#include "cpl_conv.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <math.h>



/************************************************************/
/* update */
/************************************************************/


/* print some usage information and quit..*/
void
ussage ()
{
  printf
    ("This tool is a simple util for stretching visible data for awips.  Works in a strange manner, and expects a floating point tif as input with data in the range of 0 to 100.0 and generates 8 bit output by applying .\n");
  printf ("Use it like:\n");
  printf ("\tsqrt_stretch <infile> <outfile>\n");
  printf ("\t\twhere:\n");
  printf
    ("\t\t\t<infile> is a single banded floating point tif.  Can be single or double. \n");
  printf
    ("\t\t\t<outfile> is the output file.  It will be deflate compressed, and tiled, with 0 as nodata.\n");
  exit (-1);
}



/* command line parsing..*/
void
parse_opts (int argc, char **argv, char *infile, char *outfile)
{
  int c;

  /* these are defaults.. */

  while (1)
    {
      static struct option long_options[] = {
	/* These options set a flag. */
	{"help", no_argument, 0, 'h'},
	{"h", no_argument, 0, 'h'},
	{0, 0, 0, 0}
      };
      /* getopt_long stores the option index here. */
      int option_index = 0;

      c = getopt_long (argc, argv, "abc:d:f:", long_options, &option_index);

      /* Detect the end of the options. */
      if (c == -1)
	break;

      switch (c)
	{
	case 'h':
	  ussage ();
	  break;
	case '?':
	  /* getopt_long already printed an error message. */
	  break;

	default:
	  abort ();
	}
    }

  /* Print any remaining command line arguments (not options). */
  if (optind + 2 == argc)
    {
      strcpy (infile, argv[optind]);
      optind++;
      strcpy (outfile, argv[optind]);
    }
  else
    {
      ussage ();
    }
}




/* Makes a copy of a dataset, and opens it for writing.. */
GDALDatasetH
make_me_a_sandwitch (GDALDatasetH * in_dataset, char *copy_file_name)
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
  papszOptions = CSLSetNameValue (papszOptions, "BIGTIFF", "YES");

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
GDAL_open_read (char *file_name)
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
unsigned char
scale (double value, double min, double max, double multi)
{

  if (value < min)
    {
      return 0;
    };
  if (value > max)
    {
      return 255;
    };

  return (unsigned char) round (sqrt (value) * multi);

  //printf("(%g,%g,%g) => %g -> %g =>  (%g - (2.0*%g)  %g\n",min,middle,max, value, result, max, value, value*2.0);
}




int
main (int argc, char *argv[])
{
  GDALDriverH hDriver;
  double adfGeoTransform[6];
  GDALDatasetH in_Dataset;
  GDALDatasetH out_Dataset;
  double *data_scan_line;
  unsigned char *out_scan_line;
  int nBlockXSize, nBlockYSize;
  int bGotMin, bGotMax;
  int bands;
  int xsize;
  double adfMinMax[2];
  char infile[2024], outfile[2024];	/*bad form.. */


  /* parse command line */
  parse_opts (argc, argv, infile, outfile);


  GDALAllRegister ();

  /* Set cache to something reasonable.. - 1/2 gig */
  CPLSetConfigOption ("GDAL_CACHEMAX", "2048");

  /* open datasets.. */
  in_Dataset = GDAL_open_read (infile);
  out_Dataset = make_me_a_sandwitch (&in_Dataset, outfile);

  /* Basic info on source dataset.. */
  GDALGetBlockSize (GDALGetRasterBand (in_Dataset, 1), &nBlockXSize,
		    &nBlockYSize);
  printf ("Info: Block=%dx%d Type=%s, ColorInterp=%s\n", nBlockXSize,
	  nBlockYSize,
	  GDALGetDataTypeName (GDALGetRasterDataType
			       (GDALGetRasterBand (in_Dataset, 1))),
	  GDALGetColorInterpretationName (GDALGetRasterColorInterpretation
					  (GDALGetRasterBand
					   (in_Dataset, 1))));

  /* Loop though bands, scaling the data.. */
  xsize = GDALGetRasterXSize (in_Dataset);
  data_scan_line = (double *) CPLMalloc (sizeof (double) * xsize);
  out_scan_line = (unsigned char *) CPLMalloc (sizeof (double) * xsize);

  for (bands = 1; bands <= GDALGetRasterCount (in_Dataset); bands++)
    {
      int x, y_index;
      GDALRasterBandH data_band, out_band;
      data_band = GDALGetRasterBand (in_Dataset, bands);
      out_band = GDALGetRasterBand (out_Dataset, bands);

      /* Set nodata for that band */
      GDALSetRasterNoDataValue (out_band, 0.0);
      for (y_index = 0; y_index < GDALGetRasterYSize (in_Dataset); y_index++)
	{
	  /* Read data.. */
	  GDALRasterIO (data_band, GF_Read, 0, y_index, xsize, 1,
			data_scan_line, xsize, 1, GDT_Float64, 0, 0);
	  for (x = 0; x < xsize; x++)
	    {
	      out_scan_line[x] = scale (data_scan_line[x], 0.0, 100.0, 25.5);
	    }
	  GDALRasterIO (out_band, GF_Write, 0, y_index, xsize, 1,
			out_scan_line, xsize, 1, GDT_Byte, 0, 0);
	}


    }

  /* close file, and we are done. */
  GDALClose (out_Dataset);
  
  /* exit gracefully with a 0 */
  return 0;
}
