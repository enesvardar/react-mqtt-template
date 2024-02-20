import dotenv from 'dotenv';
dotenv.config();
import { Server as SocketIOServer } from 'socket.io';
import http from 'http';
import socketIO from './protocols/socketIO';
import mqttClient from './protocols/mqtt';

const server = http.createServer();

server.listen(3002, () => {
  console.log('listening on *:3002 WEP');
});

const io = new SocketIOServer(server, {
  cors: {
    origin: '*',
    methods: ['GET', 'POST'],
  },
});

io.on('connection', socketIO);

mqttClient.on('message', function (topic: string, message: Buffer) {
  if (topic === process.env.MQTT_TOPIC_NODE) {
    const arr: string[] = message.toString().split('/');
    const mac: string = arr[0];

    const cmd: string = arr[1];

    io.emit('message', { mac, cmd });
  }
});
