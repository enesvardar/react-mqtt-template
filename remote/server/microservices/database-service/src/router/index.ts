import { Router } from 'express';
import { userRouter } from './userRouter';

const router: Router = Router();

router.use('/', userRouter);

module.exports = router;
