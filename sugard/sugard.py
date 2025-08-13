from awscrt import mqtt
from awsiot import mqtt_connection_builder
from tidepool import Tidepool
from datetime import datetime

import sys
import time
import json

import creds.tidepool_creds

cred_path = "creds/"
endpoint = "a3tnwpx5rjol8f-ats.iot.us-east-1.amazonaws.com"
client_id = "glucoseSource"
topic = "glucose/value"
poll_period = 1  # minutes
max_pub_period = 15  # minutes
pub_thresh = 10  # percent
keepalive = 20 * 60  # max allowed

cert_path = cred_path + "data_server.cert.pem"
ca_path = cred_path + "root-CA.crt"
key_path = cred_path + "data_server.private.key"

last_val = None


# Callback when connection is accidentally lost.
def on_connection_interrupted(connection, error, **kwargs):
    print("Connection interrupted. error: {}".format(error))


# Callback when an interrupted connection is re-established.
def on_connection_resumed(connection, return_code, session_present, **kwargs):
    print(
        "Connection resumed. return_code: {} session_present: {}".format(
            return_code, session_present
        )
    )

    if return_code == mqtt.ConnectReturnCode.ACCEPTED and not session_present:
        print("Session did not persist. Resubscribing to existing topics...")
        resubscribe_future, _ = connection.resubscribe_existing_topics()

        # Cannot synchronously wait for resubscribe result because we're on the connection's event-loop thread,
        # evaluate result with a callback instead.
        resubscribe_future.add_done_callback(on_resubscribe_complete)


def on_resubscribe_complete(resubscribe_future):
    resubscribe_results = resubscribe_future.result()
    print("Resubscribe results: {}".format(resubscribe_results))

    for topic, qos in resubscribe_results["topics"]:
        if qos is None:
            sys.exit("Server rejected resubscribe to topic: {}".format(topic))


# Callback when the subscribed topic receives a message
def on_message_received(topic, payload, dup, qos, retain, **kwargs):
    print("Received message from topic '{}': {}".format(topic, payload))


# Callback when the connection successfully connects
def on_connection_success(connection, callback_data):
    assert isinstance(callback_data, mqtt.OnConnectionSuccessData)
    print(
        "Connection Successful with return code: {} session present: {}".format(
            callback_data.return_code, callback_data.session_present
        )
    )


# Callback when a connection attempt fails
def on_connection_failure(connection, callback_data):
    assert isinstance(callback_data, mqtt.OnConnectionFailureData)
    print("Connection failed with error code: {}".format(callback_data.error))


# Callback when a connection has been disconnected or shutdown successfully
def on_connection_closed(connection, callback_data):
    print("Connection closed")


if __name__ == "__main__":
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

    print(f"Connecting to {endpoint} with client ID '{client_id}'...")
    connect_future = mqtt_connection.connect()

    # Future.result() waits until a result is available
    connect_future.result()
    print("Connected!")

    tp = Tidepool().connect(creds.tidepool_creds.uname, creds.tidepool_creds.passwd)
    if tp is None:
        sys.exit("Couldn't connect to Tidepool")

    try:
        while True:
            val, timestamp = tp.bg()
            if val and timestamp:
                # TODO: Fast update publish
                print(
                    f"{datetime.now()}: Got new bg: {round(val)} from {timestamp.isoformat()}"
                )

                publish = (
                    abs(last_val - val) > (last_val * pub_thresh / 100.0)
                    if last_val
                    else True
                )

                if publish:
                    message = f'{{"value": {round(val)}, "timestamp": {timestamp.isoformat()}}}'
                    print("    --> Publishing to '{}': {}".format(topic, message))
                    message_json = json.dumps(message)
                    mqtt_connection.publish(
                        topic=topic, payload=message_json, qos=mqtt.QoS.AT_LEAST_ONCE
                    )
                    last_val = val
            else:
                print("Couldn't get bg")

            time.sleep(60 * poll_period)

    except KeyboardInterrupt:
        # Disconnect
        print("Disconnecting...")
        disconnect_future = mqtt_connection.disconnect()
        disconnect_future.result()
        print("Disconnected!")
