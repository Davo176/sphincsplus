
//
//  PQCgenKAT_sign.c
//
//  Modified from a file created by Bassham, Lawrence E (Fed) on 8/29/17.
//  Copyright © 2017 Bassham, Lawrence E (Fed). All rights reserved.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "rng.h"
#include "api.h"

#define	MAX_MARKER_LEN		50

#define KAT_SUCCESS          0
#define KAT_FILE_OPEN_ERROR -1
#define KAT_DATA_ERROR      -3
#define KAT_CRYPTO_FAILURE  -4

int		FindMarker(FILE *infile, const char *marker);
int		ReadHex(FILE *infile, unsigned char *A, int Length, char *str);
void	fprintBstr(FILE *fp, char *S, unsigned char *A, unsigned long long L);

char    AlgName[] = "My Alg Name";

int
main()
{
    char                fn_req[50], fn_rsp[32];
    FILE                *fp_req, *fp_rsp;
    unsigned char       seed[48];
    unsigned char       msg[3300];
    unsigned char       entropy_input[48];
    unsigned char       *m, *sm, *m1;
    unsigned long long  mlen, smlen, mlen1;
    int                 count;
    int                 done;
    unsigned char       pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
    int                 ret_val;

    // Create the REQUEST file
    sprintf(fn_req, "signed_java_%s-robust.rsp", EXPORTINFO); //should be changed from _ref_ to _java_ or _csharp_

    for (int i=0; i<48; i++)
        entropy_input[i] = (unsigned char)i;

    //Create the RESPONSE file based on what's in the REQUEST file
    if ( (fp_req = fopen(fn_req, "r")) == NULL ) {
        printf("Couldn't open <%s> for read\n", fn_req);
        return KAT_FILE_OPEN_ERROR;
    }

    if ( (fp_rsp = fopen("Will.txt", "w")) == NULL ) {
        printf("Couldn't open <%s> for read\n", fn_req);
        return KAT_FILE_OPEN_ERROR;
    }
    done = 0;
    do {
        if ( FindMarker(fp_req, "count = ") )
            ret_val = fscanf(fp_req, "%d", &count);
        else {
            done = 1;
            break;
        }

        randombytes_init(seed, NULL);

        if ( !ReadHex(fp_req, pk, CRYPTO_PUBLICKEYBYTES, "pk = ") ) {
            printf("ERROR: unable to read 'pk' from <%s>\n", fn_req);
            return KAT_DATA_ERROR;
        }

        if ( FindMarker(fp_req, "mlen = ") )
            ret_val = fscanf(fp_req, "%llu", &mlen);
        else {
            printf("ERROR: unable to read 'mlen' from <%s>\n", fn_req);
            return KAT_DATA_ERROR;
        }

        m = (unsigned char *)calloc(mlen, sizeof(unsigned char));
        m1 = (unsigned char *)calloc(mlen+CRYPTO_BYTES, sizeof(unsigned char));
        sm = (unsigned char *)calloc(mlen+CRYPTO_BYTES, sizeof(unsigned char));

        if ( !ReadHex(fp_req, m, (int)mlen, "msg = ") ) {
            printf("ERROR: unable to read 'msg' from <%s>\n", fn_req);
            return KAT_DATA_ERROR;
        }

        

        if ( FindMarker(fp_req, "smlen = ") )
            ret_val = fscanf(fp_req, "%llu", &smlen);
        else {
            printf("ERROR: unable to read 'smlen' from <%s>\n", fn_req);
            return KAT_DATA_ERROR;
        }

        if ( !ReadHex(fp_req, sm, (int)smlen, "sm = ") ) {
            printf("ERROR: unable to read 'sm' from <%s>\n", fn_req);
            return KAT_DATA_ERROR;
        }

        if ( (ret_val = crypto_sign_open(m1, &mlen1, sm, smlen, pk)) != 0) {
            printf("crypto_sign_open returned <%d>\n", ret_val);
            return KAT_CRYPTO_FAILURE;
        }

        if ( mlen != mlen1 ) {
            printf("crypto_sign_open returned bad 'mlen': Got <%llu>, expected <%llu>\n", mlen1, mlen);
            return KAT_CRYPTO_FAILURE;
        }

        if ( memcmp(m, m1, mlen) ) {
            printf("crypto_sign_open returned bad 'm' value\n");
            return KAT_CRYPTO_FAILURE;
        }
        printf("Passed\n");

        free(m);
        free(m1);
        free(sm);

    } while ( !done );

    fclose(fp_req);
    fclose(fp_rsp);

    return KAT_SUCCESS;
}

//
// ALLOW TO READ HEXADECIMAL ENTRY (KEYS, DATA, TEXT, etc.)
//
int
FindMarker(FILE *infile, const char *marker)
{
	char	line[MAX_MARKER_LEN];
	size_t		i, len;
	int curr_line;

	len = strlen(marker);
	if ( len > MAX_MARKER_LEN-1 )
		len = MAX_MARKER_LEN-1;

	for ( i=0; i<len; i++ )
	  {
	    curr_line = fgetc(infile);
	    line[i] = (char)curr_line;
	    if (curr_line == EOF )
	      return 0;
	  }
	line[len] = '\0';

	while ( 1 ) {
		if ( !strncmp(line, marker, len) )
			return 1;

		for ( i=0; i<len-1; i++ )
			line[i] = line[i+1];
		curr_line = fgetc(infile);
		line[len-1] = (char)curr_line;
		if (curr_line == EOF )
		    return 0;
		line[len] = '\0';
	}

	// shouldn't get here
	return 0;
}

//
// ALLOW TO READ HEXADECIMAL ENTRY (KEYS, DATA, TEXT, etc.)
//
int
ReadHex(FILE *infile, unsigned char *A, int Length, char *str)
{
	int			i, ch, started;
	unsigned char	ich;

	if ( Length == 0 ) {
		A[0] = 0x00;
		return 1;
	}
	memset(A, 0x00, (size_t)Length);
	started = 0;
	if ( FindMarker(infile, str) )
		while ( (ch = fgetc(infile)) != EOF ) {
			if ( !isxdigit(ch) ) {
				if ( !started ) {
					if ( ch == '\n' )
						break;
					else
						continue;
				}
				else
					break;
			}
			started = 1;
			if ( (ch >= '0') && (ch <= '9') )
				ich = (unsigned char)(ch - '0');
			else if ( (ch >= 'A') && (ch <= 'F') )
				ich = (unsigned char)(ch - 'A' + 10);
			else if ( (ch >= 'a') && (ch <= 'f') )
				ich = (unsigned char)(ch - 'a' + 10);
            else // shouldn't ever get here
                ich = 0;

			for ( i=0; i<Length-1; i++ )
				A[i] = (A[i] << 4) | (A[i+1] >> 4);
			A[Length-1] = (A[Length-1] << 4) | ich;
		}
	else
		return 0;

	return 1;
}

void
fprintBstr(FILE *fp, char *S, unsigned char *A, unsigned long long L)
{
	unsigned long long  i;

	fprintf(fp, "%s", S);

	for ( i=0; i<L; i++ )
		fprintf(fp, "%02X", A[i]);

	if ( L == 0 )
		fprintf(fp, "00");

	fprintf(fp, "\n");
}

