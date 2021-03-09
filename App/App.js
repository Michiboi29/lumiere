import { StatusBar } from 'expo-status-bar';
import React from 'react';
import { StyleSheet, Text, View, Strong, TouchableOpacity, TextInput } from 'react-native';
import { MaterialCommunityIcons } from '@expo/vector-icons';
import AppLoading from 'expo-app-loading';
import * as Font from 'expo-font';
import FlashMessage from "react-native-flash-message";
import { showMessage, hideMessage } from "react-native-flash-message";
import AsyncStorage from '@react-native-async-storage/async-storage';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';


let customFonts = {
  'Audiowide_400Regular': require("./assets/fonts/Audiowide-Regular.ttf")
};

async function fetchWithTimeout(resource, options) {
  const { timeout = 1000 } = options;
  
  const controller = new AbortController();
  const id = setTimeout(() => controller.abort(), timeout);

  const response = await fetch(resource, {
    ...options,
    signal: controller.signal  
  });
  clearTimeout(id);

  return response;
}

export default class App extends React.Component{
  constructor(){
    super();
    this.state = {
      fontsLoaded: false,
      lightState: "NAN", //important but fixed with fetch
      txtButton: "oups",
      colorButton: "orange",
      nameIcon1: "lightbulb-off",
      colorIcon1: "black",
      couleurConnection: 'orange',
      messageConnection: "Waiting...",
      urlIP: 'http://192.168.0.194', // important
      oldIP: 'http://192.168.0.194',
      alarmReleased: false,
      alarmHour: '0',
      alarmMin: '0',
      alarmTime: "8:00", //important
      alarm: true,
      nameIcon2: 'alarm',
      colorIcon2: 'white',
    }
    this.loadData();
    this._loadFontsAsync();
    this.getState();
    this.alarmInit();
  }
  async _loadFontsAsync() {
    await Font.loadAsync(customFonts);
    this.setState({ fontsLoaded: true });
  }

  connection(conn){
    if(conn === true){
      this.setState({
        couleurConnection: 'green',
        messageConnection: "Connected",
        // refressh old ip
        oldIP: this.state.urlIP,
      });
    }
    else{
      this.setState({
        couleurConnection: 'red',
        messageConnection: "No Connection",
        lightState: "OFF",
        txtButton: "oups",
        colorButton: "orange",
        nameIcon1: "lightbulb-off",
        colorIcon1: "black",
        // go back to old ip
        //urlIP: this.state.oldIP,
      });
    }
  }

  switchs(currentState) {
    this.setState({ lightState: currentState});
    if(currentState === "ON"){
      this.setState({
        txtButton: 'OFF',
        colorButton: 'grey',
        nameIcon1: "lightbulb-on-outline",
        colorIcon1: "#e8deb0"  
      });
    }
    else if(currentState === "OFF"){
      this.setState({
        txtButton: 'ON',
        colorButton: 'green',
        nameIcon1: "lightbulb-outline",
        colorIcon1: "#096677" 
      });
    }
  }

