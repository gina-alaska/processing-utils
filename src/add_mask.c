#include "gdal.h"
#include "cpl_string.h"
#include "cpl_conv.h"

/************************************************************/
/* Simple util that adds a mask to a file */
/* Use like:            */
/* ./add_mask <infile>  */
/* compile like: gcc $(gdal-config --cflags) -o add_mask add_mask.c $(gdal-config --libs) */
/************************************************************/


GDALDatasetH GDAL_open(char *file_name)
{
    GDALDatasetH  gdalDataset;
    gdalDataset = GDALOpen( file_name, GA_Update );
    if( gdalDataset == NULL )
    {
        printf("Hmm, could not open '%s' for reading.. this be an error, exiting..\n", file_name);
        exit(-1);
    }
    return gdalDataset;
}


int main( int argc, const char* argv[] )
{
    GDALDriverH   hDriver;
    double        adfGeoTransform[6];
    GDALDatasetH  in_Dataset;
    GDALDatasetH  mask_Dataset;
    GDALDatasetH  out_Dataset;
    GDALRasterBandH mask_band;
    char            *mask_scan_line, *data_scan_line;
    int             nBlockXSize, nBlockYSize;
    int             bGotMin, bGotMax;
    int             bands;
    int             xsize;
    double          adfMinMax[2];
    
    GDALAllRegister();
    
    if ( argc != 2) {
        printf("Usage: %s <file to add mask to> \n", argv[0]);
        printf("This utility adds a nodata mask to a file, were the datadata value is \"0\"\n");
        printf("contact/blame jay@alaska.edu for questions/problems.\n");
        return 0;
    }
    
    /* open datasets..*/
    in_Dataset = GDAL_open( argv[1]);
    
    /* add mask.. */
    GDALCreateDatasetMaskBand( in_Dataset, 0);
    
    /* Basic info on source dataset..*/
    GDALGetBlockSize(GDALGetRasterBand( in_Dataset, 1 ) , &nBlockXSize, &nBlockYSize );
    printf( "Block=%dx%d Type=%s, ColorInterp=%s\n",
                nBlockXSize, nBlockYSize,
                GDALGetDataTypeName(GDALGetRasterDataType( GDALGetRasterBand( in_Dataset, 1 ))),
                GDALGetColorInterpretationName(
                    GDALGetRasterColorInterpretation(GDALGetRasterBand( in_Dataset, 1 ))));
    
    /* Loop though bands, wiping values with mask values of 0.. */
    xsize = GDALGetRasterXSize( in_Dataset );
    mask_scan_line = (char *) CPLMalloc(sizeof(char)*xsize);
    data_scan_line = (char *) CPLMalloc(sizeof(char)*xsize);
    for (bands=1; bands <= GDALGetRasterCount( in_Dataset ); bands ++ ) {
        int x;
        GDALRasterBandH data_band, out_band;
        int y_index = 0;
        
        printf("INFO: Doing band %d of %d\n", bands,GDALGetRasterCount( in_Dataset ) );
        data_band =  GDALGetRasterBand( in_Dataset, bands);
        mask_band = GDALGetMaskBand(data_band);
        for (y_index = 0; y_index <GDALGetRasterYSize( in_Dataset ); y_index ++ ) {
            /* Read data..*/
            GDALRasterIO( data_band, GF_Read, 0, y_index, xsize , 1, data_scan_line, xsize , 1, GDT_Byte, 0, 0 );
            
            for(x=0; x < xsize; x++) {
                /* if mask is set to 0, then mask off...*/
                /* lame nodata handleing, but such is life.. */
                if ( data_scan_line[x] == 0 )
                    mask_scan_line[x]=0;
                else
                    mask_scan_line[x]=255;
            }
            
            /* now write out band..*/
            GDALRasterIO( mask_band, GF_Write, 0, y_index, xsize , 1, mask_scan_line, xsize , 1, GDT_Byte, 0, 0 );
        }
        
    }
    
    GDALClose(in_Dataset);
}
