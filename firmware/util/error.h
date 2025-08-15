#ifndef ERROR_H_
#define ERROR_H_

#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log_ctrl.h>

/* Macro called upon a fatal error, reboots the device. */
#define FATAL_ERROR()					\
	LOG_ERR("Fatal error! Rebooting the device.");	\
	LOG_PANIC();					\
	IF_ENABLED(CONFIG_REBOOT, (sys_reboot(0)))

#endif /*ERROR_H_*/