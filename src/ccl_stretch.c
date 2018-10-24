#include "gdal.h"
#include "cpl_string.h"
#include "cpl_conv.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <math.h>



/************************************************************/
/*  Makes "fire" color images from viirs */
/************************************************************/


/* print some usage information and quit..*/
void
usage ()
{
    printf ("This tool is a simple util for making fire color images. \n");
    printf ("Use it like:\n");
    printf
    ("\tviirs_fire_stretch (--ccl1 ccl1_file) (--ccl2 ccl2_file )  (--ccl3 ccl3_file ) (--ccl4 ccl4_file ) (--ccl5 ccl5_file )  <outfile>\n");
    printf ("\t\twhere:\n");
    printf
    ("\t\t\t<outfile> is the output file.  It will be deflate compressed, and tiled, with 0 as nodata.\n");
    printf
    ("\t\t\t(blue-cap) is how much to stretch the blue - try 1.0 or 0.75 .\n");

    exit(-1);
}



/*******************************************************************************************

Notes from curtis =>

red = M-12 brightness temperatures (K)

green = M-11 reflectance

blue = M-10 reflectance


  scaled_red = ((red - 273.) / (333. - 273.))^2.5
  img_r = bytscl(scaled_red, min=0., max=1.)
  img_g = bytscl(green, min=0., max=1.)
  img_b = bytscl(blue, min=0., max=0.75)


#I band mix, fire natural color

red = i4 brightness temperatures (K)

green = i2 reflectance

blue = i1 reflectance


  scaled_red = ((red - 273.) / (333. - 273.))^2.5
  img_r = bytscl(scaled_red, min=0., max=1.)
  img_g = bytscl(green, min=0., max=1.)
  img_b = bytscl(blue, min=0., max=1.)
*********************************************************************************************/



/* command line parsing..*/
void
parse_opts (int argc, char **argv, char *ccl1, char *ccl2,char *ccl3,char *ccl4,char *ccl5,char * outfile)
{
    int c;
    while (1)
    {
        static struct option long_options[] = {
            /* These options set a flag. */
            {"help", no_argument, 0, 'h'},
            {"h", no_argument, 0, 'h'},
            {"ccl1", required_argument, 0, '1'},
            {"ccl2", required_argument, 0, '2'},
            {"ccl3", required_argument, 0, '3'},
            {"ccl4", required_argument, 0, '4'},
            {"ccl5", required_argument, 0, '5'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "hr:g:b:c:j?", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
        case 'h':
            usage ();
            break;
        case '1':
            strcpy (ccl1, optarg);
            break;
        case '2':
            strcpy (ccl2, optarg);
            break;
        case '3':
            strcpy (ccl3, optarg);
            break;
        case '4':
            strcpy (ccl4, optarg);
            break;
        case '5':
            strcpy (ccl5, optarg);
            break;
        default:
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

}




/* Makes a copy of a dataset, and opens it for writing.. */
GDALDatasetH
create_output_dateset (GDALDatasetH * in_dataset, char *copy_file_name, int jpeg)
{
    char **papszOptions = NULL;
    const char *pszFormat = "GTiff";
    double adfGeoTransform[6];
    GDALDriverH hDriver;
    GDALDatasetH out_gdalfile;
    hDriver = GDALGetDriverByName (pszFormat);

    papszOptions = CSLSetNameValue (papszOptions, "TILED", "YES");

    if (jpeg)  {
        papszOptions = CSLSetNameValue (papszOptions, "COMPRESS", "JPEG");
        papszOptions = CSLSetNameValue (papszOptions, "PHOTOMETRIC","YCBCR");
    }
    else {
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
        exit(-1);
    }
    return gdalDataset;
}



/* performs the stretch - first scales to 1.0 to 255, then does a piecewise color enhancement.*/
char
scale (double min, double max, double value)
{
    double result = 0.0;
    //printf("min %g max %g value %g\n", min, max, value);

    if (isnan (value))
    {
        return 0;
    }

    if (value < min)
    {
        return 0;
    }
    if (value >= max)
    {
        return 255;
    }

    result = ((value - min) / (max - min)) * 255.0;

    //printf("%g -> %g\n", value, result);

    if (result < 1.0)
        return 1;
    if (result >= 255.0)
        return 255;

    return (char) round (result);
}

double
scale_k (double k)
{
    double result = 0.0;
    if (k == 0.0)
    {
        return 0.0;
    }
    if (k > 0.0 && k < 273.0)
    {
        return 0.001;
    };
    result = pow ((k - 273.0) / (333.0 - 273.0), 2.5);
    if (result > 1.0)
    {
        result = 1.0;
    };
    return result;

}

int
main (int argc, char *argv[])
{
    GDALDriverH hDriver;
    double adfGeoTransform[6];
    GDALDatasetH in_Datasets[5];
    GDALDatasetH out_Dataset;
    double *ccl[5];
    char *out_scan_line[3];
    int nBlockXSize, nBlockYSize;
    int bGotMin, bGotMax;
    int bands;
    int xsize;
    int y_index;
    int i;
    char ccl1[2024], ccl2[2024], ccl3[2024], ccl4[2014], ccl5[2014],outfile[2024];	/*bad form.. */

    /* parse command line */
    parse_opts (argc, argv, ccl1,ccl2,ccl3,ccl4,ccl5,outfile);

    GDALAllRegister ();

    /* Set cache to something reasonable.. - 1/2 gig */
    CPLSetConfigOption ("GDAL_CACHEMAX", "2048");

    /* open datasets.. */
    in_Datasets[0] = GDAL_open_read (ccl1);
    in_Datasets[1] = GDAL_open_read (ccl2);
    in_Datasets[2] = GDAL_open_read (ccl3);
    in_Datasets[3] = GDAL_open_read (ccl4);
    in_Datasets[4] = GDAL_open_read (ccl5);

    out_Dataset = create_output_dateset(in_Datasets, outfile,0);

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
   
    for(i=0;i<5;i++) { ccl[i] = (double *) CPLMalloc (sizeof (double) * xsize); } 
    for(i=0;i<3;i++) { out_scan_line[i] = (char *) CPLMalloc (sizeof (char) * xsize);}

    for (y_index = 0; y_index < GDALGetRasterYSize (out_Dataset); y_index++)
    {
	/* read */
	for(i=0;i<5;i++) {
	    GDALRasterBandH band;
 	    band = GDALGetRasterBand(in_Datasets[i], 1);
            GDALRasterIO (band, GF_Read, 0, y_index, xsize, 1, ccl[i], xsize, 1, GDT_Float64, 0, 0);
	 }

	for(i=0;i<xsize;i++) {
		out_scan_line[0][i] = (char)((ccl[4][i]*0.75 + ccl[3][i]*0.25) * 2.56 + 0.5);
		out_scan_line[1][i] = (char)((ccl[3][i]*0.25 + ccl[2][i]*0.5 + ccl[1][i]*0.25)  * 2.56 + 0.5);
		out_scan_line[2][i] = (char)((ccl[1][i]*0.25 + ccl[0][i]*0.75) * 2.56 + 0.5);

	}
		
	/* calculate.. */
	for(i=0;i<3;i++) {
	   GDALRasterBandH band;
           band = GDALGetRasterBand(out_Dataset, i+1);
	   GDALRasterIO (band, GF_Write, 0, y_index, xsize, 1, out_scan_line[i], xsize, 1, GDT_Byte, 0, 0);
	}
    }

    /* close file, and we are done. */
    GDALClose (out_Dataset);

    return 0;

}
