#include <Arduino.h>
#include <SPIFFS.h>
#include "main_config.h"

long read_config_long(uint16_t index, long def)
{
  File fp = SPIFFS.open(CONFIG_FNAME, FILE_READ);
  if( !fp )
    return def;
  
  size_t fsize = fp.size();
  if( fsize < (index + 1) * sizeof(long) )
    return def;

  bool ret = fp.seek(index * sizeof(long));
  if( !ret ){
    fp.close();
    return def;
  }

  long value;
  if( fp.read((uint8_t*)&value, sizeof(long)) != sizeof(long) ){
    fp.close();
    return def;
  }
  fp.close();

  return value;
}

long write_config_long(uint16_t index, long value)
{
  File fp = SPIFFS.open(CONFIG_FNAME, FILE_WRITE);
  if( !fp )
    return -1;
  
  size_t fsize = fp.size();
  long temp = 0;
  if( fsize < index * sizeof(long) ){
    fp.seek(fsize / sizeof(long) * sizeof(long));
    for( int i = fsize / sizeof(long) ; i < index ; i++ )
      fp.write((uint8_t*)&temp, sizeof(long));
  }

  if( fp.write((uint8_t*)&value, sizeof(long)) != sizeof(long) ){
    fp.close();
    return -1;
  }
  fp.close();

  return 0;
}

String read_config_string(const char *fname)
{
  File fp = SPIFFS.open(fname, FILE_READ);
  if( !fp )
    return String("");

  String text = fp.readString();
  fp.close();

  return text;
}

long write_config_string(const char *fname, const char *text)
{
  File fp = SPIFFS.open(fname, FILE_WRITE);
  if( !fp )
    return -1;

  long ret = fp.write((uint8_t*)text, strlen(text));
  fp.close();
  if( ret != strlen(text) )
    return -1;

  return 0;
}
