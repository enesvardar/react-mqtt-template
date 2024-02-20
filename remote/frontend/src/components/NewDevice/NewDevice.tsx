import React, { useState } from "react";
import Modal from "react-modal";
import {
  Button,
  ButtonContainer,
  CustomStyles,
  ImgContainer,
  InputContainer,
} from "./NewDevice.styes";
import { AxiosResponse } from "axios";
import * as useAxios from "../../hooks/useAxios";
import { RootState } from "../../redux/store";
import { useDispatch, useSelector } from "react-redux";
import { IUser } from "../../definitions/interfaces/user";
import { addDevice } from "../../redux/userSlice";


const NewDevice = () => {

  const user: IUser = useSelector((state: RootState) => state.userReducer.user);
  const [isModalOpen, setIsModalOpen] = useState(false);

  const [_mac, setMacAddress] = useState("");
  const [_name, setDeviceName] = useState("");
  const dispatch = useDispatch();

  const handleSubmit = async () => {
    const _id = user._id;
    const res: AxiosResponse = await useAxios.post("/newDevice", {_id, mac: parseInt(_mac), name: _name});

    if (res.status === 200) {
      dispatch(addDevice({mac: parseInt(_mac) , name: _name }));
    }

    setIsModalOpen(false);
  };


  return (
    <>
      <Button onClick={()=>{setIsModalOpen(true)}} $border={false}>ADD DEVICES</Button>
      <Modal isOpen={isModalOpen} onRequestClose={()=>{setIsModalOpen(false)}} style={CustomStyles}>
        <ImgContainer>
          <img src="device.png"/>
        </ImgContainer>
        <InputContainer>
          <input
            type="text"
            value={_mac}
            onChange={(e)=>{setMacAddress(e.target.value)}}
            placeholder="Mac Address"
          />
        </InputContainer>
        <InputContainer>
          <input
            type="text"
            value={_name}
            onChange={(e)=>{setDeviceName(e.target.value)}}
            placeholder="Device Name"
          />
        </InputContainer>
        <ButtonContainer>
          <Button onClick={handleSubmit}>Add</Button>
          <Button onClick={()=>{setIsModalOpen(false)}}>Close</Button>
        </ButtonContainer>
      </Modal>
    </>
  );
};

export default NewDevice;
