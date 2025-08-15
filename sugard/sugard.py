from awscrt import mqtt
from awsiot import mqtt_connection_builder
from tidepool import Tidepool
from datetime import datetime

import sys
import time
import json
import logging

import creds.tidepool_creds

log = logging.getLogger("sugard")
log.setLevel(logging.INFO)

try:
    from systemd.journal import JournalHandler

    log.addHandler(JournalHandler())
except ImportError:
    pass

endpoint = "a3tnwpx5rjol8f-ats.iot.us-east-1.amazonaws.com"
client_id = "glucoseSource"
topic = "glucose/value"
poll_period = 1  # minutes
max_pub_period = 15  # minutes
pub_thresh = 10  # percent
keepalive = 20 * 60  # max allowed

last_val = None


# Callback when connection is accidentally lost.
def on_connection_interrupted(connection, error, **kwargs):
    log.error("Connection interrupted. error: {}".format(error))


# Callback when an interrupted connection is re-established.
def on_connection_resumed(connection, return_code, session_present, **kwargs):
    log.info(
        "Connection resumed. return_code: {} session_present: {}".format(
            return_code, session_present
        )
    )

    if return_code == mqtt.ConnectReturnCode.ACCEPTED and not session_present:
        log.info("Session did not persist. Resubscribing to existing topics...")
        resubscribe_future, _ = connection.resubscribe_existing_topics()

        # Cannot synchronously wait for resubscribe result because we're on the connection's event-loop thread,
        # evaluate result with a callback instead.
        resubscribe_future.add_done_callback(on_resubscribe_complete)


def on_resubscribe_complete(resubscribe_future):
    resubscribe_results = resubscribe_future.result()
    log.info("Resubscribe results: {}".format(resubscribe_results))

    for topic, qos in resubscribe_results["topics"]:
        if qos is None:
            sys.exit("Server rejected resubscribe to topic: {}".format(topic))


# Callback when the subscribed topic receives a message
def on_message_received(topic, payload, dup, qos, retain, **kwargs):
    log.info("Received message from topic '{}': {}".format(topic, payload))


# Callback when the connection successfully connects
def on_connection_success(connection, callback_data):
    assert isinstance(callback_data, mqtt.OnConnectionSuccessData)
    log.info(
        "Connection Successful with return code: {} session present: {}".format(
            callback_data.return_code, callback_data.session_present
        )
    )


# Callback when a connection attempt fails
def on_connection_failure(connection, callback_data):
    assert isinstance(callback_data, mqtt.OnConnectionFailureData)
    log.info("Connection failed with error code: {}".format(callback_data.error))


# Callback when a connection has been disconnected or shutdown successfully
def on_connection_closed(connection, callback_data):
    log.info("Connection closed")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        log.error("ERROR: Required argument, must provide path to credentials folder")
        sys.exit(-1)

    # Set paths to required credentials
    cred_path = sys.argv[1]
    cert_path = cred_path + "data_server.cert.pem"
    ca_path = cred_path + "root-CA.crt"
    key_path = cred_path + "data_server.private.key"

    # Create a MQTT connection from the command line data
    mqtt_connection = mqtt_connection_builder.mtls_from_path(
        endpoint=endpoint,
        port=None,
        cert_filepath=cert_path,
        pri_key_filepath=key_path,
        ca_filepath=ca_path,
        on_connection_interrupted=on_connection_interrupted,
        on_connection_resumed=on_connection_resumed,
        client_id=client_id,
        clean_session=False,
        keep_alive_secs=keepalive,
        http_proxy_options=None,
        on_connection_success=on_connection_success,
        on_connection_failure=on_connection_failure,
        on_connection_closed=on_connection_closed,
    )

    log.info(f"Connecting to {endpoint} with client ID '{client_id}'...")
    connect_future = mqtt_connection.connect()

    # Future.result() waits until a result is available
    connect_future.result()
    log.info("Connected!")

    tp = Tidepool().connect(creds.tidepool_creds.uname, creds.tidepool_creds.passwd)
    if tp is None:
        sys.exit("Couldn't connect to Tidepool")

    try:
        while True:
            val, timestamp = tp.bg()
            if val and timestamp:
                # TODO: Fast update publish
                log.info(
                    f"{datetime.now()}: Got new bg: {round(val)} from {timestamp.isoformat()}"
                )

                publish = (
                    abs(last_val - val) > (last_val * pub_thresh / 100.0)
                    if last_val
                    else True
                )

                if publish:
                    message = {}
                    message["value"] = round(val)
                    message["timestamp"] = timestamp.isoformat()
                    message_json = json.dumps(message)

                    log.info(
                        "    --> Publishing to '{}': {}".format(topic, message_json)
                    )
                    mqtt_connection.publish(
                        topic=topic,
                        payload=message_json,
                        qos=mqtt.QoS.AT_LEAST_ONCE,
                        retain=True,
                    )
                    last_val = val
            else:
                log.warning("Couldn't get bg")

            time.sleep(60 * poll_period)

    except KeyboardInterrupt:
        # Disconnect
        log.info("Disconnecting...")
        disconnect_future = mqtt_connection.disconnect()
        disconnect_future.result()
        log.info("Disconnected!")
