/******************************************************************************
 *
 *  Copyright (C) 2012-2014 Realtek Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#define LOG_TAG "bt_vendor_usb"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include "bt_syslog.h"
#include "bt_vendor_if.h"
#include "bt_hci_bluez.h"

void USB_hw_config_start(void);

static vnd_if_cb_t vnd_usb;

static void btmp_log(const char *fmt_str, ...)
{
    va_list ap;
    char log_buf[1024];
    va_start(ap, fmt_str);
    vsnprintf(log_buf, 1024, fmt_str, ap);
    va_end(ap);

    fprintf(stdout, "%s\n", log_buf);
}

static void usb_vendor_init(const char *dev_node)
{
    const char *port;

    vnd_usb.fd = -1;

    port = dev_node ?: BLUETOOTH_USB_DEVICE_PORT;
    snprintf(vnd_usb.port_name, VND_PORT_NAME_MAXLEN, "%s", port);
}

static int usb_vendor_open(void)
{
    int dev_id;

    SYSLOGI("usb vendor open: opening %s", vnd_usb.port_name);

    /** USB port name is discarded, as bluez hci interfaces
     *  use socket to communicate with HCI core.
     */
    dev_id = hci_devid(NULL);

    vnd_usb.fd = hci_open_dev(dev_id);
    if (vnd_usb.fd == -1) {
        SYSLOGE("usb vendor open: unable to open dev_id %d(uid %d, gid %d): %s",
                dev_id, getuid(), getgid(), strerror(errno));
		btmp_log("usb vendor open: unable to open dev_id %d(uid %d, gid %d): %s",
                dev_id, getuid(), getgid(), strerror(errno));
        return -1;
    }
    struct stat filestat;
    
    fstat(vnd_usb.fd, &filestat);
    SYSLOGI("device fd = %d open", vnd_usb.fd);
    printf("File type:                ");
    switch (filestat.st_mode & S_IFMT) {
    case S_IFBLK:  printf("block device\n");            break;
    case S_IFCHR:  printf("character device\n");        break;
    case S_IFDIR:  printf("directory\n");               break;
    case S_IFIFO:  printf("FIFO/pipe\n");               break;
    case S_IFLNK:  printf("symlink\n");                 break;
    case S_IFREG:  printf("regular file\n");            break;
    case S_IFSOCK: printf("socket\n");                  break;
    default:       printf("unknown?\n");                break;
    }

    printf("I-node number:            %ld\n", (long) filestat.st_ino);
    printf("Mode:                     %lo (octal)\n",
           (unsigned long) filestat.st_mode);
    printf("Link count:               %ld\n", (long) filestat.st_nlink);
    printf("Ownership:                UID=%ld   GID=%ld\n",
           (long) filestat.st_uid, (long) filestat.st_gid);
    printf("Preferred I/O block size: %ld bytes\n",
           (long) filestat.st_blksize);
    printf("File size:                %lld bytes\n",
           (long long) filestat.st_size);
    printf("Blocks allocated:         %lld\n",
           (long long) filestat.st_blocks);
    printf("Last status change:       %s", ctime(&filestat.st_ctime));
    printf("Last file access:         %s", ctime(&filestat.st_atime));
    printf("Last file modification:   %s", ctime(&filestat.st_mtime));
    return vnd_usb.fd;
}

static void usb_vendor_close(void)
{
    int res;

    if (vnd_usb.fd == -1)
        return;

    SYSLOGI("device fd = %d close", vnd_usb.fd);

    res = hci_close_dev(vnd_usb.fd);
    if (res  < 0)
        SYSLOGE("Failed to close(fd %d): %s", vnd_usb.fd, strerror(res));

    vnd_usb.fd = -1;
}

/*****************************************************************************
**
**   BLUETOOTH VENDOR INTERFACE LIBRARY FUNCTIONS
**
*****************************************************************************/

static int USB_bt_vnd_init(const bt_vendor_callbacks_t* p_cb, unsigned char *local_bdaddr, const char *dev_node)
{
    SYSLOGI("%s: dev_node %s", __FUNCTION__, dev_node);

    if (p_cb == NULL) {
        SYSLOGE("init failed with no user callbacks!");
        return -1;
    }

    usb_vendor_init(dev_node);

    /* store reference to user callbacks */
    bt_vendor_cbacks = (bt_vendor_callbacks_t *)p_cb;

    /* This is handed over from the stack */
    memcpy(vnd_local_bd_addr, local_bdaddr, 6);

    return 0;
}


/** Requested operations */
static int USB_bt_vnd_op(bt_vendor_opcode_t opcode, void *param)
{
    int retval = 0;

    SYSLOGI("op for %d", opcode);

    switch (opcode) {
    case BT_VND_OP_POWER_CTRL:
        {
            return 0;
        }
        break;

    case BT_VND_OP_FW_CFG:
        {
            USB_hw_config_start();
        }
        break;

    case BT_VND_OP_SCO_CFG:
        {
            retval = -1;
        }
        break;

    case BT_VND_OP_USERIAL_OPEN:
        {
            int (*fd_array)[] = (int (*)[]) param;
            int fd, idx;
            fd = usb_vendor_open();
            if (fd != -1) {
                for (idx=0; idx < CH_MAX; idx++)
                    (*fd_array)[idx] = fd;

                retval = 1;
            }
        }
        break;

    case BT_VND_OP_USERIAL_CLOSE:
        {
            usb_vendor_close();
        }
        break;

    case BT_VND_OP_GET_LPM_IDLE_TIMEOUT:
        {
            uint32_t *timeout_ms = (uint32_t *) param;
            *timeout_ms = 250;
        }
        break;

    case BT_VND_OP_LPM_SET_MODE:
        {
            if (bt_vendor_cbacks)
                bt_vendor_cbacks->lpm_cb(BT_VND_OP_RESULT_SUCCESS);
        }
        break;

    case BT_VND_OP_LPM_WAKE_SET_STATE:
        break;

    default:
         break;
    }

    return retval;
}

/** Closes the interface */
static void USB_bt_vnd_cleanup( void )
{
    SYSLOGI("cleanup");
    bt_vendor_cbacks = NULL;
}

// Entry point of DLib
const bt_vendor_interface_t USB_BLUETOOTH_VENDOR_LIB_INTERFACE = {
    sizeof(bt_vendor_interface_t),
    USB_bt_vnd_init,
    USB_bt_vnd_op,
    USB_bt_vnd_cleanup
};

const bt_vendor_interface_t *USB_bt_vnd_if = &USB_BLUETOOTH_VENDOR_LIB_INTERFACE;
