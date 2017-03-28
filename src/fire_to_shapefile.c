/****************************************
* Originally from Dan S. and Kevin E. 
* Makes shapefiles from modis mod14 hdf files. 
*****************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libshp/shapefil.h>

void usage(char **argv) {
	fprintf(stderr, "usage: %s [-a] in out\n", argv[0]);
	fprintf(stderr, "	-a	append to existing shapefile\n");
	fprintf(stderr, "	in	name of input firepoints file\n");
	fprintf(stderr, "	out	basename of output shapefile\n");
	exit(-1);
}

int main(int argc, char **argv) {
	char *fn_in;
	char *fn_out;
	int append;

	FILE *fin;
	SHPHandle shp_handle;
	SHPObject *shape;
	int shape_id;
	DBFHandle dbf_handle;
	int therm_field;
	int time_field;
	int platform_field;

	if(argc == 3) {
		append = 0;
		fn_in = argv[1];
		fn_out = argv[2];
	} else if(argc == 4) {
		if(strcmp(argv[1], "-a")) usage(argv);
		append = 1;
		fn_in = argv[2];
		fn_out = argv[3];
	} else usage(argv);

	fin = fopen(fn_in, "r");
	if(!fin) {
		fprintf(stderr, "cannot open input\n");
		return -1;
	}

	if(append) {
		shp_handle = SHPOpen(fn_out, "rb+");
		if(!shp_handle) {
			fprintf(stderr, "cannot open output shapefile\n");
			return -1;
		}

		dbf_handle = DBFOpen(fn_out, "rb+");
		if(!dbf_handle) {
			fprintf(stderr, "cannot open output dbf\n");
			return -1;
		}

		therm_field = DBFGetFieldIndex(dbf_handle, "TEMP");
		time_field = DBFGetFieldIndex(dbf_handle, "TIME");
		platform_field = DBFGetFieldIndex(dbf_handle, "PLATFORM");
		if(therm_field == -1 || time_field == -1 || platform_field == -1) {
			fprintf(stderr, "dbf file does not have all required fields\n");
			return -1;
		}
	} else {
		shp_handle = SHPCreate(fn_out, SHPT_POINT);
		if(!shp_handle) {
			fprintf(stderr, "cannot create output shapefile\n");
			return -1;
		}

		dbf_handle = DBFCreate(fn_out);
		if(!dbf_handle) {
			fprintf(stderr, "cannot create output dbf\n");
			return -1;
		}

		therm_field = DBFAddField(dbf_handle, "TEMP", FTDouble, 10, 2);
		time_field = DBFAddField(dbf_handle, "TIME", FTString, 20, 0);
		platform_field = DBFAddField(dbf_handle, "PLATFORM", FTString, 5, 0);
		if(therm_field == -1 || time_field == -1 || platform_field == -1) {
			fprintf(stderr, "could not create dbf fields\n");
			return -1;
		}
	}

	for(;;) {
		double lon, lat, therm;
		int mon, day, year, hhmm;
		char platform_id;
		int n;
		n = fscanf(fin, "%lf,%lf,%lf,%*f,%*f,%d/%d/%d,%d,%c,%*d\n", &lat, &lon, &therm, &mon, &day, &year, &hhmm, &platform_id);
		if(n<0) break;
		if(n != 8) {
			fprintf(stderr, "could not parse line in input (read %d fields)\n", n);
			return -1;
		}

		char timestring[1000];
		sprintf(timestring, "%02d/%02d/%02d %02d:%02d UTC", mon, day, year, hhmm/100, hhmm%100);

		char *platform_label;
		if(platform_id == 'T') platform_label = "Terra";
		else if(platform_id == 'A') platform_label = "Aqua";
		else {
			fprintf(stderr, "unknown platform code: %c\n", platform_id);
			return -1;
		}

		shape = SHPCreateObject(
			SHPT_POINT, -1, 0, NULL, NULL,
			1, &lon, &lat, NULL, NULL);
		shape_id = SHPWriteObject(shp_handle, -1, shape);
		SHPDestroyObject(shape);

		DBFWriteDoubleAttribute(dbf_handle, shape_id, therm_field, therm);
		DBFWriteStringAttribute(dbf_handle, shape_id, time_field, timestring);
		DBFWriteStringAttribute(dbf_handle, shape_id, platform_field, platform_label);
	}

	SHPClose(shp_handle);
	DBFClose(dbf_handle);
	fclose(fin);

	return 0;
}
