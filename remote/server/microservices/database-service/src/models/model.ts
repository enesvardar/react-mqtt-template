import { model, Schema, Model, Document } from 'mongoose';

interface IDevice extends Document {
  name: string; // String is shorthand for {type: String}
  mac: number;
  type: string;
}

interface IUser extends Document {
  firstName: string; // String is shorthand for {type: String}
  lastName: string;
  userName: string;
  password: string;
  type: string;
  devices: IDevice[];
}

const UserSchema: Schema = new Schema({
  firstName: { type: String, required: true },
  lastName: { type: String, required: true },
  userName: { type: String, required: true },
  password: { type: String, required: true },
  type: { type: String, required: true },
  devices: { type: Array, required: true },
});

export const UserModal: Model<IUser> = model<IUser>('User', UserSchema);
