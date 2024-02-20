import styled from 'styled-components'

export const Container = styled.div`
  width: 20vw;
  height: 20vh;
  display: flex;
  justify-content: center;
  align-items: center;
  flex-direction: column;
  padding: 50px;
  border-radius: 50px;
  margin: auto;
`;

export const Img = styled.img`
  marginTop: "15px";
  width: 25%;
`;

export const Title = styled.h1`
  font-size: 24px;
  font-family: Arial,Helvetica,sans-serif;
  font-weight: normal;
`;

export const Form = styled.form`
  width: 80%;
  flex-direction: column;
  display: flex;
  justify-content: flex-end;  
`;

export const Input = styled.input`
  color: grey;
  font-size: 15px;
  padding: 15px;
  margin-top: 10px;
  `;

export const Button = styled.button`
  width: 30%;
  padding: 10px;
  border-radius: 10px;
  font-size: 15px;
  margin-top: 20px;
  align-self: flex-end; 
  background-color: #DFF5E9;
  `;