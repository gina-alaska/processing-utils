#include "gdal.h"
#include "cpl_string.h"
#include "cpl_conv.h"

/************************************************************/
/* update */
/************************************************************/


/* print some usage information and quit..*/
void ussage ( ) {
        printf("This tool is a simple util for stretching thermal data for awips.  Works in a strange manner, just a FWI.\n");
 	printf(" (see https://github.com/Unidata/awips2/blob/master_14.4.1/edexOsgi/com.raytheon.uf.common.dataplugin.satellite/src/com/raytheon/uf/common/dataplugin/satellite/units/ir/IRTempToPixelConverter.java ) \n")
        printf("Use it like:\n");
        printf("\tawips_thermal_stretch (--min min_level) (--max max_level ) (--middle middle_level ) <infile> <outfile>\n");
        printf("\t\twhere:\n");
        printf("\t\t\t<infile> is a single banded floating point tif.  Can be single or double. \n");
        printf("\t\t\t<outfile> is the output file.  It will be deflate compressed, and tiled, with 0 as nodata.\n");
        exit(-1);
}



/* command line parsing..*/
void parse_opts(int argc, char **argv, double *min, double *middle, double *max, char *infile, char *outfile)
{
  int c;
  *min=418.15;
  *middle=238.15;
  *max=656.3;

  while (1)
    {
      static struct option long_options[] =
        {
          /* These options set a flag. */
          {"help",      no_argument,            0,      'h'},
          {"h",         no_argument,            0,      'h'},
          {"min",       required_argument,      0,      'm'},
          {"max",       required_argument,      0,      'M'},
          {"middle",    required_argument,      0,      'z'},
          {0, 0, 0, 0}
        };
      /* getopt_long stores the option index here. */
      int option_index = 0;

      c = getopt_long (argc, argv, "abc:d:f:",
                       long_options, &option_index);

      /* Detect the end of the options. */
      if (c == -1)
        break;

      switch (c)
        {
        case 'h':
          puts("Usage:");
          exit(0);
          break;
        case 'm':
          *min = atof(optarg);
          break;
        case 'M':
          *max = atof(optarg);
          break;
        case 'z':
          *middle = atof(optarg);
          break;
        case '?':
          /* getopt_long already printed an error message. */
          break;

        default:
          abort ();
        }
    }

  /* Print any remaining command line arguments (not options). */
  if (optind + 2 == argc) {
        strcpy(infile,argv[optind]);
        optind++;
        strcpy(outfile,argv[optind]);
  } else {
        ussage();
   }

  printf("Info: Infile=%s, outfile=%s min=%g, max=%g, middle = %g\n", infile, outfile, *min, *max, *middle);

}




/* Makes a copy of a dataset, and opens it for writing.. */
GDALDatasetH make_me_a_sandwitch(GDALDatasetH *in_dataset, char *copy_file_name)
{
    char **papszOptions = NULL;
    const char *pszFormat = "GTiff";
    double        adfGeoTransform[6];
    GDALDriverH hDriver;
    GDALDatasetH out_gdalfile;
    hDriver = GDALGetDriverByName( pszFormat );
    papszOptions = CSLSetNameValue( papszOptions, "TILED", "YES" );
    papszOptions = CSLSetNameValue( papszOptions, "COMPRESS", "DEFLATE" );
    
        /*Perhaps controversal - default to bigtiff... */
    papszOptions = CSLSetNameValue( papszOptions, "BIGTIFF", "YES" ); 
    
    /*return GDALCreateCopy( hDriver, copy_file_name, *in_dataset, FALSE, papszOptions, NULL, NULL );*/
    out_gdalfile = GDALCreate(hDriver, copy_file_name,
        GDALGetRasterXSize( *in_dataset ),
        GDALGetRasterYSize( *in_dataset ),
        GDALGetRasterCount( *in_dataset ),
        GDT_Byte, papszOptions );

   /* Set geotransform */
   GDALGetGeoTransform( *in_dataset, adfGeoTransform );
   GDALSetGeoTransform(out_gdalfile,  adfGeoTransform );

   /* Set projection */
   GDALSetProjection(out_gdalfile,GDALGetProjectionRef( *in_dataset ) );
   return out_gdalfile; 
}


/* Opens a file */ 
GDALDatasetH GDAL_open_read(char *file_name)
{
    GDALDatasetH  gdalDataset;
    gdalDataset = GDALOpen( file_name, GA_ReadOnly );
    if( gdalDataset == NULL )
    {
        printf("Hmm, could not open '%s' for reading.. this be an error, exiting..\n", file_name);
        exit(-1);
    }
}



