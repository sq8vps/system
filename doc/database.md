# Nabla System Configuration Database Format

All multi-byte fields are stored least significant byte first.

## Magic Word
`NABLADB`

## Branch entry

* Identifier: `0x01`
* Entry count (32 bits) - number of child entries
* Name length (32 bits) of the *Name* field in bytes, including terminator
* Name - UTF-8 string with terminator


## Value entry

* Identifier (dependent on data type):
    - Byte (8 bits): `0x02`
    - Word (16 bits): `0x03`
    - Doubleword (32 bits): `0x04`
    - Quadword (64 bits): `0x05`
    - Boolean (8 bits): `0x06`
    - UTF-8 string (n * 8 bits): `0x07`
    - UNIX timestamp (64 bits): `0x08`
    - UUID (128 bits): `0x09`
    - IEEE 754 single precision float (32 bits): `0x0A`
    - IEEE 754 double precision float (64 bits): `0x0B`
    - Multibyte (n * 8 bits): `0x0C` - e.g. for numbers longer than 128 bits

* Name length (32 bits) of the *Name* field in bytes, including terminator
* Name - UTF-8 string with terminator
* Field length (32 bits) in bytes - only in *UTF-8 string* (including terminator) and *Multibyte* values 
* Value (with terminator in *UTF-8 string*)