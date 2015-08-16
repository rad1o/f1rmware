#include "mesh.h"

#include "fs_util.h"

#define BLOCK 512

float atof(char* num)
 {
     if (!num || !*num)
         return 0;
     float integerPart = 0;
     float fractionPart = 0;
     int divisorForFraction = 1;
     int sign = 1;
     uint8_t inFraction = 0;
     while( *num == ' ' )
         ++num;

     /*Take care of +/- sign*/
     if (*num == '-')
     {
         ++num;
         sign = -1;
     }
     else if (*num == '+')
     {
         ++num;
     }
     while (*num != '\0' && *num != ' ' )
     {
         if (*num >= '0' && *num <= '9')
         {
             if (inFraction)
             {
                 /*See how are we converting a character to integer*/
                 fractionPart = fractionPart*10 + (*num - '0');
                 divisorForFraction *= 10;
             }
             else
             {
                 integerPart = integerPart*10 + (*num - '0');
             }
         }
         else if (*num == '.')
         {
             if (inFraction)
                 return sign * (integerPart + fractionPart/divisorForFraction);
             else
                 inFraction = 1;
         }
         else
         {
             return sign * (integerPart + fractionPart/divisorForFraction);
         }
         ++num;
     }
     return sign * (integerPart + fractionPart/divisorForFraction);
 }

int atoi(char *str)
{
    int res = 0; // Initialize result

    while( *str == ' ' )
        ++str;
    // Iterate through all characters of input string and update result
    for (int i = 0; str[i] != '\0' && str[i] != ' '; ++i)
        res = res*10 + str[i] - '0';

    // return result.
    return res;
}

uint8_t isNumber( char c )
{
    return (c == '0' ||
            c == '1' ||
            c == '2' ||
            c == '3' ||
            c == '4' ||
            c == '5' ||
            c == '6' ||
            c == '7' ||
            c == '8' ||
            c == '9' ||
            c == '.' ||
            c == '+' ||
            c == '-' );
}

void addVertex( char* line, float* vertices, int* numVertices )
{
    char number[256];
    int numberCnt = 0;
    while( *line != '\0' )
    {
        if( isNumber( *line ) )
        {
            number[numberCnt] = *line;
            ++numberCnt;
        }
        else
        {
            if( numberCnt > 0 )
            {
                number[numberCnt] = '\0';
                vertices[*numVertices] = atof( number );
                numberCnt = 0;
                ++*numVertices;
            }
        }
        ++line;
    }
    if( numberCnt > 0 )
    {
        number[numberCnt] = '\0';
        vertices[*numVertices] = atof( number );
        numberCnt = 0;
        ++*numVertices;
    }
}

void addFaces( char* line, int* faces, int* numFaces )
{
    char number[256];
    int numberCnt = 0;
    while( *line != '\0' )
    {
        if( isNumber( *line ) )
        {
            number[numberCnt] = *line;
            ++numberCnt;
        }
        else if( *line == '/' )
        {
            if( numberCnt > 0 )
            {
                number[numberCnt] = '\0';
                faces[*numFaces] = atoi( number ) - 1;
                numberCnt = 0;
                ++*numFaces;
            }
        }
        else
        {
            numberCnt = 0;
        }
        ++line;
    }
}

void processLine( char* line, float* vertices, int* numVertices, int* faces, int* numFaces )
{
    while( *line == ' ' ) ++line;
    if( line[0] == 'v' && line[1] == ' ' )
        addVertex( line, vertices, numVertices );
    if( line[0] == 'f' && line[1] == ' ' )
        addFaces( line, faces, numFaces );
}

uint8_t getMeshSizes( char *fname, int* verticeNumb, int* facesNumb )
{
    UINT readbytes;
    uint8_t mesh_buffer[BLOCK];

    FIL file;
    FRESULT res=f_open(&file, fname, FA_OPEN_EXISTING|FA_READ);
    if(res!=FR_OK)
    {
        return 0;
    }

    res=f_read(&file, mesh_buffer, BLOCK, &readbytes);

    if(res!=FR_OK)
    {
        return 0;
    }
    uint8_t firstSign = 1;
    *verticeNumb = 0;
    *facesNumb = 0;

    while( readbytes > 0 )
    {
        for( int i = 0; i < readbytes; ++i )
        {
            if( mesh_buffer[i] == '\n')
            {
                firstSign = 1;
                continue;
            }

            if( firstSign )
            {
                if( mesh_buffer[i] == 'f' )
                    ++(*facesNumb);

                if( mesh_buffer[i] == 'v' )
                    ++(*verticeNumb);

                if( mesh_buffer[i] != ' ' )
                    firstSign = 0;
                continue;
            }
        }

        res=f_read(&file, mesh_buffer, BLOCK, &readbytes);
    }

    return 1;
}

uint8_t getMesh( char *fname, float* vertices, int verticeNumb, int* faces, int facesNumb )
{
    UINT readbytes;
    uint8_t mesh_buffer[BLOCK];

    FIL file;
    FRESULT res=f_open(&file, fname, FA_OPEN_EXISTING|FA_READ);
    if(res!=FR_OK)
    {
        return 0;
    }

    char line[1024];
    int linePos = 0;
    res=f_read(&file, mesh_buffer, BLOCK, &readbytes);

    int numVertices = 0;
    int numFaces = 0;
    while( readbytes > 0 )
    {
        for( int i = 0; i < readbytes; ++i )
        {
            if( mesh_buffer[i] == '\n' )
            {
                line[linePos] = '\0';
                processLine(line,
                            vertices, &numVertices,
                            faces, &numFaces );
                linePos = 0;
                continue;
            }
            line[linePos] = mesh_buffer[i];
            ++linePos;
        }
        res=f_read(&file, mesh_buffer, BLOCK, &readbytes);
    }

    return 1;
}
