#ifndef  __RPIINFO_H
#define  __RPIINFO_H

#include <stdint.h>

#define CELSIUS       0
#define FAHRENHEIT    1
#define TEMPERATURE_TYPE  CELSIUS
/*
 * The operating temperature for a
 * Raspberry Pi is between 0°C and 85°C. Specifically, the CPU is qualified from -40°C to
 * 85°C and the LAN is qualified from 0°C to 70°C.
 */
#define TEMP_WARNING  50	// 122F
#define TEMP_CRITICAL 85	// 185F

#define ETH0_ADDRESS    0
#define WLAN0_ADDRESS   1
#define IPADDRESS_TYPE ETH0_ADDRESS //  or WLAN0_ADDRESS


#define IP_DISPLAY_OPEN     0
#define IP_DISPLAY_CLOSE    1
#define IP_SWITCH       IP_DISPLAY_OPEN
#define CUSTOM_DISPLAY   "UCTRONICS"

#define CPU_LOAD_WARNING 90
#define CPU_LOAD_CRITICAL 95

#define MEMORY_WARNING 80
#define MEMORY_CRITICAL 90


char* get_ip_address(void);
void get_sd_memory(uint32_t *MemSize, uint32_t *freesize);
void get_cpu_memory(float *Totalram, float *freeram);
uint8_t get_temperature(void);
uint8_t get_cpu_message(void);
uint8_t get_hard_disk_memory(uint16_t *diskMemSize, uint16_t *useMemSize);

#endif /*__RPIINFO_H*/
