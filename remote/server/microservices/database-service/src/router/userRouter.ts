import express from 'express';
import { verifyToken } from '../mildware/verify-token';
import { newDevice, refresh, signIn } from '../controllers/userControllers';

export const userRouter = express.Router();

userRouter.post('/', signIn);
userRouter.post('/refresh', verifyToken, refresh);
userRouter.post('/newDevice', verifyToken, newDevice);
