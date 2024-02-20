import styled from "styled-components";

export const Header = styled.div`
  width: 100vw;
  background-color: #00979d;
  position: fixed;
  left: 0;
  bottom: 0;

`;

export const FooterContent = styled.div`
  padding: 20px;
`;

export const GitHubLink = styled.a`
  display: flex;
  align-items: center;
  color: white;
  text-decoration: none;
  font-family: Arial, sans-serif; /* Yazı tipini ayarlayabilirsiniz */
  font-weight: bold;
  font-style: italic;
`;

export const GitHubIcon = styled.img`
  width: 30px; 
  height: 30px; 
  margin-right: 10px; 
`;