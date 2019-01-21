/**
 * @file usb_moded-worker.h
 *
 * Copyright (C) 2013-2018 Jolla. All rights reserved.
 *
 * @author: Philippe De Swert <philippe.deswert@jollamobile.com>
 * @author: Simo Piiroinen <simo.piiroinen@jollamobile.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the Lesser GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the Lesser GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef  USB_MODED_WORKER_H_
# define USB_MODED_WORKER_H_

# include <stdbool.h>

# include "usb_moded-dyn-config.h"

/* ========================================================================= *
 * Constants
 * ========================================================================= */

/* ========================================================================= *
 * Prototypes
 * ========================================================================= */

/* -- worker -- */

bool                   worker_bailing_out          (void);
const char            *worker_get_kernel_module       (void);
bool                   worker_set_kernel_module       (const char *module);
void                   worker_clear_kernel_module     (void);
mode_list_elem_t      *worker_get_usb_mode_data    (void);
void                   worker_set_usb_mode_data    (mode_list_elem_t *data);
void                   worker_request_hardware_mode(const char *mode);
void                   worker_clear_hardware_mode  (void);
void                   worker_switch_to_mode       (const char *mode);
bool                   worker_init                 (void);
void                   worker_quit                 (void);
void                   worker_wakeup               (void);

#endif /* USB_MODED_WORKER_H_ */