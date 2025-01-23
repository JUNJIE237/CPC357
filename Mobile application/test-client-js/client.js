const WebSocket = require('ws');
const socket = new WebSocket("ws://10.207.200.190/ws");

socket.onopen = () => {
    console.log("Connected to the server.");
};

socket.onmessage = (event) => {
    const data = JSON.parse(event.data);
    if (data.status === "fall detected") {
        console.log("Alert: Status is not safe!");
        // Take necessary action
    }
};

socket.onclose = () => {
    console.log("Disconnected from the server.");
};
