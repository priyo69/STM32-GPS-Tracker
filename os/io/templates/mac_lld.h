/*
    ChibiOS/RT - Copyright (C) 2006-2007 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file templates/mac_lld.h
 * @brief MAC Driver subsystem low level driver header template
 * @addtogroup MAC_LLD
 * @{
 */

#ifndef _MAC_LLD_H_
#define _MAC_LLD_H_

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief Number of available descriptors/buffers.
 */
#if !defined(MAC_TRANSMIT_DESCRIPTORS) || defined(__DOXYGEN__)
#define MAC_TRANSMIT_DESCRIPTORS        2
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief Structure representing a MAC driver.
 */
typedef struct {
  enum {ifStopped = 0,
        ifStarted}      md_state;       /**< @brief Interface status.*/
  Semaphore             md_tdsem;       /**< Transmit semaphore.*/
  Semaphore             md_rdsem;       /**< Receive semaphore.*/
} MACDriver;

/**
 * @brief Structure representing a transmission descriptor.
 */
typedef struct {

} MACTransmitDescriptor;

/**
 * @brief Structure representing a receive descriptor.
 */
typedef struct {

} MACReceiveDescriptor;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void mac_lld_init(void);
  void mac_lld_set_address(MACDriver *macp, uint8_t *p);
  void mac_lld_start(MACDriver *macp);
  void mac_lld_stop(MACDriver *macp);
  MACTransmitDescriptor *max_lld_get_transmit_descriptor(MACDriver *macp);
  void mac_lld_release_transmit_descriptor(MACDriver *macp,
                                           MACTransmitDescriptor *tdp);
  uint8_t *mac_lld_get_transmit_buffer(MACTransmitDescriptor *tdp);
  MACReceiveDescriptor *max_lld_get_receive_descriptor(MACDriver *macp);
  void mac_lld_release_receive_descriptor(MACDriver *macp,
                                          MACReceiveDescriptor *rdp);
  uint8_t *mac_lld_get_receive_buffer(MACReceiveDescriptor *rdp);
#ifdef __cplusplus
}
#endif

#endif /* _MAC_LLD_H_ */

/** @} */