/*
 *  Copyright (C) 2002-2007  The DOSBox Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* $Id: vga_misc.cpp,v 1.35 2007-10-13 16:34:06 c2woody Exp $ */

#include "dosbox.h"
#include "inout.h"
#include "pic.h"
#include "vga.h"
#include <math.h>


void vga_write_p3d4(Bitu port,Bitu val,Bitu iolen);
Bitu vga_read_p3d4(Bitu port,Bitu iolen);
void vga_write_p3d5(Bitu port,Bitu val,Bitu iolen);
Bitu vga_read_p3d5(Bitu port,Bitu iolen);

static Bitu vga_read_p3da(Bitu port,Bitu iolen) {
	vga.internal.attrindex=false;
	vga.tandy.pcjr_flipflop=false;
	Bit8u retval=0;
	double timeInFrame = PIC_FullIndex()-vga.draw.delay.framestart;

 	vga.internal.attrindex=false;
 	vga.tandy.pcjr_flipflop=false;

	switch (machine) {
	case MCH_HERC:
	{
		// 3BAh (R):  Status Register
		// bit   0  Horizontal sync
		//       3  Video signal
		//       7  Vertical sync
		if(timeInFrame >= vga.draw.delay.vrstart &&
			timeInFrame <= vga.draw.delay.vrend)
			retval |= 0x80;
		double timeInLine=fmod(timeInFrame,vga.draw.delay.htotal);
		if(timeInLine >= vga.draw.delay.hrstart &&
			timeInLine <= vga.draw.delay.hrend)
			retval |= 1;
		retval |= 0x10;		//Hercules ident
		break;
	}
	default:
		// 3DAh (R):  Status Register
		// bit   0  Horizontal or Vertical blanking
		//       3  Vertical sync

		if(timeInFrame >= vga.draw.delay.vrstart &&
			timeInFrame <= vga.draw.delay.vrend)
			retval |= 8;
		if(timeInFrame >= vga.draw.delay.vdend)
			retval |= 1;
		else {
			double timeInLine=fmod(timeInFrame,vga.draw.delay.htotal);
			if(timeInLine >= (vga.draw.delay.hblkstart) && 
					timeInLine <= vga.draw.delay.hblkend){
				retval |= 1;
			}
		}
	}
	return retval;
}

static void write_p3c2(Bitu port,Bitu val,Bitu iolen) {
	vga.misc_output=val;
	if (val & 0x1) {
		IO_RegisterWriteHandler(0x3d4,vga_write_p3d4,IO_MB);
		IO_RegisterReadHandler(0x3d4,vga_read_p3d4,IO_MB);
		IO_RegisterReadHandler(0x3da,vga_read_p3da,IO_MB);

		IO_RegisterWriteHandler(0x3d5,vga_write_p3d5,IO_MB);
		IO_RegisterReadHandler(0x3d5,vga_read_p3d5,IO_MB);

		IO_FreeWriteHandler(0x3b4,IO_MB);
		IO_FreeReadHandler(0x3b4,IO_MB);
		IO_FreeWriteHandler(0x3b5,IO_MB);
		IO_FreeReadHandler(0x3b5,IO_MB);
		IO_FreeReadHandler(0x3ba,IO_MB);
	} else {
		IO_RegisterWriteHandler(0x3b4,vga_write_p3d4,IO_MB);
		IO_RegisterReadHandler(0x3b4,vga_read_p3d4,IO_MB);
		IO_RegisterReadHandler(0x3ba,vga_read_p3da,IO_MB);

		IO_RegisterWriteHandler(0x3b5,vga_write_p3d5,IO_MB);
		IO_RegisterReadHandler(0x3b5,vga_read_p3d5,IO_MB);


		IO_FreeWriteHandler(0x3d4,IO_MB);
		IO_FreeReadHandler(0x3d4,IO_MB);
		IO_FreeWriteHandler(0x3d5,IO_MB);
		IO_FreeReadHandler(0x3d5,IO_MB);
		IO_FreeReadHandler(0x3da,IO_MB);
	}
	/*
		0	If set Color Emulation. Base Address=3Dxh else Mono Emulation. Base Address=3Bxh.
		2-3	Clock Select. 0: 25MHz, 1: 28MHz
		5	When in Odd/Even modes Select High 64k bank if set
		6	Horizontal Sync Polarity. Negative if set
		7	Vertical Sync Polarity. Negative if set
			Bit 6-7 indicates the number of lines on the display:
			1:  400, 2: 350, 3: 480
			Note: Set to all zero on a hardware reset.
			Note: This register can be read from port 3CCh.
	*/
}


static Bitu read_p3cc(Bitu port,Bitu iolen) {
	return vga.misc_output;
}

// VGA feature control register
static Bitu read_p3ca(Bitu port,Bitu iolen) {
	return 0;
}

static Bitu read_p3c8(Bitu port,Bitu iolen) {
	return 0x10;
}

static Bitu read_p3c2(Bitu port,Bitu iolen) {
	if (GCC_UNLIKELY(machine==MCH_EGA)) {
		Bitu retcode=0x60;
		retcode |= (vga.draw.vret_triggered ? 0x80 : 0x00);
		vga.draw.vret_triggered=false;
		// ega colour monitor
		if ((((vga.misc_output>>2)&3)==0) || (((vga.misc_output>>2)&3)==3)) retcode|=0x10;
		return retcode;
	} else {
		return 0x70;
	}
}

void VGA_SetupMisc(void) {
	if (IS_EGAVGA_ARCH) {
		vga.draw.vret_triggered=false;
		IO_RegisterReadHandler(0x3c2,read_p3c2,IO_MB);
		IO_RegisterWriteHandler(0x3c2,write_p3c2,IO_MB);
		if (IS_VGA_ARCH) {
			IO_RegisterReadHandler(0x3ca,read_p3ca,IO_MB);
			IO_RegisterReadHandler(0x3cc,read_p3cc,IO_MB);
		} else {
			IO_RegisterReadHandler(0x3c8,read_p3c8,IO_MB);
		}
	} else if (machine==MCH_CGA || IS_TANDY_ARCH) {
		IO_RegisterReadHandler(0x3da,vga_read_p3da,IO_MB);
	} else if (machine==MCH_HERC) {
		IO_RegisterReadHandler(0x3ba,vga_read_p3da,IO_MB);
	}
}