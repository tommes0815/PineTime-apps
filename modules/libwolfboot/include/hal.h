/* hal.h
 *
 * The HAL API definitions.
 *
 * Copyright (C) 2020 wolfSSL Inc.
 *
 * This file is part of wolfBoot.
 *
 * wolfBoot is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfBoot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

#ifndef H_HAL_
#define H_HAL_

#include <inttypes.h>

#include "target.h"
#include "nvmc.h"
#include "mtd.h"

extern mtd_dev_t *mtd0;

#define hal_flash_write(a,d,l) flash_write(a,d,l)
#define hal_flash_erase(a,l)   flash_erase(a,l)
#define hal_flash_unlock() do{}while(0)
#define hal_flash_lock() do{}while(0)

#include "spi_flash.h"
#define ext_flash_lock() do{}while(0)
#define ext_flash_unlock() do{}while(0)
#define ext_flash_read(dst, addr, sz) mtd_read(mtd0, addr, dst, sz)
#define ext_flash_write(src, addr, sz)  mtd_write(mtd0, addr, src, sz)
#define ext_flash_erase(a,l)  mtd_erase(mtd0, a, l)


#endif /* H_HAL_ */
