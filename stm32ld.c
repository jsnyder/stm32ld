// STM32 bootloader client

#include <stdio.h>
#include <unistd.h>
#include "serial.h"
#include "type.h"
#include "stm32ld.h"

static ser_handler stm32_ser_id = ( ser_handler )-1;

#define STM32_RETRY_COUNT             10

// ****************************************************************************
// Helper functions and macros

// Check initialization
#define STM32_CHECK_INIT\
  if( stm32_ser_id == ( ser_handler )-1 )\
    return STM32_NOT_INITIALIZED_ERROR

// Check received byte
#define STM32_EXPECT( expected )\
  if( stm32h_read_byte() != expected )\
    return STM32_COMM_ERROR;

#define STM32_READ_AND_CHECK( x )\
  if( ( x = stm32h_read_byte() ) == -1 )\
    return STM32_COMM_ERROR;

// Helper: send a command to the STM32 chip
static int stm32h_send_command( u8 cmd )
{
  ser_write_byte( stm32_ser_id, cmd );
  ser_write_byte( stm32_ser_id, ~cmd );
}

// Helper: read a byte from STM32 with timeout
static int stm32h_read_byte()
{
  return ser_read_byte( stm32_ser_id );
}

// Helper: append a checksum to a packet and send it
static int stm32h_send_packet_with_checksum( u8 *packet, u32 len )
{
  u8 chksum = 0;
  u32 i;

  for( i = 0; i < len; i ++ )
    chksum ^= packet[ i ];
  ser_write( stm32_ser_id, packet, len );
  ser_write_byte( stm32_ser_id, chksum );
  return STM32_OK;
}

// Helper: send an address to STM32
static int stm32h_send_address( u32 address )
{
  u8 addr_buf[ 4 ];

  addr_buf[ 0 ] = address >> 24;
  addr_buf[ 1 ] = ( address >> 16 ) & 0xFF;
  addr_buf[ 2 ] = ( address >> 8 ) & 0xFF;
  addr_buf[ 3 ] = address & 0xFF;
  return stm32h_send_packet_with_checksum( addr_buf, 4 );
}

// Helper: intiate BL communication
static int stm32h_connect_to_bl()
{
  int res;

  // Flush all incoming data
  ser_set_timeout_ms( stm32_ser_id, SER_NO_TIMEOUT );
  while( stm32h_read_byte() != -1 );
  ser_set_timeout_ms( stm32_ser_id, STM32_COMM_TIMEOUT );

  // Initiate communication
  ser_write_byte( stm32_ser_id, STM32_CMD_INIT );
  res = stm32h_read_byte();
  return res == STM32_COMM_ACK || res == STM32_COMM_NACK ? STM32_OK : STM32_INIT_ERROR;
}

// ****************************************************************************
// Implementation of the protocol

int stm32_init( const char *portname, u32 baud )
{
  // Open port
  if( ( stm32_ser_id = ser_open( portname ) ) == ( ser_handler )-1 )
    return STM32_PORT_OPEN_ERROR;

  // Setup port
  ser_setup( stm32_ser_id, baud, SER_DATABITS_8, SER_PARITY_EVEN, SER_STOPBITS_1 );

  // Connect to bootloader
  return stm32h_connect_to_bl();
}

// Get bootloader version
// Expected response: ACK version OPTION1 OPTION2 ACK
int stm32_get_version( u8 *major, u8 *minor )
{
  u8 i, version;
  int temp, total;

  STM32_CHECK_INIT;
  stm32h_send_command( STM32_CMD_GET_COMMAND );
  STM32_EXPECT( STM32_COMM_ACK );
  STM32_READ_AND_CHECK( total );
  for( i = 0; i < total + 1; i ++ )
  {
    STM32_READ_AND_CHECK( temp );
    if( i == 0 )
      version = ( u8 )temp;
  }
  *major = version >> 4;
  *minor = version & 0x0F;
  STM32_EXPECT( STM32_COMM_ACK );
  return STM32_OK;
}

