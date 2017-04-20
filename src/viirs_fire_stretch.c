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
ussage ()
{
    printf ("This tool is a simple util for making fire color images. \n");
    printf ("Use it like:\n");
    printf
    ("\tviirs_fire_stretch (--red red_file) (--green green_file ) (--blue blue_file ) (--blue-cap 0.75)  <outfile>\n");
    printf ("\t\twhere:\n");
    printf
    ("\t\t\t<outfile> is the output file.  It will be deflate compressed, and tiled, with 0 as nodata.\n");
    printf
    ("\t\t\t(blue-cap) is how much to stretch the blue - try 1.0 or 0.75 .\n");

    abort ();
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
parse_opts (int argc, char **argv, char *red, char *blue, char *green,
            double *blue_cap, char *outfile)
{
    int c;

    while (1)
    {
        static struct option long_options[] = {
            /* These options set a flag. */
            {"help", no_argument, 0, 'h'},
            {"h", no_argument, 0, 'h'},
            {"red", required_argument, 0, 'r'},
            {"green", required_argument, 0, 'g'},
            {"blue", required_argument, 0, 'b'},
            {"blue-cap", required_argument, 0, 'c'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "hr:g:b:c:?", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
        case 'h':
            ussage ();
            break;
        case 'r':
            strcpy (red, optarg);
            break;
        case 'g':
            strcpy (green, optarg);
            break;
        case 'b':
            strcpy (blue, optarg);
            break;
        case 'c':
            *blue_cap = atof (optarg);
            break;
        case '?':
            /* getopt_long already printed an error message. */
            break;

        default:
            ussage ();
        }
    }

    /* Print any remaining command line arguments (not options). */
    if (optind + 1 == argc)
    {
        strcpy (outfile, argv[optind]);
    }
    else
    {
        ussage ();
    }

    printf ("Info:  outfile=%s red=%s, green=%s, blue = %s\n",
            outfile, red, green, blue);

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
        abort ();
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
    GDALDatasetH in_Datasets[3];
    GDALDatasetH out_Dataset;
    double *data_scan_line;
    char *out_scan_line;
    int nBlockXSize, nBlockYSize;
    int bGotMin, bGotMax;
    int bands;
    int xsize;
    double blue_cap;
    char red[2024], green[2024], blue[2024], outfile[2024];	/*bad form.. */
    double min, middle, max;


    /* parse command line */
    parse_opts (argc, argv, red, blue, green, &blue_cap, outfile);


    GDALAllRegister ();

    /* Set cache to something reasonable.. - 1/2 gig */
    CPLSetConfigOption ("GDAL_CACHEMAX", "2048");

    /* open datasets.. */
    in_Datasets[0] = GDAL_open_read (red);
    in_Datasets[1] = GDAL_open_read (green);
    in_Datasets[2] = GDAL_open_read (blue);

    out_Dataset = make_me_a_sandwitch (in_Datasets, outfile);

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
    data_scan_line = (double *) CPLMalloc (sizeof (double) * xsize);
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
            GDALRasterIO (data_band, GF_Read, 0, y_index, xsize, 1,
                          data_scan_line, xsize, 1, GDT_Float64, 0, 0);
            for (x = 0; x < xsize; x++)
            {
                if (bands == 1)
                {
                    data_scan_line[x] = scale_k (data_scan_line[x]);
                };
                if (bands == 3)
                {
                    out_scan_line[x] = scale (0.0, blue_cap, data_scan_line[x]);
                }
                else
                {
                    out_scan_line[x] = scale (0.0, 1.0, data_scan_line[x]);
                };
            }
            GDALRasterIO (out_band, GF_Write, 0, y_index, xsize, 1,
                          out_scan_line, xsize, 1, GDT_Byte, 0, 0);
        }

    }

    /* close file, and we are done. */
    GDALClose (out_Dataset);

    return 0;

}