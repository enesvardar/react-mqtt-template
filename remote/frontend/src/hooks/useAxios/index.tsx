import axios, { AxiosInstance, AxiosResponse } from 'axios';

export const post = async (page: string, data?: any): Promise<AxiosResponse> => {
  var instance: AxiosInstance;

  instance = axios.create({
    baseURL: `${process.env.REACT_APP_HTTP_ADDRESS}` + page,
    timeout: 3000,
    method: 'POST',
    headers:
      page === '/'
        ? {}
        : { token: JSON.parse(localStorage.getItem('token') || '') },
  });

  const result: AxiosResponse = await instance
    .request({ data })
    .then((req) => {
      return req;
    })
    .catch((err) => {
      return err.response;
    });

  return result;
};
