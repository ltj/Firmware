/**
 * @file teensysense.cpp
 *
 * Driver for Teensy I2C slave
 *
 * @author Lars Toft Jacobsen <lars@boxed.dk> <latj@itu.dk>
 */

#include <nuttx/config.h>

#include <drivers/device/i2c.h>

#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>

#include <nuttx/wqueue.h>
#include <nuttx/clock.h>

#include <systemlib/perf_counter.h>
#include <systemlib/err.h>
#include <systemlib/systemlib.h>

#include <board_config.h>

#include <drivers/drv_teensysense.h>

#ifndef CONFIG_SCHED_WORKQUEUE
# error This requires CONFIG_SCHED_WORKQUEUE.
#endif

#define ADDR			PX4_I2C_OBDEV_TEENSY	/**< I2C adress of Teensy 3.x slave */
#define CMD_HELLO		0x40


class TEENSYSENSE : public device::I2C
{
public:
	TEENSYSENSE(int bus, int teensysense);
	virtual ~TEENSYSENSE();


	virtual int		init();
	virtual int		probe();
	virtual int		info();
	virtual int		ioctl(struct file *filp, int cmd, unsigned long arg);
	virtual void	start();

private:
	work_s			_work;

	bool			_running;
	bool			_should_run;

	static void		cycle_trampoline(void *arg);
	void			cycle();
};


namespace
{
TEENSYSENSE *g_teensysense = nullptr;
}

void teensysense_usage();

extern "C" __EXPORT int teensysense_main(int argc, char *argv[]);

TEENSYSENSE::TEENSYSENSE(int bus, int teensysense) :
	I2C("teensysense", TEENSY0_DEVICE_PATH, bus, teensysense, 100000 /* maximum speed supported */),
	_running(false),
	_should_run(true)
{
	memset(&_work, 0, sizeof(_work));
}

TEENSYSENSE::~TEENSYSENSE()
{
}

int
TEENSYSENSE::init()
{
	int ret;
	ret = I2C::init();

	if (ret != OK) {
		return ret;
	}

	warnx("Teensy initialized");

	return OK;
}

int
TEENSYSENSE::probe()
{
	// uint8_t val;
	// return get(val);
	int ret = OK;
	return ret;
}

int
TEENSYSENSE::info()
{
	int ret = OK;
	return ret;
}

int
TEENSYSENSE::ioctl(struct file *filp, int cmd, unsigned long arg)
{
	int ret = -EINVAL;

	switch (cmd) {
		case TEENSY_SENSOR_START:
			start();
			ret = OK;
		default:
			ret = CDev::ioctl(filp, cmd, arg);
	}

	return ret;
}

void
TEENSYSENSE::start()
{
	warnx("Scheduling cycle");
	work_queue(HPWORK, &_work, (worker_t)&TEENSYSENSE::cycle_trampoline, this, 5);
}


void
TEENSYSENSE::cycle_trampoline(void *arg)
{
	
	TEENSYSENSE *dev = (TEENSYSENSE *)arg;

	dev->cycle();
}

/**
 * Main loop function
 */
void
TEENSYSENSE::cycle()
{
	// if (!_should_run) {
	// 	_running = false;
	// 	return;
	// }

	// TODO: do something by default
	warnx("This is where the Teensy kicks in");
	debug("This is where the Teensy kicks in");

	/* re-queue ourselves to run again later */
	_running = true;
	work_queue(HPWORK, &_work, (worker_t)&TEENSYSENSE::cycle_trampoline, this, USEC2TICK(1000000));
}

void
teensysense_usage()
{
	warnx("missing command: try 'start', 'test', 'info'");
	warnx("options:");
	warnx("    -b i2cbus (%d)", PX4_I2C_BUS_EXPANSION);
	warnx("    -a addr (0x%x)", ADDR);
}

int
teensysense_main(int argc, char *argv[])
{
	int i2cdevice = -1;
	int teensyadr = ADDR; /* 7bit */

	int ch;

	/* jump over start/off/etc and look at options first */
	while ((ch = getopt(argc, argv, "a:b:")) != EOF) {
		switch (ch) {
		case 'a':
			teensyadr = strtol(optarg, NULL, 0);
			break;

		case 'b':
			i2cdevice = strtol(optarg, NULL, 0);
			break;

		default:
			teensysense_usage();
			exit(0);
		}
	}

        if (optind >= argc) {
            teensysense_usage();
            exit(1);
        }

	const char *verb = argv[optind];

	int fd;
	int ret;

	if (!strcmp(verb, "start")) {
		if (g_teensysense != nullptr)
			errx(1, "already started");

		if (i2cdevice == -1) {
			i2cdevice = PX4_I2C_BUS_EXPANSION;
			g_teensysense = new TEENSYSENSE(PX4_I2C_BUS_EXPANSION, teensyadr);

			if (g_teensysense != nullptr && OK != g_teensysense->init()) {
				delete g_teensysense;
				g_teensysense = nullptr;
			}

			if (g_teensysense == nullptr) {
				errx(1, "init failed");
			}
		}

		if (g_teensysense == nullptr) {
			g_teensysense = new TEENSYSENSE(i2cdevice, teensyadr);

			if (g_teensysense == nullptr)
				errx(1, "new failed");

			if (OK != g_teensysense->init()) {
				delete g_teensysense;
				g_teensysense = nullptr;
				errx(1, "init failed");
			}
		}

		fd = open(TEENSY0_DEVICE_PATH, 0);
		if (fd == -1) {
			errx(1, "Unable to open " TEENSY0_DEVICE_PATH);
		}
		ret = ioctl(fd, TEENSY_SENSOR_START, 0);
		close(fd);
		exit(0);
	}

	/* need the driver past this point */
	if (g_teensysense == nullptr) {
		warnx("not started");
		teensysense_usage();
		exit(1);
	}

	if (!strcmp(verb, "test")) {
		fd = open(TEENSY0_DEVICE_PATH, O_RDONLY);

		if (fd == -1) {
			errx(1, "Unable to open " TEENSY0_DEVICE_PATH);
		}

		//TODO: perform test here
		// ret = ioctl(fd, RGBLED_SET_MODE, (unsigned long)RGBLED_MODE_PATTERN);
		ret = OK;

		close(fd);
		exit(ret);
	}

	if (!strcmp(verb, "info")) {
		g_teensysense->info();
		exit(0);
	}

	teensysense_usage();
	exit(0);
}
