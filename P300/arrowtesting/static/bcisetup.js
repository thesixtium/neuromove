const topLeft = document.getElementById("tl");
const topRight = document.getElementById("tr");
var r = document.querySelector(':root');

topLeftClick = function() {
    console.log("new user!!");

    window.location.href = "/screenside";
}

topRightClick = function() {

    console.log("existing user!!");

    window.location.href = "/screenside";
}

topLeft.addEventListener("click", topLeftClick);
topRight.addEventListener("click", topRightClick);
