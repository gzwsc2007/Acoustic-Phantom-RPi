/*
Includes for broadcom and mmal headers wrapped in C style interface.
*/

#ifndef __MMALINCLUDES_H_
#define __MMALINCLUDES_H_

extern "C"
{
#include "bcm_host.h"
#include "interface/vcos/vcos.h"
#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_logging.h"
#include "interface/mmal/mmal_buffer.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_connection.h"
}

#endif
