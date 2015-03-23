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

#include <systemlib/perf_counter.h>
#include <systemlib/err.h>
#include <systemlib/systemlib.h>

#include <board_config.h>

#include <drivers/drv_teensysense.h>

#define ADDR			PX4_I2C_OBDEV_TEENSY	/**< I2C adress of Teensy 3.x slave */
#define CMD_HELLO		0x40


class TEENSYSENSE : public device::I2C
{
public:
	TEENSYSENSE(int bus, int rgbled);
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

	static void		teensy_trampoline(void *arg);
	void			teensy();
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
	start();

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
	int ret = OK;
	return ret;
}

void
TEENSYSENSE::start()
{
	work_queue(HPWORK, &_work, (worker_t)&TEENSYSENSE::teensy_trampoline, this, 1);
}


void
TEENSYSENSE::teensy_trampoline(void *arg)
{
	TEENSYSENSE *dev = (TEENSYSENSE *)arg;

	if (g_teensysense != nullptr) {
		dev->teensy();
	}
}

/**
 * Main loop function
 */
void
TEENSYSENSE::teensy()
{
	// if (!_should_run) {
	// 	_running = false;
	// 	return;
	// }

	// TODO: do something by default
	warnx("This is where the Teensy kicks in");

	/* re-queue ourselves to run again later */
	_running = true;
	work_queue(HPWORK, &_work, (worker_t)&TEENSYSENSE::teensy_trampoline, this, 1000);
}

void
teensysense_usage()
{
	warnx("missing command: try 'start', 'test', 'info'");
	warnx("options:");
	warnx("    -b i2cbus (%d)", PX4_I2C_BUS_TEENSY);
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
			// try the external bus first
			i2cdevice = PX4_I2C_BUS_TEENSY;
			g_teensysense = new TEENSYSENSE(PX4_I2C_BUS_EXPANSION, teensyadr);

			if (g_teensysense != nullptr && OK != g_teensysense->init()) {
				delete g_teensysense;
				g_teensysense = nullptr;
			}

			if (g_teensysense == nullptr) {
				// fall back to default bus
				if (PX4_I2C_BUS_TEENSY == PX4_I2C_BUS_EXPANSION) {
					errx(1, "init failed");
				}
				i2cdevice = PX4_I2C_BUS_TEENSY;
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
		exit(0);
	}

	/* need the driver past this point */
	if (g_teensysense == nullptr) {
		warnx("not started");
		teensysense_usage();
		exit(1);
	}

	if (!strcmp(verb, "test")) {
		fd = open(TEENSY0_DEVICE_PATH, 0);

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
