const topLeft = document.getElementById("tl");
const topRight = document.getElementById("tr");
var r = document.querySelector(':root');

topLeftClick = function() {
    console.log("screen on left!");

    window.location.href = "/setup";
}

topRightClick = function() {
    
    console.log("screen on right!");

    window.location.href = "/setup";
}

topLeft.addEventListener("click", topLeftClick);
topRight.addEventListener("click", topRightClick);
