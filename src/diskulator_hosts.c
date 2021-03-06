/**
 * #FUJINET CONFIG
 * Diskulator Hosts/Devices
 */

#include <stdlib.h>
#include <string.h>
#include "diskulator_hosts.h"
#include "screen.h"
#include "fuji_sio.h"
#include "fuji_typedefs.h"
#include "error.h"
#include "die.h"
#include "input.h"
#include "bar.h"

char text_empty[]="Empty";

typedef enum _substate
  {
   HOSTS,
   DEVICES,
   DONE
  } SubState;

/**
 * Display Hosts Slots
 */
void diskulator_hosts_display_host_slots(HostSlots *hs)
{
  unsigned char i;

  // Display host slots
  for (i = 0; i < 8; i++)
    {
      unsigned char n = i + 1;
      unsigned char ni[2];
      
      utoa(n, ni, 10);
      screen_puts(2, i + 1, ni);
      
      if (hs->host[i][0] != 0x00)
	screen_puts(5, i + 1, hs->host[i]);
      else
	screen_puts(5, i + 1, text_empty);
    }
}

/**
 * Display device slots
 */
void diskulator_hosts_display_device_slots(unsigned char y, DeviceSlots *ds)
{
  unsigned char i;

  // Display device slots
  for (i = 0; i < 8; i++)
    {
      unsigned char d[6];
      
      d[1] = 0x20;
      d[2] = 0x31 + i;
      d[4] = 0x20;
      d[5] = 0x00;
      
      if (ds->slot[i].file[0] != 0x00)
        {
	  d[0] = ds->slot[i].hostSlot + 0x31;
	  d[3] = (ds->slot[i].mode == 0x02 ? 'W' : 'R');
        }
      else
        {
	  d[0] = 0x20;
	  d[3] = 0x20;
        }
      
      screen_puts(0, i + y, d);
      
      if (ds->slot[i].file[0] != 0x00)
	screen_puts(5,i+y,ds->slot[i].file);
      else
	screen_puts(5,i+y,text_empty);
    }
}

/**
 * Keys for Hosts mode
 */
void diskulator_hosts_keys_hosts(void)
{
  screen_clear_line(20);
  screen_clear_line(21);
  screen_puts(0,20,"\xD9" "\x91\x8d\x98\x19Slot\xD9" "\xA5" "\x19" "dit Slot\xD9\xB2\xA5\xB4\xB5\xB2\xAE\x19Select Files");
  screen_puts(2,21,"\xD9" "\xA3" "\x19" "onfig" "\xD9" "\xB4\xA1\xA2" "\x19" "Drive Slots" "\xD9" "\xAF\xB0\xB4\xA9\xAF\xAE" "\x19" "Boot");
}

/**
 * Keys for Devices mode
 */
void diskulator_hosts_keys_devices(void)
{
  screen_clear_line(20);
  screen_clear_line(21);
  screen_puts(0,20,"\xD9""\x91\x8d\x98\x19Slot\xD9""\xA5\x19ject slot\xD9""\xA3\x19onfiguration");
  screen_puts(3,21,"\xD9\xB4\xA1\xA2\x19Host Slots \xD9\xB2\x19""ead \xD9\xB7\x19rite");
}

/**
 * Diskulator hosts setup
 */
void diskulator_hosts_setup(HostSlots *hs, DeviceSlots *ds)
{
  unsigned char retry=5;
  
  screen_dlist_diskulator_hosts();
  
  screen_puts(3, 0, "TNFS HOST LIST");
  screen_puts(24, 9, "DRIVE SLOTS");

  while (retry>0)
    {
      fuji_sio_read_host_slots(hs);
      
      if (fuji_sio_error())
	retry--;
      else
	break;
    }

  if (fuji_sio_error())
    error_fatal(ERROR_READING_HOST_SLOTS);

  retry=5;
  
  diskulator_hosts_display_host_slots(hs);

  while (retry>0)
    {
      fuji_sio_read_device_slots(ds);

      if (fuji_sio_error())
	retry--;
      else
	break;
    }
  
  if (fuji_sio_error())
    error_fatal(ERROR_READING_DEVICE_SLOTS);
  
  diskulator_hosts_display_device_slots(11,ds);

  diskulator_hosts_keys_hosts();
  
  bar_show(2);
}

