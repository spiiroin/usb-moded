/**
  @file usb_moded-modules.c
 
  Copyright (C) 2010 Nokia Corporation. All rights reserved.

  @author: Philippe De Swert <philippe.de-swert@nokia.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the Lesser GNU General Public License 
  version 2 as published by the Free Software Foundation. 

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.
 
  You should have received a copy of the Lesser GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>

#include "usb_moded.h"
#include "usb_moded-modules.h"
#include "usb_moded-log.h"
#include "usb_moded-config.h"
#include "usb_moded-dbus.h"
#include "usb_moded-dbus-private.h"
#include "usb_moded-config.h"

/** load module 
 *
 * @param module Name of the module to load
 * @return 0 on success, non-zero on failure
 *
 */
int usb_moded_load_module(const char *module)
{
	gchar *command; 
	int ret = 0;
	
	command = g_strconcat("modprobe ", module, NULL);
	ret = system(command);
	g_free(command);
	if( ret == 0)
		log_info("Module %s loaded successfully\n", module);
	return(ret);
}

/** unload module
 *  
 * @param module Name of the module to unload
 * @return 0 on success, non-zero on failure
 *
 */
int usb_moded_unload_module(const char *module)
{
	gchar *command;
	int ret = 0;

	command = g_strconcat("rmmod ", module, NULL);
	ret = system(command);
	g_free(command);

	return(ret);
}

/** find which module is loaded 
 *
 * @return The name of the loaded module, or NULL if no modules are loaded.
 */
const char * usb_moded_find_module(void)
{

	if(system("lsmod | grep g_nokia") == 0)
		return(MODULE_NETWORK);

	if(system("lsmod | grep g_file_storage") == 0)
		return(MODULE_MASS_STORAGE);

	if(system("lsmod | grep g_ether") == 0)
		return(MODULE_WINDOWS_NET);
	
	return(NULL);
}

/** clean up for modules when usb gets disconnected
 *
 * @param module The name of the module to unload
 * @return 0 on success, non-zero on failure
 *
 */
int usb_moded_module_cleanup(const char *module)
{
  	int retry = 0, success;
	
	if(!strcmp(module, MODULE_NONE))
		goto END;
	/* wait a bit for all components listening on dbus to clean up their act 
	sleep(2); */
	/* check if things were not reconnected in that timespan 
	if(get_usb_connection_state())
		return(0);
	*/
	
	success = usb_moded_unload_module(module);
	while(success)
	{
		/* module did not get unloaded. We will wait a bit and try again */
		sleep(1);
		success = usb_moded_unload_module(module);
		log_debug("unloading success = %d\n", success);
		if(!success)
			break;
		if(!usb_moded_find_module())
			goto END;
		retry++;
		if(retry == 2)
			break;
	}
	if(!strcmp(module, MODULE_NETWORK))
	{
		if(retry >= 2)
		{	
			/* we exited the loop due to max retry's. Module is not unloaded yet
			   lets go for more extreme measures
			   lsof, then various options of kill
			*/
			log_info("DIE DIE DIE! Free USB-illy!\n");
kill:		
			/* DIRTY DESPERATE WORKAROUND */
			/*system("for i in `lsof -t /dev/ttyGS*`; do cat /proc/$i/cmdline | sed 's/|//g' | sed "s/\x00/ /g" | awk '{ print $1 }' | xargs kill; done");
			system("for i in `ps ax | grep phonet-at | grep -v grep | awk '{ print $1 }'`; do kill -9 $i ; done");*/
			/* kill anything that still claims the usb tty's */
			system("for i in `lsof -t /dev/ttyGS*`; do kill -s SIGTERM $i ; done");
			system("for i in `lsof -t /dev/gc*`; do kill -s SIGTERM $i ; done");
			system("for i in `lsof -t /dev/mtp*`; do kill -s SIGTERM $i ; done");
			/* try to unload again and give up if it did not work yet */
			success = usb_moded_unload_module(module);
			if(success && retry < 10)
			{
				retry++;
				goto kill;
			}
			if(success && retry == 10)
			{
				system("for i in `lsof -t /dev/ttyGS*`; do kill -9 $i ; done");
				system("for i in `lsof -t /dev/gc*`; do kill -9 $i ; done");
				system("for i in `lsof -t /dev/mtp*`; do kill -9 $i ; done");
				/* try again since there seem to be hard to kill processes there */
				system("killall -9 obexd");
				system("killall -9 msycnd");
				success = usb_moded_unload_module(module);
			}

		}
	}
	if(!success)
		log_info("Module %s unloaded successfully\n", module);
	else
	{
		log_err("Module %s did not unload! Failing and going to undefined.\n", module);
		return(1);
	}
END:
	return(0);
}

/** try to unload modules to support switching
 *
 *
 * @param force force unloading with a nasty clean-up on TRUE, or just try unloading when FALSE
 * @return 0 on success, 1 on failure, 2 if hard clean-up failed
 */

int usb_moded_module_switch_prepare (int force)
{
	const char *unload;
	int ret = 1; 

        unload = usb_moded_find_module();
        if(unload)
	{
		if(force)
			ret = usb_moded_module_cleanup(unload);
		else
                	ret = usb_moded_unload_module(unload);
	}
	if(ret && force)
		return(2);
	else 
		return(ret);
}

/** check for loaded modules and clean-up if they are not for the chosen mode 
 *
 * @param module_name  module name to check for
 *
 */
void check_module_state(const char *module_name)
{
  const char *module;

  module = usb_moded_find_module();
  if(module != NULL)
  {
        /* do nothing if the right module is already loaded */
        if(strcmp(usb_moded_find_module(), module_name) != 0)
        {
                log_debug("%s not loaded, cleaning up\n", module_name);
                usb_moded_module_switch_prepare(TRUE);
        }
  }
}


#ifdef NOKIA
gboolean usb_module_timeout_cleanup(gpointer data)
{
	usb_moded_module_cleanup(get_usb_module());
	return FALSE;
}
#endif /* NOKIA */
