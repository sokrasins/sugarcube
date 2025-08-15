#include "glucose_msg.h"

#include <zephyr/data/json.h>

int64_t msg_glucose_decode(char *json_msg, int json_size, int *glucose)
{
    struct glucose {
        int value;
        const char *timestamp;
    };

    struct glucose g;
    struct json_obj_descr descr[] = { 
        JSON_OBJ_DESCR_PRIM(struct glucose, value, JSON_TOK_NUMBER), 
        JSON_OBJ_DESCR_PRIM(struct glucose, timestamp, JSON_TOK_STRING), 
    };

    int64_t rc = json_obj_parse(json_msg, json_size, descr, 2, (void *)&g);
    *glucose = g.value;
    return rc;
}

