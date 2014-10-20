/*
 * Free as in the oxygen you breathe, do whatever you like with this, at your own risk
 * Author: Filipe Almeida <filipe@marmelos.com>
 */

#ifndef __NX_OM_KAFKA_H
#define __NX_OM_KAFKA_H

#include "../../../common/types.h"
#include <librdkafka/rdkafka.h>

static void msg_delivered (rd_kafka_t *rk, void *payload, size_t len, int error_code, void *opaque, void *msg_opaque);
static void io_err_handler(nx_module_t *module, nx_exception_t *e);

typedef struct nx_om_kafka_conf_t {
	char* brokerlist;
	char* topic;
	char* compression;
	int partition;
	rd_kafka_conf_t *kafka_conf;
	rd_kafka_topic_conf_t *topic_conf;
	rd_kafka_t *rk;
	rd_kafka_topic_t *rkt;
	nx_logdata_t	*logdata;
} nx_om_kafka_conf_t;


#endif	/* __NX_OM_KAFKA_H */
