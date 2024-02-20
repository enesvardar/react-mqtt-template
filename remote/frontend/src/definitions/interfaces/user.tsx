import { IDevice } from "./device";

export interface IUser {
    firstName: string;
    lastName: string;
    userName: string;
    type: string;
    devices: IDevice[];
    _id: number;
};







