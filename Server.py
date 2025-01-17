import pymongo
import paho.mqtt.client as mqtt
from collections import deque
from datetime import datetime, timezone
import subprocess
import re
import csv
import os

# MongoDB configuration
try:
    # Connect to MongoDB
    mongo_client = pymongo.MongoClient('mongodb://localhost:27017/')
    db = mongo_client['test']  # Connect to the 'test' database
    collection = db['users']  # Access the 'users' collection
    print("Connected to MongoDB!")
except pymongo.errors.ConnectionFailure as e:
    print(f"Error connecting to MongoDB: {e}")

# MQTT configuration
mqtt_broker_address = "34.60.18.131"
mqtt_topics = [("temperature", 0), ("heartbeat", 0), ("health",0)]  # Topics with QoS level 0

# Initialize a circular buffer for heartbeat values
heartbeat_buffer = deque(maxlen=10)

# Define the callback function for connection
def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print("Successfully connected to the MQTT broker")
        # Subscribe to multiple topics
        client.subscribe(mqtt_topics)
    else:
        print(f"Failed to connect, reason code: {reason_code}")

# Function to write the buffer to a temporary CSV file
def save_to_csv(buffer):
    temp_csv_path = "GHMM Model/buffer.csv"  # Temporary CSV file name
    try:
        with open(temp_csv_path, mode="w", newline="") as file:
            writer = csv.writer(file)
            writer.writerow(list(buffer))  # Single row with buffer values
        print(f"Buffer saved to temporary CSV file: {temp_csv_path}")
    except Exception as e:
        print(f"Error writing to CSV file: {e}")

def get_log_likelihood(model_file, test_file):
    # Run mlpack_hmm_loglik with correct arguments
    result = subprocess.run(
        [
            "mlpack_hmm_loglik",
            "--input_file", test_file,
            "--input_model_file", model_file,
            #"--verbose", "false"
        ],
        capture_output=True,
        text=True
    )
    
    # Parse and return the log-likelihood
    try:
        print(result)
        match = re.search(r"log_likelihood:\s*(-?\d+\.?\d*)", result.stdout)
        if match:
            # Convert the matched value to a float and return it
            return float(match.group(1))
        #log_likelihood = float(result.stdout.strip())
        #return log_likelihood
    except ValueError:
        raise ValueError(f"Unexpected output: {result.stdout.strip()}")

# Define the callback function for ingesting data into MongoDB
def on_message(client, userdata, message):
    payload = message.payload.decode("utf-8")
    topic = message.topic
    print(f"Received message on topic {topic}: {payload}")
    
    # Convert MQTT timestamp to datetime
    timestamp = datetime.now(timezone.utc)
    datetime_obj = timestamp.strftime("%Y-%m-%dT%H:%M:%S.%fZ")

    # Process the payload and insert into MongoDB with the topic information
    document = {"timestamp": datetime_obj, "topic": topic, "data": payload}
    try:
        # Insert the data into MongoDB
        collection.insert_one(document)
        print("Data ingested into MongoDB")
    except Exception as e:
        print(f"Error inserting data into MongoDB: {e}")

    # Check if the topic is heartbeat and update the buffer
    if topic == "heartbeat":
        try:
            heartbeat_value = float(payload)  # Assuming heartbeat values are numeric
            heartbeat_buffer.append(heartbeat_value)  # Add to the buffer
            print(f"Updated heartbeat buffer: {list(heartbeat_buffer)}")
            if len(heartbeat_buffer) == heartbeat_buffer.maxlen:
                save_to_csv(heartbeat_buffer)
                healthy_ll = get_log_likelihood("GHMM Model/healthy_hmm.xml", "GHMM Model/buffer.csv")
                unhealthy_ll = get_log_likelihood("GHMM Model/unhealthy_hmm.xml","GHMM Model/buffer.csv")

                # Compare likelihoods
                if healthy_ll > unhealthy_ll:
                    prediction="Healthy"
                else:
                    prediction="Unhealthy"

                # Example usage
                print(f"Predicted class: {prediction}")
                prediction_result = {"timestamp":datetime_obj, "topic": "health","data":prediction}
                try:
                    # Insert the data into MongoDB
                    collection.insert_one(prediction_result)
                    print("Prediction result ingested into MongoDB")
                except Exception as e:
                    print(f"Error inserting prediction result into MongoDB: {e}")
        except ValueError:
            print(f"Invalid heartbeat value: {payload}")

# Create a MQTT client instance
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
# Attach the callbacks using explicit methods
client.on_connect = on_connect
client.on_message = on_message

# Connect to MQTT broker
client.connect(mqtt_broker_address, 1883, 60)

# Start the MQTT loop
client.loop_forever()