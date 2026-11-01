#ifndef BREAD_PRIVACY_H
#define BREAD_PRIVACY_H
#include <stddef.h>
#include <stdint.h>
enum bread_privacy_capability { BREAD_PRIVACY_CAMERA=0, BREAD_PRIVACY_MICROPHONE, BREAD_PRIVACY_LOCATION, BREAD_PRIVACY_NETWORK, BREAD_PRIVACY_BLUETOOTH, BREAD_PRIVACY_USB, BREAD_PRIVACY_SENSORS, BREAD_PRIVACY_CAPABILITY_COUNT };
void privacy_init(void);
void privacy_reset(void);
int privacy_set(enum bread_privacy_capability capability,int enabled);
int privacy_allows(enum bread_privacy_capability capability);
const char *privacy_name(enum bread_privacy_capability capability);
int privacy_find(const char *name,enum bread_privacy_capability *capability);
int privacy_serialize(char *output,size_t capacity);
int privacy_deserialize(const char *input);
#endif
