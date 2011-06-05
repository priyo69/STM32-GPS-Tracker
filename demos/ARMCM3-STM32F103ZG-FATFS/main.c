/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011 Giovanni Di Sirio.

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

#include <stdio.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "test.h"
#include "shell.h"
#include "evtimer.h"

#include "ff.h"

/*===========================================================================*/
/* Card insertion monitor.                                                   */
/*===========================================================================*/

#define SDC_POLLING_INTERVAL            10
#define SDC_POLLING_DELAY               10

/**
 * @brief   Card monitor timer.
 */
static VirtualTimer tmr;

/**
 * @brief   Debounce counter.
 */
static unsigned cnt;

/**
 * @brief   Card event sources.
 */
static EventSource inserted_event, removed_event;

/**
 * @brief   Inserion monitor function.
 *
 * @param[in] sdcp      pointer to the @p SDCDriver object
 *
 * @notapi
 */
bool_t sdc_lld_is_card_inserted(SDCDriver *sdcp) {

  return !palReadPad(GPIOF, GPIOF_SD_DETECT);
}

/**
 * @brief   Protection detection.
 * @note    Not supported.
 *
 * @param[in] sdcp      pointer to the @p SDCDriver object
 *
 * @notapi
 */
bool_t sdc_lld_is_write_protected(SDCDriver *sdcp) {

  return FALSE;
}

/**
 * @brief   Inserion monitor timer callback function.
 *
 * @param[in] p         pointer to the @p SDCDriver object
 *
 * @notapi
 */
static void tmrfunc(void *p) {
  SDCDriver *sdcp = p;

  if (cnt > 0) {
    if (sdcIsCardInserted(sdcp)) {
      if (--cnt == 0) {
        chEvtBroadcastI(&inserted_event);
      }
    }
    else
      cnt = SDC_POLLING_INTERVAL;
  }
  else {
    if (!sdcIsCardInserted(sdcp)) {
      cnt = SDC_POLLING_INTERVAL;
      chEvtBroadcastI(&removed_event);
    }
  }
  chVTSetI(&tmr, MS2ST(SDC_POLLING_DELAY), tmrfunc, sdcp);
}

/**
 * @brief   Polling monitor start.
 *
 * @param[in] sdcp      pointer to the @p SDCDriver object
 *
 * @notapi
 */
static void tmr_init(SDCDriver *sdcp) {

  chEvtInit(&inserted_event);
  chEvtInit(&removed_event);
  chSysLock();
  cnt = SDC_POLLING_INTERVAL;
  chVTSetI(&tmr, MS2ST(SDC_POLLING_DELAY), tmrfunc, sdcp);
  chSysUnlock();
}

/*===========================================================================*/
/* FatFs related.                                                            */
/*===========================================================================*/

/**
 * @brief FS object.
 */
FATFS SDC_FS;

/* FS mounted and ready.*/
static bool_t fs_ready = FALSE;

/* Generic large buffer.*/
uint8_t fbuff[1024];

static FRESULT scan_files(char *path)
{
  FRESULT res;
  FILINFO fno;
  DIR dir;
  int i;
  char *fn;

  res = f_opendir(&dir, path);
  if (res == FR_OK) {
    i = strlen(path);
    for (;;) {
      res = f_readdir(&dir, &fno);
      if (res != FR_OK || fno.fname[0] == 0)
        break;
      if (fno.fname[0] == '.')
        continue;
      fn = fno.fname;
      if (fno.fattrib & AM_DIR) {
        sprintf(&path[i], "/%s", fn);
        res = scan_files(path);
        if (res != FR_OK)
          break;
        path[i] = 0;
      }
      else {
//        iprintf("%s/%s\r\n", path, fn);
      }
    }
  }
  return res;
}


/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

#define SHELL_WA_SIZE   THD_WA_SIZE(2048)
#define TEST_WA_SIZE    THD_WA_SIZE(256)

static void cmd_mem(BaseChannel *chp, int argc, char *argv[]) {
  size_t n, size;
  char buf[52];

  (void)argv;
  if (argc > 0) {
    shellPrintLine(chp, "Usage: mem");
    return;
  }
  n = chHeapStatus(NULL, &size);
  sprintf(buf, "core free memory : %u bytes", chCoreStatus());
  shellPrintLine(chp, buf);
  sprintf(buf, "heap fragments   : %u", n);
  shellPrintLine(chp, buf);
  sprintf(buf, "heap free total  : %u bytes", size);
  shellPrintLine(chp, buf);
}

