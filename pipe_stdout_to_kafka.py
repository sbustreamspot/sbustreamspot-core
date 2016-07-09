#!/usr/bin/env python

import argparse
import json
import pdb
from pykafka import KafkaClient
from pykafka.exceptions import OffsetOutOfRangeError, RequestTimedOut
from pykafka.partitioners import HashingPartitioner
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--url', help='Kafka URL', required=True)
parser.add_argument('--topic', help='Kafka topic', required=True)
args = vars(parser.parse_args())

kafka_client = KafkaClient(args['url'])
kafka_topic = kafka_client.topics[args['topic']]
producer = kafka_topic.get_producer(
                partitioner=HashingPartitioner(),
                sync=True, linger_ms=1, ack_timeout_ms=30000, max_retries=0)

for line in sys.stdin:
    line_json = json.loads(line.strip()) # ensure it parses
    producer.produce(json.dumps(line_json), "0")
