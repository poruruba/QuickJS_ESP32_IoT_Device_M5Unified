#ifndef _CONFIG_UTILS_H_
#define _CONFIG_UTILS_H_

long read_config_long(uint16_t index, long def);
long write_config_long(uint16_t index, long value);
String read_config_string(const char *fname);
long write_config_string(const char *fname, const char *text);

#endif
