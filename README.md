Command line tool to convert a firmware binary (.bin) file into an IAR Simple Code .sim file.

The Simple Code format is defined here:

http://netstorage.iar.com/SuppDB/Public/UPDINFO/006220/simple_code.htm

This program places the input bin file (a contiguous binary blob) within a single data record of a sim file.

Memory region information does not exist in a bin file to allow creation of multiple records which means that memory regions spaced far apart can potentially contain a lot of unnecessary padding in the sim file as they would in the bin file.

An optional start address in can be specified. This is where the binary blob will be located.

Usage: bin2sim [FILE IN] [FILE OUT] [OPTION]
Options:
  -s [start address]        decimal address where binary data should be written (default 0).

Originally compiled with Mingw C compiler on Windows 10.