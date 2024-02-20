import { Socket, io } from "socket.io-client";
import { AppDispatch } from "../../redux/store";
import { useDispatch } from "react-redux";
import { deviceSetOnline } from "../../redux/userSlice";

export interface ISocketReaceveDataFrame {
  mac: number,
  cmd: string,
};

export interface ISocketSendDataFrame {
  mac: number,
  cmd: string,
};

export class SocketIO {

  socket: Socket;
  dispatch: any;

  constructor() {
    this.socket = io(process.env.REACT_APP_WEBSOCKET_ADDRESS || "");
  }

  init(_dispatch: AppDispatch) {
    this.dispatch = _dispatch;
    this.socket = io(process.env.REACT_APP_WEBSOCKET_ADDRESS || "");
    this.socket.on('message', (msg) => {
      this.msgResolver(msg)
    })
  }

  emit = (command: ISocketSendDataFrame) => {
    this.socket.emit("command", command);
  }

  sendPing = (mac: number) => {
    let command: ISocketSendDataFrame = { mac: mac, cmd: "CMD_PING" }
    this.emit(command);
  }

  msgResolver = (data: ISocketReaceveDataFrame) => {

    console.log(data.cmd)

    switch (data.cmd) {

      case "CMD_PING":

        this.dispatch(deviceSetOnline(data.mac));

        break;

      default:

        break;

    }
  }
}

export const socket: SocketIO = new SocketIO();












