/*************************************************************************
* Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/

/* command line tool that sends a BACnet service, and displays the reply */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>       /* for time */
#include <signal.h>
#include "iot_device_obj.h"


#define PRINT_ENABLED 1
#define MODIFICATIONS 1

#include "bacdef.h"
#include "config.h"
#include "bactext.h"
#include "bacerror.h"
#include "iam.h"
#include "arf.h"
#include "tsm.h"
#include "address.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "net.h"
#include "datalink.h"
#include "whois.h"
/* some demo stuff needed */
#include "filename.h"
#include "handlers.h"
#include "client.h"
#include "txbuf.h"
#include "dlenv.h"

#ifdef MODIFICATIONS
    #define MAX_NUM_OF_DEVICES 10
    #define BAC_ADDRESS_MULT 1
    #define DEBUG 1
    #ifdef DEBUG
    #define _DEBUG_TEST 1
    #else
    #define _DEBUG_TEST 0
    #endif

#ifdef IOT
    IOT_DEVICE_OBJECT_D2C head = NULL;
#endif

#define debug_print(fmt, ...) \
        do { if (_DEBUG_TEST) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __func__, __VA_ARGS__); } while (0)
#endif
/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

#ifdef MODIFICATIONS
    static uint16_t dev_count = 0;
    static uint32_t deviceId[MAX_NUM_OF_DEVICES];
#endif
/* converted command line arguments */
static uint32_t Target_Device_Object_Instance = BACNET_MAX_INSTANCE;
static uint32_t Target_Object_Instance = BACNET_MAX_INSTANCE;
static BACNET_OBJECT_TYPE Target_Object_Type = OBJECT_ANALOG_INPUT;
static BACNET_PROPERTY_ID Target_Object_Property = PROP_ACKED_TRANSITIONS;
static int32_t Target_Object_Index = BACNET_ARRAY_ALL;
/* the invoke id is needed to filter incoming messages */
static uint8_t Request_Invoke_ID = 0;
static BACNET_ADDRESS Target_Address;
static bool Error_Detected = false;

int epics(uint32_t target_device_obj_ins);

#ifdef MODIFICATIONS
            struct address_entry {
                struct address_entry *next;
                uint8_t Flags;
                uint32_t device_id;
                unsigned max_apdu;
                BACNET_ADDRESS address;
            };

            static struct address_table {
                struct address_entry *first;
                struct address_entry *last;
            } Address_Table = {
            0};


            struct address_entry *alloc_address_entry(
                void)
            {
                struct address_entry *rval;
                rval = (struct address_entry *) calloc(1, sizeof(struct address_entry));
                if (Address_Table.first == 0) {
                    Address_Table.first = Address_Table.last = rval;
                } else {
                    Address_Table.last->next = rval;
                    Address_Table.last = rval;
                }
                return rval;
            }



            bool bacnet_address_matches(
                BACNET_ADDRESS * a1,
                BACNET_ADDRESS * a2)
            {
                int i = 0;
                if (a1->net != a2->net)
                    return false;
                if (a1->len != a2->len)
                    return false;
                for (; i < a1->len; i++)
                    if (a1->adr[i] != a2->adr[i])
                        return false;
                return true;
            }

            void address_table_add(
                uint32_t device_id,
                unsigned max_apdu,
                BACNET_ADDRESS * src)
            {
                struct address_entry *pMatch;
                uint8_t flags = 0;

                pMatch = Address_Table.first;
                while (pMatch) {
                    if (pMatch->device_id == device_id) {
                        if (bacnet_address_matches(&pMatch->address, src))
                            return;
                        flags |= BAC_ADDRESS_MULT;
                        pMatch->Flags |= BAC_ADDRESS_MULT;
                    }
                    pMatch = pMatch->next;
                }

                pMatch = alloc_address_entry();

                pMatch->Flags = flags;
                pMatch->device_id = device_id;
                pMatch->max_apdu = max_apdu;
                pMatch->address = *src;

                return;
            }

#endif

static void MyErrorHandler(
    BACNET_ADDRESS * src,
    uint8_t invoke_id,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        printf("BACnet Error: %s: %s\r\n",
            bactext_error_class_name((int) error_class),
            bactext_error_code_name((int) error_code));
        Error_Detected = true;
    }
}

