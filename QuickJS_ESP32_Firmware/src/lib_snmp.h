#ifndef _LIB_SNMP_H_
#define _LIB_SNMP_H_

#define NUM_OF_PRIV_NUMBER      3
#define NUM_OF_PRIV_STRING      3
#define NUM_OF_PRIV_TIMESTAMP   3

#define NUMBER_DEFAULT  INT_MIN
#define PRIVETE_OID_BASE  ".1.3.6.1.4.1.8072.9999"

long snmp_initialize(void);
long snmp_loop(void);
long snmp_set_number(unsigned char index, int value);
long snmp_set_string(unsigned char index, const char *p_str);
long snmp_set_timestamp(unsigned char index, unsigned long value);
long snmp_clear_value(void);

#endif