import styled from "styled-components";

export const CustomStyles = {
  overlay: {
    backgroundColor: 'rgba(0, 0, 0, 0.5)'
  },
  content: {
    top: '50%',
    left: '50%',
    right: 'auto',
    bottom: 'auto',
    marginRight: '-50%',
    transform: 'translate(-50%, -50%)',
    border: 'none',
    borderRadius: '10px',
    backgroundColor: '#fff',
    boxShadow: '0px 0px 10px rgba(0, 0, 0, 0.3)',
    width: '15vw',
  }
};

export const ImgContainer = styled.div`
  display: flex;
  justify-content: center;
  align-items: center;
  width: 100%;
  height: 100%;
`;

export const InputContainer = styled.div`
  margin-bottom: 20px;
  input {
    padding: 8px;
    border: 1px solid #ccc;
    border-radius: 5px;
    width: 100%;
    font-size: 15px;
    box-sizing: border-box;
    color: #333; /* input içindeki metnin rengi */
    ::placeholder {
      color: #999;
    }
  }
`;

export const Button = styled.button<{ $border?: boolean }>`

  cursor: pointer;
  display: inline-block;
  text-decoration: none;
  text-transform: uppercase;
  border: none;
  font-family: Open Sans, Lucida Grande, Lucida, Verdana, sans-serif;
  border-radius: 1px;
  background-color: #00979d;
  color: white;
  font-size: 12px;
  line-height: 22px;
  font-weight: bold;
  letter-spacing: 0.5px;
  white-space: nowrap;
  font-style: normal;
  padding: 12px;
  font-weight: 800;

  &:hover {
    background-color: #69a4a5;
  }
`;

export const ButtonContainer = styled.div`
  display: flex;
  justify-content: space-between;
  Button {
    width: 30%;
    font-size: 12px;
    padding: 5px;
  }
`;