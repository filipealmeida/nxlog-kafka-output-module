/*
 * Free as in the oxygen you breathe, do whatever you like with this, at your own risk
 * Author: Filipe Almeida
 */

#include "../../../common/module.h"
#include "../../../common/event.h"
#include "../../../common/error_debug.h"
#include "../../../common/alloc.h"
#include "om_kafka.h"

#include <librdkafka/rdkafka.h>

#define NX_LOGMODULE NX_LOGMODULE_MODULE

static void om_kafka_start(nx_module_t *module) {
	log_debug("Kafka module start entrypoint");
	//TODO: consolidate/merge with init
}

static void om_kafka_stop(nx_module_t *module) {
	log_debug("Kafka module stop entrypoint");
	ASSERT(module != NULL);
	//TODO: cleanup
}

static void om_kafka_config(nx_module_t *module) {
	const nx_directive_t* volatile curr;
	nx_om_kafka_conf_t* volatile modconf;
	nx_exception_t e;
	curr = module->directives;
	modconf = apr_pcalloc(module->pool, sizeof(nx_om_kafka_conf_t));
	module->config = modconf;
	modconf->partition = RD_KAFKA_PARTITION_UA;

	while (curr != NULL) {
		if (nx_module_common_keyword(curr->directive) == TRUE) { //ignore common configuration keywords

		} else if (strcasecmp(curr->directive, "brokerlist") == 0) {
			modconf->brokerlist = curr->args;
		} else if (strcasecmp(curr->directive, "topic") == 0) {
			modconf->topic = curr->args;
		} else if (strcasecmp(curr->directive, "compression") == 0) {
			modconf->compression = curr->args;
		} else if (strcasecmp(curr->directive, "partition") == 0) {
			modconf->partition = atoi(curr->args);
		} else {
			log_warn("Kafka output module ignored directive \"%s\" set to \"%s\"\n", curr->directive, curr->args);
		}
		curr = curr->next;
	}

}

static void om_kafka_init(nx_module_t *module) {
	log_debug("Kafka module init entrypoint");
	char errstr[512];
	nx_om_kafka_conf_t* modconf;
	modconf = (nx_om_kafka_conf_t*) module->config;

	rd_kafka_conf_t *conf;
	rd_kafka_topic_conf_t *topic_conf;
	/* Kafka configuration */
	conf = rd_kafka_conf_new();
	/* Topic configuration */
	topic_conf = rd_kafka_topic_conf_new();

	rd_kafka_conf_set_dr_cb(conf, msg_delivered);

	if (rd_kafka_conf_set(conf, "compression.codec", modconf->compression, errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
		log_error("Unable to set compression codec %s", modconf->compression);
	} else {
		log_info("Kafka compression set to %s", modconf->compression);
	}

	if (!(modconf->rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr)))) {
		log_error("Failed to create new producer: %s\n", errstr);
	}

	if (rd_kafka_brokers_add(modconf->rk, modconf->brokerlist) == 0) {
		log_error("No valid brokers specified (%s)", modconf->brokerlist);
	} else {
		log_info("Kafka brokers set to %s", modconf->brokerlist);
	}

	modconf->rkt = rd_kafka_topic_new(modconf->rk, modconf->topic, topic_conf);

	modconf->kafka_conf = conf;
	modconf->topic_conf = topic_conf;
}

/**
 * Message delivery report callback.
 * Called once for each message.
 * See rdkafka.h for more information.
 */
static void msg_delivered (rd_kafka_t *rk, void *payload, size_t len, int error_code, void *opaque, void *msg_opaque) {
	if (error_code) {
		log_error("Message delivery failed: %s\n", rd_kafka_err2str(error_code));
	} else {
		//log_debug(stderr, "Message delivered (%zd bytes)\n", len);
		log_debug("Message delivered (%zd bytes)\n", len);
	}
}

static void om_kafka_write(nx_module_t *module) {
	nx_om_kafka_conf_t* modconf;
	modconf = (nx_om_kafka_conf_t*) module->config;
	nx_logdata_t *logdata;
	if (nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING) {
		log_warn("Kafka module not running.");
		return;
	}

	if (module->output.buflen == 0) {
		if ((logdata = nx_module_logqueue_peek(module)) != NULL) {
			module->output.logdata = logdata;
			if (rd_kafka_produce(modconf->rkt, modconf->partition, RD_KAFKA_MSG_F_COPY,
					/* Payload and length */
					logdata->raw_event->buf, (int) logdata->raw_event->len,
					/* Optional key and its length */
					NULL, 0,
					/* Message opaque, provided in delivery report callback as msg_opaque. */
					NULL) == -1) {
				log_error("Unable to produce message");
				rd_kafka_poll(modconf->rk, 0);
			} else {
				//TODO: report on message
				log_debug("Message sent");
				rd_kafka_poll(modconf->rk, 0);
				nx_module_logqueue_pop(module, module->output.logdata);
				nx_logdata_free(module->output.logdata);
				module->output.logdata = NULL;
			}
		}
	}
}

static void om_kafka_event(nx_module_t *module, nx_event_t *event) {
	log_debug("Kafka module event entrypoint");
	nx_exception_t e;
	ASSERT(module != NULL);
	ASSERT(event != NULL);

	nx_om_kafka_conf_t* modconf;
	modconf = (nx_om_kafka_conf_t*) module->config;
	switch (event->type) {
		case NX_EVENT_DATA_AVAILABLE:
			log_debug("Output buflen: %d, bufstart: %d", (int) module->output.buflen, (int) module->output.bufstart);
			try {
				om_kafka_write(module);
			} catch (e) {
				io_err_handler(module, &e);
			}
			break;
		case NX_EVENT_READ:
			break;
		case NX_EVENT_WRITE:
			break;
		case NX_EVENT_RECONNECT:
			break;
		case NX_EVENT_DISCONNECT:
			break;
		case NX_EVENT_TIMEOUT:
			break;
		case NX_EVENT_POLL:
			if (nx_module_get_status(module) == NX_MODULE_STATUS_RUNNING) {
				nx_module_pollset_poll(module, FALSE);
			} 
			break;
		default:
		nx_panic("invalid event type: %d", event->type);
	}
}

static void io_err_handler(nx_module_t *module, nx_exception_t *e) NORETURN;
static void io_err_handler(nx_module_t *module, nx_exception_t *e)
{
    ASSERT(e != NULL);
    ASSERT(module != NULL);

    nx_module_stop_self(module);
    om_kafka_stop(module);
    rethrow(*e);
}

NX_MODULE_DECLARATION nx_om_kafka_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_OUTPUT,
    NULL,			// capabilities
    om_kafka_config,		// config
    om_kafka_start,		// start
    om_kafka_stop, 		// stop
    NULL,			// pause
    NULL,			// resume
    om_kafka_init,		// init
    NULL,			// shutdown
    om_kafka_event,		// event
    NULL,			// info
    NULL,			// exports
};
