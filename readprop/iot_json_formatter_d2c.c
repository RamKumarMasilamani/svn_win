#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include <json.h>
#include "iot_device_obj.h"

D2C_DATA *head_ptr_d2c_data_list = NULL;

void json_formatter(IOT_DEVICE_OBJECT_D2C dev){

    char bacnet_prefix[20];
    char buff[50];
    uint8_t count = 0;
    unsigned int obj_counter = 0;
    bool isstring = false;

    D2C_DATA d2c_data = NULL, temp;
    IOT_PROPERTY_LIST prop_list;
/*schema*/
//{
//   "device_id": "BACNET_114", // bacnet is prefix inside the iot platform
//   "timestamp": "2019-01-22T14:50:06.380162496Z",
//   "properties": [{
//     "name": "2:202", // object Measurand Room Temperature
//     "value": 21.15999984741211,
//     "last_changed" : "2019-01-22T14:50:06.544Z" // equals timestamp
//   },{
//     "name": "2:203", // object Measurand Volatile Organic Components for Air Quality
//     "value": 866.5999755859375,
//     "last_changed" : "2019-01-22T14:50:06.544Z" // equals timestamp
//   }]
// }
      /*Creating a json object*/
    json_object * devobj = json_object_new_object();
    json_object * proparray = json_object_new_array();
    json_object * pobj[50];// = json_object_new_object();

    if(head_ptr_d2c_data_list != NULL)
        d2c_data = *head_ptr_d2c_data_list;
    memset(buff, 0, 50);
    strcpy(bacnet_prefix, "BACNET_");
    if(sprintf(buff,"%d",dev->device_id))
    strcat(bacnet_prefix, buff);
    //json_object_object_add(devobj,"object identifier", json_object_new_string(bacnet_prefix)); debatable
    json_object_object_add(devobj,"device_id", json_object_new_string("ravi-test"));
    json_object_object_add(devobj,"timestamp", json_object_new_string(dev->ts_current));

    prop_list = dev->iot_prop_list;
    prop_list = prop_list->next;

    while(prop_list!=NULL){
        pobj[obj_counter] = json_object_new_object();
        json_object_object_add(pobj[obj_counter],"name", json_object_new_string(prop_list->prop_instance));
        memset(buff, 0, 50);
        strcpy(buff, prop_list->present_value);
        count = 0;
        while(buff[count]!='\0'){
            if((buff[count]>='a'&&buff[count]<='z')||(buff[count]>='A'&&buff[count]<='Z'))
                isstring = true;
            count++;
        }
        if(isstring)
            json_object_object_add(pobj[obj_counter],"value", json_object_new_string(prop_list->present_value));
        else
            json_object_object_add(pobj[obj_counter],"value", json_object_new_double(atof(prop_list->present_value)));

        json_object_object_add(pobj[obj_counter],"last_changed", json_object_new_string(prop_list->ts_lastchanged));
        json_object_array_add(proparray, pobj[obj_counter]);
        prop_list = prop_list->next;
        obj_counter++;
    }
    json_object_object_add(devobj,"properties", proparray);
    printf ("The json object created: %s\n",json_object_to_json_string(devobj));

    if(d2c_data == NULL){
        d2c_data = (struct _d2c_data *)malloc(sizeof(struct _d2c_data));
        d2c_data->d2c_str = (char *) malloc(4000);
        strcpy(d2c_data->d2c_str, json_object_to_json_string(devobj));
        d2c_data->next = NULL;
        head_ptr_d2c_data_list = &d2c_data;
    }
    else
    {
        d2c_data = *head_ptr_d2c_data_list;
        temp = (struct _d2c_data *)malloc(sizeof(struct _d2c_data));
        temp->d2c_str = (char *) malloc(4000);
        strcpy(temp->d2c_str, json_object_to_json_string(devobj));
        temp->next = d2c_data;
        head_ptr_d2c_data_list = &temp;
    }
}