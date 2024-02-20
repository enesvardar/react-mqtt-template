import { Socket } from 'socket.io';
import mqttClient from './mqtt';

const socketIO = (socket: Socket) => {
  console.log('socket io connect');

  socket.on('disconnect', () => {
    console.log('socket io disconnect');
  });

  socket.on('command', (data: { mac: string; cmd: string }) => {
    console.log(data);

    mqttClient.publish(
      process.env.MQTT_TOPIC_ESP + data.mac,
      String(data.cmd),
      {
        qos: 0,
        retain: false,
      }
    );
  });
};

export default socketIO;
