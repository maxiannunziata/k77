/*
 * meta_logging.h
 *
 *  Created on: Mar 23, 2018
 *      Author: iotto
 */

#ifndef TENTA_META_FS_META_LOGGING_H_
#define TENTA_META_FS_META_LOGGING_H_

#include "base/logging.h"

namespace tenta {

#if TENTA_LOG_ENABLE
#define TENTA_OK_TO_LOG() true

#if TENTA_LOG_DB_ENABLE == 1
#define TENTA_DB_OK_TO_LOG() true
#else
#define TENTA_DB_OK_TO_LOG() false
#endif

#if TENTA_LOG_NET_ENABLE == 1
#define TENTA_NET_OK_TO_LOG() true
#else
#define TENTA_NET_OK_TO_LOG() false
#endif

#if TENTA_LOG_CACHE_ENABLE == 1
#define TENTA_CACHE_OK_TO_LOG() true
#else
#define TENTA_CACHE_OK_TO_LOG() false
#endif

#if TENTA_LOG_COOKIE_ENABLE == 1
#define TENTA_COOKIE_OK_TO_LOG() true
#else
#define TENTA_COOKIE_OK_TO_LOG() false
#endif

#if TENTA_LOG_HISTORY_ENABLE == 1
#define TENTA_HISTORY_OK_TO_LOG() true
#else
#define TENTA_HISTORY_OK_TO_LOG() false
#endif

#if TENTA_LOG_GUI_ENABLE == 1
#define TENTA_GUI_OK_TO_LOG() true
#else
#define TENTA_GUI_OK_TO_LOG() false
#endif

#if TENTA_LOG_FF_ENABLE == 1
#define TENTA_FF_OK_TO_LOG() true
#else
#define TENTA_FF_OK_TO_LOG() false
#endif

#else // TENTA_LOG_ENABLE
#define TENTA_OK_TO_LOG() false
#define TENTA_DB_OK_TO_LOG() false
#define TENTA_NET_OK_TO_LOG() false
#define TENTA_CACHE_OK_TO_LOG() false
#define TENTA_COOKIE_OK_TO_LOG() false
#define TENTA_HISTORY_OK_TO_LOG() false
#define TENTA_GUI_OK_TO_LOG() false
#define TENTA_FF_OK_TO_LOG() false
#endif // TENTA_LOG_ENABLE

#define TENTA_LOG(severity) \
    LAZY_STREAM(LOG_STREAM(severity), TENTA_OK_TO_LOG())

#define TENTA_LOG_DB(severity) \
    LAZY_STREAM(LOG_STREAM(severity), TENTA_DB_OK_TO_LOG())

#define TENTA_LOG_NET(severity) \
    LAZY_STREAM(LOG_STREAM(severity), TENTA_NET_OK_TO_LOG())

#define TENTA_LOG_CACHE(severity) \
    LAZY_STREAM(LOG_STREAM(severity), TENTA_CACHE_OK_TO_LOG())

#define TENTA_LOG_COOKIE(severity) \
    LAZY_STREAM(LOG_STREAM(severity), TENTA_COOKIE_OK_TO_LOG())

#define TENTA_LOG_HISTORY(severity) \
    LAZY_STREAM(LOG_STREAM(severity), TENTA_HISTORY_OK_TO_LOG())

#define TENTA_LOG_GUI(severity) \
    LAZY_STREAM(LOG_STREAM(severity), TENTA_GUI_OK_TO_LOG())

#define TENTA_LOG_FF(severity) \
    LAZY_STREAM(LOG_STREAM(severity), TENTA_FF_OK_TO_LOG())
} // namespace tenta
#endif /* TENTA_META_FS_META_LOGGING_H_ */
