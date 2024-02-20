import { Request, Response } from 'express';
import jwt from 'jsonwebtoken';
import bcrypt from 'bcrypt';
import { UserModal } from '../models/model';

const SALT = '$2b$10$Tqk2X.YYAkJZcUauOYUvWu';

export const refresh = async (req: Request, res: Response) => {
  res.status(200).json();
};

export const signIn = async (req: Request, res: Response) => {
  const { userName, password } = req.body;

  try {
    const user: any = await UserModal.findOne({ userName });

    if (!user) {
      return res.status(404).send('User not found');
    }

    const passwordMatch = await bcrypt.compare(password, user.password);

    if (!passwordMatch) {
      return res.status(404).send('Sorry, your password was wrong.');
    }

    const token = jwt.sign(
      { userName: user.userName },
      process.env.JWT_KEY as string
    );
    res.status(200).json({ user, token });
  } catch (error) {
    res.status(500).send('Internal server error');
  }
};

export const newDevice = async (req: Request, res: Response) => {
  const { _id, mac, name } = req.body;

  try {
    const user: any = await UserModal.findById({ _id });
    user.devices.push({
      name: name,
      mac: mac,
      online: false,
    });
    await user.save();
    res.status(200).send('');
  } catch {
    res.status(404).send('Data base not reachable');
  }
};