void MyAbortHandler(
    BACNET_ADDRESS * src,
    uint8_t invoke_id,
    uint8_t abort_reason,
    bool server)
{
    (void) server;
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        printf("BACnet Abort: %s\r\n",
            bactext_abort_reason_name((int) abort_reason));
        Error_Detected = true;
    }
}

void MyRejectHandler(
    BACNET_ADDRESS * src,
    uint8_t invoke_id,
    uint8_t reject_reason)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        printf("BACnet Reject: %s\r\n",
            bactext_reject_reason_name((int) reject_reason));
        Error_Detected = true;
    }
}

/** Handler for a ReadProperty ACK.
 * @ingroup DSRP
 * Doesn't actually do anything, except, for debugging, to
 * print out the ACK data of a matching request.
 *
 * @param service_request [in] The contents of the service request.
 * @param service_len [in] The length of the service_request.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param service_data [in] The BACNET_CONFIRMED_SERVICE_DATA information
 *                          decoded from the APDU header of this message.
 */
void My_Read_Property_Ack_Handler(
    uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA * service_data)
{
    int len = 0;
    BACNET_READ_PROPERTY_DATA data;

    printf("My_Read_Property_Ack_Handler gets called\n");

    if (address_match(&Target_Address, src) &&
        (service_data->invoke_id == Request_Invoke_ID)) {
        len =
            rp_ack_decode_service_request(service_request, service_len, &data);
        if (len > 0) {
            rp_ack_print_data(&data);
        }
    }
}

#ifdef MODIFICATIONS
            static void my_i_am_handler(
                uint8_t * service_request,
                uint16_t service_len,
                BACNET_ADDRESS * src)
            {
                int len = 0;
                uint32_t device_id = 0;
                unsigned max_apdu = 0;
                int segmentation = 0;
                uint16_t vendor_id = 0;
                unsigned i = 0;

 #ifdef IOT
                IOT_DEVICE_OBJECT_D2C bacnet_device = NULL, temp = NULL;
#endif

                (void) service_len;
                len =
                    iam_decode_service_request(service_request, &device_id, &max_apdu,
                    &segmentation, &vendor_id);
// #ifdef MODIFICATIONS
//                 deviceId[dev_count++] = (unsigned long) device_id;
// #endif
            #if PRINT_ENABLED
                fprintf(stderr, "Received I-Am Request");
            #endif
                if (len != -1) {

#ifdef IOT
        /*found at least one device....allocate the iot device structure*/
        bacnet_device = (IOT_DEVICE_OBJECT_D2C)malloc(sizeof(struct iot_device_object_d2c));
        if(bacnet_device == NULL){
            fprintf(stderr, "failure allocating memory for the d2c structure of bacnet device\n");
            return;
        }
        if(head == NULL)/*first device so point the address to the head ptr*/
            head = bacnet_device;
        else{
            temp = head;
            while(temp->next != NULL)
                temp = temp->next;
            temp->next = bacnet_device;
        }
        bacnet_device->next = NULL;
        bacnet_device->device_id = device_id;
#endif

            #if PRINT_ENABLED
                    fprintf(stderr, " from %lu, MAC = ", (unsigned long) device_id);
                    if ((src->mac_len == 6) && (src->len == 0)) {
                        fprintf(stderr, "%u.%u.%u.%u %02X%02X\n",
                            (unsigned)src->mac[0], (unsigned)src->mac[1],
                            (unsigned)src->mac[2], (unsigned)src->mac[3],
                            (unsigned)src->mac[4], (unsigned)src->mac[5]);

#ifdef IOT
                            bacnet_device->ip_address[0] = src->mac[0];
                            bacnet_device->ip_address[1] = src->mac[1];
                            bacnet_device->ip_address[2] = src->mac[2];
                            bacnet_device->ip_address[3] = src->mac[3];
#endif
                    } else {
                        for (i = 0; i < src->mac_len; i++) {
                            fprintf(stderr, "%02X", (unsigned)src->mac[i]);
                            if (i < (src->mac_len-1)) {
                                fprintf(stderr, ":");
                            }
                        }
                        fprintf(stderr, "\n");
                    }
            #endif
                    address_table_add(device_id, max_apdu, src);
                } else {
            #if PRINT_ENABLED
                    fprintf(stderr, ", but unable to decode it.\n");
            #endif
                }

                return;
            }
