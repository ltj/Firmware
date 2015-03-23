/**
 * @file drv_teensysense.h
 *
 * Teensy I2C device API
 */

#pragma once

#include <stdint.h>
#include <sys/ioctl.h>

#define TEENSY0_DEVICE_PATH "/dev/teensy0"

/*
 * ioctl() definitions
 */

#define _TEENSYIOCBASE		(0x3000)
#define _TEENSYIOC(_n)		(_IOC(_RGBLEDIOCBASE, _n))

