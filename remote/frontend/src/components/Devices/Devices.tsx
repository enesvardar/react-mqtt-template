import { useDispatch, useSelector } from "react-redux";
import { RootState } from "../../redux/store";
import { IUser } from "../../definitions/interfaces/user";
import {
  TableContainer,
  TableTitle,
  Container,
} from "./Devices.styes";
import { useEffect } from "react";
import { socket } from "../../hooks/useSocket";
import { deviceSetOffline } from "../../redux/userSlice";

const Devices = () => {

  const user: IUser = useSelector((state: RootState) => state.userReducer.user);
  const dispatch = useDispatch();

  useEffect(() => {

    user.devices.forEach(device => {
      dispatch(deviceSetOffline(device.mac));
      socket.sendPing(device.mac);
    })

  }, [])

  return (
    <Container>
      {user.devices.length > 0 ? (
        <>
          <TableTitle>
            <table>
              <thead>
                <tr>
                  <th>Name</th>
                  <th>Mac Address</th>
                  <th>Status</th>
                </tr>
              </thead>
            </table>
          </TableTitle>

          <TableContainer>
            {user.devices.map((device, index) => (
              <table key={index}>
                <thead>
                  <tr>
                    <th>{device.name.toUpperCase()}</th>
                    <th>{device.mac.toString()}</th>
                    <th>
                      {" "}
                      {device.online === false ? (
                        <img src="offline.png"></img>
                      ) : (
                        <img src="online.png"></img>
                      )}
                    </th>
                  </tr>
                </thead>
              </table>
            ))}
          </TableContainer>
        </>
      ) : (
        <></>
      )}
    </Container>
  );
};

export default Devices;
