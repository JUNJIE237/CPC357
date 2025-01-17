import pymongo
import paho.mqtt.client as mqtt
from datetime import datetime, timezone

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
mqtt_topics = [("iot/temperature", 0), ("iot/heartbeat", 0)]  # Topics with QoS level 0

# Define the callback function for connection
def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print("Successfully connected to the MQTT broker")
        # Subscribe to multiple topics
        client.subscribe(mqtt_topics)
    else:
        print(f"Failed to connect, reason code: {reason_code}")

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

# Create a MQTT client instance
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
# Attach the callbacks using explicit methods
client.on_connect = on_connect
client.on_message = on_message

# Connect to MQTT broker
client.connect(mqtt_broker_address, 1883, 60)

# Start the MQTT loop
client.loop_forever()