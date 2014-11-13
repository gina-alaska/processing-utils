#include "gdal.h"
#include "cpl_conv.h" /* for CPLMalloc() */
#include "ogr_core.h"
#include "ogr_srs_api.h"
/*Compile like: gcc $(gdal-config --cflags) -o get_gcp get_gcp.c $(gdal-config --libs)*/


int main(int argc, char **argv)
{
    GDALDatasetH  hDataset;

    GDALAllRegister();

    hDataset = GDALOpen( argv[1], GA_ReadOnly );
    if( hDataset != NULL )
    {
	int a;
	GDAL_GCP* gcp_list;
 	OGRSpatialReferenceH 	gcp_hSRS, output_hSRS;
	OGRCoordinateTransformationH	trans;
	char * gcp_proj, *proj4_proj;
	
	gcp_list = GDALGetGCPs(hDataset);

	/* Get GCP projection.. */
	
        gcp_proj = (char *)GDALGetGCPProjection(hDataset);

        /* Get Projection .. */
	gcp_hSRS = OSRNewSpatialReference(NULL);
	output_hSRS = OSRNewSpatialReference(NULL);
	OSRSetWellKnownGeogCS(output_hSRS, "WGS84");
        OSRImportFromWkt(gcp_hSRS,&gcp_proj);
	OSRExportToProj4(gcp_hSRS,&proj4_proj);
	trans=OCTNewCoordinateTransformation(gcp_hSRS,output_hSRS);

	/*Something like: "--- \n- a: 2\n  b: 4\n- 2\n- 3\n- 4\n" */
	printf("--- \n");
	for (a=0; a<GDALGetGCPCount(hDataset); a++ ) {
		printf("- x: %g\n", gcp_list[a].dfGCPPixel);
		printf("  y: %g\n", gcp_list[a].dfGCPLine);
                printf("  gx: %15.5f\n", gcp_list[a].dfGCPX);
                printf("  gy: %15.5f\n", gcp_list[a].dfGCPY);
		printf("  name: '%s'\n", gcp_list[a].pszId);
		printf("  info: '%s'\n", gcp_list[a].pszInfo);
		printf("  id: %d\n", a);
		printf("  proj: \'%s\'\n",  GDALGetGCPProjection(hDataset));
		printf("  proj4: \'%s\'\n",  proj4_proj);

		OCTTransform(trans,1, &(gcp_list[a].dfGCPX), &(gcp_list[a].dfGCPY), NULL);
                printf("  lon: %15.5f\n", gcp_list[a].dfGCPX);
                printf("  lat: %15.5f\n", gcp_list[a].dfGCPY);

	};
    }
    else {

		fprintf(stderr, "ERROR: could not open \"%s\"\n", argv[1]);
 	}

}