  async saveData(){
    try {
      await AsyncStorage.setItem('urlIP', this.state.urlIP);
      await AsyncStorage.setItem('alarmTime', this.state.alarmTime);
      const response = await fetchWithTimeout(this.state.urlIP+"/post-data", {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          Accept: '*/*',
          'Cache-Control': 'no-cache',
          'Accept-Encoding': 'gzip, deflate, br',
          Connection: 'keep-alive',},
        body: JSON.stringify({
          state: this.state.lightState,
          alarm: this.state.alarm,
          alarmTime: this.state.alarmTime,
        })
      });
      const data = await response.json();
      /*const responseFetch = await fetchWithTimeout(this.state.urlIP+"/state", {method: 'GET'});
      const dataFetch = await responseFetch.text();
      console.log("dataFetch => " + dataFetch);*/
    } catch (err) {
      // saving error
      console.error(err);
      showMessage({
        message: "Connection ...",
        description: "blah blah",
        type: "warning",
      });
    }
  }
  async loadData(){
    try {
      const valueIP = await AsyncStorage.getItem('urlIP');
      const valueTime = await AsyncStorage.getItem('alarmTime');
      if(valueIP !== null && valueTime !== null) {
        // value previously stored
        this.setState({
          urlIP: valueIP,
          alarmTime: valueTime,
        });
      }
    } catch (err) {
      // saving error
      console.error(err);
    }
  }

  async getState(){
    try{
      const response = await fetchWithTimeout(this.state.urlIP+"/state", {method: 'GET'});
      const data = await response.text();
      console.log("data => " + data);
      this.connection(true);
      this.switchs(data);
      this.saveData();
    }
    catch (err){
      this.connection(false);
      console.error(err);
      showMessage({
        message: "Connection ...",
        description: "blah blah",
        type: "danger",
      });
    }
  }

  async changeState(){ 
    try{
      let newState;
      if(this.state.lightState === "OFF"){
        newState = "ON";
      }
      else{
        newState = "OFF";
      }
      const response = await fetchWithTimeout(this.state.urlIP+"/state", {
        method: 'POST',
        headers: {'Content-Type': 'text/plain'},
        body: newState
      });
      const data = await response.text();

      this.saveData();
      this.connection(true);
      this.switchs(newState);
    }
    catch (err){
      this.connection(false);
      console.error(err);
      showMessage({
        message: "Connection ...",
        description: "blah blah",
        type: "warning",
      });
    }
  }

  alarmInit(){
    this.state.alarmHour = this.state.alarmTime.split(':')[0];
    this.state.alarmMin = this.state.alarmTime.split(':')[1];
    console.log("time:" + this.state.alarmTime);
    console.log("hour/min: " + this.state.alarmHour +"/"+ this.state.alarmMin);
    this.saveData();
  }


  handleURLInput = (text) => {
    let url = "http://" + text; 
    this.setState({
      urlIP: url
    })
  }

  handleAlarmInput = (text) => {
    console.log(text);
    let time = text;
    this.setState({
      alarmTime: time,
    })
  }

  handleAlarmTouch() {
    this.alarmInit();
    if(this.state.alarm){
      this.setState({
        alarm: false,
        nameIcon2: 'alarm-off',
        colorIcon2: 'black',
      });
    }
    else{
      this.setState({
        alarm: true,
        nameIcon2: 'alarm',
        colorIcon2: 'white',
      });
    }
  }

  render(){
    /*setInterval(() => {
      var today = new Date();
      var hour =  today.getHours();
      var min =  today.getMinutes();
      var sec =  today.getSeconds();
      var time = hour + ":" + min + ":" + sec
      console.log(time);
      if(hour === this.state.alarmHour && min === this.state.alarmMin && this.state.alarm === true){
        if(this.state.alarmReleased === false){
          if(this.state.lightState === "OFF"){
            console.log("boogaaa");
            this.changeState()
            this.state.alarmReleased = true;
          }
        }
      }
      else{this.state.alarmReleased = false;}
    }, 60000);*/
    if (this.state.fontsLoaded) {
    return (
    <View style={styles.page}>
      
      <View style={styles.container}>
        <Text style={styles.title}>Lumiere </Text>
        <MaterialCommunityIcons name={this.state.nameIcon1} size={82} color={this.state.colorIcon1} />
        <TouchableOpacity onPress={() => {this.changeState()}} style={[styles.button, {backgroundColor: this.state.colorButton}]}>
          <Text style={styles.buttonText}>{this.state.txtButton}</Text>
        </TouchableOpacity>
        <StatusBar style="auto" />
        <FlashMessage position="top" />
      </View>
      <View style={styles.connectionContainer}>
        <TouchableOpacity onPress={() => {this.getState()}}>
          <Text style={{color: this.state.couleurConnection}}>{this.state.messageConnection}</Text>
        </TouchableOpacity>
        <TextInput
          ref={input => { this.textInput = input }}
          style={{
            height: 30,
            color: 'grey',}}
          autoCorrect={false}
          placeholder = "New IP"
          value= {this.state.urlIP.split('/')[2]}
          placeholderTextColor = 'grey'
          onChangeText={this.handleURLInput}
          clearButtonMode='always'
          onEndEditing={() => {this.getState()}}
        />
      </View>
      <View style={styles.alarmContainer}>
        <TouchableOpacity onPress={() => {this.handleAlarmTouch()}}>
        <MaterialCommunityIcons name={this.state.nameIcon2} size={30} color={this.state.colorIcon2} />
        </TouchableOpacity>
        <TextInput
          ref={input2 => { this.textInput2 = input2 }}
          style={{
            height: 30,
            color: 'white',
            alignSelf: 'center',
            textAlign: 'center',}}
          autoCorrect={false}
          placeholder = "alarm clock"
          value= {this.state.alarmTime}
          placeholderTextColor = 'white'
          onChangeText={this.handleAlarmInput}
          clearButtonMode='always'
          onEndEditing={() => {this.alarmInit()}}
        />
      </View>
    </View>
    );
    } else {
      return <AppLoading />;
    }
  }
}

const styles = StyleSheet.create({
  page:{
    flex: 1,
    backgroundColor: '#40464B',
    alignItems: 'center',
    justifyContent: 'center',
  },
  container: {
    flex: 1,
    backgroundColor: '#40464B',
    alignItems: 'center',
    justifyContent: 'center',
  },
  title: {
    color: 'white',
    textAlign: 'center', 
    fontSize: 32,
    textTransform: 'uppercase',
    fontFamily:'Audiowide_400Regular',
    marginBottom: 50
  },
  button: {
    elevation: 15,
    height: 100,
    width: 200, 
    marginTop: 50,
    marginLeft: 1,
    marginRight: 1,
    borderRadius: 20,
    borderWidth: 2,
    borderColor: '#40464B'
  },
  buttonText:{
    flex: 1,
    color: "#fff",
    fontWeight: "bold",
    textTransform: "uppercase",
    alignSelf: "center",
    fontSize: 58,
    marginTop: 8,
  },
  connectionContainer: {
    position: 'absolute',
    bottom: 20,
    alignItems: 'center',
    justifyContent: 'center',
    marginTop: 100,
  },
  alarmContainer: {
    position: 'absolute',
    top: 50,
    alignItems: 'center',
    justifyContent: 'center',
  },
});
