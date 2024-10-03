# Nabla System Configuration Database Format

All multi-byte fields, except the UUID, are stored least significant byte first.

## Header
* Magic word: `_NABLADB` without terminator (8 bytes)
* Data size: number of bytes occupied by all entries (4 bytes)
* CRC32 of the entire file (4 bytes) - calculated for this field set to 0, with 0xEDB88320 polynomial (reversed notation) and 0xFFFFFFFF initial CRC

## Entry

* Identifier (1 byte, dependent on data type):
	- End tag: `0x00`
	- Null: `0x01` - entry without value
    - Byte (8 bits): `0x02`
    - Word (16 bits): `0x03`
    - Doubleword (32 bits): `0x04`
    - Quadword (64 bits): `0x05`
    - Boolean (8 bits): `0x06`
    - UTF-8 string (n * 8 bits): `0x07`
    - UNIX timestamp (64 bits): `0x08`
    - UUID (128 bits): `0x09` - stored most significant byte first
    - IEEE 754 single precision float (32 bits): `0x0A`
    - IEEE 754 double precision float (64 bits): `0x0B`
    - Multibyte (n * 8 bits): `0x0C` - e.g. for numbers longer than 128 bits

* Name length (32 bits) of the *Name* field in bytes, including terminator
* Data length (32 bits) in bytes - including termiator in *UTF-8 string*
* Name - UTF-8 string with terminator
* Value (with terminator in *UTF-8 string*)

## Array
An array of consecutive entries can be created. To indicate an array, the *Identifier* field must be ORed with `0x80`.
Array elements do not need to be of the same type.
An array consist of the following fields:
* Identifier (1 byte) - `0x80`
* Name length (32 bits) of the *Name* field in bytes, including terminator
* Array element count (32 bits)
* Name - UTF-8 string with terminator
* Payload - for each entry:
	- Identifier (1 byte, dependent on data type) - must be the same for all elements, ORed with `0x80` and `0x40`
	- Data size (32 bits) in bytes - including termiator in *UTF-8 string*
	- Value (with terminator in *UTF-8 string*)

## End tag
The end tag is an entry with type `0x00`, field and name length set to zero, and no name field (i.e., 9 consecutive zeros).