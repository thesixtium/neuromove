const stop = document.getElementById("s");
const cstop = stop.getContext("2d");
const go = document.getElementById("g");
const cgo = go.getContext("2d");

function draw(){
cgo.beginPath();
cgo.moveTo(250, 15);
cgo.lineTo (325, 80);
cgo.lineTo (285, 80);
cgo.lineTo (285, 205);
cgo.lineTo (215, 205);
cgo.lineTo (215, 80);
cgo.lineTo (175, 80);
cgo.lineTo (252, 14);
cgo.lineWidth = 4;
cgo.strokeStyle = "white";
cgo.stroke();
cgo.fillStyle = "white";
cgo.fill();

cstop.beginPath();
cstop.moveTo(200, 10);
cstop.lineTo(300, 10);
cstop.lineTo(365, 60);
cstop.lineTo(365, 160);
cstop.lineTo(300, 210);
cstop.lineTo(200, 210);
cstop.lineTo(135, 160);
cstop.lineTo(135, 60);
cstop.lineTo(200, 10);
/*cstop.moveTo(35, 25);
cstop.lineTo(70, 25);
cstop.lineTo(90, 45);
cstop.lineTo(90, 75);
cstop.lineTo(70, 95);
cstop.lineTo(35, 95);
cstop.lineTo(15, 75);
cstop.lineTo(15, 45);
cstop.lineTo(35, 25); */
cstop.lineWidth = 4;
cstop.strokeStyle = "white";
cstop.stroke();
cstop.fillStyle = "white";
cstop.fill();
}
addEventListener('DOMContentLoaded', draw());