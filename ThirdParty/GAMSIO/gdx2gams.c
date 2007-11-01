#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//GDX client
#include "gdxwrap.h"

void gdx_error(int n) {
//	int n = GDXGetLastError(gdxio);
	char* msg;
	if (!GDXErrorStr(n, &msg))
		fprintf(stderr, "Error %d in GDX I/O library. Cannot retrieve error message.\n", n);
	else
		fprintf(stderr, "Error %d in GDX I/O library: %s.\n", n, msg);
	exit(EXIT_FAILURE);
}


int main(int argc, char** argv) {
	PGXFile gdxhandle=NULL;
	int errornr;
	int nrsy=0, nruel=0;
	int dim, type;
	int recCount;
	TgdxStrIndex Elements; // symbol names
	TgdxValues Values; // symbol values
	int afdim; // mostly 1, I think.
	int i,j,k;
	char buffer[1024];

	if (argc==0)
		exit(EXIT_FAILURE);
	if (argc==1) {
		fprintf(stderr, "Usage: %s <gdx-file>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// init gdxio library
	if (gdxWrapInit(NULL, buffer, 1024)) {
		fprintf(stderr, "Error loading GDX I/O library: %s\n", buffer);
		exit(EXIT_FAILURE);
	}

	errornr=GDXOpenRead(&gdxhandle, argv[1]);
	if (errornr) gdx_error(errornr);

	if (GDXSystemInfo(gdxhandle, &nrsy, &nruel)==0) {
		fprintf(stderr, "Error retrieving symbol infos.\n");
		exit(EXIT_FAILURE);
	}

	for (i=0; i<10; ++i) Elements[i]=(char*)malloc(32*sizeof(char));
	Elements[1][0]=0;
	for (i=1; i<=nrsy; ++i) {
		GDXSymbolInfo(gdxhandle, i, buffer, &dim, &type);
		if (type!=dt_var) continue;

		GDXDataReadStrStart(gdxhandle, i, &recCount);

		for (j=1;  j<=recCount; j++) {
			int binary;
			GDXDataReadStr(gdxhandle, Elements, Values, &afdim);
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

		GDXDataReadDone(gdxhandle);
	}
	for (i=0; i<10; ++i) free(Elements[i]);

  return EXIT_SUCCESS;
}
