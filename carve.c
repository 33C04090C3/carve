/******************************************************************************/
/* SIMPLE FILE CARVING UTILITY                                                */
/******************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_BLOCK_SIZE  4096

// get the size of a stdio file
// NB: does not preserve the file position
bool get_file_size( FILE *f, size_t* s )
{
    size_t size = 0;
    if( f == NULL || s == NULL ) 
    {
        return false;
    }
    if( fseek( f , 0, SEEK_END ) == -1 )
    {
        return false;
    }
    size = (size_t)ftell(f);
    if( size == (size_t)-1 )
    {
        return false;
    }
    if( fseek( f, 0, SEEK_SET ) == -1 )
    {
        return false;
    }
    *s = size;
    return true;
}

void print_usage(void)
{
    printf("Usage: carve <input_file> <offset> <length> <output_filename>\n");
    printf("Offsets and lengths may be specified in hexadecimal by using '0x' in front of the offset.\n");
    printf("Values which do not begin with '0x' will be interpreted as decimal.\n");
    printf("If length is specified as '-' the amount carved will be the remainder of the file from the offset to the end.\n" );
    return;
}

int main( int argc, char *argv[] )
{
    uint8_t  *transferBuf    = NULL;
    int      retVal          = -1;
    FILE     *inputfile      = NULL;
    FILE     *outputfile     = NULL;
    size_t   inputsize       = 0;
    size_t   count           = 0;
    size_t   offset          = 0;
    size_t   length          = 0;
    size_t   block_count     = 0;
    size_t   last_block_size = 0; 

    if( argc < 5 )
    {
        print_usage();
        return 0;
    }

    inputfile = fopen( argv[1],"rb" );
    if( inputfile == NULL )
    {
        printf( "Error: cannot open file %s\n", argv[1] );
        goto end;
    }
    if( get_file_size( inputfile, &inputsize ) == false )
    {
        printf( "Error: cannot get input file size\n" );
        goto end;
    }
    printf("Size of input file %s is %lu bytes\n",argv[1],inputsize);

    if( ( strlen(argv[2]) >= 2 ) && ( argv[2][0] == '0' ) && ( argv[2][1] == 'x' ) )
    {
        offset = strtoul( argv[2], NULL, 16 );
    }
    else 
    {
        offset = strtoul( argv[2], NULL, 10 );
    }

    if( argv[3][0] == '-' && ( offset < inputsize ) )
    {
        length = inputsize - offset;        
    }
    else
    { 
        if( ( strlen(argv[3]) >= 2 ) && ( argv[3][0] == '0' ) && ( argv[3][1] == 'x' ) )
        {
            length = strtoul( argv[3], NULL, 16 );
        }
        else 
        {
            length = strtoul(argv[3], NULL, 10 );
        }
    }
    
    if( length == 0 )
    {
        printf("Error: length cannot be 0!\n");
        goto end;
    }
    // sanity checks
    if(offset > inputsize)
    {
        printf("Error: offset 0x%08lX (%lu) is larger than file '%s' (%lu bytes)\n",
               offset,
               offset,
               argv[1],
               inputsize);
        goto end;
    }
    if((offset + length) > inputsize)
    {
        printf("Error: offset 0x%lX (%lu) plus length %lu is greater than size of '%s'(%lu bytes long)!\n",
               offset,
               offset,
               length,
               argv[1],
               inputsize);
        goto end;
    }
    // there are no entire blocks if the length is less than the size of one block
    block_count = length / DEFAULT_BLOCK_SIZE;
    // there are no partial blocks if the length is exactly divisible by the size of one block
    last_block_size = length % DEFAULT_BLOCK_SIZE;

    transferBuf = (uint8_t*)calloc( DEFAULT_BLOCK_SIZE, sizeof(uint8_t) );
    if( transferBuf == NULL )
    {
        printf( "Cannot allocate transfer buffer\n" );
        goto end;
    }

    // seek to the specified offset
    if( fseek( inputfile, offset, SEEK_SET ) != 0 )
    {     
        printf("Error: could not seek to offset 0x%lX (%lu) in file '%s'\n",offset,offset,argv[1]);
        goto end;
    }
    outputfile = fopen(argv[4],"w+b");
    if(!outputfile)
    {
        printf("Error: could not create output file '%s'\n",argv[4]);
        goto end;
    }

    for( count = 0; count < block_count; count++ )
    {
        if( fread( transferBuf, DEFAULT_BLOCK_SIZE, 1, inputfile ) != 1 )
        {
            printf( "Unable to read block %lu\n", count );
            goto end;
        }
        if( fwrite( transferBuf, DEFAULT_BLOCK_SIZE, 1, outputfile ) != 1 )
        {
            printf( "Unable to write block %lu\n", count );
            goto end;
        }
    }
    if( last_block_size > 0 )
    {
        if( fread( transferBuf, last_block_size, 1, inputfile ) != 1 )
        {
            printf( "Unable to read last block\n" );
            goto end;
        }
        if( fwrite( transferBuf, last_block_size, 1, outputfile ) != 1 )
        {
            printf( "Unable to read last block\n" );
            goto end;
        }
    } 
    
    printf("%lu bytes (%lu blocks) carved from offset 0x%lX in file '%s' and written to file '%s'\n",
           length,
           block_count == 0 ? 1 : block_count,
           offset,
           argv[1],
           argv[4]);

    retVal = 0;

end:
    if( transferBuf != NULL )
    {
        free(transferBuf);
    }
    if( inputfile != NULL )
    {
        fclose(inputfile);
    }
    if( outputfile != NULL )
    {
        fclose(outputfile);
    }
    return retVal;
}
