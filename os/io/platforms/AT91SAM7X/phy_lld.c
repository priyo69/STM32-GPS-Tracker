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
 * @file templates/mac_lld.c
 * @brief MAC Driver subsystem low level driver source template
 * @addtogroup MAC_LLD
 * @{
 */

#include <ch.h>
#include <mac.h>
#include <phy.h>

#include "mii.h"

/**
 * @brief Low level PHY initialization.
 */
void phy_lld_init(void) {

}

/**
 * Resets a PHY device.
 *
 * @param[in] macp pointer to the @p MACDriver object
 */
void phy_lld_reset(MACDriver *macp) {

  /*
   * Disables the pullups on all the pins that are latched on reset by the PHY.
   * The status latched into the PHY is:
   *   PHYADDR  = 00001
   *   PCS_LPBK = 0 (disabled)
   *   ISOLATE  = 0 (disabled)
   *   RMIISEL  = 0 (MII mode)
   *   RMIIBTB  = 0 (BTB mode disabled)
   *   SPEED    = 1 (100mbps)
   *   DUPLEX   = 1 (full duplex)
   *   ANEG_EN  = 1 (auto negotiation enabled)
   */
  AT91C_BASE_PIOB->PIO_PPUDR = PHY_LATCHED_PINS;

#ifdef PIOB_PHY_PD_MASK
  /*
   * PHY power control.
   */
  AT91C_BASE_PIOB->PIO_OER = PIOB_PHY_PD_MASK;       // Becomes an output.
  AT91C_BASE_PIOB->PIO_PPUDR = PIOB_PHY_PD_MASK;     // Default pullup disabled.
  AT91C_BASE_PIOB->PIO_SODR = PIOB_PHY_PD_MASK;      // Output to high level.
#endif

  /*
   * PHY reset by pulsing the NRST pin.
   */
  AT91C_BASE_RSTC->RSTC_RMR = 0xA5000100;
  AT91C_BASE_RSTC->RSTC_RCR = 0xA5000000 | AT91C_RSTC_EXTRST;
  while (!(AT91C_BASE_RSTC->RSTC_RSR & AT91C_RSTC_NRSTL))
    ;
}

/**
 * @brief Reads a PHY register.
 *
 * @param[in] macp pointer to the @p MACDriver object
 * @param addr the register address
 * @return The register value.
 */
phyreg_t phy_lld_get(MACDriver *macp, phyaddr_t addr) {

  AT91C_BASE_EMAC->EMAC_MAN = (0b01 << 30) |            /* SOF */
                              (0b10 << 28) |            /* RW */
                              (PHY_ADDRESS << 23) |     /* PHYA */
                              (addr << 18) |            /* REGA */
                              (0b10 << 16);             /* CODE */
  while (!( AT91C_BASE_EMAC->EMAC_NSR & AT91C_EMAC_IDLE))
    ;
  return (phyreg_t)(AT91C_BASE_EMAC->EMAC_MAN & 0xFFFF);
}

/**
 * @brief Writes a PHY register.
 *
 * @param[in] macp pointer to the @p MACDriver object
 * @param addr the register address
 * @param value the new register value
 */
void phy_lld_put(MACDriver *macp, phyaddr_t addr, phyreg_t value) {

  AT91C_BASE_EMAC->EMAC_MAN = (0b01 << 30) |            /* SOF */
                              (0b01 << 28) |            /* RW */
                              (PHY_ADDRESS << 23) |     /* PHYA */
                              (addr << 18) |            /* REGA */
                              (0b10 << 16) |            /* CODE */
                              value;
  while (!( AT91C_BASE_EMAC->EMAC_NSR & AT91C_EMAC_IDLE))
    ;
}

/** @} */