/* performs the stretch - first scales to 1.0 to 255, then does a piecewise color enhancement.*/
char scale(double value, double min, double max) {
	int i=0;
	double x[6]={0.0, 30.0, 60.0, 120.0, 190.0, 255.0}; /*source */
	double y[6]={0.0,110.0, 160.0, 210.0, 240.0, 255.0}; /*target*/
	double b,m;

	if ( value > MAX_MODIS) return 0;		/*nodata value*/

	value = 1.0 +(254.0 + 0.9999)*(value - min)/(max - min);
	
	/*Find correct "bin"*/
	while ( i < 4 && !(value >= x[i] && value <= x[i+1] )) { i++;};

	/* perform the mapping */
	m = (y[i+1]-y[i])/(x[i+1] - x[i]);
	b = y[i+1] - m*x[i+1];
	value = (m * value + b);

	/* double check values make sence.*/
	if (value > 255 ) return 255;
	return (char)value;
	
	
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




int main( int argc, const char* argv[] )
{
    GDALDriverH   hDriver;
    double        adfGeoTransform[6];
    GDALDatasetH  in_Dataset;
    GDALDatasetH  out_Dataset;
    unsigned int    *data_scan_line;
    char 	    *out_scan_line;
    int             nBlockXSize, nBlockYSize;
    int             bGotMin, bGotMax;
    int             bands;
    int             xsize;
    double          adfMinMax[2];
    
    GDALAllRegister();


    /* ussage..*/
    if (argc != 3 ) ussage();
    
    /* Set cache to something reasonable.. - 1/2 gig*/
    CPLSetConfigOption( "GDAL_CACHEMAX", "512" );

    /* open datasets..*/
    in_Dataset = GDAL_open_read( argv[1]);
    out_Dataset= make_me_a_sandwitch(&in_Dataset,argv[2]);
    
    /* Basic info on source dataset..*/
    GDALGetBlockSize(GDALGetRasterBand( in_Dataset, 1 ) , &nBlockXSize, &nBlockYSize );
    printf( "Block=%dx%d Type=%s, ColorInterp=%s\n",
                nBlockXSize, nBlockYSize,
                GDALGetDataTypeName(GDALGetRasterDataType( GDALGetRasterBand( in_Dataset, 1 ))),
                GDALGetColorInterpretationName(
                    GDALGetRasterColorInterpretation(GDALGetRasterBand( in_Dataset, 1 ))));
    
    /* Loop though bands, scaling the data.. */
    xsize = GDALGetRasterXSize( in_Dataset );
    data_scan_line = (unsigned int *) CPLMalloc(sizeof(unsigned int)*xsize);
    out_scan_line = (char *) CPLMalloc(sizeof(char)*xsize);

    for (bands=1; bands <= GDALGetRasterCount( in_Dataset ); bands ++ ) {
        int x;
	unsigned int min=9999999,max=0;	/* probibly a better way to set these..*/
	double dmin,dmax;
        GDALRasterBandH data_band, out_band;
        int y_index = 0;
        data_band =  GDALGetRasterBand( in_Dataset, bands);
        out_band =  GDALGetRasterBand( out_Dataset, bands);

	/* Set nodata for that band*/
	GDALSetRasterNoDataValue(out_band,0.0);

	/*Find Min,Max, required for scaling*/
	for (y_index = 0; y_index <GDALGetRasterYSize( in_Dataset ); y_index ++ ) {
            /* Read data..*/
            GDALRasterIO( data_band, GF_Read, 0, y_index, xsize , 1, data_scan_line, xsize , 1,GDT_UInt32 , 0, 0 );
	    for(x=0; x < xsize; x++) { 
			if ( data_scan_line[x] < MAX_MODIS ) {
				if ( data_scan_line[x] > max ) max = data_scan_line[x];
				else if ( data_scan_line[x] < min ) min = data_scan_line[x];
			}
		} 
	}

	dmax = (double)max;
	dmin = (double)min;

	printf("Info: For Band %d -> Min=%g,Max=%g\n", bands, dmin,dmax);

        for (y_index = 0; y_index <GDALGetRasterYSize( in_Dataset ); y_index ++ ) {
	    double scaled;

            /* Read data..*/
            GDALRasterIO( data_band, GF_Read, 0, y_index, xsize , 1, data_scan_line, xsize , 1,GDT_UInt32 , 0, 0 );

	    /* scale each .. */
            for(x=0; x < xsize; x++) {
		out_scan_line[x] = scale(data_scan_line[x], dmin, dmax);
            }
            
            /* now write out band..*/
            GDALRasterIO( out_band, GF_Write, 0, y_index, xsize , 1, out_scan_line, xsize , 1, GDT_Byte, 0, 0 );
        }
        
    }
    
    
    /* close file, and we are done.*/
    GDALClose(out_Dataset);

 }


