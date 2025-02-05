const topLeft = document.getElementById("tl");
const topRight = document.getElementById("tr");
const bottomLeft = document.getElementById("bl");
const bottomRight = document.getElementById("br");
const centre = document.getElementById("cen");
var r = document.querySelector(':root');

topLeftClick = function() {
    topLeft.style.backgroundColor = "pink";
    sessionStorage.setItem('location', 'topleft');
    sessionStorage.setItem('pright', '50%');
    sessionStorage.setItem('top', '0%');
    sessionStorage.setItem('mleft', '-20%');
    sessionStorage.setItem('pleft', '0%');
    sessionStorage.setItem('mright', '10%');

    window.location.href = "/local";
}

topRightClick = function() {
    topRight.style.backgroundColor = "pink";   
    sessionStorage.setItem('location', 'topright'); 
    sessionStorage.setItem('pleft', '50%');
    sessionStorage.setItem('top', '0%');
    sessionStorage.setItem('mright', '-20%');
    sessionStorage.setItem('pright', '0%');
    sessionStorage.setItem('mleft', '10%');

    window.location.href = "/local";
}

bottomLeftClick = function() {
    bottomLeft.style.backgroundColor = "pink";
    sessionStorage.setItem('location', 'bottomleft');
    
    sessionStorage.setItem('pright', '50%');
    sessionStorage.setItem('top', '40%');
    sessionStorage.setItem('mleft', '-20%');
    sessionStorage.setItem('pleft', '0%');
    sessionStorage.setItem('mright', '10%');

    window.location.href = "/local";
}

bottomRightClick = function() {
    bottomRight.style.backgroundColor = "pink";
    sessionStorage.setItem('location', 'bottomright');
    
    sessionStorage.setItem('pleft', '50%');
    sessionStorage.setItem('top', '40%');
    sessionStorage.setItem('mright', '-20%');
    sessionStorage.setItem('pright', '0%');
    sessionStorage.setItem('mleft', '10%');

    window.location.href = "/local";
}

centreClick = function() {
    centre.style.backgroundColor = "pink";
    sessionStorage.setItem('location', 'centre');
    sessionStorage.setItem('pright', '0%');
    sessionStorage.setItem('pleft', '0%');
    sessionStorage.setItem('top', '20%');
    sessionStorage.setItem('mleft', '10%');
    sessionStorage.setItem('mright', '10%');
    window.location.href = "/local";
}

topLeft.addEventListener("click", topLeftClick);
topRight.addEventListener("click", topRightClick);
bottomLeft.addEventListener("click", bottomLeftClick);
bottomRight.addEventListener("click", bottomRightClick);
centre.addEventListener("click", centreClick);
