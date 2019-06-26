/*
  This file is part of the Arduino NINA firmware.
  Copyright (c) 2018 Arduino SA. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _WIRING_MATH_
#define _WIRING_MATH_

#ifdef __cplusplus
extern "C" {
#endif

// FIXME: There is a conflict with long int random(void) in stdlib.
// This needs to be handled differently.
// This is a hack to make it compile under ESP-IDF 4
// extern long random(long);
#define random esp32_random
extern long esp32_random(long);


#ifdef __cplusplus
}
#endif

#endif /* _WIRING_MATH_ */
