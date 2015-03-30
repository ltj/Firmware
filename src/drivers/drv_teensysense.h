/**
 * @file drv_teensysense.h
 *
 * Teensy I2C device API
 */

#pragma once

#include <stdint.h>
#include <sys/ioctl.h>

#define TEENSY0_DEVICE_PATH "/dev/teensy0"

enum TEENSY_SENSOR_TYPE {
	TEENSY_SENSOR_TYPE_TEST = 0
};

struct teensy_sensor_report {
	uint8_t type;
	int16_t value;
};

/*
 * ioctl() definitions
 */

#define _TEENSYIOCBASE		(0x3000)
#define _TEENSYIOC(_n)		(_IOC(_TEENSYIOCBASE, _n))

#define TEENSY_SENSOR_START	_TEENSYIOC(1)

#define TEENSY_SENSOR_STOP  _TEENSYIOC(2)

#define TEENSY_SENSOR_TEST  _TEENSYIOC(3)

#define TEENSY_SENSOR_READ  _TEENSYIOC(4)