/**
 * Handle jump keys (1-8, shift 1-8)
 */
void diskulator_hosts_handle_jump_keys(unsigned char k,unsigned char *i, SubState *ss)
{
  switch(k)
    {
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
      *i=k-'1';
      bar_show((*i)+(*ss==DEVICES ? ORIGIN_DEVICE_SLOTS : ORIGIN_HOST_SLOTS));
      break;
    }
}

/**
 * Handle nav keys depending on sub-state (HOSTS or DEVICES)
 */
void diskulator_hosts_handle_nav_keys(unsigned char k, unsigned char *i, SubState *new_substate)
{
  unsigned char o;
  
  if (*new_substate==DEVICES)
    o=ORIGIN_DEVICE_SLOTS;
  else
    o=ORIGIN_HOST_SLOTS;

  input_handle_nav_keys(k,o,8,i);
}

/**
 * Edit a host slot
 */
void diskulator_hosts_edit_host_slot(unsigned char i, HostSlots* hs)
{
  if (hs->host[i][0] == 0x00)
    {
      char tmp[2]={0,0};
      screen_clear_line(i+1);
      tmp[0]=i+0x31;
      screen_puts(2,i+1,tmp);
    }
  screen_input(4, i+1, hs->host[i]);
  if (hs->host[i][0] == 0x00)
    screen_puts(5, i+1, text_empty);
  fuji_sio_write_host_slots(hs);
}

/**
 * Eject image from device slot
 */
void diskulator_hosts_eject_device_slot(unsigned char i, unsigned char pos, DeviceSlots* ds)
{
  char tmp[2]={0,0};

  tmp[0]=i+'1'; // string denoting now ejected device slot.
  
  fuji_sio_umount_device(i);
  memset(ds->slot[i].file,0,sizeof(ds->slot[i].file));
  ds->slot[i].hostSlot=0xFF;
  fuji_sio_write_device_slots(ds);
  
  screen_clear_line((i+pos-2));
  screen_puts(2,(i+pos-2),tmp);
  screen_puts(5,(i+pos-2),text_empty);
}

/**
 * Set device slot to read or write
 */
void diskulator_hosts_set_device_slot_mode(unsigned char i, unsigned char mode, DeviceSlots* ds)
{
  unsigned char tmp_hostSlot;
  unsigned char tmp_file[FILE_MAXLEN];
  unsigned char *full_path;

  full_path=(unsigned char *)malloc(256);
  
  // temporarily stash current values.
  tmp_hostSlot=ds->slot[i].hostSlot;
  memcpy(tmp_file,ds->slot[i].file,FILE_MAXLEN);
  fuji_sio_get_filename_for_device_slot(i,full_path);

  // Unmount slot
  fuji_sio_umount_device(i);

  // Slot is now wiped, need to re-populate from stash.
  ds->slot[i].hostSlot=tmp_hostSlot;
  ds->slot[i].mode=mode;
  memcpy(ds->slot[i].file,tmp_file,FILE_MAXLEN);
  fuji_sio_set_filename_for_device_slot(i,full_path);
  
  fuji_sio_write_device_slots(ds);
  fuji_sio_mount_device(i,mode);

  // If we couldn't mount read/write, then re-mount again as read-only.
  if (fuji_sio_error())
    {
      fuji_sio_umount_device(i);

      // Slot is now wiped, need to re-populate from stash.
      ds->slot[i].hostSlot=tmp_hostSlot;
      ds->slot[i].mode=mode;
      memcpy(ds->slot[i].file,tmp_file,FILE_MAXLEN);
      fuji_sio_set_filename_for_device_slot(i,full_path);

      // Try again.
      fuji_sio_write_device_slots(ds);
      fuji_sio_mount_device(i,mode);
    }

  // And update device slot display.
  diskulator_hosts_display_device_slots(11,ds);

  free(full_path);
}

