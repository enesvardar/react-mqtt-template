import express, { Express } from 'express';
import bodyParser from 'body-parser';
import mongoose from 'mongoose';
import dotenv from 'dotenv';
import cors from 'cors';
import { userRouter } from './router/userRouter';

const app: Express = express();

app.use(cors());
app.use(bodyParser.json({ limit: '30mb' }));
app.use(bodyParser.urlencoded({ limit: '30mb', extended: true }));
dotenv.config();

app.use('/', userRouter);

mongoose.set('strictQuery', false);
mongoose.connect(process.env.MONGO_URL as string).then(() => {
  console.log('Connected to MongoDB');
});

app.listen(3001, () => {
  console.log('Listining the Port');
});
