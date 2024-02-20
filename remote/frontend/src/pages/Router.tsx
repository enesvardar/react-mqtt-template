
import { BrowserRouter as Router, Route, Switch } from 'react-router-dom'
import styled from 'styled-components';
import Login from './Login';
import UserForm from './UserForm';

const Body = styled.div`
  width: 100vw;
  height: 100vh;
  overflow: hidden;
  display: flex;
  flex-direction: column;
`;

function App() {

  return (
    <Body>
      <Router>
        <Switch>

          <Route exact path="/">
            <Login />
          </Route>

          <Route path="/devices">
            <UserForm />
          </Route>

        </Switch>
      </Router>
    </Body>
  );
}

export default App;
