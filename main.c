/*
    Program name: bin2sim
    Brief: Converts a firmware binary (.bin) file into an IAR Simple Code .sim file.
    Usage: bin2sim [FILE IN] [FILE OUT] [OPTION]
    Author: Mikael Stewart
    Date: 10 April 2018
    Version: 1.0
    Repository: https://github.com/mmmstew/bin2sim

    References:
        http://netstorage.iar.com/SuppDB/Public/UPDINFO/006220/simple_code.htm
*/

#include <stdio.h>
#include <stdlib.h>

#define HEADER_SIZE_BYTES 14

unsigned int GetFileSize(FILE* fp)
{
    fseek(fp, 0, SEEK_END);
    return ftell(fp);
}

int WriteHeader(FILE * fout, unsigned int program_size)
{
    size_t rb;
    unsigned char header[HEADER_SIZE_BYTES] = {0x7f, 0x49, 0x41, 0x52, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    header[8] = (program_size&0xFF000000)>>24;
    header[9] = (program_size&0x00FF0000)>>16;
    header[10] = (program_size&0x0000FF00)>>8;
    header[11] = (program_size&0x000000FF)>>0;

    fseek(fout, 0, SEEK_SET);
    rb = fwrite(header, 1, HEADER_SIZE_BYTES, fout);

    if (rb != HEADER_SIZE_BYTES)
    {
        printf("\r\nCould not read header.\r\n");
        return 1;
    }

    return 0;
}

int WriteDataRecord(FILE * fin, FILE * fout, unsigned int start_address)
{
    // binary file goes in one data record
    // we cannot split into multiple records to save space as this information is not contained in a bin file

    int i;
    unsigned char fileByte;
    unsigned int number_of_program_bytes = GetFileSize(fin);
    unsigned char record[12] = {0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    fseek(fin, 0, SEEK_SET);
    fseek(fout, HEADER_SIZE_BYTES, SEEK_SET);

    // write data record entry
    record[4] = (start_address&0xFF000000)>>24;
    record[5] = (start_address&0x00FF0000)>>16;
    record[6] = (start_address&0x0000FF00)>>8;
    record[7] = (start_address&0x000000FF)>>0;
    record[8] = (number_of_program_bytes&0xFF000000)>>24;
    record[9] = (number_of_program_bytes&0x00FF0000)>>16;
    record[10] = (number_of_program_bytes&0x0000FF00)>>8;
    record[11] = (number_of_program_bytes&0x000000FF)>>0;
    if (fwrite(record, 1, 12, fout) == 0)
    {
        printf("\r\nCould not write to output file.\r\n");
        return 1;
    }

    // write program bytes
    for (i=0;i<number_of_program_bytes;i++)
    {
        if (fread(&fileByte, 1, 1, fin) == 0)
        {
            printf("\r\nCould not read input file.\r\n");
            return 1;
        }
        if (fwrite(&fileByte, 1, 1, fout) == 0)
        {
            printf("\r\nCould not write to output file.\r\n");
            return 1;
        }

    }
    return 0;
}

unsigned int CalculateChecksum(FILE* fp)
{
    unsigned int checksum = 0;  // 4 byte quantity
    unsigned char fileByte;

    fseek(fp, 0, SEEK_SET);
    while (fread(&fileByte, 1, 1, fp))
    {
        checksum += fileByte;
    }
    checksum = ~checksum +1;

    printf("Calculated checksum = 0x%08x\r\n", checksum);
    return checksum;
}

int WriteEndRecord(FILE * fout)
{
    unsigned char record[5] = {0x03, 0x00, 0x00, 0x00, 0x00};
    unsigned int foutSize = GetFileSize(fout);
    unsigned int checksum;

    // write record entry
    fseek(fout, foutSize, SEEK_SET);
    if (fwrite(record, 1, 1, fout) == 0)
    {
        printf("\r\nCould not write to output file.\r\n");
        return 1;
    }

    // compute checksum of fout now, then append to fout
    checksum = CalculateChecksum(fout);
    record[1] = (checksum&0xFF000000)>>24;
    record[2] = (checksum&0x00FF0000)>>16;
    record[3] = (checksum&0x0000FF00)>>8;
    record[4] = (checksum&0x000000FF)>>0;
    if (fwrite(record+1, 1, 4, fout) == 0)
    {
        printf("\r\nCould not write to output file.\r\n");
        return 1;
    }

    return 0;
}

int main (int argc, char *argv[])
{
    unsigned int start_address = 0;
    FILE * fin = NULL;
    FILE * fout = NULL;

    if (argc != 3 && argc != 5)
    {
        goto paramError;
    }

    fin = fopen(argv[1], "rb");
    if (fin == NULL)
    {
        printf("Could not open input file.\r\n");
        return 1;
    }

    fout = fopen(argv[2], "w+b");
    if (fout == NULL)
    {
        printf("Could not open output file.\r\n");
        return 1;
    }

    if (argc == 5)
    {
        if (argv[3][0] == '-')
        {
            if (argv[3][1] == 's')
            {
                start_address = strtoul(argv[4], NULL, 10);
            }
            else
            {
                goto paramError;
            }
        }
        else
        {
            goto paramError;
        }
    }

    unsigned int program_size = GetFileSize(fin);   // binary file size is the program size
    if (WriteHeader(fout, program_size))
        goto close;
    if (WriteDataRecord(fin, fout, start_address))
        goto close;
    if (WriteEndRecord(fout))
        goto close;

close:
    fclose(fin);
    fclose(fout);
    return 0;

paramError:
    printf("Usage: bin2sim [FILE IN] [FILE OUT] [OPTION]\r\n");
    printf("Options:\r\n");
    printf("  -s [start address]        decimal address where binary data should be written (default 0).\r\n");
    return 1;
}
