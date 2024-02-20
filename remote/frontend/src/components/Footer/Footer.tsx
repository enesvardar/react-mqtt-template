import { FooterContent, GitHubIcon, GitHubLink, Header } from "./Footer.styes";

const Footer = () => {
  return (
    <Header>
      <FooterContent>
        <GitHubLink href="https://github.com/enesvardar">
          <GitHubIcon src="github-sign.png" alt="GitHub Logo" />
          enesvardar
        </GitHubLink>
      </FooterContent>
    </Header>
  );
};

export default Footer;