// Get chip ID
int stm32_get_chip_id( u16 *version )
{
  int vh, vl;

  STM32_CHECK_INIT;
  stm32h_send_command( STM32_CMD_GET_ID );
  STM32_EXPECT( STM32_COMM_ACK );
  STM32_EXPECT( 1 );
  STM32_READ_AND_CHECK( vh );
  STM32_READ_AND_CHECK( vl );
  STM32_EXPECT( STM32_COMM_ACK );
  *version = ( ( u16 )vh << 8 ) | ( u16 )vl;
  return STM32_OK;
}

// Write unprotect
int stm32_write_unprotect()
{
  STM32_CHECK_INIT;
  stm32h_send_command( STM32_CMD_WRITE_UNPROTECT );
  STM32_EXPECT( STM32_COMM_ACK );
  STM32_EXPECT( STM32_COMM_ACK );
  // At this point the system got a reset, so we need to re-enter BL mode
  usleep( 200000 );
  return stm32h_connect_to_bl();
}

// Erase flash
int stm32_erase_flash()
{
  STM32_CHECK_INIT;
  stm32h_send_command( STM32_CMD_ERASE_FLASH );
  STM32_EXPECT( STM32_COMM_ACK );
  ser_write_byte( stm32_ser_id, 0xFF );
  ser_write_byte( stm32_ser_id, 0x00 );
  STM32_EXPECT( STM32_COMM_ACK );
  return STM32_OK;
}
    
// Extended erase flash
int stm32_extended_erase_flash()
{
  STM32_CHECK_INIT;
  stm32h_send_command( STM32_CMD_EXTENDED_ERASE_FLASH );
  STM32_EXPECT( STM32_COMM_ACK );
  ser_write_byte( stm32_ser_id, 0xFF );
  ser_write_byte( stm32_ser_id, 0xFF );
  ser_write_byte( stm32_ser_id, 0x00 );
  ser_set_timeout_ms( stm32_ser_id, SER_INF_TIMEOUT );
  STM32_EXPECT( STM32_COMM_ACK );
  return STM32_OK;
}
    

// Program flash
// Requires pointers to two functions: get data and progress report
int stm32_write_flash( p_read_data read_data_func, p_progress progress_func )
{
  u32 wrote = 0;
  u8 data[ STM32_WRITE_BUFSIZE + 1 ];
  u32 datalen, address = STM32_FLASH_START_ADDRESS;

  STM32_CHECK_INIT; 
  while( 1 )
  {
    // Read data to program
    if( ( datalen = read_data_func( data + 1, STM32_WRITE_BUFSIZE ) ) == 0 )
      break;
    data[ 0 ] = ( u8 )( datalen - 1 );

    // Send write request
    stm32h_send_command( STM32_CMD_WRITE_FLASH );
    STM32_EXPECT( STM32_COMM_ACK );
    
    // Send address
    stm32h_send_address( address );
    STM32_EXPECT( STM32_COMM_ACK );

    // Send data
    stm32h_send_packet_with_checksum( data, datalen + 1 );
    STM32_EXPECT( STM32_COMM_ACK );

    // Call progress function (if provided)
    wrote += datalen;
    if( progress_func )
      progress_func( wrote );

    // Advance to next data
    address += datalen;
  }
  return STM32_OK;
}

// "Go" command - Description in: AN3155 Application note
int stm32_go_command( void )
{
  u32 address = STM32_FLASH_START_ADDRESS; // TODO: get address as optional parameter from command line

  STM32_CHECK_INIT;

  // Send "Go" request
  stm32h_send_command( STM32_CMD_GO );
  STM32_EXPECT( STM32_COMM_ACK );

  // Send address to Jump & Run
  stm32h_send_address( address );
  STM32_EXPECT( STM32_COMM_ACK );

  // Expect second ACK as acknowledge of address validation & checksum 
  // NOTE: second ACK should come but in real it's not coming :-(  ... not required ...
  //STM32_EXPECT( STM32_COMM_ACK );

  return STM32_OK;
}


