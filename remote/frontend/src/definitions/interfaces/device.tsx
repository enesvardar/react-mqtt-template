
export class IDevice {
  name: string;
  mac: number;
  online: boolean;

  constructor(name: string, mac: number) {
    this.name = name;
    this.mac = mac;
    this.online = false;
  }
}