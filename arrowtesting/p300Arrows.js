const _idoptions = [/*"a",*/ "b", /*"c", */"d", "e", "f"/*, "g", "h", "i"*/];
const _arrows = [/*"ca",*/ "cb", /*"cc",*/ "cd", "ce", "cf"/*, "cg", "ch", "ci"*/];
const _sequence1 = [];
const _sequence2 = [];
const _sequence3 = [];

const start_time = Date.now();
/*const l45 = document.getElementById("ca");
l45.width = 100;
l45.height = 100;
const cl45 = l45.getContext("2d");*/
const fwd = document.getElementById("cb");
const cfwd = fwd.getContext("2d");
//const r45 = document.getElementById("cc");
//const cr45 = r45.getContext("2d");
const l = document.getElementById("cd");
const cl = l.getContext("2d");
const stop = document.getElementById("ce");
const cstop = stop.getContext("2d");
const r = document.getElementById("cf");
const cr = r.getContext("2d");
/*const l135 = document.getElementById("cg");
const cl135 = l135.getContext("2d");
const bwd = document.getElementById("ch");
const cbwd = bwd.getContext("2d");
const r135 = document.getElementById("ci");
const cr135 = r135.getContext("2d");*/

const _canvases = [/*cl45,*/ cfwd, /*cr45, */cl, cstop ,cr, /*cl135, cbwd, cr135*/];
const defaultColour = "white";
function pickSequence(array, array2){
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
    //for(let j = 0; j < 3; j++){
        pickSequence(array, array2);
        //array2[0].fillStyle = "black";
        //array2[0].fill()
        console.log("array at flash: " + array);
        let i = 0;
    let interval = setInterval(function(){
        if (i <array2.length){
            
            console.log("start i: " + array[i]);
            console.log(Date.now() - start_time);
            document.getElementById(array[i]).style.backgroundColor = "black";
            array2[i].fillStyle = "white";
            array2[i].fill();
            setTimeout(function(){
                document.getElementById(array[i]).style.backgroundColor = "white";
                array2[i].fillStyle = "black";
                array2[i].fill();
            }, 200);
            console.log("end i: " + array[i]);
            console.log(Date.now() - start_time);
            i++;
          /*  if (array[i] == "ca"){
                l45.width +=15;
                drawArrows();
            }*/
        } else {
            drawArrows();
            clearInterval(interval);
        }
    }, 640);
}

function drawArrows(){
   /* 
    cl45.beginPath();
    cl45.moveTo(85, 100);
    cl45.lineTo(35, 60);
    cl45.lineTo(17, 80);
    cl45.lineTo(20, 30);
    cl45.lineTo(70, 30);
    cl45.lineTo(55, 45);
    cl45.lineTo(85, 100);
    cl45.lineWidth = 10;
    cl45.strokeStyle = "black";
    cl45.stroke();
    //_canvases[0].fillStyle = "pink";
    cl45.fillStyle = "white";
    cl45.fill();
    document.getElementById("a").style.backgroundColor = "black";
*/
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
    document.getElementById("b").style.backgroundColor = "black";
/*
    cr45.beginPath();
    cr45.moveTo(15, 100);
    cr45.lineTo(65, 60);
    cr45.lineTo(83, 80);
    cr45.lineTo(80, 30);
    cr45.lineTo(30, 30);
    cr45.lineTo(45, 45);
    cr45.lineTo(15, 100);
    cr45.lineWidth = 10;
    cr45.stroke();
    cr45.fillStyle = "white";
    cr45.fill();
    document.getElementById("c").style.backgroundColor = "black";

    cl135.beginPath();
    cl135.moveTo(85, 20);
    cl135.lineTo(35, 60);
    cl135.lineTo(17, 40);
    cl135.lineTo(20, 90);
    cl135.lineTo(70, 90);
    cl135.lineTo(55, 75);
    cl135.lineTo(85, 20);
    cl135.lineWidth = 10;
    cl135.stroke();
    cl135.fillStyle = "white";
    cl135.fill();

    cbwd.beginPath();
    cbwd.moveTo(40, 10);
    cbwd.lineTo(40, 60);
    cbwd.lineTo(10, 60)
    cbwd.lineTo(50, 100);
    cbwd.lineTo(90, 60);
    cbwd.lineTo(60, 60);
    cbwd.lineTo(60, 10);
    cbwd.lineTo(40, 10);
    cbwd.lineWidth = 10;
    cbwd.stroke();
    cbwd.fillStyle = "white";
    cbwd.fill();

    cr135.beginPath();
    cr135.moveTo(15, 20);
    cr135.lineTo(65, 60);
    cr135.lineTo(83, 40);
    cr135.lineTo(80, 90);
    cr135.lineTo(30, 90);
    cr135.lineTo(45, 75);
    cr135.lineTo(15, 20);
    cr135.lineWidth = 10;
    cr135.stroke();
    cr135.fillStyle = "white";
    cr135.fill();*/

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
    document.getElementById("d").style.backgroundColor = "black";

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
    document.getElementById("f").style.backgroundColor = "black";

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
    document.getElementById("e").style.backgroundColor = "black";
}
function doTheThing(){
    
//flashSequence(_sequence1, _sequence2);
   //flashSequence(_sequence1);
    //setTimeout(function(){
    console.log("block start: " + Date.now()-start_time);
    for(let i = 0; i<2; i++){
    setTimeout(function(){flashSequence(_sequence1, _sequence2); /*console.log("cycle "+ i+ " start: " + Date.now()-start_time);*/}, (2650*i));}//}, 2500);*/
    console.log("cycle " + i + "end: " + Date.now()-start_time);
    //flashSequence(_sequence3);
    
}
//console.log("block end: " + Date.now()-start_time);
//}
addEventListener('DOMContentLoaded', drawArrows());
addEventListener('DOMContentLoaded', doTheThing());
//src = "test-logic.js"