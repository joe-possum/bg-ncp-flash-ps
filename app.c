/* standard library headers */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

/* BG stack headers */
#include "bg_types.h"
#include "gecko_bglib.h"

/* Own header */
#include "app.h"
#include "dump.h"
#include "support.h"
#include "common.h"

#define MODE_READ 1
#define MODE_WRITE 2
#define MODE_DELETE 4

// App booted flag
static bool appBooted = false;
static struct {
  uint16 key;
  uint8 *data;
  uint8 len, mode;
} config = { .data = NULL, .mode = 0, };
  
const char *getAppOptions(void) {
  return "r<kek>w<key>d<key>s<string-data>h<hex-data>";
}

void appOption(int option, const char *arg) {
  int di;
  printf("appOption(option: %c, arg='%s')\n",option,arg);
  switch(option) {
  case 'r':
    sscanf(arg,"%i",&di);
    config.mode |= MODE_READ;
    config.key = di;
    break;
  case 'w':
    sscanf(arg,"%i",&di);
    config.mode |= MODE_WRITE;
    config.key = di;
    break;
  case 'd':
    sscanf(arg,"%i",&di);
    config.mode |= MODE_DELETE;
    config.key = di;
    break;
  case 'h':
    config.len = strlen(arg);
    if(config.len & 1) {
      fprintf(stderr,"Error: hex argument requires even number of chars\n");
    }
    config.len >>= 1;
    config.data = malloc(config.len);
    for(int i = 0; i < config.len; i++) {
      char buf[3];
      strncpy(buf,&arg[i<<1],2);
      sscanf(buf,"%x",&di);
      config.data[i] = di;
    }
    break;
  case 's':
    config.len = strlen(arg);
    config.data = strdup(arg);
    break;
  default:
    fprintf(stderr,"Unhandled option '-%c'\n",option);
    exit(1);
  }
}

void appInit(void) {
  printf("key: %d\n",config.key);
  if(MODE_READ == config.mode) return;
  if(MODE_DELETE == config.mode) return;
  if((MODE_WRITE == config.mode)&&(config.data)) return;
  printf("Usage: bg-ncp-flash-ps [ -r <key> ] [ -w <key> ] [ -d <key> ] [ -h <hex-data> ] [ -s <string-data> ] \n");
  exit(1);
}

/***********************************************************************************************//**
 *  \brief  Event handler function.
 *  \param[in] evt Event pointer.
 **************************************************************************************************/
void appHandleEvents(struct gecko_cmd_packet *evt)
{
  if (NULL == evt) {
    return;
  }

  // Do not handle any events until system is booted up properly.
  if ((BGLIB_MSG_ID(evt->header) != gecko_evt_system_boot_id)
      && !appBooted) {
#if defined(DEBUG)
    printf("Event: 0x%04x\n", BGLIB_MSG_ID(evt->header));
#endif
    millisleep(50);
    return;
  }

  /* Handle events */
#ifdef DUMP
  switch (BGLIB_MSG_ID(evt->header)) {
  default:
    dump_event(evt);
  }
#endif
  switch (BGLIB_MSG_ID(evt->header)) {
  case gecko_evt_system_boot_id: /*********************************************************************************** system_boot **/
#define ED evt->data.evt_system_boot
    appBooted = true;
    switch(config.mode) {
    case MODE_READ:
      gecko_cmd_flash_ps_load(config.key);
      break;
    case MODE_WRITE:
      gecko_cmd_flash_ps_save(config.key,config.len,config.data);
      break;
    case MODE_DELETE:
      gecko_cmd_flash_ps_erase(config.key);
      break;
    }
    exit(0);
    break;
#undef ED

  default:
    break;
  }
}
