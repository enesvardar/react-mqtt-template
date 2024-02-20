import mqtt from 'mqtt';

const mqttClient = mqtt.connect(
  `mqtt://${process.env.MQTT_HOST}:${process.env.MQTT_PORT}`,
  {
    clientId: process.env.MQTT_CLIENT_ID,
    clean: true,
    connectTimeout: 4000,
    username: 'emqx',
    password: 'public',
    reconnectPeriod: 1000,
  }
);

mqttClient.on('connect', () => {
  console.log('Connected MQTT');
  mqttClient.subscribe([process.env.MQTT_TOPIC_NODE as string]);
});

export default mqttClient;
