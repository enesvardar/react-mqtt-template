import styled from "styled-components";

export const Header = styled.div`
  width: 100vw;
  background-color: #00979d;
  align-self: flex-start;
  display: flex;
  flex-direction: column;
`;

export const HeaderLine = styled.div`
  width: 100vw;
  align-self: flex-start;
  border-bottom: solid white;
  display: flex;
  flex-direction: row;
  justify-content: center;
  align-items: center;
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
export const Img = styled.img`
  position: absolute;
  left: 20px;
  width: 50px;
`;
