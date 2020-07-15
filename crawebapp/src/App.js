import React from 'react';
import './App.css';
import DatePicker from "react-datepicker";
import axios from "axios"
import "react-datepicker/dist/react-datepicker.css";
import ToggleButton from 'react-toggle-button'
import { Donut } from 'react-dial-knob'

const SEC_IN_HOUR=3600;
const TIME_OFFSET_HOURS=-7;
class App extends React.Component {
  constructor(props) {
    super(props);
    this.state = {
      currentDate: new Date(),
      daily: false,
      openPos: 0,
      closePos: 0,
      fetchedSettings: false
    };
    this.getDeviceTime=this.getDeviceTime.bind(this);
    this.getOpenPosition = this.getOpenPosition.bind(this);
    this.getClosePosition = this.getClosePosition.bind(this);
    this.setOpenPosition = this.setOpenPosition.bind(this);
    this.setClosePosition = this.setClosePosition.bind(this);
    this.addFeedingTime = this.addFeedingTime.bind(this);
  }

  openFeeder() {
    console.log("Sending Open Feeder Request")
    axios.get('/api/open')
      .then(function (response) {
        console.log(response);
      })
      .catch(function (error) {
        console.log(error);
      })
  }

  closeFeeder() {
    console.log("Sending Close Feeder Request")
    axios.get('/api/close')
      .then(function (response) {
        console.log(response);
      })
      .catch(function (error) {
        console.log(error);
      })
  }

  dispenseFeeder() {
    console.log("Sending Dispense Feeder Request")
    axios.get('/api/dispense')
      .then(function (response) {
        console.log(response);
      })
      .catch(function (error) {
        console.log(error);
      })
  }


  async getDeviceTime() {
    return new Promise(async (resolve, reject) => {
      try {
       const response = await axios.get('/api/time')
        const deviceEpochSeconds = response.data.status
       
        const updatedTimeStamp=new Date(deviceEpochSeconds*1000);
        this.setState({ currentDate: updatedTimeStamp })
        resolve(new Date(deviceEpochSeconds*1000));
      }
      catch (error) {
        reject(error);
      }
    })
  }
  async setOpenPosition(isInteracting) {
    if (!this.state.fetchedSettings) return;
    return new Promise(async (resolve, reject) => {
      if (!isInteracting) {
        try {
          const response = await axios.post('/api/openpos?' + this.state.openPos)
          return resolve(response)
        }
        catch (error) {
          return reject(error)
        }
      } else {
        return resolve();
      }
    })
  }


  async getOpenPosition() {
    return new Promise(async (resolve, reject) => {
      try {
        const response = await axios.get('/api/openpos')
        const openPos = response.data.result
        this.setState({ openPos })
        resolve(openPos);
      }
      catch (error) {
        reject(error);
      }
    })
  }

  async setClosePosition(isInteracting) {
    if (!this.state.fetchedSettings) return;
    return new Promise(async (resolve, reject) => {

      if (!isInteracting) {
        try {
          const response = await axios.post('/api/closepos?' + this.state.closePos)
          return resolve(response)
        }
        catch (error) {
          return reject(error);
        }
      }
      else {
        return resolve();
      }
    })
  }

  async getClosePosition() {
    return new Promise(async (resolve, reject) => {
      try {
        const response = await axios.get('/api/closepos')
        const closePos = response.data.result
        this.setState({ closePos })
        return resolve(closePos);
      }
      catch (error) {
        console.log(error);
        return reject(error);
      }
    })
  }

  addFeedingTime() {
    return new Promise(async (resolve, reject) => {
        try {
          const timeOffestSeconds = TIME_OFFSET_HOURS*SEC_IN_HOUR;
          const feedingTimeStamp = Math.floor( (this.state.currentDate.getTime() / 1000) ) 
          console.log("Adding date: ", feedingTimeStamp);
          const response = await axios.post('/api/addfeeddate?' + feedingTimeStamp)
          return resolve(response)
        }
        catch (error) {
          return reject(error);
        }
    })
  }

  renderAddDateEntry() {
    return <div>
      <DatePicker
        selected={this.state.currentDate}
        onChange={date => this.setState({ currentDate: date }, () => { console.log("updated: ", this.state.currentDate) })}
        showTimeSelect
        showTimeSelectOnly={this.state.daily}
        timeFormat="hh:mm aa"
        timeIntervals={1}
        timeCaption="time"
        dateFormat={this.state.daily?"hh:mm aa" : "MMMM d, yyyy h:mm aa"}
      />

      <ToggleButton
        inactiveLabel={<div style={{ width: "50px", backgroundColor: "tomato" }}><p>Once</p></div>}
        activeLabel={<div style={{ width: "50px", backgroundColor: "cyan" }}><p>Daily</p></div>}
        value={this.state.daily}
        onToggle={(value) => {
          console.log("button toggled: ", value)
          this.setState({ daily: !this.state.daily })
        }} />

      <button onClick={this.addFeedingTime}>Add Feeding Time</button>
    </div>

  }

  async componentDidMount() {

    try {
      await this.getDeviceTime();
      await this.getOpenPosition();
      await this.getClosePosition();
   
   
      this.setState({
        fetchedSettings: true
      }, () => {
        console.log("Fetched Settings")
      })
    }
    catch (error) {
      console.log("Failed to fetch Settings: ", error)
    }
  }


  render() {
    return (
      <div>
        <button onClick={this.openFeeder}>Open</button>
        <button onClick={this.closeFeeder}>Close</button>
        <button onClick={this.dispenseFeeder}>Dispense</button>
        {this.state.fetchedSettings && <div className="config">
          <Donut
            diameter={200}
            min={-180}
            max={180}
            step={1}
            value={this.state.openPos}
            theme={{
              donutColor: 'tomato'
            }}
            onValueChange={(value) => this.setState({ openPos: value })}
            onInteractionChange={() => this.setOpenPosition()}
            ariaLabelledBy={'open-label'}
          >
            <label id={'open-label'}>FeederOpenPosition</label>
          </Donut>

          <Donut
            diameter={200}
            min={-180}
            max={180}
            step={1}
            value={this.state.closePos}
            theme={{
              donutColor: 'cyan'
            }}
            onValueChange={(value) => this.setState({ closePos: value })}
            onInteractionChange={() => this.setClosePosition()}
            ariaLabelledBy={'close-label'}
          >
            <label id={'close-label'}>FeederClosePosition</label>
          </Donut>
        </div>}
        {this.renderAddDateEntry()}

      </div>
    );
  }
}

export default App;
