// Copyright (C) 2007 Stefan Vigerske
// All Rights Reserved.
// This code is published under the Common Public License.
//
// Author: Stefan Vigerske

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//GDX client
#include "gdxcc.h"

gdxHandle_t gdxhandle=NULL;
char buffer[1024];

void gdx_error(int n) {
//	int n = GDXGetLastError(gdxio);
	if (!gdxErrorStr(gdxhandle, n, buffer))
		fprintf(stderr, "Error %d in GDX I/O library. Cannot retrieve error message.\n", n);
	else
		fprintf(stderr, "Error %d in GDX I/O library: %s.\n", n, buffer);
	exit(EXIT_FAILURE);
}


int main(int argc, char** argv) {
	int errornr;
	int nrsy=0, nruel=0;
	int dim, type;
	int recCount;
	char** Elements; // symbol names
	double* Values; // symbol values
	int afdim; // mostly 1, I think.
	int i,j,k;

	if (argc==0)
		exit(EXIT_FAILURE);
	if (argc==1) {
		fprintf(stderr, "Usage: %s <gdx-file>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// init gdxio library
	if (!gdxCreate(&gdxhandle, buffer, 1024)) {
		fprintf(stderr, "Error loading GDX I/O library: %s\n", buffer);
		exit(EXIT_FAILURE);
	}

	gdxOpenRead(gdxhandle, argv[1], &errornr);
	if (errornr) gdx_error(errornr);

	if (gdxSystemInfo(gdxhandle, &nrsy, &nruel)==0) {
		fprintf(stderr, "Error retrieving symbol infos.\n");
		exit(EXIT_FAILURE);
	}

	Elements=(char**)malloc(10*sizeof(char*));
	for (i=0; i<10; ++i) Elements[i]=(char*)malloc(32*sizeof(char));
	Elements[1][0]=0;
	Values=(double*)malloc(10*sizeof(double));
	for (i=1; i<=nrsy; ++i) {
		gdxSymbolInfo(gdxhandle, i, buffer, &dim, &type);
		if (type!=dt_var) continue;

		gdxDataReadStrStart(gdxhandle, i, &recCount);

		for (j=1;  j<=recCount; j++) {
			int binary;
			gdxDataReadStr(gdxhandle, Elements, Values, &afdim);
			binary=!strncmp(buffer, "y_", 2);
			printf(buffer);
			if (binary) printf(".fx");
			else printf(".l");
			if (recCount>1) {
				printf("(");
				for (k=0; k<dim; k++) {
					printf("'%s'", Elements[k]);
					if (k<dim-1) printf(",");
				}
				printf(")");
			}
			printf(" = ");
			if (binary)
				printf("%g", (Values[0]<0.5 ? 0. : 1.));
			else
				printf("%.20g", Values[0]);
			printf(";\n");
		}

		gdxDataReadDone(gdxhandle);
	}
	for (i=0; i<10; ++i) free(Elements[i]);
	free(Elements);
	free(Values);
	
	gdxClose(gdxhandle);
	gdxFree(&gdxhandle);

  return EXIT_SUCCESS;
}
