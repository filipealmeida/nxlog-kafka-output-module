nxlog-kafka-output-module
=========================

An output module for nxlog to write to kafka brokers using librdkafka

# Foreword
This was implemented in a couple of hours and it requires proper structure and testing.
It was coded by example inspired on the other output modules provided.
The author unfortunately hasn't enough time to properly develop this much needed module.
The author asks of the nxlog developers to add and maintain the module in their great product.
Other developers are welcome to keep this going, will gladly add any challenger to the contributors of this project.

In any case it's functional and you can use it at your own risk.

# Usage

## Requirements
	The GNU toolchain
	GNU make
   	pthreads
	zlib
	[https://github.com/edenhill/librdkafka](librdkafka)
	[http://nxlog-ce.sourceforge.net/] nxlog-ce, patch included for version 2.8.1248

## Instructions


### Building and installing
Download and install librdkafka

	git clone https://github.com/edenhill/librdkafka.git
	cd librdkafka
	./configure
	make
	sudo make install
	cd ..

Download and uncompress [http://sourceforge.net/projects/nxlog-ce/files/](nxlog-ce source code) - nxlog-ce-2.8.1248.tar.gz

	tar zxvf nxlog-ce-2.8.1248.tar.gz
	cd nxlog-ce-2.8.1248

Download or clone nxlog-kafka-output-module from git 

	git clone https://github.com/filipealmeida/nxlog-kafka-output-module.git
	patch -p0 -f < nxlog-kafka-output-module/patch/nxlog-ce-2.8.1248-kafka-output-module.patch

Export libraries

	export LIBS="-lrdkafka -lrt -lz"

Build and install nxlog with kafka output module

	./configure
	make
	sudo make install

### Sample nxlog configuration

	########################################
	# Global directives                    #
	########################################
	User nxlog
	Group nxlog
	LogFile /var/log/nxlog/nxlog.log
	LogLevel INFO
	########################################
	# Modules                              #
	########################################
	<Input inFile>
	  Module      im_file
	  File        "/home/user/logs/myevents.log"
	  SavePos     TRUE
	  Recursive   TRUE
	</Input>
	<Output outKafka>
	  Module      om_kafka
	  BrokerList  10.150.137.205:9092,expert-services.telecom.pt:9092
	  Topic       test
	  #-- Partition   <number> - defaults to RD_KAFKA_PARTITION_UA
	  #-- Compression, one of none, gzip, snappy
	  Compression gzip
	</Output>
	########################################
	# Routes                               #
	########################################
	<Route 1>
	  Path        inFile => outKafka
	</Route>
