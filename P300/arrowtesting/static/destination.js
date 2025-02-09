var _root = document.querySelector(':root');
var map0 = document.getElementById("map0");
var map1 = document.getElementById("map1");
var map2 = document.getElementById("map2");
var map3 = document.getElementById("map3");
var map4 = document.getElementById("map4");
var dotsArray = [[]];
const _idoptions = [0, 1, 2, 3];

const _sequence1 = [];
const _sequence2 = [];
const _sequence3 = [];
var start_time;

function drawDots(){
//dots.style.top = '40%';
//dots.style.left = '20%';
map1.style.opacity = "0";
map2.style.opacity = "0";
map3.style.opacity = "0";
map4.style.opacity = "0";

//getData();
}

function getData() {
    fetch('/dotcoords', {
    method: 'GET',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({data: dotsArray})
  })
  .then(response => response.text())
  .then(result => {
    console.log("\n \n \n HERE'S THE THING: " + result+ "\n \n \n");
  })
  .catch(error => {
    console.error('Error:', error);
  });      
}

function sendData(time, id, target) {
  var data = [time, id, target];
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

const _canvases = [map1, map2, map3, map4];
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
  console.log("dots: " + array2);
}

function flashSequence(array, array2){
  //flash the arrows
  //for(let j = 0; j < 3; j++){
      pickSequence(array, array2);
      //array2[0].fillStyle = "black";
      //array2[0].fill()
      console.log("array at flash: " + array);
      let i = 0;
  let interval = setInterval(function(){
      if (i <array2.length){
          
          console.log("start i: " + array[i]);
          console.log(performance.now()- start_time);

          if (i>0){
          sendData((performance.now() - start_time).toFixed(10), array[i], -1);
              }
          array2[i].style.opacity = "0";
          setTimeout(function(){
            array2[i].style.opacity = "1";
          }, 100);
          console.log("end i: " + array[i]);
          console.log(performance.now() - start_time);
          i++;
        /*  if (array[i] == "ca"){
              l45.width +=15;
              drawArrows();
          }*/
      } else {
          clearInterval(interval);
      }
  }, (300));
  console.log("end sequence: " + array[i]);
  console.log(performance.now() - start_time);
}

function doTheThing(){
    if(sessionStorage.getItem('pright')){
    _root.style.setProperty('--pright', sessionStorage.getItem('pright'));}
    if(sessionStorage.getItem('pleft')){
    _root.style.setProperty('--pleft', (sessionStorage.getItem('pleft')));}
    if(sessionStorage.getItem('top') == '40%'){
        _root.style.setProperty('--top', '20%')}
    else if (sessionStorage.getItem('top') == '20%'){
        _root.style.setProperty('--top', '10%')
    }
    else if (sessionStorage.getItem('top')){
    _root.style.setProperty('--top', (sessionStorage.getItem('top')));}
    if(sessionStorage.getItem('mleft')){
    _root.style.setProperty('--mleft', (sessionStorage.getItem('mleft')));}
    if(sessionStorage.getItem('mright')){
    _root.style.setProperty('--mright', (sessionStorage.getItem('mright')));}
    if (sessionStorage.getItem('location') == 'centre'){
        _root.style.setProperty('--top', '10%');
        _root.style.setProperty('--pright', '-10%');
        _root.style.setProperty('--mright', '-10%');
        _root.style.setProperty('--pleft', '10%');
        _root.style.setProperty('--mleft', '10%');}
        
    if (sessionStorage.getItem('location') == 'topleft'){
      _root.style.setProperty('--top', '0%');
      _root.style.setProperty('--pright', '60%');
      _root.style.setProperty('--mright', '60%');
      _root.style.setProperty('--pleft', '-15%');
      _root.style.setProperty('--mleft', '-15%');}
        
    if (sessionStorage.getItem('location') == 'topright'){
      _root.style.setProperty('--top', '0%');
      _root.style.setProperty('--pright', '-15%');
      _root.style.setProperty('--mright', '-15%');
      _root.style.setProperty('--pleft', '29%');
      _root.style.setProperty('--mleft', '29%');}
        
      if (sessionStorage.getItem('location') == 'bottomleft'){
        _root.style.setProperty('--top', '20%');
        _root.style.setProperty('--pright', '60%');
        _root.style.setProperty('--mright', '60%');
        _root.style.setProperty('--pleft', '-15%');
        _root.style.setProperty('--mleft', '-15%');}


        start_time = performance.now();       
    }
    function randomFlash(){sendData(performance.now() - start_time, -1);
    console.log("block start: " + performance.now() - start_time);
    for(let i = 0; i<3; i++){
    setTimeout(function(){flashSequence(_sequence1, _sequence2); /*console.log("cycle "+ i+ " start: " + Date.now()-start_time);*/}, (2000*i+(Math.random()*150+50)));}//}, 2500);*/
    console.log("cycle " + i + "end: " + performance.now() - start_time);
        //flashSequence(_sequence3);
    
    }
    function toggleFullScreen() {
        if (!document.fullscreenElement) {
          document.documentElement.requestFullscreen();
        } else if (document.exitFullscreen) {
          document.exitFullscreen();
        }
      }

      function simulateKeyPress(key) {
        const event = new KeyboardEvent('keydown', {key});
        textField.dispatchEvent(event);
      }

      document.addEventListener(
        "keydown",
        (e) => {
          if (e.key === "a") {
            toggleFullScreen();
          }
        },
        false,
      );
    

    addEventListener('DOMContentLoaded', drawDots());
   // addEventListener('DOMContentLoaded', simulateKeyPress('a'));
    addEventListener('DOMContentLoaded', doTheThing());
    addEventListener('DOMContentLoaded', randomFlash());