/**
 * Diskulator interactive - hosts
 */
void diskulator_hosts_hosts(Context *context, SubState *new_substate)
{
  unsigned char k;
  unsigned char i=0;

  while (*new_substate==HOSTS)
    {
      if (input_handle_console_keys() == 0x03)
	{
	  *new_substate=DONE;
	  context->state = MOUNT_AND_BOOT;
	}

      k=input_handle_key();
      diskulator_hosts_handle_jump_keys(k,&i,new_substate);
      diskulator_hosts_handle_nav_keys(k,&i,new_substate);
      
      switch(k)
	{
	case 0x7F:
	  i=0;
	  *new_substate = DEVICES;
	  diskulator_hosts_keys_devices();
	  bar_show(i+ORIGIN_DEVICE_SLOTS);
	  break;
	case 'B':
	case 'b':
	  *new_substate=DONE;
	  context->state = MOUNT_AND_BOOT;
	  break;
	case 'C':
	case 'c':
	  context->state=DISKULATOR_INFO;
	  *new_substate=DONE;
	  break;
	case 'E':
	case 'e':
	  diskulator_hosts_edit_host_slot(i,&context->hostSlots);
	  break;
	case 0x9b: // RETURN
	  if (context->hostSlots.host[i][0]==0x00) // empty host slot?
	    break; // do nothing
	  
	  context->state=DISKULATOR_SELECT;
	  context->host_slot=i;
	  fuji_sio_mount_host(context->host_slot,&context->hostSlots);
	  
	  if (fuji_sio_error())
	    {
	      error(ERROR_MOUNTING_HOST_SLOT);
	      wait_a_moment();
	      context->state=CONNECT_WIFI;
	      *new_substate=DONE;
	      return;
	    }
	  else
	    *new_substate=DONE;

	  break;
	}
    }
}

/**
 * Diskulator interactive - device slots
 */
void diskulator_hosts_devices(Context *context, SubState *new_substate)
{
  unsigned char k;
  unsigned char i=0;

  while (*new_substate==DEVICES)
    {
      if (input_handle_console_keys() == 0x03)
	{
	  *new_substate=DONE;
	  context->state = MOUNT_AND_BOOT;
	}
      k=input_handle_key();
      diskulator_hosts_handle_jump_keys(k,&i,new_substate);
      diskulator_hosts_handle_nav_keys(k,&i,new_substate);
      
      switch(k)
	{
	case 'C':
	case 'c':
	  context->state=DISKULATOR_INFO;
	  *new_substate=DONE;
	  break;
	case 'E':
	case 'e':
	  diskulator_hosts_eject_device_slot(i,ORIGIN_DEVICE_SLOTS,&context->deviceSlots);
	  break;
	case 0x7F:
	  i=0;
	  *new_substate = HOSTS;
	  diskulator_hosts_keys_hosts();
	  bar_show(i+ORIGIN_HOST_SLOTS);
	  break;
	case 'R':
	case 'r':
	  diskulator_hosts_set_device_slot_mode(i,MODE_READ,&context->deviceSlots);
	  break;
	case 'W':
	case 'w':
	  diskulator_hosts_set_device_slot_mode(i,MODE_WRITE,&context->deviceSlots);
	  break;
	}
    }
}

/**
 * Clear file context
 */
void diskulator_hosts_clear_file_context(Context *context)
{
  memset(context->directory,0,sizeof(context->directory));
  memset(context->filter,0,sizeof(context->filter));
  context->dir_page=0;
}

/**
 * Connect wifi State
 */
State diskulator_hosts(Context *context)
{
  SubState ss=HOSTS;
    
  diskulator_hosts_setup(&context->hostSlots,&context->deviceSlots);
  diskulator_hosts_clear_file_context(context);
  
  while (ss != DONE)
    {
      switch(ss)
      	{
      	case HOSTS:
      	  diskulator_hosts_hosts(context,&ss);
      	  break;
      	case DEVICES:
      	  diskulator_hosts_devices(context,&ss);
      	  break;
      	}
    }
  
  return context->state;
}