static void cmd_threads(BaseChannel *chp, int argc, char *argv[]) {
  static const char *states[] = {
    "READY",
    "CURRENT",
    "SUSPENDED",
    "WTSEM",
    "WTMTX",
    "WTCOND",
    "SLEEPING",
    "WTEXIT",
    "WTOREVT",
    "WTANDEVT",
    "SNDMSGQ",
    "SNDMSG",
    "WTMSG",
    "FINAL"
  };
  Thread *tp;
  char buf[60];

  (void)argv;
  if (argc > 0) {
    shellPrintLine(chp, "Usage: threads");
    return;
  }
  shellPrintLine(chp, "    addr    stack prio refs     state time");
  tp = chRegFirstThread();
  do {
    sprintf(buf, "%8lx %8lx %4u %4i %9s %u",
            (uint32_t)tp, (uint32_t)tp->p_ctx.r13,
            (unsigned int)tp->p_prio, tp->p_refs - 1,
            states[tp->p_state], (unsigned int)tp->p_time);
    shellPrintLine(chp, buf);
    tp = chRegNextThread(tp);
  } while (tp != NULL);
}

static void cmd_test(BaseChannel *chp, int argc, char *argv[]) {
  Thread *tp;

  (void)argv;
  if (argc > 0) {
    shellPrintLine(chp, "Usage: test");
    return;
  }
  tp = chThdCreateFromHeap(NULL, TEST_WA_SIZE, chThdGetPriority(),
                           TestThread, chp);
  if (tp == NULL) {
    shellPrintLine(chp, "out of memory");
    return;
  }
  chThdWait(tp);
}

static void cmd_tree(BaseChannel *chp, int argc, char *argv[]) {
  FRESULT err;
  DWORD clusters;
  FATFS *fsp;

  (void)argv;
  if (argc > 0) {
    shellPrintLine(chp, "Usage: tree");
    return;
  }
  if (!fs_ready) {
    shellPrintLine(chp, "File System not mounted");
    return;
  }
  err = f_getfree("/", &clusters, &fsp);
  if (err != FR_OK) {
    shellPrintLine(chp, "FS: f_getfree() failed");
    return;
  }
  sprintf((void *)fbuff,
          "FS: %lu free clusters, %lu sectors per cluster, %lu bytes free",
          clusters, (uint32_t)SDC_FS.csize,
          clusters * (uint32_t)SDC_FS.csize * (uint32_t)SDC_BLOCK_SIZE);
  shellPrintLine(chp, (void *)fbuff);
  fbuff[0] = 0;
  scan_files((char *)fbuff);
}

static const ShellCommand commands[] = {
  {"mem", cmd_mem},
  {"threads", cmd_threads},
  {"test", cmd_test},
  {"tree", cmd_tree},
  {NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
  (BaseChannel *)&SD2,
  commands
};

/*===========================================================================*/
/* Main and generic code.                                                    */
/*===========================================================================*/

/*
 * MMC card insertion event.
 */
static void InsertHandler(eventid_t id) {
  FRESULT err;

  (void)id;
  /*
   * On insertion MMC initialization and FS mount.
   */
  if (sdcConnect(&SDCD1)) {
    return;
  }
  err = f_mount(0, &SDC_FS);
  if (err != FR_OK) {
    sdcDisconnect(&SDCD1);
    return;
  }
  fs_ready = TRUE;
}

/*
 * MMC card removal event.
 */
static void RemoveHandler(eventid_t id) {

  (void)id;
  sdcDisconnect(&SDCD1);
  fs_ready = FALSE;
}

/*
 * Red LED blinker thread, times are in milliseconds.
 */
static WORKING_AREA(waThread1, 128);
static msg_t Thread1(void *arg) {

  (void)arg;
  while (TRUE) {
    palClearPad(GPIOF, GPIOF_LED4);
    palSetPad(GPIOF, GPIOF_LED1);
    chThdSleepMilliseconds(250);
    palClearPad(GPIOF, GPIOF_LED1);
    palSetPad(GPIOF, GPIOF_LED2);
    chThdSleepMilliseconds(250);
    palClearPad(GPIOF, GPIOF_LED2);
    palSetPad(GPIOF, GPIOF_LED3);
    chThdSleepMilliseconds(250);
    palClearPad(GPIOF, GPIOF_LED3);
    palSetPad(GPIOF, GPIOF_LED4);
    chThdSleepMilliseconds(250);
  }
}

/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * Activates the serial driver 1 using the driver default configuration.
   */
  sdStart(&SD1, NULL);

  /*
   * Activates the card insertion monitor.
   */
  tmr_init(&SDCD1);

  /*
   * Creates the blinker thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and check the button state.
   */
  while (TRUE) {
    if (!palReadPad(GPIOG, GPIOG_USER_BUTTON))
      TestThread(&SD1);
    chThdSleepMilliseconds(500);
  }
}