#endif

static void Init_Service_Handlers(
    void)
{
    Device_Init(NULL);
    /* we need to handle who-is
       to support dynamic device binding to us */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    /* handle i-am to support binding to other devices */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, my_i_am_handler);
    /* set the handler for all the services we don't implement
       It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    /* we must implement read property - it's required! */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY,handler_read_property);
    // /* handle the data coming back from confirmed requests */
    apdu_set_confirmed_ack_handler(SERVICE_CONFIRMED_READ_PROPERTY,My_Read_Property_Ack_Handler);
    /* handle any errors coming back */
    apdu_set_error_handler(SERVICE_CONFIRMED_READ_PROPERTY, MyErrorHandler);
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

    static void signal_handler(int signo)
    {
        printf("inside timer handler\n");
        switch (signo) 
        {
                case SIGTSTP:
                    for(int numdevices = 0; numdevices < dev_count; numdevices++)
                        epics(deviceId[numdevices]);
                break;
                
        }
    }


int main(
    int argc,
    char *argv[])
{
    BACNET_ADDRESS src = {
        0
    };  /* address where message came from */
    #ifdef MODIFICATIONS
        BACNET_ADDRESS dest;
    #endif
    uint16_t pdu_len = 0;
    unsigned timeout = 100;     /* milliseconds */
    unsigned max_apdu = 0;
    time_t elapsed_seconds = 0;
    time_t last_seconds = 0;
    time_t current_seconds = 0;
    time_t timeout_seconds = 0;
    time_t total_seconds = 0;
    bool found = false;

    struct sigaction p_sig_action;
    p_sig_action.sa_handler = signal_handler;
    sigaction(SIGTSTP, &p_sig_action, NULL);

    #ifdef IOT
    IOT_DEVICE_OBJECT_D2C temp = NULL;
#endif

    struct itimerval tv;
    tv.it_interval.tv_sec = 0;
    tv.it_interval.tv_usec = 5000000;
    tv.it_value.tv_sec = 0;
    tv.it_value.tv_usec = 5000000;
    setitimer(ITIMER_REAL, &tv, NULL);

    #ifdef MODIFICATIONS
        datalink_get_broadcast_address(&dest);
    #endif

    /* setup my info */
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    address_init();
    Init_Service_Handlers();
    dlenv_init();
    atexit(datalink_cleanup);
    /* configure the timeout values */
    last_seconds = time(NULL);
    timeout_seconds = (apdu_timeout() / 1000) * apdu_retries();
#ifdef MODIFICATIONS
    Send_WhoIs_To_Network(&dest, -1/*Target_Object_Instance_Min*/,
        -1/*Target_Object_Instance_Max*/);

            /* loop forever */
    for (;;) {
        /* increment timer - exit if timed out */
        current_seconds = time(NULL);
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }
        if (Error_Detected)
            break;
        /* increment timer - exit if timed out */
        elapsed_seconds = current_seconds - last_seconds;
        if (elapsed_seconds) {
#if defined(BACDL_BIP) && BBMD_ENABLED
            bvlc_maintenance_timer(elapsed_seconds);
#endif
        }
        total_seconds += elapsed_seconds;
        if (total_seconds > timeout_seconds)
            break;
        /* keep track of time for next check */
        last_seconds = current_seconds;
    }
#endif

// void mytimerhandler(void)
// {

//      for(numdevices = 0; numdevices < dev_count; numdevices++)
//     {
//         epics(deviceId[numdevices]);
//     }
// }
   
    printf("RAVI -- Calling EPICS\n");


#ifdef IOT
    temp = head;
    while(temp!=NULL){

        // printf("IOT dev %d was found in the list\n", temp->device_id);
        //  fprintf(stderr, "%u.%u.%u.%u %02X%02X\n",
        //         (unsigned)src->mac[0], (unsigned)src->mac[1],
        //         (unsigned)src->mac[2], (unsigned)src->mac[3],
        //         (unsigned)src->mac[4], (unsigned)src->mac[5]);

        epics(temp);
        temp = temp->next;
    }
#endif
    printf("before for loop\n");
    for(;;){}
    return 0;
}
