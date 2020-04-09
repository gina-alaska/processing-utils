#include "gdal.h"
#include "cpl_string.h"
#include "cpl_conv.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <math.h>


/************************************************************/
/* Makes snow melt rgb from viirs data */
/************************************************************/


/* print some usage information and quit..*/
void
usage ()
{
  printf ("This tool is a simple util for making viirs snow melt rgbs. \n");
  printf ("Use it like:\n");
  printf
    ("\tsnow melt (--red 1.6_um_file i03) (--green 1.24um m08 file ) (--blue 0.64um  i01 file ) <outfile> \n");
  printf ("\t\twhere:\n");
  printf
    ("\t\t\t<outfile> is the output file.  It will be deflate compressed, and tiled, with 0 as nodata.\n");
  printf
    ("\t\t\t(blue-cap) is how much to stretch the blue - try 1.0 or 0.75 .\n");

  exit (-1);
}



/*******************************************************************************************

 red=1.61um (i03) green=1.24um (m8) blue=0.64um (i01)  equal percentages... gamma=1.
4:03
for all max=100 min=0

*********************************************************************************************/



/* command line parsing..*/
void
parse_opts (int argc, char **argv, char *m8, char *i3, char *i1,
	    char *outfile)
{
  int c;

  while (1)
    {
      static struct option long_options[] = {
	/* These options set a flag. */
	{"help", no_argument, 0, 'h'},
	{"h", no_argument, 0, 'h'},
	{"red", required_argument, 0, 'e'},
	{"green", required_argument, 0, 'f'},
	{"blue", required_argument, 0, 'g'},
	{0, 0, 0, 0}
      };
      /* getopt_long stores the option index here. */
      int option_index = 0;

      c =
	getopt_long (argc, argv, "ha:c:d:e:f:g:?", long_options,
		     &option_index);

      /* Detect the end of the options. */
      if (c == -1)
	break;

      switch (c)
	{
	case 'h':
	  usage ();
	  break;
	case 'a':
	case 'e':
	  strcpy (i3, optarg);
	  break;
	case 'c':
	case 'f':
	  strcpy (m8, optarg);
	  break;
	case 'd':
	case 'g':
	  strcpy (i1, optarg);
	  break;
	case '?':
	  /* getopt_long already printed an error message. */
	  printf ("Error of some unknown origin.\n");
	  break;
	default:
	  printf ("Error - unreconized option => \"%s\". \n", optarg);
	  usage ();
	}
    }

  /* Print any remaining command line arguments (not options). */
  if (optind + 1 == argc)
    {
      strcpy (outfile, argv[optind]);
    }
  else
    {
      usage ();
    }

  printf ("Info:  outfile=%s i3=%s, m8=%s, i1=%s\n", outfile, i3, m8, i1);

}




/* Makes a copy of a dataset, and opens it for writing.. */
GDALDatasetH
create_output_dateset (GDALDatasetH * in_dataset, char *copy_file_name,
		       int jpeg)
{
  char **papszOptions = NULL;
  const char *pszFormat = "GTiff";
  double adfGeoTransform[6];
  GDALDriverH hDriver;
  GDALDatasetH out_gdalfile;
  hDriver = GDALGetDriverByName (pszFormat);

  papszOptions = CSLSetNameValue (papszOptions, "TILED", "YES");

  if (jpeg)
    {
      papszOptions = CSLSetNameValue (papszOptions, "COMPRESS", "JPEG");
      papszOptions = CSLSetNameValue (papszOptions, "PHOTOMETRIC", "YCBCR");
    }
  else
    {
      papszOptions = CSLSetNameValue (papszOptions, "COMPRESS", "DEFLATE");
    }

  /*Perhaps controversal - default to bigtiff... */
  papszOptions = CSLSetNameValue (papszOptions, "BIGTIFF", "YES");



  /*return GDALCreateCopy( hDriver, copy_file_name, *in_dataset, FALSE, papszOptions, NULL, NULL ); */
  out_gdalfile = GDALCreate (hDriver, copy_file_name,
			     GDALGetRasterXSize (*in_dataset),
			     GDALGetRasterYSize (*in_dataset),
			     3, GDT_Byte, papszOptions);

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

char
gamma_scale (double value, double gamma_val)
{
  return (char) round (pow ((value / 255.0), (1.0 / gamma_val)) * 255.0);
}

int
main (int argc, char *argv[])
{
  GDALDriverH hDriver;
  double adfGeoTransform[6];
  GDALDatasetH in_Datasets[3];
  GDALDatasetH out_Dataset;
  char *data_scan_line;
  char *out_scan_line;
  int nBlockXSize, nBlockYSize;
  int bands;
  int xsize;
  int jpeg = 0;
  char outfile[2024], i1[2024], i3[2024], m8[2024];	/*bad form.. */

  /* parse command line */
  parse_opts (argc, argv, m8, i3, i1, outfile);

  GDALAllRegister ();

  /* Set cache to something reasonable.. - 1/2 gig */
  CPLSetConfigOption ("GDAL_CACHEMAX", "2048");

  /* open datasets.. */
  in_Datasets[0] = GDAL_open_read (i3);
  in_Datasets[1] = GDAL_open_read (m8);
  in_Datasets[2] = GDAL_open_read (i1);


  out_Dataset = create_output_dateset (in_Datasets, outfile, jpeg);

  /* Basic info on source dataset.. */
  GDALGetBlockSize (GDALGetRasterBand (in_Datasets[0], 1), &nBlockXSize,
		    &nBlockYSize);
  printf ("Info: Block=%dx%d Type=%s, ColorInterp=%s\n", nBlockXSize,
	  nBlockYSize,
	  GDALGetDataTypeName (GDALGetRasterDataType
			       (GDALGetRasterBand (in_Datasets[0], 1))),
	  GDALGetColorInterpretationName (GDALGetRasterColorInterpretation
					  (GDALGetRasterBand
					   (in_Datasets[0], 1))));

  /* Loop though bands, scaling the data.. */
  xsize = GDALGetRasterXSize (in_Datasets[0]);
  data_scan_line = (char *) CPLMalloc (sizeof (char) * xsize);
  out_scan_line = (char *) CPLMalloc (sizeof (char) * xsize);

  for (bands = 1; bands <= 3; bands++)
    {
      int x, y_index;
      GDALRasterBandH data_band, out_band;

      printf ("Info: Processing band #%d\n", bands);
      data_band = GDALGetRasterBand (in_Datasets[bands - 1], 1);
      out_band = GDALGetRasterBand (out_Dataset, bands);

      /* Set nodata for that band */
      GDALSetRasterNoDataValue (out_band, 0.0);

      for (y_index = 0; y_index < GDALGetRasterYSize (out_Dataset); y_index++)
	{
	  /* Read data.. */
	  /*GDALRasterIO (data_band, GF_Read, 0, y_index, xsize, 1,
	     data_scan_line, xsize, 1, GDT_Float64, 0, 0); */
	  GDALRasterIO (GDALGetRasterBand (in_Datasets[bands - 1], 1),
			GF_Read, 0, y_index, xsize, 1, data_scan_line, xsize,
			1, GDT_Byte, 0, 0);
	  for (x = 0; x < xsize; x++)
	    {
	      out_scan_line[x] =
		gamma_scale ((double) data_scan_line[x], 1.0);
	    }
	  GDALRasterIO (out_band, GF_Write, 0, y_index, xsize, 1,
			out_scan_line, xsize, 1, GDT_Byte, 0, 0);
	}

    }

  /* close file, and we are done. */
  GDALClose (out_Dataset);

  return 0;

}
