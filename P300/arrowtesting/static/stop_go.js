const sstop = document.getElementById("s");
const cstop = sstop.getContext("2d");
const sgo = document.getElementById("g");
const cgo = sgo.getContext("2d");
const sgo2 = document.getElementById("g2");
const cgo2 = sgo2.getContext("2d");

const _idoptions = ["s", "g", "g2"];
const _arrows = ["stop", "go", "go2"];
const _sequence1 = [];
const _sequence2 = [];
const _sequence3 = [];
var _root = document.querySelector(':root');

const start_time = performance.now() * 1000;

function sendData(time, id) {
    var data = [time, id];
    fetch('/localBCI', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify({data: data})
      })
      .then(response => response.text())
      .then(result => {
        console.log(result);
      })
      .catch(error => {
        console.error('Error:', error);
      });
}

const _canvases = [cstop, cgo, cgo2];
const defaultColour = "white";
function pickSequence(array, array2){
    //decide sequence in which to flash
    let temp = [];
    array.forEach(element => {
        temp.push(element);
    });
    console.log("temp end: " + temp[temp.length - 1]);
    array2.length = 0;
    array.length = 0;
    let i =0;
    while (i < _idoptions.length){
        let index = Math.floor(Math.random() * _idoptions.length);
        if ((i == 0 && temp[temp.length - 1] == _idoptions[index]) || array.includes(_idoptions[index])){
            continue;
        } else {
            array.push(_idoptions[index]);
            array2.push(_canvases[index]);
            i++;
        }
    }
    
    array.unshift(array[0]);
    array2.unshift(array2[0]);
    console.log("array: " + array);
    console.log("arrows: " + array2);
}
function flashSequence(array, array2){
    //flash the arrows
        pickSequence(array, array2);
        console.log("array at flash: " + array);
        let i = 0;
    let interval = setInterval(function(){
        if (i <array2.length){
            
            console.log("start i: " + array[i]);
            console.log(performance.now() * 1000- start_time);

            if (i>0){
            sendData((performance.now() * 1000 - start_time).toFixed(10), array[i]);
                }
            document.getElementById(array[i]).style.backgroundColor = "black";
            array2[i].fillStyle = "white";
            array2[i].fill();
            setTimeout(function(){
                document.getElementById(array[i]).style.backgroundColor = "white";
                array2[i].fillStyle = "black";
                array2[i].fill();
            }, 100);
            console.log("end i: " + array[i]);
            console.log(performance.now() * 1000 - start_time);
            i++;
        } else {
            draw();
            clearInterval(interval);
        }
    }, (300));
    console.log("end sequence: " + array[i]);
    console.log(performance.now() * 1000 - start_time);
}

function draw(){
cgo.beginPath();
cgo.moveTo(50, 95);
cgo.lineTo(50, 45);
cgo.lineTo(20, 45);
cgo.lineTo(60, 5);
cgo.lineTo(100, 45);
cgo.lineTo(70, 45);
cgo.lineTo(70, 95);
cgo.lineTo(45, 95);
cgo.lineWidth = 10;
cgo.stroke();
cgo.fillStyle = "white";
cgo.fill();
document.getElementById("g").style.backgroundColor = "black";


cgo2.beginPath();
cgo2.moveTo(50, 95);
cgo2.lineTo(50, 45);
cgo2.lineTo(20, 45);
cgo2.lineTo(60, 5);
cgo2.lineTo(100, 45);
cgo2.lineTo(70, 45);
cgo2.lineTo(70, 95);
cgo2.lineTo(45, 95);
cgo2.lineWidth = 10;
cgo2.stroke();
cgo2.fillStyle = "white";
cgo2.fill();
document.getElementById("g2").style.backgroundColor = "black";


cstop.beginPath();
cstop.moveTo(42, 20);
cstop.lineTo(80, 20);
cstop.lineTo(100, 40);
cstop.lineTo(100, 70);
cstop.lineTo(80, 90);
cstop.lineTo(45, 90);
cstop.lineTo(25, 70);
cstop.lineTo(25, 40);
cstop.lineTo(45, 20);
cstop.lineWidth = 10;
cstop.strokeStyle = "white";
cstop.stroke();
cstop.fillStyle = "white";
cstop.fill();
document.getElementById("s").style.backgroundColor = "black";
}

function doTheThing(){
    if(sessionStorage.getItem('pright')){
    _root.style.setProperty('--pright', sessionStorage.getItem('pright'));}
    if(sessionStorage.getItem('pleft')){
    _root.style.setProperty('--pleft', sessionStorage.getItem('pleft'));}
    if(sessionStorage.getItem('top')){
    _root.style.setProperty('--top', sessionStorage.getItem('top'));}
    if(sessionStorage.getItem('mleft')){
    _root.style.setProperty('--mleft', sessionStorage.getItem('mleft'));}
    if(sessionStorage.getItem('mright')){
    _root.style.setProperty('--mright', sessionStorage.getItem('mright'));}

    sendData(performance.now() * 1000 - start_time);
    console.log("block start: " + performance.now() * 1000-start_time);
    for(let i = 0; i<8; i++){
    setTimeout(function(){flashSequence(_sequence1, _sequence2); }, (1260*i+(Math.random()*150)));}
    console.log("cycle " + i + "end: " + performance.now() * 1000-start_time);    
}
function toggleFullScreen() {
    if (!document.fullscreenElement) {
      document.documentElement.requestFullscreen();
    } else if (document.exitFullscreen) {
      document.exitFullscreen();
    }
  }
  document.addEventListener(
    "keydown",
    (e) => {
      if (e.key === "Enter") {
        toggleFullScreen();
      }
    },
    false,
  );

addEventListener('DOMContentLoaded', draw());
addEventListener('DOMContentLoaded', doTheThing());
