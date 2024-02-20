import { NextFunction, Request, Response } from 'express';
import jwt from 'jsonwebtoken';

export const verifyToken = (
  req: Request,
  res: Response,
  next: NextFunction
) => {
  const token: string = req.headers.token as string;

  if (!token) return res.status(404).json({ msg: 'No token provided' });

  jwt.verify(token, process.env.JWT_KEY as string, (err, decoded) => {
    if (err)
      return res.status(404).json({ msg: 'Failed to authenticate token' });
    next();
  });
};
