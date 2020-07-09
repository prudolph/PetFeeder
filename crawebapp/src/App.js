import React from 'react';
import './App.css';
import DatePicker from "react-datepicker";
import axios from "axios"
import "react-datepicker/dist/react-datepicker.css";
import ToggleButton from 'react-toggle-button'
import { Donut } from 'react-dial-knob'

class App extends React.Component {
  constructor(props) {
    super(props);
    this.state = {
      currentDate: new Date(),
      recurring: false,
      openPos: 0,
      closePos: 0,
      fetchedSettings: false
    };
    this.getOpenPosition = this.getOpenPosition.bind(this);
    this.getClosePosition = this.getClosePosition.bind(this);
this.setOpenPosition=this.setOpenPosition.bind(this);
this.setClosePosition=this.setClosePosition.bind(this)
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



  async setOpenPosition(isInteracting) {
    if(!this.state.fetchedSettings)return;
    return new Promise(async (resolve, reject) => {
      if (!isInteracting) {
        try{
          const response = await axios.post('/api/openpos?' + this.state.openPos)
          return resolve(response)
        }
        catch(error) {
          return reject(error)
        }
      }else{
        return resolve();
      }
    })
  }


  async getOpenPosition() {
    return new Promise(async(resolve, reject) => {
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
    if(!this.state.fetchedSettings)return;
    return new Promise(async(resolve, reject) => {

      if (!isInteracting) {
       try{
        const response = await axios.post('/api/closepos?' + this.state.closePos)
          return resolve(response)
        }
        catch(error) {
          return reject(error);
        }
      }
      else{
        return resolve();
      }
    })
  }

  async getClosePosition() {
    return new Promise(async(resolve, reject) => {
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
    console.log("Adding time: ");
  }

  renderAddDateEntry() {
    return <div>
      <DatePicker
        selected={this.state.currentDate}
        onChange={date => this.setState({ currentDate: date }, () => { console.log("updated: ", this.state.currentDate) })}
        showTimeSelect
        timeFormat="HH:mm"
        timeIntervals={15}
        timeCaption="time"
        dateFormat="MMMM d, yyyy h:mm aa"
      />

      <ToggleButton
        inactiveLabel={<div style={{ width: "50px", backgroundColor: "tomato" }}><p>Once</p></div>}
        activeLabel={<div style={{ width: "50px", backgroundColor: "cyan" }}><p>Daily</p></div>}
        value={this.state.recurring}
        onToggle={(value) => {
          console.log("button toggled: ", value)
          this.setState({ recurring: !this.state.recurring })
        }} />
      <button onClick={this.openFeeder}>Add Feeding Time</button>
    </div>

  }

  async componentDidMount() {

    try{
    await this.getOpenPosition();
    await this.getClosePosition();
    this.setState({
      fetchedSettings:true
    },()=>{
      console.log("Fetched Settings")
    })}
    catch(error){
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
            onInteractionChange={()=>this.setOpenPosition()}
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
            onInteractionChange={()=>this.setClosePosition()}
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
