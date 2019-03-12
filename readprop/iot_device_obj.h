#ifndef _IOT_DEVICE_OBJECT_H_
#define _IOT_DEVICE_OBJECT_H_

#include <stdint.h>
#include <stdbool.h>
//#include "bacdef.h"

#define TS_LEN 30
#define MAX_NAME_LEN 10
#define MAX_DEV_OBJECTS 100

#define IOT

struct iot_ts_date_and_time {
    int year;
    int month;
    int day;
    int hour;
    int minutes;
    int seconds;
    int msec;
};

struct iot_property_list{
    uint16_t obj_prop_type;// ObjectId is pair of 2 object properties - type and instance
    uint16_t obj_prop_instance;// ObjectId is pair of 2 object properties - type and instance
    char prop_instance[30];// ObjectId is pair of 2 object properties - type and instance
    uint8_t isempty;
    char present_value[30];
    char ts_lastchanged[30];
    struct iot_property_list *next;
};

struct iot_device_object_d2c{

    uint8_t ip_address[4];
    uint8_t device_id;
    char ts_current[30];
    char *description;
    char *segmt;
    char *obj_type;
    struct iot_property_list *iot_prop_list;
    struct iot_device_object_d2c *next;
};

typedef struct _d2c_data{
    char *d2c_str;
    struct _d2c_data *next;
} *D2C_DATA;

typedef D2C_DATA *_HEAD_PTR_D2C_DATA_LIST;

typedef struct iot_device_object_d2c *IOT_DEVICE_OBJECT_D2C;
typedef struct iot_property_list *IOT_PROPERTY_LIST;
typedef struct iot_ts_date_and_time TS_DATE_AND_TIME;

char *get_ts_string(char *buff);
void json_formatter(IOT_DEVICE_OBJECT_D2C dev);
int init_sock_interface(void);
void send_d2c_data_to_sock_client(int sockfd);

#endif