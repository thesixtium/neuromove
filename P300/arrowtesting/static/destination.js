var _root = document.querySelector(':root');
var map = document.getElementById("map");
var dots = document.getElementsByClassName('dot');
var dot1 = document.getElementById("dot1");
var dot2 = document.getElementById("dot2");
var dot3 = document.getElementById("dot3");
var dot4 = document.getElementById("dot4");
var dot5 = document.getElementById("dot5");

function drawDots(){
//dots.style.top = '40%';
//dots.style.left = '20%';
getData();
}

function getData() {
    /*fetch("/dotcoords", {
        method: 'GET',
        headers: {
          'Content-Type': 'application/json'
        },
    success: function(response) {
        dot1.style.x = response[0][0];
        dot1.style.y = response[0][1];
        console.log(response[0]);
    },
    error: function(error) {
        console.log(error);
    }
});*/
      
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
    if (sessionStorage.getItem('centre') == 'true'){
        _root.style.setProperty('--pright', '-10%');
        _root.style.setProperty('--mright', '-10%');
        _root.style.setProperty('--pleft', '10%');
        _root.style.setProperty('--mleft', '10%');}
    
    }

    addEventListener('DOMContentLoaded', doTheThing());
    //addEventListener('DOMContentLoaded', drawDots());
