#ifndef _IOT_DEVICE_OBJECT_H_
#define _IOT_DEVICE_OBJECT_H_

#include <stdint.h>
#include <stdbool.h>
//#include "bacdef.h"

#define TS_LEN 30
#define MAX_NAME_LEN 10
#define MAX_DEV_OBJECTS 100

#define IOT

struct iot_device_object_d2c{

    uint8_t ip_address[4];
    uint8_t device_id;
    uint8_t *ts_current;
    uint8_t *description;
    uint8_t obj_instance;
    uint8_t obj_type;

    struct iot_property_list{
        //uint8_t *name; //identifier:instance - to be formatted as json string elsewhere
        uint8_t prop_instance;// ObjectId is pair of 2 object properties - type and instance
        uint8_t prop_id;//
        double present_value;
        uint8_t *ts_lastchanged;
        struct iot_property_list *next;
    };
    struct iot_device_object_d2c *next;
};

// struct iot_device_scan_d2c{

//     uint8_t ip_address[4];
//     uint8_t device_id;
//     uint8_t *ts_current;
//     uint8_t *description;
//     uint8_t obj_instance;
//     uint8_t obj_type;

//     struct iot_property_list{
//         uint8_t *name;
//         double value;
//         uint8_t *ts_lastchanged;
//     };
//     struct iot_device_scan_d2c *next;
// };

// struct iot_device_object_sync_d2c{

//     uint8_t ip_address[4];
//     uint8_t device_id;
//     uint8_t *ts_current;
//     uint8_t *description;
//     uint8_t obj_instance;
//     uint8_t obj_type;

//     struct iot_property_list{
//         uint8_t *name;
//         double value;
//         uint8_t *ts_lastchanged;
//     };
//     struct iot_device_object_sync_d2c *next;
// };

// struct iot_device_value_update_d2c{

//     uint8_t ip_address[4];
//     uint8_t device_id;
//     uint8_t *ts_current;
//     uint8_t *description;
//     uint8_t obj_instance;
//     uint8_t obj_type;

//     struct iot_property_list{
//         uint8_t *name;
//         double value;
//         uint8_t *ts_lastchanged;
//     };
//     struct iot_device_value_update_d2c *next;
// };

// typedef struct iot_device_scan_d2c *IOT_DEVICE_SCAN_D2C;
// typedef struct iot_device_object_sync_d2c *IOT_DEVICE_OBJECT_SYNC_D2C;
// typedef struct iot_device_value_update_d2c *IOT_DEVICE_VALUE_UPDATE_D2C;

typedef struct iot_device_object_d2c *IOT_DEVICE_OBJECT_D2C;
typedef struct iot_property_list *IOT_PROPERTY_LIST;

#endif