/*
 *
 *  Copyright (c) 2004-2005 Warren Jasper <wjasper@tx.ncsu.edu>
 *                          Mike Erickson <merickson@nc.rr.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <asm/types.h>

#include "pmd.h"
#include "usb-1208LS.h"


int main (int argc, char **argv)
{
  int flag;
  signed short svalue;
  int temp, i;
  int ch;
  int rate;
  __s16 sdata[1024];
  __u16 value;
  __u16 count;
  __u8 gains[8];
  __u8 options;
  __u8 input, pin = 0, channel, gain;

  HIDInterface*  hid = 0x0;
  hid_return ret;
  int interface;
	float current, volts, power;
  ret = hid_init();
  if (ret != HID_RET_SUCCESS) {
    fprintf(stderr, "hid_init failed with return code %d\n", ret);
    return -1;
  }

  if ((interface = PMD_Find_Interface(&hid, 0, USB1208LS_PID)) < 0) {
    fprintf(stderr, "USB 1208LS not found.\n");
    exit(1);
  } else {
    //printf("USB 208LS Device is found! interface = %d\n", interface);
  }


  /* config mask 0x01 means all inputs */
  usbDConfigPort_USB1208LS(hid, DIO_PORTB, DIO_DIR_IN);
  usbDConfigPort_USB1208LS(hid, DIO_PORTA, DIO_DIR_OUT);
  usbDOut_USB1208LS(hid, DIO_PORTA, 0x0);
  usbDOut_USB1208LS(hid, DIO_PORTA, 0x0);

       //gain = BP_20_00V;
	gain = BP_10_00V;
	//gain = BP_5_00V;
	//gain = BP_4_00V;
	//gain = BP_2_50V;
	//gain = BP_2_00V;
	//gain = BP_1_25V;
	//gain = BP_1_00V;
channel=0;
	 svalue = usbAIn_USB1208LS(hid, channel, gain);
	current = volts_LS(gain,svalue);
channel=1;
	 svalue = usbAIn_USB1208LS(hid, channel, gain);
	volts = volts_LS(gain,svalue) - current; //subtract voltage drop across 1 ohm resistor

	current = current*0.95; //resistance of 1 ohm (nominal) resistor
	printf("Heater %.2fA\t",current);
	printf("%.2fV\t",volts);
	power = current*volts;
	printf("%.2fW\n",power);



      ret = hid_close(hid);
      if (ret != HID_RET_SUCCESS) {
	fprintf(stderr, "hid_close failed with return code %d\n", ret);
	return 1;
      }

      hid_delete_HIDInterface(&hid);

      ret = hid_cleanup();
      if (ret != HID_RET_SUCCESS) {
	fprintf(stderr, "hid_cleanup failed with return code %d\n", ret);
	return 1;
      }
      return 0;


}
  



