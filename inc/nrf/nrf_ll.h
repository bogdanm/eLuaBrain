// Low level nRF operations (must be implemented by host)

#ifndef __NRF_LL_H__
#define __NRF_LL_H__

#include "type.h"

void nrf_ll_init( void );
void nrf_ll_set_ce( int state );
void nrf_ll_set_csn( int state );
int nrf_ll_get_irq( void );
void nrf_ll_send_packet( const u8 *packet, u16 len );
void nrf_ll_read_packet( u8 *packet, u16 len );
void nrf_ll_flush( u16 len );
void nrf_ll_delay_us( u32 delay );

#define nrf_ll_ce_low()       nrf_ll_set_ce( 0 )
#define nrf_ll_ce_high()      nrf_ll_set_ce( 1 )
#define nrf_ll_csn_low()      nrf_ll_set_csn( 0 )
#define nrf_ll_csn_high()     nrf_ll_set_csn( 1 )

#endif // #ifndef __NRF_LL_H__

