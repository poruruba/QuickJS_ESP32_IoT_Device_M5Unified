#include <Arduino.h>
#include <LittleFS.h>
#include "main_config.h"

long read_config_long(uint16_t index, long def)
{
  if( !LittleFS.exists(CONFIG_FNAME) ){
    File fp = LittleFS.open(CONFIG_FNAME, FILE_WRITE);
    if( fp )
      fp.close();
    return def;
  }

  File fp = LittleFS.open(CONFIG_FNAME, FILE_READ);
  if( !fp )
    return def;
  
  size_t fsize = fp.size();
  if( fsize < (index + 1) * sizeof(long) ){
    fp.close();
    return def;
  }

  fp.seek(index * sizeof(long));
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
  File fp = LittleFS.open(CONFIG_FNAME, "a+");
  if (!fp)
    return -1;
  
  size_t fsize = fp.size();
  if( fsize < index * sizeof(long) ){
    fp.seek(fsize / sizeof(long) * sizeof(long));
    const long temp = 0;
    for( int i = fsize / sizeof(long) ; i < index ; i++ )
      fp.write((uint8_t*)&temp, sizeof(long));
  }

  fp.seek(index * sizeof(long));
  if( fp.write((uint8_t*)&value, sizeof(long)) != sizeof(long) ){
    fp.close();
    return -1;
  }
  fp.close();

  return 0;
}

String read_config_string(const char *fname)
{
  if( !LittleFS.exists(fname) ){
    File fp = LittleFS.open(fname, FILE_WRITE);
    if( fp )
      fp.close();
    return String("");
  }

  File fp = LittleFS.open(fname, FILE_READ);
  if( !fp )
    return String("");

  String text = fp.readString();
  fp.close();

  return String(text.c_str());
}

long write_config_string(const char *fname, const char *text)
{
  File fp = LittleFS.open(fname, FILE_WRITE);
  if( !fp )
    return -1;

  long ret = fp.write((uint8_t*)text, strlen(text));
  fp.close();
  if( ret != strlen(text) )
    return -1;

  return 0;
}