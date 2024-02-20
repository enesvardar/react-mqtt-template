import { createSlice, PayloadAction } from "@reduxjs/toolkit";
import { IUser } from "../definitions/interfaces/user";
import { IDevice } from "../definitions/interfaces/device";


function getLocalStorage(): IUser {

  if (localStorage.getItem("user") != null) {

    const user: IUser = JSON.parse(localStorage.getItem("user") || "")
    return user
  }
  else {

    const result: IUser = {
      _id: 0,
      devices: [],
      firstName: "",
      lastName: "",
      type: "user",
      userName: ""
    }

    return result;
  }
}

// Define a type for the slice state
export interface UserState {
  user: IUser
}

// Define the initial state using that type
const initialState: UserState = {
  user: getLocalStorage(),
}

// Define the initial state using that type
export const userSlice = createSlice({

  name: 'user',

  initialState,

  reducers: {

    login: (state, action: PayloadAction<{ user: IUser, token: string }>) => {

      const { user, token } = action.payload;
      state.user = user;

      localStorage.setItem("user", JSON.stringify(state.user));
      localStorage.setItem("token", JSON.stringify(token));
    },

    addDevice: (state, action: PayloadAction<{ mac: number, name: string }>) => {

      const { mac, name } = action.payload;
      let device: IDevice = new IDevice(name, mac);

      state.user.devices.push(device);
      localStorage.setItem("user", JSON.stringify(state.user));
    },

    deviceSetOnline: (state, action: PayloadAction<Number>) => {

      let index = state.user.devices.findIndex((element) => element.mac == action.payload);
      if (index != -1) {
        state.user.devices[index].online = true;
        localStorage.setItem("user", JSON.stringify(state.user));
      }
    },

    deviceSetOffline: (state, action: PayloadAction<number>) => {

      let index = state.user.devices.findIndex((element) => element.mac == action.payload);
      if (index != -1) {
        state.user.devices[index].online = false;
        localStorage.setItem("user", JSON.stringify(state.user));
      }
    },

  },
});

export const {
  login,
  addDevice,
  deviceSetOnline,
  deviceSetOffline,
} = userSlice.actions;

export default userSlice.reducer;
