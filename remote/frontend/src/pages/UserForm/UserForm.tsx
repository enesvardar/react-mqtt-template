import { useEffect } from "react";
import { useHistory } from "react-router-dom";
import { AxiosResponse } from "axios";
import * as useAxios from "../../hooks/useAxios";
import Footer from "../../components/Footer";
import NavBar from "../../components/NavBar";
import Devices from "../../components/Devices";
import { useDispatch } from "react-redux";
import { socket } from "../../hooks/useSocket";

const UserForm = () => {
  const history = useHistory();
  const dispatch = useDispatch();

  const pageRefresh = async () => {
    const res: AxiosResponse = await useAxios.post("/refresh");

    if (res.status !== 200) {
      localStorage.clear();
      history.push("../");
    }
  };

  useEffect(() => {
    socket.init(dispatch)
    pageRefresh();
  }, []);

  return (
    <>
      <NavBar />
      <Devices />
      <Footer />
    </>
  );
};

export default UserForm;
