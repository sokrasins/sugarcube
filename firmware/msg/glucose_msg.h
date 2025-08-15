#ifndef GLUCOSE_MSG_H_
#define GLUCOSE_MSG_H_

#include <stdint.h>

//typedef struct {
//    int value;
//    const char *timestamp;
//} glucose_msg_t;

int64_t msg_glucose_decode(char *json_msg, int json_size, int *glucose);

#endif /*GLUCOSE_MSG_H_*/