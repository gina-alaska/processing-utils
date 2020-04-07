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
    printf ("This tool is a simple util for making viirs night time microphsyics. \n");
    printf ("Use it like:\n");
    printf
    ("\tnight_time_micro (--12um|red 12um_file) (--11um|green 11um_file ) (--3_74um|blue 3_74um_file ) <outfile> \n");
    printf ("\t\twhere:\n");
    printf
    ("\t\t\t<outfile> is the output file.  It will be deflate compressed, and tiled, with 0 as nodata.\n");
    printf
    ("\t\t\t(blue-cap) is how much to stretch the blue - try 1.0 or 0.75 .\n");

    exit(-1);
}



/*******************************************************************************************

Notes from Carl =>
Red = 12um - 11um (m16 - m13)
Green = 11um - 3.74 (i05 - i04)
Blue = 11um (i05)

Also for the NT Micro...
Red  min=-6.7  max=+2.6
Green  min=-3.1  max=+5.2
Blue  min=-29.55   max=+19.45

*********************************************************************************************/



/* command line parsing..*/
void
parse_opts (int argc, char **argv, char *i3_75, char *i11, char *m12, char *outfile)
{
    int c;

    while (1)
    {
        static struct option long_options[] = {
            /* These options set a flag. */
            {"help", no_argument, 0, 'h'},
            {"h", no_argument, 0, 'h'},
            {"12um", required_argument, 0, 'a'},
            {"11um", required_argument, 0, 'c'},
            {"3_74um", required_argument, 0, 'd'},
            {"red", required_argument, 0, 'e'},
            {"green", required_argument, 0, 'f'},
            {"blue", required_argument, 0, 'g'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "ha:c:d:?", long_options, &option_index);

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
            strcpy (m12, optarg);
            break;
        case 'c':
	case 'f':
            strcpy (i11, optarg);
            break;
        case 'd':
	case 'g':
	    strcpy (i3_75, optarg);
            break;
        case '?':
            /* getopt_long already printed an error message. */
	    printf("Error of some unknown origin.\n");
            break;
        default:
	    printf("Error - unreconized option => \"%s\". \n", optarg);
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

    printf ("Info:  outfile=%s m3_75=%s, b11=%s, b12_m=%s\n",
            outfile, i3_75, i11, m12);

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

double
k2c (double k)
{
    return k - 273.15;
}



double scale_red(double um12, double um11) 
{
	/* Red = 12um - 11um (m16 - m13) */
	/* RED min=-6.7, max=+2.6 */
	double min= -6.7, max= +2.6;
	double delta = 9.3;
	double diff;

	if (isnan(um12) || isnan(um11)) {return 0.0;}
	
	diff = (k2c(um12)- k2c(um11)); 
	//printf("RED: um12 = %g um11 = %g, diff = %g, value = %g\n", um12, um11, diff, ((diff - min) / delta)*255.0 );
	if (diff < min ) { return 1.0; };
	if (diff > max ) { return 255.0;};
	return ((diff - min) / delta)*255.0 ;

}

double scale_green(double um3_74, double um11)
{
/* Green = 11um - 3.74 (i05 - i04) */
/* Green  min=-3.1  max=+5.2 */
        double min= -3.1, max= 5.2;
        double delta;/* = ( .0-3.1 + 5.2);*/
        double diff;

	if (isnan(um3_74) || isnan(um11)) {return 0.0;}
	delta = max-min;
	diff = (k2c(um11)- k2c(um3_74));
	//printf("GREEN:  um11 = %g um3_74 = %g, diff = %g, value = %g\n", um11, um3_74, diff, ((diff - min) / delta)*255.0 );
        if (diff < min ) { return 1.0; };
        if (diff > max ) { return 255.0;};
	return ((diff - min) / delta)*255.0 ;
}

double scale_blue(double um11)
{
/* Blue = 11um (i05) */
/* Blue  min=-29.55   max=+19.45 */
        double min=-29.55, max=19.45; 
        double delta;
	double c_um11;
	if (isnan(um11)) {return 0.0;}
	delta = max-min;
	
	c_um11 = k2c(um11);

        /*printf("BLUE:  um11 = %g delta = %g, value = %g\n", um11, delta, ((c_um11 - min) / delta)*255.0 );*/
        if (c_um11 < min ) { return 1.0; };
        if (c_um11 > max ) { return 255.0;};
        return ((c_um11 - min) / delta)*255.0 ;
}


double gamma_scale(double value, double gamma_val )
{
	return  pow((value / 255.0), (1.0 / gamma_val)) * 255.0; 
}

int
main (int argc, char *argv[])
{
    GDALDriverH hDriver;
    double adfGeoTransform[6];
    GDALDatasetH in_Datasets[3];
    GDALDatasetH out_Dataset;
    double *i3_75_data_scan_line, *i11_data_scan_line, *m12_data_scan_line;
    char *out_scan_line;
    int nBlockXSize, nBlockYSize;
    int bGotMin, bGotMax;
    int bands;
    int xsize;
    int jpeg=0;
    double blue_cap;
    char outfile[2024], i3_75[2024], i11[2024], m12[2024]; /*bad form.. */
    double min, middle, max;


    /* parse command line */
    parse_opts (argc, argv, i3_75, i11, m12,outfile);


    GDALAllRegister ();

    /* Set cache to something reasonable.. - 1/2 gig */
    CPLSetConfigOption ("GDAL_CACHEMAX", "2048");

    /* open datasets.. */
    in_Datasets[0] = GDAL_open_read (i3_75);
    in_Datasets[1] = GDAL_open_read (i11);
    in_Datasets[2] = GDAL_open_read (m12);


    out_Dataset = create_output_dateset(in_Datasets, outfile,jpeg);

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
    i3_75_data_scan_line = (double *) CPLMalloc (sizeof (double) * xsize);
    i11_data_scan_line = (double *) CPLMalloc (sizeof (double) * xsize);
    m12_data_scan_line = (double *) CPLMalloc (sizeof (double) * xsize);
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
                          data_scan_line, xsize, 1, GDT_Float64, 0, 0);*/
	    GDALRasterIO (GDALGetRasterBand(in_Datasets[0],1), GF_Read, 0, y_index, xsize, 1,
                          i3_75_data_scan_line, xsize, 1, GDT_Float64, 0, 0);
            GDALRasterIO (GDALGetRasterBand(in_Datasets[1],1), GF_Read, 0, y_index, xsize, 1,
                          i11_data_scan_line, xsize, 1, GDT_Float64, 0, 0);
            GDALRasterIO (GDALGetRasterBand(in_Datasets[2],1), GF_Read, 0, y_index, xsize, 1,
                          m12_data_scan_line, xsize, 1, GDT_Float64, 0, 0);

            for (x = 0; x < xsize; x++)
            {
                if (bands == 1)
                {
                    out_scan_line[x] = scale_red(m12_data_scan_line[x], i11_data_scan_line[x]);
                } else if (bands == 3)
                {
                    out_scan_line[x] = gamma_scale(scale_blue(i11_data_scan_line[x]), 2.0);
                }
                else
                {
                    out_scan_line[x] = scale_green(i3_75_data_scan_line[x], i11_data_scan_line[x]);
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
