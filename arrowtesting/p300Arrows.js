const _idoptions = ["a", "b", "c", "d", "e", "f", "g", "h", "i"];
const _arrows = ["ca", "cb", "cc", "cd", "ce", "cf", "cg", "ch", "ci"];
const _sequence1 = [];
const _sequence2 = [];
const _sequence3 = [];
const defaultColour = "blue";
function pickSequence(array){
    let temp = [];
    array.forEach(element => {
        temp.push(element);
    });
    console.log("temp end: " + temp[temp.length - 1]);
    array.length = 0;
    let i =0;
    while (i < _idoptions.length){
        let index = Math.floor(Math.random() * _idoptions.length);
        if ((i == 0 && temp[temp.length - 1] == _idoptions[index]) || array.includes(_idoptions[index])){
            continue;
        } else {
            array.push(_idoptions[index]);
            i++;
        }
    }
    console.log("array: " + array[0]);
}
function flashSequence(array){
    //for(let j = 0; j < 3; j++){
        pickSequence(array);
    let i = 0;
    let interval = setInterval(function(){
        if (i < _idoptions.length){
            document.getElementById(array[i]).style.backgroundColor = "salmon";
            setTimeout(function(){
                document.getElementById(array[i]).style.backgroundColor = defaultColour;
            }, 100);
            i++;
        } else {
            clearInterval(interval);
        }
    }, 220);
}
function doTheThing(){
    const l45 = document.getElementById("ca");
    const cl45 = l45.getContext("2d");
    const fwd = document.getElementById("cb");
    const cfwd = fwd.getContext("2d");
    const r45 = document.getElementById("cc");
    const cr45 = r45.getContext("2d");
    const l = document.getElementById("cd");
    const cl = l.getContext("2d");
    const stop = document.getElementById("ce");
    const cstop = stop.getContext("2d");
    const r = document.getElementById("cf");
    const cr = r.getContext("2d");
    const l135 = document.getElementById("cg");
    const cl135 = l135.getContext("2d");
    const bwd = document.getElementById("ch");
    const cbwd = bwd.getContext("2d");
    const r135 = document.getElementById("ci");
    const cr135 = r135.getContext("2d");
    cl45.beginPath();
    cl45.moveTo(85, 100);
    cl45.lineTo(35, 60);
    cl45.lineTo(17, 80);
    cl45.lineTo(20, 30);
    cl45.lineTo(70, 30);
    cl45.lineTo(55, 45);
    cl45.lineTo(85, 100);
    cl45.lineWidth = 4;
    cl45.stroke();
    cl45.fillStyle = "white";
    cl45.fill();

    cfwd.beginPath();
    cfwd.moveTo(40, 95);
    cfwd.lineTo(40, 45);
    cfwd.lineTo(10, 45)
    cfwd.lineTo(50, 5);
    cfwd.lineTo(90, 45);
    cfwd.lineTo(60, 45);
    cfwd.lineTo(60, 95);
    cfwd.lineTo(40, 95);
    cfwd.lineWidth = 4;
    cfwd.stroke();
    cfwd.fillStyle = "white";
    cfwd.fill();

    cr45.beginPath();
    cr45.moveTo(15, 100);
    cr45.lineTo(65, 60);
    cr45.lineTo(83, 80);
    cr45.lineTo(80, 30);
    cr45.lineTo(30, 30);
    cr45.lineTo(45, 45);
    cr45.lineTo(15, 100);
    cr45.lineWidth = 4;
    cr45.stroke();
    cr45.fillStyle = "white";
    cr45.fill();

    cl135.beginPath();
    cl135.moveTo(85, 20);
    cl135.lineTo(35, 60);
    cl135.lineTo(17, 40);
    cl135.lineTo(20, 90);
    cl135.lineTo(70, 90);
    cl135.lineTo(55, 75);
    cl135.lineTo(85, 20);
    cl135.lineWidth = 4;
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
    cbwd.lineWidth = 4;
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
    cr135.lineWidth = 4;
    cr135.stroke();
    cr135.fillStyle = "white";
    cr135.fill();

    cl.beginPath();
    cl.moveTo(90, 70);
    cl.lineTo(40, 70);
    cl.lineTo(40, 90);
    cl.lineTo(5, 60);
    cl.lineTo(40, 30);
    cl.lineTo(40, 50);
    cl.lineTo(90, 50);
    cl.lineTo(90, 70);
    cl.lineWidth = 4;
    cl.stroke();
    cl.fillStyle = "white";
    cl.fill();

    cr.beginPath();
    cr.moveTo(10, 70);
    cr.lineTo(60, 70);
    cr.lineTo(60, 90);
    cr.lineTo(95, 60);
    cr.lineTo(60, 30);
    cr.lineTo(60, 50);
    cr.lineTo(10, 50);
    cr.lineTo(10, 70);
    cr.lineWidth = 4;
    cr.stroke();
    cr.fillStyle = "white";
    cr.fill();

    cstop.beginPath();
    cstop.moveTo(35, 25);
    cstop.lineTo(70, 25);
    cstop.lineTo(90, 45);
    cstop.lineTo(90, 75);
    cstop.lineTo(70, 95);
    cstop.lineTo(35, 95);
    cstop.lineTo(15, 75);
    cstop.lineTo(15, 45);
    cstop.lineTo(35, 25); 
    cstop.lineWidth = 4;
    cstop.stroke();
    cstop.fillStyle = "white";
    cstop.fill();

   //flashSequence(_sequence1);
    //setTimeout(function(){
    for(let i = 0; i<7; i++){
    setTimeout(function(){flashSequence(_sequence1);}, (1800*i));}//}, 2500);
    //flashSequence(_sequence3);
}
//}
addEventListener('DOMContentLoaded', doTheThing);
//src = "test-logic.js"