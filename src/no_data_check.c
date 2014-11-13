#include "gdal.h"
#include "cpl_string.h"
#include "cpl_conv.h"

/************************************************************/
/* Simple test for no data pixels pixels.. */
/* Use like:            */
/* ./saturate <infile> <outfile> */
/* <infile> .*/
/* questions or comments? jc@alaska.edu     */
/* complile like:  gcc $(gdal-config --cflags) -o  no_data_check no_data_check.c $(gdal-config --libs) */
/************************************************************/



/* Makes a copy of a dataset, and opens it for writing.. */
GDALDatasetH make_me_a_sandwitch(GDALDatasetH *in_dataset, char *filename)
{
    char **papszOptions = NULL;
    const char *pszFormat = "GTiff";
    GDALDriverH hDriver;
    GDALDatasetH out_gdalfile;
    hDriver = GDALGetDriverByName( pszFormat );
    papszOptions = CSLSetNameValue( papszOptions, "TILED", "YES" );
    papszOptions = CSLSetNameValue( papszOptions, "COMPRESS", "DEFLATE" );
    
    /*Create copy..*/
    /*return GDALCreateCopy( hDriver, filename, *in_dataset, FALSE, papszOptions, NULL, NULL );*/
    return GDALCreate(hDriver, filename,
        GDALGetRasterXSize( *in_dataset ),
        GDALGetRasterYSize( *in_dataset ),
        1,
        GDT_Byte, papszOptions );
}

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

void ussage(char *progname)
{
    fprintf(stderr, "Whooowww.. I don't work like that..");
    fprintf(stderr, "use me like:\n");
    fprintf(stderr, "%s <infile> <outfile>\n", progname);
    fprintf(stderr, "\tinfile can be any image that gdal reads\n");
    fprintf(stderr, "\t<outfile> will be a single banded tiff)\n");
    fprintf(stderr, "Problems? Questions? Complain to jay@alaska.edu so he can ignore them..\n\nBy now.\n\n");
    exit(-1);
}

int main( int argc, const char* argv[] )
{
    GDALDriverH   hDriver;
    double        adfGeoTransform[6];
    GDALDatasetH  in_Dataset;
    GDALDatasetH  mask_Dataset;
    GDALDatasetH  out_Dataset;
    GDALRasterBandH mask_band;
    unsigned char   *out_scan_line, *data_scan_line;
    int             nBlockXSize, nBlockYSize;
    int             bGotMin, bGotMax;
    int             bands;
    int             xsize;
    double          adfMinMax[2];
    int             valid_data_pixels[10];
    int             saturated_data_pixels[10];
    int             y_index, x;
    GDALRasterBandH  out_band;
    
    
    if ( argc != 3 ) {
        ussage(argv[0]);
    }

    
    GDALAllRegister();
    
    /* Set cache to something reasonable.. - 1/2 gig*/
    CPLSetConfigOption( "GDAL_CACHEMAX", "512" );

    /* open datasets..*/
    in_Dataset = GDAL_open_read( argv[1]);
    out_Dataset = make_me_a_sandwitch(&in_Dataset, argv[2]);
    
    /* Basic info on source dataset..*/
    GDALGetBlockSize(GDALGetRasterBand( in_Dataset, 1 ) , &nBlockXSize, &nBlockYSize );
    
    /* Loop though bands, checking for saturated pixels .... */
    xsize = GDALGetRasterXSize( in_Dataset );
    data_scan_line = (char *) CPLMalloc(sizeof(char)*xsize);
    out_scan_line = (char *) CPLMalloc(sizeof(char)*xsize);
    
    /* The output band... */
    out_band =  GDALGetRasterBand( out_Dataset, 1);

   
    /* wipe counters.. */ 
    for (bands=1; bands <= GDALGetRasterCount( in_Dataset ); bands ++ ) {
        valid_data_pixels[bands] = 0;
        saturated_data_pixels[bands] = 0;
    }
    
    /* loop though the lines of the data, looking for no data and saturated pixels..*/
    for (y_index = 0; y_index <GDALGetRasterYSize( in_Dataset ); y_index ++ ) {
        for (bands=1; bands <= GDALGetRasterCount( in_Dataset ); bands ++ ) {
            GDALRasterBandH data_band;
            /* Read data..*/
            data_band =  GDALGetRasterBand( in_Dataset, bands);
            GDALRasterIO( data_band, GF_Read, 0, y_index, xsize , 1, data_scan_line, xsize , 1, GDT_Byte, 0, 0 );
            /* If first band, then copy into output slice.. */
            if (bands==1) {
                unsigned char  data_value;
                for(x=0; x < xsize; x++) {
                    /*shift to make darker...*/
                   out_scan_line[x] = data_scan_line[x] >> 1 ;
                   if ( out_scan_line[x] ==0 && data_scan_line[x] != 0) {out_scan_line[x] = 1;}
                }
            }
            
            /* Loop though, looking for saturated pixels and no-data values.. */
            for(x=0; x < xsize; x++) {
                if (  data_scan_line[x] == 0 )  {
                    out_scan_line[x] = 255;
                }
            }
        }
        GDALRasterIO( out_band, GF_Write, 0, y_index, xsize , 1, out_scan_line, xsize , 1, GDT_Byte, 0, 0 );
    }
    
    GDALClose(out_Dataset);
}
