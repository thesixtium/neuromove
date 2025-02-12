const _idoptions = ["a", "b", "c", "d", "e"];
const _arrows = [/*"ca",*/ "cb", /*"cc",*/ "cd", "ce", "cf"/*, "cg"*/, "ch"/*, "ci"*/];
const _sequence1 = [];
const _sequence2 = [];
const _sequence3 = [];
var _root = document.querySelector(':root');
var start_time;
var numcycles = 20;
var j = 0;
var k = 0;

const fwd = document.getElementById("cb");
const cfwd = fwd.getContext("2d");
const l = document.getElementById("cd");
const cl = l.getContext("2d");
const stop = document.getElementById("ce");
const cstop = stop.getContext("2d");
const r = document.getElementById("cf");
const cr = r.getContext("2d");
const sw = document.getElementById("ch");
const csw = sw.getContext("2d");
document.getElementById("startbox").style.opacity = '1';

switchClick = function() {
    console.log("switch mode!");
    window.location.href = "/destination";
}

function sendData(time, id, target) {
    var data = [time, id, target];
    fetch('/outputpls', {
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

const _canvases = [cfwd, cl, cstop ,cr, csw ];
const defaultColour = "white";
function pickSequence(array, array2){
    //decide sequence in which to flash
    let temp = [];
    array.forEach(element => {
        temp.push(element);
    });
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
function flashSequence(array, array2, train_target){

    //flash the arrows
        pickSequence(array, array2);
        let i = 0;
    let interval = setInterval(function(){
      
        if (i <array2.length){

            if (i>0){
            sendData((performance.now() - start_time).toFixed(10), (array[i]).codePointAt(0)-97, train_target);
                }
            document.getElementById(array[i]).style.backgroundColor = "black";
            array2[i].fillStyle = "white";
            array2[i].fill();
            setTimeout(function(){
                document.getElementById(array[i]).style.backgroundColor = "white";
                array2[i].fillStyle = "black";
                array2[i].fill();
            }, 100);
            i++;
        } else {
            drawArrows();
            clearInterval(interval);
        }
      
    }, (300));
}

function drawArrows(){

    cfwd.beginPath();
    cfwd.moveTo(40, 95);
    cfwd.lineTo(40, 45);
    cfwd.lineTo(10, 45)
    cfwd.lineTo(50, 5);
    cfwd.lineTo(90, 45);
    cfwd.lineTo(60, 45);
    cfwd.lineTo(60, 95);
    cfwd.lineTo(35, 95);
    cfwd.lineWidth = 10;
    cfwd.stroke();
    cfwd.fillStyle = "white";
    cfwd.fill();
    document.getElementById("a").style.backgroundColor = "black";

    cl.beginPath();
    cl.moveTo(90, 70);
    cl.lineTo(40, 70);
    cl.lineTo(40, 90);
    cl.lineTo(5, 60);
    cl.lineTo(40, 30);
    cl.lineTo(40, 50);
    cl.lineTo(90, 50);
    cl.lineTo(90, 75);
    cl.lineWidth = 10;
    cl.stroke();
    cl.fillStyle = "white";
    cl.fill();
    document.getElementById("b").style.backgroundColor = "black";

    cr.beginPath();
    cr.moveTo(10, 70);
    cr.lineTo(60, 70);
    cr.lineTo(60, 90);
    cr.lineTo(95, 60);
    cr.lineTo(60, 30);
    cr.lineTo(60, 50);
    cr.lineTo(10, 50);
    cr.lineTo(10, 75);
    cr.lineWidth = 10;
    cr.stroke();
    cr.fillStyle = "white";
    cr.fill();
    document.getElementById("d").style.backgroundColor = "black";

    cstop.beginPath();
    cstop.moveTo(32, 25);
    cstop.lineTo(70, 25);
    cstop.lineTo(90, 45);
    cstop.lineTo(90, 75);
    cstop.lineTo(70, 95);
    cstop.lineTo(35, 95);
    cstop.lineTo(15, 75);
    cstop.lineTo(15, 45);
    cstop.lineTo(35, 25); 
    cstop.lineWidth = 10;
    cstop.stroke();
    cstop.fillStyle = "white";
    cstop.fill();
    document.getElementById("c").style.backgroundColor = "black";

    csw.beginPath();
    csw.arc(30, 50, 40, Math.PI+1.5, Math.PI/2);
    csw.lineTo(20, 90);
    csw.lineTo(20, 10);
    csw.lineTo(30, 10);
    csw.lineWidth = 10;
    csw.stroke();
    csw.fillStyle = "white";
    csw.fill();
    document.getElementById("e").style.backgroundColor = "black";
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
    
    start_time = performance.now();
    }


function changeBox(){
  if (j == 5){
    sendData((performance.now() - start_time).toFixed(10), "Training Complete", -1);      
    document.getElementById("starttext").innerHTML = "Training complete. <br> Press enter to <br> go to local driving.";
  }
  else if (j == 1){
      document.getElementById("starttext").innerHTML = "Press Enter to <br>continue. <br>Target now <.";
    }
  else if (j == 2){
      document.getElementById("starttext").innerHTML = "Press Enter to <br>continue. <br>Target now STOP.";
    }
  else if (j == 3){
      document.getElementById("starttext").innerHTML = "Press Enter to <br>continue. <br>Target now >.";
    }
  else if (j == 4){
      document.getElementById("starttext").innerHTML = "Press Enter to <br>continue. <br>Target now D.";
    }

    document.getElementById("startbox").style.opacity = '1';
}

function flashStuff(){
  document.getElementById("startbox").style.opacity = '0';
  sendData((performance.now() - start_time).toFixed(10), "Trial Started", -1);    
  k = 0;
  let m = 0;
  for (index = 0; index <= numcycles; index++){
    setTimeout(function(){
      if (k <numcycles){
      flashSequence(_sequence1, _sequence2, j);
      k++;
      }
      if (k == numcycles){
        m++;
      }
      console.log("k = " + k);
      console.log("m = " + m);
        if (k ==  numcycles && m>1){          
        sendData((performance.now() - start_time).toFixed(10), "Trial Ended", -1);
        j++;
        changeBox();
        }
    }, (2000*index+(Math.random()*150+50)));}
  }
    //flashSequence(_sequence3);
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
      if (e.key === "a") {
        toggleFullScreen();
      }
    },
    false,
  );

addEventListener('DOMContentLoaded', drawArrows());
document.addEventListener('DOMContentLoaded', doTheThing());
document.addEventListener(
  "keydown",
  (e) => {
    if (e.key === "Enter") {
      if (j!= 5){
      flashStuff();
      }
      if (j == 5){
        window.location.href ="/local";
      }
    }
  },
  false,
);
document.getElementById("h").addEventListener("click", switchClick);
