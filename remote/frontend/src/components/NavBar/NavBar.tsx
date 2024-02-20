import { useHistory } from "react-router";
import { Button, Header, HeaderLine, Img } from "./NavBar.styes";
import NewDevice from '../NewDevice';

const NavBar = () => {

  const history = useHistory();

  function refreshPage() {
    window.location.reload();
  }

  return (
    <Header>
      <HeaderLine style={{ justifyContent: "space-between" }}>
        <Button style={{ backgroundColor: "#105b72c2" }}>Devices</Button>
        <Button onClick={() => { history.replace("/") }}>LOG OUT</Button>
      </HeaderLine>
      <HeaderLine>
        <Img src="/devices.png"></Img>
        <Button onClick={refreshPage} $border={false}>REFRESH</Button>
        <NewDevice />
      </HeaderLine>
    </Header>
  );
};

export default NavBar;

