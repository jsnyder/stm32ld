// STM32 loader

#ifndef __STM32LD_H__
#define __STM32LD_H__

#include "type.h"

// Error codes
enum
{
  STM32_OK = 0,
  STM32_PORT_OPEN_ERROR,
  STM32_COMM_ERROR,
  STM32_INIT_ERROR,
  STM32_TIMEOUT_ERROR,
  STM32_NOT_INITIALIZED_ERROR
};

// Communication data
#define STM32_COMM_ACK      0x79
#define STM32_COMM_NACK     0x1F
#define STM32_COMM_TIMEOUT  2000000
#define STM32_WRITE_BUFSIZE 256
#define STM32_FLASH_START_ADDRESS 0x08000000
enum
{
  STM32_CMD_INIT = 0x7F,
  STM32_CMD_GET_COMMAND = 0x00,
  STM32_CMD_ERASE_FLASH = 0x43,
  STM32_CMD_GET_ID = 0x02,
  STM32_CMD_WRITE_FLASH = 0x31,
  STM32_CMD_WRITE_UNPROTECT = 0x73
};

// Function types for stm32_write_flash
typedef u32 ( *p_read_data )( u8 *dst, u32 len );
typedef void ( *p_progress )( u32 wrote );

// Loader functions
int stm32_init( const char* portname, u32 baud );
int stm32_get_bl_version( u8 *major, u8 *minor ); 
int stm32_get_chip_id( u16 *version );
int stm32_write_unprotect();
int stm32_erase_flash();
int stm32_write_flash( p_read_data read_data_func, p_progress progress_func );

#endif

