import { useFormik } from "formik";
import { useHistory } from "react-router-dom";
import { useDispatch } from "react-redux";
import { AxiosResponse } from "axios";
import { Button, Container, Form, Img, Input, Title } from "./Login.styes";
import * as useAxios from '../../hooks/useAxios'
import { login } from "../../redux/userSlice";
import { useEffect } from "react";

const Login = () => {

  const history = useHistory();
  const dispatch = useDispatch();

  const formik = useFormik({

    initialValues: {
      userName: "",
      password: "",
    },

    onSubmit: async (identity) => {

      const res: AxiosResponse = await useAxios.post("/", identity);

      if (res.status === 200) {
        const { user, token } = res.data;
        history.replace(`../devices`);
        dispatch(login({ user, token }));
      }
    },
  });

  useEffect(() => {
    localStorage.clear()
  }, [])

  return (
    <Container>
      <Img src="device.png"></Img>
      <Title> Sign in to Devices </Title>
      <Form onSubmit={formik.handleSubmit}>
        <Input
          id="userName"
          name="userName"
          type="text"
          onChange={formik.handleChange}
          value={formik.values.userName}
          placeholder="username"
        />
        <Input
          id="password"
          name="password"
          type="password"
          onChange={formik.handleChange}
          value={formik.values.password}
          placeholder="password"
        />
        <Button>Login</Button>
      </Form>
    </Container >
  );
};

export default Login;
